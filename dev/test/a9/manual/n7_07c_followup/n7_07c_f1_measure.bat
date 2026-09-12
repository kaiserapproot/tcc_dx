@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
goto :f1_main

:probe
set "TAG=%~1"
set "SRC=%~2"
set "LOG=%OUT%\%~n2.log"
"%TCC%" "%~dp0%SRC%" -o "%OUT%\%~n2.exe" >"%LOG%" 2>&1
if errorlevel 1 (
  echo !TAG!=COMPILE_FAIL
  findstr /i "default arguments" "%LOG%" >nul 2>&1
  if not errorlevel 1 echo   diagnostic=default_arguments
) else (
  "%OUT%\%~n2.exe" >"%OUT%\%~n2_run.log" 2>&1
  if errorlevel 1 (
    echo !TAG!=RUN_FAIL
    set /a BAD=1
  ) else (
    echo !TAG!=PASS
  )
)
exit /b 0

:f1_main
set "TCC=..\..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_07c_f1_out"
if not exist "%OUT%" mkdir "%OUT%"
set /a BAD=0

echo === N7-07C-F1 CAUSAL ISOLATION (MEASUREMENT ONLY) ===
echo TCC=%TCC%
echo PHASE=N7-07C-F1
echo TCC_PRODUCTION_CHANGE=NONE
echo.

echo --- generic M(int value=7) matrix ---
call :probe PLAIN_DEFAULT_ARG_SCALAR n7_07c_f1_plain_scalar.cpp
call :probe PLAIN_DEFAULT_ARG_ARRAY n7_07c_f1_plain_array.cpp
call :probe EXTERN_C_DEFAULT_ARG_SCALAR n7_07c_f1_extern_c_scalar.cpp
call :probe EXTERN_C_DEFAULT_ARG_ARRAY n7_07c_f1_extern_c_array.cpp
call :probe EXTERN_C_EXPLICIT_ARG_SCALAR n7_07c_f1_extern_c_explicit.cpp
echo.

echo --- vec2 stub matrix ---
call :probe VEC2_PLAIN_SCALAR n7_07c_f1_vec2_plain_scalar.cpp
call :probe VEC2_PLAIN_ARRAY n7_07c_f1_vec2_plain_array.cpp
call :probe VEC2_EXTERN_C_SCALAR n7_07c_f1_vec2_extern_c_scalar.cpp
call :probe VEC2_EXTERN_C_ARRAY n7_07c_f1_vec2_extern_c_array.cpp
echo.

if not "!BAD!"=="0" (
  echo N7_07C_F1=FAIL
  popd
  exit /b 1
)
echo N7_07C_F1=PASS
popd
exit /b 0
