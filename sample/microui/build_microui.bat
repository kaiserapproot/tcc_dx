@echo off
REM microuiビルド用バッチ
setlocal

REM tcc_set.batが実行済みか判定（環境変数TCC_PATHを仮定）
if not defined TCC_PATH (
    call ..\..\dev\tcc_set.bat
)

REM microuiのビルド
"%~dp0..\..\dev\tcc.exe" microuigl.c microui.c renderer.c -lopengl32 -lglut32 -lgdi32 -luser32 -lkernel32 -lgdi32 -lmsvcrt -ld3d9 -g -o microuigl.exe

endlocal
