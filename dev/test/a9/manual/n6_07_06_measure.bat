@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "ROOT=..\..\.."
set "OUT=%TEMP%\tcc_n6_07_06"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
for %%I in ("!ROOT!") do set "TCC_DEV_ROOT=%%~fI"
for %%I in ("!ROOT!\..") do set "REPO=%%~fI"
set "HARNESS=!REPO!\x64\Release\n6_07_00_harness.exe"
set "LOG=!OUT!\harness.log"

echo === N6-07-06: direct relocate LIMITED boundary gate ===
echo BASE_COMMIT=8ebd840
echo.

if not exist "!HARNESS!" (
  if not defined VSCMD_VER (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
  )
  msbuild n6_07_00_harness.vcxproj /p:Configuration=Release /p:Platform=x64 /v:minimal >"!OUT!\build.log" 2>&1
  if not "!errorlevel!"=="0" (
    type "!OUT!\build.log"
    echo N6_07_06_BUILD=FAIL
    popd
    exit /b 1
  )
)

"!HARNESS!" "!TCC_DEV_ROOT!" >"!LOG!" 2>&1
set "HRC=!errorlevel!"
type "!LOG!"
if not "!HRC!"=="0" (
  echo N6_07_06=FAIL harness_rc=!HRC!
  popd
  exit /b 1
)
"%FINDSTR%" /c:"DIRECT_RELOCATE_CAPABILITY=LIMITED" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo N6_07_06=FAIL missing_limited_marker
  popd
  exit /b 1
)
"%FINDSTR%" /c:"HOST_MANUAL_EXECUTION_CLEANUP_BEFORE_TCC_DELETE=YES" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo N6_07_06=FAIL missing_host_cleanup
  popd
  exit /b 1
)
"%FINDSTR%" /c:"TCC_DELETE_WITH_LIVE_TLS=FAIL_CLOSED" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo N6_07_06=FAIL missing_delete_fail_closed_child
  popd
  exit /b 1
)
echo DIRECT_RELOCATE_CAPABILITY=LIMITED
echo DIRECT_RELOCATE_REPORTS_FULL_N6_SUPPORT=NO
echo N6_07_06_CLASS=UNSUPPORTED_BUT_SAFE_AND_EXPLICIT
echo N6_07_06=PASS
echo N6_07_06_MEASURE=PASS
popd
exit /b 0
