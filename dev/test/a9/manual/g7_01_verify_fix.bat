@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "ROOT=..\..\..\.."
set "TCC=%ROOT%\dev\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "CPPUNIT=%ROOT%\sample\cppunit"
set "OUT=%TEMP%\g7_01_verify_fix"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAIL=0

call :run_case MIN_REPRO g7_01_inline_strip_main.cpp
call :run_case N6_05_CRASH n6_05_no_winapi.cpp
call :run_cppunit_compile TestCase.cpp
call :run_cppunit_compile SimpleString.cpp

echo === G7-01-FIX verify done failures=!FAIL! ===
popd
exit /b !FAIL!

:run_case
set "LABEL=%~1"
set "SRC=%~2"
"%TCC%" -w "%SRC%" -o "%OUT%\%LABEL%.exe" >"%OUT%\%LABEL%.log" 2>&1
if errorlevel 1 (
    echo %LABEL%=FAIL
    type "%OUT%\%LABEL%.log" | findstr /i "error:"
    set /a FAIL+=1
    goto :eof
)
echo %LABEL%=PASS
goto :eof

:run_cppunit_compile
set "LABEL=%~1"
set "SRC=%CPPUNIT%\%LABEL%"
"%TCC%" -c -DMINIMUM_SET -Dcu_NO_EXPLICIT -I "%CPPUNIT%" -w "%SRC%" -o "%OUT%\%LABEL%.o" >"%OUT%\%LABEL%.log" 2>&1
if errorlevel 1 (
    echo %LABEL%_COMPILE=FAIL
    type "%OUT%\%LABEL%.log" | findstr /i "error:"
    set /a FAIL+=1
    goto :eof
)
echo %LABEL%_COMPILE=PASS
goto :eof
