#include <windows.h>
#include <stdio.h>
#include <string.h>

// -------------------------------------------------------
// stdbool.h がない TCC 向け対策
// -------------------------------------------------------
#ifndef __bool_true_false_are_defined
typedef int bool;
#define true  1
#define false 0
#define __bool_true_false_are_defined 1
#endif

// -------------------------------------------------------
// cimgui インクルード
// -------------------------------------------------------
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

// -------------------------------------------------------
// cbridge 関数宣言
// -------------------------------------------------------
extern int       cbridge_ImGui_ImplWin32_Init(void* hwnd);
extern void      cbridge_ImGui_ImplWin32_Shutdown(void);
extern void      cbridge_ImGui_ImplWin32_NewFrame(void);
extern long long cbridge_ImGui_ImplWin32_WndProcHandler(void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam);

extern int       cbridge_ImGui_ImplDX9_Init(void* device);
extern void      cbridge_ImGui_ImplDX9_Shutdown(void);
extern void      cbridge_ImGui_ImplDX9_NewFrame(void);
extern void      cbridge_ImGui_ImplDX9_RenderDrawData(void* draw_data);
extern int       cbridge_ImGui_ImplDX9_CreateDeviceObjects(void);
extern void      cbridge_ImGui_ImplDX9_InvalidateDeviceObjects(void);

// -------------------------------------------------------
// D3D9 定数
// -------------------------------------------------------
#define D3D_SDK_VERSION                32
#define D3DADAPTER_DEFAULT             0
#define D3DDEVTYPE_HAL                 1
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x00000040
#define D3DSWAPEFFECT_DISCARD          1
#define D3DFMT_A8R8G8B8                21
#define D3DFMT_D16                     80
#define D3DFMT_UNKNOWN                 0
#define D3DCLEAR_TARGET                0x00000001
#define D3DPT_TRIANGLELIST             4
#define D3DFVF_XYZ                     0x002
#define D3DFVF_DIFFUSE                 0x040
#define D3DFVF_CUSTOM                  (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define D3DPOOL_DEFAULT                0
#define D3DUSAGE_WRITEONLY             0x00000008
#define D3DCOLOR_RGBA(r,g,b,a)         ((DWORD)(((a)<<24)|((r)<<16)|((g)<<8)|(b)))
#define D3DCOLOR_XRGB(r,g,b)          D3DCOLOR_RGBA(r,g,b,0xff)

// -------------------------------------------------------
// D3D9 構造体
// -------------------------------------------------------
typedef struct D3DPRESENT_PARAMETERS {
    UINT                BackBufferWidth;
    UINT                BackBufferHeight;
    UINT                BackBufferFormat;      // D3DFORMAT
    UINT                BackBufferCount;
    UINT                MultiSampleType;       // D3DMULTISAMPLE_TYPE
    DWORD               MultiSampleQuality;
    UINT                SwapEffect;            // D3DSWAPEFFECT
    HWND                hDeviceWindow;
    BOOL                Windowed;
    BOOL                EnableAutoDepthStencil;
    UINT                AutoDepthStencilFormat; // D3DFORMAT
    DWORD               Flags;
    UINT                FullScreen_RefreshRateInHz;
    UINT                PresentationInterval;
} D3DPRESENT_PARAMETERS;

typedef struct D3DVIEWPORT9 {
    DWORD X, Y, Width, Height;
    float MinZ, MaxZ;
} D3DVIEWPORT9;

// カスタム頂点（座標 + 色）
typedef struct {
    float x, y, z;
    DWORD color;
} CustomVertex;

// -------------------------------------------------------
// IDirect3D9 Vtbl（必要なメソッドのみ）
// -------------------------------------------------------
typedef struct IDirect3D9Vtbl {
    // IUnknown
    HRESULT (__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG   (__stdcall* AddRef)(void*);
    ULONG   (__stdcall* Release)(void*);
    // IDirect3D9
    HRESULT (__stdcall* RegisterSoftwareDevice)(void*, void*);
    UINT    (__stdcall* GetAdapterCount)(void*);
    HRESULT (__stdcall* GetAdapterIdentifier)(void*, UINT, DWORD, void*);
    UINT    (__stdcall* GetAdapterModeCount)(void*, UINT, UINT);
    HRESULT (__stdcall* EnumAdapterModes)(void*, UINT, UINT, UINT, void*);
    HRESULT (__stdcall* GetAdapterDisplayMode)(void*, UINT, void*);
    HRESULT (__stdcall* CheckDeviceType)(void*, UINT, UINT, UINT, UINT, BOOL);
    HRESULT (__stdcall* CheckDeviceFormat)(void*, UINT, UINT, UINT, DWORD, UINT, UINT);
    HRESULT (__stdcall* CheckDeviceMultiSampleType)(void*, UINT, UINT, UINT, BOOL, UINT, DWORD*);
    HRESULT (__stdcall* CheckDepthStencilMatch)(void*, UINT, UINT, UINT, UINT, UINT);
    HRESULT (__stdcall* CheckDeviceFormatConversion)(void*, UINT, UINT, UINT, UINT);
    HRESULT (__stdcall* GetDeviceCaps)(void*, UINT, UINT, void*);
    HMONITOR(__stdcall* GetAdapterMonitor)(void*, UINT);
    HRESULT (__stdcall* CreateDevice)(void*, UINT, UINT, HWND, DWORD, D3DPRESENT_PARAMETERS*, void**);
} IDirect3D9Vtbl;
typedef struct IDirect3D9 { IDirect3D9Vtbl* lpVtbl; } IDirect3D9;

// -------------------------------------------------------
// IDirect3DDevice9 Vtbl（必要なメソッドのみ）
// -------------------------------------------------------
typedef struct IDirect3DDevice9Vtbl {
    // IUnknown
    HRESULT (__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG   (__stdcall* AddRef)(void*);
    ULONG   (__stdcall* Release)(void*);
    // IDirect3DDevice9
    HRESULT (__stdcall* TestCooperativeLevel)(void*);
    UINT    (__stdcall* GetAvailableTextureMem)(void*);
    HRESULT (__stdcall* EvictManagedResources)(void*);
    HRESULT (__stdcall* GetDirect3D)(void*, void**);
    HRESULT (__stdcall* GetDeviceCaps)(void*, void*);
    HRESULT (__stdcall* GetDisplayMode)(void*, UINT, void*);
    HRESULT (__stdcall* GetCreationParameters)(void*, void*);
    HRESULT (__stdcall* SetCursorProperties)(void*, UINT, UINT, void*);
    void    (__stdcall* SetCursorPosition)(void*, int, int, DWORD);
    BOOL    (__stdcall* ShowCursor)(void*, BOOL);
    HRESULT (__stdcall* CreateAdditionalSwapChain)(void*, D3DPRESENT_PARAMETERS*, void**);
    HRESULT (__stdcall* GetSwapChain)(void*, UINT, void**);
    UINT    (__stdcall* GetNumberOfSwapChains)(void*);
    HRESULT (__stdcall* Reset)(void*, D3DPRESENT_PARAMETERS*);
    HRESULT (__stdcall* Present)(void*, const void*, const void*, HWND, const void*);
    HRESULT (__stdcall* GetBackBuffer)(void*, UINT, UINT, UINT, void**);
    HRESULT (__stdcall* GetRasterStatus)(void*, UINT, void*);
    HRESULT (__stdcall* SetDialogBoxMode)(void*, BOOL);
    void    (__stdcall* SetGammaRamp)(void*, UINT, DWORD, const void*);
    void    (__stdcall* GetGammaRamp)(void*, UINT, void*);
    HRESULT (__stdcall* CreateTexture)(void*, UINT, UINT, UINT, DWORD, UINT, UINT, void**, HANDLE*);
    HRESULT (__stdcall* CreateVolumeTexture)(void*, UINT, UINT, UINT, UINT, DWORD, UINT, UINT, void**, HANDLE*);
    HRESULT (__stdcall* CreateCubeTexture)(void*, UINT, UINT, DWORD, UINT, UINT, void**, HANDLE*);
    HRESULT (__stdcall* CreateVertexBuffer)(void*, UINT, DWORD, DWORD, UINT, void**, HANDLE*);
    HRESULT (__stdcall* CreateIndexBuffer)(void*, UINT, DWORD, UINT, UINT, void**, HANDLE*);
    HRESULT (__stdcall* CreateRenderTarget)(void*, UINT, UINT, UINT, UINT, BOOL, void**, HANDLE*);
    HRESULT (__stdcall* CreateDepthStencilSurface)(void*, UINT, UINT, UINT, UINT, BOOL, void**, HANDLE*);
    HRESULT (__stdcall* UpdateSurface)(void*, void*, const RECT*, void*, const POINT*);
    HRESULT (__stdcall* UpdateTexture)(void*, void*, void*);
    HRESULT (__stdcall* GetRenderTargetData)(void*, void*, void*);
    HRESULT (__stdcall* GetFrontBufferData)(void*, UINT, void*);
    HRESULT (__stdcall* StretchRect)(void*, void*, const RECT*, void*, const RECT*, UINT);
    HRESULT (__stdcall* ColorFill)(void*, void*, const RECT*, DWORD);
    HRESULT (__stdcall* CreateOffscreenPlainSurface)(void*, UINT, UINT, UINT, UINT, void**, HANDLE*);
    HRESULT (__stdcall* SetRenderTarget)(void*, DWORD, void*);
    HRESULT (__stdcall* GetRenderTarget)(void*, DWORD, void**);
    HRESULT (__stdcall* SetDepthStencilSurface)(void*, void*);
    HRESULT (__stdcall* GetDepthStencilSurface)(void*, void**);
    HRESULT (__stdcall* BeginScene)(void*);
    HRESULT (__stdcall* EndScene)(void*);
    HRESULT (__stdcall* Clear)(void*, DWORD, const void*, DWORD, DWORD, float, DWORD);
    HRESULT (__stdcall* SetTransform)(void*, DWORD, const void*);
    HRESULT (__stdcall* GetTransform)(void*, DWORD, void*);
    HRESULT (__stdcall* MultiplyTransform)(void*, DWORD, const void*);
    HRESULT (__stdcall* SetViewport)(void*, const D3DVIEWPORT9*);
    HRESULT (__stdcall* GetViewport)(void*, D3DVIEWPORT9*);
    HRESULT (__stdcall* SetMaterial)(void*, const void*);
    HRESULT (__stdcall* GetMaterial)(void*, void*);
    HRESULT (__stdcall* SetLight)(void*, DWORD, const void*);
    HRESULT (__stdcall* GetLight)(void*, DWORD, void*);
    HRESULT (__stdcall* LightEnable)(void*, DWORD, BOOL);
    HRESULT (__stdcall* GetLightEnable)(void*, DWORD, BOOL*);
    HRESULT (__stdcall* SetClipPlane)(void*, DWORD, const float*);
    HRESULT (__stdcall* GetClipPlane)(void*, DWORD, float*);
    HRESULT (__stdcall* SetRenderState)(void*, DWORD, DWORD);
    HRESULT (__stdcall* GetRenderState)(void*, DWORD, DWORD*);
    HRESULT (__stdcall* CreateStateBlock)(void*, UINT, void**);
    HRESULT (__stdcall* BeginStateBlock)(void*);
    HRESULT (__stdcall* EndStateBlock)(void*, void**);
    HRESULT (__stdcall* SetClipStatus)(void*, const void*);
    HRESULT (__stdcall* GetClipStatus)(void*, void*);
    HRESULT (__stdcall* GetTexture)(void*, DWORD, void**);
    HRESULT (__stdcall* SetTexture)(void*, DWORD, void*);
    HRESULT (__stdcall* GetTextureStageState)(void*, DWORD, DWORD, DWORD*);
    HRESULT (__stdcall* SetTextureStageState)(void*, DWORD, DWORD, DWORD);
    HRESULT (__stdcall* GetSamplerState)(void*, DWORD, DWORD, DWORD*);
    HRESULT (__stdcall* SetSamplerState)(void*, DWORD, DWORD, DWORD);
    HRESULT (__stdcall* ValidateDevice)(void*, DWORD*);
    HRESULT (__stdcall* SetPaletteEntries)(void*, UINT, const void*);
    HRESULT (__stdcall* GetPaletteEntries)(void*, UINT, void*);
    HRESULT (__stdcall* SetCurrentTexturePalette)(void*, UINT);
    HRESULT (__stdcall* GetCurrentTexturePalette)(void*, UINT*);
    HRESULT (__stdcall* SetScissorRect)(void*, const RECT*);
    HRESULT (__stdcall* GetScissorRect)(void*, RECT*);
    HRESULT (__stdcall* SetSoftwareVertexProcessing)(void*, BOOL);
    BOOL    (__stdcall* GetSoftwareVertexProcessing)(void*);
    HRESULT (__stdcall* SetNPatchMode)(void*, float);
    float   (__stdcall* GetNPatchMode)(void*);
    HRESULT (__stdcall* DrawPrimitive)(void*, UINT, UINT, UINT);
    HRESULT (__stdcall* DrawIndexedPrimitive)(void*, UINT, INT, UINT, UINT, UINT, UINT);
    HRESULT (__stdcall* DrawPrimitiveUP)(void*, UINT, UINT, const void*, UINT);
    HRESULT (__stdcall* DrawIndexedPrimitiveUP)(void*, UINT, UINT, UINT, UINT, const void*, UINT, const void*, UINT);
    HRESULT (__stdcall* ProcessVertices)(void*, UINT, UINT, UINT, void*, void*, DWORD);
    HRESULT (__stdcall* CreateVertexDeclaration)(void*, const void*, void**);
    HRESULT (__stdcall* SetVertexDeclaration)(void*, void*);
    HRESULT (__stdcall* GetVertexDeclaration)(void*, void**);
    HRESULT (__stdcall* SetFVF)(void*, DWORD);
    HRESULT (__stdcall* GetFVF)(void*, DWORD*);
    HRESULT (__stdcall* CreateVertexShader)(void*, const DWORD*, void**);
    HRESULT (__stdcall* SetVertexShader)(void*, void*);
    HRESULT (__stdcall* GetVertexShader)(void*, void**);
    HRESULT (__stdcall* SetVertexShaderConstantF)(void*, UINT, const float*, UINT);
    HRESULT (__stdcall* GetVertexShaderConstantF)(void*, UINT, float*, UINT);
    HRESULT (__stdcall* SetVertexShaderConstantI)(void*, UINT, const int*, UINT);
    HRESULT (__stdcall* GetVertexShaderConstantI)(void*, UINT, int*, UINT);
    HRESULT (__stdcall* SetVertexShaderConstantB)(void*, UINT, const BOOL*, UINT);
    HRESULT (__stdcall* GetVertexShaderConstantB)(void*, UINT, BOOL*, UINT);
    HRESULT (__stdcall* SetStreamSource)(void*, UINT, void*, UINT, UINT);
    HRESULT (__stdcall* GetStreamSource)(void*, UINT, void**, UINT*, UINT*);
    HRESULT (__stdcall* SetStreamSourceFreq)(void*, UINT, UINT);
    HRESULT (__stdcall* GetStreamSourceFreq)(void*, UINT, UINT*);
    HRESULT (__stdcall* SetIndices)(void*, void*);
    HRESULT (__stdcall* GetIndices)(void*, void**);
    HRESULT (__stdcall* CreatePixelShader)(void*, const DWORD*, void**);
    HRESULT (__stdcall* SetPixelShader)(void*, void*);
    HRESULT (__stdcall* GetPixelShader)(void*, void**);
    HRESULT (__stdcall* SetPixelShaderConstantF)(void*, UINT, const float*, UINT);
    HRESULT (__stdcall* GetPixelShaderConstantF)(void*, UINT, float*, UINT);
    HRESULT (__stdcall* SetPixelShaderConstantI)(void*, UINT, const int*, UINT);
    HRESULT (__stdcall* GetPixelShaderConstantI)(void*, UINT, int*, UINT);
    HRESULT (__stdcall* SetPixelShaderConstantB)(void*, UINT, const BOOL*, UINT);
    HRESULT (__stdcall* GetPixelShaderConstantB)(void*, UINT, BOOL*, UINT);
    HRESULT (__stdcall* DrawRectPatch)(void*, UINT, const float*, const void*);
    HRESULT (__stdcall* DrawTriPatch)(void*, UINT, const float*, const void*);
    HRESULT (__stdcall* DeletePatch)(void*, UINT);
    HRESULT (__stdcall* CreateQuery)(void*, UINT, void**);
} IDirect3DDevice9Vtbl;
typedef struct IDirect3DDevice9 { IDirect3DDevice9Vtbl* lpVtbl; } IDirect3DDevice9;

// IDirect3DVertexBuffer9
typedef struct IDirect3DVertexBuffer9Vtbl {
    HRESULT (__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG   (__stdcall* AddRef)(void*);
    ULONG   (__stdcall* Release)(void*);
    HRESULT (__stdcall* GetDevice)(void*, void**);
    HRESULT (__stdcall* SetPrivateData)(void*, const void*, const void*, DWORD, DWORD);
    HRESULT (__stdcall* GetPrivateData)(void*, const void*, void*, DWORD*);
    HRESULT (__stdcall* FreePrivateData)(void*, const void*);
    DWORD   (__stdcall* SetPriority)(void*, DWORD);
    DWORD   (__stdcall* GetPriority)(void*);
    void    (__stdcall* PreLoad)(void*);
    UINT    (__stdcall* GetType)(void*);
    HRESULT (__stdcall* Lock)(void*, UINT, UINT, void**, DWORD);
    HRESULT (__stdcall* Unlock)(void*);
    HRESULT (__stdcall* GetDesc)(void*, void*);
} IDirect3DVertexBuffer9Vtbl;
typedef struct IDirect3DVertexBuffer9 { IDirect3DVertexBuffer9Vtbl* lpVtbl; } IDirect3DVertexBuffer9;

// -------------------------------------------------------
// Direct3DCreate9 関数宣言
// -------------------------------------------------------
typedef IDirect3D9* (__stdcall* PFN_Direct3DCreate9)(UINT);
static PFN_Direct3DCreate9 g_pDirect3DCreate9 = NULL;

// -------------------------------------------------------
// グローバル変数
// -------------------------------------------------------
static IDirect3D9*           g_d3d        = NULL;
static IDirect3DDevice9*     g_dev        = NULL;
static IDirect3DVertexBuffer9* g_vb       = NULL;
static D3DPRESENT_PARAMETERS g_d3dpp;

static int   g_show_demo      = 1;
static int   g_show_my_window = 1;
static float g_clear_color[3] = { 0.0f, 0.2f, 0.4f };
static float g_counter        = 0.0f;
static int   g_device_lost    = 0;

// -------------------------------------------------------
// DPI 対応
// -------------------------------------------------------
typedef enum PROCESS_DPI_AWARENESS {
    PROCESS_DPI_UNAWARE           = 0,
    PROCESS_SYSTEM_DPI_AWARE      = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
typedef HRESULT(__stdcall* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);

static void EnableDpiAwareness()
{
    HMODULE shcore = LoadLibraryA("shcore.dll");
    if (shcore) {
        PFN_SetProcessDpiAwareness fn =
            (PFN_SetProcessDpiAwareness)GetProcAddress(shcore, "SetProcessDpiAwareness");
        if (fn) { fn(PROCESS_PER_MONITOR_DPI_AWARE); return; }
    }
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        BOOL(__stdcall* fn2)(void) =
            (BOOL(__stdcall*)(void))GetProcAddress(user32, "SetProcessDPIAware");
        if (fn2) fn2();
    }
}

// -------------------------------------------------------
// WndProc
// -------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (cbridge_ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return TRUE;
    if (msg == WM_SIZE && g_dev) {
        // リサイズ時はデバイスロスト扱いでリセット
        g_device_lost = 1;
    }
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// -------------------------------------------------------
// 頂点バッファ作成
// -------------------------------------------------------
BOOL CreateVertexBuffer()
{
    HRESULT hr = g_dev->lpVtbl->CreateVertexBuffer(g_dev,
        3 * sizeof(CustomVertex),
        D3DUSAGE_WRITEONLY,
        D3DFVF_CUSTOM,
        D3DPOOL_DEFAULT,
        (void**)&g_vb, NULL);
    if (hr != 0) { printf("CreateVertexBuffer failed: 0x%08X\n", hr); return FALSE; }

    CustomVertex vertices[] = {
        {  0.0f,  0.5f, 0.5f, D3DCOLOR_XRGB(255,   0,   0) },
        {  0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(  0, 255,   0) },
        { -0.5f, -0.5f, 0.5f, D3DCOLOR_XRGB(  0,   0, 255) },
    };
    void* pData = NULL;
    hr = g_vb->lpVtbl->Lock(g_vb, 0, sizeof(vertices), &pData, 0);
    if (hr != 0) { printf("VB Lock failed\n"); return FALSE; }
    memcpy(pData, vertices, sizeof(vertices));
    g_vb->lpVtbl->Unlock(g_vb);
    return TRUE;
}

// -------------------------------------------------------
// D3D9 初期化
// -------------------------------------------------------
BOOL InitD3D(HWND hwnd)
{
    // d3d9.dll を動的ロード
    HMODULE hD3D9 = LoadLibraryA("d3d9.dll");
    if (!hD3D9) { printf("d3d9.dll not found\n"); return FALSE; }
    g_pDirect3DCreate9 = (PFN_Direct3DCreate9)GetProcAddress(hD3D9, "Direct3DCreate9");
    if (!g_pDirect3DCreate9) { printf("Direct3DCreate9 not found\n"); return FALSE; }

    g_d3d = g_pDirect3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d) { printf("Direct3DCreate9 failed\n"); return FALSE; }

    // クライアントサイズを取得
    RECT rect;
    GetClientRect(hwnd, &rect);

    memset(&g_d3dpp, 0, sizeof(g_d3dpp));
    g_d3dpp.BackBufferWidth            = rect.right - rect.left;
    g_d3dpp.BackBufferHeight           = rect.bottom - rect.top;
    g_d3dpp.BackBufferFormat           = D3DFMT_A8R8G8B8;
    g_d3dpp.BackBufferCount            = 1;
    g_d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.hDeviceWindow              = hwnd;
    g_d3dpp.Windowed                   = TRUE;
    g_d3dpp.EnableAutoDepthStencil     = FALSE;
    g_d3dpp.PresentationInterval       = 1; // D3DPRESENT_INTERVAL_ONE

    HRESULT hr = g_d3d->lpVtbl->CreateDevice(g_d3d,
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hwnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &g_d3dpp,
        (void**)&g_dev);
    if (hr != 0) { printf("CreateDevice failed: 0x%08X\n", hr); return FALSE; }

    return CreateVertexBuffer();
}

// -------------------------------------------------------
// ImGui 初期化
// -------------------------------------------------------
void InitImGui(HWND hwnd)
{
    igCreateContext(NULL);

    ImGuiIO* io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    igStyleColorsDark(NULL);

    cbridge_ImGui_ImplWin32_Init(hwnd);
    cbridge_ImGui_ImplDX9_Init(g_dev);
}

// -------------------------------------------------------
// デバイスリセット（ウィンドウリサイズ・デバイスロスト時）
// -------------------------------------------------------
void ResetDevice(HWND hwnd)
{
    cbridge_ImGui_ImplDX9_InvalidateDeviceObjects();

    // 頂点バッファ解放
    if (g_vb) { g_vb->lpVtbl->Release(g_vb); g_vb = NULL; }

    // クライアントサイズ再取得
    RECT rect;
    GetClientRect(hwnd, &rect);
    g_d3dpp.BackBufferWidth  = rect.right  - rect.left;
    g_d3dpp.BackBufferHeight = rect.bottom - rect.top;

    HRESULT hr = g_dev->lpVtbl->Reset(g_dev, &g_d3dpp);
    if (hr != 0) {
        printf("Reset failed: 0x%08X\n", hr);
        return;
    }

    CreateVertexBuffer();
    cbridge_ImGui_ImplDX9_CreateDeviceObjects();
    g_device_lost = 0;
}

// -------------------------------------------------------
// レンダリング
// -------------------------------------------------------
void Render(HWND hwnd)
{
    // デバイスロスト・リサイズチェック
    if (g_device_lost) {
        HRESULT hr = g_dev->lpVtbl->TestCooperativeLevel(g_dev);
        if (hr == 0 || hr == 0x88760869 /* D3DERR_DEVICENOTRESET */) {
            ResetDevice(hwnd);
        } else {
            Sleep(10);
            return;
        }
    }

    DWORD clearColor = D3DCOLOR_RGBA(
        (int)(g_clear_color[0] * 255),
        (int)(g_clear_color[1] * 255),
        (int)(g_clear_color[2] * 255),
        255);

    g_dev->lpVtbl->Clear(g_dev, 0, NULL, D3DCLEAR_TARGET, clearColor, 1.0f, 0);

    if (g_dev->lpVtbl->BeginScene(g_dev) == 0)
    {
        // 三角形描画
        if (g_vb)
        {
            g_dev->lpVtbl->SetFVF(g_dev, D3DFVF_CUSTOM);
            g_dev->lpVtbl->SetStreamSource(g_dev, 0, (void*)g_vb, 0, sizeof(CustomVertex));
            g_dev->lpVtbl->SetVertexShader(g_dev, NULL);
            g_dev->lpVtbl->SetPixelShader(g_dev, NULL);
            g_dev->lpVtbl->SetTexture(g_dev, 0, NULL);
            // ライティング無効化
            g_dev->lpVtbl->SetRenderState(g_dev, 137 /* D3DRS_LIGHTING */, FALSE);
            g_dev->lpVtbl->DrawPrimitive(g_dev, D3DPT_TRIANGLELIST, 0, 1);
        }

        // ImGui フレーム開始
        cbridge_ImGui_ImplDX9_NewFrame();
        cbridge_ImGui_ImplWin32_NewFrame();
        igNewFrame();

        // デモウィンドウ
        if (g_show_demo)
            igShowDemoWindow(&g_show_demo);

        // 独自ウィンドウ
        if (g_show_my_window)
        {
            igBegin("TCC + cimgui + DX9", &g_show_my_window, 0);

            igText("Hello from TCC + DirectX9!");
            igSeparator();

            igColorEdit3("Background", g_clear_color, 0);
            igSeparator();

            if (igButton("Count up", (ImVec2){0,0}))
                g_counter += 1.0f;
            igSameLine(0.0f, -1.0f);
            igText("count = %.0f", g_counter);

            igSeparator();
            ImGuiIO* io = igGetIO_Nil();
            igText("%.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);

            igEnd();
        }

        // ImGui 描画
        igRender();
        cbridge_ImGui_ImplDX9_RenderDrawData(igGetDrawData());

        g_dev->lpVtbl->EndScene(g_dev);
    }

    HRESULT hr = g_dev->lpVtbl->Present(g_dev, NULL, NULL, NULL, NULL);
    if (hr == 0x88760868 /* D3DERR_DEVICELOST */) {
        g_device_lost = 1;
    }
}

// -------------------------------------------------------
// クリーンアップ
// -------------------------------------------------------
void Cleanup()
{
    cbridge_ImGui_ImplDX9_Shutdown();
    cbridge_ImGui_ImplWin32_Shutdown();
    igDestroyContext(NULL);

    if (g_vb)  { g_vb->lpVtbl->Release(g_vb);   g_vb  = NULL; }
    if (g_dev) { g_dev->lpVtbl->Release(g_dev);  g_dev = NULL; }
    if (g_d3d) { g_d3d->lpVtbl->Release(g_d3d);  g_d3d = NULL; }
}

// -------------------------------------------------------
// main
// -------------------------------------------------------
int main()
{
    EnableDpiAwareness();

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"DX9ImGuiWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"DX9ImGuiWindow", L"TCC + cimgui + DirectX9",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) { printf("CreateWindow failed\n"); return 1; }

    if (!InitD3D(hwnd)) {
        printf("InitD3D failed\n");
        Cleanup();
        return 1;
    }

    InitImGui(hwnd);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    MSG msg;
    memset(&msg, 0, sizeof(msg));
    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Render(hwnd);
    }

    Cleanup();
    return 0;
}
