@echo off
rem SSE / SSE2 intrinsics (dev\include\xmmintrin.h, emmintrin.h):
rem   - simd\simd_values.c, built by TCC as C and as C++, must print exactly
rem     simd\simd_values_expected.txt, which is the same program's output when
rem     built by MSVC (manual\simd_golden_msvc.bat makes it)
rem   - two TUs per language (one through <intrin.h>, one <emmintrin.h> alone)
rem   - <xmmintrin.h> alone
rem   - the x64 / older names (_mm_cvtss_si64x, _mm_stream_si64x, _mm_setl_epi64,
rem     ...) through <intrin.h> alone, C and C++
rem Keep this file CRLF: cmd can misread call/goto labels in LF-only files.
setlocal EnableExtensions EnableDelayedExpansion
goto :main
:build_run_ok
rem %1 = tag, %2 = sources
"%TCC%" %~2 -o "%OUT%\%~1.exe" >"%OUT%\%~1.log" 2>&1
if errorlevel 1 (
  echo %~1=BUILD_FAIL
  type "%OUT%\%~1.log"
  set /a FAILED+=1
  goto :eof
)
"%OUT%\%~1.exe" >nul 2>&1
if errorlevel 1 (
  echo %~1=RUN_FAIL rc=!errorlevel!
  set /a FAILED+=1
  goto :eof
)
echo %~1=PASS
goto :eof
:values_match
rem %1 = tag, %2 = source; the output must be byte for byte the expected file
"%TCC%" %2 -o "%OUT%\%~1.exe" >"%OUT%\%~1.log" 2>&1
if errorlevel 1 (
  echo %~1=BUILD_FAIL
  type "%OUT%\%~1.log"
  set /a FAILED+=1
  goto :eof
)
"%OUT%\%~1.exe" >"%OUT%\%~1.txt"
if errorlevel 1 (
  echo %~1=RUN_FAIL
  set /a FAILED+=1
  goto :eof
)
fc /b simd_values_expected.txt "%OUT%\%~1.txt" >"%OUT%\%~1.fc" 2>&1
if errorlevel 1 (
  echo %~1=DIFFERS_FROM_MSVC see %OUT%\%~1.txt
  set /a FAILED+=1
  goto :eof
)
echo %~1=PASS
goto :eof
:main
pushd "%~dp0simd"
set "TCC=..\..\..\tcc.exe"
if not "%TCC_EXE%"=="" set "TCC=%TCC_EXE%"
set "OUT=%TEMP%\tcc_simd_gate"
if not exist "%OUT%" mkdir "%OUT%"
set /a FAILED=0
echo === SSE / SSE2 intrinsics gate ===
call :values_match SIMD_VALUES_C simd_values.c
call :values_match SIMD_VALUES_CPP simd_values_cpp.cpp
call :build_run_ok SIMD_TWO_TU_C "simd_tu1.c simd_tu2.c"
call :build_run_ok SIMD_TWO_TU_CPP "simd_tu1_cpp.cpp simd_tu2_cpp.cpp"
call :build_run_ok SIMD_XMMINTRIN_ONLY "simd_xmm_only.c"
call :build_run_ok SIMD_INTRIN_H_X64_NAMES_C "simd_intrin_names.c"
call :build_run_ok SIMD_INTRIN_H_X64_NAMES_CPP "simd_intrin_names_cpp.cpp"
if not "!FAILED!"=="0" (
  echo SIMD_GATE=FAIL failed=!FAILED!
  popd
  exit /b 1
)
echo SIMD_GATE=PASS
popd
exit /b 0