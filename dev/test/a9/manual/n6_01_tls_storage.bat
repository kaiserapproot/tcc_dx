@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_01_tls_storage"
if not exist "%OUT%" mkdir "%OUT%"
set "LOG=%OUT%\n6_01_tls_storage.log"

echo === N6-01 TLS storage primitive ===
"%TCC%" n6_01_tls_storage.cpp -o "%OUT%\n6_01_tls_storage.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo N6_01_TLS_STORAGE_PRIMITIVE=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\n6_01_tls_storage.exe" >"%LOG%" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%"
  echo N6_01_TLS_STORAGE_PRIMITIVE=RUN_FAIL
  popd
  exit /b 1
)
findstr /c:"N6_01_TLS_STORAGE_PRIMITIVE=PASS" "%LOG%" >nul
if errorlevel 1 (
  type "%LOG%"
  echo N6_01_TLS_STORAGE_PRIMITIVE=OUTPUT_FAIL
  popd
  exit /b 1
)
type "%LOG%"
echo N6_01_TLS_STORAGE_PRIMITIVE=PASS
echo N6_01_CONSTRUCTOR_IMPLEMENTED=NO
echo N6_01_DESTRUCTOR_IMPLEMENTED=NO
echo N6_01_THREAD_EXIT_CLEANUP=NO
popd
exit /b 0
