@echo off
rem Re-checks dev\dxguid\dxguid.c against the archive it was written from.
rem
rem The original (mingw-w64 lib64, COFF x86-64) is no longer in the tree.  It is
rem read back from git history: commit 8b153f7 added it as dev/lib/libdxguid.a.
rem   1. git cat-file writes that blob to %TEMP%
rem   2. certutil: its SHA-256 must be the one recorded in dxguid.c
rem   3. verify_original.c (built with dev\tcc.exe) parses the ar / COFF data
rem      and compares every GUID with the DXGUID() lines, in both directions
rem
rem This is a manual check and is not called from run_all.bat: it needs git
rem and the full history (a shallow clone does not have the blob).  The gate
rem that runs on every build is dev\test\a9\sdk_gate.bat, which proves that
rem dev\lib\libdxguid.a is exactly what make_lib.bat builds from dxguid.c.
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set "OUT=%TEMP%\tcc_dxguid_verify"
if not exist "%OUT%" mkdir "%OUT%"
set "ORIG_REV=8b153f7:dev/lib/libdxguid.a"
set "ORIG_SHA256=13de09be7e4d740eeae184b068a1f36833f293f1006f6e6d2fbcb1f937bd7334"

echo === dxguid.c against the original archive ===
git cat-file blob %ORIG_REV% > "%OUT%\libdxguid_original.a"
if errorlevel 1 (
  echo DXGUID_VERIFY=FAIL cannot read %ORIG_REV% from git history
  popd
  exit /b 1
)
certutil -hashfile "%OUT%\libdxguid_original.a" SHA256 | "%FINDSTR%" /i /c:"%ORIG_SHA256%" >nul
if errorlevel 1 (
  echo DXGUID_VERIFY=FAIL the archive from git history does not have the recorded SHA-256
  popd
  exit /b 1
)
echo ORIGINAL_SHA256=%ORIG_SHA256%
"%TCC%" verify_original.c -o "%OUT%\verify_original.exe"
if errorlevel 1 (
  echo DXGUID_VERIFY=FAIL verify_original.c did not build
  popd
  exit /b 1
)
"%OUT%\verify_original.exe" "%OUT%\libdxguid_original.a" dxguid.c
set "RC=!errorlevel!"
popd
exit /b %RC%
