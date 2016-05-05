@echo off
setlocal

REM This script must be run after vcvarsall.bat has been run,
REM so that cl.exe is in your path.
where cl.exe || goto vsmissing_err

REM HEREPATH is <drive_letter>:<script_directory>
set HEREPATH=%~d0%~p0
set TOP=%HEREPATH%

REM Get canonical path for TOP
pushd .
cd %TOP%
set TOP=%CD%
popd

set SRC=%TOP%\src
set BUILD=%TOP%\output

REM Start with the most strict warning level
set WARNINGS=-W4 -WX
set DEFINES=-DTRACING_ENABLED=1
set CLFLAGS=%CLFLAGS% -nologo -FC -EHsc -Z7 -Oi -GR- -Gm- -Od -Z7 %DEFINES% %WARNINGS% -Debug

echo SRC=%SRC%, BUILD=%BUILD%
echo Building SPDR examples with flags: %CLFLAGS%

mkdir %BUILD%
del /Q %BUILD%\

set EXAMPLES=%TOP%\examples
set SPDR=-I "%TOP%\include" %TOP%\src\spdr_win32_unit.c

cl.exe %CLFLAGS% %SPDR% %EXAMPLES%\test.c -Fe%BUILD%\test.exe
if %errorlevel% neq 0 goto err
cl.exe %CLFLAGS% %SPDR% -Tp %EXAMPLES%\test-cxx.cc -Fe%BUILD%\test-cxx.exe
if %errorlevel% neq 0 goto err

echo SUCCESS: Successfully built
endlocal
exit /b 0

:vsmissing_err
echo ERROR: CL.EXE missing. Have you run vcvarsall.bat?
exit /b 1

:err
endlocal
echo ERROR: Failed to build
exit /b %errorlevel%
