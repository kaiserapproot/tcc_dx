@echo off
REM Directx9ビルド用バッチ
setlocal

REM TCC_PATH環境変数が未定義ならtcc_set.batを実行
if not defined TCC_PATH (
    call ..\..\dev\tcc_set.bat
)

REM Directx9のビルド
"%~dp0..\..\dev\tcc.exe" dock_win.c -lgdi32 -luser32 -lkernel32 -lgdi32 -lmsvcrt -ld3d9

endlocal
