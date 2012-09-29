CFLAGS+=-I../include -I../deps
LIB_SRC=../src/*-common.c ../deps/timer_lib/timer.c

all: without-tracing with-tracing

with-tracing: CFLAGS+=-DTRACING_ENABLED=1
with-tracing: test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@

without-tracing: test.c $(LIB_SRC)
	$(CC) $(CFLAGS) $^ -o $@
