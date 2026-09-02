@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_pr_n4_break_continue_dtor"
if not exist "%OUT%" mkdir "%OUT%"

echo === PR-N4 break/continue scope-exit destructor qualification ===
"%TCC%" pr_n4_break_continue_dtor.cpp -o "%OUT%\pr_n4_break_continue_dtor.exe"
if not "!errorlevel!"=="0" (
  echo PR_N4_BREAK_CONTINUE_SCOPE_EXIT_DTOR=COMPILE_FAIL
  popd
  exit /b 1
)
set "FAILED=0"
call :run_case 1 "N4_WHILE_BREAK_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"
if "!FAILED!"=="0" echo N4_OUTER_SCOPE_NOT_DESTROYED=PASS
call :run_case 2 "N4_WHILE_CONTINUE_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 3 "N4_NESTED_BREAK_DTOR_ORDER"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 4 "N4_NESTED_CONTINUE_DTOR_ORDER"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 5 "N4_FOR_CONTINUE_TARGET"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 6 "N4_DO_WHILE_CONTINUE_TARGET"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 7 "N4_TEMP_PLUS_LOCAL_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 8 "N4_SWITCH_BREAK_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 9 "N4_FOR_INIT_CONTINUE_PRESERVE"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 10 "N4_FOR_INIT_BREAK_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 11 "N4_SWITCH_BREAK_PRESERVES_LOOP"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 12 "N4_LOOP_BREAK_PRESERVES_SWITCH"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 13 "N4_SWITCH_CONTINUE_OUTER_LOOP"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 14 "N4_TEMP_CONTINUE_REPEAT"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_switch_unsafe "pr_n4_switch_unsafe_case.cpp" "N4_SWITCH_CASE_UNSAFE_ENTRY"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_switch_unsafe "pr_n4_switch_unsafe_default.cpp" "N4_SWITCH_DEFAULT_UNSAFE_ENTRY"
if not "!errorlevel!"=="0" set "FAILED=1"
if "!FAILED!"=="0" (
  echo INVALID_DTOR=0
  echo DOUBLE_DTOR=0
  echo MISSED_DTOR=0
  echo PR_N4_BREAK_CONTINUE_SCOPE_EXIT_DTOR=PASS
) else (
  echo PR_N4_BREAK_CONTINUE_SCOPE_EXIT_DTOR=FAIL
)
popd
exit /b !FAILED!

:run_case
echo === %~2 ===
"%OUT%\pr_n4_break_continue_dtor.exe" %1
if not "!errorlevel!"=="0" (
  echo %~2=FAIL
  exit /b 1
)
echo %~2=PASS
exit /b 0

:run_switch_unsafe
echo === %~2 ===
"%TCC%" -c %~1 -o "%OUT%\%~n1.o" >"%OUT%\%~n1.log" 2>&1
set "EC=!errorlevel!"
set "CRASH="
if !EC! lss 0 set "CRASH=1"
findstr /c:"internal error" "%OUT%\%~n1.log" >nul 2>nul
if not errorlevel 1 set "CRASH=1"
if defined CRASH (
  echo %~2=CRASH
  type "%OUT%\%~n1.log"
  exit /b 1
)
if !EC! equ 0 (
  echo %~2=FAIL
  exit /b 1
)
findstr /c:"switch case enters a scope requiring initialization is unsupported" "%OUT%\%~n1.log" >nul 2>nul
if errorlevel 1 (
  echo %~2=WRONG_DIAGNOSTIC
  type "%OUT%\%~n1.log"
  exit /b 1
)
echo %~2=FAIL_CLOSED
exit /b 0
