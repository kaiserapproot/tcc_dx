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
set "RUNLOG=%OUT%\local_static_dtor.run.log"
"%TCC%" -run ..\local_static_dtor.cpp >"%RUNLOG%" 2>&1
if not "!errorlevel!"=="0" goto run_fail
set "COUNT=0"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"END" "%RUNLOG%"') do set /a COUNT+=1
if not "!COUNT!"=="1" goto run_fail
set "COUNT=0"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"D5" "%RUNLOG%"') do set /a COUNT+=1
if not "!COUNT!"=="1" goto run_fail
set "COUNT=0"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"DQ13" "%RUNLOG%"') do set /a COUNT+=1
if not "!COUNT!"=="1" goto run_fail
echo LOCAL_STATIC_DTOR_TCC_RUN=PASS

set "MULTIOUT=%OUT%\multi"
if not exist "%MULTIOUT%" mkdir "%MULTIOUT%"
set "MULTILOG=%MULTIOUT%\local_static_multi.log"
"%TCC%" -c local_static_multi_a.cpp -o "%MULTIOUT%\a.o" >"%MULTILOG%.a" 2>&1
if not "!errorlevel!"=="0" goto multi_fail
"%TCC%" -c local_static_multi_b.cpp -o "%MULTIOUT%\b.o" >"%MULTILOG%.b" 2>&1
if not "!errorlevel!"=="0" goto multi_fail
"%TCC%" "%MULTIOUT%\a.o" "%MULTIOUT%\b.o" local_static_multi_main.cpp -o "%MULTIOUT%\local_static_multi.exe" >"%MULTILOG%.link" 2>&1
if not "!errorlevel!"=="0" goto multi_fail
"%MULTIOUT%\local_static_multi.exe" >"%MULTILOG%" 2>&1
if not "!errorlevel!"=="0" goto multi_fail
set "COUNT=0"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"END" "%MULTILOG%"') do set /a COUNT+=1
if not "!COUNT!"=="1" goto multi_fail
set "COUNT=0"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"DB" "%MULTILOG%"') do set /a COUNT+=1
if not "!COUNT!"=="1" goto multi_fail
set "COUNT=0"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"DA" "%MULTILOG%"') do set /a COUNT+=1
if not "!COUNT!"=="1" goto multi_fail
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"END" "%MULTILOG%"') do set "MULTI_END_LINE=%%n"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"DB" "%MULTILOG%"') do set "MULTI_DB_LINE=%%n"
for /f "tokens=1 delims=:" %%n in ('findstr /n /x /c:"DA" "%MULTILOG%"') do set "MULTI_DA_LINE=%%n"
if !MULTI_END_LINE! geq !MULTI_DB_LINE! goto multi_fail
if !MULTI_DB_LINE! geq !MULTI_DA_LINE! goto multi_fail
echo LOCAL_STATIC_DTOR_MULTI_TU=PASS
echo LOCAL_STATIC_DTOR=PASS
echo LOCAL_STATIC_DTOR_REVERSE_ORDER=PASS
echo PR_N5_LOCAL_STATIC_DTOR=PASS
popd
exit /b 0

:run_fail
type "%RUNLOG%"
echo LOCAL_STATIC_DTOR_TCC_RUN=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:multi_fail
if exist "%MULTILOG%.a" type "%MULTILOG%.a"
if exist "%MULTILOG%.b" type "%MULTILOG%.b"
if exist "%MULTILOG%.link" type "%MULTILOG%.link"
if exist "%MULTILOG%" type "%MULTILOG%"
echo LOCAL_STATIC_DTOR_MULTI_TU=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

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
