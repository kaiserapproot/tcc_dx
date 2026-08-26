@echo off
rem BUG-33: static members declared in a header and defined in another TU
rem must link (function and data both).  Exes go to %TEMP%.
setlocal
pushd "%~dp0link"
set "TCC=..\..\..\tcc.exe"
set "OUT=%TEMP%\tcc_bug33_link"
if not exist "%OUT%" mkdir "%OUT%"
"%TCC%" bug33_link_main.cpp bug33_link_lib.cpp -o "%OUT%\bug33_link.exe"
if errorlevel 1 (
  echo   [BUG33 LINK BUILD FAIL]
  popd
  exit /b 1
)
"%OUT%\bug33_link.exe"
if errorlevel 1 (
  echo   [BUG33 LINK RUN FAIL]
  popd
  exit /b 1
)
popd
exit /b 0
