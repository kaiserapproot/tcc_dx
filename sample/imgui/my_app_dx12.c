// my_app_dx12.c
// TCC + cimgui + DirectX12
//
// D3D12 の初期化・レンダリング・リサイズは全て C++ 側（cbridge_DX12_*）が行う。
// TCC は ImGui の UI ロジックだけを担当し、D3D12 COM を一切触らない。

#include <windows.h>
#include <stdio.h>
#include <string.h>

#ifndef __bool_true_false_are_defined
typedef int bool;
#define true  1
#define false 0
#define __bool_true_false_are_defined 1
#endif

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

// -------------------------------------------------------
// extern 宣言：Win32 ラッパー
// -------------------------------------------------------
extern int       cbridge_ImGui_ImplWin32_Init(void* hwnd);
extern void      cbridge_ImGui_ImplWin32_Shutdown(void);
extern void      cbridge_ImGui_ImplWin32_NewFrame(void);
extern long long cbridge_ImGui_ImplWin32_WndProcHandler(
                     void* hwnd, unsigned int msg,
                     unsigned long long wParam, long long lParam);

// -------------------------------------------------------
// extern 宣言：DX12 完全管理バックエンド
//
// 【設計】
//   TCC は MS x64 ABI の「構造体値返し／値渡し」を
//   正しく扱えないため D3D12 COM を直接呼べない。
//   cbridge_DX12_Init がデバイス・スワップチェーン・
//   RTV・ImGui DX12 バックエンドを一括初期化する。
//   TCC 側は igNewFrame() / igRender() と UI ロジックのみ。
// -------------------------------------------------------
extern int  cbridge_DX12_Init(void* hwnd, int numBackBuffers, int numFramesInFlight);
extern void cbridge_DX12_NewFrame(void);
extern void cbridge_DX12_Shutdown(void);
extern void cbridge_DX12_ResizeBuffers(unsigned int w, unsigned int h);
extern void cbridge_DX12_Render(const float* clear_color);  // float[4] RGBA

// -------------------------------------------------------
// アプリ状態
// -------------------------------------------------------
static int   g_imgui_initialized = 0;
static int   g_show_demo         = 1;
static int   g_show_my_window    = 1;
static float g_clear_color[4]    = { 0.0f, 0.2f, 0.4f, 1.0f };  // RGBA
static float g_counter           = 0.0f;

// -------------------------------------------------------
// DPI 設定
// -------------------------------------------------------
typedef enum {
    PROCESS_DPI_UNAWARE            = 0,
    PROCESS_SYSTEM_DPI_AWARE       = 1,
    PROCESS_PER_MONITOR_DPI_AWARE  = 2
} PROCESS_DPI_AWARENESS;
typedef HRESULT(__stdcall* PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);

static void EnableDpiAwareness(void) {
    HMODULE h = LoadLibraryA("shcore.dll");
    if (h) {
        PFN_SetProcessDpiAwareness fn =
            (PFN_SetProcessDpiAwareness)GetProcAddress(h, "SetProcessDpiAwareness");
        if (fn) { fn(PROCESS_PER_MONITOR_DPI_AWARE); return; }
    }
    h = LoadLibraryA("user32.dll");
    if (h) {
        BOOL(__stdcall*fn2)(void) =
            (BOOL(__stdcall*)(void))GetProcAddress(h, "SetProcessDPIAware");
        if (fn2) fn2();
    }
}

// -------------------------------------------------------
// Render フレーム
// -------------------------------------------------------
static void Render(void) {
    // ImGui フレーム開始（順序: DX12_NewFrame → Win32_NewFrame → igNewFrame）
    cbridge_DX12_NewFrame();
    cbridge_ImGui_ImplWin32_NewFrame();
    igNewFrame();

    // --- UI ---
    if (g_show_demo) igShowDemoWindow(&g_show_demo);

    if (g_show_my_window) {
        igBegin("TCC + cimgui + DX12", &g_show_my_window, 0);
        igText("Hello from TCC + DirectX12!");
        igSeparator();
        igColorEdit3("Background", g_clear_color, 0);
        igSeparator();
        if (igButton("Count up", (ImVec2){0, 0})) g_counter += 1.0f;
        igSameLine(0.0f, -1.0f);
        igText("count = %.0f", g_counter);
        igSeparator();
        ImGuiIO* io = igGetIO_Nil();
        igText("%.3f ms/frame (%.1f FPS)",
               1000.0f / io->Framerate, io->Framerate);
        igEnd();
    }

    igRender();

    // C++ 側でバリア・クリア・RenderDrawData・Present を一括実行
    cbridge_DX12_Render(g_clear_color);
}

// -------------------------------------------------------
// WndProc
// -------------------------------------------------------
static HWND g_hwnd = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (cbridge_ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return TRUE;

    if (msg == WM_SIZE) {
        UINT w = (UINT)(lParam & 0xFFFF);
        UINT h = (UINT)((lParam >> 16) & 0xFFFF);
        if (w > 0 && h > 0 && g_imgui_initialized)
            cbridge_DX12_ResizeBuffers(w, h);
    }
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// -------------------------------------------------------
// main
// -------------------------------------------------------
int main(void) {
    EnableDpiAwareness();

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"DX12ImGuiWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    // クライアント領域をちょうど 800x600 にする
    RECT wr = { 0, 0, 800, 600 };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    g_hwnd = CreateWindowW(
        L"DX12ImGuiWindow", L"TCC + cimgui + DirectX12",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right  - wr.left,
        wr.bottom - wr.top,
        NULL, NULL, wc.hInstance, NULL);
    if (!g_hwnd) { printf("CreateWindow failed\n"); return 1; }

    // ImGui コンテキストを先に作る
    // （cbridge_DX12_Init 内の ImGui_ImplDX12_Init がコンテキストを必要とするため）
    igCreateContext(NULL);
    ImGuiIO* io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    igStyleColorsDark(NULL);

    // D3D12 + ImGui DX12 バックエンドを C++ 側で一括初期化
    printf("Calling cbridge_DX12_Init...\n"); fflush(stdout);
    if (!cbridge_DX12_Init(g_hwnd, 3, 3)) {
        printf("cbridge_DX12_Init FAILED\n");
        igDestroyContext(NULL);
        return 1;
    }
    printf("cbridge_DX12_Init OK\n"); fflush(stdout);

    // Win32 バックエンド
    cbridge_ImGui_ImplWin32_Init(g_hwnd);

    g_imgui_initialized = 1;

    ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hwnd);

    MSG msg;
    memset(&msg, 0, sizeof(msg));
    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        Render();
    }

    // クリーンアップ (renderer -> platform -> context)
    cbridge_DX12_Shutdown();
    cbridge_ImGui_ImplWin32_Shutdown();
    igDestroyContext(NULL);
    return 0;
}
