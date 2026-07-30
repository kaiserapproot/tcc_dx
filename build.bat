@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64

echo ========================================
echo Building tcc.vcxproj x64 Release
echo ========================================
msbuild tcc.vcxproj /p:Configuration=Release /p:Platform=x64
if errorlevel 1 exit /b 1

echo ========================================
echo Building tcc.vcxproj Win32 Debug
echo ========================================
msbuild tcc.vcxproj /p:Configuration=Debug /p:Platform=Win32
if errorlevel 1 exit /b 1

echo ========================================
echo Copying Release tcc.exe to dev folder...
echo ========================================
copy /Y x64\Release\tcc.exe dev\tcc.exe
if errorlevel 1 exit /b 1

echo ========================================
echo Smoke tests
echo ========================================
dev\tcc.exe -v
if errorlevel 1 exit /b 1
dev\tcc.exe dev\test\smoke\hello.c -o dev\test\smoke\hello.exe
if errorlevel 1 exit /b 1
dev\test\smoke\hello.exe
if errorlevel 1 exit /b 1

echo ========================================
echo Regression: repro_double6
echo ========================================
dev\tcc.exe dev\test\repro_double6.c -o dev\test\repro_double6.exe
if errorlevel 1 exit /b 1
dev\test\repro_double6.exe
if errorlevel 1 exit /b 1

echo ========================================
echo CUnit build
echo ========================================
msbuild test\vs_test\con_c_vs_test.vcxproj /p:Configuration="Debug Unicode" /p:Platform=x64
if errorlevel 1 exit /b 1
pushd "test\vs_test\x64\Debug Unicode"
.\con_c_vs_test.exe
if errorlevel 1 (
  popd
  exit /b 1
)
popd

echo ========================================
echo L1 tests: run_all.bat
echo ========================================
call dev\test\run_all.bat
if errorlevel 1 exit /b 1

echo ========================================
echo Build Completed
echo ========================================
endlocal
