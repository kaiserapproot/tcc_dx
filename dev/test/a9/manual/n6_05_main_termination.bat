@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_05_main"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-05: main-thread normal termination (TLS before static/atexit) ===

call :n6_pos n6_05_main_return 17 "TLS_DTOR" "MAIN_RETURN_TLS_DTOR"
call :n6_pos n6_05_main_fallthrough 0 "TLS_DTOR" "MAIN_FALLTHROUGH_TLS_DTOR"
call :n6_pos n6_05_main_exit 23 "TLS_DTOR" "MAIN_EXIT_TLS_DTOR"
call :n6_pos n6_05_main_lifo 0 "LIFO_ORDER=PASS" "MAIN_LIFO"
call :n6_pos n6_05_main_reclaim 0 "OUTSTANDING=0" "MAIN_TLS_RECLAIM"
call :n6_pos n6_05_auto_vs_tls 17 "ORDER_AUTO_BEFORE_TLS=PASS" "AUTO_BEFORE_TLS"
call :n6_pos n6_05_no_tls_main 0 "MAIN_STATE_AT_ATEXIT=3" "NO_TLS_MAIN_FINALIZED"

call :gate_tls_order n6_05_tls_vs_n5_static "first" "ORDER_TLS_BEFORE_N5=PASS" "TLS_BEFORE_N5_CASE1"
call :gate_tls_order n6_05_tls_vs_n5_static "second" "ORDER_TLS_BEFORE_N5=PASS" "TLS_BEFORE_N5_CASE2"
call :gate_tls_order n6_05_tls_vs_atexit "first" "ORDER_TLS_BEFORE_ATEXIT=PASS" "TLS_BEFORE_ATEXIT_CASE1"
call :gate_tls_order n6_05_tls_vs_atexit "second" "ORDER_TLS_BEFORE_ATEXIT=PASS" "TLS_BEFORE_ATEXIT_CASE2"

call :exit_auto_zero n6_05_main_exit "MAIN_EXIT_AUTO_DTOR_COUNT"
call :abort_no_tls n6_05_abort_no_tls_dtor "ABORT_TLS_DTOR"

call :fail_closed n6_05_post_finalize_existing_tls "POST_FINALIZE_EXISTING_TLS"
call :fail_closed n6_05_post_finalize_new_tls "POST_FINALIZE_NEW_TLS"
call :fail_closed n6_05_finalize_reentry "FINALIZER_REENTRY_DURING_DRAIN"
call :fail_closed n6_05_worker_exit_fail_closed "WORKER_CALL_TO_EXIT"

call :compile_only n6_05_member_main_no_global "MEMBER_MAIN_COMPILE"
call :mixed_c_main "MIXED_C_MAIN_CPP_MEMBER"

if not "!FAILED!"=="0" (
  echo N6_05_MAIN_TERMINATION=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N6_05_MAIN_TERMINATION=PASS
echo MAIN_THREAD_TLS_DTOR=PASS
echo TLS_BEFORE_N5_STATIC_DTOR=PASS
echo TLS_BEFORE_ATEXIT_CALLBACK=PASS
echo MAIN_FINALIZATION_TOMBSTONE=PASS
popd
exit /b 0

:n6_pos
set "NAME=%~1"
set "EXPECT_RC=%~2"
set "NEED=%~3"
set "KEY=%~4"
set "LOG=!OUT!\!NAME!.log"
"!TCC!" !NAME!.cpp -o "!OUT!\!NAME!.exe" >"!LOG!.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "!LOG!.compile"
  echo !KEY!=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"!OUT!\!NAME!.exe" >"!LOG!" 2>&1
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

:gate_tls_order
set "NAME=%~1"
set "ARG=%~2"
set "NEED=%~3"
set "KEY=%~4"
set "LOG=!OUT!\!NAME!_!ARG!.log"
"!TCC!" !NAME!.cpp -o "!OUT!\!NAME!_!ARG!.exe" >"!LOG!.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "!LOG!.compile"
  echo !KEY!=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"!OUT!\!NAME!_!ARG!.exe" "!ARG!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if not "!RC!"=="0" (
  echo !KEY!=RUN_FAIL rc=!RC!
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"!NEED!" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=ORDER_FAIL missing=!NEED!
  set /a FAILED+=1
  goto :eof
)
echo !KEY!=PASS
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

:abort_no_tls
set "NAME=%~1"
set "KEY=%~2"
set "LOG=!OUT!\!NAME!.log"
"!TCC!" !NAME!.cpp -o "!OUT!\!NAME!.exe" >"!LOG!.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "!LOG!.compile"
  echo !KEY!=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"!OUT!\!NAME!.exe" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if exist "!LOG!" type "!LOG!"
if !RC! LSS 0 (
  echo !KEY!=CRASH rc=!RC!
  set /a FAILED+=1
  goto :eof
)
if not !RC! EQU 3 (
  echo !KEY!=FAIL unexpected_rc=!RC!
  set /a FAILED+=1
  goto :eof
)
if exist "!LOG!" "!FINDSTR!" /c:"TLS_DTOR" "!LOG!" >nul
if exist "!LOG!" if not errorlevel 1 (
  echo !KEY!=FAIL tls_dtor_ran
  set /a FAILED+=1
  goto :eof
)
echo !KEY!=NOT_RUN
goto :eof

:fail_closed
set "NAME=%~1"
set "KEY=%~2"
set "LOG=!OUT!\!NAME!.log"
"!TCC!" !NAME!.cpp -o "!OUT!\!NAME!.exe" >"!LOG!.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "!LOG!.compile"
  echo !KEY!=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"!OUT!\!NAME!.exe" >"!LOG!" 2>&1
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

:compile_only
set "NAME=%~1"
set "KEY=%~2"
"!TCC!" -c !NAME!.cpp -o "!OUT!\!NAME!.o" >"!OUT!\!NAME!.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "!OUT!\!NAME!.compile"
  echo !KEY!=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
echo !KEY!=PASS
goto :eof

:mixed_c_main
set "KEY=%~1"
set "LOG=!OUT!\n6_05_mixed_c_main.log"
"!TCC!" -c n6_05_mixed_c_main_cpp_member.cpp -o "!OUT!\mixed_cpp.o" >"!LOG!.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "!LOG!.compile"
  echo !KEY!=COMPILE_FAIL_CPP
  set /a FAILED+=1
  goto :eof
)
"!TCC!" -c n6_05_mixed_c_main_cpp_member.c -o "!OUT!\mixed_c.o" >>"!LOG!.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "!LOG!.compile"
  echo !KEY!=COMPILE_FAIL_C
  set /a FAILED+=1
  goto :eof
)
"!TCC!" "!OUT!\mixed_c.o" "!OUT!\mixed_cpp.o" -o "!OUT!\mixed.exe" >>"!LOG!.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "!LOG!.compile"
  echo !KEY!=LINK_FAIL
  set /a FAILED+=1
  goto :eof
)
"!OUT!\mixed.exe" >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if not "!RC!"=="0" (
  echo !KEY!=RUN_FAIL rc=!RC!
  set /a FAILED+=1
  goto :eof
)
"!FINDSTR!" /c:"TLS_DTOR" "!LOG!" >nul
if errorlevel 1 (
  echo !KEY!=MISSING_TLS_DTOR
  set /a FAILED+=1
  goto :eof
)
echo !KEY!=PASS
goto :eof
