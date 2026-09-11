@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_02_retest"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0

echo === N7-02 AMATERAS CONSUMER RETEST GATE ===
echo N7_02_RETEST=IN_PROGRESS
echo BLOCKER_CLASS=MEMBER_BINOP_OVERLOAD_RESOLUTION_BY_ARG_TYPE
echo.

call :run n7_02_retest_binop_scalar_ovl MEMBER_BINOP_SCALAR_OVERLOAD
call :run n7_02_retest_binop_explicit MEMBER_BINOP_EXPLICIT_CALL
call :run n7_02_retest_default_ctor_order DEFAULT_CTOR_DECL_ORDER

echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_02_RETEST=FAIL
  popd
  exit /b 1
)
echo N7_02_RETEST=PASS
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
