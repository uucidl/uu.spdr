CFLAGS+=-I../include -I../deps

LIB_SRC=../src/*-common.c ../deps/timer_lib/timer.c

PLATFORM?=posix

ifeq ($(PLATFORM),posix)
LIB_SRC+=../src/*-posix.c
endif

ifeq ($(PLATFORM),macosx)
LIB_SRC+=../src/*-posix.c
LIB_SRC+=../src/*-macosx.c
endif

ifeq ($(PLATFORM),linux)
LIB_SRC+=../src/*-posix.c
LIB_SRC+=../src/*-linux.c
endif

ifeq ($(PLATFORM),win32)
LIB_SRC+=../src/*-windows.c
endif

ifeq ($(PLATFORM),win64)
LIB_SRC+=../src/*-windows.c
endif

all: without-tracing with-tracing mt-with-tracing mt-without-tracing
clean: 
	rm -f *-tracing

mt-with-tracing with-tracing scope-tracing: CFLAGS+=-DTRACING_ENABLED=1
with-tracing: test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

without-tracing: test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

mt-with-tracing: test-mt.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

mt-without-tracing: test-mt.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

scope-tracing: test-scope.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@
