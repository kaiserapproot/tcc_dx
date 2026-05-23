# TCC + cimgui + DirectX11 セットアップ手順書

## 全体構成

```
cimgui/                          ← クローン済み
  cimgui.cpp
  cimgui.h
  cimgui_impl.cpp
  cimgui_impl.h
  backends_wrapper.cpp           ← 新規作成
  imgui_impl_cbridge.cpp         ← 新規作成
  imgui/                         ← サブモジュール
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_demo.cpp
    backends/
      imgui_impl_dx11.cpp
      imgui_impl_win32.cpp

↓ MSVCでDLLビルド

cimgui.dll
cimgui.def                       ← tcc -impdef で生成

↓ TCCでコンパイル

my_app.c → my_app.exe
```

---

## Step 1: サブモジュールの初期化

```bash
cd cimgui
git submodule update --init --recursive
```

`imgui/` フォルダ内に `imgui.cpp`、`backends/imgui_impl_dx11.cpp` などが入っていることを確認。

---

## Step 2: 追加ファイルの作成

### 2-1. `backends_wrapper.cpp` を作成

`cimgui/` 直下に作成。`IMGUI_IMPL_API` を空にして二重装飾を防ぐ。

```cpp
// backends_wrapper.cpp
#define IMGUI_IMPL_API
#include "imgui/backends/imgui_impl_dx11.cpp"
#include "imgui/backends/imgui_impl_win32.cpp"
```

### 2-2. `imgui_impl_cbridge.cpp` を作成

`cimgui/` 直下に作成。TCC（C言語）側から呼べる `extern "C"` ラッパー。
各関数に `__declspec(dllexport)` を個別に付けることが重要。

```cpp
// imgui_impl_cbridge.cpp
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include <windows.h>

// imgui_impl_win32.h はインクルードせず直接宣言（二重装飾回避）
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern bool    ImGui_ImplWin32_Init(void* hwnd);
extern void    ImGui_ImplWin32_Shutdown();
extern void    ImGui_ImplWin32_NewFrame();

#define CBRIDGE_API extern "C" __declspec(dllexport)

CBRIDGE_API int cbridge_ImGui_ImplDX11_Init(void* device, void* context) {
    return (int)ImGui_ImplDX11_Init((ID3D11Device*)device, (ID3D11DeviceContext*)context);
}
CBRIDGE_API void cbridge_ImGui_ImplDX11_Shutdown(void) { ImGui_ImplDX11_Shutdown(); }
CBRIDGE_API void cbridge_ImGui_ImplDX11_NewFrame(void) { ImGui_ImplDX11_NewFrame(); }
CBRIDGE_API void cbridge_ImGui_ImplDX11_RenderDrawData(void* draw_data) {
    ImGui_ImplDX11_RenderDrawData((ImDrawData*)draw_data);
}
CBRIDGE_API int  cbridge_ImGui_ImplWin32_Init(void* hwnd) { return (int)ImGui_ImplWin32_Init(hwnd); }
CBRIDGE_API void cbridge_ImGui_ImplWin32_Shutdown(void)   { ImGui_ImplWin32_Shutdown(); }
CBRIDGE_API void cbridge_ImGui_ImplWin32_NewFrame(void)   { ImGui_ImplWin32_NewFrame(); }
CBRIDGE_API long long cbridge_ImGui_ImplWin32_WndProcHandler(
    void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam)
{
    return (long long)ImGui_ImplWin32_WndProcHandler((HWND)hwnd, (UINT)msg, (WPARAM)wParam, (LPARAM)lParam);
}
```

> **重要：** `extern "C" { ... }` ブロック形式では `__declspec(dllexport)` が効かない。
> `CBRIDGE_API` マクロを各関数に個別に付ける形にすること。

---

## Step 3: cimgui.dll を MSVC でビルド

「x64 Native Tools Command Prompt for VS 20xx」を開き、`cimgui/` フォルダで実行：

```bat
cl.exe /LD /O2 /MD /EHsc ^
  /DCIMGUI_DLL ^
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
  /link d3d9.lib d3d10.lib d3d11.lib d3d12.lib dxgi.lib opengl32.lib user32.lib ^
  /OUT:cimgui.dll
```

| オプション | 意味 |
|---|---|
| `/LD` | DLLとしてビルド |
| `/DCIMGUI_DLL` | エクスポートマクロを有効化 |
| `/MD` | マルチスレッドDLL CRT |
| `imgui_demo.cpp` | `ShowDemoWindow` など必須（忘れるとリンクエラー） |
| `backends_wrapper.cpp` | 全バックエンドを `IMGUI_IMPL_API` 空で取り込むラッパー |
| `imgui_impl_cbridge.cpp` | C言語向けエクスポートラッパー（全バックエンド対応） |
| `d3d9.lib d3d10.lib d3d11.lib d3d12.lib` | 各 DirectX ランタイム |
| `opengl32.lib` | OpenGL2 ランタイム |

### ビルド確認

```bat
dumpbin /exports cimgui.dll | findstr cbridge
```

以下の関数が出力されることを確認：

```
cbridge_ImGui_ImplWin32_Init
cbridge_ImGui_ImplWin32_Shutdown
cbridge_ImGui_ImplWin32_NewFrame
cbridge_ImGui_ImplWin32_WndProcHandler

cbridge_ImGui_ImplDX9_Init
cbridge_ImGui_ImplDX9_Shutdown
cbridge_ImGui_ImplDX9_NewFrame
cbridge_ImGui_ImplDX9_RenderDrawData
cbridge_ImGui_ImplDX9_CreateDeviceObjects
cbridge_ImGui_ImplDX9_InvalidateDeviceObjects

cbridge_ImGui_ImplDX10_Init
cbridge_ImGui_ImplDX10_Shutdown
cbridge_ImGui_ImplDX10_NewFrame
cbridge_ImGui_ImplDX10_RenderDrawData
cbridge_ImGui_ImplDX10_CreateDeviceObjects
cbridge_ImGui_ImplDX10_InvalidateDeviceObjects

cbridge_ImGui_ImplDX11_Init
cbridge_ImGui_ImplDX11_Shutdown
cbridge_ImGui_ImplDX11_NewFrame
cbridge_ImGui_ImplDX11_RenderDrawData
cbridge_ImGui_ImplDX11_CreateDeviceObjects
cbridge_ImGui_ImplDX11_InvalidateDeviceObjects

cbridge_ImGui_ImplDX12_Init
cbridge_ImGui_ImplDX12_Shutdown
cbridge_ImGui_ImplDX12_NewFrame
cbridge_ImGui_ImplDX12_RenderDrawData
cbridge_ImGui_ImplDX12_CreateDeviceObjects
cbridge_ImGui_ImplDX12_InvalidateDeviceObjects

cbridge_ImGui_ImplOpenGL2_Init
cbridge_ImGui_ImplOpenGL2_Shutdown
cbridge_ImGui_ImplOpenGL2_NewFrame
cbridge_ImGui_ImplOpenGL2_RenderDrawData
cbridge_ImGui_ImplOpenGL2_CreateFontsTexture
cbridge_ImGui_ImplOpenGL2_DestroyFontsTexture
cbridge_ImGui_ImplOpenGL2_CreateDeviceObjects
cbridge_ImGui_ImplOpenGL2_DestroyDeviceObjects
```

---

## Step 4: .def ファイルの生成

```bat
tcc -impdef cimgui.dll -o cimgui.def
```

> **注意：** `tiny_impdef` ではなく TCC 付属の `-impdef` オプションを使う。

---

## Step 5: TCC 用 C コード（my_app.c）のポイント

### stdbool.h 対策

TCC には `stdbool.h` がない場合があるため、`cimgui.h` のインクルード前に定義する：

```c
#ifndef __bool_true_false_are_defined
typedef int bool;
#define true  1
#define false 0
#define __bool_true_false_are_defined 1
#endif

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
```

### cbridge 関数の宣言

`cimgui_impl.h` はインクルードせず、直接 `extern` 宣言する：

```c
// --- Win32 共通 ---
extern int       cbridge_ImGui_ImplWin32_Init(void* hwnd);
extern void      cbridge_ImGui_ImplWin32_Shutdown(void);
extern void      cbridge_ImGui_ImplWin32_NewFrame(void);
extern long long cbridge_ImGui_ImplWin32_WndProcHandler(
    void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam);

// --- DirectX 9 ---
extern int  cbridge_ImGui_ImplDX9_Init(void* device);
extern void cbridge_ImGui_ImplDX9_Shutdown(void);
extern void cbridge_ImGui_ImplDX9_NewFrame(void);
extern void cbridge_ImGui_ImplDX9_RenderDrawData(void* draw_data);
extern int  cbridge_ImGui_ImplDX9_CreateDeviceObjects(void);
extern void cbridge_ImGui_ImplDX9_InvalidateDeviceObjects(void);

// --- DirectX 10 ---
extern int  cbridge_ImGui_ImplDX10_Init(void* device);
extern void cbridge_ImGui_ImplDX10_Shutdown(void);
extern void cbridge_ImGui_ImplDX10_NewFrame(void);
extern void cbridge_ImGui_ImplDX10_RenderDrawData(void* draw_data);
extern int  cbridge_ImGui_ImplDX10_CreateDeviceObjects(void);
extern void cbridge_ImGui_ImplDX10_InvalidateDeviceObjects(void);

// --- DirectX 11 ---
extern int  cbridge_ImGui_ImplDX11_Init(void* device, void* context);
extern void cbridge_ImGui_ImplDX11_Shutdown(void);
extern void cbridge_ImGui_ImplDX11_NewFrame(void);
extern void cbridge_ImGui_ImplDX11_RenderDrawData(void* draw_data);
extern int  cbridge_ImGui_ImplDX11_CreateDeviceObjects(void);
extern void cbridge_ImGui_ImplDX11_InvalidateDeviceObjects(void);

// --- DirectX 12 ---
// D3D12_CPU_DESCRIPTOR_HANDLE.ptr → SIZE_T
// D3D12_GPU_DESCRIPTOR_HANDLE.ptr → unsigned long long
extern int  cbridge_ImGui_ImplDX12_Init(
    void* device, int num_frames_in_flight, unsigned int rtv_format,
    void* cbv_srv_heap,
    SIZE_T font_srv_cpu_desc_handle,
    unsigned long long font_srv_gpu_desc_handle);
extern void cbridge_ImGui_ImplDX12_Shutdown(void);
extern void cbridge_ImGui_ImplDX12_NewFrame(void);
extern void cbridge_ImGui_ImplDX12_RenderDrawData(void* draw_data, void* command_list);
extern int  cbridge_ImGui_ImplDX12_CreateDeviceObjects(void);
extern void cbridge_ImGui_ImplDX12_InvalidateDeviceObjects(void);

// --- OpenGL2 ---
extern int  cbridge_ImGui_ImplOpenGL2_Init(void);
extern void cbridge_ImGui_ImplOpenGL2_Shutdown(void);
extern void cbridge_ImGui_ImplOpenGL2_NewFrame(void);
extern void cbridge_ImGui_ImplOpenGL2_RenderDrawData(void* draw_data);
extern int  cbridge_ImGui_ImplOpenGL2_CreateFontsTexture(void);
extern void cbridge_ImGui_ImplOpenGL2_DestroyFontsTexture(void);
extern int  cbridge_ImGui_ImplOpenGL2_CreateDeviceObjects(void);
extern void cbridge_ImGui_ImplOpenGL2_DestroyDeviceObjects(void);
```

### `igGetIO()` の名前変更

新バージョンの cimgui では `igGetIO()` が `igGetIO_Nil()` に変更されている：

```c
ImGuiIO* io = igGetIO_Nil();
```

### 関数の戻り値に `->` を直接使えない

TCC の制限として、関数の戻り値に直接 `->` を使えないため一時変数に受ける：

```c
// NG
igGetIO_Nil()->Framerate;

// OK
ImGuiIO* io = igGetIO_Nil();
io->Framerate;
```

### COM インターフェースの完全な Vtbl 定義が必要

`ID3D11VertexShader`、`ID3D11PixelShader`、`ID3D11InputLayout` など
`->lpVtbl->Release()` を呼ぶ型はすべて Vtbl 構造体を定義しておく必要がある。
前方宣言（`typedef struct ID3D11VertexShader ID3D11VertexShader;`）だけでは不足。

```c
typedef struct ID3D11VertexShaderVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
} ID3D11VertexShaderVtbl;
typedef struct ID3D11VertexShader { ID3D11VertexShaderVtbl* lpVtbl; } ID3D11VertexShader;
// PixelShader、InputLayout も同様
```

### ウィンドウサイズとクライアントサイズの違い

`CreateWindowW` で指定するサイズは**ウィンドウ全体**のサイズであり、タイトルバーやボーダーを含む。
ImGui がマウス座標の基準にするのは**クライアント領域**（純粋な描画可能領域）なので両者は異なる。

```
┌─────────────────────────────┐ ←─ ウィンドウ全体: 800px
│  タイトルバー (約31px)       │
├─────────────────────────────┤
│ボ│                       │ボ│
│ー│  クライアント領域       │ー│  ← 784 x 561
│ダ│  （実際の描画領域）     │ダ│
│ー│                       │ー│
│(8)│                      │(8)│
└─────────────────────────────┘
         ↑ ボーダー (約8px)
```

- 横：800 - 左ボーダー(8) - 右ボーダー(8) = **784px**
- 縦：600 - タイトルバー(31) - 上ボーダー(8) = **561px**

※ピクセル数は Windows のテーマやバージョンによって異なる。

クライアント領域をちょうど `800 x 600` にしたい場合は `AdjustWindowRect` を使う：

```c
RECT rect = { 0, 0, 800, 600 };
AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
HWND hwnd = CreateWindowW(L"...", L"...", WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    rect.right - rect.left,   // ボーダー込みの正しい幅
    rect.bottom - rect.top,   // タイトルバー込みの正しい高さ
    ...);
```

### SwapChain のバッファサイズは `0` にする

`Width` と `Height` を固定値にすると、ImGui のマウス座標（クライアントサイズ基準）と
レンダリングバッファサイズが一致せずマウス位置がずれる。
`0` を指定するとクライアントサイズを自動使用するため一致する。

```c
scd.BufferDesc.Width  = 0;  // ← 固定値（800など）にしてはいけない
scd.BufferDesc.Height = 0;  // ← 固定値（600など）にしてはいけない
```

---

## Step 6: TCC でコンパイル

```bat
tcc my_app.c cimgui.def d3d11.def -luser32 -lkernel32 -lmsvcrt -limm32 -o my_app.exe
```

---

## 実行時のファイル配置

```
my_app.exe
cimgui.dll          ← 必須（同じフォルダ）
cimgui.h            ← コンパイル時のみ必要
d3d11.dll           ← Windows標準（システムにある）
D3DCompiler_47.dll  ← シェーダーコンパイルに必要
```

---

## トラブルシューティング

| エラー | 原因 | 対処 |
|---|---|---|
| `stdbool.h が見つかりません` | TCC に stdbool.h がない | `cimgui.h` の前に `typedef int bool` を定義 |
| `undefined symbol 'cbridge_...'` | DLL に cbridge 関数が未エクスポート | `CBRIDGE_API` を各関数に個別に付けてリビルド |
| `cbridge_` が def に出ない | `extern "C" {}` ブロック内では dllexport が効かない | `CBRIDGE_API` マクロを各関数に個別に付ける |
| `undefined symbol 'igGetIO'` | 新バージョンで名前変更 | `igGetIO_Nil()` を使う |
| `pointer が必要です` | 関数戻り値への直接 `->` | 一時変数に受けてから `->` を使う |
| `不完全型の参照解除` | Vtbl 定義がない | 該当型の Vtbl 構造体を定義する |
| `2 つ以上のストレージ クラス` | `IMGUI_IMPL_API` が二重装飾 | `backends_wrapper.cpp` で `#define IMGUI_IMPL_API` を空にする |
| `imgui_demo.cpp` 未追加 | `ShowDemoWindow` 等が未解決 | ビルドに `imgui/imgui_demo.cpp` を追加 |
| マウス位置が下にずれる（DPI） | DPI 非対応 | `main()` 先頭で `EnableDpiAwareness()` を呼ぶ |
| マウス位置が下にずれる（サイズ） | SwapChain バッファサイズ固定 | `BufferDesc.Width/Height` を `0` にして自動設定にする |
