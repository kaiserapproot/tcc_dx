@echo off
REM dx9_shaderビルド用バッチ
setlocal

REM TCC_PATH環境変数が未定義ならtcc_set.batを実行
if not defined TCC_PATH (
    call ..\..\dev\tcc_set.bat
)

REM dx9_shaderのビルド
"%~dp0..\..\dev\tcc.exe" tcc_dx9_sheder.c -lgdi32 -luser32 -lkernel32 -lgdi32 -lmsvcrt -ld3d9 -ld3dcompiler_47

endlocal
