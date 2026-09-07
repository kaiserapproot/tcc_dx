@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "ROOT=..\..\.."
set "OUT=%TEMP%\tcc_n6_06b_00"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0

for %%I in ("!ROOT!") do set "TCC_DEV_ROOT=%%~fI"
for %%I in ("!ROOT!\..") do set "REPO=%%~fI"
set "HARNESS=!REPO!\x64\Release\n6_06b_libtcc_harness.exe"

echo === N6-06B-00: direct relocate contract measurement ===
echo BASE_COMMIT=0fe0c4e
echo PRODUCTION_CHANGE=NONE
echo PUBLIC_API_CHANGE=NONE
echo N6_06B_IMPLEMENTATION_START=NO
echo.

if not exist "!HARNESS!" (
  if not defined VSCMD_VER (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
  )
  msbuild n6_06b_libtcc_harness.vcxproj /p:Configuration=Release /p:Platform=x64 /v:minimal >"!OUT!\build.log" 2>&1
  if not "!errorlevel!"=="0" (
    type "!OUT!\build.log"
    echo N6_06B_00_BUILD=FAIL
    popd
    exit /b 1
  )
)

if not exist "!HARNESS!" (
  echo N6_06B_00_BUILD=MISSING_EXE
  popd
  exit /b 1
)

set "LOG=!OUT!\n6_06b_harness.log"
"!HARNESS!" "!TCC_DEV_ROOT!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if not "!RC!"=="0" (
  echo N6_06B_00_MEASUREMENT=RUN_FAIL rc=!RC!
  popd
  exit /b 1
)

"%FINDSTR%" /c:"N6_06B_00_MEASUREMENT=PASS" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo N6_06B_00_MEASUREMENT=OUTPUT_FAIL
  set /a FAILED+=1
)

"%FINDSTR%" /c:"MANUAL_EXECUTION_END_OBSERVABLE_BY_TCC=NO" "!LOG!" >nul 2>&1
if errorlevel 1 set /a FAILED+=1

if not "!FAILED!"=="0" (
  echo N6_06B_00_GATE=FAIL
  popd
  exit /b 1
)
echo N6_06B_00_GATE=PASS
popd
exit /b 0
