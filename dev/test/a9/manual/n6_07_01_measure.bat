@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "ROOT=..\..\.."
set "TCC=!ROOT!\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_07_01"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

echo === N6-07-01: DLL thread_local compile fail-closed gate ===
echo BASE_COMMIT=8ebd840
echo PRODUCTION_CHANGE=NONE
echo.

call :dll_probe n6_07_00_dll_trivial TRIVIAL
call :dll_probe n6_07_00_dll_nontrivial NONTRIVIAL

if not "!FAILED!"=="0" (
  echo N6_07_01=FAIL
  popd
  exit /b 1
)
echo DLL_TLS_CURRENT_BEHAVIOR=COMPILE_FAIL_CLOSED
echo DLL_TLS_SILENT_FALLBACK=NO
echo DLL_N6_TLS_CURRENTLY_UNSAFE_ACCEPTANCE=NO
echo N6_07_01_CLASS=ALREADY_FAIL_CLOSED
echo N6_07_01=PASS
echo N6_07_01_MEASURE=PASS
popd
exit /b 0

:dll_probe
set "SRC=%~1.cpp"
set "TAG=%~2"
set "LOG=!OUT!\dll_%~1.log"
set "DLL=!OUT!\%~1.dll"
if exist "!DLL!" del /q "!DLL!"
"%TCC%" -shared "!SRC!" -o "!DLL!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if "!RC!"=="0" (
  if exist "!DLL!" (
    echo DLL_!TAG!_COMPILE=UNEXPECTED_PASS
    set /a FAILED+=1
  ) else (
    echo DLL_!TAG!_COMPILE=UNEXPECTED_PASS_NO_DLL
    set /a FAILED+=1
  )
) else (
  if !RC! LSS 0 (
    echo DLL_!TAG!_COMPILE=TCC_CRASH
    set /a FAILED+=1
  ) else (
    echo DLL_!TAG!_COMPILE=FAIL_CLOSED
  )
)
"%FINDSTR%" /i /c:"thread_local TLS in DLL is unsupported" "!LOG!" >nul 2>&1
if not errorlevel 1 (
  echo DLL_!TAG!_DIAGNOSTIC=YES
) else (
  "%FINDSTR%" /i /c:"thread_local" "!LOG!" >nul 2>&1
  if not errorlevel 1 (
    echo DLL_!TAG!_DIAGNOSTIC=COMPILE_ERROR
  ) else (
    echo DLL_!TAG!_DIAGNOSTIC=CHECK_LOG
  )
)
if exist "!DLL!" (
  echo DLL_!TAG!_ARTIFACT=YES
  set /a FAILED+=1
) else (
  echo DLL_!TAG!_ARTIFACT=NO
)
exit /b 0
