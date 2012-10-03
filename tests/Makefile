CFLAGS+=-I../include -I../deps

LIB_SRC=../src/spdr-common.c
LIB_SRC+=../deps/timer_lib/timer.c
LIB=libspdr.a

PLATFORM?=posix

ifeq ($(PLATFORM),posix)
LIB_SRC+=../src/spdr-posix.c
LIB_SRC+=../src/spdr-thread-posix.c
endif

ifeq ($(PLATFORM),macosx)
LIB_SRC+=../src/spdr-posix.c
LIB_SRC+=../src/spdr-thread-posix.c
LIB_SRC+=../src/spdr-macosx.c
endif

ifeq ($(PLATFORM),linux)
LIB_SRC+=../src/spdr-posix.c
LIB_SRC+=../src/spdr-linux.c
endif

ifeq ($(PLATFORM),win32)
LIB_SRC+=../src/spdr-windows.c
endif

ifeq ($(PLATFORM),win64)
LIB_SRC+=../src/spdr-windows.c
endif

all: without-tracing with-tracing mt-with-tracing mt-without-tracing cxx-tracing
clean:
	rm -f *-tracing $(LIB)

$(LIB): $(LIB_SRC:%.c=%.o)
	ar rcs $@ $^

mt-with-tracing with-tracing scope-tracing cxx-tracing: \
	CFLAGS+=-DTRACING_ENABLED=1

with-tracing: test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

without-tracing: test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

mt-with-tracing: test-mt.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

mt-without-tracing: test-mt.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

cxx-tracing: test-cxx.cc $(LIB)
	$(CXX) $(CFLAGS) -L. -lspdr $< -o $@

scope-tracing: test-scope.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@
