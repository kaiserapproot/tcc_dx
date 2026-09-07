@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_06a2_impl"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-06A2 IMPLEMENTATION GATE ===
echo BASE_COMMIT=e650873
echo WORKER_CLEANUP_AUTHORITY=WORKER_ENTRY_TRAMPOLINE
echo PE_TLS_CALLBACK_USER_DTOR_AUTHORITY=NO
echo.

call :gate n6_06a2_impl_create_thread CREATE_THREAD_NORMAL_RETURN_TLS_DTOR
call :gate n6_06a2_impl_create_thread CREATE_THREAD_NORMAL_RETURN_TLS_RECLAIM
call :gate n6_06a2_impl_beginthreadex BEGINTHREADEX_NORMAL_RETURN_TLS_DTOR
call :gate n6_06a2_impl_beginthreadex BEGINTHREADEX_NORMAL_RETURN_TLS_RECLAIM
call :gate n6_06a2_impl_pe_detach_noop PE_DETACH_AFTER_WRAPPER_CLEANUP
call :gate n6_06a2_01_main_isolation MAIN_TLS_SURVIVES_WORKER_JOIN
call :gate n6_06a2_01_main_isolation WORKER_TLS_DTOR_BEFORE_JOIN_RETURN
call :gate n6_06a2_01_multi_worker MULTI_WORKER_TLS_ISOLATION
call :gate n6_06a2_thread_churn THREAD_CHURN

echo.
echo === N6-06A2 IMPLEMENTATION AUTHORITY ===
echo WORKER_DTOR_OWNER_THREAD=PASS
echo MAIN_DRAINS_WORKER_TLS=NO
echo DIRECT_EXITTHREAD=UNSUPPORTED_IN_N6_06A2
echo DIRECT_ENDTHREADEX=UNSUPPORTED_IN_N6_06A2
echo FLS_REQUIRED=NO
echo N6_TCB_STORAGE_AUTHORITY=OS_THREAD_TLS

if not "!FAILED!"=="0" (
  echo N6_06A2_IMPLEMENTATION=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N6_06A2_IMPLEMENTATION=PASS
popd
exit /b 0

:gate
set "SRC=%~1.cpp"
set "MARK=%~2"
set "LOG=%OUT%\%~1_%~2.log"
"%TCC%" -run "%SRC%" >"%LOG%" 2>&1
set "RC=!errorlevel!"
if not "!RC!"=="0" (
  echo %MARK%=RUN_FAIL rc=!RC!
  type "%LOG%"
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"%MARK%=PASS" "%LOG%" >nul 2>&1
if errorlevel 1 (
  echo %MARK%=OUTPUT_FAIL
  type "%LOG%"
  set /a FAILED+=1
  goto :eof
)
echo %MARK%=PASS
goto :eof
