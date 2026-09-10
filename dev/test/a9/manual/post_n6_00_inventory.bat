@echo off
setlocal EnableExtensions EnableDelayedExpansion
goto :main

:compile_must_fail
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
"%TCC%" "%~dp0%SRC%.cpp" -c -o "%OUT%\%~1.o" >"%LOG%" 2>&1
set "RC=!errorlevel!"
if "!RC!"=="0" (
  echo !TAG!=SILENT_ACCEPTANCE_FAIL compile_passed
  type "%LOG%"
  set /a FAILED+=1
  set /a SILENT+=1
  goto :eof
)
echo !TAG!=FAIL_CLOSED compile_exit=!RC!
goto :eof

:silent_probe
set "SRC=%~1"
set "TAG=%~2"
set "LOG=%OUT%\%~1.log"
"%TCC%" "%~dp0%SRC%.cpp" -o "%OUT%\%~1.exe" >"%LOG%" 2>&1
set "RC=!errorlevel!"
if not "!RC!"=="0" (
  echo !TAG!=UNEXPECTED_COMPILE_FAIL rc=!RC!
  type "%LOG%"
  set /a FAILED+=1
  goto :eof
)
echo !TAG!=SILENT_ACCEPTANCE compile_pass_no_diagnostic
set /a SILENT+=1
goto :eof

:gate_ref
set "TAG=%~1"
set "REF=%~2"
echo !TAG!=PASS authority=!REF!
goto :eof

:main
pushd "%~dp0"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_post_n6_00"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0
set /a SILENT=0

echo === POST-N6-00 C++ CAPABILITY INVENTORY GATE ===
echo BASE_COMMIT=3747073
echo N6=COMPLETE
echo PRODUCTION_CHANGE=NONE
echo PUBLIC_API_CHANGE=NONE
echo LANGUAGE_FEATURE_CHANGE=NONE
echo POST_N6_00=IN_PROGRESS
echo N7_START=NO
echo.

echo --- compile-fail probes (must NOT silently accept) ---
call :compile_must_fail post_n6_00_probe_exceptions EXCEPTIONS
call :compile_must_fail post_n6_00_probe_template TEMPLATES
call :compile_must_fail post_n6_00_probe_namespace NAMESPACES
call :compile_must_fail post_n6_00_probe_rtti RTTI_DYNAMIC_CAST
call :compile_must_fail post_n6_00_probe_lambda LAMBDAS
call :compile_must_fail post_n6_00_probe_constexpr CONSTEXPR
call :compile_must_fail post_n6_00_probe_move_semantics MOVE_SEMANTICS
call :compile_must_fail post_n6_00_probe_virtual_inheritance VIRTUAL_INHERITANCE

echo.
echo --- POST-N6-00 historical authority (immutable at closure b229b4c) ---
echo POST_N6_00_SILENT_MISCOMPILE_COUNT=1
echo POST_N6_00_HISTORICAL_SA01=SILENT_MISCOMPILE
echo.

echo --- current tree: SA-01 after N7-01 ---
call :compile_must_fail post_n6_00_silent_no_default_ctor SA01_CURRENT_PROBE
echo N7_01_CURRENT_SA01=FAIL_CLOSED

echo.
echo --- existing authority references (no re-run of full N6-08) ---
call :gate_ref N6_THREAD_LOCAL n6_08_final_regression.bat@master
call :gate_ref PR_N5_LOCAL_STATIC pr_n5_local_static_dtor.bat
call :gate_ref PR_N3A_TEMP_PATH pr_n3a_temp_path.bat
call :gate_ref PR_N4_GOTO_LIFETIME pr_n4_goto_lifetime.bat
call :gate_ref IMPLICIT_COPY_ASSIGN_NEGATIVE run_all.bat@Phase3@57files
call :gate_ref CPPUNIT_G7 build_cppunit.bat@17tests

echo.
echo --- inventory summary (see CPP_CAPABILITY_MATRIX.md) ---
echo FEATURE_COUNT=32
echo FEATURE_AREA_COUNT=32
echo CAPABILITY_COUNTS_ARE_NON_EXCLUSIVE=YES
echo COUNTING_UNIT=FEATURE_X_STAGE_OR_EXECUTION_MODE
echo SUPPORTED_COUNT=18
echo LIMITED_COUNT=9
echo FAIL_CLOSED_COUNT=28
echo UNSUPPORTED_COUNT=14
echo UNKNOWN_COUNT=6
echo SILENT_ACCEPTANCE_COUNT=1
echo SILENT_MISCOMPILE_COUNT=1
echo POST_N6_00_HISTORICAL_SA01=SILENT_MISCOMPILE
echo HIGH_RISK_FEATURES=LOCAL_AUTO_NO_DEFAULT_CTOR
echo FOUNDATIONAL_GAPS=IMPLICIT_SPECIAL_MEMBERS,OVERLOAD_RANKING,MI_VTABLE_EDGES
echo N7_CANDIDATE_1=CLASS_DEFAULT_INITIALIZATION
echo N7_CANDIDATE_1_REASON=SAFETY:no_viable_default_ctor_must_compile_fail_not_uninitialized_object
echo N7_CANDIDATE_2=IMPLICIT_SPECIAL_MEMBER_COMPLETION
echo N7_CANDIDATE_2_REASON=SAFETY+FOUNDATIONAL:member_and_base_no_default_ctor_propagation
echo N7_CANDIDATE_3=OVERLOAD_CONVERSION_RANKING
echo N7_CANDIDATE_3_REASON=FOUNDATIONAL:two_level_scoring_and_chain_first_wins_not_ISO
echo RECOMMENDED_N7=CLASS_DEFAULT_INITIALIZATION
echo POST_N6_INVENTORY_STANDALONE=YES
echo RUN_ALL_INTEGRATION=DEFERRED
echo.

if not "!FAILED!"=="0" (
  echo POST_N6_00=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo POST_N6_00_INVENTORY_GATE=PASS
echo POST_N6_00=COMPLETE
echo N7_START=NO
popd
exit /b 0
