@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_02_tls_lazy_ctor"
if not exist "%OUT%" mkdir "%OUT%"
set "LOG=%OUT%\n6_02_tls_lazy_ctor.log"

echo === N6-02 per-thread lazy non-trivial default constructor ===
"%TCC%" n6_02_tls_lazy_ctor.cpp -o "%OUT%\n6_02_tls_lazy_ctor.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo N6_02_TLS_LAZY_CTOR=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\n6_02_tls_lazy_ctor.exe" >"%LOG%" 2>&1
set "RUN_RC=!errorlevel!"
if not "!RUN_RC!"=="0" (
  type "%LOG%"
  echo N6_02_TLS_LAZY_CTOR=RUN_FAIL exit=!RUN_RC!
  popd
  exit /b 1
)
findstr /c:"N6_02_TLS_LAZY_CTOR=PASS" "%LOG%" >nul
if errorlevel 1 (
  type "%LOG%"
  echo N6_02_TLS_LAZY_CTOR=OUTPUT_FAIL
  popd
  exit /b 1
)
type "%LOG%"
echo N6_02_TLS_LAZY_CTOR=PASS
echo N6_02_INITIALIZATION=PER_THREAD_LAZY_ON_FIRST_ACCESS
echo N6_02_CTOR_EXACTLY_ONCE=PER_THREAD
echo N6_02_NONTRIVIAL_DTOR=NOT_IMPLEMENTED
echo N6_02_DTOR_REGISTRY=NOT_IMPLEMENTED
echo N6_02_THREAD_EXIT_CLEANUP=NOT_IMPLEMENTED
popd
exit /b 0
