@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_04"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0
echo === N7-04 CLASS MEMBER ARRAY DEFAULT CONSTRUCTION ===
echo BASE_COMMIT=c3302f7
call :run SCALAR_CLASS_MEMBER_REGRESSION n7_04a_scalar_member.cpp
call :run TRIVIAL_MEMBER_ARRAY n7_04a_trivial_array.cpp
call :run MEMBER_ARRAY_4 n7_04_member_array_count.cpp
call :run MEMBER_DECLARATION_ORDER n7_04_decl_order.cpp
call :run USER_OUTER_CTOR_MEMBER_ARRAY ..\n7_04_explicit_outer_member_array_ctor.cpp
call :run SYNTHETIC_OUTER_MEMBER_ARRAY ..\n7_04_implicit_member_array.cpp
call :run GLOBAL_OUTER_MEMBER_ARRAY n7_04_global_outer.cpp
call :run AMATERAS_CLASS_MEMBER_ARRAY_MIN_REPRO n7_04_amateras_gl_state.cpp
call :delegate n7_03_measure.bat N7_03_REGRESSION
call :expect_fail NO_DEFAULT_CTOR_FAIL_CLOSED n7_04_no_default_ctor_neg.cpp
call :expect_fail ARRAY_ELEMENT_NONTRIVIAL_DTOR n7_04a_dtor_probe.cpp
echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_04=FAIL
  popd
  exit /b 1
)
echo N7_04=PASS
popd
exit /b 0
:run
set "TAG=%~1"
set "SRC=%~2"
"%TCC%" "%~dp0%SRC%" -o "%OUT%\%~2.exe" >"%OUT%\%~2.log" 2>&1
if not "!errorlevel!"=="0" (echo !TAG!=COMPILE_FAIL & set /a SILENT+=1 & exit /b 0)
"%OUT%\%~2.exe" >nul 2>&1
if not "!errorlevel!"=="0" (echo !TAG!=RUN_FAIL & set /a SILENT+=1 & exit /b 0)
echo !TAG!=PASS
exit /b 0
:expect_fail
set "TAG=%~1"
set "SRC=%~2"
"%TCC%" "%~dp0%SRC%" -c -o "%OUT%\%~2.o" >"%OUT%\%~2.log" 2>&1
if "!errorlevel!"=="0" (echo !TAG!=UNEXPECTED_PASS & set /a SILENT+=1) else (echo !TAG!=FAIL_CLOSED)
exit /b 0
:delegate
call "%~dp0%~1" >nul 2>&1
if not "!errorlevel!"=="0" (echo %~2=FAIL & set /a SILENT+=1) else (echo %~2=PASS)
exit /b 0
