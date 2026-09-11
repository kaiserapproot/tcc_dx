@echo off
setlocal ENABLEEXTENSIONS
rem TCC-side entry point: delegate to Amateras consumer harness (measurement only).
set "AMATERAS_ROOT=E:\work\work_cross_platform\kaiser_system\amateras"
if defined TCC_CPP_EXE (
  set "TCC_CPP_EXE=%TCC_CPP_EXE%"
) else (
  set "TCC_CPP_EXE=E:\work\work_github\tpp\tcc_dx\dev\tcc.exe"
)
set "TCC_CPP_EXE=%TCC_CPP_EXE%"
call "%AMATERAS_ROOT%\test\tcc\n7_06\build_run_n7_06.bat"
exit /b %ERRORLEVEL%
