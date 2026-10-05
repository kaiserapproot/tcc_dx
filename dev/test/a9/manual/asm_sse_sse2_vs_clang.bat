@echo off
rem Manual check for the SSE / SSE2 entries of x86_64-asm.h (not in run_all):
rem asm\sse_sse2_list.s is assembled by TCC and by clang, both objects are
rem disassembled by llvm-objdump without addresses and bytes, and the text must
rem be the same.  Equivalent encodings are therefore accepted (the movq forms
rem differ in bytes between the two assemblers).
rem   set LLVM_BIN=<folder with clang.exe and llvm-objdump.exe>
rem   asm_sse_sse2_vs_clang.bat
rem After a PASS, asm\sse_sse2_expected.h can be regenerated with
rem   tcc ..\asm\sse_sse2_list.s ..\asm\sse_sse2_check.c -o check.exe
rem   check.exe dump > ..\asm\sse_sse2_expected.h
rem Keep this file CRLF: cmd can misread call/goto labels in LF-only files.
setlocal EnableExtensions
pushd "%~dp0..\asm"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
if "%LLVM_BIN%"=="" (
  echo ASM_VS_CLANG=SKIP set LLVM_BIN to the folder with clang.exe and llvm-objdump.exe
  popd
  exit /b 2
)
set "OUT=%TEMP%\tcc_asm_vs_clang"
if not exist "%OUT%" mkdir "%OUT%"
"%TCC%" -c sse_sse2_list.s -o "%OUT%\tcc.o" || goto fail
"%LLVM_BIN%\clang.exe" -c --target=x86_64-pc-windows-msvc sse_sse2_list.s -o "%OUT%\clang.o" || goto fail
"%LLVM_BIN%\llvm-objdump.exe" -d --no-show-raw-insn --no-leading-addr "%OUT%\tcc.o" | findstr /v /c:"file format" > "%OUT%\tcc.txt"
"%LLVM_BIN%\llvm-objdump.exe" -d --no-show-raw-insn --no-leading-addr "%OUT%\clang.o" | findstr /v /c:"file format" > "%OUT%\clang.txt"
fc "%OUT%\tcc.txt" "%OUT%\clang.txt" > "%OUT%\fc.txt"
if errorlevel 1 (
  type "%OUT%\fc.txt"
  goto fail
)
echo ASM_VS_CLANG=PASS
popd
exit /b 0
:fail
echo ASM_VS_CLANG=FAIL
popd
exit /b 1