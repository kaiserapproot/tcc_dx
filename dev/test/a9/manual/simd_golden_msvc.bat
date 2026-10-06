@echo off
rem Remakes simd\simd_values_expected.txt from MSVC (not in run_all).
rem simd_values.c is built by cl /Od as C and as C++; both outputs must be the
rem same, and that output becomes the expected file of simd_gate.bat.
rem   set VSDEVCMD=<path to VsDevCmd.bat>   (default: VS 2022 Professional)
rem   simd_golden_msvc.bat
rem Keep this file CRLF: cmd can misread call/goto labels in LF-only files.
setlocal EnableExtensions
set "NoDefaultCurrentDirectoryInExePath="
if "%VSDEVCMD%"=="" set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
  echo SIMD_GOLDEN=SKIP set VSDEVCMD to VsDevCmd.bat
  exit /b 2
)
call "%VSDEVCMD%" -arch=x64 >nul 2>&1
pushd "%~dp0..\simd"
set "OUT=%TEMP%\tcc_simd_golden"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /Od /TC simd_values.c /Fo"%OUT%\c.obj" /Fe"%OUT%\msvc_c.exe" >"%OUT%\cl_c.log" 2>&1 || goto fail
cl /nologo /Od /TP simd_values.c /Fo"%OUT%\cpp.obj" /Fe"%OUT%\msvc_cpp.exe" >"%OUT%\cl_cpp.log" 2>&1 || goto fail
"%OUT%\msvc_c.exe" >"%OUT%\msvc_c.txt" || goto fail
"%OUT%\msvc_cpp.exe" >"%OUT%\msvc_cpp.txt" || goto fail
fc /b "%OUT%\msvc_c.txt" "%OUT%\msvc_cpp.txt" >nul || goto fail
copy /y "%OUT%\msvc_c.txt" simd_values_expected.txt >nul || goto fail
echo SIMD_GOLDEN=WRITTEN simd\simd_values_expected.txt
popd
exit /b 0
:fail
echo SIMD_GOLDEN=FAIL see %OUT%
popd
exit /b 1