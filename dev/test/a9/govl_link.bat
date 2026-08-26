@echo off
rem BUG-30 / G-OVL: a member declared in a header and defined in ANOTHER TU
rem must link - the call site emits an extern reference, and overloads must
rem still pick the right one.  Exes go to %TEMP% so the tree stays clean.
setlocal
pushd "%~dp0link"
set "TCC=..\..\..\tcc.exe"
set "OUT=%TEMP%\tcc_govl_link"
if not exist "%OUT%" mkdir "%OUT%"
"%TCC%" govl_link_main.cpp govl_link_lib.cpp -o "%OUT%\govl_link.exe"
if errorlevel 1 (
  echo   [GOVL LINK BUILD FAIL]
  popd
  exit /b 1
)
"%OUT%\govl_link.exe"
if errorlevel 1 (
  echo   [GOVL LINK RUN FAIL]
  popd
  exit /b 1
)
popd
exit /b 0
