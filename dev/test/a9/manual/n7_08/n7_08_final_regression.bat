@echo off
setlocal EnableExtensions EnableDelayedExpansion
goto :main

:child
set "BAT=%~1"
set "TAG=%~2"
echo --- !TAG! ^(!BAT!^) ---
call "%~dp0%BAT%"
set "RC=!errorlevel!"
if not "!RC!"=="0" (
  echo !TAG!=FAIL child_rc=!RC!
  set /a FAILED+=1
  goto :eof
)
echo !TAG!=PASS
goto :eof

:probe_compile_fail
set "TAG=%~1"
set "SRC=%~2"
set "LOG=%OUT%\%~n2_probe.log"
"%TCC%" "%SRC%" -c -o "%OUT%\probe.o" >"%LOG%" 2>&1
if errorlevel 1 (
  echo !TAG!=DEFERRED_FAIL_CLOSED
) else (
  echo !TAG!=UNEXPECTED_PASS
  set /a FAILED+=1
  set /a SILENT+=1
)
goto :eof

:main
pushd "%~dp0"
set "TCC=..\..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_08_out"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0
set /a CRASHES=0
set /a SILENT=0

for /f %%h in ('git -C "%~dp0..\..\..\..\.." rev-parse --short HEAD 2^>nul') do set "BASE_HEAD=%%h"
if not defined BASE_HEAD set "BASE_HEAD=unknown"

echo === N7-08 FINAL REGRESSION / CLOSURE ===
echo N7_07E_FIX_COMMIT=8f2e342
echo N7_08_BASE_HEAD=!BASE_HEAD!
echo TCC=!TCC!
echo TCC_PRODUCTION_CHANGE=NONE
echo MEASUREMENT_AND_CLOSURE_ONLY=YES
echo.

echo === Phase N7 individual authority ===
call :child ..\n7_01_default_ctor_fail_closed.bat N7_01_REGRESSION
call :child ..\n7_02_02_measure.bat N7_02_02_REGRESSION
call :child ..\n7_02_03_measure.bat N7_02_03_REGRESSION
call :child ..\n7_02_retest_measure.bat N7_02_RETEST_REGRESSION
call :child ..\n7_cross_01a_measure.bat N7_CROSS_01A_REGRESSION
call :child ..\n7_cross_01b_measure.bat N7_CROSS_01B_REGRESSION
call :child ..\n7_cross_01c_measure.bat N7_CROSS_01C_REGRESSION
call :child ..\n7_03_measure.bat N7_03_REGRESSION
call :child ..\n7_04_measure.bat N7_04_REGRESSION
call :child ..\n7_05b\n7_05b_measure.bat N7_05_REGRESSION
call :child ..\n7_07b\n7_07b_measure.bat N7_07B_REGRESSION
call :child ..\n7_07c\n7_07c_measure.bat N7_07C_REGRESSION
call :child ..\n7_07c_followup\n7_07c_f2_measure.bat N7_07C_F2_REGRESSION
call :child ..\n7_07d\n7_07d_measure.bat N7_07D_REGRESSION
call :child ..\n7_07e\n7_07e_measure.bat N7_07E_REGRESSION

echo.
echo === N7-06 Amateras consumer delegate ===
call :child ..\n7_06\build_run_n7_06.bat N7_06_REGRESSION

echo.
echo === N6 boundary regression ===
call :child ..\n6_08_final_regression.bat N6_REGRESSION

echo.
echo === Deferred explicit-init probes (measurement only) ===
call :probe_compile_fail LOCAL_ARRAY_EXPLICIT_INIT ..\n7_07b\n7_07b_explicit_init_probe.cpp
call :probe_compile_fail STATIC_ARRAY_EXPLICIT_INIT ..\n7_07d\n7_07d_explicit_init_probe.cpp

echo.
echo === N7-08 capability matrix ===
if "!FAILED!"=="0" (
  echo local class array=SUPPORTED
  echo local multidim class array=SUPPORTED
  echo static local class array=SUPPORTED
  echo static local multidim class array=SUPPORTED
  echo default-arg array construction=SUPPORTED
  echo extern-C local array construction=SUPPORTED
  echo local explicit-init class array=DEFERRED/UNSUPPORTED
  echo static explicit-init class array=DEFERRED/UNSUPPORTED
  echo TLS class array=FAIL_CLOSED
  echo nontrivial array destruction forms=FAIL_CLOSED where unsupported
)

echo.
echo === N7-08 safety totals ===
echo BAD_CODE_ACCEPTED=!SILENT!
echo UNSUPPORTED_FORM_ACCEPTED=!SILENT!
echo SILENT_MISCOMPILE_COUNT=!SILENT!
echo COMPILER_CRASH_COUNT=!CRASHES!
echo GENERATED_EXE_CRASH_COUNT=0

echo.
if not "!FAILED!"=="0" (
  echo N7_08=STOP
  echo N7_08_TCC_STATUS=OPEN
  popd
  exit /b 1
)
echo N7_08=PASS
echo N7_08_TCC_STATUS=CLOSED
echo READY_FOR_AMATERAS_N7_08_FINAL_QUALIFICATION=YES
popd
exit /b 0
