@echo off
rem dev\include / dev\lib gate (PR #31 review).
rem   - <intrin.h> stays usable from C++ without <x86intrin.h>.  (From C it has
rem     never compiled with TCC: _mingw.h's __debugbreak uses GCC inline asm.
rem     That is a separate, older gap and is not gated here.)
rem   - a direct <x86intrin.h> include is rejected (fail-closed, no empty stub).
rem   - msimg32.def and d3dcompiler_47.def resolve their imports.
rem Sources are in a9\link\.  Exes go to %TEMP%.
setlocal EnableExtensions EnableDelayedExpansion
goto :main
:build_run_ok
rem %1 = tag, %2 = sources and options
set "TAG=%~1"
set "EXE=%OUT%\%~1.exe"
set "LOG=%OUT%\%~1.log"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" %~2 -o "!EXE!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if not "!RC!"=="0" (
  echo !TAG!=BUILD_FAIL rc=!RC!
  type "!LOG!"
  set /a FAILED+=1
  goto :eof
)
"!EXE!" >nul 2>&1
set "RRC=!errorlevel!"
if not "!RRC!"=="0" (
  echo !TAG!=RUN_FAIL rc=!RRC!
  set /a FAILED+=1
  goto :eof
)
echo !TAG!=PASS
goto :eof

:build_must_fail
rem %1 = tag, %2 = sources and options, %3 = ASCII text the diagnostic must contain
set "TAG=%~1"
set "PAT=%~3"
set "EXE=%OUT%\%~1.exe"
set "LOG=%OUT%\%~1.log"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" %~2 -o "!EXE!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo !TAG!=SILENT_ACCEPTANCE
  set /a FAILED+=1
  goto :eof
)
if !RC! LSS 0 (
  echo !TAG!=TCC_CRASH rc=!RC!
  type "!LOG!"
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"!PAT!" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo !TAG!=REJECTED_WITH_WRONG_DIAGNOSTIC expected=!PAT!
  type "!LOG!"
  set /a FAILED+=1
  goto :eof
)
echo !TAG!=REJECTED_AS_EXPECTED
goto :eof
:main
pushd "%~dp0link"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set "OUT=%TEMP%\tcc_sdk_gate"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0

echo === dev\include / dev\lib gate ===
call :build_run_ok SDK_INTRIN_H_CPP "sdk_intrin_cpp.cpp"
call :build_must_fail SDK_X86INTRIN_H_FAIL_CLOSED "sdk_x86intrin.c" "is not supported by this TCC build"
call :build_run_ok SDK_DEF_MSIMG32_D3DCOMPILER_47 "sdk_defs.c -lmsimg32 -ld3dcompiler_47"

if not "!FAILED!"=="0" (
  echo SDK_GATE=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo SDK_GATE=PASS
popd
exit /b 0
