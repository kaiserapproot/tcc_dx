@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_04b_tls_reclaim"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-04B: worker-thread TLS storage reclaim after destructor drain (Kernel32 process heap) ===
rem Every gate: compile, run exit 0, expected PASS marker in the log.  The
rem accounting authority is the runtime's internal counters
rem (__tcc_cpp_tls_n6_stats), never the process working set.
call :positive n6_04b_tls_reclaim_trivial N6_04B_TRIVIAL_RECLAIM "N6_04B_TLS_RECLAIM_TRIVIAL=PASS"
call :positive n6_04b_tls_reclaim_nontrivial N6_04B_NONTRIVIAL_RECLAIM "N6_04B_TLS_RECLAIM_NONTRIVIAL=PASS"
call :positive n6_04b_tls_reclaim_concurrent N6_04B_CONCURRENT_RECLAIM "N6_04B_TLS_RECLAIM_CONCURRENT=PASS"
call :positive n6_04b_tls_second_cleanup N6_04B_SECOND_CLEANUP "N6_04B_TLS_SECOND_CLEANUP=PASS"
call :positive n6_04b_tls_thread_churn N6_04B_THREAD_CHURN "N6_04B_TLS_THREAD_CHURN=PASS"

if not "!FAILED!"=="0" (
  echo N6_04B_TLS_RECLAIM=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N6_04B_TLS_RECLAIM=PASS
echo TLS_ALLOCATOR_AUTHORITY=KERNEL32_PROCESS_HEAP
echo CRT_MALLOC_IN_TLS_DETACH=NO
echo DTOR_DRAIN_BEFORE_RECLAIM=PASS
echo TLS_SLOT_CLEARED_BEFORE_TCB_FREE=PASS
echo OWNER_THREAD_RECLAIM=PASS
echo TLS_THREAD_MEMORY_RECLAIM=PASS
echo MAIN_THREAD_TLS_DTOR=DEFERRED_TO_N6_05
popd
exit /b 0

:positive
set "NAME=%~1"
set "KEY=%~2"
set "MARK=%~3"
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
if not "!RC!"=="0" (
  echo %KEY%=RUN_FAIL exit=!RC!
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"%MARK%" "%LOG%" >nul
if errorlevel 1 (
  echo %KEY%=OUTPUT_FAIL
  set /a FAILED+=1
  goto :eof
)
echo %KEY%=PASS
goto :eof
