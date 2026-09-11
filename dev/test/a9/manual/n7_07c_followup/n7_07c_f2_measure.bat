@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
goto :f2_main

:expect_fail
set "TAG=%~1"
set "SRC=%~2"
set "NEEDLE=%~3"
set "LOG=%OUT%\%~n2.log"
"%TCC%" "%~dp0%SRC%" -o "%OUT%\%~n2.exe" >"%LOG%" 2>&1
if errorlevel 1 (
  findstr /i /c:"%NEEDLE%" "%LOG%" >nul 2>&1
  if errorlevel 1 (echo !TAG!=FAIL_NO_DIAG & set /a BAD=1) else (echo !TAG!=PASS)
) else (
  echo !TAG!=UNEXPECTED_PASS
  set /a BAD=1
  set /a SILENT+=1
)
exit /b 0

:expect_pass
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~n2.exe"
"%TCC%" "%~dp0%SRC%" -o "%EXE%" >"%OUT%\%~n2.log" 2>&1
if errorlevel 1 (
  echo !TAG!=COMPILE_FAIL
  type "%OUT%\%~n2.log"
  set /a BAD=1
  exit /b 0
)
"%EXE%" >"%OUT%\%~n2_run.log" 2>&1
if errorlevel 1 (
  echo !TAG!=RUN_FAIL
  set /a BAD=1
) else (
  echo !TAG!=PASS
)
exit /b 0

:expect_pass_rel
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~n2.exe"
"%TCC%" "%~dp0%SRC%" -o "%EXE%" >"%OUT%\%~n2.log" 2>&1
if errorlevel 1 (
  echo !TAG!=COMPILE_FAIL
  set /a BAD=1
  exit /b 0
)
"%EXE%" >"%OUT%\%~n2_run.log" 2>&1
if errorlevel 1 (
  echo !TAG!=RUN_FAIL
  set /a BAD=1
) else (
  echo !TAG!=PASS
)
exit /b 0

:f2_main
set "TCC=..\..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_07c_f2_out"
if not exist "%OUT%" mkdir "%OUT%"
set /a BAD=0
set /a SILENT=0

echo === N7-07C-F2 DEFAULT-ARG IMPLICIT CTOR ===
echo TCC=%TCC%
echo.

echo --- default-arg positive ---
call :expect_pass DEFAULT_ARG_CTOR_PLAIN_SCALAR n7_07c_f1_plain_scalar.cpp
call :expect_pass DEFAULT_ARG_CTOR_PLAIN_ARRAY n7_07c_f1_plain_array.cpp
call :expect_pass DEFAULT_ARG_CTOR_EXTERN_C_SCALAR n7_07c_f1_extern_c_scalar.cpp
call :expect_pass DEFAULT_ARG_CTOR_EXTERN_C_ARRAY n7_07c_f1_extern_c_array.cpp
call :expect_pass MULTI_DEFAULT_ARG_LOCAL_ARRAY n7_07c_f2_multi_default.cpp
call :expect_pass MULTIDIM_DEFAULT_ARG_CLASS_ARRAY n7_07c_f2_multidim_default.cpp
call :expect_pass DEFAULT_ARG_EVALUATION_ORDER n7_07c_f2_eval_per_call.cpp
call :expect_pass IMPLICIT_BASE_DEFAULT_ARG_CTOR n7_07c_f2_implicit_base_default_arg.cpp
call :expect_pass IMPLICIT_MEMBER_DEFAULT_ARG_CTOR n7_07c_f2_implicit_member_default_arg.cpp
echo.

echo --- vec2 matrix ---
call :expect_pass VEC2_PLAIN_SCALAR n7_07c_f1_vec2_plain_scalar.cpp
call :expect_pass VEC2_PLAIN_ARRAY n7_07c_f1_vec2_plain_array.cpp
call :expect_pass VEC2_EXTERN_C_SCALAR n7_07c_f1_vec2_extern_c_scalar.cpp
call :expect_pass VEC2_EXTERN_C_ARRAY n7_07c_f1_vec2_extern_c_array.cpp
echo.

echo --- fail-closed ---
call :expect_fail PARTIAL_DEFAULT_ARG_ARRAY n7_07c_f2_partial_default_fail.cpp "default constructor"
echo.

echo --- N7-07C core regression (authority) ---
call :expect_pass_rel PLAIN_ZERO_PARAM_LOCAL_ARRAY ../n7_07c/n7_07c_local_array_ctor.cpp
call :expect_pass_rel MULTIDIM_ZERO_PARAM_LOCAL_ARRAY ../n7_07c/n7_07c_local_array_multidim.cpp
call :expect_pass_rel IMPLICIT_CTOR_LOCAL_ARRAY ../n7_07c/n7_07c_local_array_implicit_ctor.cpp
call :expect_pass_rel TRIVIAL_LOCAL_STRUCT_ARRAY ../n7_07c/n7_07c_trivial_array.cpp
call :expect_pass_rel GLOBAL_CLASS_ARRAY_DEFAULT_CTOR ../n7_07c/n7_07c_global_control.cpp
call :expect_pass_rel CLASS_MEMBER_ARRAY_DEFAULT_CTOR ../n7_07c/n7_07c_member_control.cpp
call :expect_fail LOCAL_CLASS_ARRAY_NONTRIVIAL_DTOR ../n7_07c/n7_07c_local_array_dtor_failclosed.cpp "destruction of local class array"
call :expect_fail POLYMORPHIC_LOCAL_CLASS_ARRAY ../n7_07c/n7_07c_polymorphic_failclosed.cpp "polymorphic local class array"
echo.

echo === N7-07C-F2 SUMMARY ===
echo BAD_CODE_ACCEPTED=!SILENT!
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!BAD!"=="0" (
  echo N7_07C_F2=FAIL
  popd
  exit /b 1
)
echo N7_07C_F2=PASS
popd
exit /b 0
