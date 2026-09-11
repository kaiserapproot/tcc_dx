@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
goto :n7_07a_main

:audit_runtime_oracle
set "TAG=%~1"
set "SRC=%~2"
set "DIAG=%~3"
set "EXPECT=%~4"
set "EXE=%OUT%\%~n2.exe"
set "LOG=%OUT%\%~n2.log"
set "DIAGLOG=%OUT%\%~n3.log"
del /q "%EXE%" 2>nul
"%TCC%" "%~dp0%SRC%" -o "%EXE%" >"%LOG%" 2>&1
if errorlevel 1 (
  echo %TAG%_COMPILE=FAIL
  echo %TAG%_RUNTIME=SKIP
  echo %TAG%_CTOR_COUNT=NA
  echo %TAG%_DIAGNOSTIC=YES
  echo %TAG%_FAIL_CLOSED=PASS
  echo %TAG%_SILENT_MISCOMPILE=NO
  exit /b 0
)
echo %TAG%_COMPILE=PASS
"%EXE%" >"%OUT%\%~n2_run.log" 2>&1
set "RC=!ERRORLEVEL!"
if "!RC!"=="0" (
  echo %TAG%_RUNTIME=PASS
  echo %TAG%_CTOR_COUNT=!EXPECT!
  echo %TAG%_DIAGNOSTIC=NO
  echo %TAG%_FAIL_CLOSED=NA
  echo %TAG%_SILENT_MISCOMPILE=NO
  exit /b 0
)
echo %TAG%_RUNTIME=FAIL
del /q "%OUT%\%~n3.exe" 2>nul
"%TCC%" "%~dp0%DIAG%" -o "%OUT%\%~n3.exe" >"%DIAGLOG%" 2>&1
set "CTOR=UNKNOWN"
if exist "!OUT!\%~n3.exe" (
  "!OUT!\%~n3.exe" >"!OUT!\%~n3_diag_run.log" 2>&1
  for /f "tokens=2 delims==" %%V in ('findstr /b "CTOR_COUNT=" "!OUT!\%~n3_diag_run.log"') do set "CTOR=%%V"
)
echo %TAG%_CTOR_COUNT=!CTOR!
echo %TAG%_DIAGNOSTIC=NO
echo %TAG%_FAIL_CLOSED=FAIL
echo %TAG%_SILENT_MISCOMPILE=YES
set /a SILENT_MISCOMPILE+=1
set "N7_07A=FAIL"
exit /b 0

:audit_compile_only
set "TAG=%~1"
set "SRC=%~2"
set "LOG=%OUT%\%~n2.log"
set "EXE=%OUT%\%~n2.exe"
del /q "%EXE%" 2>nul
"%TCC%" "%~dp0%SRC%" -o "%EXE%" >"%LOG%" 2>&1
if errorlevel 1 (
  echo %TAG%_COMPILE=FAIL
  echo %TAG%_DIAGNOSTIC=YES
  echo %TAG%_RUNTIME=SKIP
  echo %TAG%_FAIL_CLOSED=PASS
  exit /b 0
)
echo %TAG%_COMPILE=PASS
echo %TAG%_DIAGNOSTIC=NO
"%EXE%" >"%OUT%\%~n2_run.log" 2>&1
set "RC=!ERRORLEVEL!"
echo %TAG%_RUNTIME=!RC!
if not "!RC!"=="0" (
  echo %TAG%_SILENT_DTOR_OMISSION=YES
  echo %TAG%_FAIL_CLOSED=FAIL
  set /a SILENT_MISCOMPILE+=1
  set "N7_07A=FAIL"
) else (
  echo %TAG%_SILENT_DTOR_OMISSION=NO
  echo %TAG%_FAIL_CLOSED=UNEXPECTED_PASS
  set /a SILENT_MISCOMPILE+=1
  set "N7_07A=FAIL"
)
exit /b 0

:audit_supported
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~n2.exe"
"%TCC%" "%~dp0%SRC%" -o "%EXE%" >"%OUT%\%~n2.log" 2>&1
if errorlevel 1 (
  echo %TAG%=COMPILE_FAIL
  set "N7_07A=FAIL"
  exit /b 0
)
"%EXE%" >"%OUT%\%~n2_run.log" 2>&1
if errorlevel 1 (
  echo %TAG%=RUN_FAIL
  set "N7_07A=FAIL"
) else (
  echo %TAG%=PASS
)
exit /b 0

:inv_row
set "NAME=%~1"
set "SRC=%~2"
set "LOG=%OUT%\inv_%NAME%.log"
"%TCC%" "%~dp0%SRC%" -c -o "%OUT%\inv_%NAME%.o" >"%LOG%" 2>&1
if errorlevel 1 (
  findstr /i /c:"error:" "%LOG%" >nul 2>&1
  if errorlevel 1 (
    echo %NAME% EXPECTED=FAIL ACTUAL=CRASH DIAGNOSTIC_PRESENT=UNKNOWN COMPILER_CRASH=YES BAD_CODE_ACCEPTED=NO>> "%OUT%\inventory.txt"
  ) else (
    echo %NAME% EXPECTED=FAIL ACTUAL=FAIL DIAGNOSTIC_PRESENT=YES COMPILER_CRASH=NO BAD_CODE_ACCEPTED=NO>> "%OUT%\inventory.txt"
  )
) else (
  echo %NAME% EXPECTED=FAIL ACTUAL=PASS DIAGNOSTIC_PRESENT=NO COMPILER_CRASH=NO BAD_CODE_ACCEPTED=YES>> "%OUT%\inventory.txt"
  set /a SILENT_MISCOMPILE+=1
  set "N7_07A=FAIL"
)
exit /b 0

:n7_07a_main
set "TCC=..\..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_07a_out"
if not exist "%OUT%" mkdir "%OUT%"
set /a SILENT_MISCOMPILE=0
set "N7_07A=PASS"

echo === N7-07A UNSUPPORTED FORMS FAIL-CLOSED AUDIT ===
echo PHASE=N7-07A
echo TCC=%TCC%
echo MEASUREMENT_ONLY=YES
echo.

call :audit_runtime_oracle TOP_LEVEL_LOCAL_CLASS_ARRAY n7_07a_local_class_array.cpp n7_07a_local_class_array_diag.cpp 4
echo.
call :audit_runtime_oracle TOP_LEVEL_MULTIDIM_CLASS_ARRAY n7_07a_local_class_array_multidim.cpp n7_07a_local_class_array_multidim_diag.cpp 6
echo.
call :audit_compile_only CLASS_MEMBER_ARRAY_NONTRIVIAL_DTOR n7_07a_member_array_dtor.cpp
echo.
call :audit_compile_only TLS_CLASS_ARRAY n7_07a_tls_class_array.cpp
echo.
call :audit_supported SUPPORTED_CONTROLS n7_07a_supported_controls.cpp
echo.

echo === TCC-5 NEGATIVE INVENTORY ===
echo TEST EXPECTED ACTUAL DIAGNOSTIC_PRESENT COMPILER_CRASH BAD_CODE_ACCEPTED> "%OUT%\inventory.txt"
call :inv_row n7_03_const_member_negative ../n7_03_const_member_negative.cpp
call :inv_row n7_03_reference_member_negative ../n7_03_reference_member_negative.cpp
call :inv_row n7_03_unassignable_member_negative ../n7_03_unassignable_member_negative.cpp
call :inv_row n7_03_unassignable_base_negative ../n7_03_unassignable_base_negative.cpp
call :inv_row implicit_assign_const_member ../../negative/implicit_assign_const_member.cpp
call :inv_row implicit_assign_reference_member ../../negative/implicit_assign_reference_member.cpp
call :inv_row no_viable_default_ctor_array ../../negative/no_viable_default_ctor_array.cpp
call :inv_row no_viable_default_ctor_auto ../../negative/no_viable_default_ctor_local.cpp
call :inv_row member_array_dtor_probe ../n7_04a_dtor_probe.cpp
call :inv_row return_array_dtor ../../negative/return_array_dtor.cpp
call :inv_row local_static_array_dtor ../../negative/local_static_array_dtor.cpp
call :inv_row no_default_ctor_member_array ../n7_04_no_default_ctor_neg.cpp
call :inv_row tls_no_default_ctor ../n7_00_storage_tls_no_default.cpp
call :inv_row tls_class_array n7_07a_tls_class_array.cpp
"%TCC%" "%~dp0n7_07a_local_class_array.cpp" -o "%OUT%\inv_top_level_class_array_silent.exe" >"%OUT%\inv_top_level_class_array_silent.log" 2>&1
if errorlevel 1 (
  echo top_level_class_array_silent EXPECTED=FAIL ACTUAL=FAIL DIAGNOSTIC_PRESENT=YES COMPILER_CRASH=NO BAD_CODE_ACCEPTED=NO>> "%OUT%\inventory.txt"
) else (
  "%OUT%\inv_top_level_class_array_silent.exe" >nul 2>&1
  if errorlevel 1 (
    echo top_level_class_array_silent EXPECTED=FAIL ACTUAL=PASS_COMPILE RUN_FAIL DIAGNOSTIC_PRESENT=NO COMPILER_CRASH=NO BAD_CODE_ACCEPTED=YES>> "%OUT%\inventory.txt"
    set /a SILENT_MISCOMPILE+=1
  ) else (
    echo top_level_class_array_silent EXPECTED=FAIL ACTUAL=PASS DIAGNOSTIC_PRESENT=NO COMPILER_CRASH=NO BAD_CODE_ACCEPTED=YES>> "%OUT%\inventory.txt"
  )
)
type "%OUT%\inventory.txt"
echo.

echo === N7-07A SUMMARY ===
echo SILENT_MISCOMPILE_COUNT=!SILENT_MISCOMPILE!
echo N7_07A=!N7_07A!
popd
if /i not "!N7_07A!"=="PASS" exit /b 1
exit /b 0
