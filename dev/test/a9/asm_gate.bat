@echo off
rem Encoding gate for the SSE / SSE2 entries of x86_64-asm.h: TCC assembles
rem asm\sse_sse2_list.s, and the bytes must match asm\sse_sse2_expected.h
rem (checked once against clang by manual\asm_sse_sse2_vs_clang.bat).
rem Also: MMX and XMM operands mixed in one OPT_MMXSSE instruction must be
rem rejected (they used to assemble as the all-xmm form), while cvtpi2ps,
rem which really mixes them, must still assemble.
rem Keep this file CRLF: cmd can misread call/goto labels in LF-only files.
setlocal EnableExtensions EnableDelayedExpansion
goto :main
:must_reject
rem %1 = .s file; the assembler must refuse it with 'bad operand'
"%TCC%" -c %1 -o "%OUT%\mix.o" >"%OUT%\mix.log" 2>&1
if not errorlevel 1 (
  echo ASM_MIX_REJECTED_%~n1=SILENT_ACCEPTANCE
  set /a FAILED+=1
  goto :eof
)
"%FINDSTR%" /c:"bad operand" "%OUT%\mix.log" >nul 2>&1
if errorlevel 1 (
  echo ASM_MIX_REJECTED_%~n1=WRONG_DIAGNOSTIC
  type "%OUT%\mix.log"
  set /a FAILED+=1
  goto :eof
)
echo ASM_MIX_REJECTED_%~n1=REJECTED_AS_EXPECTED
goto :eof
:main
pushd "%~dp0asm"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_asm_gate"
if not exist "%OUT%" mkdir "%OUT%"
set "EXE=%OUT%\sse_sse2_check.exe"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0
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
call :must_reject mix_paddq_mm_xmm.s
call :must_reject mix_paddq_xmm_mm.s
call :must_reject mix_pavgb_mm_xmm.s
call :must_reject mix_paddd_mm_xmm.s
"%TCC%" -c mix_ok_cvtpi2ps.s -o "%OUT%\mix_ok.o" >"%OUT%\mix_ok.log" 2>&1
if errorlevel 1 (
  echo ASM_MIX_OK_CVTPI2PS=BUILD_FAIL
  type "%OUT%\mix_ok.log"
  set /a FAILED+=1
) else (
  echo ASM_MIX_OK_CVTPI2PS=PASS
)
if not "!FAILED!"=="0" (
  echo ASM_GATE=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo ASM_GATE=PASS
popd
exit /b 0