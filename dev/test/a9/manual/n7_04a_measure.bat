@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_04a"
if not exist "%OUT%" mkdir "%OUT%"
echo === N7-04A CAUSAL ===
echo BASE_COMMIT=c3302f7
echo PRODUCTION_CHANGE=NONE
call :expect_fail IMPLICIT_OUTER_MEMBER_ARRAY_BEFORE ..\negative\implicit_class_array_member.cpp
call :expect_fail EXPLICIT_OUTER_MEMBER_ARRAY_BEFORE ..\negative\explicit_outer_member_array_ctor.cpp
call :run SCALAR_CLASS_MEMBER_DEFAULT_CTOR n7_04a_scalar_member.cpp
call :run TRIVIAL_MEMBER_ARRAY n7_04a_trivial_array.cpp
call :expect_fail MULTIDIM_MEMBER_ARRAY_BEFORE n7_04a_multidim.cpp
call :probe_dtor n7_04a_dtor_probe.cpp
popd
exit /b 0
:run
set "TAG=%~1"
set "SRC=%~2"
"%TCC%" "%~dp0%SRC%" -o "%OUT%\%~2.exe" >"%OUT%\%~2.log" 2>&1
if not "!errorlevel!"=="0" (echo !TAG!=COMPILE_FAIL & exit /b 0)
"%OUT%\%~2.exe" >nul 2>&1
if not "!errorlevel!"=="0" (echo !TAG!=RUN_FAIL & exit /b 0)
echo !TAG!=PASS
exit /b 0
:expect_fail
set "TAG=%~1"
set "SRC=%~2"
"%TCC%" "%~dp0%SRC%" -c -o "%OUT%\%~2.o" >"%OUT%\%~2.log" 2>&1
if "!errorlevel!"=="0" (echo !TAG!=UNEXPECTED_PASS) else (echo !TAG!=COMPILE_FAIL)
exit /b 0
:probe_dtor
set "TAG=ARRAY_ELEMENT_NONTRIVIAL_DTOR"
"%TCC%" "%~dp0%~1" -c -o "%OUT%\%~1.o" >"%OUT%\%~1.log" 2>&1
if "!errorlevel!"=="0" (echo !TAG!=UNEXPECTED_PASS) else (echo !TAG!=COMPILE_FAIL)
exit /b 0
