@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_03a"
if not exist "%OUT%" mkdir "%OUT%"

echo === N7-03A CAUSAL AUTHORITY ===
echo BASE_COMMIT=304607d
echo PRODUCTION_CHANGE=NONE
echo.

call :run TRIVIAL_SCALAR_COPY_ASSIGN n7_03a_trivial_scalar.cpp
call :run TRIVIAL_MULTI_MEMBER_COPY_ASSIGN n7_03a_trivial_multi.cpp
call :run EXPLICIT_COPY_ASSIGN_CONTROL n7_03a_explicit_control.cpp
call :expect_fail NESTED_USER_ASSIGN_BEFORE n7_03a_nested_user.cpp
call :run NESTED_TRIVIAL_IMPLICIT_ASSIGN n7_03a_nested_implicit.cpp
call :run BASE_USER_ASSIGN_BEFORE n7_03a_base_user.cpp
call :expect_fail ARRAY_CLASS_MEMBER_ASSIGN_BEFORE n7_03a_array_member.cpp
call :run CHAIN_ASSIGNMENT n7_03a_chain.cpp
call :expect_fail CONST_MEMBER_FAIL_CLOSED n7_03a_const_neg.cpp
call :expect_fail REFERENCE_MEMBER_FAIL_CLOSED n7_03a_ref_neg.cpp
call :expect_fail UNASSIGNABLE_MEMBER_FAIL_CLOSED n7_03a_unassign_member_neg.cpp
call :expect_fail UNASSIGNABLE_BASE_FAIL_CLOSED n7_03a_unassign_base_neg.cpp

echo.
echo N7_03A=MEASURED
popd
exit /b 0

:run
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~2.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%" -o "!EXE!" >"%OUT%\%~2.log" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL
  exit /b 0
)
"!EXE!" >nul 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_FAIL
  exit /b 0
)
echo !TAG!=PASS
exit /b 0

:expect_fail
set "TAG=%~1"
set "SRC=%~2"
"%TCC%" "%~dp0%SRC%" -c -o "%OUT%\%~2.o" >"%OUT%\%~2.log" 2>&1
if "!errorlevel!"=="0" (
  echo !TAG!=UNEXPECTED_PASS
) else (
  echo !TAG!=COMPILE_FAIL
)
exit /b 0
