@echo off
setlocal enabledelayedexpansion
pushd "%~dp0"
set "TCC=..\tcc.exe"
set /a FAILED=0

rem === Known investigation sources (compile failure is a documented gap,
rem     NOT a regression) -> excluded from gating, shown for info only.
rem     Add here ONLY with a reason so real regressions stay visible.
rem       sm3 / sm6 / t_assign : static-member `Counter::x = v` write
rem                              is not implemented yet (Stage 4 gap).
set "KNOWNFAIL= sm3.cpp sm6.cpp t_assign.cpp "

rem === Phase 1: compile-only (every source must pass -c) ===
for %%f in (smoke\*.c smoke\*.cpp a2\*.c a2\*.cpp a3\*.c a3\*.cpp a4\*.cpp a5\*.cpp a6\*.cpp a8\*.cpp a9\*.cpp a7\member_call.cpp a7\default_arg.cpp a7\inline_member.cpp a7\typedef_class.cpp) do (
    echo === %%f ===
    "%TCC%" -c "%%f" -o "%%~dpnf.o"
    if errorlevel 1 (
        set "ISKNOWN="
        echo !KNOWNFAIL! | findstr /c:" %%~nxf " >nul && set "ISKNOWN=1"
        if defined ISKNOWN (
            echo   [known gap, non-gating] %%f
        ) else (
            echo   [COMPILE FAIL] %%f
            set /a FAILED+=1
        )
    )
)

rem === Phase 2: execution gate (link + run, require exit code 0) ===
rem A compile-only pass misses runtime regressions - e.g. BUG-12's
rem feat6a_big_struct compiled fine but exited nonzero at run time.
rem Targets are the exit-0 suite: all a9 + a7 member tests + smoke hello.
rem Exes go to a temp dir so the working tree stays clean.
set "EXEOUT=%TEMP%\tcc_exec_gate"
if not exist "%EXEOUT%" mkdir "%EXEOUT%"
for %%f in (a9\*.cpp a7\member_call.cpp a7\default_arg.cpp a7\inline_member.cpp a7\typedef_class.cpp smoke\hello.c) do (
    "%TCC%" "%%f" -o "%EXEOUT%\%%~nf.exe" >nul
    rem Use `neq 0` (not `errorlevel 1`): a crash exits with a NEGATIVE code
    rem (e.g. 0xC0000005 access violation), which `if errorlevel 1` misses.
    if !errorlevel! neq 0 (
        echo   [BUILD FAIL] %%f
        set /a FAILED+=1
    ) else (
        "%EXEOUT%\%%~nf.exe" >nul
        if !errorlevel! neq 0 (
            echo   [RUN FAIL] %%f
            set /a FAILED+=1
        )
    )
)

echo === a2\mixed_link.bat ===
call a2\mixed_link.bat
if errorlevel 1 set /a FAILED+=1

echo === run_all summary: !FAILED! gating failure(s) ===
popd
exit /b !FAILED!
