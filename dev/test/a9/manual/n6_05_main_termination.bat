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
call :n6_pos n6_05_main_lifo 0 "DTOR_B" "MAIN_LIFO"
call :n6_pos n6_05_main_reclaim 0 "OUTSTANDING=0" "MAIN_TLS_RECLAIM"
call :n6_pos n6_05_auto_vs_tls 17 "STEP AUTO" "AUTO_BEFORE_TLS"
"%FINDSTR%" /c:"STEP TLS" "%OUT%\n6_05_auto_vs_tls.log" >nul
if errorlevel 1 (
  echo AUTO_BEFORE_TLS=MISSING_TLS_STEP
  set /a FAILED+=1
) else (
  echo AUTO_BEFORE_TLS_TLS_STEP=PASS
)
call :n6_pos n6_05_no_tls_main 0 "MAIN_STATE_AT_ATEXIT=3" "NO_TLS_MAIN_FINALIZED"

call :n6_ord n6_05_tls_vs_n5_static 1 "STEP TLS_DTOR" "STEP N5_STATIC_DTOR" "TLS_BEFORE_N5_CASE1"
call :n6_ord n6_05_tls_vs_n5_static "2" "STEP TLS_DTOR" "STEP N5_STATIC_DTOR" "TLS_BEFORE_N5_CASE2"
call :n6_ord n6_05_tls_vs_atexit 1 "STEP TLS_DTOR" "STEP ATEXIT_CALLBACK" "TLS_BEFORE_ATEXIT_CASE1"
call :n6_ord n6_05_tls_vs_atexit "2" "STEP TLS_DTOR" "STEP ATEXIT_CALLBACK" "TLS_BEFORE_ATEXIT_CASE2"

call :exit_auto_zero n6_05_main_exit "MAIN_EXIT_AUTO_DTOR_COUNT"
call :abort_no_tls n6_05_abort_no_tls_dtor "ABORT_TLS_DTOR"

call :fail_closed n6_05_post_finalize_existing_tls "POST_FINALIZE_EXISTING_TLS"
call :fail_closed n6_05_post_finalize_new_tls "POST_FINALIZE_NEW_TLS"
call :fail_closed n6_05_finalize_reentry "FINALIZER_REENTRY_DURING_DRAIN"
call :fail_closed n6_05_worker_exit_fail_closed "WORKER_CALL_TO_EXIT"

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
if not "!RC!"=="!EXPECT_RC!" (
  echo %KEY%=RC_FAIL expected=!EXPECT_RC! got=!RC!
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"!NEED!" "%LOG%" >nul
if errorlevel 1 (
  echo %KEY%=OUTPUT_FAIL missing=!NEED!
  set /a FAILED+=1
  goto :eof
)
echo %KEY%=PASS
goto :eof

:n6_ord
set "NAME=%~1"
set "ARG=%~2"
set "NEED1=%~3"
set "NEED2=%~4"
set "KEY=%~5"
set "LOG=%OUT%\%NAME%_%ARG%.log"
"%TCC%" %NAME%.cpp -o "%OUT%\%NAME%_%ARG%.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo %KEY%=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"%OUT%\%NAME%_%ARG%.exe" %ARG% >"%LOG%" 2>&1
set "RC=!errorlevel!"
type "%LOG%"
if not "!RC!"=="0" (
  echo %KEY%=RUN_FAIL rc=!RC!
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"!NEED1!" "%LOG%" >nul
if errorlevel 1 (
  echo %KEY%=ORDER_FAIL missing=!NEED1!
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"!NEED2!" "%LOG%" >nul
if errorlevel 1 (
  echo %KEY%=ORDER_FAIL missing=!NEED2!
  set /a FAILED+=1
  goto :eof
)
echo %KEY%=PASS
goto :eof

:exit_auto_zero
set "NAME=%~1"
set "KEY=%~2"
set "LOG=%OUT%\%NAME%.log"
"%FINDSTR%" /c:"AUTO_DTOR" "%LOG%" >nul
if not errorlevel 1 (
  echo %KEY%=FAIL expected_no_AUTO_DTOR
  set /a FAILED+=1
  goto :eof
)
echo %KEY%=0
goto :eof

:abort_no_tls
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
if exist "%LOG%" type "%LOG%"
if exist "%LOG%" "%FINDSTR%" /c:"TLS_DTOR" "%LOG%" >nul
if exist "%LOG%" if not errorlevel 1 (
  echo %KEY%=FAIL tls_dtor_ran
  set /a FAILED+=1
  goto :eof
)
echo %KEY%=NOT_RUN
goto :eof

:fail_closed
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
if "!RC!"=="0" (
  if exist "%LOG%" type "%LOG%"
  echo %KEY%=FAIL expected_abort
  set /a FAILED+=1
  goto :eof
)
if "!RC!"=="9" (
  if exist "%LOG%" type "%LOG%"
  echo %KEY%=FAIL got_exit_9
  set /a FAILED+=1
  goto :eof
)
echo %KEY%=FAIL_CLOSED
goto :eof
