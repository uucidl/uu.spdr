// @language: c99

#include <stdio.h>

static void calibration_nop32();

int main(int argc, char** argv)
{
    --argc;
    argv++;
    if (argc == 0) { fprintf(stderr, "Error: missing n argument.\n"); return 1; }
    int x = atoi(*argv);
    while (x--) {
        calibration_nop32();
    }
}


#if defined(_MSC_VER)
#include <intrin.h>

#pragma optimize("", off)
static void calibration_nop32()
{
    __nop(); // 32
    __nop(); // 31
    __nop(); // 30
    __nop(); // 29
    __nop(); // 28
    __nop(); // 27
    __nop(); // 26
    __nop(); // 25
    __nop(); // 24
    __nop(); // 23
    __nop(); // 22
    __nop(); // 21
    __nop(); // 20
    __nop(); // 19
    __nop(); // 18
    __nop(); // 17
    __nop(); // 16
    __nop(); // 15
    __nop(); // 14
    __nop(); // 13
    __nop(); // 12
    __nop(); // 11
    __nop(); // 10
    __nop(); //  9
    __nop(); //  8
    __nop(); //  7
    __nop(); //  6
    __nop(); //  5
    __nop(); //  4
    __nop(); //  3
    __nop(); //  2
    __nop(); //  1
}
#pragma optimize("", on)

#endif



