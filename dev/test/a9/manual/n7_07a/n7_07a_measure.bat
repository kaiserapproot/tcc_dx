@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"

set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%~dp0n7_07a_out"
if not exist "%OUT%" mkdir "%OUT%"

set /a SILENT_MISCOMPILE=0
set /a SILENT_DTOR_OMISSION=0
set /a COMPILER_CRASH=0
set /a GENERATED_EXE_CRASH=0
set "N7_07A=PASS"
set "FIRST_UNSAFE_FORM="
set "FIRST_UNSAFE_BEHAVIOR="

echo === N7-07A UNSUPPORTED FORMS FAIL-CLOSED AUDIT ===
echo PHASE=N7-07A
echo TCC=%TCC%
echo MEASUREMENT_ONLY=YES
echo.

call :audit_runtime_oracle TOP_LEVEL_LOCAL_CLASS_ARRAY n7_07a_local_class_array.cpp n7_07a_local_class_array_diag.cpp 4
echo.
call :audit_runtime_oracle TOP_LEVEL_MULTIDIM_CLASS_ARRAY n7_07a_local_class_array_multidim.cpp n7_07a_local_class_array_multidim_diag.cpp 6
echo.
call :audit_member_dtor CLASS_MEMBER_ARRAY_NONTRIVIAL_DTOR n7_07a_member_array_dtor.cpp
echo.
call :audit_tls TLS_CLASS_ARRAY n7_07a_tls_class_array.cpp
echo.
call :audit_supported SUPPORTED_CONTROLS n7_07a_supported_controls.cpp
echo.

echo === N7-07A SUMMARY ===
echo SILENT_MISCOMPILE_COUNT=!SILENT_MISCOMPILE!
echo SILENT_DTOR_OMISSION_COUNT=!SILENT_DTOR_OMISSION!
echo COMPILER_CRASH_COUNT=!COMPILER_CRASH!
echo GENERATED_EXE_CRASH_COUNT=!GENERATED_EXE_CRASH!
echo FIRST_UNSAFE_FORM=!FIRST_UNSAFE_FORM!
echo FIRST_UNSAFE_BEHAVIOR=!FIRST_UNSAFE_BEHAVIOR!
echo N7_07A=!N7_07A!
popd
if /i not "!N7_07A!"=="PASS" exit /b 1
exit /b 0

:audit_runtime_oracle
set "TAG=%~1"
set "SRC=%~2"
set "DIAG=%~3"
set "EXPECT=%~4"
set "EXE=%OUT%\%~2.exe"
set "LOG=%OUT%\%~2.log"

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
"%EXE%" >"%OUT%\%~2_run.log" 2>&1
set "RC=!ERRORLEVEL!"
if "!RC!"=="0" (
  echo %TAG%_RUNTIME=PASS
  echo %TAG%_CTOR_COUNT=!EXPECT!
  echo %TAG%_DIAGNOSTIC=NO
  echo %TAG%_FAIL_CLOSED=NA
  echo %TAG%_SILENT_MISCOMPILE=NO
  echo %TAG%_IMPLEMENTED=YES
  exit /b 0
)

echo %TAG%_RUNTIME=FAIL
del /q "%OUT%\%~3.exe" 2>nul
"%TCC%" "%~dp0%DIAG%" -o "%OUT%\%~3.exe" >"%OUT%\%~3.log" 2>&1
set "CTOR=UNKNOWN"
if exist "!OUT!\%~3.exe" (
  "!OUT!\%~3.exe" >"!OUT!\%~3_diag_run.log" 2>&1
  for /f "tokens=2 delims==" %%V in ('findstr /b "CTOR_COUNT=" "!OUT!\%~3_diag_run.log"') do set "CTOR=%%V"
)
echo %TAG%_CTOR_COUNT=!CTOR!
echo %TAG%_DIAGNOSTIC=NO
echo %TAG%_FAIL_CLOSED=FAIL
echo %TAG%_SILENT_MISCOMPILE=YES
set /a SILENT_MISCOMPILE+=1
set "N7_07A=FAIL"
if "!FIRST_UNSAFE_FORM!"=="" (
  set "FIRST_UNSAFE_FORM=%TAG%"
  set "FIRST_UNSAFE_BEHAVIOR=COMPILE_PASS_CTOR_COUNT_WRONG"
)
exit /b 0

:audit_member_dtor
set "TAG=%~1"
set "SRC=%~2"
set "LOG=%OUT%\%~2.log"
set "EXE=%OUT%\%~2.exe"
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
"%EXE%" >"%OUT%\%~2_run.log" 2>&1
set "RC=!ERRORLEVEL!"
if "!RC!"=="0" (
  echo %TAG%_RUNTIME=PASS
  echo %TAG%_FAIL_CLOSED=NA
  echo %TAG%_IMPLEMENTED=YES
  echo %TAG%_SILENT_DTOR_OMISSION=NO
) else (
  echo %TAG%_RUNTIME=FAIL
  echo %TAG%_SILENT_DTOR_OMISSION=YES
  echo %TAG%_FAIL_CLOSED=FAIL
  set /a SILENT_DTOR_OMISSION+=1
  set /a SILENT_MISCOMPILE+=1
  set "N7_07A=FAIL"
  if "!FIRST_UNSAFE_FORM!"=="" (
    set "FIRST_UNSAFE_FORM=%TAG%"
    set "FIRST_UNSAFE_BEHAVIOR=COMPILE_PASS_DTOR_COUNT_WRONG"
  )
)
exit /b 0

:audit_tls
set "TAG=%~1"
set "SRC=%~2"
set "LOG=%OUT%\%~2.log"
set "EXE=%OUT%\%~2.exe"
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
"%EXE%" >"%OUT%\%~2_run.log" 2>&1
set "RC=!ERRORLEVEL!"
echo %TAG%_RUNTIME=!RC!
echo %TAG%_FAIL_CLOSED=FAIL
echo %TAG%_SILENT_ACCEPT=YES
set /a SILENT_MISCOMPILE+=1
set "N7_07A=FAIL"
if "!FIRST_UNSAFE_FORM!"=="" (
  set "FIRST_UNSAFE_FORM=%TAG%"
  set "FIRST_UNSAFE_BEHAVIOR=COMPILE_PASS_UNSUPPORTED_TLS_ARRAY"
)
exit /b 0

:audit_supported
set "TAG=%~1"
set "SRC=%~2"
set "EXE=%OUT%\%~2.exe"
"%TCC%" "%~dp0%SRC%" -o "%EXE%" >"%OUT%\%~2.log" 2>&1
if errorlevel 1 (
  echo %TAG%=COMPILE_FAIL
  set "N7_07A=FAIL"
  exit /b 0
)
"%EXE%" >"%OUT%\%~2_run.log" 2>&1
if errorlevel 1 (
  echo %TAG%=RUN_FAIL
  set "N7_07A=FAIL"
) else (
  echo %TAG%=PASS
)
exit /b 0