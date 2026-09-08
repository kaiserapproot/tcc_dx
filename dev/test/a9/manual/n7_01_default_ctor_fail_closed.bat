@echo off
setlocal EnableExtensions EnableDelayedExpansion
goto :main

:compile_must_fail
set "SRC=%~1"
set "TAG=%~2"
set "PAT=%~3"
set "SRCDIR=%~4"
if "!SRCDIR!"=="" set "SRCDIR=%~dp0"
set "LOG=%OUT%\%~1.log"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
"%TCC%" "!SRCDIR!%SRC%.cpp" -c -o "%OUT%\%~1.o" >"%LOG%" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo !TAG!=SILENT_ACCEPTANCE compile_passed
  type "%LOG%"
  set /a FAILED+=1
  set /a SILENT+=1
  goto :eof
)
if !RC! LSS 0 (
  echo !TAG!=TCC_CRASH rc=!RC!
  type "%LOG%"
  set /a FAILED+=1
  goto :eof
)
if not "!PAT!"=="" (
  "%FINDSTR%" /c:"!PAT!" "%LOG%" >nul 2>&1
  if errorlevel 1 (
    echo !TAG!=FAIL_CLOSED_NO_DIAGNOSTIC expected=!PAT!
    type "%LOG%"
    set /a FAILED+=1
    goto :eof
  )
)
echo !TAG!=FAIL_CLOSED rc=!RC!
goto :eof

:compile_run_ok
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
set "EXE=%OUT%\%~1.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if not "!RC!"=="0" (
  echo !TAG!=COMPILE_FAIL rc=!RC!
  type "!LOG!"
  set /a FAILED+=1
  goto :eof
)
"!EXE!" >nul 2>&1
set "RRC=!errorlevel!"
if not "!RRC!"=="0" (
  echo !TAG!=RUN_FAIL rc=!RRC!
  set /a FAILED+=1
  goto :eof
)
echo !TAG!=PASS
goto :eof

:main
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_01"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0
set /a SILENT=0

echo === N7-01 NO VIABLE DEFAULT CONSTRUCTOR FAIL-CLOSED GATE ===
echo BASE_COMMIT=dbd43f6
echo N7_01_PRODUCTION_CHANGE=FRONTEND_ONLY
echo.

echo --- negative: 4 silent paths ---
call :compile_must_fail no_viable_default_ctor_local N7_01_LOCAL_AUTO_NO_DEFAULT_CTOR "class has no default constructor" "%~dp0..\negative\"
call :compile_must_fail no_viable_default_ctor_array N7_01_ARRAY_NO_DEFAULT_CTOR "class has no default constructor" "%~dp0..\negative\"
call :compile_must_fail no_viable_default_ctor_global N7_01_GLOBAL_NO_DEFAULT_CTOR "class has no default constructor" "%~dp0..\negative\"
call :compile_must_fail no_viable_default_ctor_local_static N7_01_LOCAL_STATIC_NO_DEFAULT_CTOR "class has no default constructor" "%~dp0..\negative\"
echo.

echo --- regression: existing fail-closed ---
call :compile_must_fail n7_00_case_d_member_no_default N7_01_MEMBER_NO_DEFAULT_CTOR "class member has no default constructor"
call :compile_must_fail n7_00_case_e_base_no_default N7_01_BASE_NO_DEFAULT_CTOR "base class has no default constructor"
call :compile_must_fail n7_00_storage_tls_no_default N7_01_THREAD_LOCAL_NO_DEFAULT_CTOR "thread_local"
echo.

echo --- positive regression ---
call :compile_run_ok n7_01_pos_user_default_ctor N7_01_USER_DEFAULT_CTOR
call :compile_run_ok n7_01_pos_explicit_nondefault_ctor N7_01_EXPLICIT_NONDEFAULT_CTOR
call :compile_run_ok n7_01_pos_trivial_implicit N7_01_TRIVIAL_IMPLICIT_DEFAULT
call :compile_run_ok n7_01_pos_default_argument_ctor N7_01_DEFAULT_ARGUMENT_CTOR
call :compile_run_ok n7_01_pos_overloaded_with_default_ctor N7_01_OVERLOADED_WITH_DEFAULT_CTOR
call :compile_run_ok n7_01_pos_trivial_array_default_init N7_01_TRIVIAL_ARRAY_DEFAULT_INIT
echo N7_01_ARRAY_DEFAULT_CTOR_REGRESSION=LIMITED_SKIPPED reason=global_and_local_array_default_ctor_not_in_FEAT-4F-4G
echo.

echo SILENT_ACCEPTANCE_COUNT=!SILENT!
echo SILENT_MISCOMPILE_COUNT=!SILENT!
echo.

if not "!FAILED!"=="0" (
  echo N7_01=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N7_01=PASS
popd
exit /b 0
