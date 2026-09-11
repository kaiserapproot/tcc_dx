@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
goto :n7_07b_main

:expect_fail_msg
set "TAG=%~1"
set "SRC=%~2"
set "NEEDLE=%~3"
set "LOG=%OUT%\%~n2.log"
"%TCC%" "%~dp0%SRC%" -o "%OUT%\%~n2.exe" >"%LOG%" 2>&1
if errorlevel 1 (
  findstr /i /c:"%NEEDLE%" "%LOG%" >nul 2>&1
  if errorlevel 1 (
    echo !TAG!=FAIL_NO_DIAG
    set /a BAD=1
  ) else (
    echo !TAG!=PASS
  )
) else (
  echo !TAG!=UNEXPECTED_PASS
  set /a BAD=1
  set /a SILENT+=1
)
exit /b 0

:expect_pass
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~n2.exe"
"%TCC%" "%~dp0%SRC%" -o "%EXE%" >"%OUT%\%~n2.log" 2>&1
if errorlevel 1 (
  echo !TAG!=COMPILE_FAIL
  set /a BAD=1
  exit /b 0
)
"%EXE%" >"%OUT%\%~n2_run.log" 2>&1
if errorlevel 1 (
  echo !TAG!=RUN_FAIL
  set /a BAD=1
) else (
  echo !TAG!=PASS
)
exit /b 0

:probe_only
set "TAG=%~1"
set "SRC=%~2"
set "LOG=%OUT%\%~n2.log"
"%TCC%" "%~dp0%SRC%" -o "%OUT%\%~n2.exe" >"%LOG%" 2>&1
if errorlevel 1 (
  echo !TAG!=COMPILE_FAIL
) else (
  "%OUT%\%~n2.exe" >"%OUT%\%~n2_probe_run.log" 2>&1
  echo !TAG!=COMPILE_PASS_RUN=!ERRORLEVEL!
)
exit /b 0

:n7_07b_main
set "TCC=..\..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_07b_out"
if not exist "%OUT%" mkdir "%OUT%"
set /a BAD=0
set /a SILENT=0

echo === N7-07B LOCAL CLASS ARRAY FAIL-CLOSED ===
echo TCC=%TCC%
echo.

call :expect_pass TOP_LEVEL_LOCAL_CLASS_ARRAY ../n7_07c/n7_07c_local_array_ctor.cpp
call :expect_pass TOP_LEVEL_MULTIDIM_CLASS_ARRAY ../n7_07c/n7_07c_local_array_multidim.cpp
call :expect_pass TRIVIAL_LOCAL_STRUCT_ARRAY n7_07b_trivial_struct_array.cpp
call :expect_pass SCALAR_LOCAL_CLASS_DEFAULT_CTOR n7_07b_scalar_class_control.cpp
call :expect_pass GLOBAL_CLASS_ARRAY_DEFAULT_CTOR n7_07b_global_class_array_control.cpp
call :expect_pass CLASS_MEMBER_ARRAY_DEFAULT_CTOR n7_07b_member_class_array_control.cpp
call :expect_fail_msg CLASS_MEMBER_ARRAY_NONTRIVIAL_DTOR ../n7_07a/n7_07a_member_array_dtor.cpp "member array"
call :expect_fail_msg TLS_CLASS_ARRAY ../n7_07a/n7_07a_tls_class_array.cpp "thread_local"
call :probe_only STATIC_LOCAL_CLASS_ARRAY_BEFORE n7_07b_static_local_probe.cpp
call :probe_only LOCAL_CLASS_ARRAY_EXPLICIT_INIT_BEFORE n7_07b_explicit_init_probe.cpp
echo STATIC_LOCAL_CLASS_ARRAY_SCOPE=DEFERRED_SEPARATE
echo.

echo === N7-07B SUMMARY ===
echo BAD_CODE_ACCEPTED=!SILENT!
echo SILENT_MISCOMPILE_COUNT=!SILENT!
if not "!BAD!"=="0" (
  echo N7_07B=FAIL
  popd
  exit /b 1
)
echo N7_07B=PASS
popd
exit /b 0
