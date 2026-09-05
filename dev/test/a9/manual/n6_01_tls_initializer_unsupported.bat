@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_01_tls_initializer"
if not exist "%OUT%" mkdir "%OUT%"
set "LOG=%OUT%\n6_01_tls_initializer.log"

echo === N6-01 unsupported initializer ===
"%TCC%" n6_01_tls_initializer_unsupported.cpp -o "%OUT%\n6_01_tls_initializer.exe" >"%LOG%" 2>&1
if "!errorlevel!"=="0" (
  type "%LOG%"
  echo N6_01_TLS_INITIALIZER_FAIL_CLOSED=COMPILE_SUCCEEDED
  popd
  exit /b 1
)
rem N6-02 renamed the stage suffix of the diagnostic; match the stable prefix.
findstr /c:"thread_local initializers are unsupported" "%LOG%" >nul
if errorlevel 1 (
  type "%LOG%"
  echo N6_01_TLS_INITIALIZER_FAIL_CLOSED=WRONG_DIAGNOSTIC
  popd
  exit /b 1
)
type "%LOG%"
echo N6_01_TLS_INITIALIZER_FAIL_CLOSED=PASS
popd
exit /b 0
