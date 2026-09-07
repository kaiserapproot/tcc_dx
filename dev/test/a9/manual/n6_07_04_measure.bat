@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "ROOT=..\..\.."
set "OUT=%TEMP%\tcc_n6_07_04"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
for %%I in ("!ROOT!") do set "TCC_DEV_ROOT=%%~fI"
for %%I in ("!ROOT!\..") do set "REPO=%%~fI"
set "HARNESS=!REPO!\x64\Release\n6_07_04_harness.exe"
set "LOG=!OUT!\harness.log"

echo === N6-07-04: tcc_delete live TLS fail-closed ===
echo BASE_COMMIT=8ebd840
echo.

if not exist "!HARNESS!" (
  if not defined VSCMD_VER (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
  )
  msbuild n6_07_04_harness.vcxproj /p:Configuration=Release /p:Platform=x64 /v:minimal >"!OUT!\build.log" 2>&1
  if not "!errorlevel!"=="0" (
    type "!OUT!\build.log"
    echo N6_07_04_BUILD=FAIL
    popd
    exit /b 1
  )
)

copy /Y "!ROOT!\tcc.exe" "!ROOT!\tcc.exe.n6_07_04.bak" >nul 2>&1
"!HARNESS!" "!TCC_DEV_ROOT!" >"!LOG!" 2>&1
set "HRC=!errorlevel!"
type "!LOG!"
if not "!HRC!"=="0" (
  echo N6_07_04=FAIL harness_rc=!HRC!
  popd
  exit /b 1
)
"%FINDSTR%" /c:"N6_07_04=PASS" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo N6_07_04=FAIL missing_pass_marker
  popd
  exit /b 1
)
echo N6_07_04_MEASURE=PASS
popd
exit /b 0
