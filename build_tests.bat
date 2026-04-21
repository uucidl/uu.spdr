@echo off
setlocal
REM This script must be run after vcvarsall.bat has been run,
REM so that cl.exe is in your path.
where cl.exe || goto error_end_nocompiler

REM HEREPATH is <drive_letter>:<script_directory>
set HEREPATH=%~d0%~p0
set TOP=%HEREPATH%

REM Get canonical path for TOP
pushd .
cd %TOP%
set TOP=%CD%
popd

IF not defined BUILD set BUILD=%TOP%\output

REM Start with the most strict warning level
set WARNINGS=-W4 -WX
REM ignore unknown pragmas
set WARNINGS=%WARNINGS% -wd4068
set CLFLAGS=%CLFLAGS% -nologo -FC -EHsc -Z7 -Oi -GR- -Gm- -O2 -Z7 %DEFINES% %WARNINGS% -DEBUG

mkdir %BUILD%
del /Q %BUILD%
set D=%BUILD%\spdr_basic_tests.exe
cl.exe -Fe:%D% ^
  %CLFLAGS% %TOP%\tests\spdr_basic_tests.c
if %errorlevel% neq 0 goto error_end_build
echo TEST	%D%
%D%
if %errorlevel% neq 0 goto error_end_test

REM Test that spdr can pass preprocessing:
REM
REM preprocess using -P and check if it compiles
REM
REM recent libatomic_ops include code like unsigned/**/char
REM which trips the MSVC preprocessor, as it does not appear
REM to follow the standard, and does not generate a space in that case.
REM
set D=%BUILD%\preprocessed.i
cl.exe -Fi:%D% ^
  -P ^
  %CLFLAGS% %TOP%\tests\spdr_basic_tests.c
if %errorlevel% neq 0 goto error_end_build
cl.exe -nologo -Zs -Tc %BUILD%\preprocessed.i
if %errorlevel% neq 0 goto error_end_build
echo TEST	%D%

endlocal
exit /b 0

REM Error cases
REM ===========

:error_end_nocompiler
echo ERROR: CL.EXE missing. Have you run vcvarsall.bat?
endlocal
exit /b 1

:error_end_build
echo ERROR: Failed to build %D%
endlocal
exit /b %errorlevel%

:error_end_test
echo ERROR: Test %D% failed
endlocal
exit /b %errorlevel%



