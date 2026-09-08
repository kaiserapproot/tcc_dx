@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_02_02"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0

echo === N7-02-02 IMPLICIT DEFAULT CTOR SYNTHESIS ===
echo N7_02_02=IN_PROGRESS
echo DESIGN=DESIGN_A_SYNTHETIC_CTOR_SYMBOL
echo.

call :probe_run n7_02_02_decisive_order IMPLICIT_OUTER_DEFAULT_CTOR
call :probe_run n7_02_02_storage_global GLOBAL_IMPLICIT_DEFAULT_CTOR
call :probe_run n7_02_02_storage_local_static LOCAL_STATIC_IMPLICIT_DEFAULT_CTOR
call :compile_only n7_02_02_nested NESTED_IMPLICIT_DEFAULT_CTOR_PROPAGATION

call :compile_must_fail n7_02_02_preflight_overload_03 N7_01_NO_DEFAULT_CTOR
call :compile_must_fail_path ..\negative\no_viable_default_ctor_local N7_01_LOCAL_NO_DEFAULT
call :compile_must_fail_path ..\negative\no_viable_default_ctor_global N7_01_GLOBAL_NO_DEFAULT
call :compile_must_fail_path ..\negative\no_viable_default_ctor_local_static N7_01_LOCAL_STATIC_NO_DEFAULT
call :compile_must_fail_path ..\negative\no_viable_default_ctor_array N7_01_ARRAY_NO_DEFAULT
call :compile_must_fail n7_00_case_d_member_no_default N7_01_MEMBER_NO_DEFAULT
call :compile_must_fail n7_00_case_e_base_no_default N7_01_BASE_NO_DEFAULT

echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if "!SILENT!"=="0" (
  echo N7_02_02=PASS
  popd
  exit /b 0
)
echo N7_02_02=FAIL
popd
exit /b 1

:probe_run
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
set "EXE=%OUT%\%~1.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL
  type "!LOG!"
  exit /b 1
)
"!EXE!" >nul 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_FAIL
  exit /b 1
)
echo !TAG!=PASS
goto :eof

:compile_only
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
"%TCC%" "%~dp0%SRC%.cpp" -c -o "%OUT%\%~1.o" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL
  type "!LOG!"
  exit /b 1
)
echo !TAG!=PASS
goto :eof

:compile_must_fail
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
"%TCC%" "%~dp0%SRC%.cpp" -c -o "%OUT%\%~1.o" >"!LOG!" 2>&1
if "!errorlevel!"=="0" (
  echo !TAG!=SILENT_ACCEPTANCE
  set /a SILENT+=1
  goto :eof
)
echo !TAG!=FAIL_CLOSED
goto :eof

:compile_must_fail_path
set "SRC=%~1"
set "TAG=%~2"
for %%I in ("%~dp0%SRC%.cpp") do set "ABSSRC=%%~fI"
set "LOG=%OUT%\%~nx1.log"
"%TCC%" "!ABSSRC!" -c -o "%OUT%\%~nx1.o" >"!LOG!" 2>&1
if "!errorlevel!"=="0" (
  echo !TAG!=SILENT_ACCEPTANCE
  set /a SILENT+=1
  goto :eof
)
echo !TAG!=FAIL_CLOSED
goto :eof
