@echo off
setlocal
set "ROOT=%~dp0..\..\..\.."
pushd "%ROOT%"

set "TCC=%CD%\dev\tcc.exe"
if not exist "%TCC%" set "TCC=%CD%\x64\Release\tcc.exe"

set CASE=%1
if "%CASE%"=="" set CASE=D
if "%CASE%"=="D" (
    set DIAG=inject_force_n6_trace
    set SRC=%CD%\dev\test\a9\manual\p0_header_defined_class.cpp
    set OUT=%CD%\dev\test\a9\manual\disc_out\capture_D.exe
) else if "%CASE%"=="F" (
    set DIAG=inject_force_tls_trace
    set SRC=%CD%\dev\test\a9\manual\p0_header_defined_class.cpp
    set OUT=%CD%\dev\test\a9\manual\disc_out\capture_F.exe
) else if "%CASE%"=="N6" (
    set DIAG=trace
    set SRC=%CD%\dev\test\a9\manual\n6_05_auto_vs_tls.cpp
    set OUT=%CD%\dev\test\a9\manual\disc_out\capture_N6.exe
) else (
    echo usage: p0_crash_capture.bat [D^|F^|N6]
    exit /b 1
)

echo === CRASH CAPTURE CASE %CASE% TCC_CRASH_DIAG=%DIAG% ===
set TCC_CRASH_DIAG=%DIAG%
"%TCC%" "%SRC%" -o "%OUT%" 1>"%CD%\dev\test\a9\manual\disc_out\capture_%CASE%.stdout" 2>"%CD%\dev\test\a9\manual\disc_out\capture_%CASE%.stderr"
set EC=%ERRORLEVEL%
echo CAPTURE_EXIT=%EC%
type "%CD%\dev\test\a9\manual\disc_out\capture_%CASE%.stderr"
exit /b %EC%
