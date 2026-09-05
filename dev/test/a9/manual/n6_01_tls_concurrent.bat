@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_01_tls_concurrent"
if not exist "%OUT%" mkdir "%OUT%"
set "LOG=%OUT%\n6_01_tls_concurrent.log"

echo === N6-01 TLS concurrent-worker isolation ===
"%TCC%" n6_01_tls_concurrent.cpp -o "%OUT%\n6_01_tls_concurrent.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo N6_01_CONCURRENT_CROSS_THREAD_ADDRESS=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\n6_01_tls_concurrent.exe" >"%LOG%" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%"
  echo N6_01_CONCURRENT_CROSS_THREAD_ADDRESS=RUN_FAIL exit=!errorlevel!
  popd
  exit /b 1
)
findstr /c:"N6_01_CONCURRENT_CROSS_THREAD_ADDRESS=PASS" "%LOG%" >nul
if errorlevel 1 (
  type "%LOG%"
  echo N6_01_CONCURRENT_CROSS_THREAD_ADDRESS=OUTPUT_FAIL
  popd
  exit /b 1
)
type "%LOG%"
echo N6_01_CONCURRENT_CROSS_THREAD_ADDRESS=PASS
popd
exit /b 0
