/* nuklear - 1.32.0 - public domain */
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_D3D9_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_d3d9.h"

/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the defines */
/*#define INCLUDE_ALL */
/*#define INCLUDE_STYLE */
/*#define INCLUDE_CALCULATOR */
/*#define INCLUDE_OVERVIEW */
/*#define INCLUDE_NODE_EDITOR */

#ifdef INCLUDE_ALL
  #define INCLUDE_STYLE
  #define INCLUDE_CALCULATOR
  #define INCLUDE_OVERVIEW
  #define INCLUDE_NODE_EDITOR
#endif

#ifdef INCLUDE_STYLE
  #include "../style.c"
#endif
#ifdef INCLUDE_CALCULATOR
  #include "../calculator.c"
#endif
#ifdef INCLUDE_OVERVIEW
  #include "../overview.c"
#endif
#ifdef INCLUDE_NODE_EDITOR
  #include "../node_editor.c"
#endif

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
static IDirect3DDevice9 *device;
static IDirect3DDevice9Ex *deviceEx;
static D3DPRESENT_PARAMETERS present;

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (device)
        {
            UINT width = LOWORD(lparam);
            UINT height = HIWORD(lparam);
            if (width != 0 && height != 0 &&
                (width != present.BackBufferWidth || height != present.BackBufferHeight))
            {
                nk_d3d9_release();
                present.BackBufferWidth = width;
                present.BackBufferHeight = height;
                HRESULT hr = IDirect3DDevice9_Reset(device, &present);
                NK_ASSERT(SUCCEEDED(hr));
                nk_d3d9_resize(width, height);
            }
        }
        break;
    }

    if (nk_d3d9_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

static void create_d3d9_device(HWND wnd)
{
    HRESULT hr;

    present.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    present.BackBufferWidth = WINDOW_WIDTH;
    present.BackBufferHeight = WINDOW_HEIGHT;
    present.BackBufferFormat = D3DFMT_X8R8G8B8;
    present.BackBufferCount = 1;
    present.MultiSampleType = D3DMULTISAMPLE_NONE;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.hDeviceWindow = wnd;
    present.EnableAutoDepthStencil = TRUE;
    present.AutoDepthStencilFormat = D3DFMT_D24S8;
    present.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    present.Windowed = TRUE;

    {/* first try to create Direct3D9Ex device if possible (on Windows 7+) */
        typedef HRESULT WINAPI Direct3DCreate9ExPtr(UINT, IDirect3D9Ex**);
        Direct3DCreate9ExPtr *Direct3DCreate9Ex = (void *)GetProcAddress(GetModuleHandleA("d3d9.dll"), "Direct3DCreate9Ex");
        if (Direct3DCreate9Ex) {
            IDirect3D9Ex *d3d9ex;
            if (SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex))) {
                hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
                    D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
                    &present, NULL, &deviceEx);
                if (SUCCEEDED(hr)) {
                    device = (IDirect3DDevice9 *)deviceEx;
                } else {
                    /* hardware vertex processing not supported, no big deal
                    retry with software vertex processing */
                    hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
                        &present, NULL, &deviceEx);
                    if (SUCCEEDED(hr)) {
                        device = (IDirect3DDevice9 *)deviceEx;
                    }
                }
                IDirect3D9Ex_Release(d3d9ex);
            }
        }
    }

    if (!device) {
        /* otherwise do regular D3D9 setup */
        IDirect3D9 *d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

        hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
            &present, &device);
        if (FAILED(hr)) {
            /* hardware vertex processing not supported, no big deal
            retry with software vertex processing */
            hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
                &present, &device);
            NK_ASSERT(SUCCEEDED(hr));
        }
        IDirect3D9_Release(d3d9);
    }
}
// Shift-JIS????UTF-8???
void sjis_to_utf8(const char* sjis, char* utf8, int utf8_size) {
    int wlen = MultiByteToWideChar(932, 0, sjis, -1, NULL, 0);
    if (wlen <= 0) { utf8[0] = 0; return; }
    wchar_t* wbuf = (wchar_t*)malloc(wlen * sizeof(wchar_t));
    MultiByteToWideChar(932, 0, sjis, -1, wbuf, wlen);

    int ulen = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (ulen <= 0) { free(wbuf); utf8[0] = 0; return; }
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8, utf8_size, NULL, NULL);

    free(wbuf);
}

int main(void)
{
    struct nk_context *ctx;
    struct nk_colorf bg;

    WNDCLASSW wc;
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    int running = 1;


char sjis_str[] = "ああああ"; 
char utf8_str[128];
sjis_to_utf8(sjis_str, utf8_str, sizeof(utf8_str));
printf("sjis_str: %s\n", sjis_str);
printf("utf8_str: %s\n", utf8_str);
for (int i = 0; sjis_str[i] != 0; i++) {
    printf("%02X ", (unsigned char)sjis_str[i]);
}
printf("\n");

    /* Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"NuklearWindowClass";
    RegisterClassW(&wc);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);

    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"Nuklear Demo",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);

    create_d3d9_device(wnd);

    /* GUI */
    ctx = nk_d3d9_init(device, WINDOW_WIDTH, WINDOW_HEIGHT);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {
        struct nk_font_atlas* atlas;
        nk_d3d9_font_stash_begin(&atlas);

        // 日本語レンジを指定
        struct nk_font_config config = nk_font_config(14.0f);
        static const nk_rune jp_ranges[] = {
            0x0020, 0x00FF, // Basic Latin
            0x3000, 0x30FF, // Hiragana, Katakana
            0x31F0, 0x31FF, // Katakana Phonetic Extensions
            0xFF00, 0xFFEF, // Full-width roman + half-width katakana
            0x4E00, 0x9FAF, // CJK Unified Ideographs
            0
        };
        config.range = jp_ranges;
        config.oversample_h = 1;
        config.oversample_v = 1;
        config.pixel_snap = 0;

        // フォントを読み撼む
        struct nk_font *droid = NULL;
        FILE *fp = fopen("../fonts/MPLUS1p-Light.ttf", "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long font_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            void *font_data = malloc(font_size);
            if (font_data && fread(font_data, 1, font_size, fp) == font_size) {
                droid = nk_font_atlas_add_from_memory(atlas, font_data, font_size, 14.0f, &config);
            }
            fclose(fp);
            // font_dataはnk_font_atlas_add_from_memory内部で保持される
        }
        if (!droid) {
            droid = nk_font_atlas_add_default(atlas, 14.0f, 0);
        }
        nk_style_load_all_cursors(ctx, atlas->cursors);
        nk_style_set_font(ctx, &droid->handle);
        nk_d3d9_font_stash_end();
    }

    /* style.c */
    #ifdef INCLUDE_STYLE
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/
    #endif

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    char jp_text[256] = "";
    FILE *jp_fp = fopen("japanese.txt", "rb"); // japanese.txt?UTF-8???
    if (jp_fp) {
        size_t n = fread(jp_text, 1, sizeof(jp_text)-1, jp_fp);
        jp_text[n] = '\0';
        fclose(jp_fp);
    } else {
        strcpy(jp_text, "???????????????????");
    }

    while (running)
    {
        /* Input */
        MSG msg;
        nk_input_begin(ctx);
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        nk_input_end(ctx);

        /* GUI */
        if (nk_begin(ctx, "Nuklear Demo", nk_rect(50, 50, 330, 300),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;

            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "button"))
                fprintf(stdout, "button pressed\n");
            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
            nk_layout_row_dynamic(ctx, 22, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
                nk_layout_row_dynamic(ctx, 120, 1);
                bg = nk_color_picker(ctx, bg, NK_RGBA);
                nk_layout_row_dynamic(ctx, 25, 1);
                bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
                bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
                bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
                bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
                nk_combo_end(ctx);
            }

            // ??????????????????????????
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_label(ctx, jp_text, NK_TEXT_LEFT);
            nk_label(ctx, utf8_str, NK_TEXT_LEFT);
        }
        nk_end(ctx);

        /* -------------- EXAMPLES ---------------- */
        #ifdef INCLUDE_CALCULATOR
          calculator(ctx);
        #endif
        #ifdef INCLUDE_OVERVIEW
          overview(ctx);
        #endif
        #ifdef INCLUDE_NODE_EDITOR
          node_editor(ctx);
        #endif
        /* ----------------------------------------- */

        /* Draw */
        {
            HRESULT hr;
            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
                D3DCOLOR_COLORVALUE(bg.r, bg.g, bg.b, bg.a), 0.0f, 0);
            NK_ASSERT(SUCCEEDED(hr));

            hr = IDirect3DDevice9_BeginScene(device);
            NK_ASSERT(SUCCEEDED(hr));
            nk_d3d9_render(NK_ANTI_ALIASING_ON);
            hr = IDirect3DDevice9_EndScene(device);
            NK_ASSERT(SUCCEEDED(hr));

            if (deviceEx) {
                hr = IDirect3DDevice9Ex_PresentEx(deviceEx, NULL, NULL, NULL, NULL, 0);
            } else {
                hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
            }
            if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICEHUNG || hr == D3DERR_DEVICEREMOVED) {
                /* to recover from this, you'll need to recreate device and all the resources */
                MessageBoxW(NULL, L"D3D9 device is lost or removed!", L"Error", 0);
                break;
            } else if (hr == S_PRESENT_OCCLUDED) {
                /* window is not visible, so vsync won't work. Let's sleep a bit to reduce CPU usage */
                Sleep(10);
            }
            NK_ASSERT(SUCCEEDED(hr));
        }
    }
    nk_d3d9_shutdown();
    if (deviceEx)IDirect3DDevice9Ex_Release(deviceEx);
    else IDirect3DDevice9_Release(device);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
