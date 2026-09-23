@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

rem Qualification for BUG-52 (local class init in #include'd files) and
rem BUG-53 (undeclared identifier in a member body).  The a9 tests themselves
rem are auto-enumerated by run_all.bat; what this script adds is the evidence
rem that the review asked for and that a pass/fail exit code cannot show:
rem   - the diagnostic BUG-53 is actually about (the implicit-declaration
rem     warning) is really emitted, not just "it compiled"
rem   - the widened BUG-52 gate is exercised at include depth 0/1/2, from
rem     system headers, and through the synthetic inline replay
rem   - run_all.bat really picks both new tests up

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_bug52_53"
if exist "%OUT%" rd /s /q "%OUT%"
mkdir "%OUT%"
set "A9=.."
set "RUNALL=..\..\run_all.bat"
set /a FAILED=0

echo === BUG-52/53 qualification ===

rem ---- BUG-53: the warning itself, not just a clean exit -------------------
"%TCC%" "%A9%\member_body_implicit_decl.cpp" -o "%OUT%\mbid.exe" 2>"%OUT%\mbid.err"
if errorlevel 1 (
    echo MEMBER_BODY_IMPLICIT_DECL_COMPILE=FAIL
    type "%OUT%\mbid.err"
    set /a FAILED+=1
) else (
    echo MEMBER_BODY_IMPLICIT_DECL_COMPILE=PASS
    findstr /c:"implicit declaration of function 'putchar'" "%OUT%\mbid.err" >nul
    if errorlevel 1 (
        echo MEMBER_BODY_IMPLICIT_DECL_WARNING=FAIL
        echo   expected the implicit-declaration diagnostic on stderr; got:
        type "%OUT%\mbid.err"
        set /a FAILED+=1
    ) else (
        echo MEMBER_BODY_IMPLICIT_DECL_WARNING=PASS
    )
    "%OUT%\mbid.exe" >nul 2>&1
    if errorlevel 1 (
        echo MEMBER_BODY_IMPLICIT_DECL_RUN=FAIL
        set /a FAILED+=1
    ) else (
        echo MEMBER_BODY_IMPLICIT_DECL_RUN=PASS
    )
)

rem ---- BUG-52: the a9 tests, run here too so this script stands alone ------
call :build_run HEADER_LOCAL_COPY_INIT_TEST "%A9%\header_local_copy_init.cpp" hlci
call :build_run INCLUDE_BOUNDARY_TEST "%A9%\include_boundary_class_init.cpp" ibci

rem ---- boundary: system headers -------------------------------------------
call :build_run SYSTEM_HEADER_BOUNDARY bug52_53_system_header.cpp sysh

rem ---- boundary: synthetic replay (:inline:) stays on the old path ---------
call :build_run SYNTHETIC_INLINE_REPLAY g7_01_inline_strip_main.cpp g701

rem ---- boundary: C mode is untouched --------------------------------------
"%TCC%" -c "%A9%\bug20_c_mode.c" -o "%OUT%\cmode.o" >nul 2>&1
if errorlevel 1 (
    echo C_MODE_NON_REGRESSION=FAIL
    set /a FAILED+=1
) else (
    echo C_MODE_NON_REGRESSION=PASS
)

rem ---- proof that run_all.bat auto-enumerates both new tests ---------------
rem Both search strings are taken verbatim from run_all.bat's two loop lines and
rem contain no %, which a batch file would eat before the tool ever saw it.
rem find, not findstr: findstr /c: fails to match a literal containing '*'
rem (measured on this box - "a9" and "a7\member_call.cpp" match, "a9\*.cpp" does not).
set /a RA=0
find /c "a9\*.c a9\*.cpp" "%RUNALL%" >nul && set /a RA+=1
find /c "in (a9\*.cpp a7\member_call.cpp" "%RUNALL%" >nul && set /a RA+=1
if "!RA!"=="2" (
    echo RUN_ALL_INCLUDES_BOTH=YES
) else (
    echo RUN_ALL_INCLUDES_BOTH=NO matched=!RA!
    set /a FAILED+=1
)

echo.
echo === BUG-52/53 qualification done failures=!FAILED! ===
popd
if not "!FAILED!"=="0" exit /b 1
exit /b 0

:build_run
set "LABEL=%~1"
set "SRC=%~2"
set "TAG=%~3"
"%TCC%" "%SRC%" -w -o "%OUT%\%TAG%.exe" 2>"%OUT%\%TAG%.err"
if errorlevel 1 (
    echo %LABEL%=COMPILE_FAIL
    type "%OUT%\%TAG%.err"
    set /a FAILED+=1
    goto :eof
)
"%OUT%\%TAG%.exe" >nul 2>&1
if errorlevel 1 (
    echo %LABEL%=RUN_FAIL
    set /a FAILED+=1
    goto :eof
)
echo %LABEL%=PASS
goto :eof
