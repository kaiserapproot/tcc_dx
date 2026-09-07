@echo off
setlocal EnableExtensions
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_05a_measure"
if not exist "%OUT%" mkdir "%OUT%"
set "LOG=%OUT%\n6_05a_measure.log"
echo === N6-05A: NORMAL_EXE termination path measurement (no production change) === >"%LOG%"

call :one n6_05a_path_return return7
call :one n6_05a_path_fallthrough fallthrough0
call :one n6_05a_path_exit exit9
call :one n6_05a_path_abort abort
call :one n6_05a_path_tls_return tls_return7

echo. >>"%LOG%"
echo === Summary markers === >>"%LOG%"
findstr /c:"ENTRY_SYMBOL=" /c:"EXIT_IMPORT=" /c:"EXIT_DEFINED=" /c:"HAS_TLS_DIR=" /c:"RUN_RC=" "%LOG%"
type "%LOG%"
popd
exit /b 0

:one
set "NAME=%~1"
set "TAG=%~2"
echo. >>"%LOG%"
echo --- %NAME% (%TAG%) --- >>"%LOG%"
"%TCC%" %NAME%.cpp -o "%OUT%\%NAME%.exe" >>"%LOG%" 2>&1
if not "!errorlevel!"=="0" (
  echo %NAME%=COMPILE_FAIL >>"%LOG%"
  goto :eof
)
"%OUT%\%NAME%.exe" >>"%LOG%" 2>&1
echo RUN_RC=!errorlevel! >>"%LOG%"
dumpbin /headers "%OUT%\%NAME%.exe" 2>nul | findstr /i "entry" >>"%LOG%" 2>&1
dumpbin /imports "%OUT%\%NAME%.exe" 2>nul | findstr /i "exit MSVCRT msvcrt" >>"%LOG%" 2>&1
dumpbin /symbols "%OUT%\%NAME%.exe" 2>nul | findstr /i " exit _exit _tcc_cpp_start _start" >>"%LOG%" 2>&1
dumpbin /exports "%OUT%\%NAME%.exe" 2>nul | findstr /i " exit _start _tcc" >>"%LOG%" 2>&1
goto :eof
