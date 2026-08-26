@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "ROOT=%~dp0..\.."
set "TCC=..\..\dev\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_cppunit_gate_%RANDOM%_%RANDOM%"
set /a FAILED=0
if not exist "%OUT%" mkdir "%OUT%"

if not exist "%TCC%" (
    echo [G7 FAIL] compiler not found: %TCC%
    set /a FAILED+=1
)

rem The twelve original library translation units are explicit.  The
rem baseline-integrity gate below derives its own 31-file set from git.
set "LIB_SRCS=RepeatedTest.cpp SimpleList.cpp SimpleString.cpp TestCase.cpp TestDecorator.cpp TestFailure.cpp TestRegistry.cpp TestResult.cpp TestRunner.cpp TestSetup.cpp TestSuite.cpp TestUtility.cpp"
for %%S in (%LIB_SRCS% all_test_tpp.cpp) do (
    echo === CPPUnit compile %%S ===
    "%TCC%" -c -DMINIMUM_SET -Dcu_NO_EXPLICIT -I . -w "%%S" -o "%OUT%\%%~nS.o" >"%OUT%\%%~nS.compile.log" 2>&1
    if !errorlevel! neq 0 (
        echo [G7 FAIL] compile: %%S
        type "%OUT%\%%~nS.compile.log"
        set /a FAILED+=1
    )
)

set "OBJECTS="
for %%O in ("%OUT%\*.o") do set "OBJECTS=!OBJECTS! "%%~fO""
echo === CPPUnit link ===
"%TCC%" !OBJECTS! -luser32 -lkernel32 -o "%OUT%\all_test_tpp.exe" >"%OUT%\link.log" 2>&1
if !errorlevel! neq 0 (
    echo [G7 FAIL] link
    type "%OUT%\link.log"
    set /a FAILED+=1
)

if exist "%OUT%\all_test_tpp.exe" (
    echo === CPPUnit run ===
    "%OUT%\all_test_tpp.exe" >"%OUT%\run.log" 2>&1
    set "RUN_EC=!errorlevel!"
    type "%OUT%\run.log"
    if !RUN_EC! neq 0 (
        echo [G7 FAIL] executable exit code !RUN_EC!
        set /a FAILED+=1
    )
    call :g7_require_line "%OUT%\run.log" "TESTS:17"
    call :g7_require_line "%OUT%\run.log" "FAILURES:0"
    call :g7_require_line "%OUT%\run.log" "ERRORS:0"
    for %%N in (test_case_lifecycle test_case_failure_record test_assert_equals test_result_counts test_failure_detail test_suite_run test_registry test_runner_run test_repeated test_decorator test_setup_hooks test_listener test_simple_string test_simple_string_grow test_simple_list test_simple_list_iter test_auto_ptr) do (
        call :g7_require_line "%OUT%\run.log" "PASS:%%N"
    )
) else (
    echo [G7 FAIL] executable was not produced
    set /a FAILED+=1
)

rem Baseline identity is part of the gate, not a reviewer-maintained list.
for /f "delims=" %%H in ('git -C "%ROOT%" rev-parse cppunit-original-base 2^>nul') do if not defined BASE_HASH set "BASE_HASH=%%H"
if /i not "!BASE_HASH!"=="ad882a3c5673a238354d6ad72bac88342a18335e" (
    echo [G7 FAIL] unexpected cppunit-original-base: !BASE_HASH!
    set /a FAILED+=1
)

set /a BASE_COUNT=0
git -C "%ROOT%" ls-tree -r --name-only cppunit-original-base -- sample/cppunit >"%OUT%\baseline.list" 2>nul
if errorlevel 1 (
    echo [G7 FAIL] cannot enumerate baseline tree
    set /a FAILED+=1
)
for /f "delims=" %%F in ('findstr /r /i "\.cpp" "%OUT%\baseline.list"') do (
    if /i not "%%F"=="sample/cppunit/all_test.cpp" call :g7_check_original "%%F"
)
for /f "delims=" %%F in ('findstr /r /i "\.h" "%OUT%\baseline.list"') do (
    call :g7_check_original "%%F"
)
if not "!BASE_COUNT!"=="31" (
    echo [G7 FAIL] baseline source/header count !BASE_COUNT! ^(expected 31^)
    set /a FAILED+=1
) else (
    echo [G7 PASS] baseline source/header count 31
)

echo === CPPUnit G7 summary: !FAILED! failure^(s^) ===
if exist "%OUT%" rmdir /s /q "%OUT%"
popd
exit /b !FAILED!

:g7_require_line
findstr /x /c:"%~2" "%~1" >nul 2>nul
if errorlevel 1 (
    echo [G7 FAIL] missing exact line: %~2
    set /a FAILED+=1
)
exit /b 0

:g7_check_original
if /i not "%~x1"==".cpp" if /i not "%~x1"==".h" exit /b 0
set /a BASE_COUNT+=1
git -C "%ROOT%" diff --exit-code cppunit-original-base -- "%~1" >nul 2>nul
if errorlevel 1 (
    echo [G7 FAIL] modified baseline file: %~1
    set /a FAILED+=1
)
exit /b 0
