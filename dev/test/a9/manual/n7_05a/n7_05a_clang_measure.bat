@echo off
setlocal EnableExtensions
pushd "%~dp0"

set CLANG=clang-cl
set OUT=n7_05a_out
if not exist "%OUT%" mkdir "%OUT%"
if exist "%OUT%\clang_summary.txt" del /f "%OUT%\clang_summary.txt"

call :run_case identical_typedef n7_05a_identical_typedef.cpp
call :run_case incompatible_typedef n7_05a_incompatible_typedef.cpp
call :run_case struct_tag_function n7_05a_struct_tag_function.cpp
call :run_case typedef_function n7_05a_typedef_function.cpp
call :run_case amateras_exact n7_05a_amateras_exact.cpp
call :run_case single_include n7_05a_single_include.cpp
call :run_case double_include n7_05a_double_include.cpp
call :run_case extern_c_to_cpp n7_05a_extern_c_to_cpp.cpp
call :run_case cpp_to_extern_c n7_05a_cpp_to_extern_c.cpp

echo === N7-05A CLANG_CL oracle summary ===
type "%OUT%\clang_summary.txt"

popd
exit /b 0

:run_case
set NAME=%~1
set SRC=%~2
echo --- CLANG %NAME% ---
"%CLANG%" /nologo /TP /W3 /c /I"%CD%" "%SRC%" /Fo"%OUT%\%NAME%.clang.obj" > "%OUT%\%NAME%.clang.log" 2>&1
if errorlevel 1 (
  echo %NAME% CLANG=FAIL
  echo %NAME% CLANG=FAIL>> "%OUT%\clang_summary.txt"
) else (
  echo %NAME% CLANG=PASS
  echo %NAME% CLANG=PASS>> "%OUT%\clang_summary.txt"
)
exit /b 0
