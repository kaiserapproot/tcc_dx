@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_02_tls_recursive_init"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-02 FIX-4: recursive TLS initialization must abort (fail-closed) ===
rem Expected program behaviour: the runtime abort()s, the SIGABRT handler
rem prints the ctor entry counts (exactly one per class = no double
rem construction) and exits 42.  Reaching UNEXPECTED_RETURN (an uninitialized
rem value handed out) or any other exit code is a failure.
call :recursive n6_02_tls_recursive_direct N6_02_RECURSIVE_INIT_DIRECT
call :recursive n6_02_tls_recursive_indirect N6_02_RECURSIVE_INIT_INDIRECT

if not "!FAILED!"=="0" (
  echo N6_02_TLS_RECURSIVE_INIT=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N6_02_TLS_RECURSIVE_INIT=PASS
popd
exit /b 0

:recursive
set "NAME=%~1"
set "KEY=%~2"
set "LOG=%OUT%\%NAME%.log"
"%TCC%" %NAME%.cpp -o "%OUT%\%NAME%.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo %KEY%=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"%OUT%\%NAME%.exe" >"%LOG%" 2>&1
set "RC=!errorlevel!"
type "%LOG%"
"%FINDSTR%" /c:"UNEXPECTED_RETURN" "%LOG%" >nul
if not errorlevel 1 (
  echo %KEY%=UNINITIALIZED_VALUE_RETURNED
  set /a FAILED+=1
  goto :eof
)
if not "!RC!"=="42" (
  echo %KEY%=WRONG_EXIT exit=!RC!
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"!KEY!=ABORT_FAIL_CLOSED" "%LOG%" >nul
if errorlevel 1 (
  echo %KEY%=OUTPUT_FAIL
  set /a FAILED+=1
  goto :eof
)
echo %KEY%=ABORT_FAIL_CLOSED exit=!RC!
goto :eof
