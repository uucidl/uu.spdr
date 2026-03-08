clang -g -O2 -o output\test2.exe -std=c11 test2\test2.c
IF %ERRORLEVEL% NEQ 0 exit /b 1

output\test2.exe

goto end

cl -std:c11 -experimental:c11atomics src\spdr2.c  -nologo
cl -O2 -Fe:output\stest.exe tests/spdr_timing_tests.c ^
   -DEBUG -Z7
:end
