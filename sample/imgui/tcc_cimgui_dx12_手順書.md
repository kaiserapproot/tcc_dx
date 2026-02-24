
# TCC + cimgui + DirectX12 セットアップ手順書

この手順書は `cimgui` リポジトリ上で TCC（Tiny C Compiler）から Dear ImGui を DirectX12 で動作させるための手順です。
基本的な流れは DirectX11 用手順と同様ですが、DX12 特有の初期化（`ImGui_ImplDX12_Init` の `InitInfo` 使用や SRV ディスクリプタ管理）に注意してください。

## 全体構成（例）

```
cimgui/                          ← クローン済み
  backends_wrapper.cpp           ← すべてのバックエンドを取り込むラッパー（このリポジトリにあり）
  imgui_impl_cbridge.cpp         ← C 呼び出し用ラッパー（このリポジトリにあり）
  imgui/                         ← imgui サブモジュール
    imgui.cpp
    imgui_draw.cpp
    backends/
      imgui_impl_dx12.cpp
      imgui_impl_win32.cpp

↓ MSVC で DLL をビルド

cimgui.dll
cimgui.def                       ← `tcc -impdef` で生成

↓ TCC でアプリをビルド

my_app_dx12.c → my_app_dx12.exe
```

---

## Step 1: サブモジュール初期化

```bat
cd cimgui
git submodule update --init --recursive
```

`imgui/` フォルダ内に `imgui.cpp`、`backends/imgui_impl_dx12.cpp`、`backends/imgui_impl_win32.cpp` 等があることを確認。

---

## Step 2: 追加ファイル（このリポジトリ既存）

リポジトリには既に DX12 用のラッパー実装が含まれています。重要点を列挙します。

- `backends_wrapper.cpp`:
  - `#define IMGUI_IMPL_API __declspec(dllexport)` 等でバックエンド実装を DLL 内に取り込んでいます。
  - `imgui_impl_dx12.cpp` と `imgui_impl_win32.cpp` を含めてください（既に含まれています）。

- `imgui_impl_cbridge.cpp`:
  - `extern "C" __declspec(dllexport)` を使った TCC 側から呼べるラッパー群を提供しています。
  - DX12 側は既に `ImGui_ImplDX12_Init(ImGui_ImplDX12_InitInfo*)` を使う実装に対応済みで、内部で SRV アロケータを渡すようになっています。

（サンプル実装がリポジトリにあるため、新規作成は不要です）

---

## Step 3: cimgui.dll を MSVC でビルド

「x64 Native Tools Command Prompt for VS」を使い、`cimgui/` フォルダで次の例を実行します（ファイルパスはプロジェクト構成に合わせて下さい）。

```bat
cl.exe /LD /O2 /MD /EHsc ^
  /I. /Iimgui /Iimgui/backends ^
  cimgui.cpp ^
  cimgui_impl.cpp ^
  imgui/imgui.cpp ^
  imgui/imgui_draw.cpp ^
  imgui/imgui_tables.cpp ^
  imgui/imgui_widgets.cpp ^
  imgui/imgui_demo.cpp ^
  backends_wrapper.cpp ^
  imgui_impl_cbridge.cpp ^
  /link d3d12.lib dxgi.lib d3dcompiler.lib user32.lib dxguid.lib kernel32.lib opengl32.lib ^
  /OUT:cimgui.dll
```

ポイント:
- DX12 用なので `d3d12.lib` と `dxgi.lib` をリンクします。
- `d3dcompiler.lib` はバックエンドのシェーダーコンパイルに必要です（imgui_impl_dx12.cpp 内で D3DCompile を使っているため）。
- `backends_wrapper.cpp` と `imgui_impl_cbridge.cpp` を必ず含めてください。

ビルド後、エクスポートを確認：

```bat
dumpbin /exports cimgui.dll | findstr cbridge
```

期待する関数（例）:
- `cbridge_ImGui_ImplDX12_Init`
- `cbridge_ImGui_ImplDX12_NewFrame`
- `cbridge_ImGui_ImplDX12_RenderDrawData`
- `cbridge_ImGui_ImplWin32_*` 系など

---

## Step 4: .def ファイルを生成

```bat
tcc -impdef cimgui.dll -o cimgui.def
```

これは TCC 側で DLL エクスポートを参照するために使います。

---

## Step 5: TCC 側 C コード（`my_app_dx12.c` のポイント）

リポジトリ付属の `my_app_dx12.c` をそのまま利用できます。実装上の重要点をまとめます。

- ImGui コンテキストを先に作成する:
  - `igCreateContext(NULL);` を呼ぶ（`ImGui_ImplDX12_Init` が内部でコンテキストを参照する可能性があるため）

- 呼び出す cbridge 関数（`imgui_impl_cbridge.cpp` でエクスポート済み）:

  ```c
  extern int  cbridge_DX12_Init(void* hwnd, int numBackBuffers, int numFramesInFlight);
  extern void cbridge_DX12_NewFrame(void);
  extern void cbridge_DX12_Render(const float* clear_color);
  extern void cbridge_DX12_ResizeBuffers(unsigned int w, unsigned int h);
  extern void cbridge_DX12_Shutdown(void);
  
  extern int cbridge_ImGui_ImplWin32_Init(void* hwnd);
  extern void cbridge_ImGui_ImplWin32_NewFrame(void);
  extern void cbridge_ImGui_ImplWin32_Shutdown(void);
  extern long long cbridge_ImGui_ImplWin32_WndProcHandler(void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam);
  ```

- フレーム順序（重要）:
  - `cbridge_DX12_NewFrame()` を呼んでから `cbridge_ImGui_ImplWin32_NewFrame()`、その後 `igNewFrame()` を呼びます。
  - 描画は `igRender()` のあとに `cbridge_DX12_Render(clear_color)` を呼びます。

- CPU/GPU ハンドルを TCC 側で渡す必要がある古い API を使う場合の注意
  - 既存のラッパーは `ImGui_ImplDX12_Init(ImGui_ImplDX12_InitInfo*)` を利用しており、アロケータ関数は C++ 側で実装しています。
  - もしレガシー API を直接使う場合は `D3D12_CPU_DESCRIPTOR_HANDLE.ptr` を `SIZE_T`、`D3D12_GPU_DESCRIPTOR_HANDLE.ptr` を `unsigned long long` として TCC 側で渡す必要があります（ただし本リポジトリの `imgui_impl_cbridge.cpp` は InitInfo ベースを使うため不要）。

---

## Step 6: TCC でコンパイル（例）

```bat
tcc my_app_dx12.c cimgui.def d3d12.def dxgi.def -luser32 -lkernel32 -lmsvcrt -o my_app_dx12.exe
```

注意:
- `d3d12.def` / `dxgi.def` 等は環境に応じて用意してください（サンプルリポジトリに `*.def` がある場合はそれを使えます）。
- TCC と DLL のアーキテクチャが一致している必要があります（x64 でビルドした DLL には x64 TCC を使う）。

---

## 実行時のファイル配置

```
my_app_dx12.exe
cimgui.dll          ← 実行時に同フォルダに配置
cimgui.def          ← コンパイル時のみ必要
d3d12.dll, dxgi.dll ← システム標準
d3dcompiler_47.dll  ← シェーダーコンパイルに必要（存在しない場合はランタイムエラー）
```

---

## よくあるトラブルと対処

- アサーション: "Backend does not support ImGuiBackendFlags_RendererHasTextures, and font atlas is not built!"
  - 原因: レンダラバックエンドがフォントテクスチャを自動生成していない（旧API期待）
  - 対処: `ImGui_ImplDX12_Init` の新しい `InitInfo` 経路（本リポジトリの `imgui_impl_cbridge.cpp` は対応済み）を使うか、レンダラの NewFrame で `io.Fonts->GetTexDataAsRGBA32()` を確実に呼ぶこと。

- アサーション: `tex->TexID == ((ImTextureID)0) && tex->BackendUserData == nullptr`（TexID/BackendUserData 関連）
  - 原因: フォント用 SRV を作成して `io.Fonts->SetTexID()` を与えていない、または DLL 古いバージョンを使用している
  - 対処: `cimgui.dll` を再ビルドして最新の DLL を読み込ませる。実行前に古い DLL が参照されていないか確認。

- 描画されるが文字が出ない／クラッシュする
  - `ImGui_ImplDX12_NewFrame()` が呼ばれているか確認（`cbridge_DX12_NewFrame()` を呼ぶこと）。
  - ビルド時に `imgui_impl_dx12.cpp` と `imgui_impl_win32.cpp` が正しくリンクされているか。
  - `d3dcompiler` のバージョン差による問題が稀にあるので、`d3dcompiler_47.dll` が正しい場所にあるか確認。

---

## 補足（開発者向け）

- `imgui_impl_dx12.cpp` の実装は `ImGui_ImplDX12_InitInfo` を受け取り、プラットフォーム I/O (`ImGui::GetPlatformIO().Textures`) を通じて動的テクスチャの更新を行います。これによりバックエンド側でフォントアトラスの生成・アップロード・SRV 登録を正しく行えるため、古い `GetTexDataAsRGBA32()` をアプリ側で呼ぶ必要はありません（ただし古いサンプル・実装を流用すると不具合になります）。

- 64bit ビルド（x64）での運用を推奨します。TCC と DLL のアーキテクチャが一致しないと呼び出しが壊れます。

---

参考: `my_app_dx12.c`（リポジトリ内のサンプル）をまずそのままビルド・実行し、動作を確認してください。動作しない場合はビルドログ・dumpbin 出力・実行時のエラーメッセージ（スクリーンショット）を共有してください。

````
