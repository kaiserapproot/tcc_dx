@echo off
msbuild tcc.vcxproj /p:Configuration=Release /p:Platform=x64 /t:Build /v:quiet
if errorlevel 1 exit /b 1
dev\tcc.exe dev\test\repro_double6.c -o dev\test\repro_double6.exe
if errorlevel 1 exit /b 1
dev\test\repro_double6.exe
if errorlevel 1 exit /b 1
