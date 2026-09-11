@echo off
setlocal EnableExtensions
pushd "%~dp0"

set TCC=..\..\..\..\tcc.exe
set OUT=n7_05b_out
if not exist "%OUT%" mkdir "%OUT%"

set FAIL=0

call :pos amateras_exact n7_05b_amateras_exact.cpp 0
call :pos tag_function n7_05b_tag_function.cpp 0
call :neg explicit_typedef_function n7_05b_explicit_typedef_function_neg.cpp
call :pos distinct_alias n7_05b_distinct_alias.cpp 0
call :neg alias_function n7_05b_alias_function_neg.cpp
call :pos identical_typedef n7_05b_identical_typedef.cpp 0
call :neg incompatible_typedef n7_05b_incompatible_typedef_neg.cpp
call :pos func_redecl n7_05b_func_redecl.cpp 0
call :neg incompat_func_redecl n7_05b_incompat_func_redecl_neg.cpp
call :pos elaborated_after_function n7_05b_elaborated_after_function.cpp 0
call :neg post_collision_unqual n7_05b_post_collision_unqual_neg.cpp
call :pos header_include n7_05b_header_include.cpp 0
call :pos extern_c n7_05b_extern_c.cpp 0
call :pos c_mode_control n7_05b_c_mode_control.c 0
call :pos forward_tag n7_05b_forward_tag.cpp 0

if %FAIL% NEQ 0 (
  echo N7_05B=FAIL
  popd
  exit /b 1
)

echo AMATERAS_EXACT_REPRO=PASS
echo STRUCT_TAG_FUNCTION_COEXISTENCE=PASS
echo ELABORATED_CLASS_NAME_AFTER_FUNCTION=PASS
echo DISTINCT_TYPEDEF_ALIAS_WITH_TAG_FUNCTION=PASS
echo EXPLICIT_TYPEDEF_FUNCTION_CONFLICT=FAIL_CLOSED
echo TYPEDEF_ALIAS_FUNCTION_CONFLICT=FAIL_CLOSED
echo INCOMPATIBLE_TYPEDEF=FAIL_CLOSED
echo SAME_FUNCTION_REDECLARATION=PASS
echo INCOMPATIBLE_FUNCTION_REDECL=FAIL_CLOSED
echo POST_COLLISION_UNQUALIFIED_TYPE=FAIL_CLOSED
echo HEADER_CLASS_FUNCTION=PASS
echo EXTERN_C_TAG_FUNCTION_COEXISTENCE=PASS
echo C_MODE_REGRESSION=PASS
echo FORWARD_TAG_FUNCTION=PASS
echo BAD_CODE_ACCEPTED=0
echo N7_05B=PASS
popd
exit /b 0

:pos
set NAME=%~1
set SRC=%~2
set EXPECT=%~3
"%TCC%" -I"%CD%" "%SRC%" -o "%OUT%\%NAME%.exe" > "%OUT%\%NAME%.log" 2>&1
if errorlevel 1 (
  echo %NAME%=FAIL compile
  type "%OUT%\%NAME%.log"
  set FAIL=1
  exit /b 0
)
if not "%EXPECT%"=="" (
  "%OUT%\%NAME%.exe" >nul 2>&1
  if errorlevel 1 (
    echo %NAME%=FAIL run exit=%ERRORLEVEL%
    set FAIL=1
    exit /b 0
  )
)
echo %NAME%=PASS
exit /b 0

:neg
set NAME=%~1
set SRC=%~2
"%TCC%" -I"%CD%" "%SRC%" -o "%OUT%\%NAME%.exe" > "%OUT%\%NAME%.log" 2>&1
if not errorlevel 1 (
  echo %NAME%=FAIL expected compile failure
  set FAIL=1
  exit /b 0
)
echo %NAME%=FAIL_CLOSED
exit /b 0
