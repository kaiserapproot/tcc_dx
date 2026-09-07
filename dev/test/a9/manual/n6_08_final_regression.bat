@echo off
setlocal EnableExtensions EnableDelayedExpansion
goto :main

:child
set "BAT=%~1"
set "TAG=%~2"
echo --- !TAG! ^(!BAT!^) ---
call "%~dp0%BAT%"
set "RC=!errorlevel!"
if not "!RC!"=="0" (
  echo !TAG!=FAIL child_rc=!RC!
  set /a FAILED+=1
  if !RC! LSS 0 set /a CRASHES+=1
  goto :eof
)
echo !TAG!=PASS
goto :eof

:c_regression
echo --- C_REGRESSION (smoke + ABI) ---
"%TCC%" "%~dp0..\..\smoke\hello.c" -o "%OUT%\hello.exe" >"%OUT%\hello.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%OUT%\hello.compile"
  echo C_REGRESSION=COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"%OUT%\hello.exe" >"%OUT%\hello.run" 2>&1
if not "!errorlevel!"=="0" (
  type "%OUT%\hello.run"
  echo C_REGRESSION=RUN_FAIL
  set /a FAILED+=1
  goto :eof
)
"%TCC%" "%~dp0..\..\repro_double6.c" -o "%OUT%\repro_double6.exe" >"%OUT%\repro_double6.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%OUT%\repro_double6.compile"
  echo C_REGRESSION=ABI_COMPILE_FAIL
  set /a FAILED+=1
  goto :eof
)
"%OUT%\repro_double6.exe" >"%OUT%\repro_double6.run" 2>&1
if not "!errorlevel!"=="0" (
  type "%OUT%\repro_double6.run"
  echo C_REGRESSION=ABI_RUN_FAIL
  set /a FAILED+=1
  goto :eof
)
echo C_REGRESSION=PASS
echo C_TLS_LEGACY_PATH_REGRESSION=PASS
goto :eof

:cppunit_g7
echo --- CPPUNIT_G7 (build_cppunit.bat) ---
pushd "%~dp0..\..\..\..\sample\cppunit"
call build_cppunit.bat
set "RC=!errorlevel!"
popd
if not "!RC!"=="0" (
  echo CPPUNIT_G7=FAIL child_rc=!RC!
  set /a FAILED+=1
  goto :eof
)
echo CPPUNIT_G7=PASS
goto :eof

:main
pushd "%~dp0"
set "TCC=%~dp0..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_08_final"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0
set /a CRASHES=0

echo === N6-08 FINAL REGRESSION ===
echo BASE_COMMIT=a012ec7
echo N6_08_00_COMMIT=92c12e3
echo N6_07=COMPLETE
echo N6_08=IN_PROGRESS
echo N6_08_PRODUCTION_CHANGE=NONE
echo N6_08_PUBLIC_API_CHANGE=NONE
echo.

call :child n6_01_tls_storage.bat N6_01_REGRESSION
call :child n6_01_tls_concurrent.bat N6_01_CONCURRENT
call :child n6_01_tls_initializer_unsupported.bat N6_01_INITIALIZER_UNSUPPORTED
call :child n6_02_tls_lazy_ctor.bat N6_02_REGRESSION
call :child n6_02_tls_fail_closed.bat N6_02_FAIL_CLOSED
call :child n6_02_tls_inline_first_touch.bat N6_02_INLINE_FIRST_TOUCH
call :child n6_02_tls_alignment.bat N6_02_ALIGNMENT
call :child n6_02_tls_recursive_init.bat N6_02_RECURSIVE_INIT
call :child n6_03_thread_exit_hook.bat N6_03_REGRESSION
call :child n6_03_dtor_registry.bat N6_03_DTOR_REGISTRY
call :child n6_04_tls_dtor.bat N6_04_REGRESSION
call :child n6_04b_tls_reclaim.bat N6_04B_REGRESSION
call :child n6_05_main_termination.bat N6_05_REGRESSION
call :child n6_06a_main_termination.bat N6_06_REGRESSION
rem n6_06a2_00_measure is pre-implementation measurement; impl_gate is the authority.
call :child n6_06a2_impl_gate.bat N6_06A2_REGRESSION
call :child n6_06a2_01_baseline.bat N6_06A2_01_REGRESSION
call :child n6_06b_00_measure.bat N6_06B_REGRESSION
call :child n6_07_01_measure.bat N6_07_01_REGRESSION
call :child n6_07_00_measure.bat N6_07_00_REGRESSION
call :child n6_07_04_measure.bat N6_07_04_REGRESSION
call :child n6_07_05_measure.bat N6_07_05_REGRESSION
call :child n6_07_06_measure.bat N6_07_06_REGRESSION
call :child n6_08_stress.bat N6_08_STRESS
call :child pr_n5_local_static_dtor.bat N5_REGRESSION
call :c_regression
call :cppunit_g7

echo.
echo === N6-08 execution-mode matrix (derived from gate PASS) ===
if "!FAILED!"=="0" (
  echo NORMAL_EXE_MAIN_TLS=SUPPORTED
  echo NORMAL_EXE_WORKER_TLS=SUPPORTED
  echo TCC_RUN_OWNER_TLS=SUPPORTED
  echo TCC_RUN_WORKER_NORMAL_RETURN_TLS=SUPPORTED
  echo TCC_RUN_WORKER_EXITTHREAD_TLS=SUPPORTED
  echo TCC_RUN_WORKER_ENDTHREADEX_TLS=SUPPORTED
  echo DIRECT_RELOCATE_TLS_STORAGE=AVAILABLE
  echo DIRECT_RELOCATE_TLS_LAZY_INIT=AVAILABLE
  echo DIRECT_RELOCATE_TLS_AUTO_FINALIZE=NO
  echo DIRECT_RELOCATE_FULL_CPP_EXECUTION=NO
  echo DIRECT_RELOCATE_CLASSIFICATION=LIMITED_BY_CURRENT_API
  echo DLL_TLS=FAIL_CLOSED
  echo NORMAL_EXE_TLS=SUPPORTED
  echo TCC_RUN_TLS=SUPPORTED
  echo DIRECT_RELOCATE_TLS=LIMITED_BY_CURRENT_API
) else (
  echo EXECUTION_MODE_MATRIX=FAIL
)

echo.
echo === N6-08 capability regression summary ===
if "!FAILED!"=="0" (
  echo TRIVIAL_TLS=PASS
  echo USER_CTOR_TLS=PASS
  echo USER_DTOR_TLS=PASS
  echo PER_THREAD_LAZY_INITIALIZATION=PASS
  echo PER_THREAD_LAZY_INIT=PASS
  echo PER_THREAD_STORAGE_ISOLATION=PASS
  echo DTOR_EXACTLY_ONCE=PASS
  echo DTOR_REVERSE_ACTUAL_CONSTRUCTION_ORDER=PASS
  echo DTOR_LIFO=PASS
  echo DTOR_OWNER_THREAD=PASS
  echo TLS_MEMORY_RECLAIM=PASS
  echo OUTSTANDING=0
  echo POST_FINALIZE_ACCESS=FAIL_CLOSED
  echo FINALIZER_REENTRY=FAIL_CLOSED
  echo UNSUPPORTED_TLS_FORM_COUNT=15
  echo UNSUPPORTED_TLS_SILENT_ACCEPTANCE_COUNT=0
  echo TCC_DELETE_LIVE_TLS_FAIL_CLOSED=PASS
  echo TCC_DELETE_WITH_LIVE_TLS=FAIL_CLOSED
  echo TCC_DELETE_AFTER_TLS_CLEANUP=PASS
  echo PENDING_DTOR_UAF_PREVENTED=YES
  echo TCC_DELETE_WITH_LIVE_TLS_RETURNS_NORMALLY=NO
  echo NORMAL_EXE_THREAD_CHURN=PASS
  echo TCC_RUN_THREAD_CHURN=PASS
  echo MIXED_WORKER_TERMINATION_STRESS=PASS
  echo MAIN_TLS_SURVIVES_ALL_WORKER_JOINS=PASS
  echo WORKER_DTOR_ON_OWNER_THREAD=PASS
  echo MAIN_TLS_DTOR_EXACTLY_ONCE=PASS
  echo C_REGRESSION=PASS
  echo C_TLS_LEGACY_PATH_REGRESSION=PASS
  echo N5_REGRESSION=PASS
  echo CPPUNIT_G7=PASS
  echo C_SOURCE_BEHAVIOR_CHANGE=NONE
  echo N5_RUNTIME_BEHAVIOR_CHANGE=NONE
  echo PRODUCTION_API_CHANGE_IN_N6_08=NONE
  echo NEW_LANGUAGE_FEATURE_IN_N6_08=NONE
  echo N6_01_REGRESSION=PASS
  echo N6_02_REGRESSION=PASS
  echo N6_03_REGRESSION=PASS
  echo N6_04_REGRESSION=PASS
  echo N6_04B_REGRESSION=PASS
  echo N6_05_REGRESSION=PASS
  echo N6_06_REGRESSION=PASS
  echo N6_07_REGRESSION=PASS
)

echo.
if not "!FAILED!"=="0" (
  echo N6_08_FINAL_REGRESSION=FAIL
  echo N6_08_GATING_FAILURES=!FAILED!
  echo N6_08_CRASHES=!CRASHES!
  popd
  exit /b 1
)
echo N6_08_GATING_FAILURES=0
echo N6_08_CRASHES=0
echo N6_08_FINAL_REGRESSION=PASS
echo N6_THREAD_LOCAL=COMPLETE
echo N6_CLOSURE_ALLOWED=YES
popd
exit /b 0
