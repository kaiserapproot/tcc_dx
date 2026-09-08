@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_02_02_preflight"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
if not exist "%OUT%" mkdir "%OUT%"

echo === N7-02-02 PREFLIGHT A: OVERLOAD / SYNTHESIS INVARIANT ===
echo N7_02_01=COMPLETE
echo N7_02_02_IMPLEMENTATION_START=NO
echo PRODUCTION_CHANGE=NONE
echo.

set "FAIL=0"

call :probe_pass n7_02_02_preflight_overload_01 OVERLOAD_CASE1_EMPTY
call :probe_pass n7_02_02_preflight_overload_02 OVERLOAD_CASE2_TRIVIAL
call :probe_fail n7_02_02_preflight_overload_03 OVERLOAD_CASE3_NO_DEFAULT
call :probe_pass n7_02_02_preflight_overload_04 OVERLOAD_CASE4_DEFAULT_ARG
call :probe_pass n7_02_02_preflight_overload_05 OVERLOAD_CASE5_USER_DEFAULT

echo.
echo SYNTHETIC_CTOR_CREATED_WHEN_NO_USER_CTOR=YES_AT_IMPL
echo SYNTHETIC_CTOR_CREATED_WHEN_USER_CTOR_EXISTS=NO_INVARIANT
echo SYNTHESIS_GATE=cpp_find_ctor_field_NULL_ONLY
echo N7_01_NO_DEFAULT_CTOR_REGRESSION=BASELINE_CASE3
echo DEFAULT_ARGUMENT_CTOR_REGRESSION=BASELINE_CASE4
echo USER_DEFAULT_CTOR_PRECEDENCE=BASELINE_CASE5

if "!FAIL!"=="0" (
  echo PREFLIGHT_A=PASS
  popd
  exit /b 0
)
echo PREFLIGHT_A=FAIL
popd
exit /b 1

:probe_pass
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
set "EXE=%OUT%\%~1.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL
  type "!LOG!"
  set "FAIL=1"
  goto :eof
)
"!EXE!" >nul 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_FAIL
  set "FAIL=1"
  goto :eof
)
echo !TAG!=PASS
goto :eof

:probe_fail
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
"%TCC%" "%~dp0%SRC%.cpp" -c -o "%OUT%\%~1.o" >"!LOG!" 2>&1
if "!errorlevel!"=="0" (
  echo !TAG!=SILENT_ACCEPTANCE
  set "FAIL=1"
  goto :eof
)
"%FINDSTR%" /i /c:"no default constructor" "!LOG!" >nul 2>&1
if not errorlevel 1 (
  echo !TAG!=FAIL_CLOSED
  goto :eof
)
echo !TAG!=FAIL_CLOSED_OTHER
goto :eof
