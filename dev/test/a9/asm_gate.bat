@echo off
rem Encoding gate for the SSE / SSE2 entries of x86_64-asm.h: TCC assembles
rem asm\sse_sse2_list.s, and the bytes must match asm\sse_sse2_expected.h
rem (checked once against clang by manual\asm_sse_sse2_vs_clang.bat).
rem Keep this file CRLF: cmd can misread call/goto labels in LF-only files.
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0asm"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_asm_gate"
if not exist "%OUT%" mkdir "%OUT%"
set "EXE=%OUT%\sse_sse2_check.exe"
echo === x86_64 assembler gate ===
rem a failed build stops here, so a stale exe from an earlier run is never run
"%TCC%" sse_sse2_list.s sse_sse2_check.c -o "%EXE%" >"%OUT%\build.log" 2>&1
if errorlevel 1 (
  echo ASM_SSE_SSE2_ENCODING=BUILD_FAIL
  type "%OUT%\build.log"
  echo ASM_GATE=FAIL
  popd
  exit /b 1
)
"%EXE%"
if errorlevel 1 (
  echo ASM_SSE_SSE2_ENCODING=MISMATCH
  echo ASM_GATE=FAIL
  popd
  exit /b 1
)
echo ASM_SSE_SSE2_ENCODING=PASS
echo ASM_GATE=PASS
popd
exit /b 0