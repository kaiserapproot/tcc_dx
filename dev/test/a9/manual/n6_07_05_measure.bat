@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "ROOT=..\..\.."
set "TCC=!ROOT!\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_07_05"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0
set /a TLS_FORM_TEST_COUNT=0
set /a TLS_FORM_FAIL_CLOSED_COUNT=0
set /a TLS_FORM_SILENT_ACCEPT=0

echo === N6-07-05: unsupported thread_local inventory gate ===
echo BASE_COMMIT=8ebd840
echo.

call :inventory_positive n6_02_tls_lazy_ctor "thread_local lazy ctor" SUPPORTED
call :inventory_positive n6_04_tls_dtor_single "user-declared dtor" SUPPORTED
call :inventory_negative n6_02_tls_member_dtor_unsupported "implicit member dtor" "implicit non-trivial destructor"
call :inventory_negative n6_02_tls_implicit_member_ctor_unsupported "implicit member ctor" "implicit default construction"
call :inventory_negative n6_02_tls_polymorphic_unsupported "polymorphic class" "polymorphic class"
call :inventory_negative n6_02_tls_no_default_ctor_unsupported "no default ctor" "without default constructor"
call :inventory_negative n6_02_tls_array_unsupported "TLS array" "plain class type"
call :inventory_negative n6_02_tls_function_static_unsupported "function static TLS" "static thread_local"
call :inventory_negative n6_02_tls_function_local_unsupported "function local TLS" "namespace scope"
call :inventory_negative n6_02_tls_extern_unsupported "extern TLS" "extern thread_local"
call :inventory_negative n6_02_tls_class_static_unsupported "class static TLS" "static thread_local"
call :inventory_negative n6_02_tls_class_member_unsupported "class member TLS" "class member is unsupported"
call :inventory_negative_run_abort42 n6_04_tls_new_tls_during_drain "TLS init during drain"
call :inventory_negative_run_abort42 n6_02_tls_recursive_direct "recursive TLS init"
call :inventory_negative_run n6_05_post_finalize_existing_tls "destroyed TLS re-access"

echo UNSUPPORTED_TLS_FORM_TEST_COUNT=!TLS_FORM_TEST_COUNT!
echo UNSUPPORTED_TLS_FORM_FAIL_CLOSED_COUNT=!TLS_FORM_FAIL_CLOSED_COUNT!
echo UNSUPPORTED_TLS_FORM_SILENT_ACCEPTANCE_COUNT=!TLS_FORM_SILENT_ACCEPT!
if not "!TLS_FORM_SILENT_ACCEPT!"=="0" (
  echo N6_07_05_CLASS=FAIL_CLOSED_IMPLEMENTATION_REQUIRED
  echo N6_07_05=FAIL
  popd
  exit /b 1
)
if not "!FAILED!"=="0" (
  echo N6_07_05=FAIL
  popd
  exit /b 1
)
echo N6_07_05_CLASS=ALREADY_FAIL_CLOSED
echo N6_07_05=PASS
echo N6_07_05_MEASURE=PASS
popd
exit /b 0

:inventory_positive
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "CLASS=%~3"
set "LOG=!OUT!\inv_!NAME!.log"
set "EXE=!OUT!\inv_!NAME!.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "!NAME!.cpp" -o "!EXE!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  if exist "!EXE!" (
    echo INVENTORY !DESC!=!CLASS!
    exit /b 0
  )
)
echo INVENTORY !DESC!=COMPILE_FAIL
set /a FAILED+=1
exit /b 0

:inventory_negative
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "EXPECT=%~3"
set "LOG=!OUT!\inv_!NAME!.log"
"%TCC%" "!NAME!.cpp" -o "!OUT!\inv_!NAME!.exe" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo INVENTORY !DESC!=ACCIDENTALLY_ACCEPTED
  set /a TLS_FORM_SILENT_ACCEPT+=1
  set /a FAILED+=1
  exit /b 0
)
"%FINDSTR%" /c:"!EXPECT!" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo INVENTORY !DESC!=FAIL_CLOSED_BUT_CHECK_DIAG
  set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
) else (
  echo INVENTORY !DESC!=FAIL_CLOSED
  set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
)
exit /b 0

:inventory_negative_run_abort42
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "LOG=!OUT!\inv_!NAME!.log"
"%TCC%" -run "!NAME!.cpp" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo INVENTORY !DESC!=ACCIDENTALLY_ACCEPTED_RUNTIME
  set /a TLS_FORM_SILENT_ACCEPT+=1
  set /a FAILED+=1
  exit /b 0
)
if "!RC!"=="42" (
  echo INVENTORY !DESC!=FAIL_CLOSED
  set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
  exit /b 0
)
echo INVENTORY !DESC!=RUNTIME_FAIL_BUT_CHECK rc=!RC!
set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
exit /b 0

:inventory_negative_run
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "LOG=!OUT!\inv_!NAME!.log"
"%TCC%" -run "!NAME!.cpp" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo INVENTORY !DESC!=ACCIDENTALLY_ACCEPTED_RUNTIME
  set /a TLS_FORM_SILENT_ACCEPT+=1
  set /a FAILED+=1
  exit /b 0
)
echo INVENTORY !DESC!=RUNTIME_FAIL_CLOSED rc=!RC!
set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
exit /b 0
