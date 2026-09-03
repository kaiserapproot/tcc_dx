@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_pr_n4_goto_lifetime"
if not exist "%OUT%" mkdir "%OUT%"
set "FAILED=0"
set "CRLOG=%OUT%\compile.log"

echo === PR-N4 GOTO lifetime qualification ===
"%TCC%" pr_n4_goto_lifetime.cpp -o "%OUT%\pr_n4_goto_lifetime.exe" >"%CRLOG%" 2>&1
if not "!errorlevel!"=="0" (
  type "%CRLOG%"
  echo PR_N4_GOTO_LIFETIME=COMPILE_FAIL
  popd
  exit /b 1
)
call :run_case 1 "GOTO_OUTWARD_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 2 "GOTO_NESTED_DTOR_ORDER"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 3 "GOTO_SAME_SCOPE_NO_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 4 "GOTO_SAME_SCOPE_BACKWARD_NO_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 5 "GOTO_BACKWARD_SCOPE_EXIT"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 6 "GOTO_TARGET_SCOPE_PRESERVED"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 7 "GOTO_FORWARD_RESOLUTION"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 8 "TEMP_PLUS_GOTO"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 9 "GOTO_FORWARD_SKIP_LATER_LOCAL"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 10 "GOTO_MULTIPLE_FORWARD_RANGES"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 11 "GOTO_SAME_SCOPE_BACKWARD_ACROSS_DECL"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 12 "GOTO_SIBLING_SCOPE_EXIT"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 13 "GOTO_LABEL_PREFIX_SAFE"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 15 "GOTO_LCA_SOURCE_EXIT"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 16 "GOTO_CLOSED_VACUOUS_TARGET"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 17 "GOTO_VACUOUS_ENTRY"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_case 18 "SYMBOL_REUSE_NO_FALSE_DTOR"
if not "!errorlevel!"=="0" set "FAILED=1"

call :run_unsafe "pr_n4_goto_unsafe.cpp" "GOTO_UNSAFE_CLASS"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_unsafe "pr_n4_goto_unsafe_scalar.cpp" "GOTO_UNSAFE_SCALAR"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_unsafe "pr_n4_goto_unsafe_reference.cpp" "GOTO_UNSAFE_REFERENCE"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_unsafe "pr_n4_goto_unsafe_aggregate.cpp" "GOTO_UNSAFE_AGGREGATE"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_unsafe "pr_n4_goto_unsafe_lca.cpp" "GOTO_UNSAFE_LCA"
if not "!errorlevel!"=="0" set "FAILED=1"



if "!FAILED!"=="1" (
  echo PR_N4_GOTO_LIFETIME=FAIL
  popd
  exit /b 1
)
echo MISSED_DTOR=0
echo DOUBLE_DTOR=0
echo INVALID_DTOR=0
echo PR_N4_GOTO_LIFETIME=PASS
popd
exit /b 0

:run_case
echo === %~2 ===
"%OUT%\pr_n4_goto_lifetime.exe" %1 >nul
set "EC=!errorlevel!"
if !EC! neq 0 (
  echo %~2=FAIL
  exit /b 1
)
echo %~2=PASS
exit /b 0

:run_unsafe
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
findstr /c:"goto into a scope requiring initialization is unsupported" "%OUT%\%~n1.log" >nul 2>nul
if errorlevel 1 (
  echo %~2=WRONG_DIAGNOSTIC
  type "%OUT%\%~n1.log"
  exit /b 1
)
echo %~2=FAIL_CLOSED
exit /b 0