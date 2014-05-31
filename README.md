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

Check include/spdr/spdr.h it is hopefully small enough to be sufficient as documentation.

Check examples/ for some typical uses.

### Tracing

```C
    SPDR_BEGIN1(spdr, "Main", "printf", SPDR_STR("format", "Hello, world."));
    printf("Hello, world\n");
    SPDR_END(spdr, "Main", "printf");
```

With:

```C
    SPDR_BEGIN1(context, category, trace_name, argument);
    SPDR_END(context, category, trace_name);
```

This will trace the printf call and associate a textual argument to
it.

Argument keys, categories and names are meant to be litteral or last for the duration of your instrument program, so as to limit their number. Traces where these values would grow with time would become hard to read and understand. Any time varying data should be kept in argument values (such as the "Hello, world." string above)

The library will also keep direct pointers to them for performance reasons, so make sure these strings are valid for the entire use of the library.

The string arguments however are copied so you may deallocate them. This is were your time-varying data should be stored.

### Reporting

Reporting may be done as the application is running or with less
overhead at the end of a tracing period, using the reporting
function. You are expected to provide I/O functions to allow the
library to write into a file/console or even produce a network stream.

For instance the following example will produce a report into
traces.json that can then be directly loaded in Chrome's tracing
console.

```C
static void report(const char* line, void* user_data)
{
    FILE* file = user_data;

    fputs(line, file);
}

static void foo()
{
    /* ... */

    FILE* f = fopen("traces.json", "wb");
    if (f) {
        spdr_report(gbl_spdr, SPDR_CHROME_REPORT, report, f);
        fclose(f);
    }
```

## Compilation

This implementation should be comformant to ANSI C90, although it
requires C99's integer types. It does not use external dependencies
besides a system's headers. This should let you compile on most
platforms.

Tested platforms as of 2012-11:
*     Linux, GCC
*     MacOSX, GCC and Clang
*     Windows, Visual Studio 2012

Sources to include for each platforms may be found in:
*     src/src-list-linux.txt (you need -lrt to link)
*     src/src-list-osx.txt
*     src/src-list-win32.txt
*     src/src-list-win64.txt

Dropping the implementation files inside src/ for your platform in a
project and importing the header found in include/ should basically be
sufficient.

Use of the library in C++ is supported, you must include the
include/spdr/spdr.hh header rather than spdr.h.

## Implementation & Design

### Allocation

During tracing, the library does not do any allocation of its own, and
uses a simple linear allocator. This allows you to control precisely
how much memory is reserved for the tracing of events and reduces
overhead.

Arguments and traces are allocated on this pre-allocated memory arena.

During reporting, some dynamic allocations may be performed.

### Threads

Use from multiple thread is supported and expected. Lock-free atomic
operations are used thanks to Hans Boehm's atomic_ops library.

Traces are stored by linear allocation into a set of buckets so that
concurrent accesses are as independant as possible. The number of
buckets used is set at compile-time, and defaults to 8.

### Clocks

Tracing requires a monotonic clock with microsecond precision.

*     QueryPerformanceCounter is used for windows.
*     CLOCK_MONOTONIC is used for Posix platforms, such as Linux.
*     The native mach clock is used for MacOSX.

You may specify your own clock/timer function to return the current
elapsed microseconds, overriding the default internal one.

See:
```C
void spdr_set_clock_microseconds_fn(struct SPDR_Context *context,
             unsigned long long (*clock_microseconds_fn)(void* user_data),
             void *user_data);
```
## Additional Contributors

*     questor <questor@unseen-academy.de>

## Third-party code

The following third-party code are used:

### Hans Boehm's atomic_ops library 7.2d

Downloaded from: http://www.hpl.hp.com/research/linux/atomic_ops/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

### Bjoern Hoehrmann's Flexible and Economical UTF-8 Decoder

Downloaded from: http://bjoern.hoehrmann.de/utf-8/decoder/dfa/

Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### MurmurHash3 by Austin Appleby

Downloaded from: https://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp

MurmurHash3 was written by Austin Appleby, and is placed in the public
domain. The author hereby disclaims copyright to this source code.
