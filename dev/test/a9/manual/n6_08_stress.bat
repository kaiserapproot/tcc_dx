@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_08_stress"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-08: thread churn and termination stress ===
echo BASE_COMMIT=a012ec7
echo N6_08_00_COMMIT=92c12e3
echo.

call :exe_positive n6_08_normal_exe_churn NORMAL_EXE_THREAD_CHURN
call :run_positive n6_08_tcc_run_churn TCC_RUN_THREAD_CHURN
call :run_positive n6_08_mixed_termination MIXED_WORKER_TERMINATION_STRESS
call :run_positive n6_08_main_worker_isolation MAIN_WORKER_ISOLATION_STRESS

if not "!FAILED!"=="0" (
  echo N6_08_STRESS=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N6_08_STRESS=PASS
popd
exit /b 0

:exe_positive
set "NAME=%~1"
set "MARK=%~2"
set "LOG=%OUT%\%NAME%.log"
"%TCC%" %NAME%.cpp -o "%OUT%\%NAME%.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo %MARK%=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"%OUT%\%NAME%.exe" >"%LOG%" 2>&1
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

:run_positive
set "NAME=%~1"
set "MARK=%~2"
set "LOG=%OUT%\%NAME%.log"
"%TCC%" -run "%NAME%.cpp" >"%LOG%" 2>&1
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
