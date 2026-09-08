@echo off
setlocal
set "ROOT=%~dp0..\..\..\.."
pushd "%ROOT%"
if errorlevel 1 (
    echo DISCRIM_FAIL=pushd
    exit /b 1
)

set "TCC=%CD%\dev\tcc.exe"
if not exist "%TCC%" set "TCC=%CD%\x64\Release\tcc.exe"
if not exist "%TCC%" (
    echo DISCRIM_FAIL=no_tcc at %TCC%
    exit /b 1
)
set OUT=%CD%\dev\test\a9\manual\disc_out
if not exist "%OUT%" mkdir "%OUT%"

call :run_case A simple inject_off dev\test\a9\manual\p0_disc_simple.cpp
call :run_case B synthetic inject_off dev\test\a9\manual\p0_header_defined_class.cpp
call :run_case C simple inject_force_n6 dev\test\a9\manual\p0_disc_simple.cpp
call :run_case D synthetic inject_force_n6 dev\test\a9\manual\p0_header_defined_class.cpp
call :run_case E simple inject_force_tls dev\test\a9\manual\p0_disc_simple.cpp
call :run_case F synthetic inject_force_tls dev\test\a9\manual\p0_header_defined_class.cpp

echo === DISCRIMINATION SUMMARY ===
exit /b 0

:run_case
set CASE=%1
set KIND=%2
set DIAG=%3
set SRC=%4
set "SRC=%CD%\%SRC%"
set BASE=%OUT%\%CASE%_%KIND%

echo.
echo === CASE %CASE% (%KIND% TCC_CRASH_DIAG=%DIAG%) FULL ===
set TCC_CRASH_DIAG=%DIAG%
"%TCC%" "%SRC%" -o "%BASE%.exe"
set FULL_EC=%ERRORLEVEL%
if %FULL_EC% LSS 0 (
    echo CASE_%CASE%_FULL=CRASH exit=%FULL_EC%
) else if %FULL_EC% GTR 0 (
    echo CASE_%CASE%_FULL=FAIL exit=%FULL_EC%
) else (
    echo CASE_%CASE%_FULL=PASS
    "%BASE%.exe"
    echo CASE_%CASE%_RUN=exit_%ERRORLEVEL%
)

echo === CASE %CASE% SPLIT ===
set TCC_CRASH_DIAG=%DIAG%
"%TCC%" -c "%SRC%" -o "%BASE%.o"
set SPLIT_C=%ERRORLEVEL%
if %SPLIT_C% NEQ 0 (
    echo CASE_%CASE%_SPLIT_COMPILE=FAIL exit=%SPLIT_C%
    goto :eof
)
"%TCC%" "%BASE%.o" -o "%BASE%_split.exe"
set SPLIT_L=%ERRORLEVEL%
if %SPLIT_L% LSS 0 (
    echo CASE_%CASE%_SPLIT_LINK=CRASH exit=%SPLIT_L%
) else if %SPLIT_L% GTR 0 (
    echo CASE_%CASE%_SPLIT_LINK=FAIL exit=%SPLIT_L%
) else (
    echo CASE_%CASE%_SPLIT=PASS
    "%BASE%_split.exe"
    echo CASE_%CASE%_SPLIT_RUN=exit_%ERRORLEVEL%
)
goto :eof
