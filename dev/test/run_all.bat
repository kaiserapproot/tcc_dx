@echo off
setlocal
pushd "%~dp0"
set TCC=..\tcc.exe
set FAILED=0

for %%f in (smoke\*.c smoke\*.cpp a2\*.c a2\*.cpp a3\*.c a3\*.cpp a4\*.cpp a5\*.cpp a6\*.cpp a7\member_call.cpp) do (
    echo === %%f ===
    "%TCC%" -c "%%f" -o "%%~dpnf.o"
    if errorlevel 1 set FAILED=1
)

echo === a2\mixed_link.bat ===
call a2\mixed_link.bat
if errorlevel 1 set FAILED=1

popd
exit /b %FAILED%