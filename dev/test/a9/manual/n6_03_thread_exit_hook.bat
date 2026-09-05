@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_03_thread_exit_hook"
if not exist "%OUT%" mkdir "%OUT%"
set "LOG=%OUT%\n6_03_thread_exit_hook.log"

echo === N6-03: thread-exit hook (PE TLS callback) in a tcc-built EXE ===
"%TCC%" n6_03_thread_exit_hook.cpp -o "%OUT%\n6_03_thread_exit_hook.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo N6_03_THREAD_EXIT_HOOK=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\n6_03_thread_exit_hook.exe" >"%LOG%" 2>&1
set "RUN_RC=!errorlevel!"
type "%LOG%"
if not "!RUN_RC!"=="0" (
  echo N6_03_THREAD_EXIT_HOOK=RUN_FAIL exit=!RUN_RC!
  popd
  exit /b 1
)
findstr /c:"N6_03_THREAD_EXIT_HOOK=PASS" "%LOG%" >nul
if errorlevel 1 (
  echo N6_03_THREAD_EXIT_HOOK=OUTPUT_FAIL
  popd
  exit /b 1
)
popd
exit /b 0
