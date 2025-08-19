@echo off
REM openglビルド用バッチ
setlocal

REM TCC_PATH環境変数が未定義ならtcc_set.batを実行
if not defined TCC_PATH (
    call ..\..\dev\tcc_set.bat
)

REM openglのビルド
"%~dp0..\..\dev\tcc.exe" main_gl.c -lopengl32 -lglut32 -lgdi32 -luser32 -lkernel32 -lgdi32 -lmsvcrt

endlocal
