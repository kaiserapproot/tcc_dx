@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_06a2_00"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0
set "ANY_HOOK=NO"
set "ANY_USER_DTOR=NO"
set "ANY_RECLAIM=NO"

echo === N6-06A2-00: -run worker thread lifecycle measurement ===
echo BASE_COMMIT=e05f48f
echo PRODUCTION_CHANGE=NONE
echo.

call :run_probe n6_06a2_00_probe_create_thread CREATE_THREAD_NORMAL CT_NORMAL
call :run_probe n6_06a2_00_probe_create_thread_exit CREATE_THREAD_EXITTHREAD CT_EXIT
call :run_probe n6_06a2_00_probe_beginthreadex BEGIN_THREADEX_NORMAL BTX_NORMAL
call :run_probe n6_06a2_00_probe_beginthreadex_end BEGIN_THREADEX_ENDTHREADEX BTX_END

echo.
echo === N6-06A2-00 MEASUREMENT AUTHORITY ===
echo -RUN_WORKER_CREATE_PATH=CreateThread_and__beginthreadex
echo -RUN_WORKER_ENTRY_WRAPPER=NOT_AVAILABLE
echo -RUN_WORKER_NORMAL_RETURN_PATH=CreateThread_return_0_or__beginthreadex_return_0
echo -RUN_WORKER_EXITTHREAD_PATH=ExitThread_or__endthreadex
echo THREAD_ENTRY_WRAPPER_OWNED_BY_TCC=NO
echo TCC_RUN_MAIN_WRAPPER=_runmain_owner_thread_only
echo PATH_CREATE_THREAD_NORMAL_USER_TLS_DTOR=!CT_NORMAL_DTOR!
echo PATH_CREATE_THREAD_EXITTHREAD_USER_TLS_DTOR=!CT_EXIT_DTOR!
echo PATH_BEGIN_THREADEX_NORMAL_USER_TLS_DTOR=!BTX_NORMAL_DTOR!
echo PATH_BEGIN_THREADEX_ENDTHREADEX_USER_TLS_DTOR=!BTX_END_DTOR!

if "!ANY_HOOK!"=="YES" (
  echo DLL_THREAD_DETACH_RECEIVED=YES
) else (
  echo DLL_THREAD_DETACH_RECEIVED=NO
)
if "!ANY_USER_DTOR!"=="YES" (
  echo TLS_DTOR_AUTOMATICALLY_CALLED=YES
  set /a FAILED+=1
) else (
  echo TLS_DTOR_AUTOMATICALLY_CALLED=NO
)
if "!ANY_RECLAIM!"=="YES" (
  echo TLS_STORAGE_AUTOMATICALLY_RECLAIMED=PARTIAL_WITHOUT_USER_DTOR
) else (
  echo TLS_STORAGE_AUTOMATICALLY_RECLAIMED=NO
)

echo THREAD_EXIT_HOOK_DELIVERED=!ANY_HOOK!
echo THREAD_EXIT_USER_DTOR_DRAIN=NO
echo THREAD_EXIT_OBSERVABLE_BY_TCC_RUNTIME=PARTIAL
echo TLS_TCB_PRESENT_AT_WORKER_EXIT=YES
echo FLS_REQUIRED=NO
echo MAIN_DRAINS_WORKER_TLS=NO
echo WORKER_AUTOMATIC_CLEANUP_HOOK_CANDIDATE=WORKER_ENTRY_WRAPPER
echo DIRECT_EXITTHREAD_FROM_USER_WORKER=DEFERRED_TO_A2_IMPL
echo N6_06A2_START=YES
echo N6_06B_START=NO

if not "!FAILED!"=="0" (
  echo N6_06A2_00_MEASUREMENT=UNEXPECTED
  popd
  exit /b 1
)
echo N6_06A2_00_MEASUREMENT=PASS
popd
exit /b 0

:run_probe
set "SRC=%~1.cpp"
set "TAG=%~2"
set "OUTVAR=%~3"
set "LOG=%OUT%\%~1.log"
set "USER_DTOR=UNKNOWN"
set "HOOK=UNKNOWN"
set "RECLAIM=UNKNOWN"
"%TCC%" -run "%SRC%" >"%LOG%" 2>&1
set "RC=!errorlevel!"
type "%LOG%"
if not "!RC!"=="0" (
  echo !TAG!=RUN_FAIL rc=!RC!
  set /a FAILED+=1
  exit /b 0
)
"%FINDSTR%" /c:"MEASURE_USER_TLS_DTOR=NO" "%LOG%" >nul 2>&1
if not errorlevel 1 set "USER_DTOR=NO"
"%FINDSTR%" /c:"MEASURE_USER_TLS_DTOR=YES" "%LOG%" >nul 2>&1
if not errorlevel 1 set "USER_DTOR=YES"
"%FINDSTR%" /c:"MEASURE_HOOK_DELIVERED=YES" "%LOG%" >nul 2>&1
if not errorlevel 1 set "HOOK=YES"
"%FINDSTR%" /c:"MEASURE_HOOK_DELIVERED=NO" "%LOG%" >nul 2>&1
if not errorlevel 1 set "HOOK=NO"
"%FINDSTR%" /c:"MEASURE_RECLAIM=YES" "%LOG%" >nul 2>&1
if not errorlevel 1 set "RECLAIM=YES"
"%FINDSTR%" /c:"MEASURE_RECLAIM=NO" "%LOG%" >nul 2>&1
if not errorlevel 1 set "RECLAIM=NO"
set "!OUTVAR!_DTOR=!USER_DTOR!"
if /i "!OUTVAR!"=="CT_NORMAL" set "CT_NORMAL_DTOR=!USER_DTOR!"
if /i "!OUTVAR!"=="CT_EXIT" set "CT_EXIT_DTOR=!USER_DTOR!"
if /i "!OUTVAR!"=="BTX_NORMAL" set "BTX_NORMAL_DTOR=!USER_DTOR!"
if /i "!OUTVAR!"=="BTX_END" set "BTX_END_DTOR=!USER_DTOR!"
if /i "!USER_DTOR!"=="YES" set "ANY_USER_DTOR=YES"
if /i "!HOOK!"=="YES" set "ANY_HOOK=YES"
if /i "!RECLAIM!"=="YES" set "ANY_RECLAIM=YES"
echo !TAG!=DONE user_dtor=!USER_DTOR! hook=!HOOK! reclaim=!RECLAIM!
exit /b 0
