# SPDR

Allow you to instrument code with traces.

## License

> Copyright (C) 2012 Nicolas Léveillé <nicolas@uucidl.com>

> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Description

This library offers you a series of macros supported by a C-based implementation to introduce timed traces inside your code. They may be used to debug your program and its threads and overwise measure the performance of groups of operations.

The library also reports its results into a JSON format compatible with the tracing user interface found in Google Chrome. These reports may be loaded inside chrome by going to chrome://tracing, which opens a dedicated interface for trace viewing.

The implementation is inspired by chrome's tracing capabilities yet was entirely written independently.

See:
    http://www.altdevblogaday.com/2012/08/21/using-chrometracing-to-view-your-inline-profiling-data/

## Usage

Check include/spdr.h it is hopefully small enough to be sufficient as documentation.

Check examples/ for some typical uses.

### Tracing

```C
    SPDR_BEGIN1(spdr, "Main", "printf", SPDR_STR("format", "Hello, world."));
    printf("Hello, world\n");
    SPDR_END(spdr, "Main", "printf");
```

This will trace the printf call and associate a textual argument to
it. The string arguments are copied so you may deallocate them,
however all the keys and categories should be litterals or kept for
the whole use of the library. The library will keep direct pointers to
them for performance reasons.

### Reporting

Reporting may be done as the application is running or with less
overhead at the end of a tracing period, using the reporting
function. You are expected to provide I/O functions to allow the
library to write into a file/console or even produce a network stream.

## Compilation

This implementation should be comformant to ANSI C90, although it
requires C99's integer types. It does not use external dependencies
besides a system's headers. This should let you compile on most
platforms.

Tested platforms as of 2012-11:
*     Linux, GCC
*     MacOSX, GCC and Clang
*     Windows, Visual Studio 2012

Dropping the implementation files inside src/ for your platform in a
project and importing the header found in include/ should basically be
sufficient.

Use of the library in C++ is supported, you must include the
include/spdr.hh header rather than spdr.h.

## Implementation & Design

### Allocation

The library does not do any allocation of its own, and uses a simple linear allocator. This allows you to control precisely how much memory is reserved for the tracing of events and reduces overhead.

Arguments and traces are allocated on this pre-allocated memory arena.

### Threads

Use from multiple thread is supported and expected. Lock-free atomic
operations are used thanks to Hans Boehm's atomic_ops library.

### Clocks

Tracing requires a monotonic clock with microsecond precision.

*     QueryPerformanceCounter is used for windows.
*     CLOCK_MONOTONIC is used for Posix platforms, such as Linux.
*     The native mach clock is used for MacOSX.

You may specify your own clock/timer function to return the current
elapsed microseconds, overriding the default internal one.

See:
```C
void spdr_set_clock_microseconds_fn(struct spdr *context,
		     unsigned long long (*clock_microseconds_fn)(void* user_data),
		     void *user_data);
```
