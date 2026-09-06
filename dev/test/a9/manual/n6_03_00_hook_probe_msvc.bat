@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
rem N6-03-00: OS-level Authority probe for the PE TLS callback thread-exit
rem hook, built with MSVC cl.exe (independent of tcc).  Manual gate: needs a
rem VS developer command prompt (cl.exe on PATH); not registered in run_all.
set "OUT=%TEMP%\tcc_n6_03_00_hook_probe"
if not exist "%OUT%" mkdir "%OUT%"
where cl.exe >nul 2>&1
if errorlevel 1 (
  echo N6_03_00_PE_TLS_CALLBACK_HOOK=SKIPPED cl.exe not on PATH
  popd
  exit /b 2
)
cl /nologo /W3 /O2 n6_03_00_hook_probe_msvc.c /Fo"%OUT%\\" /Fe"%OUT%\hook_probe.exe" /link /SUBSYSTEM:CONSOLE >"%OUT%\cl.log" 2>&1
if errorlevel 1 (
  type "%OUT%\cl.log"
  echo N6_03_00_PE_TLS_CALLBACK_HOOK=BUILD_FAIL
  popd
  exit /b 1
)
"%OUT%\hook_probe.exe"
set "RC=!errorlevel!"
if not "!RC!"=="0" (
  echo N6_03_00_PE_TLS_CALLBACK_HOOK=FAIL exit=!RC!
  popd
  exit /b 1
)
popd
exit /b 0
