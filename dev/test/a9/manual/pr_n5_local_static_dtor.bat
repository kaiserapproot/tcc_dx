@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_pr_n5_local_static_dtor"
if not exist "%OUT%" mkdir "%OUT%"
set "LOG=%OUT%\local_static_dtor.log"
set "FAILED=0"

echo === PR-N5 function-local static destructor qualification ===
"%TCC%" ..\local_static_dtor.cpp -o "%OUT%\local_static_dtor.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo PR_N5_LOCAL_STATIC_DTOR=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\local_static_dtor.exe" >"%LOG%" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%"
  echo PR_N5_LOCAL_STATIC_DTOR=RUN_FAIL
  popd
  exit /b 1
)

call :check_once "C5"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "C7"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "C11"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "CC111"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "END"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "D111"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "D11"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "D7"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "D5"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "DQ13"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_absent "C99"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_absent "D99"
if not "!errorlevel!"=="0" set "FAILED=1"
if not "!FAILED!"=="0" (
  echo PR_N5_LOCAL_STATIC_DTOR=FAIL
  popd
  exit /b 1
)

call :line_of "END" END_LINE
call :line_of "D111" D111_LINE
call :line_of "D11" D11_LINE
call :line_of "D7" D7_LINE
call :line_of "D5" D5_LINE
call :line_of "DQ13" DQ13_LINE
if !END_LINE! geq !D111_LINE! goto order_fail
if !D111_LINE! geq !D11_LINE! goto order_fail
if !D11_LINE! geq !D7_LINE! goto order_fail
if !D7_LINE! geq !D5_LINE! goto order_fail
if !DQ13_LINE! geq !D111_LINE! goto order_fail
echo LOCAL_STATIC_DTOR=PASS
echo LOCAL_STATIC_DTOR_REVERSE_ORDER=PASS
echo PR_N5_LOCAL_STATIC_DTOR=PASS
popd
exit /b 0

:order_fail
type "%LOG%"
echo LOCAL_STATIC_DTOR_REVERSE_ORDER=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:check_once
set "COUNT=0"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"%~1" "%LOG%"') do set /a COUNT+=1
if "!COUNT!"=="1" exit /b 0
echo expected exactly one line: %~1
type "%LOG%"
exit /b 1

:check_absent
findstr /n /x /c:"%~1" "%LOG%" >nul 2>nul
if not errorlevel 1 (
  echo unexpected line: %~1
  type "%LOG%"
  exit /b 1
)
exit /b 0

:line_of
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"%~1" "%LOG%"') do set "%~2=%%n"
exit /b 0
