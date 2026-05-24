@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

echo ========================================
echo Building tcc.vcxproj x64 Debug
echo ========================================
msbuild tcc.vcxproj /p:Configuration=Debug /p:Platform=x64

echo ========================================
echo Building tcc.vcxproj x64 Release
echo ========================================
msbuild tcc.vcxproj /p:Configuration=Release /p:Platform=x64

echo ========================================
echo Copying Release tcc.exe to dev folder...
echo ========================================
copy /Y x64\Release\tcc.exe dev\

echo ========================================
echo Build Completed
echo ========================================
pause
endlocal
