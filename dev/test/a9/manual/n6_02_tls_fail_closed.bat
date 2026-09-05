@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_02_tls_fail_closed"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0

echo === N6-02 fail-closed gates (thread_local class type) ===
rem Each case must: exit non-zero, print the expected diagnostic, produce
rem no output file, and not crash (negative exit code).
call :negative n6_02_tls_dtor_unsupported "thread_local object with non-trivial destructor is unsupported in N6-02"
call :negative n6_02_tls_member_dtor_unsupported "thread_local object with non-trivial destructor is unsupported in N6-02"
call :negative n6_02_tls_no_default_ctor_unsupported "thread_local object of class without default constructor is unsupported"
call :negative n6_02_tls_polymorphic_unsupported "thread_local polymorphic class object is unsupported in N6-02"
call :negative n6_02_tls_implicit_member_ctor_unsupported "implicit default construction of non-trivial member is unsupported"
call :negative n6_02_tls_array_unsupported "N6-02 supports only thread_local int or a plain class type"
rem --- N6-02 REVIEW FIX-2: alignment authority ---
call :negative n6_02_tls_overaligned_unsupported "over-aligned thread_local is unsupported in N6-02"
call :negative n6_02_tls_aligned_attr_unsupported "thread_local with an alignment attribute is unsupported in N6-02"
rem --- N6-02 REVIEW FIX-5: unsupported forms ---
call :negative n6_02_tls_function_static_unsupported "static thread_local is unsupported in N6-02"
call :negative n6_02_tls_function_local_unsupported "thread_local is supported only at namespace scope"
call :negative n6_02_tls_extern_unsupported "extern thread_local is unsupported in N6-02"
call :negative n6_02_tls_class_static_unsupported "static thread_local is unsupported in N6-02"
call :negative n6_02_tls_class_member_unsupported "thread_local class member is unsupported in N6-02"

if not "!FAILED!"=="0" (
  echo N6_02_TLS_FAIL_CLOSED=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N6_02_TLS_FAIL_CLOSED=PASS
echo N6_02_NONTRIVIAL_DTOR=FAIL_CLOSED
echo N6_02_POLYMORPHIC_CLASS=FAIL_CLOSED
echo N6_02_NO_DEFAULT_CTOR=FAIL_CLOSED
echo N6_02_OVERALIGNED_CLASS=FAIL_CLOSED
echo FUNCTION_STATIC_THREAD_LOCAL=FAIL_CLOSED
echo EXTERN_THREAD_LOCAL=FAIL_CLOSED
echo CLASS_STATIC_THREAD_LOCAL=FAIL_CLOSED
popd
exit /b 0

:negative
set "NAME=%~1"
set "EXPECT=%~2"
set "LOG=%OUT%\%NAME%.log"
if exist "%OUT%\%NAME%.exe" del /q "%OUT%\%NAME%.exe"
"%TCC%" %NAME%.cpp -o "%OUT%\%NAME%.exe" >"%LOG%" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  type "%LOG%"
  echo %NAME%=COMPILE_SUCCEEDED
  set /a FAILED+=1
  goto :eof
)
if !RC! LSS 0 (
  type "%LOG%"
  echo %NAME%=TCC_CRASH exit=!RC!
  set /a FAILED+=1
  goto :eof
)
if exist "%OUT%\%NAME%.exe" (
  type "%LOG%"
  echo %NAME%=BAD_CODE_ACCEPTED
  set /a FAILED+=1
  goto :eof
)
findstr /c:"%EXPECT%" "%LOG%" >nul
if errorlevel 1 (
  type "%LOG%"
  echo %NAME%=WRONG_DIAGNOSTIC
  set /a FAILED+=1
  goto :eof
)
echo %NAME%=FAIL_CLOSED exit=!RC!
goto :eof
