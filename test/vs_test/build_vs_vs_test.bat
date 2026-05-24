@echo off
setlocal
pushd "%~dp0"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" echo [WARN] vcvars が見つかりません: C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat
if defined VCVARS_PATH call "%VCVARS_PATH%"

echo --- cppuniut プロジェクトのビルドを開始します ---
set "CPPUNIT_PROJ=..\cppuniut\cppuniut.vcxproj"
msbuild "%CPPUNIT_PROJ%" /m /p:Configuration="Debug" /p:Platform="Win32"
if errorlevel 1 goto error
msbuild "%CPPUNIT_PROJ%" /m /p:Configuration="Release" /p:Platform="Win32"
if errorlevel 1 goto error
msbuild "%CPPUNIT_PROJ%" /m /p:Configuration="Debug" /p:Platform="x64"
if errorlevel 1 goto error
msbuild "%CPPUNIT_PROJ%" /m /p:Configuration="Release" /p:Platform="x64"
if errorlevel 1 goto error

echo --- VS ビルドを開始します ---
msbuild "%~dp0vs_test.sln" /m /p:Configuration="Debug MBCS" /p:Platform="Win32"
if errorlevel 1 goto error
msbuild "%~dp0vs_test.sln" /m /p:Configuration="Debug Unicode" /p:Platform="Win32"
if errorlevel 1 goto error
msbuild "%~dp0vs_test.sln" /m /p:Configuration="Release MBCS" /p:Platform="Win32"
if errorlevel 1 goto error
msbuild "%~dp0vs_test.sln" /m /p:Configuration="Release Unicode" /p:Platform="Win32"
if errorlevel 1 goto error
msbuild "%~dp0vs_test.sln" /m /p:Configuration="Debug MBCS" /p:Platform="x64"
if errorlevel 1 goto error
msbuild "%~dp0vs_test.sln" /m /p:Configuration="Debug Unicode" /p:Platform="x64"
if errorlevel 1 goto error
msbuild "%~dp0vs_test.sln" /m /p:Configuration="Release MBCS" /p:Platform="x64"
if errorlevel 1 goto error
msbuild "%~dp0vs_test.sln" /m /p:Configuration="Release Unicode" /p:Platform="x64"
if errorlevel 1 goto error
echo --- ビルドが正常に完了しました ---
popd >nul 2>&1
endlocal
exit /b 0
:error
echo --- ビルドエラーが発生しました ---
popd >nul 2>&1
endlocal
exit /b 1
