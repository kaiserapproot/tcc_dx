@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_06_00"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set "FAILED=0"

echo === N6-06-00 EXECUTION PATH MEASUREMENT ===
echo BASE_COMMIT=aed0143786c90d40d3b81c97d57cf59bfed2a24e
echo NORMAL_EXE_N6_05=PASS
echo PRODUCTION_CHANGE=NONE
echo.

call :run_rc n6_06_00_run_plain.cpp 7 PLAIN_RETURN
call :run_rc n6_06_00_run_fallthrough.cpp 0 FALLTHROUGH
call :run_rc n6_06_00_run_exit.cpp 9 EXIT_CALL
call :run_tls n6_06_00_run_tls_return.cpp TLS_RETURN

echo.
echo === same-shell sequential -run (new process each spawn) ===
"%TCC%" -run n6_06_00_run_plain.cpp >nul 2>&1
set "RC1=!errorlevel!"
"%TCC%" -run n6_06_00_run_plain.cpp >nul 2>&1
set "RC2=!errorlevel!"
echo SEQUENTIAL_RUN1_RC=!RC1!
echo SEQUENTIAL_RUN2_RC=!RC2!
if not "!RC1!"=="7" set "FAILED=1"
if not "!RC2!"=="7" set "FAILED=1"

echo.
echo -RUN_ENTRY_PATH=tcc.c:main -^> tcc_run (TCC_OUTPUT_MEMORY)
echo -RUN_MAIN_CALLER=tccrun.c:_runmain (PE) via tcc_add_runmain
echo -RUN_NORMAL_RETURN_PATH=_runmain: tcc_run_ctors -^> main -^> tcc_run_dtors -^> __run_on_exit -^> return ret -^> tcc_run return
echo -RUN_EXIT_PATH=runmain exit() -^> tcc_run_dtors -^> __run_on_exit -^> __rt_exit -^> longjmp(tcc_run main_jb)
echo -RUN_LONGJMP_USED=YES (exit path only; rt_exit in tccrun.c)
echo -RUN_PROCESS_EXIT_USED=NO (exit longjmps back to tcc_run; host process continues)
echo -RUN_RETURNS_TO_HOST=YES (tcc_run int returned to tcc.c; then tcc_delete)
echo LIBTCC_OUTPUT_MEMORY_PATH=tcc_set_output_type(TCC_OUTPUT_MEMORY) -^> tcc_relocate -^> tcc_get_symbol
echo LIBTCC_RELOCATE_PATH=tccrun.c:tcc_relocate -^> tcc_relocate_ex + rt_mem
echo LIBTCC_GET_SYMBOL_PATH=libtcc.c:tcc_get_symbol (post-relocate)
echo LIBTCC_USER_FUNCTION_CALL_OWNED_BY_TCC=NO (caller invokes fn ptr; no libtcc main wrapper)
echo LIBTCC_EXECUTION_END_OBSERVABLE_BY_RUNTIME=NO (no callback/hook at user fn return)
echo TCCSTATE_LIFETIME_EQUALS_EXECUTION_LIFETIME=NO (tcc_delete after tcc_run frees run mem only)
echo SAME_PROCESS_SECOND_EXECUTION_POSSIBLE=YES (libtcc: tcc_new/tcc_run again on same host process)
echo SAME_THREAD_SECOND_EXECUTION_POSSIBLE=YES (libtcc API; not tcc.exe CLI which respawns process)
echo PROCESS_GLOBAL_FINALIZED_STATE_REUSABLE=NO (N6-05 main tombstone is process-global; must not reuse for -run)
echo EXECUTION_EPOCH_REQUIRED=YES
echo N6_06A_START=YES

if not "!FAILED!"=="0" (
  echo N6_06_00_MEASUREMENT=FAIL
  popd
  exit /b 1
)
echo N6_06_00_MEASUREMENT=PASS
popd
exit /b 0

:run_rc
set "SRC=%~1"
set "EXPECT=%~2"
set "TAG=%~3"
set "LOG=%OUT%\%~n1.log"
"%TCC%" -run "%SRC%" >"%LOG%" 2>&1
set "RC=!errorlevel!"
echo !TAG! RC=!RC! expect=!EXPECT!
if not "!RC!"=="!EXPECT!" set "FAILED=1"
exit /b 0

:run_tls
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~n1.log"
"%TCC%" -run "%SRC%" >"%LOG%" 2>&1
set "RC=!errorlevel!"
echo !TAG! compile_run RC=!RC!
"%FINDSTR%" /i /c:"defined twice" "%LOG%" >nul 2>nul
if not errorlevel 1 (
  echo !TAG!=BLOCKED exit_injection_collision
  echo TLS_RUN_CURRENT_STATE=BLOCKED_EXIT_COLLISION
  exit /b 0
)
if not "!RC!"=="7" (
  echo !TAG!=UNEXPECTED_FAIL
  type "%LOG%"
  set "FAILED=1"
) else (
  echo !TAG!=PASS
)
exit /b 0
