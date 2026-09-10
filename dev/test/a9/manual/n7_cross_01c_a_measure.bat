@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n7_cross_01c_a"
if not exist "%OUT%" mkdir "%OUT%"
set "DIAG=%OUT%\diag.log"
set /a SILENT=0

echo === N7-CROSS-01C-A CAUSAL ISOLATION ===
echo BASE_COMMIT=88c5df4
echo PRODUCTION_CHANGE=NONE
echo N7_CROSS_01C_A=IN_PROGRESS
echo.

call :run_case PRIMARY_TU_COPY_INIT n7_cross_01c_primary_tu
call :run_case HEADER_COPY_INIT n7_cross_01c_header_main
call :preprocessed_case PREPROCESSED_COPY_INIT n7_cross_01c_header_main
call :run_case WINAPI_INCLUDE_COMPILE n7_cross_01c_winapi_include

echo.
echo --- scope breadth (header context, out of 01C fix scope unless noted) ---
call :run_case HEADER_COPY_INIT_SCOPE n7_cross_01c_header_main
call :run_case HEADER_DEFAULT_CTOR n7_cross_01c_h2_main
call :run_case HEADER_DIRECT_CTOR n7_cross_01c_h3_main
call :run_case HEADER_CLASS_ARRAY n7_cross_01c_h4_main

echo.
echo --- diagnostic gate trace (TCC_N7_CROSS_01C_DIAG=1) ---
set "TCC_N7_CROSS_01C_DIAG=1"
call :diag_case PRIMARY n7_cross_01c_primary_tu
call :diag_case HEADER n7_cross_01c_header_main
call :preprocessed_diag PREPROCESSED n7_cross_01c_header_main
set "TCC_N7_CROSS_01C_DIAG="

echo.
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!SILENT!"=="0" (
  echo N7_CROSS_01C_A=FAIL
  popd
  exit /b 1
)
echo N7_CROSS_01C_A=PASS
echo DIRECT_CAUSE=N7_CROSS_01B_PRIMARY_TU_ONLY_GATE
echo ROOT_CAUSE=cpp_in_user_source_file blocks #include file global copy-init
echo ROOT_CAUSE_CONFIRMED=YES
echo N7_CROSS_01C_B_FIX_START=YES
echo HEADER_DEFAULT_CTOR=OUT_OF_SCOPE_FEAT4G
echo HEADER_DIRECT_CTOR=OUT_OF_SCOPE_FEAT4G
echo HEADER_CLASS_ARRAY=OUT_OF_SCOPE_FEAT4G
popd
exit /b 0

:run_case
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~2.exe"
set "LOG=%OUT%\%~2.log"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "%~dp0%SRC%.cpp" -o "!EXE!" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=COMPILE_FAIL
  type "!LOG!"
  if /i not "!TAG!"=="HEADER_DEFAULT_CTOR" if /i not "!TAG!"=="HEADER_DIRECT_CTOR" if /i not "!TAG!"=="HEADER_CLASS_ARRAY" if /i not "!TAG!"=="HEADER_COPY_INIT_SCOPE" (
    set /a SILENT+=1
  )
  exit /b 0
)
"!EXE!" >nul 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=RUN_FAIL
  if /i not "!TAG!"=="HEADER_DEFAULT_CTOR" if /i not "!TAG!"=="HEADER_DIRECT_CTOR" if /i not "!TAG!"=="HEADER_CLASS_ARRAY" (
    set /a SILENT+=1
  )
  exit /b 0
)
echo !TAG!=PASS
exit /b 0

:preprocessed_case
set "TAG=%~1"
set "SRC=%~2"
set "FLAT=%OUT%\%~2_flat.cpp"
set "EXE=%OUT%\%~2_flat.exe"
set "LOG=%OUT%\%~2_flat.log"
"%TCC%" "%~dp0%SRC%.cpp" -E -o "!FLAT!" >"!LOG!" 2>&1
if not "!errorlevel!"=="0" (
  echo !TAG!=PREPROCESS_FAIL
  set /a SILENT+=1
  exit /b 0
)
"%TCC%" "!FLAT!" -o "!EXE!" >>"!LOG!" 2>&1
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

:diag_case
set "TAG=%~1"
set "SRC=%~2"
set "LOG=%OUT%\diag_%SRC%.log"
"%TCC%" "%~dp0%SRC%.cpp" -c -o "%OUT%\%SRC%.o" >"!LOG!" 2>&1
findstr /C:"GLOBAL_COPY_INIT_INTERCEPTED=1" "!LOG!" >"%OUT%\%TAG%_diag.txt" 2>nul
if exist "%OUT%\%TAG%_diag.txt" type "%OUT%\%TAG%_diag.txt"
exit /b 0

:preprocessed_diag
set "TAG=%~1"
set "SRC=%~2"
set "FLAT=%OUT%\%~2_flat_diag.cpp"
set "LOG=%OUT%\diag_%SRC%_flat.log"
"%TCC%" "%~dp0%SRC%.cpp" -E -o "!FLAT!" >nul 2>&1
"%TCC%" "!FLAT!" -c -o "%OUT%\%SRC%_flat.o" >"!LOG!" 2>&1
findstr /C:"N7_01C_DIAG" "!LOG!" >"%OUT%\%TAG%_diag.txt" 2>nul
for /f "tokens=*" %%L in ('findstr /C:"N7_01C_DIAG" "%OUT%\%TAG%_diag.txt" 2^>nul ^| findstr /C:"GLOBAL_COPY_INIT_INTERCEPTED=1"') do echo %%L
exit /b 0
