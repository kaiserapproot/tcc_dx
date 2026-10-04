@echo off
rem Rebuilds dev\lib\libdxguid.a from dxguid.c with TCC.
rem   make_lib.bat            -> ..\lib\libdxguid.a
rem   make_lib.bat <path.a>   -> that file (the gate in dev\test\a9\sdk_gate.bat
rem                              uses this to compare a fresh build with the
rem                              committed archive)
rem Target: x86-64 (ELF64 relocatable in an ar archive - the only object
rem format TCC links).  dxguid.c says where the GUID values come from.
rem The build is reproducible: the same dxguid.c and tcc.exe give a
rem byte-identical archive.  The source is compiled by its relative name from
rem this directory so no absolute path ends up in the object, and the object
rem goes to %TEMP% so the working tree stays clean.
setlocal EnableExtensions
pushd "%~dp0"
set "TCC=..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUTLIB=..\lib\libdxguid.a"
if not "%~1"=="" set "OUTLIB=%~1"
set "OBJDIR=%TEMP%\tcc_libdxguid_obj"
if not exist "%OBJDIR%" mkdir "%OBJDIR%"
"%TCC%" -c dxguid.c -o "%OBJDIR%\dxguid.o"
if errorlevel 1 (
  echo LIBDXGUID=COMPILE_FAIL
  popd
  exit /b 1
)
"%TCC%" -ar rcs "%OUTLIB%" "%OBJDIR%\dxguid.o"
if errorlevel 1 (
  echo LIBDXGUID=AR_FAIL
  popd
  exit /b 1
)
echo LIBDXGUID=BUILT %OUTLIB%
popd
exit /b 0
