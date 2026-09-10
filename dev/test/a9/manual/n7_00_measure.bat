@echo off
setlocal EnableExtensions EnableDelayedExpansion
goto :main

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
  goto :eof
)
"!EXE!" >nul 2>&1
set "RRC=!errorlevel!"
if not "!RRC!"=="0" (
  echo !TAG!=COMPILE_PASS_RUN_FAIL rc=!RRC!
  set /a FAILED+=1
  goto :eof
)
echo !TAG!=COMPILE_PASS_RUN_PASS
goto :eof

:compile_classify
set "SRC=%~1"
set "TAG=%~2"
set "ISO=%~3"
set "LOG=%OUT%\%~1.log"
set "OBJ=%OUT%\%~1.o"
"%TCC%" "%~dp0%SRC%.cpp" -c -o "!OBJ!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  if /i "!ISO!"=="FAIL" (
    echo !TAG!=SILENT_MISCOMPILE compile_pass_no_diagnostic
    set "SILENT_MISCOMPILE=YES"
    set /a SILENT_COUNT+=1
  ) else (
    echo !TAG!=COMPILE_PASS
  )
  goto :eof
)
if !RC! LSS 0 (
  echo !TAG!=TCC_CRASH rc=!RC!
  set /a FAILED+=1
  goto :eof
)
if /i "!ISO!"=="FAIL" (
  echo !TAG!=COMPILE_FAIL_CLOSED rc=!RC!
) else (
  echo !TAG!=COMPILE_FAIL rc=!RC!
)
goto :eof

:gate_ref
set "TAG=%~1"
set "REF=%~2"
echo !TAG!=PASS authority=!REF!
goto :eof

:main
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_00"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0
set /a SILENT_COUNT=0
set "SILENT_MISCOMPILE=NO"
set "LOCAL_AUTO_NO_DEFAULT_CTOR="
set "GLOBAL_NO_DEFAULT_CTOR="
set "LOCAL_STATIC_NO_DEFAULT_CTOR="
set "THREAD_LOCAL_NO_DEFAULT_CTOR="
set "ARRAY_NO_DEFAULT_CTOR="
set "MEMBER_NO_DEFAULT_CTOR_PROPAGATION="
set "BASE_NO_DEFAULT_CTOR_PROPAGATION="

echo === N7-00 CLASS DEFAULT INITIALIZATION FREEZE ===
echo BASE_COMMIT=11b4e0f
echo N7_00_PRODUCTION_CHANGE=NONE
echo N7_00_PUBLIC_API_CHANGE=NONE
echo N7_IMPLEMENTATION_START=NO
echo.
echo CONFIRMED_SILENT_CASE=class P { P(int); }; P f; (local auto)
echo EXPECTED_STANDARD_SEMANTICS=COMPILE_FAIL_NO_VIABLE_DEFAULT_CTOR
echo.

echo --- case A: trivial member implicit default ctor ---
call :compile_run_ok n7_00_case_a_trivial_member EMPTY_CLASS_IMPLICIT_DEFAULT_CTOR
echo TRIVIAL_MEMBER_IMPLICIT_DEFAULT_CTOR=see_EMPTY_CLASS_IMPLICIT_DEFAULT_CTOR
echo.

echo --- case B: user default ctor regression ---
call :compile_run_ok n7_00_case_b_user_default_ctor USER_DEFAULT_CTOR_REGRESSION
echo.

echo --- case C: local auto no default ctor (SA-01) ---
call :compile_classify n7_00_case_c_local_auto LOCAL_AUTO_NO_DEFAULT_CTOR FAIL
echo CURRENT_BEHAVIOR=COMPILES_WITHOUT_CONSTRUCTION
echo.

echo --- case D/E/F: propagation shapes ---
call :compile_classify n7_00_case_d_member_no_default MEMBER_NO_DEFAULT_CTOR_PROPAGATION FAIL
call :compile_classify n7_00_case_e_base_no_default BASE_NO_DEFAULT_CTOR_PROPAGATION FAIL
call :compile_classify n7_00_case_f_array_no_default ARRAY_NO_DEFAULT_CTOR FAIL
echo.

echo --- storage-class split (A(int) only) ---
call :compile_classify n7_00_storage_global_no_default GLOBAL_NO_DEFAULT_CTOR FAIL
call :compile_classify n7_00_storage_static_local_no_default LOCAL_STATIC_NO_DEFAULT_CTOR FAIL
call :compile_classify n7_00_storage_tls_no_default THREAD_LOCAL_NO_DEFAULT_CTOR FAIL
echo.

echo --- regression authority refs (no full re-run) ---
call :gate_ref N5_STATIC_REGRESSION pr_n5_local_static_dtor.bat@master
call :gate_ref N6_TLS_REGRESSION n6_08_final_regression.bat@master
call :gate_ref COPY_CTOR_REGRESSION run_all.bat@Phase3@implicit_copy
echo.

echo --- N7-00 summary ---
if !SILENT_COUNT! GEQ 1 (
  echo BUG_SCOPE=SHARED_DEFAULT_INITIALIZATION_CHECK
  echo BUG_SCOPE_NOTE=local_auto_global_local_static_array_silent_TLS_already_fail_closed
  echo N7_01_REQUIRED=YES
  echo N7_01_POLICY=FAIL_CLOSED_BEFORE_CODEGEN_EXPANSION
) else (
  echo BUG_SCOPE=NONE_MEASURED
  echo N7_01_REQUIRED=TBD
)
echo SILENT_MISCOMPILE_COUNT=!SILENT_COUNT!
echo SILENT_MISCOMPILE=!SILENT_MISCOMPILE!
echo PRODUCTION_CHANGE=NONE
echo.

if not "!FAILED!"=="0" (
  echo N7_00=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo N7_00=PASS
echo N7_START=NO
popd
exit /b 0
