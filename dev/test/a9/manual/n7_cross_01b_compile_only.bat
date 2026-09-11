@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_cross_01b_compile_only"
if not exist "%OUT%" mkdir "%OUT%"

if /i "%N7_01B_COMPILE_GATE%"=="POST" (
  set "GATE_MODE=POST"
) else (
  set "GATE_MODE=PRE"
)
set /a SILENT=0

echo === N7-CROSS-01B COMPILE-ONLY GATE ===
echo N7_CROSS_01B_COMPILE_ONLY=IN_PROGRESS
echo COMPILE_ONLY=YES
echo RUN_EXE=NO
echo GATE_MODE=!GATE_MODE!
echo TCC=!TCC!
echo.

if /i "!GATE_MODE!"=="PRE" goto gate_pre
goto gate_post

:gate_pre
call :compile_must_pass n7_cross_01a_b_class_const 01_CLASS_CONSTANT_CONTROL
call :compile_must_pass n7_cross_01a_c_local_func 02_LOCAL_FUNCTION_CALL_CONTROL
call :compile_must_fail n7_cross_01a_d_global_func 03_GLOBAL_NONCONST_CLASS_FUNCTION_CALL
call :compile_must_fail n7_cross_01a_e_global_const 04_GLOBAL_CONST_CLASS_FUNCTION_CALL
call :compile_must_fail n7_cross_01b_before_main 05_BEFORE_MAIN
call :compile_must_fail n7_cross_01b_return_value 06_RETURN_VALUE
call :compile_must_fail n7_cross_01b_multiple 07_MULTIPLE_GLOBALS
call :compile_must_fail n7_cross_01b_decl_order 08_DECLARATION_ORDER
call :compile_must_fail n7_cross_01b_nested 09_NESTED_VALUE
call :compile_must_fail n7_cross_01b_mixed_n7_02 10_N7_02_MIXED_GLOBALS
call :compile_must_fail n7_00_minimal_static_dyninit 11_AMATERAS_MIN_REPRO_A
call :compile_must_fail n7_cross_01a_minimal_dyninit 11_AMATERAS_MIN_REPRO_B
call :compile_must_pass n7_cross_01a_a_scalar GLOBAL_SCALAR_CONSTANT_INIT_REGRESSION
call :compile_must_fail n7_cross_01b_scalar_probe GLOBAL_SCALAR_NONCONST_DYNAMIC_INIT
goto gate_done

:gate_post
call :compile_must_pass n7_cross_01a_b_class_const 01_CLASS_CONSTANT_CONTROL
call :compile_must_pass n7_cross_01a_c_local_func 02_LOCAL_FUNCTION_CALL_CONTROL
call :compile_must_pass n7_cross_01a_d_global_func 03_GLOBAL_NONCONST_CLASS_FUNCTION_CALL
call :compile_must_pass n7_cross_01a_e_global_const 04_GLOBAL_CONST_CLASS_FUNCTION_CALL
call :compile_must_pass n7_cross_01b_before_main 05_BEFORE_MAIN
call :compile_must_pass n7_cross_01b_return_value 06_RETURN_VALUE
call :compile_must_pass n7_cross_01b_multiple 07_MULTIPLE_GLOBALS
call :compile_must_pass n7_cross_01b_decl_order 08_DECLARATION_ORDER
call :compile_must_pass n7_cross_01b_nested 09_NESTED_VALUE
call :compile_must_pass n7_cross_01b_mixed_n7_02 10_N7_02_MIXED_GLOBALS
call :compile_must_pass n7_00_minimal_static_dyninit 11_AMATERAS_MIN_REPRO_A
call :compile_must_pass n7_cross_01a_minimal_dyninit 11_AMATERAS_MIN_REPRO_B
call :compile_must_pass n7_cross_01a_a_scalar GLOBAL_SCALAR_CONSTANT_INIT_REGRESSION
call :compile_must_fail n7_cross_01b_scalar_probe GLOBAL_SCALAR_NONCONST_DYNAMIC_INIT
goto gate_done

:gate_done
echo.
echo COMPILE_FAIL_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_CROSS_01B_COMPILE_ONLY=FAIL
  popd
  exit /b 1
)
echo N7_CROSS_01B_COMPILE_ONLY=PASS
if /i "!GATE_MODE!"=="PRE" (
  echo NEXT_STEP=01B_WIP_WITH_TCC_EXE_OVERRIDE
) else (
  echo NEXT_STEP=01B_SINGLE_RUN_GATE
)
popd
exit /b 0

:compile_must_pass
set "SRC=%~1"
set "TAG=%~2"
set "EXE=%OUT%\%~1.exe"
set "LOG=%OUT%\%~1.log"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL
  type "!LOG!"
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=COMPILE_PASS
exit /b 0

:compile_must_fail
set "SRC=%~1"
set "TAG=%~2"
set "EXE=%OUT%\%~1.exe"
set "LOG=%OUT%\%~1.log"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
if "!errorlevel!"=="0" (
  echo !TAG!=UNEXPECTED_COMPILE_PASS
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=COMPILE_FAIL_AS_EXPECTED
exit /b 0
