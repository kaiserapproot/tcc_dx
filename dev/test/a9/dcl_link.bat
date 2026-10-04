@echo off
rem C++ [dcl.link]/7 gate (PR #31 review).
rem   A  extern "C" int a;         declaration - needs a definition elsewhere
rem   B  extern "C" { int b; }     definition
rem   C  extern "C" C c;           declaration - no default constructor needed
rem   D  extern "C" int d = 123;   definition
rem   E  extern "C" int e(int); then int e(int) { } outside: still the C function
rem   F  calls inside an extern "C" block still resolve overloads
rem plus the reason the rule was added: two C++ TUs that both include
rem windows.h must link, and a GUID declared in one TU must resolve to the
rem definition in the other.  Sources are in a9\link\ (a subdirectory, so the
rem a9\*.cpp globs in run_all.bat never pick them up).  Exes go to %TEMP%.
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
set "OUT=%TEMP%\tcc_dcl_link"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0

echo === [dcl.link]/7 gate ===
call :build_run_ok DCL_LINK_A_SINGLE_FORM_DECLARES "dcl_link_a_use.cpp dcl_link_a_def.c"
call :build_must_fail DCL_LINK_A_ALONE_UNDEFINED "dcl_link_a_use.cpp" "undefined symbol 'dcl_a'"
call :build_run_ok DCL_LINK_B_BLOCK_FORM_DEFINES "dcl_link_b_block.cpp"
call :build_run_ok DCL_LINK_C_CLASS_SINGLE_FORM_DECLARES "dcl_link_c_class.cpp"
call :build_run_ok DCL_LINK_D_INITIALIZER_DEFINES "dcl_link_d_init.cpp"
call :build_must_fail DCL_LINK_BLOCK_CLASS_STILL_NEEDS_CTOR "dcl_link_neg_block_class.cpp" "class has no default constructor"
call :build_run_ok DCL_LINK_TWO_TU_WINDOWS_H "dcl_link_w1.cpp dcl_link_w2.cpp"
call :build_run_ok DCL_LINK_GUID_DECL_AND_DEF "dcl_link_g1.cpp dcl_link_g2.cpp"
call :build_run_ok DCL_LINK_E_DEF_KEEPS_C_LINKAGE "dcl_link_e_def.cpp dcl_link_e_use.c"
call :build_run_ok DCL_LINK_F_CALL_IN_BLOCK_RESOLVES "dcl_link_f_resolve.cpp"

if not "!FAILED!"=="0" (
  echo DCL_LINK=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo DCL_LINK=PASS
popd
exit /b 0
