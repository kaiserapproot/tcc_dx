## サンプル一覧と内容

### sample/DirectX11
- `tcc_dx11_mina.c`: DirectX11を用いた基本的な描画サンプル。D3D11の定数や構造体を自前で定義し、TCC環境で動作するよう工夫されています。

### sample/Directx9
- `dock_win.c`: DirectX9とWindows APIを使ったウィジェット・ビューポート管理のサンプル。複数ウィンドウやドラッグ操作などUI要素の実装例。

### sample/dx9_shader
- `tcc_dx9_sheder.c`: DirectX9のシェーダー（頂点シェーダ）をTCCで動作させるサンプル。D3D9とD3DCompilerを利用し、シェーダーコードの埋め込みもあり。

### sample/ide
- `jpedit.c`: 日本語IME対応エディタのサンプル。Nuklear D3D9を用いたGUI、IME入力、コンソール連携など多機能なエディタ実装例。

### sample/Introduction
- `main.c`: ポインタや構造体の使い方を日本語コメントで解説した教育用サンプル。

### sample/microui
- `microui.c`: microuiライブラリの実装。GUI部品の描画やイベント管理を行う軽量UIライブラリ。

### sample/microui_dx_ime
- `main.c`: microuiとDirectX9、IME対応を組み合わせたサンプル。日本語入力やフォント描画、UI操作例。

### sample/nuklear
- `nuklear.c`: NuklearライブラリのOpenGL版サンプル。UI部品の描画やイベント処理、GL2実装例。

### sample/nuklear_dx9
- `main.c`: NuklearライブラリのDirectX9版サンプル。ワイド文字対応やTCC環境向けの互換ラッパーも含む。

### sample/nuklear_edit
- `jp_main.c`: Nuklear D3D9を用いた日本語IME対応エディタのサンプル。DirectX9とIME連携、UI部品の拡張例。

### sample/opengl
- `main_gl.c`: OpenGLを用いた基本的なウィンドウ・描画サンプル。GL/GLUを使った描画やウィンドウ管理。

### sample/rc
- `rc2obj.c`: リソースコンパイラ（.rc→.obj変換）のサンプル。RT_ICON/RT_GROUP_ICON対応、TCCでビルド可能。
# TCC 日本語化・DirectX対応・リソースコンパイラ構築手順

## 概要
このワークスペースは、Tiny C Compiler（TCC）を日本語化し、DirectXやOpenGL対応、リソースコンパイラ（rc2obj等）を含む環境を構築できます。

## 構築・ビルド方法

### 1. 環境セットアップ
- Windows環境で動作します。
- 必要に応じて `dev\tcc_set.bat` を実行し、環境変数 `TCC_PATH` を設定してください。

### 2. サンプルのビルド
各サンプルフォルダにビルド用バッチファイル（.bat）が用意されています。  
例:  
- `sample\opengl\build_opengl.bat`
- `sample\Directx9\build_directx9.bat`
- `sample\rc\build.bat`
- `sample\ide\build.bat`
- `sample\nuklear_dx9\build.bat`
- `sample\microui_dx_ime\build.bat`

バッチファイルをダブルクリック、またはコマンドプロンプトで実行してください。

### 3. リソースコンパイラ
- `sample\rc\rc2obj.c` はリソースコンパイラのサンプルです。
- `rc2obj.exe` をビルド後、`.rc` ファイルを `.obj` に変換できます。

### 4. DirectX/日本語対応
- DirectX9やOpenGLのサンプルコードが含まれています。
- 日本語入力やIME対応エディタのサンプルも `sample\ide` フォルダにあります。

### 5. オプション・詳細
- `tcc` コマンドの日本語ヘルプやオプションは [tcc.c](tcc.c) 内や、各バッチファイルのコメントを参照してください。
- サンプル実行時は必要なDLLやライブラリがパスに存在することを確認してください。

## 参考
- 詳細は [readme.md](readme.md) を参照してください。
- サンプルコードやバッチファイルを編集することで、独自の拡張や日本語対応を追加できます。

---
