@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_04_tls_dtor"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-04: per-thread thread_local destructor LIFO drain (PE TLS callback) ===
rem Positive gates: compile, run exit 0, expected PASS marker in the log.
call :positive n6_04_tls_dtor_frontend N6_04_NONTRIVIAL_DTOR_FRONTEND ""
call :positive n6_04_tls_dtor_single N6_04_TLS_DTOR_SINGLE "N6_04_TLS_DTOR_SINGLE=PASS"
call :positive n6_04_tls_dtor_lifo N6_04_TLS_DTOR_LIFO "N6_04_TLS_DTOR_LIFO=PASS"
call :positive n6_04_tls_dtor_existing_access N6_04_DTOR_ACCESS_EXISTING_TLS "N6_04_DTOR_ACCESS_EXISTING_TLS=PASS"
rem Fail-closed gate: first-time TLS initialization from a destructor during the
rem drain must abort (runtime), never construct, never register; exit 42 comes
rem from the SIGABRT handler (see the .cpp).
call :abort42 n6_04_tls_new_tls_during_drain N6_04_NEW_TLS_DURING_DRAIN

if not "!FAILED!"=="0" (
  echo N6_04_TLS_DTOR=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N6_04_TLS_DTOR=PASS
echo NONTRIVIAL_DTOR_FRONTEND_ACCEPTANCE=YES
echo DTOR_EXECUTION_CONTEXT=PE_TLS_CALLBACK_DLL_THREAD_DETACH
echo DTOR_ORDER=REVERSE_ACTUAL_CONSTRUCTION_ORDER
echo EXISTING_TLS_ACCESS_DURING_DRAIN=PASS
echo NEW_TLS_INITIALIZATION_DURING_DRAIN=FAIL_CLOSED
echo NEW_DTOR_REGISTRATION_DURING_DRAIN=FAIL_CLOSED
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
if not "%MARK%"=="" (
  "%FINDSTR%" /c:"%MARK%" "%LOG%" >nul
  if errorlevel 1 (
    echo %KEY%=OUTPUT_FAIL
    set /a FAILED+=1
    goto :eof
  )
)
echo %KEY%=PASS
goto :eof

:abort42
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
type "%LOG%"
"%FINDSTR%" /c:"UNEXPECTED_RETURN" "%LOG%" >nul
if not errorlevel 1 (
  echo %KEY%=NEW_TLS_SILENTLY_ACCEPTED
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"UNEXPECTED_JOIN" "%LOG%" >nul
if not errorlevel 1 (
  echo %KEY%=NEW_TLS_SILENTLY_ACCEPTED
  set /a FAILED+=1
  goto :eof
)
if not "!RC!"=="42" (
  echo %KEY%=WRONG_EXIT exit=!RC!
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"%KEY%=ABORT_FAIL_CLOSED" "%LOG%" >nul
if errorlevel 1 (
  echo %KEY%=OUTPUT_FAIL
  set /a FAILED+=1
  goto :eof
)
echo %KEY%=ABORT_FAIL_CLOSED exit=!RC!
goto :eof
