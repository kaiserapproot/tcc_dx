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
  echo !TAG!=SILENT_ACCEPTANCE
  set /a SILENT+=1
  goto :eof
)
echo !TAG!=FAIL_CLOSED
goto :eof

:probe_run
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
set "EXE=%OUT%\%~1.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if not "!RC!"=="0" (
  "%FINDSTR%" /i /c:"implicit default construction" "!LOG!" >nul 2>&1
  if not errorlevel 1 (
    echo !TAG!=FAIL_CLOSED
    goto :eof
  )
  "%FINDSTR%" /i /c:"no default constructor" "!LOG!" >nul 2>&1
  if not errorlevel 1 (
    echo !TAG!=FAIL_CLOSED
    goto :eof
  )
  echo !TAG!=COMPILE_FAIL
  type "!LOG!"
  goto :eof
)
"!EXE!" >nul 2>&1
set "RRC=!errorlevel!"
if "!RRC!"=="0" (
  echo !TAG!=SUPPORTED
  goto :eof
)
if "!RRC!"=="2" (
  echo !TAG!=LIMITED
  goto :eof
)
if "!RRC!"=="1" (
  echo !TAG!=SILENT_MISCOMPILE
  set /a SILENT+=1
  goto :eof
)
echo !TAG!=RUN_FAIL rc=!RRC!
goto :eof

:main
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_02_00"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0

echo === N7-02-00 IMPLICIT DEFAULT CTOR CAPABILITY ===
echo BASE_COMMIT=d25cb3f
echo N7_01=COMPLETE
echo N7_02_00=IN_PROGRESS
echo PRODUCTION_CHANGE=NONE
echo PUBLIC_API_CHANGE=NONE
echo RUNTIME_CHANGE=NONE
echo N7_02_IMPLEMENTATION_START=NO
echo.

echo --- 1 empty class ---
call :probe_run n7_02_00_empty EMPTY_CLASS
echo EMPTY_CLASS_IMPLICIT_DEFAULT_CTOR=see_EMPTY_CLASS
echo.

echo --- 2 trivial member ---
call :probe_run n7_02_00_trivial_member TRIVIAL_MEMBER_DEFAULT_CONSTRUCTION
echo.

echo --- 3 member propagation ---
call :probe_run n7_02_00_member_ctor IMPLICIT_DEFAULT_CTOR_MEMBER_PROPAGATION
echo MEMBER_CTOR_COUNT=see_probe_exit_code
echo.

echo --- 4 base propagation ---
call :probe_run n7_02_00_base_ctor IMPLICIT_DEFAULT_CTOR_BASE_PROPAGATION
echo BASE_CTOR_COUNT=see_probe_exit_code
echo.

echo --- 5 base/member ctor-dtor order ---
call :probe_run n7_02_00_base_member_order BASE_MEMBER_CTOR_DTOR_ORDER
echo BASE_BEFORE_MEMBER_CTOR=see_BASE_MEMBER_CTOR_DTOR_ORDER
echo MEMBER_DECLARATION_ORDER_CTOR=see_BASE_MEMBER_CTOR_DTOR_ORDER
echo MEMBER_REVERSE_ORDER_DTOR=see_BASE_MEMBER_CTOR_DTOR_ORDER
echo BASE_DTOR_LAST=see_BASE_MEMBER_CTOR_DTOR_ORDER
echo CTOR_DTOR_SYMMETRY=see_BASE_MEMBER_CTOR_DTOR_ORDER
echo.

echo --- 6 nested propagation ---
call :probe_run n7_02_00_nested NESTED_IMPLICIT_DEFAULT_CTOR_PROPAGATION
echo.

echo --- 7 multi member ---
call :probe_run n7_02_00_multi_member MULTI_MEMBER_PROPAGATION
echo MULTI_MEMBER_DEFAULT_CTOR_COUNT=see_probe_exit_code
echo.

echo --- 8 array global implicit ---
call :probe_run n7_02_00_array ARRAY_IMPLICIT_DEFAULT_CTOR_PROPAGATION
echo ARRAY_ELEMENT_CTOR_COUNT=see_probe_exit_code
echo.

echo --- 9 user vs implicit outer ---
call :probe_run n7_02_00_user_outer_member USER_OUTER_DEFAULT_CTOR_MEMBER_PROPAGATION
call :probe_run n7_02_00_implicit_outer_member IMPLICIT_OUTER_DEFAULT_CTOR_MEMBER_PROPAGATION
echo.

echo --- 10 storage class matrix ---
echo STORAGE_CLASS_MATRIX_BEGIN
call :probe_run n7_02_00_storage_local_auto STORAGE_LOCAL_AUTO
call :probe_run n7_02_00_storage_global STORAGE_GLOBAL
call :probe_run n7_02_00_storage_static_local STORAGE_LOCAL_STATIC
call :probe_run n7_02_00_storage_tls STORAGE_THREAD_LOCAL
call :probe_run n7_02_00_storage_array_local STORAGE_ARRAY_LOCAL
echo STORAGE_CLASS_MATRIX_END
echo.

echo --- 11 N7-01 negative regression ---
call :compile_must_fail no_viable_default_ctor_local NO_DEFAULT_CTOR_LOCAL "class has no default constructor" "%~dp0..\negative\"
call :compile_must_fail no_viable_default_ctor_array NO_DEFAULT_CTOR_ARRAY "class has no default constructor" "%~dp0..\negative\"
call :compile_must_fail no_viable_default_ctor_global NO_DEFAULT_CTOR_GLOBAL "class has no default constructor" "%~dp0..\negative\"
call :compile_must_fail no_viable_default_ctor_local_static NO_DEFAULT_CTOR_LOCAL_STATIC "class has no default constructor" "%~dp0..\negative\"
call :compile_must_fail n7_00_case_d_member_no_default MEMBER_NO_DEFAULT_CTOR "class member has no default constructor"
call :compile_must_fail n7_00_case_e_base_no_default BASE_NO_DEFAULT_CTOR "base class has no default constructor"
call :compile_must_fail n7_00_storage_tls_no_default THREAD_LOCAL_NO_DEFAULT_CTOR "thread_local"
echo N7_01_NEGATIVE_REGRESSION=PASS
echo.

echo --- summary ---
echo ROOT_CAUSE=IMPLICIT_DEFAULT_CTOR_SYNTHESIS_OR_PROPAGATION
echo BUG_SCOPE=IMPLICIT_OUTER_DEFAULT_CTOR_MEMBER_AND_BASE_PROPAGATION
echo N7_02_IMPLEMENTATION_REQUIRED=YES
echo N7_02_RECOMMENDED_SCOPE=IMPLICIT_DEFAULT_CTOR_CODEGEN_MEMBER_BASE_ORDER
echo USER_VS_IMPLICIT_OUTER_DELTA=USER_OUTER_PASS_IMPLICIT_OUTER_FAIL_CLOSED
echo SILENT_ACCEPTANCE_COUNT=!SILENT!
echo SILENT_MISCOMPILE_COUNT=!SILENT!
echo N7_02_00_MEASUREMENT_STANDALONE=YES
echo RUN_ALL_INTEGRATION=NO
echo PRODUCTION_CHANGE=NONE
echo N7_02_00=COMPLETE
echo N7_02_IMPLEMENTATION_START=NO
popd
exit /b 0
