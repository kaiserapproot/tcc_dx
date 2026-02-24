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
extern int       cbridge_ImGui_ImplDX11_Init(void* device, void* context);
extern void      cbridge_ImGui_ImplDX11_Shutdown(void);
extern void      cbridge_ImGui_ImplDX11_NewFrame(void);
extern void      cbridge_ImGui_ImplDX11_RenderDrawData(void* draw_data);
extern int       cbridge_ImGui_ImplWin32_Init(void* hwnd);
extern void      cbridge_ImGui_ImplWin32_Shutdown(void);
extern void      cbridge_ImGui_ImplWin32_NewFrame(void);
extern long long cbridge_ImGui_ImplWin32_WndProcHandler(void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam);

// -------------------------------------------------------
// D3D11 定数
// -------------------------------------------------------
#define DXGI_SWAP_EFFECT_FLIP_DISCARD         4
#define D3D11_USAGE_DEFAULT                   0
#define D3D11_BIND_VERTEX_BUFFER              0x0001
#define D3D_DRIVER_TYPE_HARDWARE              1
#define DXGI_FORMAT_R8G8B8A8_UNORM            28
#define DXGI_FORMAT_R32G32B32_FLOAT           6
#define DXGI_FORMAT_R32G32B32A32_FLOAT        2
#define D3D11_SDK_VERSION                     7
#define D3D11_INPUT_PER_VERTEX_DATA           0
#define D3D11_APPEND_ALIGNED_ELEMENT          0xffffffff
#define D3DCOMPILE_DEBUG                      1
#define D3DCOMPILE_SKIP_OPTIMIZATION          4
#define D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST 4

// -------------------------------------------------------
// D3D11 構造体・列挙体
// -------------------------------------------------------
typedef enum D3D_FEATURE_LEVEL {
    D3D_FEATURE_LEVEL_9_1  = 0x9100,
    D3D_FEATURE_LEVEL_11_0 = 0xb000,
    D3D_FEATURE_LEVEL_11_1 = 0xb100
} D3D_FEATURE_LEVEL;

typedef enum D3D_COUNTER_TYPE { D3D_COUNTER_GPU_IDLE = 0 } D3D_COUNTER_TYPE;

typedef enum D3D11_MAP {
    D3D11_MAP_READ               = 1,
    D3D11_MAP_WRITE              = 2,
    D3D11_MAP_READ_WRITE         = 3,
    D3D11_MAP_WRITE_DISCARD      = 4,
    D3D11_MAP_WRITE_NO_OVERWRITE = 5
} D3D11_MAP;

typedef enum D3D11_DEVICE_CONTEXT_TYPE {
    D3D11_DEVICE_CONTEXT_IMMEDIATE = 0,
    D3D11_DEVICE_CONTEXT_DEFERRED  = 1
} D3D11_DEVICE_CONTEXT_TYPE;

typedef struct DXGI_MODE_DESC {
    UINT Width; UINT Height;
    struct { UINT Numerator; UINT Denominator; } RefreshRate;
    UINT Format; UINT ScanlineOrdering; UINT Scaling;
} DXGI_MODE_DESC;

typedef struct DXGI_SAMPLE_DESC { UINT Count; UINT Quality; } DXGI_SAMPLE_DESC;

typedef struct DXGI_SWAP_CHAIN_DESC {
    DXGI_MODE_DESC   BufferDesc;
    DXGI_SAMPLE_DESC SampleDesc;
    UINT BufferUsage; UINT BufferCount;
    HWND OutputWindow; BOOL Windowed;
    UINT SwapEffect;   UINT Flags;
} DXGI_SWAP_CHAIN_DESC;

typedef struct D3D11_BUFFER_DESC {
    UINT ByteWidth; UINT Usage; UINT BindFlags;
    UINT CPUAccessFlags; UINT MiscFlags; UINT StructureByteStride;
} D3D11_BUFFER_DESC;

typedef struct D3D11_SUBRESOURCE_DATA {
    const void* pSysMem; UINT SysMemPitch; UINT SysMemSlicePitch;
} D3D11_SUBRESOURCE_DATA;

typedef struct D3D11_INPUT_ELEMENT_DESC {
    const char* SemanticName; UINT SemanticIndex; UINT Format;
    UINT InputSlot; UINT AlignedByteOffset;
    UINT InputSlotClass; UINT InstanceDataStepRate;
} D3D11_INPUT_ELEMENT_DESC;

typedef struct D3D11_VIEWPORT {
    float TopLeftX; float TopLeftY;
    float Width;    float Height;
    float MinDepth; float MaxDepth;
} D3D11_VIEWPORT;

typedef struct { float x, y, z; float r, g, b, a; } Vertex;

// -------------------------------------------------------
// ID3DBlob
// -------------------------------------------------------
typedef struct ID3DBlobVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
    void*  (__stdcall* GetBufferPointer)(void*);
    SIZE_T (__stdcall* GetBufferSize)(void*);
} ID3DBlobVtbl;
typedef struct ID3DBlob { ID3DBlobVtbl* lpVtbl; } ID3DBlob;

// -------------------------------------------------------
// ID3D11Device
// -------------------------------------------------------
typedef struct ID3D11DeviceVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
    HRESULT(__stdcall* CreateBuffer)(void*, const D3D11_BUFFER_DESC*, const D3D11_SUBRESOURCE_DATA*, void**);
    HRESULT(__stdcall* CreateTexture1D)(void*, const void*, const D3D11_SUBRESOURCE_DATA*, void**);
    HRESULT(__stdcall* CreateTexture2D)(void*, const void*, const D3D11_SUBRESOURCE_DATA*, void**);
    HRESULT(__stdcall* CreateTexture3D)(void*, const void*, const D3D11_SUBRESOURCE_DATA*, void**);
    HRESULT(__stdcall* CreateShaderResourceView)(void*, void*, const void*, void**);
    HRESULT(__stdcall* CreateUnorderedAccessView)(void*, void*, const void*, void**);
    HRESULT(__stdcall* CreateRenderTargetView)(void*, void*, const void*, void**);
    HRESULT(__stdcall* CreateDepthStencilView)(void*, void*, const void*, void**);
    HRESULT(__stdcall* CreateInputLayout)(void*, const D3D11_INPUT_ELEMENT_DESC*, UINT, const void*, SIZE_T, void**);
    HRESULT(__stdcall* CreateVertexShader)(void*, const void*, SIZE_T, void*, void**);
    HRESULT(__stdcall* CreateGeometryShader)(void*, const void*, SIZE_T, void*, void**);
    HRESULT(__stdcall* CreateGeometryShaderWithStreamOutput)(void*, const void*, SIZE_T, const void*, UINT, const UINT*, UINT, UINT, void*, void**);
    HRESULT(__stdcall* CreatePixelShader)(void*, const void*, SIZE_T, void*, void**);
    HRESULT(__stdcall* CreateHullShader)(void*, const void*, SIZE_T, void*, void**);
    HRESULT(__stdcall* CreateDomainShader)(void*, const void*, SIZE_T, void*, void**);
    HRESULT(__stdcall* CreateComputeShader)(void*, const void*, SIZE_T, void*, void**);
    HRESULT(__stdcall* CreateClassLinkage)(void*, void**);
    HRESULT(__stdcall* CreateBlendState)(void*, const void*, void**);
    HRESULT(__stdcall* CreateDepthStencilState)(void*, const void*, void**);
    HRESULT(__stdcall* CreateRasterizerState)(void*, const void*, void**);
    HRESULT(__stdcall* CreateSamplerState)(void*, const void*, void**);
    HRESULT(__stdcall* CreateQuery)(void*, const void*, void**);
    HRESULT(__stdcall* CreatePredicate)(void*, const void*, void**);
    HRESULT(__stdcall* CreateCounter)(void*, const void*, void**);
    HRESULT(__stdcall* CreateDeferredContext)(void*, UINT, void**);
    HRESULT(__stdcall* OpenSharedResource)(void*, HANDLE, const void*, void**);
    HRESULT(__stdcall* CheckFormatSupport)(void*, UINT, UINT*);
    HRESULT(__stdcall* CheckMultisampleQualityLevels)(void*, UINT, UINT, UINT*);
    void   (__stdcall* CheckCounterInfo)(void*, void*);
    HRESULT(__stdcall* CheckCounter)(void*, const void*, D3D_COUNTER_TYPE*, UINT*, char*, UINT*, char*, UINT*, wchar_t*, UINT*);
    HRESULT(__stdcall* CheckFeatureSupport)(void*, UINT, void*, UINT);
    HRESULT(__stdcall* GetPrivateData)(void*, const void*, UINT*, void*);
    HRESULT(__stdcall* SetPrivateData)(void*, const void*, UINT, const void*);
    HRESULT(__stdcall* SetPrivateDataInterface)(void*, const void*, const void*);
    D3D_FEATURE_LEVEL(__stdcall* GetFeatureLevel)(void*);
    UINT   (__stdcall* GetCreationFlags)(void*);
    HRESULT(__stdcall* GetDeviceRemovedReason)(void*);
    void   (__stdcall* GetImmediateContext)(void*, void**);
    HRESULT(__stdcall* SetExceptionMode)(void*, UINT);
    UINT   (__stdcall* GetExceptionMode)(void*);
} ID3D11DeviceVtbl;
typedef struct ID3D11Device { ID3D11DeviceVtbl* lpVtbl; } ID3D11Device;

// -------------------------------------------------------
// ID3D11DeviceContext
// -------------------------------------------------------
typedef struct ID3D11DeviceContextVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
    void   (__stdcall* GetDevice)(void*, void**);
    HRESULT(__stdcall* GetPrivateData)(void*, const void*, UINT*, void*);
    HRESULT(__stdcall* SetPrivateData)(void*, const void*, UINT, const void*);
    HRESULT(__stdcall* SetPrivateDataInterface)(void*, const void*, const void*);
    void   (__stdcall* VSSetConstantBuffers)(void*, UINT, UINT, void* const*);
    void   (__stdcall* PSSetShaderResources)(void*, UINT, UINT, void* const*);
    void   (__stdcall* PSSetShader)(void*, void*, void* const*, UINT);
    void   (__stdcall* PSSetSamplers)(void*, UINT, UINT, void* const*);
    void   (__stdcall* VSSetShader)(void*, void*, void* const*, UINT);
    void   (__stdcall* DrawIndexed)(void*, UINT, UINT, INT);
    void   (__stdcall* Draw)(void*, UINT, UINT);
    HRESULT(__stdcall* Map)(void*, void*, UINT, D3D11_MAP, UINT, void*);
    void   (__stdcall* Unmap)(void*, void*, UINT);
    void   (__stdcall* PSSetConstantBuffers)(void*, UINT, UINT, void* const*);
    void   (__stdcall* IASetInputLayout)(void*, void*);
    void   (__stdcall* IASetVertexBuffers)(void*, UINT, UINT, void* const*, const UINT*, const UINT*);
    void   (__stdcall* IASetIndexBuffer)(void*, void*, UINT, UINT);
    void   (__stdcall* DrawIndexedInstanced)(void*, UINT, UINT, UINT, INT, UINT);
    void   (__stdcall* DrawInstanced)(void*, UINT, UINT, UINT, UINT);
    void   (__stdcall* GSSetConstantBuffers)(void*, UINT, UINT, void* const*);
    void   (__stdcall* GSSetShader)(void*, void*, void* const*, UINT);
    void   (__stdcall* IASetPrimitiveTopology)(void*, int);
    void   (__stdcall* VSSetShaderResources)(void*, UINT, UINT, void* const*);
    void   (__stdcall* VSSetSamplers)(void*, UINT, UINT, void* const*);
    void   (__stdcall* Begin)(void*, void*);
    void   (__stdcall* End)(void*, void*);
    HRESULT(__stdcall* GetData)(void*, void*, void*, UINT, UINT);
    void   (__stdcall* SetPredication)(void*, void*, BOOL);
    void   (__stdcall* GSSetShaderResources)(void*, UINT, UINT, void* const*);
    void   (__stdcall* GSSetSamplers)(void*, UINT, UINT, void* const*);
    void   (__stdcall* OMSetRenderTargets)(void*, UINT, void* const*, void*);
    void   (__stdcall* OMSetRenderTargetsAndUnorderedAccessViews)(void*, UINT, void* const*, void*, UINT, UINT, void* const*, const UINT*);
    void   (__stdcall* OMSetBlendState)(void*, void*, const float[4], UINT);
    void   (__stdcall* OMSetDepthStencilState)(void*, void*, UINT);
    void   (__stdcall* SOSetTargets)(void*, UINT, void* const*, const UINT*);
    void   (__stdcall* DrawAuto)(void*);
    void   (__stdcall* DrawIndexedInstancedIndirect)(void*, void*, UINT);
    void   (__stdcall* DrawInstancedIndirect)(void*, void*, UINT);
    void   (__stdcall* Dispatch)(void*, UINT, UINT, UINT);
    void   (__stdcall* DispatchIndirect)(void*, void*, UINT);
    void   (__stdcall* RSSetState)(void*, void*);
    void   (__stdcall* RSSetViewports)(void*, UINT, const D3D11_VIEWPORT*);
    void   (__stdcall* RSSetScissorRects)(void*, UINT, const void*);
    void   (__stdcall* CopySubresourceRegion)(void*, void*, UINT, UINT, UINT, UINT, void*, UINT, const void*);
    void   (__stdcall* CopyResource)(void*, void*, void*);
    void   (__stdcall* UpdateSubresource)(void*, void*, UINT, const void*, const void*, UINT, UINT);
    void   (__stdcall* CopyStructureCount)(void*, void*, UINT, void*);
    void   (__stdcall* ClearRenderTargetView)(void*, void*, const float[4]);
} ID3D11DeviceContextVtbl;
typedef struct ID3D11DeviceContext { ID3D11DeviceContextVtbl* lpVtbl; } ID3D11DeviceContext;

// -------------------------------------------------------
// IDXGISwapChain
// -------------------------------------------------------
typedef struct IDXGISwapChainVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
    HRESULT(__stdcall* SetPrivateData)(void*, const void*, UINT, const void*);
    HRESULT(__stdcall* SetPrivateDataInterface)(void*, const void*, const void*);
    HRESULT(__stdcall* GetPrivateData)(void*, const void*, UINT*, void*);
    HRESULT(__stdcall* GetParent)(void*, const void*, void**);
    HRESULT(__stdcall* GetDevice)(void*, const void*, void**);
    HRESULT(__stdcall* Present)(void*, UINT, UINT);
    HRESULT(__stdcall* GetBuffer)(void*, UINT, const void*, void**);
} IDXGISwapChainVtbl;
typedef struct IDXGISwapChain { IDXGISwapChainVtbl* lpVtbl; } IDXGISwapChain;

// -------------------------------------------------------
// ID3D11RenderTargetView
// -------------------------------------------------------
typedef struct ID3D11RenderTargetViewVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
} ID3D11RenderTargetViewVtbl;
typedef struct ID3D11RenderTargetView { ID3D11RenderTargetViewVtbl* lpVtbl; } ID3D11RenderTargetView;

// -------------------------------------------------------
// ID3D11Buffer
// -------------------------------------------------------
typedef struct ID3D11BufferVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
} ID3D11BufferVtbl;
typedef struct ID3D11Buffer { ID3D11BufferVtbl* lpVtbl; } ID3D11Buffer;

// -------------------------------------------------------
// ID3D11VertexShader / PixelShader / InputLayout
// -------------------------------------------------------
typedef struct ID3D11VertexShaderVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
} ID3D11VertexShaderVtbl;
typedef struct ID3D11VertexShader { ID3D11VertexShaderVtbl* lpVtbl; } ID3D11VertexShader;

typedef struct ID3D11PixelShaderVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
} ID3D11PixelShaderVtbl;
typedef struct ID3D11PixelShader { ID3D11PixelShaderVtbl* lpVtbl; } ID3D11PixelShader;

typedef struct ID3D11InputLayoutVtbl {
    HRESULT(__stdcall* QueryInterface)(void*, const void*, void**);
    ULONG  (__stdcall* AddRef)(void*);
    ULONG  (__stdcall* Release)(void*);
} ID3D11InputLayoutVtbl;
typedef struct ID3D11InputLayout { ID3D11InputLayoutVtbl* lpVtbl; } ID3D11InputLayout;

typedef struct ID3D11Texture2D { void* lpVtbl; } ID3D11Texture2D;

// -------------------------------------------------------
// D3D11 関数宣言
// -------------------------------------------------------
HRESULT __stdcall D3D11CreateDeviceAndSwapChain(
    void*, int, HMODULE, UINT, const void*, UINT, UINT,
    const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**,
    void*, ID3D11DeviceContext**);

typedef HRESULT(__stdcall *PFN_D3DCompile)(
    const void*, SIZE_T, const char*, const void*, void*, const char*,
    const char*, UINT, UINT, void**, void**);

static PFN_D3DCompile g_pD3DCompile = NULL;

// -------------------------------------------------------
// グローバル変数
// -------------------------------------------------------
static IDXGISwapChain*         g_swapchain    = NULL;
static ID3D11Device*           g_dev          = NULL;
static ID3D11DeviceContext*    g_ctx          = NULL;
static ID3D11RenderTargetView* g_backbuffer   = NULL;
static ID3D11Buffer*           g_vertexbuffer = NULL;
static ID3D11VertexShader*     g_vertexshader = NULL;
static ID3D11PixelShader*      g_pixelshader  = NULL;
static ID3D11InputLayout*      g_layout       = NULL;

static int   g_show_demo      = 1;
static int   g_show_my_window = 1;
static float g_clear_color[3] = { 0.0f, 0.2f, 0.4f };
static float g_counter        = 0.0f;

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
    // Windows 8.1 以降
    HMODULE shcore = LoadLibraryA("shcore.dll");
    if (shcore) {
        PFN_SetProcessDpiAwareness fn =
            (PFN_SetProcessDpiAwareness)GetProcAddress(shcore, "SetProcessDpiAwareness");
        if (fn) { fn(PROCESS_PER_MONITOR_DPI_AWARE); return; }
    }
    // フォールバック: Windows Vista 以降
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        BOOL(__stdcall* fn2)(void) =
            (BOOL(__stdcall*)(void))GetProcAddress(user32, "SetProcessDPIAware");
        if (fn2) fn2();
    }
}

// -------------------------------------------------------
// D3DCompiler ロード
// -------------------------------------------------------
static BOOL LoadD3DCompiler()
{
    if (g_pD3DCompile) return TRUE;
    const char* dlls[] = {
        "d3dcompiler_47.dll", "D3DCompiler_47.dll",
        "D3DCompiler_46.dll", "D3DCompiler_43.dll"
    };
    for (int i = 0; i < 4; i++) {
        HMODULE h = LoadLibraryA(dlls[i]);
        if (h) {
            g_pD3DCompile = (PFN_D3DCompile)GetProcAddress(h, "D3DCompile");
            if (g_pD3DCompile) return TRUE;
        }
    }
    return FALSE;
}

// -------------------------------------------------------
// WndProc
// -------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (cbridge_ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return TRUE;
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// -------------------------------------------------------
// D3D11 初期化
// -------------------------------------------------------
BOOL InitD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC scd;
    memset(&scd, 0, sizeof(scd));
    scd.BufferDesc.Width                   = 0;
    scd.BufferDesc.Height                  = 0;
    scd.BufferDesc.RefreshRate.Numerator   = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.SampleDesc.Count                   = 1;
    scd.SampleDesc.Quality                 = 0;
    scd.BufferUsage                        = 0x20;
    scd.BufferCount                        = 2;
    scd.OutputWindow                       = hwnd;
    scd.Windowed                           = TRUE;
    scd.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        NULL, 0, D3D11_SDK_VERSION,
        &scd, &g_swapchain, &g_dev, NULL, &g_ctx);
    if (hr != 0) { printf("D3D11CreateDeviceAndSwapChain failed: 0x%08X\n", hr); return FALSE; }

    // バックバッファ
    ID3D11Texture2D* backbuffer = NULL;
    static const GUID IID_ID3D11Texture2D =
        { 0x6f15aaf2, 0xd208, 0x4e89, {0x9a,0xb4,0x48,0x95,0x35,0xd3,0x4f,0x9c} };
    hr = g_swapchain->lpVtbl->GetBuffer(g_swapchain, 0, &IID_ID3D11Texture2D, (void**)&backbuffer);
    if (hr != 0) { printf("GetBuffer failed: 0x%08X\n", hr); return FALSE; }

    hr = g_dev->lpVtbl->CreateRenderTargetView(g_dev, (void*)backbuffer, NULL, (void**)&g_backbuffer);
    if (hr != 0) { printf("CreateRenderTargetView failed: 0x%08X\n", hr); return FALSE; }

    // 頂点バッファ
    Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f },
        { 0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f },
        {-0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f }
    };
    D3D11_BUFFER_DESC bd;
    memset(&bd, 0, sizeof(bd));
    bd.ByteWidth = sizeof(vertices);
    bd.Usage     = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initData;
    memset(&initData, 0, sizeof(initData));
    initData.pSysMem = vertices;
    hr = g_dev->lpVtbl->CreateBuffer(g_dev, &bd, &initData, (void**)&g_vertexbuffer);
    if (hr != 0) { printf("CreateBuffer failed: 0x%08X\n", hr); return FALSE; }

    // シェーダー
    if (!LoadD3DCompiler()) { printf("D3DCompiler not found\n"); return FALSE; }

    const char* vsSrc =
        "struct VSIn  { float3 pos: POSITION; float4 col: COLOR; };\n"
        "struct VSOut { float4 pos: SV_Position; float4 col: COLOR; };\n"
        "VSOut main(VSIn i) { VSOut o; o.pos=float4(i.pos,1.0); o.col=i.col; return o; }\n";
    const char* psSrc =
        "struct PSIn { float4 pos: SV_Position; float4 col: COLOR; };\n"
        "float4 main(PSIn i) : SV_Target { return i.col; }\n";

    ID3DBlob *vsBlob=NULL, *psBlob=NULL, *errBlob=NULL;

    hr = g_pD3DCompile(vsSrc, strlen(vsSrc), "vs.hlsl", NULL, NULL, "main", "vs_4_0",
        D3DCOMPILE_DEBUG|D3DCOMPILE_SKIP_OPTIMIZATION, 0, (void**)&vsBlob, (void**)&errBlob);
    if (hr != 0) {
        const char* msg = errBlob ? (const char*)errBlob->lpVtbl->GetBufferPointer(errBlob) : "(none)";
        printf("VS compile failed: %s\n", msg);
        if (errBlob) errBlob->lpVtbl->Release(errBlob);
        return FALSE;
    }
    if (errBlob) { errBlob->lpVtbl->Release(errBlob); errBlob = NULL; }

    hr = g_pD3DCompile(psSrc, strlen(psSrc), "ps.hlsl", NULL, NULL, "main", "ps_4_0",
        D3DCOMPILE_DEBUG|D3DCOMPILE_SKIP_OPTIMIZATION, 0, (void**)&psBlob, (void**)&errBlob);
    if (hr != 0) {
        const char* msg = errBlob ? (const char*)errBlob->lpVtbl->GetBufferPointer(errBlob) : "(none)";
        printf("PS compile failed: %s\n", msg);
        if (errBlob) errBlob->lpVtbl->Release(errBlob);
        vsBlob->lpVtbl->Release(vsBlob);
        return FALSE;
    }
    if (errBlob) { errBlob->lpVtbl->Release(errBlob); errBlob = NULL; }

    hr = g_dev->lpVtbl->CreateVertexShader(g_dev,
        vsBlob->lpVtbl->GetBufferPointer(vsBlob),
        vsBlob->lpVtbl->GetBufferSize(vsBlob), NULL, (void**)&g_vertexshader);
    if (hr != 0) { printf("CreateVertexShader failed\n"); return FALSE; }

    hr = g_dev->lpVtbl->CreatePixelShader(g_dev,
        psBlob->lpVtbl->GetBufferPointer(psBlob),
        psBlob->lpVtbl->GetBufferSize(psBlob), NULL, (void**)&g_pixelshader);
    if (hr != 0) { printf("CreatePixelShader failed\n"); return FALSE; }

    D3D11_INPUT_ELEMENT_DESC layout[2];
    memset(layout, 0, sizeof(layout));
    layout[0].SemanticName      = "POSITION";
    layout[0].Format            = DXGI_FORMAT_R32G32B32_FLOAT;
    layout[0].AlignedByteOffset = 0;
    layout[0].InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;
    layout[1].SemanticName      = "COLOR";
    layout[1].Format            = DXGI_FORMAT_R32G32B32A32_FLOAT;
    layout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    layout[1].InputSlotClass    = D3D11_INPUT_PER_VERTEX_DATA;

    hr = g_dev->lpVtbl->CreateInputLayout(g_dev, layout, 2,
        vsBlob->lpVtbl->GetBufferPointer(vsBlob),
        vsBlob->lpVtbl->GetBufferSize(vsBlob), (void**)&g_layout);
    vsBlob->lpVtbl->Release(vsBlob);
    psBlob->lpVtbl->Release(psBlob);
    if (hr != 0) { printf("CreateInputLayout failed\n"); return FALSE; }

    return TRUE;
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
    cbridge_ImGui_ImplDX11_Init(g_dev, g_ctx);
}

// -------------------------------------------------------
// レンダリング（hwnd を受け取りクライアントサイズを毎フレーム取得）
// -------------------------------------------------------
void Render(HWND hwnd)
{
    RECT rect;
GetClientRect(hwnd, &rect);
float width  = (float)(rect.right  - rect.left);
float height = (float)(rect.bottom - rect.top);
printf("ClientRect: %.0f x %.0f\n", width, height);

// ImGuiが認識しているサイズも確認
ImGuiIO* io = igGetIO_Nil();
printf("ImGui DisplaySize: %.0f x %.0f\n", io->DisplaySize.x, io->DisplaySize.y);
    // クライアント領域のサイズを毎フレーム取得（タイトルバー・ボーダーを除いた純粋な描画領域）
   // RECT rect;
    GetClientRect(hwnd, &rect);
     width  = (float)(rect.right  - rect.left);
     height = (float)(rect.bottom - rect.top);

    float clearColor[4] = { g_clear_color[0], g_clear_color[1], g_clear_color[2], 1.0f };
    g_ctx->lpVtbl->ClearRenderTargetView(g_ctx, (void*)g_backbuffer, clearColor);
    g_ctx->lpVtbl->OMSetRenderTargets(g_ctx, 1, (void* const*)&g_backbuffer, NULL);

    // ビューポートをクライアントサイズに合わせる
    D3D11_VIEWPORT vp;
    memset(&vp, 0, sizeof(vp));
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width    = width;
    vp.Height   = height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_ctx->lpVtbl->RSSetViewports(g_ctx, 1, &vp);

    // 三角形描画
    if (g_layout && g_vertexbuffer && g_vertexshader && g_pixelshader)
    {
        g_ctx->lpVtbl->IASetInputLayout(g_ctx, (void*)g_layout);
        UINT stride = sizeof(Vertex), offset = 0;
        g_ctx->lpVtbl->IASetVertexBuffers(g_ctx, 0, 1, (void* const*)&g_vertexbuffer, &stride, &offset);
        g_ctx->lpVtbl->IASetPrimitiveTopology(g_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_ctx->lpVtbl->VSSetShader(g_ctx, (void*)g_vertexshader, NULL, 0);
        g_ctx->lpVtbl->PSSetShader(g_ctx, (void*)g_pixelshader,  NULL, 0);
        g_ctx->lpVtbl->Draw(g_ctx, 3, 0);
    }

    // ImGui フレーム開始
    cbridge_ImGui_ImplDX11_NewFrame();
    cbridge_ImGui_ImplWin32_NewFrame();
    igNewFrame();

    // デモウィンドウ
    if (g_show_demo)
        igShowDemoWindow(&g_show_demo);

    // 独自ウィンドウ
    if (g_show_my_window)
    {
        igBegin("TCC + cimgui + DX11", &g_show_my_window, 0);

        igText("Hello from TCC!");
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
    cbridge_ImGui_ImplDX11_RenderDrawData(igGetDrawData());

    g_swapchain->lpVtbl->Present(g_swapchain, 1, 0);
}

// -------------------------------------------------------
// クリーンアップ
// -------------------------------------------------------
void Cleanup()
{
    cbridge_ImGui_ImplDX11_Shutdown();
    cbridge_ImGui_ImplWin32_Shutdown();
    igDestroyContext(NULL);

    if (g_layout)       g_layout->lpVtbl->Release((void*)g_layout);
    if (g_pixelshader)  g_pixelshader->lpVtbl->Release((void*)g_pixelshader);
    if (g_vertexshader) g_vertexshader->lpVtbl->Release((void*)g_vertexshader);
    if (g_vertexbuffer) g_vertexbuffer->lpVtbl->Release((void*)g_vertexbuffer);
    if (g_backbuffer)   g_backbuffer->lpVtbl->Release((void*)g_backbuffer);
    if (g_ctx)          g_ctx->lpVtbl->Release((void*)g_ctx);
    if (g_swapchain)    g_swapchain->lpVtbl->Release((void*)g_swapchain);
    if (g_dev)          g_dev->lpVtbl->Release((void*)g_dev);
}

// -------------------------------------------------------
// main
// -------------------------------------------------------
int main()
{
    // DPI 対応を最初に設定（ウィンドウ作成前に必ず呼ぶ）
    EnableDpiAwareness();

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"DX11ImGuiWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"DX11ImGuiWindow", L"TCC + cimgui + DirectX11",
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
        Render(hwnd);  // hwnd を渡してクライアントサイズを取得
    }

    Cleanup();
    return 0;
}