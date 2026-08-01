@echo off
REM ---------------------------------------------------------------------------
REM Rebuild libcunit in C++ mode (.cpp) and verify it behaves like the C build.
REM
REM   cunit/      : original C sources (unmodified)
REM   cunit_cpp/  : C++ sources (.cpp); test_cunit.cpp uses a class internally
REM
REM Usage: run from the dev directory:  cunit\make_lib_cpp.bat
REM ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0\.."
set TCC=%CD%\tcc.exe
set SRC=cunit_cpp
set OBJ=cunit_cpp_build
set CSRC=cunit
set COBJ=cunit_build
set FILES=Automated Basic Console CUError MyMem TestDB TestRun test_cunit Util Win

if not exist "%OBJ%" mkdir "%OBJ%"
if not exist "%COBJ%" mkdir "%COBJ%"

echo === build library as C++ ===
for %%F in (%FILES%) do (
  "%TCC%" -c -m64 -I %SRC% %SRC%\%%F.cpp -o %OBJ%\%%F.o
  if errorlevel 1 exit /b 1
)
"%TCC%" -ar rcs %OBJ%\libcunit_cpp.a %OBJ%\Basic.o %OBJ%\Console.o %OBJ%\CUError.o %OBJ%\MyMem.o %OBJ%\TestDB.o %OBJ%\TestRun.o %OBJ%\test_cunit.o %OBJ%\Util.o %OBJ%\Win.o %OBJ%\Automated.o
if errorlevel 1 exit /b 1

echo === build reference library as C ===
for %%F in (%FILES%) do (
  "%TCC%" -c -m64 -I %CSRC% %CSRC%\%%F.c -o %COBJ%\%%F.o
  if errorlevel 1 exit /b 1
)
"%TCC%" -ar rcs %COBJ%\libcunit_c.a %COBJ%\Basic.o %COBJ%\Console.o %COBJ%\CUError.o %COBJ%\MyMem.o %COBJ%\TestDB.o %COBJ%\TestRun.o %COBJ%\test_cunit.o %COBJ%\Util.o %COBJ%\Win.o %COBJ%\Automated.o
if errorlevel 1 exit /b 1

echo === compare output of the same test built both ways ===
"%TCC%" -m64 -I %CSRC% %CSRC%\test.c   %COBJ%\libcunit_c.a   -o %COBJ%\test_c.exe
if errorlevel 1 exit /b 1
"%TCC%" -m64 -I %SRC%  %SRC%\test.cpp  %OBJ%\libcunit_cpp.a  -o %OBJ%\test_cpp.exe
if errorlevel 1 exit /b 1
REM The elapsed-time line varies between runs, so drop it before comparing.
%COBJ%\test_c.exe  2>&1 | findstr /v /c:"seco" > %COBJ%\out_c.txt
%OBJ%\test_cpp.exe 2>&1 | findstr /v /c:"seco" > %OBJ%\out_cpp.txt
fc %COBJ%\out_c.txt %OBJ%\out_cpp.txt > nul
if errorlevel 1 (
  echo [NG] C and C++ builds produced different output
  exit /b 1
)
echo [OK] C and C++ builds produce identical output

echo === run the C++ feature suite on top of libcunit ===
"%TCC%" -m64 -I %SRC% %SRC%\cpp_suite.cpp %OBJ%\libcunit_cpp.a -o %OBJ%\cpp_suite.exe
if errorlevel 1 exit /b 1
%OBJ%\cpp_suite.exe
if errorlevel 1 (
  echo [NG] C++ feature suite failed
  exit /b 1
)

echo.
echo ALL OK
endlocal
