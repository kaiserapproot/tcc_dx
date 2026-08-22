@echo off
setlocal enabledelayedexpansion
pushd "%~dp0"
set "TCC=..\tcc.exe"
rem C2 (crash gate): TCC_EXE overrides the compiler under test - used ONLY
rem by the one-shot negative verification with a scratch binary, so the
rem gate itself can be proven to catch a crash without touching dev\tcc.exe.
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set /a FAILED=0
set /a CRASHES=0
set "CRLOG=%TEMP%\tcc_gate_err.log"

rem === Known investigation sources (compile failure is a documented gap,
rem     NOT a regression) -> excluded from gating, shown for info only.
rem     Add here ONLY with a reason so real regressions stay visible.
rem       sm3 / sm6 / t_assign : static-member `Counter::x = v` write
rem                              is not implemented yet (Stage 4 gap).
set "KNOWNFAIL= sm3.cpp sm6.cpp t_assign.cpp "

rem === Phase 1: compile-only (every source must pass -c) ===
rem   a9\*.c is enumerated too: the C-mode non-regression guard for the C++
rem   name-hiding fix (bug20_c_mode.c) is a .c file, and without this it would
rem   never actually be gated.
rem C2: a compiler CRASH is gating even for KNOWNFAIL sources.  Since C1's
rem crash net converts stack overflow / AV into "tcc: internal error" +
rem exit 1, crash detection needs BOTH checks: a negative errorlevel (raw
rem crash, net missed it) and the internal-error marker on stderr.
for %%f in (smoke\*.c smoke\*.cpp a2\*.c a2\*.cpp a3\*.c a3\*.cpp a4\*.cpp a5\*.cpp a6\*.cpp a8\*.cpp a9\*.c a9\*.cpp a7\member_call.cpp a7\default_arg.cpp a7\inline_member.cpp a7\typedef_class.cpp) do (
    echo === %%f ===
    "%TCC%" -c "%%f" -o "%%~dpnf.o" 2>"%CRLOG%"
    set "EC=!errorlevel!"
    set "ISCRASH="
    if !EC! lss 0 set "ISCRASH=1"
    findstr /c:"internal error" "%CRLOG%" >nul 2>nul && set "ISCRASH=1"
    if defined ISCRASH (
        echo   [CRASH] %%f exit=!EC!
        type "%CRLOG%"
        set /a CRASHES+=1
        set /a FAILED+=1
    ) else if !EC! neq 0 (
        type "%CRLOG%"
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
    "%TCC%" "%%f" -o "%EXEOUT%\%%~nf.exe" >nul 2>"%CRLOG%"
    rem Use `neq 0` (not `errorlevel 1`): a crash exits with a NEGATIVE code
    rem (e.g. 0xC0000005 access violation), which `if errorlevel 1` misses.
    set "EC=!errorlevel!"
    if !EC! neq 0 (
        type "%CRLOG%"
        set "ISCRASH="
        if !EC! lss 0 set "ISCRASH=1"
        findstr /c:"internal error" "%CRLOG%" >nul 2>nul && set "ISCRASH=1"
        if defined ISCRASH (
            echo   [CRASH] %%f exit=!EC!
            set /a CRASHES+=1
        ) else (
            echo   [BUILD FAIL] %%f
        )
        set /a FAILED+=1
    ) else (
        "%EXEOUT%\%%~nf.exe" >nul
        if !errorlevel! neq 0 (
            echo   [RUN FAIL] %%f
            set /a FAILED+=1
        )
    )
)

rem === Phase 3: negative gate (compilation MUST fail) ===
rem Inverse of Phase 1: these sources pin down what tcc has to REJECT, e.g.
rem using a class name that is currently hidden by a parameter.  KNOWNFAIL
rem only drops a source from gating, so it cannot catch a rejection that
rem silently turns into an acceptance - this loop can.
rem a9\negative\ is a subdirectory, so a9\*.cpp above never picks it up.
rem Matching is done on ASCII only ("<name>.cpp:<line>: error:") because tcc
rem prints diagnostics in CP932 Japanese while the .expected files are UTF-8;
rem comparing the message text itself would compare mismatched code pages.
set "NEGOUT=%TEMP%\tcc_neg_gate"
if not exist "%NEGOUT%" mkdir "%NEGOUT%"
for %%f in (a9\negative\*.cpp) do (
    echo === %%f [negative] ===
    "%TCC%" -c "%%f" -o "%NEGOUT%\%%~nf.o" >"%NEGOUT%\%%~nf.log" 2>&1
    set "EC=!errorlevel!"
    set "ISCRASH="
    if !EC! lss 0 set "ISCRASH=1"
    findstr /c:"internal error" "%NEGOUT%\%%~nf.log" >nul 2>nul && set "ISCRASH=1"
    if defined ISCRASH (
        echo   [CRASH] %%f exit=!EC!
        type "%NEGOUT%\%%~nf.log"
        set /a CRASHES+=1
        set /a FAILED+=1
    ) else if !EC! equ 0 (
        echo   [NEGATIVE FAIL] %%f compiled but was expected to be rejected
        set /a FAILED+=1
    ) else (
        if exist "%%~dpnf.expected" (
            set "NEGPAT="
            for /f "usebackq delims=" %%p in ("%%~dpnf.expected") do set "NEGPAT=%%p"
            findstr /c:"!NEGPAT!" "%NEGOUT%\%%~nf.log" >nul
            if errorlevel 1 (
                echo   [NEGATIVE FAIL] %%f rejected, but not with "!NEGPAT!"
                type "%NEGOUT%\%%~nf.log"
                set /a FAILED+=1
            )
        )
    )
)

echo === a2\mixed_link.bat ===
call a2\mixed_link.bat
if errorlevel 1 set /a FAILED+=1

rem BUG-30: a member declared in a header and defined in another TU must
rem link and still resolve to the right overload (a9\link\ is a subdirectory
rem so the a9\*.cpp globs above never pick these two up individually).
echo === a9\govl_link.bat ===
call a9\govl_link.bat
if errorlevel 1 set /a FAILED+=1

rem BUG-33: static members declared in a header and defined in another TU.
echo === a9\bug33_link.bat ===
call a9\bug33_link.bat
if errorlevel 1 set /a FAILED+=1

rem === Phase 4: crash corpus (C2, crash-prevention plan) ===
rem Real-world sources that are NOT expected to compile yet - only the
rem compiler surviving them is gated.  Diagnostics are fine; a crash
rem (negative exit, or C1's "tcc: internal error" net firing) is not.
rem This keeps "tcc never crashes on the cppunit corpus" true at every
rem commit, long before G7 makes the corpus actually compile.
set "CCOUT=%TEMP%\tcc_crash_corpus"
if not exist "%CCOUT%" mkdir "%CCOUT%"
for %%f in (..\..\sample\cppunit\*.cpp) do (
    "%TCC%" -c -DMINIMUM_SET -Dcu_NO_EXPLICIT -I ..\..\sample\cppunit "%%f" -o "%CCOUT%\%%~nf.o" >nul 2>"%CRLOG%"
    set "EC=!errorlevel!"
    set "ISCRASH="
    if !EC! lss 0 set "ISCRASH=1"
    findstr /c:"internal error" "%CRLOG%" >nul 2>nul && set "ISCRASH=1"
    if defined ISCRASH (
        echo   [CRASH] cppunit corpus: %%~nxf exit=!EC!
        type "%CRLOG%"
        set /a CRASHES+=1
        set /a FAILED+=1
    )
)
echo === crash corpus: done ^(crashes so far: !CRASHES!^) ===

echo === run_all summary: !FAILED! gating failure(s), !CRASHES! crash(es) ===
popd
exit /b !FAILED!
