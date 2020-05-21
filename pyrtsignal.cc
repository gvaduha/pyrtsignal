#include <stdio.h>
#include <signal.h>
#include <Python.h>
#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>


static void call_sighandler(int signal, int val);

// RT signalling code
void rt_handler(int signal, siginfo_t *info, void *arg __attribute__ ((__unused__))) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    sigprocmask(SIG_BLOCK, &mask, 0);

fprintf(stderr, "----------> got signal");
    int val = info->si_value.sival_int;
    call_sighandler(signal, val);

    sigprocmask(SIG_UNBLOCK, &mask, 0);
}

int subscribe_signal(int signal, bool unsubscribe = false) {
    struct sigaction action;

    if (unsubscribe) {
	    action.sa_flags = SA_RESETHAND;
	    action.sa_handler = SIG_DFL;
    }
    else {
	    action.sa_flags = SA_SIGINFO;
	    action.sa_sigaction = rt_handler;
    }
    sigemptyset(&action.sa_mask);
    return sigaction(signal, &action, 0); 
}

int send_rt_signal(pid_t pid, int signal, int value) {
    union sigval sivalue;
    sivalue.sival_int = value;

    //fprintf(stderr, "===> sending rt singal to %d: #%d with %d\n", pid,signal, value);
    int res = sigqueue(pid, signal, sivalue);
    return res; 
}

// Python wrapper code
static PyObject *sighandlers_map[32];

static PyObject* set_sighandler(PyObject *self, PyObject *args) {
    PyObject *temp;
    int signal;

    if (PyArg_ParseTuple(args, "iO:set_sighandler", &signal, &temp)) {
        if (signal < 0 || signal > 31) {
            PyErr_SetString(PyExc_TypeError, "signal number should be in 0-31 range");
            return 0;
        }
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable (int,int) function");
            return 0;
        }
        Py_XINCREF(temp);
        Py_XDECREF(sighandlers_map[signal]);
        sighandlers_map[signal] = temp;

	subscribe_signal(SIGRTMIN+signal);
    }
    Py_RETURN_NONE;
}

static PyObject* test_sighandler(PyObject *self, PyObject *args) {
	PyObject *arglist, *result;
	int signal, val;
	if (PyArg_ParseTuple(args, "ii", &signal, &val)) {
        arglist = Py_BuildValue("ii", signal, val);
        if (signal < 0 || signal > 31) {
            PyErr_SetString(PyExc_TypeError, "signal number should be in 0-31 range");
            return 0;
        }
        if (0 == sighandlers_map[signal]) {
            PyErr_SetString(PyExc_TypeError, "signal handler is null for this signal");
            return 0; /* Pass error back */
	    }
       	result = PyEval_CallObject(sighandlers_map[signal], arglist);
        Py_DECREF(arglist);
        if (result == 0) {
            PyErr_SetString(PyExc_TypeError, "handler execution result is null");
            return 0; /* Pass error back */
	    }
        Py_DECREF(result);
	}
	Py_RETURN_NONE;
}

static PyObject* send_signal(PyObject *self, PyObject *args) {
    long pid;
	int signal, val;
	if (PyArg_ParseTuple(args, "lii", &pid, &signal, &val)) {
        if (signal < 0 || signal > 31) {
            PyErr_SetString(PyExc_TypeError, "signal number should be in 0-31 range");
            return 0;
        }
        if (send_rt_signal((pid_t)pid, SIGRTMIN+signal, val) < 0) {
            PyErr_SetString(PyExc_TypeError, strerror(errno));
            return 0;
        }
	}
	Py_RETURN_NONE;
}

static void call_sighandler(int signal, int val) {
	PyObject *arglist, *result;
    signal = signal - SIGRTMIN;
    arglist = Py_BuildValue("ii", signal, val);
    if (0 == sighandlers_map[signal]) {
        //TODO: call py error handler. install error handler with set_errorhandler
        //fprintf(stderr, "===> CALL NO!!! SIGHANDLER: %d=%d\n", signal, val);
        return;
    }
    //fprintf(stderr, "===> BEFORE!!! CALL SIGHANDLER: %d=%d (%lx) args:(%lx)\n", signal, val, sighandlers_map[signal], arglist);
   	result = PyEval_CallObject(sighandlers_map[signal], arglist);
    //fprintf(stderr, "===> AFTER!!! CALL SIGHANDLER: %d=%d (%lx)\n", signal, val, sighandlers_map[signal]);

    Py_DECREF(arglist);
    if (result == 0) {
        //TODO: call py error handler. install error handler with set_errorhandler
        //fprintf(stderr, "===> ERROR!!! CALL SIGHANDLER: %d=%d\n", signal, val);
        return;
    }
    Py_DECREF(result);
}

static PyObject* suspend_for_signal(PyObject *self, PyObject *args) {
    int signal;
	if (PyArg_ParseTuple(args, "i", &signal)) {
        if (signal < 0 || signal > 31) {
            PyErr_SetString(PyExc_TypeError, "signal number should be in 0-31 range");
            return 0;
        }
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, signal);
        if (sigsuspend(&mask) < 0 && errno != EINTR) {
            PyErr_SetString(PyExc_TypeError, strerror(errno));
            return 0;
        }
	}
	Py_RETURN_NONE;
}

static PyMethodDef mod_methods[] = { 
    {   
        "set_sighandler", set_sighandler, METH_VARARGS,
        "Set real time singal handler in python code. Call (int signal_number, pyfunc handler), handler is (int signum, int value)"
    },  
    {
        "test_sighandler", test_sighandler, METH_VARARGS,
        "Test real time hadler callback. Call (int signum, int value)"
    },  
    {
        "send_signal", send_signal, METH_VARARGS,
        "Send realtime signal. Call (int signum, int value)"
    },  
    {
        "suspend_for_signal", suspend_for_signal, METH_VARARGS,
        "Wait for real time signal. Call (int signum)"
    },  
    {0, 0, 0, 0}
};

static struct PyModuleDef mod_definition = { 
    PyModuleDef_HEAD_INIT,
    "pyrtsignal",
    "A Python real time signals extension module.",
    -1, 
    mod_methods
};

PyMODINIT_FUNC PyInit_pyrtsignal(void) {
    memset(&sighandlers_map, 0, sizeof(PyObject*)*32);
    Py_Initialize();
    PyObject *m = PyModule_Create(&mod_definition);
    /* Adding module globals */
    if (PyModule_AddIntConstant(m, "SIGRTMIN", 0)) {
        goto except;
    }
    if (PyModule_AddIntConstant(m, "SIGRTMAX", 31)) {
        goto except;
    }
    goto finally;
except:
    Py_XDECREF(m);
    m = 0;
finally:
    return m;
}
