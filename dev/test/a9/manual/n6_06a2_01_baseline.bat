@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_06a2_01"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-06A2-01: test-only baseline (items 9/10) ===
echo BASE_COMMIT=e650873
echo PRODUCTION_CHANGE=NONE
echo.

call :run n6_06a2_01_main_isolation MAIN_TLS_SURVIVES_WORKER_JOIN
call :run n6_06a2_01_multi_worker MULTI_WORKER_TLS_ISOLATION

if not "!FAILED!"=="0" (
  echo N6_06A2_01_BASELINE=FAIL
  popd
  exit /b 1
)
echo N6_06A2_01_BASELINE=PASS
echo MAIN_DRAINS_WORKER_TLS=NO
popd
exit /b 0

:run
set "SRC=%~1.cpp"
set "MARK=%~2"
set "LOG=%OUT%\%~1.log"
"%TCC%" -run "%SRC%" >"%LOG%" 2>&1
set "RC=!errorlevel!"
type "%LOG%"
if not "!RC!"=="0" (
  echo %MARK%=RUN_FAIL rc=!RC!
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"%MARK%=PASS" "%LOG%" >nul 2>&1
if errorlevel 1 (
  echo %MARK%=OUTPUT_FAIL
  set /a FAILED+=1
  goto :eof
)
echo %MARK%=PASS
goto :eof
