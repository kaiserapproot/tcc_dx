@echo off
set "TCC_ROOT=%~dp0"
set "TCC=%TCC_ROOT%tcc.exe"
set "PATH=%TCC_ROOT%;%PATH%"
set "C_INCLUDE_PATH=%TCC_ROOT%include"
set "CPATH=%TCC_ROOT%include"
set "LIBRARY_PATH=%TCC_ROOT%lib"
echo TCC path added: %TCC_ROOT%
echo TCC: %TCC%
echo C_INCLUDE_PATH: %C_INCLUDE_PATH%
echo CPATH: %CPATH%
echo LIBRARY_PATH: %LIBRARY_PATH%
