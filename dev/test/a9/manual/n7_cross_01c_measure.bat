@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_cross_01c"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0

echo === N7-CROSS-01C INCLUDED HEADER DYNAMIC INIT ===
echo BASE_COMMIT=88c5df4
echo N7_CROSS_01C=IN_PROGRESS
echo.

call :run PRIMARY_TU_COPY_INIT n7_cross_01c_primary_tu
call :run HEADER_CLASS_DYNAMIC_COPY_INIT n7_cross_01c_header_main
call :run NESTED_INCLUDE_DYNAMIC_INIT n7_cross_01c_nested_main
call :run INCLUDE_GUARD_SINGLE_INIT n7_cross_01c_guard_main
call :run AMATERAS_MIN_REPRO_A n7_00_minimal_static_dyninit
call :run AMATERAS_MIN_REPRO_B n7_cross_01a_minimal_dyninit
call :run WINAPI_INCLUDE_COMPILE n7_cross_01c_winapi_include

echo.
echo --- reentry / registration count ---
set "TCC_N7_CROSS_01C_DIAG=1"
set "LOG=%OUT%\reg_count.log"
"%TCC%" "%~dp0n7_cross_01c_header_main.cpp" -c -o "%OUT%\reg.o" >"!LOG!" 2>&1
set "REG_COUNT=0"
for /f %%A in ('findstr /C:"GLOBAL_COPY_INIT_INTERCEPTED=1" "!LOG!" ^| find /c /v ""') do set "REG_COUNT=%%A"
echo GLOBAL_COPY_INIT_REGISTRATION_COUNT_ACTUAL=!REG_COUNT!
if not "!REG_COUNT!"=="1" (
  echo GLOBAL_COPY_INIT_REENTRANT_REGISTRATION=YES
  set /a SILENT+=1
) else (
  echo GLOBAL_COPY_INIT_REENTRANT_REGISTRATION=NO
  echo GLOBAL_COPY_INIT_REGISTRATION_COUNT_EXPECTED=1
)
set "TCC_N7_CROSS_01C_DIAG="

echo.
echo --- 01B hardening regression (delegated) ---
call "%~dp0n7_cross_01b_hardening_measure.bat"
if not "!errorlevel!"=="0" (
  echo N7_CROSS_01B_HARDENING_REGRESSION=FAIL
  set /a SILENT+=1
) else (
  echo N7_CROSS_01B_HARDENING_REGRESSION=PASS
)

echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_CROSS_01C=FAIL
  popd
  exit /b 1
)
echo HEADER_CLASS_DYNAMIC_COPY_INIT=PASS
echo NESTED_INCLUDE_DYNAMIC_INIT=PASS
echo N7_CROSS_01C=PASS
popd
exit /b 0

:run
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~2.exe"
set "LOG=%OUT%\%~2.log"
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
  echo !TAG!=RUN_FAIL rc=!errorlevel!
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=PASS
exit /b 0
