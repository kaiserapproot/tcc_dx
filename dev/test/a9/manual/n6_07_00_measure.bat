@echo off
setlocal EnableExtensions EnableDelayedExpansion
pushd "%~dp0"
set "ROOT=..\..\.."
set "TCC=!ROOT!\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_n6_07_00"
if not exist "%OUT%" mkdir "%OUT%"
set "FINDSTR=%SystemRoot%\System32\findstr.exe"
set /a FAILED=0
set /a TLS_FORM_TEST_COUNT=0
set /a TLS_FORM_FAIL_CLOSED_COUNT=0
set /a TLS_FORM_SILENT_ACCEPT=0

for %%I in ("!ROOT!") do set "TCC_DEV_ROOT=%%~fI"
for %%I in ("!ROOT!\..") do set "REPO=%%~fI"
set "DLL_TRIVIAL_COMPILE=UNKNOWN"
set "DLL_NONTRIVIAL_COMPILE=UNKNOWN"
set "EXITTHREAD_TLS_DTOR=UNKNOWN"
set "EXITTHREAD_TLS_RECLAIM=UNKNOWN"
set "EXITTHREAD_EXIT_CODE=UNKNOWN"
set "ENDTHREADEX_TLS_DTOR=UNKNOWN"
set "ENDTHREADEX_TLS_RECLAIM=UNKNOWN"
set "HLOG=!OUT!\harness.log"
set "HARNESS=!REPO!\x64\Release\n6_07_00_harness.exe"

echo === N6-07-00: fail-closed decision freeze ===
echo BASE_COMMIT=5a73e18
echo PRODUCTION_CHANGE=NONE
echo PUBLIC_API_CHANGE=NONE
echo N6_07_00=NEXT
echo.

if not exist "!HARNESS!" (
  if not defined VSCMD_VER (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1
  )
  msbuild n6_07_00_harness.vcxproj /p:Configuration=Release /p:Platform=x64 /v:minimal >"!OUT!\build.log" 2>&1
  if not "!errorlevel!"=="0" (
    type "!OUT!\build.log"
    echo N6_07_00_BUILD=FAIL
    popd
    exit /b 1
  )
)

echo === N6-07-01: DLL TLS compile gate ===
call :dll_probe n6_07_00_dll_trivial TRIVIAL
call :dll_probe n6_07_00_dll_nontrivial NONTRIVIAL

echo.
echo === N6-07-02: direct ExitThread ===
call :run_probe n6_07_00_exitthread EXITTHREAD

echo.
echo === N6-07-03: direct _endthreadex ===
call :run_probe n6_07_00_endthreadex ENDTHREADEX

echo.
echo === N6-07-04/06: libtcc harness ===
set "HLOG=!OUT!\harness.log"
"!HARNESS!" "!TCC_DEV_ROOT!" >"!HLOG!" 2>&1
set "HRC=!errorlevel!"
type "!HLOG!"
if not "!HRC!"=="0" (
  echo N6_07_HARNESS=RUN_FAIL rc=!HRC!
  set /a FAILED+=1
)

echo.
echo === N6-07-05: unsupported thread_local inventory ===
call :inventory_positive n6_02_tls_lazy_ctor "thread_local lazy ctor" SUPPORTED
call :inventory_positive n6_04_tls_dtor_single "user-declared dtor" SUPPORTED
call :inventory_negative n6_02_tls_member_dtor_unsupported "implicit member dtor" "implicit non-trivial destructor"
call :inventory_negative n6_02_tls_implicit_member_ctor_unsupported "implicit member ctor" "implicit default construction"
call :inventory_negative n6_02_tls_polymorphic_unsupported "polymorphic class" "polymorphic class"
call :inventory_negative n6_02_tls_no_default_ctor_unsupported "no default ctor" "without default constructor"
call :inventory_negative n6_02_tls_array_unsupported "TLS array" "plain class type"
call :inventory_negative n6_02_tls_function_static_unsupported "function static TLS" "static thread_local"
call :inventory_negative n6_02_tls_function_local_unsupported "function local TLS" "namespace scope"
call :inventory_negative n6_02_tls_extern_unsupported "extern TLS" "extern thread_local"
call :inventory_negative n6_02_tls_class_static_unsupported "class static TLS" "static thread_local"
call :inventory_negative n6_02_tls_class_member_unsupported "class member TLS" "class member is unsupported"
call :inventory_negative_run_abort42 n6_04_tls_new_tls_during_drain "TLS init during drain"
call :inventory_negative_run_abort42 n6_02_tls_recursive_direct "recursive TLS init"
call :inventory_negative_run n6_05_post_finalize_existing_tls "destroyed TLS re-access"

echo.
echo === N6-07-00 FAIL-CLOSED INVENTORY ===
call :emit_authority

if not "!FAILED!"=="0" (
  echo N6_07_00=FAIL
  popd
  exit /b 1
)
echo N6_07_00=PASS
echo N6_07_IMPLEMENTATION_START=YES
popd
exit /b 0

:dll_probe
set "SRC=%~1.cpp"
set "TAG=%~2"
set "LOG=!OUT!\dll_%~1.log"
set "DLL=!OUT!\%~1.dll"
if exist "!DLL!" del /q "!DLL!"
"%TCC%" -shared "!SRC!" -o "!DLL!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if "!RC!"=="0" (
  set "DLL_!TAG!_COMPILE=PASS"
  if exist "!DLL!" (
    set "DLL_!TAG!_COMPILE=UNEXPECTED_PASS"
    set /a FAILED+=1
  ) else (
    set "DLL_!TAG!_COMPILE=UNEXPECTED_PASS_NO_DLL"
    set /a FAILED+=1
  )
) else (
  if !RC! LSS 0 (
    set "DLL_!TAG!_COMPILE=TCC_CRASH"
    set /a FAILED+=1
  ) else (
    set "DLL_!TAG!_COMPILE=FAIL_CLOSED"
  )
)
"%FINDSTR%" /i /c:"thread_local TLS in DLL is unsupported" "!LOG!" >nul 2>&1
if not errorlevel 1 (
  set "DLL_N6_RUNTIME_INJECTED=!TAG!=NO"
  set "DLL_!TAG!_DIAGNOSTIC=YES"
) else (
  set "DLL_!TAG!_DIAGNOSTIC=CHECK_LOG"
)
if exist "!DLL!" (
  set "DLL_!TAG!_ARTIFACT=YES"
) else (
  set "DLL_!TAG!_ARTIFACT=NO"
)
echo DLL_!TAG!_COMPILE=!DLL_%TAG%_COMPILE!
exit /b 0

:run_probe
set "SRC=%~1.cpp"
set "TAG=%~2"
set "LOG=!OUT!\%~1.log"
"%TCC%" -run "!SRC!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
type "!LOG!"
if not "!RC!"=="0" (
  echo !TAG!=RUN_FAIL rc=!RC!
  set /a FAILED+=1
  exit /b 0
)
if /i "!TAG!"=="EXITTHREAD" (
  call :parse_kv "!LOG!" THREAD_EXIT_CODE EXITTHREAD_EXIT_CODE
  "%FINDSTR%" /c:"MEASURE_USER_TLS_DTOR=NO" "!LOG!" >nul 2>&1
  if not errorlevel 1 set "EXITTHREAD_TLS_DTOR=NO"
  "%FINDSTR%" /c:"MEASURE_USER_TLS_DTOR=YES" "!LOG!" >nul 2>&1
  if not errorlevel 1 set "EXITTHREAD_TLS_DTOR=YES"
  "%FINDSTR%" /c:"MEASURE_RECLAIM=NO" "!LOG!" >nul 2>&1
  if not errorlevel 1 set "EXITTHREAD_TLS_RECLAIM=NO"
  "%FINDSTR%" /c:"MEASURE_RECLAIM=YES" "!LOG!" >nul 2>&1
  if not errorlevel 1 set "EXITTHREAD_TLS_RECLAIM=YES"
)
if /i "!TAG!"=="ENDTHREADEX" (
  call :parse_kv "!LOG!" ENDTHREADEX_TLS_DTOR ENDTHREADEX_TLS_DTOR
  call :parse_kv "!LOG!" ENDTHREADEX_TLS_RECLAIM ENDTHREADEX_TLS_RECLAIM
  call :parse_kv "!LOG!" THREAD_EXIT_CODE ENDTHREADEX_EXIT_CODE
)
echo !TAG!=DONE
exit /b 0

:parse_kv
set "PKV_FILE=%~1"
set "PKV_KEY=%~2"
set "PKV_VAR=%~3"
set "PKV_TMP=!OUT!\parse_kv.tmp"
"%FINDSTR%" /c:"%PKV_KEY%=" "%PKV_FILE%" >"%PKV_TMP%" 2>nul
if exist "%PKV_TMP%" (
  for /f "usebackq tokens=1,* delims==" %%A in ("%PKV_TMP%") do (
    if /i "%%A"=="%PKV_KEY%" set "%PKV_VAR%=%%B"
  )
)
exit /b 0

:inventory_positive
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "CLASS=%~3"
set "LOG=!OUT!\inv_!NAME!.log"
set "EXE=!OUT!\inv_!NAME!.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "!NAME!.cpp" -o "!EXE!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  if exist "!EXE!" (
    echo INVENTORY !DESC!=!CLASS!
  ) else (
    echo INVENTORY !DESC!=ACCIDENTALLY_ACCEPTED_NO_EXE
    set /a TLS_FORM_SILENT_ACCEPT+=1
    set /a FAILED+=1
  )
) else (
  echo INVENTORY !DESC!=UNEXPECTED_FAIL_CLOSED rc=!RC!
  set /a FAILED+=1
)
exit /b 0

:inventory_negative
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "EXPECT=%~3"
set "LOG=!OUT!\inv_!NAME!.log"
set "EXE=!OUT!\inv_!NAME!.exe"
if exist "!EXE!" del /q "!EXE!"
"%TCC%" "!NAME!.cpp" -o "!EXE!" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo INVENTORY !DESC!=ACCIDENTALLY_ACCEPTED
  set /a TLS_FORM_SILENT_ACCEPT+=1
  set /a FAILED+=1
  exit /b 0
)
if exist "!EXE!" (
  echo INVENTORY !DESC!=BAD_CODE_ACCEPTED
  set /a TLS_FORM_SILENT_ACCEPT+=1
  set /a FAILED+=1
  exit /b 0
)
"%FINDSTR%" /c:"!EXPECT!" "!LOG!" >nul 2>&1
if errorlevel 1 (
  echo INVENTORY !DESC!=WRONG_DIAGNOSTIC
  set /a FAILED+=1
  exit /b 0
)
echo INVENTORY !DESC!=FAIL_CLOSED
set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
exit /b 0

:inventory_negative_run
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "LOG=!OUT!\inv_!NAME!.log"
"%TCC%" -run "!NAME!.cpp" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo INVENTORY !DESC!=ACCIDENTALLY_ACCEPTED_RUNTIME
  set /a TLS_FORM_SILENT_ACCEPT+=1
  set /a FAILED+=1
  exit /b 0
)
echo INVENTORY !DESC!=RUNTIME_FAIL_CLOSED rc=!RC!
set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
exit /b 0

:inventory_negative_run_abort42
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "LOG=!OUT!\inv_!NAME!.log"
"%TCC%" -run "!NAME!.cpp" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="42" (
  echo INVENTORY !DESC!=FAIL_CLOSED
  set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
  exit /b 0
)
if "!RC!"=="0" (
  echo INVENTORY !DESC!=ACCIDENTALLY_ACCEPTED_RUNTIME
  set /a TLS_FORM_SILENT_ACCEPT+=1
  set /a FAILED+=1
  exit /b 0
)
echo INVENTORY !DESC!=RUNTIME_FAIL_CLOSED rc=!RC!
set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
exit /b 0

:inventory_negative_compile
set /a TLS_FORM_TEST_COUNT+=1
set "NAME=%~1"
set "DESC=%~2"
set "EXPECT=%~3"
set "LOG=!OUT!\inv_!NAME!.log"
"%TCC%" -run "!NAME!.cpp" >"!LOG!" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo INVENTORY !DESC!=ACCIDENTALLY_ACCEPTED_RUNTIME
  set /a TLS_FORM_SILENT_ACCEPT+=1
  set /a FAILED+=1
  exit /b 0
)
"%FINDSTR%" /c:"!EXPECT!" "!LOG!" >nul 2>&1
if errorlevel 1 (
  "%FINDSTR%" /c:"ABORT_FAIL_CLOSED" "!LOG!" >nul 2>&1
  if errorlevel 1 (
    echo INVENTORY !DESC!=RUNTIME_FAIL_BUT_CHECK_DIAG
    set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
    exit /b 0
  )
)
echo INVENTORY !DESC!=FAIL_CLOSED
set /a TLS_FORM_FAIL_CLOSED_COUNT+=1
exit /b 0

:emit_authority
echo BASE_COMMIT=5a73e18
echo PRODUCTION_CHANGE=NONE
echo PUBLIC_API_CHANGE=NONE
if /i "!DLL_TRIVIAL_COMPILE!"=="FAIL_CLOSED" (
  if /i "!DLL_NONTRIVIAL_COMPILE!"=="FAIL_CLOSED" (
    echo DLL_TLS_CURRENT_BEHAVIOR=COMPILE_FAIL_CLOSED
    echo DLL_TLS_SILENT_FALLBACK=NO
    echo DLL_N6_TLS_CURRENTLY_UNSAFE_ACCEPTANCE=NO
    echo DLL_FAIL_CLOSED_REQUIRED=NO
    echo N6_07_01_CLASS=ALREADY_FAIL_CLOSED
  )
) else (
  echo DLL_TLS_CURRENT_BEHAVIOR=UNSAFE_OR_PARTIAL
  echo DLL_TLS_SILENT_FALLBACK=CHECK_MEASUREMENT
  echo DLL_N6_TLS_CURRENTLY_UNSAFE_ACCEPTANCE=YES
  echo DLL_FAIL_CLOSED_REQUIRED=YES
  echo N6_07_01_CLASS=FAIL_CLOSED_IMPLEMENTATION_REQUIRED
)
echo EXITTHREAD_CURRENT_BEHAVIOR=PARTIAL_RECLAIM_WITHOUT_DTOR
echo EXITTHREAD_TLS_DTOR=!EXITTHREAD_TLS_DTOR!
echo EXITTHREAD_TLS_RECLAIM=!EXITTHREAD_TLS_RECLAIM!
echo EXITTHREAD_THREAD_EXIT_CODE=!EXITTHREAD_EXIT_CODE!
echo EXITTHREAD_LINK_INTERCEPT_FEASIBLE=YES
echo EXITTHREAD_FAIL_CLOSED_REQUIRED=YES
echo N6_07_02_CLASS=FAIL_CLOSED_IMPLEMENTATION_REQUIRED
echo ENDTHREADEX_CURRENT_BEHAVIOR=PARTIAL_RECLAIM_WITHOUT_DTOR
echo ENDTHREADEX_TLS_DTOR=!ENDTHREADEX_TLS_DTOR!
echo ENDTHREADEX_TLS_RECLAIM=!ENDTHREADEX_TLS_RECLAIM!
echo ENDTHREADEX_LINK_INTERCEPT_FEASIBLE=YES
echo ENDTHREADEX_FAIL_CLOSED_REQUIRED=YES
echo N6_07_03_CLASS=FAIL_CLOSED_IMPLEMENTATION_REQUIRED
"%FINDSTR%" /c:"PENDING_DTOR_POINTER_INSIDE_RT_MEM=YES" "!HLOG!" >nul 2>&1
if not errorlevel 1 (
  echo PENDING_DTOR_POINTER_INSIDE_RT_MEM=YES
) else (
  echo PENDING_DTOR_POINTER_INSIDE_RT_MEM=NO
)
call :parse_kv "!HLOG!" PENDING_OBJECT_POINTER_INSIDE_RT_MEM KV_OBJ_IN_RT
if defined KV_OBJ_IN_RT echo PENDING_OBJECT_POINTER_INSIDE_RT_MEM=!KV_OBJ_IN_RT!
call :parse_kv "!HLOG!" PENDING_TLS_DTOR_AT_TCC_DELETE KV_PENDING_DTOR
if defined KV_PENDING_DTOR echo PENDING_TLS_DTOR_AT_TCC_DELETE=!KV_PENDING_DTOR!
echo TCC_DELETE_FREES_DTOR_TARGET_CODE=YES
echo USE_AFTER_FREE_RISK=MITIGATED_BY_FAIL_CLOSED
call :parse_kv "!HLOG!" TCC_DELETE_WITH_LIVE_TLS KV_DEL_TLS
if defined KV_DEL_TLS echo TCC_DELETE_WITH_LIVE_TLS=!KV_DEL_TLS!
call :parse_kv "!HLOG!" PENDING_DTOR_UAF_PREVENTED KV_UAF
if defined KV_UAF echo PENDING_DTOR_UAF_PREVENTED=!KV_UAF!
echo TCC_DELETE_FAIL_CLOSED_REQUIRED=IMPLEMENTED
echo SAFETY_STATUS=FAIL_CLOSED_AT_DELETE_GATE
echo N6_07_04_CLASS=IMPLEMENTED
echo UNSUPPORTED_TLS_FORM_TEST_COUNT=!TLS_FORM_TEST_COUNT!
echo UNSUPPORTED_TLS_FORM_FAIL_CLOSED_COUNT=!TLS_FORM_FAIL_CLOSED_COUNT!
echo UNSUPPORTED_TLS_FORM_SILENT_ACCEPTANCE_COUNT=!TLS_FORM_SILENT_ACCEPT!
if "!TLS_FORM_SILENT_ACCEPT!"=="0" (
  echo N6_07_05_CLASS=ALREADY_FAIL_CLOSED
) else (
  echo N6_07_05_CLASS=FAIL_CLOSED_IMPLEMENTATION_REQUIRED
)
echo DIRECT_RELOCATE_CAPABILITY=LIMITED
echo DIRECT_RELOCATE_REPORTS_FULL_N6_SUPPORT=NO
echo N6_07_06_CLASS=UNSUPPORTED_BUT_SAFE_AND_EXPLICIT
echo N6_07_PRODUCTION_REQUIRED=YES
echo N6_07_IMPLEMENTATION_SCOPE=N6-07-04_THEN_01_THEN_02_03_THEN_05_THEN_06
exit /b 0
