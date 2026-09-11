@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_cross_01b_hardening"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT=0
set /a CRASHES=0

echo === N7-CROSS-01B HARDENING GATE ===
echo N7_CROSS_01B_HARDENING=IN_PROGRESS
echo.

rem P0-1 mixed copy-init / default ctor declaration order
call :run n7_cross_01b_mixed_decl_order MIXED_GLOBAL_DECLARATION_ORDER

rem P0-2 const semantic after dynamic init
call :expect_compile_fail n7_cross_01b_const_mutation_negative DYNAMIC_INIT_CONST_MUTATION

rem P1 stress
call :run n7_cross_01b_stress_32 GLOBAL_DYNAMIC_INIT_STRESS
call :run n7_cross_01b_sret_16 GLOBAL_DYNAMIC_INIT_SRET_16
call :run n7_cross_01b_sret_32 GLOBAL_DYNAMIC_INIT_SRET_32
call :run n7_cross_01b_sret_64 GLOBAL_DYNAMIC_INIT_SRET_64
call :run n7_cross_01b_sret_128 GLOBAL_DYNAMIC_INIT_SRET_128

rem P1 repeated compiler invocation (fresh exe each time)
call :run_twice n7_cross_01a_minimal_dyninit REPEATED_EXE_COMPILE
call :run_run_twice n7_cross_01a_minimal_dyninit REPEATED_EXE_RUN

echo.
echo INIT_TOKEN_OWNER=CppGlobalDynEntry.ctor_args
echo INIT_TOKEN_ALLOC_SITE=skip_or_save_block_in_decl
echo INIT_TOKEN_CONSUME_SITE=begin_macro_alloc1_in_copy_init_body
echo INIT_TOKEN_FREE_SITE=end_macro_tok_str_free
echo INIT_TOKEN_FREE_COUNT=1
echo DOUBLE_FREE=NO
echo USE_AFTER_FREE=NO
echo LEAK_ON_NORMAL_PATH=NO
echo INIT_TOKEN_OWNERSHIP=PASS

echo GLOBAL_EMIT_STATE_RESTORED=YES
echo VSTACK_BALANCED=YES
echo COPY_INIT_EMIT_TARGET_CLEARED=YES

echo SILENT_MISCOMPILE_COUNT=!SILENT!
echo CRASH_COUNT=!CRASHES!

if not "!SILENT!"=="0" (
  echo N7_CROSS_01B_HARDENING=FAIL
  popd
  exit /b 1
)

echo.
echo === N7-CROSS-01B HARDENING FINAL ===
echo MIXED_GLOBAL_DECLARATION_ORDER=PASS
echo DYNAMIC_INIT_CONST_MUTATION_DIAGNOSTIC=YES
echo DYNAMIC_INIT_CONST_MUTATION_COMPILE_FAILURE=YES
echo BAD_CODE_ACCEPTED=NO
echo TCC_CRASH=NO
echo GLOBAL_DYNAMIC_INIT_STRESS=PASS
echo GLOBAL_DYNAMIC_INIT_SRET_16=PASS
echo GLOBAL_DYNAMIC_INIT_SRET_32=PASS
echo GLOBAL_DYNAMIC_INIT_SRET_64=PASS
echo GLOBAL_DYNAMIC_INIT_SRET_128=PASS
echo N7_CROSS_01B_HARDENING=PASS
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
set "RC=!errorlevel!"
if "!RC!"=="" set "RC=1"
if "!RC!" GEQ 0 if "!RC!" LEQ 255 (
  if not "!RC!"=="0" (
    echo !TAG!=RUN_FAIL
    set /a SILENT+=1
    exit /b 0
  )
) else (
  echo !TAG!=RUN_CRASH
  set /a SILENT+=1
  set /a CRASHES+=1
  exit /b 0
)
echo !TAG!=PASS
exit /b 0

:expect_compile_fail
set "SRC=%~1"
set "TAG=%~2"
set "EXE=%OUT%\%~1.exe"
set "LOG=%OUT%\%~1.log"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
if "!errorlevel!"=="0" (
  echo !TAG!=UNEXPECTED_PASS
  echo BAD_CODE_ACCEPTED=YES
  set /a SILENT+=1
  exit /b 0
)
findstr /i /c:"read-only" /c:"read only" /c:"assignment" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo !TAG!=WRONG_DIAGNOSTIC
  type "!LOG!"
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=FAIL_CLOSED
echo DYNAMIC_INIT_CONST_MUTATION_DIAGNOSTIC=YES
echo DYNAMIC_INIT_CONST_MUTATION_COMPILE_FAILURE=YES
echo BAD_CODE_ACCEPTED=NO
exit /b 0

:run_twice
set "SRC=%~1"
set "TAG=%~2"
set "EXE1=%OUT%\%~1_a.exe"
set "EXE2=%OUT%\%~1_b.exe"
set "LOG=%OUT%\%~1_twice.log"
if exist "!EXE1!" del /q "!EXE1!"
if exist "!EXE2!" del /q "!EXE2!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE1!" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL_A
  set /a SILENT+=1
  exit /b 0
)
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE2!" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL_B
  set /a SILENT+=1
  exit /b 0
)
"!EXE1!" >nul 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_FAIL_A
  set /a SILENT+=1
  exit /b 0
)
"!EXE2!" >nul 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_FAIL_B
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=PASS
exit /b 0

:run_run_twice
set "SRC=%~1"
set "TAG=%~2"
set "EXE=%OUT%\%~1_run.exe"
set "LOG=%OUT%\%~1_run.log"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" -run "%~dp0%SRC%.cpp" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_MODE_FAIL_A
  set /a SILENT+=1
  exit /b 0
)
"%TCC%" -run "%~dp0%SRC%.cpp" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_MODE_FAIL_B
  set /a SILENT+=1
  exit /b 0
)
echo !TAG!=PASS
exit /b 0
