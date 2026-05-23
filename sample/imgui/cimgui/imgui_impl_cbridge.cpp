// imgui_impl_cbridge.cpp
//
// TCC（C言語）から呼び出せる extern "C" ラッパー。
// CBRIDGE_API を各関数に個別に付けることで確実にエクスポートされる。

#include "imgui/imgui.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <stdio.h>

#define CBRIDGE_API extern "C" __declspec(dllexport)

// -------------------------------------------------------
// Win32 共通
// -------------------------------------------------------
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern bool    ImGui_ImplWin32_Init(void* hwnd);
extern void    ImGui_ImplWin32_Shutdown();
extern void    ImGui_ImplWin32_NewFrame();

CBRIDGE_API int cbridge_ImGui_ImplWin32_Init(void* hwnd) {
    return (int)ImGui_ImplWin32_Init(hwnd);
}
CBRIDGE_API void cbridge_ImGui_ImplWin32_Shutdown(void) { ImGui_ImplWin32_Shutdown(); }
CBRIDGE_API void cbridge_ImGui_ImplWin32_NewFrame(void) { ImGui_ImplWin32_NewFrame(); }
CBRIDGE_API long long cbridge_ImGui_ImplWin32_WndProcHandler(
    void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam)
{
    return (long long)ImGui_ImplWin32_WndProcHandler(
        (HWND)hwnd, (UINT)msg, (WPARAM)wParam, (LPARAM)lParam);
}

// -------------------------------------------------------
// DirectX 9
// -------------------------------------------------------
#include "imgui/backends/imgui_impl_dx9.h"

CBRIDGE_API int  cbridge_ImGui_ImplDX9_Init(void* d) { return (int)ImGui_ImplDX9_Init((IDirect3DDevice9*)d); }
CBRIDGE_API void cbridge_ImGui_ImplDX9_Shutdown(void)  { ImGui_ImplDX9_Shutdown(); }
CBRIDGE_API void cbridge_ImGui_ImplDX9_NewFrame(void)  { ImGui_ImplDX9_NewFrame(); }
CBRIDGE_API void cbridge_ImGui_ImplDX9_RenderDrawData(void* d) { ImGui_ImplDX9_RenderDrawData((ImDrawData*)d); }
CBRIDGE_API int  cbridge_ImGui_ImplDX9_CreateDeviceObjects(void) { return (int)ImGui_ImplDX9_CreateDeviceObjects(); }
CBRIDGE_API void cbridge_ImGui_ImplDX9_InvalidateDeviceObjects(void) { ImGui_ImplDX9_InvalidateDeviceObjects(); }

// -------------------------------------------------------
// DirectX 10
// -------------------------------------------------------
#include "imgui/backends/imgui_impl_dx10.h"

CBRIDGE_API int  cbridge_ImGui_ImplDX10_Init(void* d) { return (int)ImGui_ImplDX10_Init((ID3D10Device*)d); }
CBRIDGE_API void cbridge_ImGui_ImplDX10_Shutdown(void)  { ImGui_ImplDX10_Shutdown(); }
CBRIDGE_API void cbridge_ImGui_ImplDX10_NewFrame(void)  { ImGui_ImplDX10_NewFrame(); }
CBRIDGE_API void cbridge_ImGui_ImplDX10_RenderDrawData(void* d) { ImGui_ImplDX10_RenderDrawData((ImDrawData*)d); }
CBRIDGE_API int  cbridge_ImGui_ImplDX10_CreateDeviceObjects(void) { return (int)ImGui_ImplDX10_CreateDeviceObjects(); }
CBRIDGE_API void cbridge_ImGui_ImplDX10_InvalidateDeviceObjects(void) { ImGui_ImplDX10_InvalidateDeviceObjects(); }

// -------------------------------------------------------
// DirectX 11
// -------------------------------------------------------
#include "imgui/backends/imgui_impl_dx11.h"

CBRIDGE_API int  cbridge_ImGui_ImplDX11_Init(void* dev, void* ctx) { return (int)ImGui_ImplDX11_Init((ID3D11Device*)dev, (ID3D11DeviceContext*)ctx); }
CBRIDGE_API void cbridge_ImGui_ImplDX11_Shutdown(void)  { ImGui_ImplDX11_Shutdown(); }
CBRIDGE_API void cbridge_ImGui_ImplDX11_NewFrame(void)  { ImGui_ImplDX11_NewFrame(); }
CBRIDGE_API void cbridge_ImGui_ImplDX11_RenderDrawData(void* d) { ImGui_ImplDX11_RenderDrawData((ImDrawData*)d); }
CBRIDGE_API int  cbridge_ImGui_ImplDX11_CreateDeviceObjects(void) { return (int)ImGui_ImplDX11_CreateDeviceObjects(); }
CBRIDGE_API void cbridge_ImGui_ImplDX11_InvalidateDeviceObjects(void) { ImGui_ImplDX11_InvalidateDeviceObjects(); }

// -------------------------------------------------------
// DirectX 12 imgui backend ラッパー（既存互換）
// -------------------------------------------------------
#include "imgui/backends/imgui_impl_dx12.h"

CBRIDGE_API int cbridge_ImGui_ImplDX12_Init(
    void* device, int num_frames_in_flight, unsigned int rtv_format,
    void* cbv_srv_heap, SIZE_T font_srv_cpu, unsigned long long font_srv_gpu)
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu; cpu.ptr = font_srv_cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu; gpu.ptr = font_srv_gpu;
    return (int)ImGui_ImplDX12_Init(
        (ID3D12Device*)device, num_frames_in_flight, (DXGI_FORMAT)rtv_format,
        (ID3D12DescriptorHeap*)cbv_srv_heap, cpu, gpu);
}
CBRIDGE_API void cbridge_ImGui_ImplDX12_Shutdown(void)  { ImGui_ImplDX12_Shutdown(); }
CBRIDGE_API void cbridge_ImGui_ImplDX12_NewFrame(void)  { ImGui_ImplDX12_NewFrame(); }
CBRIDGE_API void cbridge_ImGui_ImplDX12_RenderDrawData(void* d, void* cl) {
    ImGui_ImplDX12_RenderDrawData((ImDrawData*)d, (ID3D12GraphicsCommandList*)cl);
}
CBRIDGE_API int  cbridge_ImGui_ImplDX12_CreateDeviceObjects(void)  { return (int)ImGui_ImplDX12_CreateDeviceObjects(); }
CBRIDGE_API void cbridge_ImGui_ImplDX12_InvalidateDeviceObjects(void) { ImGui_ImplDX12_InvalidateDeviceObjects(); }

// -------------------------------------------------------
// OpenGL2
// -------------------------------------------------------
#include "imgui/backends/imgui_impl_opengl2.h"

CBRIDGE_API int  cbridge_ImGui_ImplOpenGL2_Init(void) { return (int)ImGui_ImplOpenGL2_Init(); }
CBRIDGE_API void cbridge_ImGui_ImplOpenGL2_Shutdown(void) { ImGui_ImplOpenGL2_Shutdown(); }
CBRIDGE_API void cbridge_ImGui_ImplOpenGL2_NewFrame(void) { ImGui_ImplOpenGL2_NewFrame(); }
CBRIDGE_API void cbridge_ImGui_ImplOpenGL2_RenderDrawData(void* d) { ImGui_ImplOpenGL2_RenderDrawData((ImDrawData*)d); }
CBRIDGE_API int  cbridge_ImGui_ImplOpenGL2_CreateDeviceObjects(void) { return (int)ImGui_ImplOpenGL2_CreateDeviceObjects(); }
CBRIDGE_API void cbridge_ImGui_ImplOpenGL2_DestroyDeviceObjects(void) { ImGui_ImplOpenGL2_DestroyDeviceObjects(); }

// ===============================================================
// DX12 完全管理バックエンド  cbridge_DX12_*
//
// 【設計方針】
//   TCC は MS x64 ABI の下記を正しく扱えない:
//     (A) 構造体値返し  GetCPUDescriptorHandleForHeapStart 等
//         → hidden pointer return (RCX=out ptr, RDX=this)
//         → TCC は RAX から読もうとしてスタック破壊
//     (B) 構造体値渡し  CreateRenderTargetView(handle_by_value) 等
//         → 1要素 struct は通常 register 渡しだが
//            COM vtbl 経由だと MSVC が特殊スタック配置する場合がある
//
//   解決策: D3D12 に触る操作を一切 C++ 側に閉じ込める。
//   TCC は cbridge_DX12_Init / Render / ResizeBuffers / Shutdown
//   の 4 関数だけ呼べばよい。引数はすべてスカラー。
// ===============================================================
// ===============================================================

#define MAX_BACK_BUFFERS     3
#define MAX_FRAMES_IN_FLIGHT 3

static ID3D12Device*              s_device     = nullptr;
static ID3D12CommandQueue*        s_cmdQueue   = nullptr;
static IDXGISwapChain3*           s_swapChain  = nullptr;
static ID3D12DescriptorHeap*      s_rtvHeap    = nullptr;
static ID3D12DescriptorHeap*      s_srvHeap    = nullptr;
static UINT                       s_srvDescSize = 0;
static UINT                       s_srvAllocIndex = 0;
static ID3D12GraphicsCommandList* s_cmdList    = nullptr;
static ID3D12Fence*               s_fence      = nullptr;
static HANDLE                     s_fenceEvent = nullptr;
static UINT64                     s_fenceValue = 0;
static UINT                       s_rtvDescSize    = 0;
static UINT                       s_numBackBuffers = 0;
static UINT                       s_numFrames      = 0;

struct FrameCtx {
    ID3D12CommandAllocator* allocator;
    UINT64                  fenceValue;
};
static FrameCtx s_frames[MAX_FRAMES_IN_FLIGHT] = {};
static UINT     s_frameIndex = 0;

static ID3D12Resource* s_rtvResource[MAX_BACK_BUFFERS] = {};

// RTV ハンドルは C++ 内部でのみ使う（TCC に渡さない）
static SIZE_T RtvPtr(UINT i) {
    D3D12_CPU_DESCRIPTOR_HANDLE base =
        s_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    return base.ptr + (SIZE_T)i * s_rtvDescSize;
}

static void WaitFenceValue(UINT64 value) {
    if (s_fence->GetCompletedValue() < value) {
        s_fence->SetEventOnCompletion(value, s_fenceEvent);
        WaitForSingleObject(s_fenceEvent, INFINITE);
    }
}

static void WaitAllFrames() {
    UINT64 v = ++s_fenceValue;
    s_cmdQueue->Signal(s_fence, v);
    WaitFenceValue(v);
}

static void CreateRTVs() {
    for (UINT i = 0; i < s_numBackBuffers; i++) {
        ID3D12Resource* buf = nullptr;
        HRESULT hr = s_swapChain->GetBuffer(i, IID_PPV_ARGS(&buf));
        if (FAILED(hr) || !buf) {
            printf("  [cbridge] CreateRTVs GetBuffer[%u] FAILED hr=0x%08X\n", i, hr);
            fflush(stdout);
            continue;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE h;
        h.ptr = RtvPtr(i);
        s_device->CreateRenderTargetView(buf, nullptr, h);
        s_rtvResource[i] = buf;
        printf("  [cbridge] CreateRTVs[%u] OK ptr=0x%llX\n", i, (unsigned long long)h.ptr);
        fflush(stdout);
    }
}

static void DestroyRTVs() {
    for (UINT i = 0; i < s_numBackBuffers; i++) {
        if (s_rtvResource[i]) { s_rtvResource[i]->Release(); s_rtvResource[i] = nullptr; }
    }
}

// -------------------------------------------------------
// cbridge_DX12_Init
//   D3D12 デバイス・スワップチェーン・ImGui バックエンドを
//   すべて C++ 側で初期化する。
//   TCC から一度だけ呼ぶ。
//   戻り値: 1=成功, 0=失敗
// -------------------------------------------------------
CBRIDGE_API int cbridge_DX12_Init(
    void* hwnd_void,
    int   numBackBuffers,
    int   numFramesInFlight)
{
    HWND hwnd = (HWND)hwnd_void;
    HRESULT hr;
    s_numBackBuffers = (UINT)numBackBuffers;
    s_numFrames      = (UINT)numFramesInFlight;

    printf("[cbridge_DX12_Init] D3D12CreateDevice\n"); fflush(stdout);
    hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&s_device));
    if (FAILED(hr)) { printf("  FAILED 0x%08X\n", hr); return 0; }
    printf("  device=%p\n", s_device); fflush(stdout);

    printf("[cbridge_DX12_Init] CreateCommandQueue\n"); fflush(stdout);
    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = s_device->CreateCommandQueue(&qd, IID_PPV_ARGS(&s_cmdQueue));
    if (FAILED(hr)) { printf("  FAILED 0x%08X\n", hr); return 0; }
    printf("  cmdQueue=%p\n", s_cmdQueue); fflush(stdout);

    printf("[cbridge_DX12_Init] CreateCommandAllocators\n"); fflush(stdout);
    for (UINT i = 0; i < s_numFrames; i++) {
        hr = s_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s_frames[i].allocator));
        if (FAILED(hr)) { printf("  FAILED[%u] 0x%08X\n", i, hr); return 0; }
        s_frames[i].fenceValue = 0;
        printf("  allocator[%u]=%p\n", i, s_frames[i].allocator); fflush(stdout);
    }

    printf("[cbridge_DX12_Init] CreateDescriptorHeap RTV\n"); fflush(stdout);
    {
        D3D12_DESCRIPTOR_HEAP_DESC d = {};
        d.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        d.NumDescriptors = s_numBackBuffers;
        hr = s_device->CreateDescriptorHeap(&d, IID_PPV_ARGS(&s_rtvHeap));
        if (FAILED(hr)) { printf("  FAILED 0x%08X\n", hr); return 0; }
    }
    s_rtvDescSize = s_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    printf("  rtvHeap=%p  rtvDescSize=%u\n", s_rtvHeap, s_rtvDescSize); fflush(stdout);

    printf("[cbridge_DX12_Init] CreateDescriptorHeap SRV\n"); fflush(stdout);
    {
        D3D12_DESCRIPTOR_HEAP_DESC d = {};
        d.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        d.NumDescriptors = 64;
        d.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = s_device->CreateDescriptorHeap(&d, IID_PPV_ARGS(&s_srvHeap));
        if (FAILED(hr)) { printf("  FAILED 0x%08X\n", hr); return 0; }
    }
    s_srvDescSize = s_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    printf("  srvHeap=%p  srvDescSize=%u\n", s_srvHeap, s_srvDescSize); fflush(stdout);

    printf("[cbridge_DX12_Init] CreateCommandList\n"); fflush(stdout);
    hr = s_device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        s_frames[0].allocator, nullptr, IID_PPV_ARGS(&s_cmdList));
    if (FAILED(hr)) { printf("  FAILED 0x%08X\n", hr); return 0; }
    s_cmdList->Close();

    printf("[cbridge_DX12_Init] CreateFence\n"); fflush(stdout);
    hr = s_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s_fence));
    if (FAILED(hr)) { printf("  FAILED 0x%08X\n", hr); return 0; }
    s_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    printf("[cbridge_DX12_Init] CreateSwapChain\n"); fflush(stdout);
    {
        IDXGIFactory4* factory = nullptr;
        hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
        if (FAILED(hr)) { printf("  CreateDXGIFactory2 FAILED 0x%08X\n", hr); return 0; }

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.BufferCount      = s_numBackBuffers;
        sd.Width            = 0;   // クライアントサイズ自動
        sd.Height           = 0;
        sd.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SwapEffect       = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        IDXGISwapChain1* sc1 = nullptr;
        hr = factory->CreateSwapChainForHwnd(s_cmdQueue, hwnd, &sd, nullptr, nullptr, &sc1);
        factory->Release();
        if (FAILED(hr) || !sc1) { printf("  CreateSwapChainForHwnd FAILED 0x%08X\n", hr); return 0; }

        hr = sc1->QueryInterface(IID_PPV_ARGS(&s_swapChain));
        sc1->Release();
        if (FAILED(hr)) { printf("  QueryInterface IDXGISwapChain3 FAILED 0x%08X\n", hr); return 0; }
        printf("  swapChain=%p\n", s_swapChain); fflush(stdout);
    }

    printf("[cbridge_DX12_Init] CreateRTVs\n"); fflush(stdout);
    CreateRTVs();

    // Use modern InitInfo so backend supports texture updates.
    printf("[cbridge_DX12_Init] ImGui_ImplDX12_Init (InitInfo)\n"); fflush(stdout);
    {
        // Simple SRV allocator using contiguous slots from s_srvHeap.
        auto SrvAlloc = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
        {
            SIZE_T offset = (SIZE_T)s_srvAllocIndex * (SIZE_T)s_srvDescSize;
            D3D12_CPU_DESCRIPTOR_HANDLE cpu = s_srvHeap->GetCPUDescriptorHandleForHeapStart(); cpu.ptr += offset;
            D3D12_GPU_DESCRIPTOR_HANDLE gpu = s_srvHeap->GetGPUDescriptorHandleForHeapStart(); gpu.ptr += (UINT64)offset;
            *out_cpu_desc_handle = cpu; *out_gpu_desc_handle = gpu;
            s_srvAllocIndex++;
        };
        auto SrvFree = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
        {
            // no-op for now
        };

        ImGui_ImplDX12_InitInfo init_info;
        memset(&init_info, 0, sizeof(init_info));
        init_info.Device = s_device;
        init_info.CommandQueue = s_cmdQueue;
        init_info.NumFramesInFlight = (int)s_numFrames;
        init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
        init_info.SrvDescriptorHeap = s_srvHeap;
        init_info.SrvDescriptorAllocFn = (void(*)(ImGui_ImplDX12_InitInfo*,D3D12_CPU_DESCRIPTOR_HANDLE*,D3D12_GPU_DESCRIPTOR_HANDLE*))SrvAlloc;
        init_info.SrvDescriptorFreeFn = (void(*)(ImGui_ImplDX12_InitInfo*,D3D12_CPU_DESCRIPTOR_HANDLE,D3D12_GPU_DESCRIPTOR_HANDLE))SrvFree;

        if (!ImGui_ImplDX12_Init(&init_info))
        {
            printf("  ImGui_ImplDX12_Init FAILED\n"); fflush(stdout);
            return 0;
        }
    }

    printf("[cbridge_DX12_Init] SUCCESS\n"); fflush(stdout);
    return 1;
}

// -------------------------------------------------------
// cbridge_DX12_Shutdown
// -------------------------------------------------------
CBRIDGE_API void cbridge_DX12_Shutdown(void) {
    WaitAllFrames();
    ImGui_ImplDX12_Shutdown();
    DestroyRTVs();
    if (s_swapChain)  { s_swapChain->Release();   s_swapChain  = nullptr; }
    if (s_cmdList)    { s_cmdList->Release();      s_cmdList    = nullptr; }
    if (s_rtvHeap)    { s_rtvHeap->Release();      s_rtvHeap    = nullptr; }
    if (s_srvHeap)    { s_srvHeap->Release();      s_srvHeap    = nullptr; }
    for (UINT i = 0; i < s_numFrames; i++) {
        if (s_frames[i].allocator) {
            s_frames[i].allocator->Release();
            s_frames[i].allocator = nullptr;
        }
    }
    if (s_fence)      { s_fence->Release();        s_fence      = nullptr; }
    if (s_fenceEvent) { CloseHandle(s_fenceEvent); s_fenceEvent = nullptr; }
    if (s_cmdQueue)   { s_cmdQueue->Release();     s_cmdQueue   = nullptr; }
    if (s_device)     { s_device->Release();       s_device     = nullptr; }
}

// -------------------------------------------------------
// cbridge_DX12_ResizeBuffers
//   WM_SIZE 時に TCC から呼ぶ
// -------------------------------------------------------
CBRIDGE_API void cbridge_DX12_ResizeBuffers(unsigned int w, unsigned int h) {
    WaitAllFrames();
    DestroyRTVs();
    s_swapChain->ResizeBuffers(s_numBackBuffers, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    CreateRTVs();
}

// -------------------------------------------------------
// cbridge_DX12_Render
//   TCC 側で igRender() を呼んだ直後にこれを呼ぶ。
//   バリア・クリア・ImGui描画・Present・フェンス信号を一括処理。
//   clear_color: float[4] (RGBA) へのポインタ
// -------------------------------------------------------
CBRIDGE_API void cbridge_DX12_NewFrame(void) {
    ImGui_ImplDX12_NewFrame();
}

CBRIDGE_API void cbridge_DX12_Render(const float* clear_color) {
    s_frameIndex++;
    FrameCtx* fc = &s_frames[s_frameIndex % s_numFrames];
    WaitFenceValue(fc->fenceValue);

    UINT bbIdx = s_swapChain->GetCurrentBackBufferIndex();

    fc->allocator->Reset();
    s_cmdList->Reset(fc->allocator, nullptr);

    // Barrier: Present → RenderTarget
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = s_rtvResource[bbIdx];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    s_cmdList->ResourceBarrier(1, &barrier);

    // Clear & OMSetRenderTargets
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    rtv.ptr = RtvPtr(bbIdx);
    s_cmdList->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
    s_cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    // ImGui DrawData
    ID3D12DescriptorHeap* heaps[] = { s_srvHeap };
    s_cmdList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), s_cmdList);

    // Barrier: RenderTarget → Present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    s_cmdList->ResourceBarrier(1, &barrier);
    s_cmdList->Close();

    ID3D12CommandList* cls[] = { s_cmdList };
    s_cmdQueue->ExecuteCommandLists(1, cls);
    s_swapChain->Present(1, 0);

    UINT64 fv = ++s_fenceValue;
    s_cmdQueue->Signal(s_fence, fv);
    fc->fenceValue = fv;
}
