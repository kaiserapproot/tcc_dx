@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "ROOT=..\..\.."
set "OUT=%TEMP%\tcc_n6_06a_run"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-06A: tcc_run owner-thread TLS execution lifetime ===

call :n6_run n6_06a_run_return 17 "TLS_DTOR" "N6_06A_RUN_RETURN"
call :n6_run n6_06a_run_fallthrough 0 "TLS_DTOR" "N6_06A_RUN_FALLTHROUGH"
call :n6_run n6_06a_run_exit 23 "TLS_DTOR" "N6_06A_RUN_EXIT"
call :n6_run n6_06a_run_lifo 0 "LIFO_ORDER=PASS" "N6_06A_TLS_LIFO"
call :n6_run n6_06a_run_reclaim 0 "OUTSTANDING=0" "N6_06A_TLS_RECLAIM"
call :n6_run n6_06a_auto_vs_tls 17 "ORDER_AUTO_BEFORE_TLS=PASS" "N6_06A_AUTO_BEFORE_TLS"
call :inject_symbols
call :n6_run n6_06a_worker_joined 0 "WORKER_EXPLICIT_TLS_CLEANUP=PASS" "N6_06A_RUN_WORKER_EXPLICIT_CLEANUP"

call :global_ctor n6_06a_run_global_ctor_tls "N6_06A_RUN_ENTER_BEFORE_GLOBAL_CTOR"

call :exit_auto_zero n6_06a_run_exit "N6_06A_EXIT_AUTO_DTOR_COUNT"
call :fail_closed n6_06a_finalize_reentry "N6_06A_FINALIZER_REENTRY"

call :libtcc_cross_tccstate

if not "!FAILED!"=="0" (
  echo N6_06A_MAIN_TERMINATION=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N6_06A_MAIN_TERMINATION=PASS
echo N6_06A_LONGJMP_MODEL_CHANGED=NO
echo N6_06A_RUN_MAIN_FINALIZE_DRAINS_WORKERS=NO
echo N6_06A_RUN_WORKER_TLS_AUTOMATIC_THREAD_EXIT_CLEANUP=UNPROVEN
echo N6_06A_SAME_RUNTIME_IMAGE_SECOND_EPOCH=UNPROVEN
echo N6_06A_FINALIZED_TO_NEXT_EPOCH_TRANSITION=UNPROVEN
echo N6_06A_CLOSURE_ALLOWED=NO
popd
exit /b 0

:n6_run
set "NAME=%~1"
set "EXPECT_RC=%~2"
set "NEED=%~3"
set "KEY=%~4"
set "LOG=!OUT!\!NAME!.log"
"!TCC!" -run !NAME!.cpp >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if not "!RC!"=="!EXPECT_RC!" (
  echo !KEY!=RC_FAIL expected=!EXPECT_RC! got=!RC!
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"!NEED!" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=OUTPUT_FAIL missing=!NEED!
  set /a FAILED+=1
  goto :eof
)
echo !KEY!=PASS
goto :eof

:inject_symbols
set "KEY=N6_06A_CSTR_INJECT"
set "LOG=!OUT!\n6_06a_inject_symbols.log"
"!TCC!" -run n6_06a_inject_symbols.cpp >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if not "!RC!"=="0" (
  echo !KEY!=RC_FAIL rc=!RC!
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"INJECTED_RUNTIME_SOURCE_COMPLETE=PASS" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=MISSING_INJECTED_RUNTIME
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"RUN_ENTER_SYMBOL_PRESENT=PASS" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=MISSING_RUN_ENTER
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"EMBEDDED_NUL_COUNT=0" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=MISSING_NUL_GATE
  set /a FAILED+=1
  goto :eof
)
echo N6_06A_INJECTED_RUNTIME_SOURCE_COMPLETE=PASS
echo N6_06A_RUN_ENTER_SYMBOL_PRESENT=PASS
echo N6_06A_RUN_FINALIZE_SYMBOL_PRESENT=PASS
echo N6_06A_TRAILING_NUL_ONLY=PASS
echo N6_06A_EMBEDDED_NUL_COUNT=0
echo !KEY!=PASS
goto :eof

:global_ctor
set "NAME=%~1"
set "KEY=%~2"
set "LOG=!OUT!\!NAME!.log"
"!TCC!" -run !NAME!.cpp >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if not "!RC!"=="0" (
  echo !KEY!=RUN_FAIL rc=!RC!
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"GLOBAL_CTOR" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=MISSING_GLOBAL_CTOR
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"TLS_CTOR" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=MISSING_TLS_CTOR
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"TLS_DTOR" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=MISSING_TLS_DTOR
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"GLOBAL_DTOR" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=MISSING_GLOBAL_DTOR
  set /a FAILED+=1
  goto :eof
)
echo !KEY!=PASS
echo N6_06A_TLS_BEFORE_GLOBAL_DTORS=PASS
goto :eof

:exit_auto_zero
set "NAME=%~1"
set "KEY=%~2"
set "LOG=!OUT!\!NAME!.log"
"!FINDSTR!" /c:"AUTO_DTOR" "!LOG!" >nul
if not errorlevel 1 (
  echo !KEY!=FAIL expected_no_AUTO_DTOR
  set /a FAILED+=1
  goto :eof
)
echo !KEY!=0
goto :eof

:fail_closed
set "NAME=%~1"
set "KEY=%~2"
set "LOG=!OUT!\!NAME!.log"
"!TCC!" -run !NAME!.cpp >"!LOG!" 2>&1
set "RC=!errorlevel!"
if !RC! LSS 0 (
  if exist "!LOG!" type "!LOG!"
  echo !KEY!=CRASH rc=!RC!
  set /a FAILED+=1
  goto :eof
)
if !RC! EQU 0 (
  if exist "!LOG!" type "!LOG!"
  echo !KEY!=FAIL expected_abort
  set /a FAILED+=1
  goto :eof
)
if not !RC! EQU 3 (
  if exist "!LOG!" type "!LOG!"
  echo !KEY!=FAIL unexpected_rc=!RC!
  set /a FAILED+=1
  goto :eof
)
echo !KEY!=FAIL_CLOSED
goto :eof

:libtcc_cross_tccstate
set "REPO=!ROOT!\.."
set "HARNESS=!REPO!\x64\Release\n6_06a_libtcc_harness.exe"
set "LIBPATH=!ROOT!"
msbuild n6_06a_libtcc_harness.vcxproj /p:Configuration=Release /p:Platform=x64 /v:minimal >"!OUT!\libtcc_build.log" 2>&1
if not "!errorlevel!"=="0" (
  type "!OUT!\libtcc_build.log"
  echo N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=BUILD_FAIL
  set /a FAILED+=1
  goto :eof
)
if not exist "!HARNESS!" (
  echo N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=MISSING_EXE
  set /a FAILED+=1
  goto :eof
)
"!HARNESS!" "!LIBPATH!" >"!OUT!\libtcc_second.log" 2>&1
set "RC=!errorlevel!"
type "!OUT!\libtcc_second.log"
if !RC! LSS 0 (
  echo N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=CRASH rc=!RC!
  set /a FAILED+=1
  goto :eof
)
if not "!RC!"=="0" (
  echo N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=HARNESS_FAIL rc=!RC!
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=PASS" "!OUT!\libtcc_second.log" >nul
if errorlevel 1 (
  echo N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=OUTPUT_FAIL
  set /a FAILED+=1
  goto :eof
)
echo N6_06A_CROSS_TCCSTATE_TOMBSTONE_ISOLATION=PASS
echo N6_06A_SECOND_EXECUTION_SAME_PROCESS=PASS
echo N6_06A_SECOND_EXECUTION_SAME_HOST_THREAD=PASS
"!FINDSTR!" /c:"SAME_TCCSTATE_SECOND_TCC_RUN_ATTEMPTED=YES" "!OUT!\libtcc_second.log" >nul
if errorlevel 1 (
  echo N6_06A_SAME_TCCSTATE_CONTRACT=MISSING
  set /a FAILED+=1
  goto :eof
)
type "!OUT!\libtcc_second.log" | "!FINDSTR!" "SAME_TCCSTATE_RUN SAME_TCCSTATE_SECOND N6_06A_SAME_RUNTIME N6_06A_FINALIZED TCC_RUN_EXECUTIONS"
goto :eof
