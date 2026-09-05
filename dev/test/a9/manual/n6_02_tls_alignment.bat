@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_02_tls_alignment"
if not exist "%OUT%" mkdir "%OUT%"
set "LOG=%OUT%\n6_02_tls_alignment.log"

echo === N6-02 FIX-2: TLS object alignment authority (calloc, 16 on x64) ===
"%TCC%" n6_02_tls_alignment.cpp -o "%OUT%\n6_02_tls_alignment.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo N6_02_TLS_ALIGNMENT=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\n6_02_tls_alignment.exe" >"%LOG%" 2>&1
set "RUN_RC=!errorlevel!"
if not "!RUN_RC!"=="0" (
  type "%LOG%"
  echo N6_02_TLS_ALIGNMENT=RUN_FAIL exit=!RUN_RC!
  popd
  exit /b 1
)
findstr /c:"N6_02_TLS_ALIGNMENT=PASS" "%LOG%" >nul
if errorlevel 1 (
  type "%LOG%"
  echo N6_02_TLS_ALIGNMENT=OUTPUT_FAIL
  popd
  exit /b 1
)
type "%LOG%"
popd
exit /b 0
