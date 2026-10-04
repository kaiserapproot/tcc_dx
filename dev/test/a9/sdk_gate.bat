@echo off
rem dev\include / dev\lib gate (PR #31 review).
rem   - <intrin.h> loads from C and C++, alone and after <windows.h>, and two C
rem     translation units that include it link (no header-emitted globals).
rem   - the intrinsics that have a TCC body really execute (__debugbreak, _abs64,
rem     __readgsqword); one without a body fails at link time, not silently.
rem   - a direct <x86intrin.h> include is rejected (fail-closed, no empty stub).
rem   - msimg32.def and d3dcompiler_47.def resolve their imports.
rem   - libdxguid.a links from C and C++, works next to a translation unit that
rem     defines GUIDs itself through INITGUID, and is byte-identical to a fresh
rem     build of dev\dxguid\dxguid.c (dev\dxguid\make_lib.bat).
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

:lib_rebuild_identical
rem The committed archive must be exactly what make_lib.bat builds from
rem dev\dxguid\dxguid.c with this tcc.exe.
set "REB=%OUT%\libdxguid_rebuilt.a"
if exist "!REB!" del /q "!REB!"
call ..\..\..\dxguid\make_lib.bat "!REB!" >"%OUT%\libdxguid_rebuild.log" 2>&1
if errorlevel 1 (
  echo SDK_LIBDXGUID_REBUILD=BUILD_FAIL
  type "%OUT%\libdxguid_rebuild.log"
  set /a FAILED+=1
  goto :eof
)
fc /b "!REB!" ..\..\..\lib\libdxguid.a >nul 2>&1
if errorlevel 1 (
  echo SDK_LIBDXGUID_REBUILD=DIFFERS_FROM_COMMITTED run dev\dxguid\make_lib.bat and commit dev\lib\libdxguid.a
  set /a FAILED+=1
  goto :eof
)
echo SDK_LIBDXGUID_REBUILD=IDENTICAL
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
call :build_run_ok SDK_INTRIN_H_C "sdk_intrin_c.c"
call :build_run_ok SDK_WINDOWS_INTRIN_H_C "sdk_windows_intrin_c.c"
call :build_run_ok SDK_INTRIN_H_ONLY_CPP "sdk_intrin_only_cpp.cpp"
call :build_run_ok SDK_INTRIN_H_CPP "sdk_intrin_cpp.cpp"
call :build_run_ok SDK_INTRIN_TWO_TU_C "sdk_intrin_tu1.c sdk_intrin_tu2.c"
call :build_run_ok SDK_INTRIN_EXEC_C "sdk_intrin_exec.c"
call :build_run_ok SDK_INTRIN_EXEC_CPP "sdk_intrin_exec_cpp.cpp"
call :build_must_fail SDK_INTRIN_DECL_ONLY_FAILS_AT_LINK "sdk_intrin_decl_only.c" "undefined symbol '_InterlockedIncrement'"
call :build_must_fail SDK_X86INTRIN_H_FAIL_CLOSED "sdk_x86intrin.c" "is not supported by this TCC build"
call :build_run_ok SDK_DEF_MSIMG32_D3DCOMPILER_47 "sdk_defs.c -lmsimg32 -ld3dcompiler_47"
call :build_run_ok SDK_LIBDXGUID_C "sdk_dxguid.c -ldxguid"
call :build_run_ok SDK_LIBDXGUID_CPP "sdk_dxguid_cpp.cpp -ldxguid"
call :build_run_ok SDK_LIBDXGUID_NEXT_TO_INITGUID "sdk_dxguid_own.c sdk_dxguid_use.c -ldxguid"
call :lib_rebuild_identical

if not "!FAILED!"=="0" (
  echo SDK_GATE=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo SDK_GATE=PASS
popd
exit /b 0
