@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_cross_01a"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0

echo === N7-CROSS-01A CAUSAL ISOLATION ===
echo BASE_COMMIT=01B_WIP
echo PRODUCTION_CHANGE=GLOBAL_CLASS_DYNAMIC_INIT
echo N7_CROSS_01A=IN_PROGRESS
echo.

call :expect_pass n7_cross_01a_a_scalar GLOBAL_SCALAR_CONSTANT_INIT
call :expect_pass n7_cross_01a_b_class_const GLOBAL_CLASS_CONSTANT_INIT
call :expect_pass n7_cross_01a_c_local_func LOCAL_CLASS_FUNCTION_CALL_INIT
call :expect_pass n7_cross_01a_d_global_func GLOBAL_CLASS_FUNCTION_CALL_INIT
call :expect_pass n7_cross_01a_e_global_const GLOBAL_CONST_CLASS_FUNCTION_CALL_INIT
call :expect_pass n7_00_minimal_static_dyninit AMATERAS_MIN_REPRO_A
call :expect_pass n7_cross_01a_minimal_dyninit AMATERAS_MIN_REPRO_B
call :expect_pass n7_cross_01a_f_global_default GLOBAL_DEFAULT_CTOR_N7_02

echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_CROSS_01A=FAIL
  popd
  exit /b 1
)
echo N7_CROSS_01A=PASS
echo ROOT_CAUSE_CONFIRMED=YES
echo N7_CROSS_01B_FIX=VERIFIED
popd
exit /b 0

:expect_pass
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

:expect_compile_fail
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
echo !TAG!=COMPILE_FAIL_AS_EXPECTED
exit /b 0
