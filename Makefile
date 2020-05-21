CC=gcc
CFLAGS:=$(shell python3-config --cflags) -fPIC
LDFLAGS:=$(shell python3-config --ldflags)

.PHONY: build

build:
	#@echo $(CFLAGS)
	#$(CC) --shared -o pycallback.so *.c $(CFLAGS) $(LDFLAGS)
	python3 setup.py build
	find build -name '*.so' -exec cp {} . \;    
