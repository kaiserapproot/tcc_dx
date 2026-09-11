@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
goto :n7_07d_main

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

:expect_pass_exit
set "TAG=%~1"
set "SRC=%~2"
set "WANT=%~3"
set "EXE=%OUT%\%~n2.exe"
"%TCC%" "%~dp0%SRC%" -o "%EXE%" >"%OUT%\%~n2.log" 2>&1
if errorlevel 1 (
  echo !TAG!=COMPILE_FAIL
  set /a BAD=1
  exit /b 0
)
"%EXE%" >"%OUT%\%~n2_run.log" 2>&1
set "GOT=!errorlevel!"
if not "!GOT!"=="!WANT!" (
  echo !TAG!=COUNT_!GOT!_WANT_!WANT!
  set /a BAD=1
) else (
  echo !TAG!=PASS
)
exit /b 0

:n7_07d_main
set "TCC=..\..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_07d_out"
if not exist "%OUT%" mkdir "%OUT%"
set /a BAD=0
set /a SILENT=0

echo === N7-07D STATIC LOCAL CLASS ARRAY FAIL-CLOSED ===
echo TCC=%TCC%
echo.

call :expect_fail STATIC_LOCAL_CLASS_ARRAY_COMPILE n7_07d_static_array_neg.cpp "implicit default construction of static local class array is unsupported"
call :expect_fail STATIC_LOCAL_CLASS_ARRAY_DIAGNOSTIC n7_07d_static_array_neg.cpp "implicit default construction of static local class array is unsupported"
call :expect_fail STATIC_LOCAL_MULTIDIM_CLASS_ARRAY_COMPILE n7_07d_static_multidim_array_neg.cpp "implicit default construction of static local class array is unsupported"
call :expect_fail STATIC_LOCAL_MULTIDIM_DIAGNOSTIC n7_07d_static_multidim_array_neg.cpp "implicit default construction of static local class array is unsupported"
call :expect_pass STATIC_LOCAL_SCALAR_CLASS n7_07d_static_scalar_control.cpp
call :expect_pass STATIC_LOCAL_SCALAR_INIT_ONCE n7_07d_static_scalar_control.cpp
call :expect_pass AUTOMATIC_LOCAL_CLASS_ARRAY n7_07d_auto_array_control.cpp
call :expect_pass_exit AUTOMATIC_LOCAL_CLASS_ARRAY_CTOR_COUNT n7_07d_auto_array_control.cpp 0
call :expect_pass GLOBAL_CLASS_ARRAY n7_07d_global_array_control.cpp
call :expect_pass TRIVIAL_STATIC_LOCAL_STRUCT_ARRAY n7_07d_trivial_static_array.cpp
echo.

echo === N7-07D SUMMARY ===
if "!SILENT!"=="0" (
  echo STATIC_LOCAL_CLASS_ARRAY_FAIL_CLOSED=PASS
) else (
  echo STATIC_LOCAL_CLASS_ARRAY_FAIL_CLOSED=FAIL
)
echo BAD_CODE_ACCEPTED=!SILENT!
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!BAD!"=="0" (
  echo N7_07D=FAIL
  popd
  exit /b 1
)
echo N7_07D=PASS
popd
exit /b 0
