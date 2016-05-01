CFLAGS+=-I../include -I../deps
CFLAGS+=-Wall -Wextra

GUESSED_PLATFORM=posix
ifeq ($(shell uname),Darwin)
GUESSED_PLATFORM=osx
endif
PLATFORM?=$(GUESSED_PLATFORM)

LIB_SRC=../src/spdr_$(PLATFORM)_unit.c
LIB=libspdr.a

CFLAGS+=-lm $(LIB_LIBS)

all: without-tracing with-tracing mt-with-tracing mt-without-tracing \
  cxx-tracing perf-tracing perf-without-tracing filling-tracing
clean:
	rm -f *-tracing $(LIB) ../src/*.o

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

cxx-tracing: CFLAGS+=-std=c++11
cxx-tracing: test-cxx.cc $(LIB)
	$(CXX) $(CFLAGS) $< -o $@ -L. -lspdr

scope-tracing: test-scope.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

tracing-with-external-clock: ../deps/timer_lib/timer.c \
  test-external_clock.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

perf-tracing: perf-test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

perf-without-tracing: perf-test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

filling-tracing: test-full.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@
