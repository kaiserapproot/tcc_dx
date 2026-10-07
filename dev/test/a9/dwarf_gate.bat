@echo off
rem DWARF gate (PR #31 review): the tccdbg.c fixes must stay debuggable.
rem Builds a C and a C++ source with -gdwarf-4 and with -gdwarf, runs them
rem under dev\gdb.exe in batch mode and requires the exact values of a
rem parameter, locals, a struct, a global, `*this`, reference parameters and
rem member pointers.
rem What this pins down:
rem   - locals: the frame offset was emitted unsigned, so gdb read
rem     rbp+4294967272 ("Cannot access memory").
rem   - C++: member functions were emitted as data members with DW_AT_type
rem     0xffffffff and references with DW_AT_type 0 ("Dwarf Error"); gdb dropped
rem     the whole unit and no breakpoint in it could be hit.
rem   - C++ member pointers had DW_AT_type 0 too ("Cannot find DIE at 0x0"),
rem     and the parameters of a function type in a declarator were listed as
rem     locals ("<\x00>", "unused_param").
rem The break lines are found by marker comment, so the sources can be edited.
rem Sources are in a9\link\.  Exes and logs go to %TEMP%.
setlocal EnableExtensions EnableDelayedExpansion
goto :main

:find_line
rem %1 = file, %2 = marker; sets LINE to the last line that carries it
set "LINE="
for /f "delims=:" %%n in ('%FINDSTR% /n /c:"%~2" "%~1"') do set "LINE=%%n"
goto :eof

:expect_value
rem %1 = log, %2 = gdb value number, %3 = regex for the rest of the line
"%FINDSTR%" /r /c:"^\$%~2 = %~3$" "%~1" >nul 2>&1
if errorlevel 1 (
  echo   missing: $%~2 = %~3
  set /a BAD+=1
)
goto :eof

:forbid
rem %1 = log, %2 = text that must not appear
"%FINDSTR%" /c:"%~2" "%~1" >nul 2>&1
if not errorlevel 1 (
  echo   unexpected: %~2
  set /a BAD+=1
)
goto :eof

:forbid_all
call :forbid "%~1" "Dwarf Error"
call :forbid "%~1" "invalid dwarf2"
call :forbid "%~1" "Cannot find DIE"
call :forbid "%~1" "Cannot access memory"
call :forbid "%~1" "No symbol"
goto :eof

:run_case
rem %1 = debug flag, %2 = tag suffix
set "FLAG=%~1"
set "SUF=%~2"

set /a BAD=0
set "EXE=%OUT%\dwarf_c_%SUF%.exe"
set "LOG=%OUT%\dwarf_c_%SUF%.log"
"%TCC%" %FLAG% dwarf_c.c -o "%EXE%" >"%LOG%" 2>&1
if errorlevel 1 (
  echo DWARF_C_%SUF%=BUILD_FAIL
  type "%LOG%"
  set /a FAILED+=1
  goto :cpp_case
)
call :find_line dwarf_c.c DWARF_C_BREAK
"%GDB%" -batch -ex "break dwarf_c.c:!LINE!" -ex "run" -ex "print arg" -ex "print local" -ex "print copy" -ex "print g_counter" "%EXE%" >"%LOG%" 2>&1
call :expect_value "%LOG%" 1 "41"
call :expect_value "%LOG%" 2 "42"
call :expect_value "%LOG%" 3 "{a = 7, b = 9}"
call :expect_value "%LOG%" 4 "1276"
call :forbid_all "%LOG%"
if "!BAD!"=="0" (
  echo DWARF_C_%SUF%=PASS
) else (
  echo DWARF_C_%SUF%=FAIL problems=!BAD!
  type "%LOG%"
  set /a FAILED+=1
)

:cpp_case
set /a BAD=0
set "EXE=%OUT%\dwarf_cpp_%SUF%.exe"
set "LOG=%OUT%\dwarf_cpp_%SUF%.log"
"%TCC%" %FLAG% dwarf_cpp.cpp -o "%EXE%" >"%LOG%" 2>&1
if errorlevel 1 (
  echo DWARF_CPP_%SUF%=BUILD_FAIL
  type "%LOG%"
  set /a FAILED+=1
  goto :eof
)
call :find_line dwarf_cpp.cpp DWARF_CPP_BREAK_MEMBER
set "LMEM=!LINE!"
call :find_line dwarf_cpp.cpp DWARF_CPP_BREAK_REF
set "LREF=!LINE!"
call :find_line dwarf_cpp.cpp DWARF_CPP_BREAK_MPTR
set "LMP=!LINE!"
"%GDB%" -batch -ex "break dwarf_cpp.cpp:!LMEM!" -ex "break dwarf_cpp.cpp:!LREF!" -ex "break dwarf_cpp.cpp:!LMP!" -ex "run" -ex "print *this" -ex "print v" -ex "delete 1" -ex "continue" -ex "print local" -ex "print a" -ex "print out" -ex "continue" -ex "print pm" -ex "print pf" -ex "print p.*pm" -ex "info locals" "%EXE%" >"%LOG%" 2>&1
call :expect_value "%LOG%" 1 "{total = 7, count = 0}"
call :expect_value "%LOG%" 2 ".*: 7"
call :expect_value "%LOG%" 3 "42"
call :expect_value "%LOG%" 4 ".*{total = 42, count = 2}"
call :expect_value "%LOG%" 5 ".*: 84"
rem ( ) & < > in an expected value would break the echo in :expect_value and
rem :forbid, so they are matched with "." there, and the "<\x00> = " local is
rem searched for directly.
call :expect_value "%LOG%" 6 ".pt::y"
call :expect_value "%LOG%" 7 ".int .\*..int.. 0x[0-9a-f]* .*twice.*"
call :expect_value "%LOG%" 8 "9"
"%FINDSTR%" /c:"x00> = " "%LOG%" >nul 2>&1
if not errorlevel 1 echo   unexpected: a local without a name
if not errorlevel 1 set /a BAD+=1
call :forbid "%LOG%" "unused_param"
call :forbid_all "%LOG%"
if "!BAD!"=="0" (
  echo DWARF_CPP_%SUF%=PASS
) else (
  echo DWARF_CPP_%SUF%=FAIL problems=!BAD!
  type "%LOG%"
  set /a FAILED+=1
)
goto :eof

:main
pushd "%~dp0link"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "GDB=..\..\..\gdb.exe"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set "OUT=%TEMP%\tcc_dwarf_gate"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0

echo === DWARF gate ===
if not exist "%GDB%" (
  echo DWARF_GATE=FAIL dev\gdb.exe is missing
  popd
  exit /b 1
)
call :run_case -gdwarf-4 V4
call :run_case -gdwarf DEFAULT

if not "!FAILED!"=="0" (
  echo DWARF_GATE=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo DWARF_GATE=PASS
popd
exit /b 0
