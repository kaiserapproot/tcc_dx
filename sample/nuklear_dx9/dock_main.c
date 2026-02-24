#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <stdlib.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_d3d9.h"

#include "..\nuklear\dock.h"
#include "..\nuklear\dock_manager.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static LPDIRECT3D9 d3d;
static LPDIRECT3DDEVICE9 d3ddev;
static struct nk_context *ctx;
static int running = 1;
static HWND wnd;

static void create_device(HWND hwnd) {
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferWidth = WINDOW_WIDTH;
    pp.BackBufferHeight = WINDOW_HEIGHT;
    pp.EnableAutoDepthStencil = FALSE;

    if (d3d->lpVtbl->CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &d3ddev) < 0) {
        d3d = NULL; d3ddev = NULL;
    }
}

static void cleanup(void) {
    if (ctx) nk_d3d9_shutdown();
    if (d3ddev) { d3ddev->lpVtbl->Release(d3ddev); d3ddev = NULL; }
    if (d3d) { d3d->lpVtbl->Release(d3d); d3d = NULL; }
}

static LRESULT CALLBACK wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CLOSE) { running = 0; PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = wndproc;
    wc.hInstance = hInst;
    wc.lpszClassName = "DockNuklear";
    RegisterClass(&wc);

    wnd = CreateWindowEx(0, wc.lpszClassName, "Nuklear Dock Demo (DX9)", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInst, NULL);
    if (!wnd) return -1;

    ShowWindow(wnd, SW_SHOWDEFAULT);
    UpdateWindow(wnd);

    create_device(wnd);
    if (!d3ddev) { MessageBox(NULL, "Failed to create D3D9 device", "Error", MB_OK); return -1; }

    ctx = nk_d3d9_init((void*)d3ddev, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!ctx) { MessageBox(NULL, "Failed to init nuklear d3d9", "Error", MB_OK); cleanup(); return -1; }

    /* Create DockManager and sample windows */
    DockManager *mgr = dock_manager_create();
    DockWindow *w1 = dock_manager_add_window(mgr, "win1", "First");
    DockWindow *w2 = dock_manager_add_window(mgr, "win2", "Second");
    DockWindow *w3 = dock_manager_add_window(mgr, "win3", "Third");

    /* Simple layout: root is a tab node containing all three */
    DockNode *root = dock_node_create_tab();
    dock_tab_add(root, w1);
    dock_tab_add(root, w2);
    dock_tab_add(root, w3);
    mgr->root = root;

    /* Main loop */
    MSG msg;
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        nk_input_begin(ctx);
        nk_input_end(ctx);

        /* compute layout for full window area */
        struct nk_rect area = nk_rect(0,0,(float)WINDOW_WIDTH,(float)WINDOW_HEIGHT);
        dock_manager_compute_layout(mgr, area);

        if (nk_begin(ctx, "Main", nk_rect(0,0,WINDOW_WIDTH,30), NK_WINDOW_TITLE)) {
            nk_layout_row_dynamic(ctx, 22, 1);
            nk_label(ctx, "Dock demo", NK_TEXT_LEFT);
        }
        nk_end(ctx);

        /* Render dock UI: iterate nodes and draw tabs */
        dock_ui_render(mgr, ctx);

        /* Draw active window content for demo */
        if (mgr->root && mgr->root->type == DOCK_NODE_TAB) {
            DockNode *tn = mgr->root;
            if (tn->tabs_count > 0) {
                DockWindow *aw = tn->tabs[tn->active_tab < 0 ? 0 : tn->active_tab];
                if (aw && nk_begin(ctx, aw->name, aw->last_bounds, NK_WINDOW_BORDER|NK_WINDOW_TITLE)) {
                    nk_layout_row_dynamic(ctx, 20, 1);
                    nk_label(ctx, aw->title, NK_TEXT_LEFT);
                }
                nk_end(ctx);
            }
        }

        /* render via nk_d3d9 */
        d3ddev->lpVtbl->Clear(d3ddev, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(30,30,30), 1.0f, 0);
        if (d3ddev->lpVtbl->BeginScene(d3ddev) >= 0) {
            nk_d3d9_render(ctx);
            d3ddev->lpVtbl->EndScene(d3ddev);
        }
        d3ddev->lpVtbl->Present(d3ddev, NULL, NULL, NULL, NULL);

        Sleep(16);
    }

    dock_manager_free(mgr);
    cleanup();
    return 0;
}
