@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
goto :n7_07e_main

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

:n7_07e_main
set "TCC=..\..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_07e_out"
if not exist "%OUT%" mkdir "%OUT%"
set /a BAD=0
set /a SILENT=0

echo === N7-07E STATIC LOCAL CLASS ARRAY CONSTRUCTION ===
echo TCC=%TCC%
echo.

call :expect_pass STATIC_LOCAL_CLASS_ARRAY n7_07e_static_array_once.cpp
call :expect_pass STATIC_LOCAL_CLASS_ARRAY_INIT_ONCE n7_07e_static_array_once.cpp
call :expect_pass STATIC_LOCAL_CLASS_ARRAY_ORDER n7_07e_static_array_order.cpp
call :expect_pass STATIC_LOCAL_MULTIDIM_ARRAY n7_07e_static_array_multidim.cpp
call :expect_pass STATIC_LOCAL_MULTIDIM_INIT_ONCE n7_07e_static_array_multidim.cpp
call :expect_pass STATIC_ARRAY_DEFAULT_ARG_CTOR n7_07e_static_array_default_arg.cpp
call :expect_pass STATIC_ARRAY_DEFAULT_ARG_EVALUATION n7_07e_static_array_default_arg_eval.cpp
call :expect_pass STATIC_ARRAY_EXTERN_C n7_07e_static_array_extern_c.cpp
call :expect_pass AMATERAS_VEC3_STATIC_ARRAY n7_07e_amateras_vec3_static_array.cpp
call :expect_fail STATIC_ARRAY_NO_DEFAULT_CTOR n7_07e_static_array_no_default_ctor.cpp "class has no default constructor"
call :expect_fail STATIC_ARRAY_NONTRIVIAL_DTOR n7_07e_static_array_dtor_failclosed.cpp "static destructor is unsupported"
call :expect_fail STATIC_ARRAY_IMPLICIT_NONTRIVIAL_DTOR n7_07e_static_array_implicit_dtor_failclosed.cpp "static destructor is unsupported"
call :expect_pass TRIVIAL_STATIC_LOCAL_STRUCT_ARRAY n7_07e_trivial_static_array.cpp
call :expect_pass STATIC_LOCAL_SCALAR n7_07e_static_scalar_control.cpp
call :expect_pass AUTOMATIC_LOCAL_CLASS_ARRAY n7_07e_auto_array_control.cpp
call :expect_pass AUTOMATIC_ARRAY_EXTERN_C n7_07e_auto_array_extern_c.cpp
call :expect_pass GLOBAL_CLASS_ARRAY n7_07e_global_array_control.cpp
call :expect_pass CLASS_MEMBER_ARRAY n7_07e_member_array_control.cpp
echo.

echo === N7-07E SUMMARY ===
echo BAD_CODE_ACCEPTED=!SILENT!
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!BAD!"=="0" (
  echo N7_07E=FAIL
  popd
  exit /b 1
)
echo N7_07E=PASS
popd
exit /b 0
