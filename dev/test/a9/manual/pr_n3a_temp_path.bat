@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_pr_n3a_temp_path"
if not exist "%OUT%" mkdir "%OUT%"

echo === N3A temp path qualification ===
"%TCC%" pr_n3a_temp_path.cpp -o "%OUT%\pr_n3a_temp_path.exe"
if not "!errorlevel!"=="0" (
  echo PR_N3A_TEMP_PATH=COMPILE_FAIL
  popd
  exit /b 1
)
set "FAILED=0"
call :run_temp_case 1 "N3A-1"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_temp_case 2 "N3A-2"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_temp_case 3 "N3A-3 conditional operand"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_temp_case 4 "N3A-4 loop condition"
if not "!errorlevel!"=="0" set "FAILED=1"
call :run_temp_case 5 "N3A-5 conditional return"
if not "!errorlevel!"=="0" set "FAILED=1"
if "%FAILED%"=="1" (
  echo PR_N3A_TEMP_PATH=FAIL
) else (
  echo PR_N3A_TEMP_PATH=PASS
)

echo === N3A reference lifetime qualification ===
"%TCC%" pr_n3a_reference_lifetime.cpp -o "%OUT%\pr_n3a_reference_lifetime.exe" >"%OUT%\reference.log" 2>&1
if not "!errorlevel!"=="0" (
  echo REFERENCE_LIFETIME_EXTENSION_SUPPORTED=NO
  type "%OUT%\reference.log"
  echo REFERENCE_LIFETIME_EXTENSION_QUALIFICATION=FAIL
  set "FAILED=1"
) else (
  echo REFERENCE_LIFETIME_EXTENSION_SUPPORTED=YES
  set "REF_FAILED=0"
  call :run_reference_case 1 "R1 complete object"
  if not "!errorlevel!"=="0" set "REF_FAILED=1"
  call :run_reference_case 2 "R2 scalar subobject"
  if not "!errorlevel!"=="0" set "REF_FAILED=1"
  call :run_reference_case 3 "R3 class subobject"
  if not "!errorlevel!"=="0" set "REF_FAILED=1"
  call :run_reference_case 4 "R4 base subobject"
  if not "!errorlevel!"=="0" set "REF_FAILED=1"
  if "!REF_FAILED!"=="1" (
    echo REFERENCE_BOUND_TEMP_LIFETIME=FAIL
    set "FAILED=1"
  ) else (
    echo REFERENCE_BOUND_TEMP_LIFETIME=PASS
  )
)
popd
if "%FAILED%"=="1" exit /b 1
exit /b 0

:run_reference_case
echo === %~2 ===
"%OUT%\pr_n3a_reference_lifetime.exe" %1
if not "!errorlevel!"=="0" (
  echo %~2=FAIL
  exit /b 1
)
echo %~2=PASS
exit /b 0

:run_temp_case
echo === %~2 ===
"%OUT%\pr_n3a_temp_path.exe" %1
if not "!errorlevel!"=="0" (
  echo %~2=FAIL
  exit /b 1
)
echo %~2=PASS
exit /b 0
