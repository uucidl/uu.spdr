CFLAGS+=-I../include -I../deps
CFLAGS+=-Wall -Wextra -Wno-missing-field-initializers

LIB_SRC=../src/spdr-common.c
LIB_SRC+=../src/chars-common.c
LIB_SRC+=../src/allocator-common.c
LIB_SRC+=../src/clock-common.c
LIB=libspdr.a


GUESSED_PLATFORM=posix
ifeq ($(shell uname),Darwin)
GUESSED_PLATFORM=osx
endif


PLATFORM?=$(GUESSED_PLATFORM)

ifeq ($(PLATFORM),posix)
LIB_SRC+=../src/spdr-posix.c
LIB_SRC+=../src/chars-posix.c
LIB_SRC+=../src/spdr-thread-posix.c
LIB_SRC+=../src/clock-posix.c
LIB_SRC+=../src/float-posix.c
LIB_LIBS+=-lrt
endif

ifeq ($(PLATFORM),osx)
LIB_SRC+=../src/spdr-posix.c
LIB_SRC+=../src/chars-posix.c
LIB_SRC+=../src/spdr-thread-posix.c
LIB_SRC+=../src/clock-osx.c
LIB_SRC+=../src/float-posix.c
endif

ifeq ($(PLATFORM),linux)
LIB_SRC+=../src/spdr-posix.c
LIB_SRC+=../src/chars-posix.c
LIB_SRC+=../src/spdr-linux.c
LIB_SRC+=../src/float-posix.c
endif

ifeq ($(PLATFORM),win32)
LIB_SRC+=../src/spdr-windows.c
LIB_SRC+=../src/clock-windows.c
LIB_SRC+=../src/chars-windows.c
LIB_SRC+=../src/float-windows.c
endif

ifeq ($(PLATFORM),win64)
LIB_SRC+=../src/spdr-windows.c
LIB_SRC+=../src/chars-windows.c
LIB_SRC+=../src/clock-windows.c
LIB_SRC+=../src/float-windows.c
endif

CFLAGS+=-lm $(LIB_LIBS)

all: without-tracing with-tracing mt-with-tracing mt-without-tracing cxx-tracing perf-tracing perf-without-tracing
clean:
	rm -f *-tracing $(LIB)

$(LIB): $(LIB_SRC:%.c=%.o)
	ar rcs $@ $^

mt-with-tracing with-tracing scope-tracing cxx-tracing perf-tracing: \
	CFLAGS+=-DTRACING_ENABLED=1

with-tracing: test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

without-tracing: test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

mt-with-tracing: test-mt.c $(LIB)
	$(CC) $(CFLAGS) -L. -lspdr $^ -o $@

mt-without-tracing: test-mt.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

cxx-tracing: test-cxx.cc $(LIB)
	$(CXX) $(CFLAGS) $< -o $@ -L. -lspdr

scope-tracing: test-scope.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

tracing-with-external-clock: ../deps/timer_lib/timer.c test-external_clock.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

perf-tracing: perf-test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

perf-without-tracing: perf-test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@
