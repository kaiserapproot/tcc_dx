@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_02_03"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0

echo === N7-02-03 GLOBAL IMPLICIT DEFAULT CTOR ===
echo N7_02_03=IN_PROGRESS
echo.

call :run n7_02_00_array GLOBAL_ARRAY_DEFAULT_CTOR
call :run n7_02_00_nested GLOBAL_NESTED_DEFAULT_CTOR
call :run n7_02_03_global_array_nested GLOBAL_ARRAY_OF_NESTED_CLASS

echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_02_03=FAIL
  popd
  exit /b 1
)
echo N7_02_03=PASS
echo N7_02_STATUS=CLOSED
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
