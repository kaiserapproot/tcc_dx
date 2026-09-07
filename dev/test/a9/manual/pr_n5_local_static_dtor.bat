@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_pr_n5_local_static_dtor"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set "LOG=%OUT%\local_static_dtor.log"
set "FAILED=0"

echo === PR-N5 function-local static destructor qualification ===
"%TCC%" ..\local_static_dtor.cpp -o "%OUT%\local_static_dtor.exe" >"%LOG%.compile" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%.compile"
  echo PR_N5_LOCAL_STATIC_DTOR=COMPILE_FAIL
  popd
  exit /b 1
)
"%OUT%\local_static_dtor.exe" >"%LOG%" 2>&1
if not "!errorlevel!"=="0" (
  type "%LOG%"
  echo PR_N5_LOCAL_STATIC_DTOR=RUN_FAIL
  popd
  exit /b 1
)

call :check_once "C5"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "C7"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "C11"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "CC111"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "END"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "D111"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "D11"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "D7"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "D5"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_once "DQ13"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_absent "C99"
if not "!errorlevel!"=="0" set "FAILED=1"
call :check_absent "D99"
if not "!errorlevel!"=="0" set "FAILED=1"
if not "!FAILED!"=="0" (
  echo PR_N5_LOCAL_STATIC_DTOR=FAIL
  popd
  exit /b 1
)

call :line_of "END" END_LINE
call :line_of "D111" D111_LINE
call :line_of "D11" D11_LINE
call :line_of "D7" D7_LINE
call :line_of "D5" D5_LINE
call :line_of "DQ13" DQ13_LINE
if !END_LINE! geq !D111_LINE! goto order_fail
if !D111_LINE! geq !D11_LINE! goto order_fail
if !D11_LINE! geq !D7_LINE! goto order_fail
if !D7_LINE! geq !D5_LINE! goto order_fail
if !DQ13_LINE! geq !D111_LINE! goto order_fail
set "RUNLOG=%OUT%\local_static_dtor.run.log"
"%TCC%" -run ..\local_static_dtor.cpp >"%RUNLOG%" 2>&1
if not "!errorlevel!"=="0" goto run_fail
call :count_marker "%RUNLOG%" "END"
if not "!COUNT!"=="1" goto run_fail
call :count_marker "%RUNLOG%" "D5"
if not "!COUNT!"=="1" goto run_fail
call :count_marker "%RUNLOG%" "DQ13"
if not "!COUNT!"=="1" goto run_fail
echo LOCAL_STATIC_DTOR_TCC_RUN=PASS

set "RUNATEXITLOG=%OUT%\run_atexit.log"
"%TCC%" -run pr_n5_run_atexit.c >"%RUNATEXITLOG%" 2>&1
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNATEXITLOG%" "M" RUN_M
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNATEXITLOG%" "F" RUN_F
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNATEXITLOG%" "A2" RUN_A2
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNATEXITLOG%" "A1" RUN_A1
if not "!errorlevel!"=="0" goto compat_fail
if !RUN_M! geq !RUN_F! goto compat_fail
if !RUN_F! geq !RUN_A2! goto compat_fail
if !RUN_A2! geq !RUN_A1! goto compat_fail
echo TCC_RUN_ATEXIT=PASS
echo TCC_RUN_FINI_ARRAY=PASS

set "RUNEXITLOG=%OUT%\run_exit.log"
"%TCC%" -run pr_n5_run_exit.c >"%RUNEXITLOG%" 2>&1
if not "!errorlevel!"=="7" goto compat_fail
call :check_line_in "%RUNEXITLOG%" "M" EXIT_M
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNEXITLOG%" "F" EXIT_F
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNEXITLOG%" "A" EXIT_A
if not "!errorlevel!"=="0" goto compat_fail
if !EXIT_M! geq !EXIT_F! goto compat_fail
if !EXIT_F! geq !EXIT_A! goto compat_fail
echo TCC_RUN_EXIT=PASS

set "RUNONEXITLOG=%OUT%\run_on_exit.log"
"%TCC%" -run pr_n5_run_on_exit.c >"%RUNONEXITLOG%" 2>&1
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNONEXITLOG%" "M" ONEXIT_M
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNONEXITLOG%" "ON" ONEXIT_ON
if not "!errorlevel!"=="0" goto compat_fail
call :check_absent_in "%RUNONEXITLOG%" "ON_BAD"
if not "!errorlevel!"=="0" goto compat_fail
if !ONEXIT_M! geq !ONEXIT_ON! goto compat_fail
echo TCC_RUN_ON_EXIT=PASS

set "RUNCPPLOG=%OUT%\run_cpp.log"
"%TCC%" -run pr_n5_run_cpp.cpp >"%RUNCPPLOG%" 2>&1
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNCPPLOG%" "GC" CPP_GC
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNCPPLOG%" "LC" CPP_LC
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNCPPLOG%" "M" CPP_M
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNCPPLOG%" "LD" CPP_LD
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNCPPLOG%" "GD" CPP_GD
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%RUNCPPLOG%" "A" CPP_A
if not "!errorlevel!"=="0" goto compat_fail
call :check_cpp_order "%RUNCPPLOG%"
if not "!errorlevel!"=="0" goto cpp_order_fail
echo TCC_RUN_CPP_DTOR_RETURN=PASS

set "RUNCPPEXITLOG=%OUT%\run_cpp_exit.log"
"%TCC%" -DPR_N5_EXIT -run pr_n5_run_cpp.cpp >"%RUNCPPEXITLOG%" 2>&1
if not "!errorlevel!"=="0" goto cpp_order_fail
call :check_cpp_order "%RUNCPPEXITLOG%"
if not "!errorlevel!"=="0" goto cpp_order_fail
echo TCC_RUN_CPP_DTOR_EXIT=PASS

set "EXECPPLOG=%OUT%\exe_cpp.log"
"%TCC%" pr_n5_run_cpp.cpp -o "%OUT%\run_cpp.exe" >"%EXECPPLOG%.compile" 2>&1
if not "!errorlevel!"=="0" goto cpp_order_fail
"%OUT%\run_cpp.exe" >"%EXECPPLOG%" 2>&1
if not "!errorlevel!"=="0" goto cpp_order_fail
call :check_cpp_order "%EXECPPLOG%"
if not "!errorlevel!"=="0" goto cpp_order_fail
echo TCC_EXE_CPP_DTOR_RETURN=PASS

set "EXECPPEXITLOG=%OUT%\exe_cpp_exit.log"
"%TCC%" -DPR_N5_EXIT pr_n5_run_cpp.cpp -o "%OUT%\run_cpp_exit.exe" >"%EXECPPEXITLOG%.compile" 2>&1
if not "!errorlevel!"=="0" goto cpp_order_fail
"%OUT%\run_cpp_exit.exe" >"%EXECPPEXITLOG%" 2>&1
if not "!errorlevel!"=="0" goto cpp_order_fail
call :check_cpp_order "%EXECPPEXITLOG%"
if not "!errorlevel!"=="0" goto cpp_order_fail
echo TCC_EXE_CPP_DTOR_EXIT=PASS

set "STRESSLOG=%OUT%\dtor_registry_stress_1024.log"
"%TCC%" -run -DPR_N5_STRESS_COUNT=1024 pr_n5_dtor_registry_stress.c >"%STRESSLOG%" 2>&1
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%STRESSLOG%" "STRESS_OK" STRESS1024_OK_LINE
if not "!errorlevel!"=="0" goto compat_fail
echo LOCAL_STATIC_DTOR_REGISTRY_1024=PASS

set "STRESSLOG=%OUT%\dtor_registry_stress_1025.log"
"%TCC%" -run -DPR_N5_STRESS_COUNT=1025 pr_n5_dtor_registry_stress.c >"%STRESSLOG%" 2>&1
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%STRESSLOG%" "STRESS_OK" STRESS1025_OK_LINE
if not "!errorlevel!"=="0" goto compat_fail
echo LOCAL_STATIC_DTOR_REGISTRY_1025=PASS

set "STRESSLOG=%OUT%\dtor_registry_stress_4096.log"
"%TCC%" -run -DPR_N5_STRESS_COUNT=4096 pr_n5_dtor_registry_stress.c >"%STRESSLOG%" 2>&1
if not "!errorlevel!"=="0" goto compat_fail
call :check_line_in "%STRESSLOG%" "STRESS_OK" STRESS4096_OK_LINE
if not "!errorlevel!"=="0" goto compat_fail
echo LOCAL_STATIC_DTOR_REGISTRY_4096=PASS

set "GLOBAL_EXITLOG=%OUT%\global_ctor_exit.log"
"%TCC%" pr_n5_global_ctor_exit.cpp -o "%OUT%\global_ctor_exit.exe" >"%GLOBAL_EXITLOG%.compile" 2>&1
if not "!errorlevel!"=="0" goto global_exit_fail
"%OUT%\global_ctor_exit.exe" >"%GLOBAL_EXITLOG%" 2>&1
if not "!errorlevel!"=="7" goto global_exit_fail
call :check_line_in "%GLOBAL_EXITLOG%" "AC" GLOBAL_AC
if not "!errorlevel!"=="0" goto global_exit_fail
call :check_line_in "%GLOBAL_EXITLOG%" "BC" GLOBAL_BC
if not "!errorlevel!"=="0" goto global_exit_fail
call :check_line_in "%GLOBAL_EXITLOG%" "AD" GLOBAL_AD
if not "!errorlevel!"=="0" goto global_exit_fail
call :check_absent_in "%GLOBAL_EXITLOG%" "BD"
if not "!errorlevel!"=="0" goto global_exit_fail
call :check_absent_in "%GLOBAL_EXITLOG%" "MAIN"
if not "!errorlevel!"=="0" goto global_exit_fail
if !GLOBAL_AC! geq !GLOBAL_BC! goto global_exit_fail
if !GLOBAL_BC! geq !GLOBAL_AD! goto global_exit_fail
echo TCC_RUN_GLOBAL_CTOR_EXIT=PASS

set "DLLLOG=%OUT%\local_static_dtor_dll.log"
"%TCC%" -shared pr_n5_local_static_dtor_dll.cpp -o "%OUT%\local_static_dtor.dll" >"%DLLLOG%" 2>&1
if "!errorlevel!"=="0" goto dll_policy_fail
"%FINDSTR%" /c:"function-local static destructor in DLL is unsupported" "%DLLLOG%" >nul 2>nul
if errorlevel 1 goto dll_policy_fail
echo CPP_LOCAL_STATIC_DTOR_DLL=UNSUPPORTED_FAIL_CLOSED
set "DLLOBJ=%OUT%\local_static_dtor_dll.o"
"%TCC%" -c pr_n5_local_static_dtor_dll.cpp -o "%DLLOBJ%" >"%DLLLOG%.object" 2>&1
if not "!errorlevel!"=="0" goto dll_policy_fail
"%TCC%" -shared "%DLLOBJ%" -o "%OUT%\local_static_dtor_object.dll" >"%DLLLOG%.object_link" 2>&1
if "!errorlevel!"=="0" goto dll_policy_fail
"%FINDSTR%" /c:"C++ destructor runtime in DLL is unsupported" "%DLLLOG%.object_link" >nul 2>nul
if errorlevel 1 goto dll_policy_fail
echo CPP_LOCAL_STATIC_DTOR_DLL_OBJECT=UNSUPPORTED_FAIL_CLOSED

set "MULTIOUT=%OUT%\multi"
if not exist "%MULTIOUT%" mkdir "%MULTIOUT%"
set "MULTILOG=%MULTIOUT%\local_static_multi.log"
"%TCC%" -c local_static_multi_a.cpp -o "%MULTIOUT%\a.o" >"%MULTILOG%.a" 2>&1
if not "!errorlevel!"=="0" goto multi_fail
"%TCC%" -c local_static_multi_b.cpp -o "%MULTIOUT%\b.o" >"%MULTILOG%.b" 2>&1
if not "!errorlevel!"=="0" goto multi_fail
"%TCC%" "%MULTIOUT%\a.o" "%MULTIOUT%\b.o" local_static_multi_main.cpp -o "%MULTIOUT%\local_static_multi.exe" >"%MULTILOG%.link" 2>&1
if not "!errorlevel!"=="0" goto multi_fail
"%MULTIOUT%\local_static_multi.exe" >"%MULTILOG%" 2>&1
if not "!errorlevel!"=="0" goto multi_fail
call :count_marker "%MULTILOG%" "END"
if not "!COUNT!"=="1" goto multi_fail
call :count_marker "%MULTILOG%" "DB"
if not "!COUNT!"=="1" goto multi_fail
call :count_marker "%MULTILOG%" "DA"
if not "!COUNT!"=="1" goto multi_fail
call :line_marker "%MULTILOG%" "END" MULTI_END_LINE
call :line_marker "%MULTILOG%" "DB" MULTI_DB_LINE
call :line_marker "%MULTILOG%" "DA" MULTI_DA_LINE
if !MULTI_END_LINE! geq !MULTI_DB_LINE! goto multi_fail
if !MULTI_DB_LINE! geq !MULTI_DA_LINE! goto multi_fail
echo LOCAL_STATIC_DTOR_MULTI_TU=PASS
set "PUREOUT=%MULTIOUT%\pure"
if not exist "%PUREOUT%" mkdir "%PUREOUT%"
set "PURELOG=%PUREOUT%\local_static_multi_pure.log"
"%TCC%" -c local_static_multi_main.cpp -o "%PUREOUT%\main.o" >"%PURELOG%.main" 2>&1
if not "!errorlevel!"=="0" goto pure_fail
"%TCC%" "%MULTIOUT%\a.o" "%MULTIOUT%\b.o" "%PUREOUT%\main.o" -o "%PUREOUT%\local_static_multi_pure.exe" >"%PURELOG%.link" 2>&1
if not "!errorlevel!"=="0" goto pure_fail
"%PUREOUT%\local_static_multi_pure.exe" >"%PURELOG%" 2>&1
if not "!errorlevel!"=="0" goto pure_fail
call :check_line_in "%PURELOG%" "END" PURE_END
if not "!errorlevel!"=="0" goto pure_fail
call :check_line_in "%PURELOG%" "DB" PURE_DB
if not "!errorlevel!"=="0" goto pure_fail
call :check_line_in "%PURELOG%" "DA" PURE_DA
if not "!errorlevel!"=="0" goto pure_fail
if !PURE_END! geq !PURE_DB! goto pure_fail
if !PURE_DB! geq !PURE_DA! goto pure_fail
echo LOCAL_STATIC_DTOR_PURE_OBJECT_LINK=PASS
echo LOCAL_STATIC_DTOR=PASS
echo LOCAL_STATIC_DTOR_REVERSE_ORDER=PASS
echo PR_N5_LOCAL_STATIC_DTOR=PASS
popd
exit /b 0

:run_fail
type "%RUNLOG%"
echo LOCAL_STATIC_DTOR_TCC_RUN=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:compat_fail
if exist "%RUNATEXITLOG%" type "%RUNATEXITLOG%"
if exist "%RUNEXITLOG%" type "%RUNEXITLOG%"
if exist "%RUNONEXITLOG%" type "%RUNONEXITLOG%"
if exist "%RUNCPPLOG%" type "%RUNCPPLOG%"
if exist "%STRESSLOG%" type "%STRESSLOG%"
echo TCC_RUN_COMPAT=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:cpp_order_fail
if exist "%RUNCPPLOG%" type "%RUNCPPLOG%"
if exist "%RUNCPPEXITLOG%" type "%RUNCPPEXITLOG%"
if exist "%EXECPPLOG%.compile" type "%EXECPPLOG%.compile"
if exist "%EXECPPLOG%" type "%EXECPPLOG%"
if exist "%EXECPPEXITLOG%.compile" type "%EXECPPEXITLOG%.compile"
if exist "%EXECPPEXITLOG%" type "%EXECPPEXITLOG%"
echo CPP_DTOR_ATEXIT_ORDER=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:global_exit_fail
if exist "%GLOBAL_EXITLOG%.compile" type "%GLOBAL_EXITLOG%.compile"
if exist "%GLOBAL_EXITLOG%" type "%GLOBAL_EXITLOG%"
echo TCC_RUN_GLOBAL_CTOR_EXIT=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:dll_policy_fail
type "%DLLLOG%"
if exist "%DLLLOG%.object" type "%DLLLOG%.object"
if exist "%DLLLOG%.object_link" type "%DLLLOG%.object_link"
echo CPP_LOCAL_STATIC_DTOR_DLL=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:pure_fail
if exist "%PURELOG%.main" type "%PURELOG%.main"
if exist "%PURELOG%.link" type "%PURELOG%.link"
if exist "%PURELOG%" type "%PURELOG%"
echo LOCAL_STATIC_DTOR_PURE_OBJECT_LINK=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:multi_fail
if exist "%MULTILOG%.a" type "%MULTILOG%.a"
if exist "%MULTILOG%.b" type "%MULTILOG%.b"
if exist "%MULTILOG%.link" type "%MULTILOG%.link"
if exist "%MULTILOG%" type "%MULTILOG%"
echo LOCAL_STATIC_DTOR_MULTI_TU=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:order_fail
type "%LOG%"
echo LOCAL_STATIC_DTOR_REVERSE_ORDER=FAIL
echo PR_N5_LOCAL_STATIC_DTOR=FAIL
popd
exit /b 1

:count_marker
set "COUNT=0"
set "MATCH=%OUT%\count_%RANDOM%.txt"
"%FINDSTR%" /n /x /c:"%~2" "%~1" >"%MATCH%" 2>nul
for /f "usebackq delims=" %%a in ("%MATCH%") do set /a COUNT+=1
exit /b 0

:line_marker
set "MATCH=%OUT%\line_%RANDOM%.txt"
"%FINDSTR%" /n /x /c:"%~2" "%~1" >"%MATCH%" 2>nul
for /f "usebackq tokens=1 delims=:" %%n in ("%MATCH%") do set "%~3=%%n"
exit /b 0

:check_once
set "MATCH=%OUT%\check_once_%RANDOM%.txt"
"%FINDSTR%" /n /x /c:"%~1" "%LOG%" >"%MATCH%" 2>nul
set "COUNT=0"
for /f "usebackq delims=" %%a in ("%MATCH%") do set /a COUNT+=1
if "!COUNT!"=="1" exit /b 0
echo expected exactly one line: %~1
type "%LOG%"
exit /b 1

:check_absent
set "MATCH=%OUT%\check_absent_%RANDOM%.txt"
"%FINDSTR%" /n /x /c:"%~1" "%LOG%" >"%MATCH%" 2>nul
for /f "usebackq delims=" %%a in ("%MATCH%") do (
  echo unexpected line: %~1
  type "%LOG%"
  exit /b 1
)
exit /b 0

:line_of
set "MATCH=%OUT%\line_of_%RANDOM%.txt"
"%FINDSTR%" /n /x /c:"%~1" "%LOG%" >"%MATCH%" 2>nul
for /f "usebackq tokens=1 delims=:" %%n in ("%MATCH%") do set "%~2=%%n"
exit /b 0

:check_line_in
set "MATCH=%OUT%\check_line_%RANDOM%.txt"
"%FINDSTR%" /n /x /c:"%~2" "%~1" >"%MATCH%" 2>nul
set "CHECK_COUNT=0"
for /f "usebackq tokens=1 delims=:" %%n in ("%MATCH%") do (
  set /a CHECK_COUNT+=1
  set "%~3=%%n"
)
if "!CHECK_COUNT!"=="1" exit /b 0
exit /b 1

:check_absent_in
set "MATCH=%OUT%\check_absent_in_%RANDOM%.txt"
"%FINDSTR%" /n /x /c:"%~2" "%~1" >"%MATCH%" 2>nul
for /f "usebackq delims=" %%a in ("%MATCH%") do exit /b 1
exit /b 0

:check_cpp_order
call :check_line_in "%~1" "GC" CPP_ORDER_GC
if not "!errorlevel!"=="0" exit /b 1
call :check_line_in "%~1" "LC" CPP_ORDER_LC
if not "!errorlevel!"=="0" exit /b 1
call :check_line_in "%~1" "M" CPP_ORDER_M
if not "!errorlevel!"=="0" exit /b 1
call :check_line_in "%~1" "LD" CPP_ORDER_LD
if not "!errorlevel!"=="0" exit /b 1
call :check_line_in "%~1" "A" CPP_ORDER_A
if not "!errorlevel!"=="0" exit /b 1
call :check_line_in "%~1" "GD" CPP_ORDER_GD
if not "!errorlevel!"=="0" exit /b 1
if !CPP_ORDER_GC! geq !CPP_ORDER_LC! exit /b 1
if !CPP_ORDER_LC! geq !CPP_ORDER_M! exit /b 1
if !CPP_ORDER_M! geq !CPP_ORDER_LD! exit /b 1
if !CPP_ORDER_LD! geq !CPP_ORDER_A! exit /b 1
if !CPP_ORDER_A! geq !CPP_ORDER_GD! exit /b 1
exit /b 0
