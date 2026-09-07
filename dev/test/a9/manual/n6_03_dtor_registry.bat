@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_03_dtor_registry"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set "LOG=%OUT%\n6_03_dtor_registry.log"

echo === N6-03: per-thread destructor registry foundation (no drain) ===
"%TCC%" n6_03_dtor_registry.cpp -o "%OUT%\n6_03_dtor_registry.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo N6_03_DTOR_REGISTRY=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\n6_03_dtor_registry.exe" >"%LOG%" 2>&1
set "RUN_RC=!errorlevel!"
type "%LOG%"
if not "!RUN_RC!"=="0" (
  echo N6_03_DTOR_REGISTRY=RUN_FAIL exit=!RUN_RC!
  popd
  exit /b 1
)
"%FINDSTR%" /c:"N6_03_DTOR_REGISTRY=PASS" "%LOG%" >nul
if errorlevel 1 (
  echo N6_03_DTOR_REGISTRY=OUTPUT_FAIL
  popd
  exit /b 1
)
popd
exit /b 0
