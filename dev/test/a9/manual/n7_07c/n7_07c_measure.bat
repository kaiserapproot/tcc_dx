@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
goto :n7_07c_main

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

:n7_07c_main
set "TCC=..\..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_07c_out"
if not exist "%OUT%" mkdir "%OUT%"
set /a BAD=0
set /a SILENT=0

echo === N7-07C LOCAL CLASS ARRAY CONSTRUCTION ===
echo TCC=%TCC%
echo.

call :expect_pass LOCAL_CLASS_ARRAY n7_07c_local_array_ctor.cpp
call :expect_pass MULTIDIM_LOCAL_CLASS_ARRAY n7_07c_local_array_multidim.cpp
call :expect_pass IMPLICIT_CTOR_LOCAL_ARRAY n7_07c_local_array_implicit_ctor.cpp
call :expect_fail LOCAL_ARRAY_NO_DEFAULT_CTOR n7_07c_local_array_no_default_ctor.cpp "default constructor"
call :expect_fail LOCAL_CLASS_ARRAY_NONTRIVIAL_DTOR n7_07c_local_array_dtor_failclosed.cpp "destruction of local class array"
call :expect_fail POLYMORPHIC_LOCAL_CLASS_ARRAY n7_07c_polymorphic_failclosed.cpp "polymorphic local class array"
call :expect_pass TRIVIAL_LOCAL_STRUCT_ARRAY n7_07c_trivial_array.cpp
call :expect_pass SCALAR_LOCAL_CLASS_DEFAULT_CTOR n7_07c_scalar_control.cpp
call :expect_pass GLOBAL_CLASS_ARRAY_DEFAULT_CTOR n7_07c_global_control.cpp
call :expect_pass CLASS_MEMBER_ARRAY_DEFAULT_CTOR n7_07c_member_control.cpp
echo.

echo === N7-07C SUMMARY ===
echo BAD_CODE_ACCEPTED=!SILENT!
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!BAD!"=="0" (
  echo N7_07C=FAIL
  popd
  exit /b 1
)
echo N7_07C=PASS
popd
exit /b 0
