@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_03"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0

echo === N7-03 IMPLICIT COPY ASSIGNMENT ===
echo BASE_COMMIT=304607d
echo N7_03=IN_PROGRESS
echo.

call :run TRIVIAL_SCALAR_COPY_ASSIGN n7_03_trivial_scalar
call :run TRIVIAL_MULTI_MEMBER_COPY_ASSIGN n7_03_trivial_multi
call :run NESTED_CLASS_COPY_ASSIGN n7_03_nested_user_assign
call :run NESTED_IMPLICIT_COPY_ASSIGN n7_03_nested_implicit
call :run BASE_CLASS_COPY_ASSIGN n7_03_base_class
call :run ARRAY_MEMBER_COPY_ASSIGN n7_03_array_member
call :run SELF_ASSIGNMENT n7_03_self_assign
call :run CHAIN_ASSIGNMENT n7_03_chain_assign
call :run USER_DEFINED_OPERATOR_ASSIGN_PRECEDENCE n7_03_user_op_precedence
call :run AMATERAS_IMPLICIT_COPY_ASSIGN_MIN_REPRO n7_03_amateras_bone_key

echo.
call :expect_fail CONST_MEMBER_COMPILE_FAILURE n7_03_const_member_negative
call :expect_fail REFERENCE_MEMBER_COMPILE_FAILURE n7_03_reference_member_negative
call :expect_fail UNASSIGNABLE_MEMBER_COMPILE_FAILURE n7_03_unassignable_member_negative
call :expect_fail UNASSIGNABLE_BASE_COMPILE_FAILURE n7_03_unassignable_base_negative

echo.
call :delegate n7_cross_01c_measure.bat N7_CROSS_01C_REGRESSION
call :delegate n7_cross_01b_hardening_measure.bat N7_CROSS_01B_REGRESSION

echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_03=FAIL
  popd
  exit /b 1
)
echo IMPLICIT_COPY_ASSIGN_RETURNS_LHS_REFERENCE=YES
echo N7_03=PASS
popd
exit /b 0

:run
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~2.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"%OUT%\%~2.log" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL
  type "%OUT%\%~2.log"
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
set "TAG=%~1"
set "SRC=%~2"
"%TCC%" "%~dp0%SRC%.cpp" -c -o "%OUT%\%~2.o" >"%OUT%\%~2.log" 2>&1
if "!errorlevel!"=="0" (
  echo !TAG!=UNEXPECTED_PASS
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=FAIL_CLOSED
exit /b 0

:delegate
call "%~dp0%~1" >nul 2>&1
if not "!errorlevel!"=="0" (
  echo %~2=FAIL
  set /a SILENT+=1
) else (
  echo %~2=PASS
)
exit /b 0
