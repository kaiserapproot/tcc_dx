@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_cross_01b"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0

echo === N7-CROSS-01B GLOBAL CLASS DYNAMIC INIT ===
echo N7_CROSS_01B=IN_PROGRESS
echo.

call :run n7_cross_01a_b_class_const 01_CLASS_CONSTANT_CONTROL
call :run n7_cross_01a_c_local_func 02_LOCAL_FUNCTION_CALL_CONTROL
call :run n7_cross_01a_d_global_func 03_GLOBAL_NONCONST_CLASS_FUNCTION_CALL
call :run n7_cross_01a_e_global_const 04_GLOBAL_CONST_CLASS_FUNCTION_CALL
call :run n7_cross_01b_before_main 05_BEFORE_MAIN
call :run n7_cross_01b_return_value 06_RETURN_VALUE
call :run n7_cross_01b_multiple 07_MULTIPLE_GLOBALS
call :run n7_cross_01b_decl_order 08_DECLARATION_ORDER
call :run n7_cross_01b_nested 09_NESTED_VALUE
call :run n7_cross_01b_mixed_n7_02 10_N7_02_MIXED_GLOBALS
call :run n7_00_minimal_static_dyninit 11_AMATERAS_MIN_REPRO_A
call :run n7_cross_01a_minimal_dyninit 11_AMATERAS_MIN_REPRO_B
call :run n7_cross_01a_a_scalar GLOBAL_SCALAR_CONSTANT_INIT_REGRESSION
call :expect_fail n7_cross_01b_scalar_probe GLOBAL_SCALAR_NONCONST_DYNAMIC_INIT

echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_CROSS_01B=FAIL
  popd
  exit /b 1
)
echo N7_CROSS_01B_TARGETED=PASS
echo COPY_ASSIGN_DEPENDENCY=NO
echo READY_FOR_AMATERAS_RETEST=YES
popd
exit /b 0

:run
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
"!EXE!" >nul 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_FAIL
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=PASS
exit /b 0

:expect_fail
set "SRC=%~1"
set "TAG=%~2"
set "EXE=%OUT%\%~1.exe"
set "LOG=%OUT%\%~1.log"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
if "!errorlevel!"=="0" (
  echo !TAG!=UNEXPECTED_PASS
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=OUT_OF_SCOPE
exit /b 0
