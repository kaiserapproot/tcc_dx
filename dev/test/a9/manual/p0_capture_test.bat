@echo off
setlocal
set "ROOT=E:\work\work_github\tpp\tcc_dx"
pushd "%ROOT%"
set "TCC=%CD%\dev\tcc.exe"
set DIAG=inject_force_n6_trace
set SRC=%CD%\dev\test\a9\manual\p0_header_defined_class.cpp
set OUT=%CD%\dev\test\a9\manual\disc_out\capture_D.exe
echo === TEST ===
set TCC_CRASH_DIAG=%DIAG%
"%TCC%" "%SRC%" -o "%OUT%" 1>"%CD%\dev\test\a9\manual\disc_out\capture_D.stdout" 2>"%CD%\dev\test\a9\manual\disc_out\capture_D.stderr"
echo CAPTURE_EXIT=%ERRORLEVEL%
type "%CD%\dev\test\a9\manual\disc_out\capture_D.stderr"
exit /b %ERRORLEVEL%
