# pyrtsignal
Real time signals (SIGRT) for Python
Ad hoc python library for handling real time signals

## Functions
* set_sighandler - Set real time singal handler in python code. Call (int signal_number, pyfunc handler), handler is (int signum, int value)
* test_sighandler - Test real time hadler callback. Call (int signum, int value)
* send_signal - Send realtime signal. Call (int signum, int value)
* suspend_for_signal - Wait for real time signal. Call (int signum)
