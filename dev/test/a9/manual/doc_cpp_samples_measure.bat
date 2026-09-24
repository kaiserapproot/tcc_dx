@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

rem Qualification for the C++ feature guide, two halves:
rem   1. every fenced cpp / c block with an int main() must build AND run with
rem      exit 0 using dev\tcc.exe - the guide claims the samples are buildable
rem   2. every `file.c:N` / `file.c:A-B` citation must be a real place in that
rem      file, and a `name()` next to it must really be there.  A bulk
rem      line-number update is easy to get half-right: the first attempt shifted
rem      only the start of each range and left impossible A>B spans behind.
rem
rem The guide's file name is not ASCII, so both tools locate it by the ASCII
rem marker "doc-sample-qualification" that the guide carries near its top.

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "ROOT=%~dp0..\..\..\.."
set "OUT=%TEMP%\tcc_doc_cpp_samples"
if exist "%OUT%" rd /s /q "%OUT%"
mkdir "%OUT%"

echo === C++ guide sample qualification ===

"%TCC%" doc_sample_extract.c -o "%OUT%\doc_sample_extract.exe" >nul 2>&1
if errorlevel 1 (
    echo DOC_CPP_SAMPLES_EXTRACTOR=BUILD_FAIL
    echo DOC_CPP_SAMPLES_FAIL=1
    popd
    exit /b 1
)

rem The extractor locates the guide itself: the file name is not ASCII and a
rem batch file cannot hand such a name to a child process without the console
rem code page mangling it.  Only the (ASCII) directory is passed.
"%OUT%\doc_sample_extract.exe" "%ROOT%" "%OUT%" >"%OUT%\index.txt"
if errorlevel 1 (
    echo DOC_CPP_SAMPLES_EXTRACT=FAIL
    echo DOC_CPP_SAMPLES_FAIL=1
    popd
    exit /b 1
)

set /a TOTAL=0
set /a CPASS=0
set /a RPASS=0
set /a FAILED=0

for %%f in ("%OUT%\doc_sample_*.c" "%OUT%\doc_sample_*.cpp") do (
    set /a TOTAL+=1
    "%TCC%" "%%f" -o "%%~dpnf.exe" 2>"%OUT%\err.txt"
    if errorlevel 1 (
        echo   [COMPILE FAIL] %%~nxf
        type "%OUT%\err.txt"
        set /a FAILED+=1
    ) else (
        set /a CPASS+=1
        "%%~dpnf.exe" >nul 2>&1
        if errorlevel 1 (
            echo   [RUN FAIL] %%~nxf exit=!errorlevel!
            set /a FAILED+=1
        ) else (
            set /a RPASS+=1
        )
    )
)

echo DOC_CPP_SAMPLES_TOTAL=!TOTAL!
echo DOC_CPP_SAMPLES_COMPILE_PASS=!CPASS!
echo DOC_CPP_SAMPLES_RUNTIME_PASS=!RPASS!
echo DOC_CPP_SAMPLES_FAIL=!FAILED!

echo.
"%TCC%" doc_source_refs_check.c -w -o "%OUT%\doc_source_refs_check.exe" >nul 2>&1
if errorlevel 1 (
    echo DOC_SOURCE_REFS_CHECKER=BUILD_FAIL
    set /a FAILED+=1
) else (
    "%OUT%\doc_source_refs_check.exe" "%ROOT%"
    if errorlevel 1 set /a FAILED+=1
)

popd
if not "!FAILED!"=="0" exit /b 1
exit /b 0
