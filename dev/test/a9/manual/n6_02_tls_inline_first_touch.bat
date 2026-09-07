@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_02_tls_inline_first_touch"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set "LOG=%OUT%\n6_02_tls_inline_first_touch.log"

echo === N6-02 FIX-1: first TLS touch from deferred inline bodies / cold nested ctor ===
"%TCC%" n6_02_tls_inline_first_touch.cpp -o "%OUT%\n6_02_tls_inline_first_touch.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo N6_02_INLINE_FIRST_TOUCH=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\n6_02_tls_inline_first_touch.exe" >"%LOG%" 2>&1
set "RUN_RC=!errorlevel!"
if not "!RUN_RC!"=="0" (
  type "%LOG%"
  echo N6_02_INLINE_FIRST_TOUCH=RUN_FAIL exit=!RUN_RC!
  popd
  exit /b 1
)
"%FINDSTR%" /c:"N6_02_INLINE_FIRST_TOUCH=PASS" "%LOG%" >nul
if errorlevel 1 (
  type "%LOG%"
  echo N6_02_INLINE_FIRST_TOUCH=OUTPUT_FAIL
  popd
  exit /b 1
)
"%FINDSTR%" /c:"N6_02_COLD_NESTED_TLS_CTOR=PASS" "%LOG%" >nul
if errorlevel 1 (
  type "%LOG%"
  echo N6_02_COLD_NESTED_TLS_CTOR=OUTPUT_FAIL
  popd
  exit /b 1
)
type "%LOG%"
echo TLS_CTOR_HOLDER_RESET_AFTER_GEN_INLINE=YES
popd
exit /b 0
