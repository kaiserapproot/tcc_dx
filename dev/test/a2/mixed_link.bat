@echo off
setlocal
pushd "%~dp0"
..\..\tcc.exe -c foo.cpp bar.c
set EC=%ERRORLEVEL%
popd
exit /b %EC%