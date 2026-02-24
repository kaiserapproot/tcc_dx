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
// OpenGL 型定義（gl.h の代わり）
// -------------------------------------------------------
typedef unsigned int   GLenum;
typedef unsigned int   GLbitfield;
typedef unsigned int   GLuint;
typedef int            GLint;
typedef int            GLsizei;
typedef float          GLfloat;
typedef double         GLdouble;
typedef unsigned char  GLboolean;
typedef void           GLvoid;

#define GL_COLOR_BUFFER_BIT   0x00004000
#define GL_DEPTH_BUFFER_BIT   0x00000100
#define GL_TRIANGLES          0x0004
#define GL_FLOAT              0x1406
#define GL_TRUE               1
#define GL_FALSE              0

// OpenGL 関数ポインタ
typedef void  (__stdcall* PFNGLCLEARCOLORPROC)(GLfloat, GLfloat, GLfloat, GLfloat);
typedef void  (__stdcall* PFNGLCLEARPROC)(GLbitfield);
typedef void  (__stdcall* PFNGLBEGINPROC)(GLenum);
typedef void  (__stdcall* PFNGLENDPROC)(void);
typedef void  (__stdcall* PFNGLCOLOR3FPROC)(GLfloat, GLfloat, GLfloat);
typedef void  (__stdcall* PFNGLVERTEX2FPROC)(GLfloat, GLfloat);
typedef void  (__stdcall* PFNGLVIEWPORTPROC)(GLint, GLint, GLsizei, GLsizei);
typedef void  (__stdcall* PFNGLMATRIXMODEPROC)(GLenum);
typedef void  (__stdcall* PFNGLLOADIDENTITYPROC)(void);
typedef void  (__stdcall* PFNGLORTHOPROC)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);

static PFNGLCLEARCOLORPROC  glClearColor_f  = NULL;
static PFNGLCLEARPROC       glClear_f       = NULL;
static PFNGLBEGINPROC       glBegin_f       = NULL;
static PFNGLENDPROC         glEnd_f         = NULL;
static PFNGLCOLOR3FPROC     glColor3f_f     = NULL;
static PFNGLVERTEX2FPROC    glVertex2f_f    = NULL;
static PFNGLVIEWPORTPROC    glViewport_f    = NULL;
static PFNGLMATRIXMODEPROC  glMatrixMode_f  = NULL;
static PFNGLLOADIDENTITYPROC glLoadIdentity_f = NULL;
static PFNGLORTHOPROC       glOrtho_f       = NULL;

#define GL_PROJECTION 0x1701
#define GL_MODELVIEW  0x1700

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

extern int       cbridge_ImGui_ImplOpenGL2_Init(void);
extern void      cbridge_ImGui_ImplOpenGL2_Shutdown(void);
extern void      cbridge_ImGui_ImplOpenGL2_NewFrame(void);
extern void      cbridge_ImGui_ImplOpenGL2_RenderDrawData(void* draw_data);
extern int       cbridge_ImGui_ImplOpenGL2_CreateDeviceObjects(void);
extern void      cbridge_ImGui_ImplOpenGL2_DestroyDeviceObjects(void);

// -------------------------------------------------------
// グローバル変数
// -------------------------------------------------------
static HDC   g_hdc  = NULL;
static HGLRC g_hglrc = NULL;
static HMODULE g_opengl32 = NULL;

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
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// -------------------------------------------------------
// WGL / OpenGL 初期化
// -------------------------------------------------------
typedef HGLRC (__stdcall* PFNWGLCREATECONTEXTPROC)(HDC);
typedef BOOL  (__stdcall* PFNWGLMAKECURRENTPROC)(HDC, HGLRC);
typedef BOOL  (__stdcall* PFNWGLDELETECONTEXTPROC)(HGLRC);
typedef PROC  (__stdcall* PFNWGLGETPROCADDRESSPROC)(LPCSTR);

static PFNWGLCREATECONTEXTPROC  wglCreateContext_f  = NULL;
static PFNWGLMAKECURRENTPROC    wglMakeCurrent_f    = NULL;
static PFNWGLDELETECONTEXTPROC  wglDeleteContext_f  = NULL;
static PFNWGLGETPROCADDRESSPROC wglGetProcAddress_f = NULL;

static void* GL_GetProc(const char* name)
{
    void* p = (void*)wglGetProcAddress_f(name);
    if (!p) p = (void*)GetProcAddress(g_opengl32, name);
    return p;
}

BOOL InitOpenGL(HWND hwnd)
{
    // opengl32.dll を動的ロード
    g_opengl32 = LoadLibraryA("opengl32.dll");
    if (!g_opengl32) { printf("opengl32.dll not found\n"); return FALSE; }

    wglCreateContext_f  = (PFNWGLCREATECONTEXTPROC) GetProcAddress(g_opengl32, "wglCreateContext");
    wglMakeCurrent_f    = (PFNWGLMAKECURRENTPROC)   GetProcAddress(g_opengl32, "wglMakeCurrent");
    wglDeleteContext_f  = (PFNWGLDELETECONTEXTPROC)  GetProcAddress(g_opengl32, "wglDeleteContext");
    wglGetProcAddress_f = (PFNWGLGETPROCADDRESSPROC) GetProcAddress(g_opengl32, "wglGetProcAddress");

    if (!wglCreateContext_f || !wglMakeCurrent_f || !wglDeleteContext_f) {
        printf("wgl functions not found\n"); return FALSE;
    }

    g_hdc = GetDC(hwnd);

    // ピクセルフォーマット設定
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int fmt = ChoosePixelFormat(g_hdc, &pfd);
    if (!fmt) { printf("ChoosePixelFormat failed\n"); return FALSE; }
    if (!SetPixelFormat(g_hdc, fmt, &pfd)) { printf("SetPixelFormat failed\n"); return FALSE; }

    // OpenGL コンテキスト作成
    g_hglrc = wglCreateContext_f(g_hdc);
    if (!g_hglrc) { printf("wglCreateContext failed\n"); return FALSE; }
    if (!wglMakeCurrent_f(g_hdc, g_hglrc)) { printf("wglMakeCurrent failed\n"); return FALSE; }

    // OpenGL 関数ロード
    glClearColor_f   = (PFNGLCLEARCOLORPROC)   GL_GetProc("glClearColor");
    glClear_f        = (PFNGLCLEARPROC)         GL_GetProc("glClear");
    glBegin_f        = (PFNGLBEGINPROC)         GL_GetProc("glBegin");
    glEnd_f          = (PFNGLENDPROC)           GL_GetProc("glEnd");
    glColor3f_f      = (PFNGLCOLOR3FPROC)       GL_GetProc("glColor3f");
    glVertex2f_f     = (PFNGLVERTEX2FPROC)      GL_GetProc("glVertex2f");
    glViewport_f     = (PFNGLVIEWPORTPROC)      GL_GetProc("glViewport");
    glMatrixMode_f   = (PFNGLMATRIXMODEPROC)    GL_GetProc("glMatrixMode");
    glLoadIdentity_f = (PFNGLLOADIDENTITYPROC)  GL_GetProc("glLoadIdentity");
    glOrtho_f        = (PFNGLORTHOPROC)         GL_GetProc("glOrtho");

    printf("OpenGL initialized\n");
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
    cbridge_ImGui_ImplOpenGL2_Init();
}

// -------------------------------------------------------
// レンダリング
// -------------------------------------------------------
void Render(HWND hwnd)
{
    // クライアントサイズを毎フレーム取得
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width  = rect.right  - rect.left;
    int height = rect.bottom - rect.top;

    glViewport_f(0, 0, (GLsizei)width, (GLsizei)height);

    // 背景クリア
    glClearColor_f(g_clear_color[0], g_clear_color[1], g_clear_color[2], 1.0f);
    glClear_f(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 座標系設定（正規化デバイス座標 -1〜1）
    glMatrixMode_f(GL_PROJECTION);
    glLoadIdentity_f();
    glOrtho_f(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode_f(GL_MODELVIEW);
    glLoadIdentity_f();

    // 三角形描画（固定機能パイプライン）
    glBegin_f(GL_TRIANGLES);
        glColor3f_f(1.0f, 0.0f, 0.0f);  glVertex2f_f( 0.0f,  0.5f);
        glColor3f_f(0.0f, 1.0f, 0.0f);  glVertex2f_f( 0.5f, -0.5f);
        glColor3f_f(0.0f, 0.0f, 1.0f);  glVertex2f_f(-0.5f, -0.5f);
    glEnd_f();

    // ImGui フレーム開始
    cbridge_ImGui_ImplOpenGL2_NewFrame();
    cbridge_ImGui_ImplWin32_NewFrame();
    igNewFrame();

    // デモウィンドウ
    if (g_show_demo)
        igShowDemoWindow(&g_show_demo);

    // 独自ウィンドウ
    if (g_show_my_window)
    {
        igBegin("TCC + cimgui + OpenGL2", &g_show_my_window, 0);

        igText("Hello from TCC + OpenGL2!");
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
    cbridge_ImGui_ImplOpenGL2_RenderDrawData(igGetDrawData());

    // バッファスワップ
    SwapBuffers(g_hdc);
}

// -------------------------------------------------------
// クリーンアップ
// -------------------------------------------------------
void Cleanup(HWND hwnd)
{
    cbridge_ImGui_ImplOpenGL2_Shutdown();
    cbridge_ImGui_ImplWin32_Shutdown();
    igDestroyContext(NULL);

    if (g_hglrc) {
        wglMakeCurrent_f(NULL, NULL);
        wglDeleteContext_f(g_hglrc);
        g_hglrc = NULL;
    }
    if (g_hdc) {
        ReleaseDC(hwnd, g_hdc);
        g_hdc = NULL;
    }
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
    wc.lpszClassName = L"GL2ImGuiWindow";
    wc.style         = CS_OWNDC;  // OpenGL では CS_OWNDC が必要
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"GL2ImGuiWindow", L"TCC + cimgui + OpenGL2",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) { printf("CreateWindow failed\n"); return 1; }

    if (!InitOpenGL(hwnd)) {
        printf("InitOpenGL failed\n");
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

    Cleanup(hwnd);
    return 0;
}
