#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include "../../dev/include/d3dx9core.h"
#include <wchar.h>
#include <tchar.h>

/* Compatibility wrappers for tcc if wide-char helpers are missing */
#ifndef HAVE_WMEMMOVE
static inline wchar_t* compat_wmemmove(wchar_t* dest, const wchar_t* src, size_t n)
{
    return (wchar_t*)memmove(dest, (const void*)src, n * sizeof(wchar_t));
}
#define wmemmove(dest, src, n) compat_wmemmove((dest),(src),(n))
#endif

#ifndef HAVE_WMEMCPY
static inline wchar_t* compat_wmemcpy(wchar_t* dest, const wchar_t* src, size_t n)
{
    return (wchar_t*)memcpy(dest, (const void*)src, n * sizeof(wchar_t));
}
#define wmemcpy(dest, src, n) compat_wmemcpy((dest),(src),(n))
#endif

/* _T and _stprintf compatibility if <tchar.h> isn't providing them under tcc */
#ifndef _T
#ifdef UNICODE
#define _T(x) L##x
#else
#define _T(x) x
#endif
#endif

#ifndef _stprintf
#ifdef UNICODE
#define _stprintf swprintf
#else
#define _stprintf sprintf
#endif
#endif
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
#ifndef IID_IDirect3D9Ex
#include <initguid.h>
DEFINE_GUID(IID_IDirect3D9Ex, \
0x02177241, 0x69fc, 0x400c, 0x8f, 0xc3, 0x36, 0x6e, 0x48, 0x28, 0x53, 0x9b);
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_MEMORY 8192
#define RENDER_INTERVAL 16

/* Custom text editor defines */
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 1000
#define FONT_HEIGHT 24
#define FONT_LINE_SPACING 2
#define FONT_LINE_HEIGHT (FONT_HEIGHT + FONT_LINE_SPACING)

/* CustomTextEdit structure */
typedef struct
{
    wchar_t lines[MAX_LINES][MAX_LINE_LENGTH];
    int line_count;
    int caret_line;
    int caret_col;
    wchar_t composition_str[MAX_LINE_LENGTH];
    BOOL has_focus;
    RECT rect;
    BOOL visible;
    BOOL processing_ime;
} CustomTextEdit;

/* Global variables */
static int current_width = WINDOW_WIDTH;
static int current_height = WINDOW_HEIGHT;
static IDirect3DDevice9* device;
#if !defined(_MSC_VER) || (_MSC_VER > 1200)
static IDirect3DDevice9Ex* deviceEx;
#endif
static D3DPRESENT_PARAMETERS present;
static struct nk_text_edit editor;
static char editor_buffer[MAX_MEMORY];
static struct nk_context* ctx;
static CustomTextEdit dx9_editor;
static int use_native_editor = 0;  /* 0: Nuklear editor, 1: DirectX9 custom editor (not Windows control) */
static int editor_visible = 1;
static int toggle_editor_type = 0;
static int editor_margin = 60;
static int header_height = 30;
static int status_height = 20;
static BOOL rendering_paused = FALSE;
static BOOL needs_redraw = TRUE;
static DWORD last_render_time = 0;
/* Fallback typedef in case D3DX headers are not visible to the compiler (tcc) */
#ifndef LPD3DXFONT
typedef struct ID3DXFont *LPD3DXFONT;
#endif
static LPD3DXFONT g_pD3DXFont = NULL;
/* D3DXCreateFont 関数プロトタイプ（古いSDKやヘッダで未定義の場合のため） */
#ifndef D3DXCreateFont
HRESULT WINAPI D3DXCreateFont(
    LPDIRECT3DDEVICE9 pDevice,
    INT Height,
    UINT Width,
    UINT Weight,
    UINT MipLevels,
    BOOL Italic,
    DWORD CharSet,
    DWORD OutputPrecision,
    DWORD Quality,
    DWORD PitchAndFamily,
    LPCTSTR pFacename,
    LPD3DXFONT* ppFont
);
#endif

/* Runtime loader for D3DXCreateFontW to avoid link-time dependency when using tcc
   Tries common D3DX9 DLL names and resolves D3DXCreateFontW with GetProcAddress. */
typedef HRESULT (WINAPI *PFN_D3DXCREATEFONTW)(
    LPDIRECT3DDEVICE9 pDevice,
    INT Height,
    UINT Width,
    UINT Weight,
    UINT MipLevels,
    BOOL Italic,
    DWORD CharSet,
    DWORD OutputPrecision,
    DWORD Quality,
    DWORD PitchAndFamily,
    const WCHAR* pFacename,
    LPD3DXFONT* ppFont
);

static PFN_D3DXCREATEFONTW pfnD3DXCreateFontW = NULL;

static HMODULE try_load_d3dx(void)
{
    const char* names[] = { "d3dx9_43.dll", "d3dx9_42.dll", "d3dx9_41.dll", "d3dx9_40.dll", "d3dx9.dll", NULL };
    int i = 0;
    HMODULE h = NULL;
    while (names[i]) {
        h = GetModuleHandleA(names[i]);
        if (!h) h = LoadLibraryA(names[i]);
        if (h) return h;
        i++;
    }
    return NULL;
}

static HRESULT create_d3dx_font_w(
    LPDIRECT3DDEVICE9 pDevice,
    INT Height,
    UINT Width,
    UINT Weight,
    UINT MipLevels,
    BOOL Italic,
    DWORD CharSet,
    DWORD OutputPrecision,
    DWORD Quality,
    DWORD PitchAndFamily,
    const WCHAR* pFacename,
    LPD3DXFONT* ppFont)
{
    if (!pfnD3DXCreateFontW) {
        HMODULE h = try_load_d3dx();
        if (!h) return E_FAIL;
        pfnD3DXCreateFontW = (PFN_D3DXCREATEFONTW)GetProcAddress(h, "D3DXCreateFontW");
        if (!pfnD3DXCreateFontW) return E_FAIL;
    }
    return pfnD3DXCreateFontW(pDevice, Height, Width, Weight, MipLevels, Italic,
        CharSet, OutputPrecision, Quality, PitchAndFamily, pFacename, ppFont);
}
static wchar_t* g_render_buffer = NULL;
static const int SAFE_BUFFER_SIZE = 100000;

/* Function declarations */
static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
static void create_d3d9_device(HWND wnd);
static void toggle_editor(HWND wnd);
static void render_frame(void);
static void render_ui(void);
static void render_dx9_editor(void);

/* CustomTextEdit functions */
static void TextEdit_Init(CustomTextEdit* edit);
static void TextEdit_Cleanup(CustomTextEdit* edit);
static void TextEdit_OnChar(CustomTextEdit* edit, wchar_t ch);
static void TextEdit_OnKeyDown(CustomTextEdit* edit, WPARAM wParam);
static void TextEdit_OnImeStartComposition(CustomTextEdit* edit, HWND hWnd);
static void TextEdit_OnImeComposition(CustomTextEdit* edit, HWND hWnd, LPARAM lParam);
static void TextEdit_OnImeEndComposition(CustomTextEdit* edit, HWND hWnd);
static void TextEdit_UpdateImePos(CustomTextEdit* edit, HWND hWnd);
static void TextEdit_SetFocus(CustomTextEdit* edit, BOOL focus);
static void TextEdit_SetRect(CustomTextEdit* edit, const RECT* rc);
static void TextEdit_SetVisible(CustomTextEdit* edit, BOOL visible);
static const char* TextEdit_GetText(CustomTextEdit* edit, char* buffer, size_t buffer_size);
static void TextEdit_SetText(CustomTextEdit* edit, const char* text);

/* TextEdit implementation */
static void TextEdit_Init(CustomTextEdit* edit)
{
    int i;
    edit->line_count = 1;
    edit->caret_line = 0;
    edit->caret_col = 0;
    edit->has_focus = FALSE;
    edit->visible = TRUE;
    edit->processing_ime = FALSE;
    edit->composition_str[0] = L'\0';

    /* Initialize all lines */
    for (i = 0; i < MAX_LINES; i++)
    {
        edit->lines[i][0] = L'\0';
    }

    SetRect(&edit->rect, 0, 0, 400, 200);
}

static void TextEdit_Cleanup(CustomTextEdit* edit)
{
    /* Nothing to do in C */
    (void)edit;
}

static void TextEdit_SetFocus(CustomTextEdit* edit, BOOL focus)
{
    edit->has_focus = focus;
}

static void TextEdit_SetRect(CustomTextEdit* edit, const RECT* rc)
{
    edit->rect = *rc;
}

static void TextEdit_SetVisible(CustomTextEdit* edit, BOOL visible)
{
    edit->visible = visible;
}

static void TextEdit_OnChar(CustomTextEdit* edit, wchar_t ch)
{
    int line_len;
    int i;

    if (!edit || !edit->visible || edit->caret_line < 0 || edit->caret_line >= MAX_LINES) return;

    if (ch == L'\r' || ch == L'\n')
    {
        /* Handle newline */
        if (edit->line_count >= MAX_LINES - 1) return;

        line_len = (int)wcslen(edit->lines[edit->caret_line]);
        if (edit->caret_col < 0) edit->caret_col = 0;
        if (edit->caret_col > line_len) edit->caret_col = line_len;

        /* Split the current line */
        /* First move all lines down */
        for (i = edit->line_count; i > edit->caret_line + 1; i--)
        {
            wcscpy_s(edit->lines[i], MAX_LINE_LENGTH, edit->lines[i - 1]);
        }

        /* Move the latter part of the current line to the new line */
        wcscpy_s(edit->lines[edit->caret_line + 1], MAX_LINE_LENGTH,
            &edit->lines[edit->caret_line][edit->caret_col]);

        /* Truncate the current line */
        edit->lines[edit->caret_line][edit->caret_col] = L'\0';

        edit->line_count++;
        edit->caret_line++;
        edit->caret_col = 0;

        needs_redraw = TRUE;
    }
    else if (ch >= 32) /* Printable characters only */
    {
        line_len = (int)wcslen(edit->lines[edit->caret_line]);
        if (edit->caret_col < 0) edit->caret_col = 0;
        if (edit->caret_col > line_len) edit->caret_col = line_len;

        if (line_len < MAX_LINE_LENGTH - 2)
        {
            /* Insert character */
            for (i = line_len; i >= edit->caret_col; i--)
            {
                edit->lines[edit->caret_line][i + 1] = edit->lines[edit->caret_line][i];
            }
            edit->lines[edit->caret_line][edit->caret_col] = ch;
            edit->caret_col++;

            needs_redraw = TRUE;
        }
    }
}

static void TextEdit_OnKeyDown(CustomTextEdit* edit, WPARAM wParam)
{
    int line_len;
    int i;

    if (!edit || !edit->visible || edit->caret_line < 0 || edit->caret_line >= MAX_LINES) return;

    switch (wParam)
    {
    case VK_LEFT:
        if (edit->caret_col > 0)
        {
            edit->caret_col--;
            needs_redraw = TRUE;
        }
        else if (edit->caret_line > 0)
        {
            edit->caret_line--;
            edit->caret_col = (int)wcslen(edit->lines[edit->caret_line]);
            needs_redraw = TRUE;
        }
        break;

    case VK_RIGHT:
        line_len = (int)wcslen(edit->lines[edit->caret_line]);
        if (edit->caret_col < line_len)
        {
            edit->caret_col++;
            needs_redraw = TRUE;
        }
        else if (edit->caret_line + 1 < edit->line_count && edit->caret_line + 1 < MAX_LINES)
        {
            edit->caret_line++;
            edit->caret_col = 0;
            needs_redraw = TRUE;
        }
        break;

    case VK_UP:
        if (edit->caret_line > 0)
        {
            edit->caret_line--;
            line_len = (int)wcslen(edit->lines[edit->caret_line]);
            if (edit->caret_col > line_len)
            {
                edit->caret_col = line_len;
            }
            needs_redraw = TRUE;
        }
        break;

    case VK_DOWN:
        if (edit->caret_line + 1 < edit->line_count && edit->caret_line + 1 < MAX_LINES)
        {
            edit->caret_line++;
            line_len = (int)wcslen(edit->lines[edit->caret_line]);
            if (edit->caret_col > line_len)
            {
                edit->caret_col = line_len;
            }
            needs_redraw = TRUE;
        }
        break;

    case VK_BACK:
        if (edit->caret_col > 0)
        {
            /* Delete character */
            line_len = (int)wcslen(edit->lines[edit->caret_line]);
            for (i = edit->caret_col - 1; i < line_len; i++)
            {
                edit->lines[edit->caret_line][i] = edit->lines[edit->caret_line][i + 1];
            }
            edit->caret_col--;
            needs_redraw = TRUE;
        }
        else if (edit->caret_line > 0)
        {
            /* Merge lines */
            edit->caret_col = (int)wcslen(edit->lines[edit->caret_line - 1]);

            /* Check buffer overflow */
            int prev_len = edit->caret_col;
            int curr_len = (int)wcslen(edit->lines[edit->caret_line]);
            if (prev_len + curr_len < MAX_LINE_LENGTH)
            {
                wcscat_s(edit->lines[edit->caret_line - 1], MAX_LINE_LENGTH,
                    edit->lines[edit->caret_line]);

                /* Delete line */
                for (i = edit->caret_line; i < edit->line_count - 1; i++)
                {
                    wcscpy_s(edit->lines[i], MAX_LINE_LENGTH, edit->lines[i + 1]);
                }
                edit->lines[edit->line_count - 1][0] = L'\0';
                edit->line_count--;
                edit->caret_line--;
                needs_redraw = TRUE;
            }
        }
        break;

    case VK_DELETE:
        line_len = (int)wcslen(edit->lines[edit->caret_line]);
        if (edit->caret_col < line_len)
        {
            /* Delete character */
            for (i = edit->caret_col; i < line_len; i++)
            {
                edit->lines[edit->caret_line][i] = edit->lines[edit->caret_line][i + 1];
            }
            needs_redraw = TRUE;
        }
        else if (edit->caret_line + 1 < edit->line_count)
        {
            /* Merge with next line */
            int curr_len = line_len;
            int next_len = (int)wcslen(edit->lines[edit->caret_line + 1]);
            if (curr_len + next_len < MAX_LINE_LENGTH)
            {
                wcscat_s(edit->lines[edit->caret_line], MAX_LINE_LENGTH,
                    edit->lines[edit->caret_line + 1]);

                /* Delete line */
                for (i = edit->caret_line + 1; i < edit->line_count - 1; i++)
                {
                    wcscpy_s(edit->lines[i], MAX_LINE_LENGTH, edit->lines[i + 1]);
                }
                edit->lines[edit->line_count - 1][0] = L'\0';
                edit->line_count--;
                needs_redraw = TRUE;
            }
        }
        break;

    case VK_HOME:
        edit->caret_col = 0;
        needs_redraw = TRUE;
        break;

    case VK_END:
        edit->caret_col = (int)wcslen(edit->lines[edit->caret_line]);
        needs_redraw = TRUE;
        break;
    }
}

static void TextEdit_OnImeStartComposition(CustomTextEdit* edit, HWND hWnd)
{
    edit->composition_str[0] = L'\0';
    edit->processing_ime = TRUE;
    TextEdit_UpdateImePos(edit, hWnd);
}

static void TextEdit_OnImeComposition(CustomTextEdit* edit, HWND hWnd, LPARAM lParam)
{
    HIMC himc;
    LONG len;
    wchar_t result[MAX_LINE_LENGTH];
    int line_len, result_len;

    if (!edit || !edit->visible || edit->caret_line < 0 || edit->caret_line >= MAX_LINES) return;

    himc = ImmGetContext(hWnd);
    if (!himc) return;

    if (lParam & GCS_RESULTSTR)
    {
        len = ImmGetCompositionStringW(himc, GCS_RESULTSTR, NULL, 0) / sizeof(wchar_t);
        if (len > 0 && len < MAX_LINE_LENGTH)
        {
            ImmGetCompositionStringW(himc, GCS_RESULTSTR, result, len * sizeof(wchar_t));
            result[len] = L'\0';

            /* Insert confirmed string directly */
            result_len = (int)wcslen(result);
            line_len = (int)wcslen(edit->lines[edit->caret_line]);

            /* Caret boundary check */
            if (edit->caret_col < 0) edit->caret_col = 0;
            if (edit->caret_col > line_len) edit->caret_col = line_len;

            /* Buffer overflow check */
            if (line_len + result_len < MAX_LINE_LENGTH - 1 && edit->caret_col <= line_len)
            {
                /* Calculate chars to move (avoid negative) */
                int chars_to_move = line_len - edit->caret_col;
                if (chars_to_move > 0)
                {
                    /* Move string to insertion point */
                    wmemmove(&edit->lines[edit->caret_line][edit->caret_col + result_len],
                        &edit->lines[edit->caret_line][edit->caret_col],
                        chars_to_move + 1); /* +1 for null terminator */
                }

                /* Insert confirmed string */
                wmemcpy(&edit->lines[edit->caret_line][edit->caret_col], result, result_len);

                /* Ensure null terminator */
                edit->lines[edit->caret_line][line_len + result_len] = L'\0';

                edit->caret_col += result_len;
                needs_redraw = TRUE;
            }
        }
        edit->composition_str[0] = L'\0';
    }
    else if (lParam & GCS_COMPSTR)
    {
        len = ImmGetCompositionStringW(himc, GCS_COMPSTR, NULL, 0) / sizeof(wchar_t);
        if (len > 0 && len < MAX_LINE_LENGTH)
        {
            ImmGetCompositionStringW(himc, GCS_COMPSTR, edit->composition_str,
                len * sizeof(wchar_t));
            edit->composition_str[len] = L'\0';
            needs_redraw = TRUE;
        }
        else
        {
            edit->composition_str[0] = L'\0';
            needs_redraw = TRUE;
        }
    }

    ImmReleaseContext(hWnd, himc);
    TextEdit_UpdateImePos(edit, hWnd);
}

static void TextEdit_OnImeEndComposition(CustomTextEdit* edit, HWND hWnd)
{
    (void)hWnd; /* Unused warning avoidance */
    edit->composition_str[0] = L'\0';
    edit->processing_ime = FALSE;
    needs_redraw = TRUE;
}

static void TextEdit_UpdateImePos(CustomTextEdit* edit, HWND hWnd)
{
    HIMC himc;
    float caretX, caretY;
    COMPOSITIONFORM cf;
    CANDIDATEFORM cand;

    himc = ImmGetContext(hWnd);
    if (!himc) return;

    /* Calculate caret position based on editor rect and font metrics */
    caretX = (float)edit->rect.left;
    caretY = (float)edit->rect.top;

    /* Add position based on text before caret */
    if (edit->caret_line >= 0 && edit->caret_line < edit->line_count)
    {
        caretY += edit->caret_line * FONT_HEIGHT;

        if (edit->caret_col > 0)
        {
            /* This is an approximation, ideally we'd measure text width */
            caretX += edit->caret_col * (FONT_HEIGHT / 2.0f);
        }
    }

    /* Set IME window position */
    ZeroMemory(&cf, sizeof(cf));
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = (LONG)caretX;
    cf.ptCurrentPos.y = (LONG)caretY;
    ImmSetCompositionWindow(himc, &cf);

    /* Set candidate window position */
    ZeroMemory(&cand, sizeof(cand));
    cand.dwIndex = 0;
    cand.dwStyle = CFS_CANDIDATEPOS;
    cand.ptCurrentPos.x = (LONG)caretX;
    cand.ptCurrentPos.y = (LONG)(caretY + FONT_HEIGHT);
    ImmSetCandidateWindow(himc, &cand);

    ImmReleaseContext(hWnd, himc);
}

static const char* TextEdit_GetText(CustomTextEdit* edit, char* buffer, size_t buffer_size)
{
    int i, pos = 0;
    char temp[MAX_LINE_LENGTH * 2];

    if (!buffer || buffer_size < 1)
        return NULL;

    buffer[0] = '\0';

    for (i = 0; i < edit->line_count && i < MAX_LINES; i++)
    {
        /* Convert wide char to multibyte */
        int result = WideCharToMultiByte(CP_UTF8, 0, edit->lines[i], -1,
            temp, sizeof(temp), NULL, NULL);
        if (result > 0)
        {
            /* Check if buffer has enough space */
            if ((pos + result) < (int)buffer_size)
            {
                strcpy_s(&buffer[pos], buffer_size - pos, temp);
                pos += result - 1;  /* -1 because we don't want to count the null terminator */

                /* Add newline if not the last line */
                if (i < edit->line_count - 1 && pos < (int)buffer_size - 1)
                {
                    buffer[pos++] = '\n';
                    buffer[pos] = '\0';
                }
            }
            else
            {
                break;
            }
        }
    }

    return buffer;
}

static void TextEdit_SetText(CustomTextEdit* edit, const char* text)
{
    int i = 0, line = 0;
    wchar_t wbuf[MAX_LINE_LENGTH];
    char linebuf[MAX_LINE_LENGTH];
    const char* p = text;

    /* Reset editor */
    for (i = 0; i < MAX_LINES; i++)
    {
        edit->lines[i][0] = L'\0';
    }

    edit->line_count = 0;
    i = 0;

    /* Parse input text line by line */
    while (*p && line < MAX_LINES)
    {
        if (*p == '\n' || *p == '\r' || *p == '\0')
        {
            linebuf[i] = '\0';

            /* Convert to wide char */
            if (MultiByteToWideChar(CP_UTF8, 0, linebuf, -1, wbuf, MAX_LINE_LENGTH) > 0)
            {
                wcscpy_s(edit->lines[line], MAX_LINE_LENGTH, wbuf);
                line++;
                edit->line_count++;
            }

            i = 0;

            /* Handle \r\n sequence */
            if (*p == '\r' && *(p + 1) == '\n')
                p++;
        }
        else
        {
            if (i < MAX_LINE_LENGTH - 1)
                linebuf[i++] = *p;
        }

        if (*p == '\0')
            break;

        p++;
    }

    /* Handle last line if it doesn't end with newline */
    if (i > 0 && line < MAX_LINES)
    {
        linebuf[i] = '\0';
        if (MultiByteToWideChar(CP_UTF8, 0, linebuf, -1, wbuf, MAX_LINE_LENGTH) > 0)
        {
            wcscpy_s(edit->lines[line], MAX_LINE_LENGTH, wbuf);
            edit->line_count++;
        }
    }

    /* Ensure we have at least one line */
    if (edit->line_count == 0)
    {
        edit->lines[0][0] = L'\0';
        edit->line_count = 1;
    }

    edit->caret_line = 0;
    edit->caret_col = 0;
    needs_redraw = TRUE;
}

/* Window procedure implementation */
static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    HRESULT hr;

    switch (msg)
    {
    case WM_CREATE:
        TextEdit_Init(&dx9_editor);
        TextEdit_SetText(&dx9_editor, "DirectX9 custom text editor with IME support.");
        break;

    case WM_DESTROY:
        TextEdit_Cleanup(&dx9_editor);

        if (g_render_buffer)
        {
            free(g_render_buffer);
            g_render_buffer = NULL;
        }

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
                current_width = width;
                current_height = height;
                hr = IDirect3DDevice9_Reset(device, &present);
                NK_ASSERT(SUCCEEDED(hr));
                nk_d3d9_resize(width, height);

                if (use_native_editor && editor_visible)
                {
                    int editor_x = editor_margin;
                    int editor_y = editor_margin + header_height;
                    int editor_width = width - (editor_margin * 2);
                    int editor_height = height - (editor_margin * 2) - header_height - status_height;

                    RECT rc = { editor_x, editor_y, editor_x + editor_width, editor_y + editor_height };
                    TextEdit_SetRect(&dx9_editor, &rc);
                }

                needs_redraw = TRUE;
            }
        }
        break;

    case WM_KEYDOWN:
        if (wparam == VK_F2)
        {
            toggle_editor_type = 1;
            return 0;
        }

        if (use_native_editor && !dx9_editor.processing_ime)
        {
            TextEdit_OnKeyDown(&dx9_editor, wparam);
            return 0;
        }
        break;

    case WM_CHAR:
        if (use_native_editor && !dx9_editor.processing_ime)
        {
            TextEdit_OnChar(&dx9_editor, (wchar_t)wparam);
            return 0;
        }
        break;

    case WM_IME_STARTCOMPOSITION:
        if (use_native_editor)
        {
            TextEdit_OnImeStartComposition(&dx9_editor, wnd);
            return 0;
        }
        break;

    case WM_IME_COMPOSITION:
        if (use_native_editor && dx9_editor.processing_ime)
        {
            TextEdit_OnImeComposition(&dx9_editor, wnd, lparam);
            return 0;
        }
        break;

    case WM_IME_ENDCOMPOSITION:
        if (use_native_editor)
        {
            TextEdit_OnImeEndComposition(&dx9_editor, wnd);
            return 0;
        }
        break;

    case WM_SETFOCUS:
        if (use_native_editor)
        {
            TextEdit_SetFocus(&dx9_editor, TRUE);
            needs_redraw = TRUE;
        }
        break;

    case WM_KILLFOCUS:
        if (use_native_editor)
        {
            TextEdit_SetFocus(&dx9_editor, FALSE);
            dx9_editor.processing_ime = FALSE;
            needs_redraw = TRUE;
        }
        break;

    case WM_PAINT:
        needs_redraw = TRUE;
        break;
    }

    if (!use_native_editor && nk_d3d9_handle_event(wnd, msg, wparam, lparam))
        return 0;
    return DefWindowProc(wnd, msg, wparam, lparam);
}

/* Device creation function */
static void create_d3d9_device(HWND wnd)
{
    HRESULT hr;
    IDirect3D9* d3d9;

    ZeroMemory(&present, sizeof(present));
    present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
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

#if !defined(_MSC_VER) || (_MSC_VER > 1200)
    {
        IDirect3D9Ex* d3d9ex = NULL;
        hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
        if (FAILED(hr))
        {
            /* Direct3D9Ex が失敗した場合は標準のDirect3D9を試みる */
            d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
            if (!d3d9)
            {
                MessageBox(NULL, _T("Direct3DCreate9 failed"), _T("Error"), MB_OK | MB_ICONERROR);
                return;
            }
        }
        else
        {
            d3d9 = (IDirect3D9*)d3d9ex;
        }
    }
#else
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d9)
    {
        MessageBox(NULL, _T("Direct3DCreate9 failed"), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }
#endif

    /* Get device capabilities */
    D3DCAPS9 caps;
    hr = IDirect3D9_GetDeviceCaps(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    if (FAILED(hr))
    {
        MessageBox(NULL, _T("GetDeviceCaps failed"), _T("Error"), MB_OK | MB_ICONERROR);
        IDirect3D9_Release(d3d9);
        return;
    }

    DWORD vertexProcessing = (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
        D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;

#if !defined(_MSC_VER) || (_MSC_VER > 1200)
    /* Check if we are using Direct3D9Ex */
    IDirect3D9Ex* d3d9ex_test = NULL;
    if (SUCCEEDED(d3d9->lpVtbl->QueryInterface(d3d9, &IID_IDirect3D9Ex, (void**)&d3d9ex_test)))
    {
        hr = d3d9ex_test->lpVtbl->CreateDeviceEx(d3d9ex_test,
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            wnd,
            vertexProcessing | D3DCREATE_FPU_PRESERVE,
            &present,
            NULL,
            &deviceEx);

        if (SUCCEEDED(hr))
        {
            device = (IDirect3DDevice9*)deviceEx;
        }
        d3d9ex_test->lpVtbl->Release(d3d9ex_test);
    }
    if (!device)
#endif
    {
        hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
            vertexProcessing | D3DCREATE_FPU_PRESERVE,
            &present, &device);
    }

    if (FAILED(hr))
    {
        hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, wnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
            &present, &device);

        if (FAILED(hr))
        {
            TCHAR buffer[256];
            _stprintf(buffer, _T("Device creation failed: 0x%08X"), (unsigned int)hr);
            MessageBox(NULL, buffer, _T("Error"), MB_OK | MB_ICONERROR);
            IDirect3D9_Release(d3d9);
            return;
        }
    }

    IDirect3D9_Release(d3d9);

    if (!device)
    {
        d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
        if (!d3d9)
        {
            MessageBox(NULL, _T("Direct3DCreate9 failed"), _T("Error"), MB_OK | MB_ICONERROR);
            return;
        }
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
        hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
            &present, &device);
        if (FAILED(hr))
        {
            hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, wnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
                &present, &device);
        }
#else
        hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
            &present, &device);
        if (FAILED(hr))
        {
            hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE | D3DCREATE_FPU_PRESERVE,
                &present, &device);
        }
#endif
        if (FAILED(hr))
        {
            TCHAR buffer[256];
            _stprintf(buffer, _T("Device creation failed: 0x%08X"), (unsigned int)hr);
            MessageBox(NULL, buffer, _T("Error"), MB_OK | MB_ICONERROR);
            IDirect3D9_Release(d3d9);
            return;
        }
        IDirect3D9_Release(d3d9);
    }

    /* Create D3DXFont for DX9 editor */

    hr = create_d3dx_font_w(device, 18, 0, FW_NORMAL, 1, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"メイリオ", &g_pD3DXFont);

    if (FAILED(hr))
    {
        hr = create_d3dx_font_w(device, 18, 0, FW_NORMAL, 1, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Arial", &g_pD3DXFont);
    }

    /* Allocate render buffer */
    g_render_buffer = (wchar_t*)malloc(SAFE_BUFFER_SIZE * sizeof(wchar_t));
}

/* Editor type switching function */
static void toggle_editor(HWND wnd)
{
    char temp_buffer[MAX_MEMORY] = { 0 };

    if (use_native_editor)
    {
        /* Get text from DX9 editor */
        TextEdit_GetText(&dx9_editor, temp_buffer, sizeof(temp_buffer));
        TextEdit_SetVisible(&dx9_editor, FALSE);
    }
    else
    {
        /* Get text from Nuklear editor */
        const char* text = nk_str_get_const(&editor.string);
        if (text)
        {
            strncpy(temp_buffer, text, sizeof(temp_buffer) - 1);
        }
    }

    use_native_editor = !use_native_editor;

    if (use_native_editor)
    {
        /* Setup DX9 editor */
        int editor_x = editor_margin;
        int editor_y = editor_margin + header_height;
        int editor_width = current_width - (editor_margin * 2);
        int editor_height = current_height - (editor_margin * 2) - header_height - status_height;

        RECT rc = { editor_x, editor_y, editor_x + editor_width, editor_y + editor_height };
        TextEdit_SetRect(&dx9_editor, &rc);
        TextEdit_SetVisible(&dx9_editor, TRUE);
        TextEdit_SetFocus(&dx9_editor, TRUE);

        if (temp_buffer[0])
        {
            TextEdit_SetText(&dx9_editor, temp_buffer);
        }
    }
    else
    {
        /* Setup Nuklear editor */
        nk_textedit_init_fixed(&editor, editor_buffer, MAX_MEMORY);

        if (temp_buffer[0])
        {
            int len = (int)strlen(temp_buffer);
            nk_textedit_delete(&editor, 0, editor.string.len);
            nk_textedit_text(&editor, temp_buffer, len);
        }
    }

    needs_redraw = TRUE;
}

/* Render DX9 editor */
static void render_dx9_editor(void)
{
    if (!g_pD3DXFont || !dx9_editor.visible) return;

    int i;
    int baseX = dx9_editor.rect.left;
    int baseY = dx9_editor.rect.top;
    wchar_t tempLine[MAX_LINE_LENGTH * 2]; /* expanded for composition */

    /* Set a background for editor area */
    D3DRECT rect;
    rect.x1 = dx9_editor.rect.left;
    rect.y1 = dx9_editor.rect.top;
    rect.x2 = dx9_editor.rect.right;
    rect.y2 = dx9_editor.rect.bottom;

    IDirect3DDevice9_Clear(device, 1, &rect, D3DCLEAR_TARGET,
        D3DCOLOR_ARGB(255, 32, 32, 48), 1.0f, 0);

    /* Draw all lines */
    for (i = 0; i < dx9_editor.line_count && i < MAX_LINES; i++)
    {
        RECT lineRect;
        SetRect(&lineRect, baseX, baseY + i * FONT_HEIGHT, dx9_editor.rect.right, baseY + (i + 1) * FONT_HEIGHT);

        /* Draw composition string if applicable */
        if (i == dx9_editor.caret_line && dx9_editor.composition_str[0] != L'\0')
        {
            wcscpy_s(tempLine, MAX_LINE_LENGTH * 2, dx9_editor.lines[i]);
            int comp_len = (int)wcslen(dx9_editor.composition_str);
            int temp_line_len = (int)wcslen(tempLine);

            if (dx9_editor.caret_col >= 0 && dx9_editor.caret_col <= temp_line_len &&
                temp_line_len + comp_len < MAX_LINE_LENGTH * 2 - 1)
            {

                if (dx9_editor.caret_col < temp_line_len)
                {
                    int chars_to_move = temp_line_len - dx9_editor.caret_col;
                    wmemmove(&tempLine[dx9_editor.caret_col + comp_len],
                        &tempLine[dx9_editor.caret_col],
                        chars_to_move + 1);
                }

                wmemcpy(&tempLine[dx9_editor.caret_col], dx9_editor.composition_str, comp_len);
                tempLine[temp_line_len + comp_len] = L'\0';
            }

            g_pD3DXFont->lpVtbl->DrawTextW(g_pD3DXFont, NULL, tempLine, -1, &lineRect,
                DT_LEFT | DT_TOP | DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
        }
        else
        {
            g_pD3DXFont->lpVtbl->DrawTextW(g_pD3DXFont, NULL, dx9_editor.lines[i], -1, &lineRect,
                DT_LEFT | DT_TOP | DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));
        }
    }

    /* Draw caret */
    if (dx9_editor.has_focus)
    {
        int caretX = baseX;
        int caretY = baseY + dx9_editor.caret_line * FONT_HEIGHT;

        if (dx9_editor.caret_col > 0)
        {
            wchar_t caretStr[MAX_LINE_LENGTH];
            wcsncpy_s(caretStr, MAX_LINE_LENGTH, dx9_editor.lines[dx9_editor.caret_line], dx9_editor.caret_col);
            caretStr[dx9_editor.caret_col] = L'\0';

            RECT tempRect = { 0, 0, 0, 0 };
            g_pD3DXFont->lpVtbl->DrawTextW(g_pD3DXFont, NULL, caretStr, -1, &tempRect,
                DT_LEFT | DT_TOP | DT_NOCLIP | DT_CALCRECT, 0);
            caretX += tempRect.right;
        }

        RECT caretRect;
        SetRect(&caretRect, caretX, caretY, caretX + 2, caretY + FONT_HEIGHT);
        g_pD3DXFont->lpVtbl->DrawTextW(g_pD3DXFont, NULL, L"|", -1, &caretRect,
            DT_LEFT | DT_TOP | DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 0));
    }
}

/* UI rendering function - separated from the main rendering function */
static void render_ui(void)
{
    /* Begin rendering UI for this frame */
    nk_input_begin(ctx);

    /* Create the single window for our application */
    if (nk_begin(ctx, "Text Editor", nk_rect(0, 0, current_width, current_height), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_dynamic(ctx, header_height, 1);
        if (use_native_editor)
        {
            nk_label(ctx, "DirectX9 Custom Editor (F2 to switch)", NK_TEXT_LEFT);
        }
        else
        {
            nk_label(ctx, "Nuklear Editor (F2 to switch)", NK_TEXT_LEFT);
        }

        float editor_height = current_height - header_height - status_height;

        if (!use_native_editor)
        {
            nk_layout_row_dynamic(ctx, editor_height - editor_margin * 2, 1);
            nk_edit_buffer(ctx, NK_EDIT_BOX | NK_EDIT_MULTILINE | NK_EDIT_AUTO_SELECT,
                &editor, 0);
        }
        else
        {
            nk_layout_row_dynamic(ctx, editor_height - editor_margin * 2, 1);
            nk_spacing(ctx, 1);
        }

        nk_layout_row_dynamic(ctx, status_height, 1);
        char status_text[256];

        if (use_native_editor)
        {
            sprintf(status_text, "DirectX9 Custom Editor - Full IME Support - %s",
                rendering_paused ? "Input Mode" : "Display Mode");
        }
        else
        {
            sprintf(status_text, "Nuklear Editor - Limited IME Support");
        }

        nk_label(ctx, status_text, NK_TEXT_LEFT);
    }
    nk_end(ctx);

    nk_input_end(ctx);
}

/* Rendering function */
static void render_frame(void)
{
    HRESULT hr;

    if (rendering_paused)
    {
        return;
    }

    DWORD current_time = GetTickCount();
    if (!needs_redraw && (current_time - last_render_time < RENDER_INTERVAL))
    {
        return;
    }

    /* Always process UI before rendering */
    render_ui();

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
        D3DCOLOR_COLORVALUE(0.10f, 0.18f, 0.24f, 1.0f), 0.0f, 0);
    NK_ASSERT(SUCCEEDED(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    NK_ASSERT(SUCCEEDED(hr));

    /* Render Nuklear UI */
    nk_d3d9_render(NK_ANTI_ALIASING_ON);

    /* Render DX9 custom editor if active */
    if (use_native_editor && dx9_editor.visible)
    {
        render_dx9_editor();
    }

    hr = IDirect3DDevice9_EndScene(device);
    NK_ASSERT(SUCCEEDED(hr));

#if !defined(_MSC_VER) || (_MSC_VER > 1200)
    if (deviceEx)
    {
        hr = IDirect3DDevice9Ex_PresentEx(deviceEx, NULL, NULL, NULL, NULL, 0);
    }
    else
    {
        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    }
#else
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
#endif

    if (FAILED(hr))
    {
        if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICEHUNG || hr == D3DERR_DEVICEREMOVED)
        {
            needs_redraw = TRUE;
        }
    }

    /* Clear Nuklear UI state */
    nk_clear(ctx);

    needs_redraw = FALSE;
    last_render_time = current_time;
}

/* Common application execution */
static int run_app(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    struct nk_font* droid = 0;
    WNDCLASS wc;
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    int running = 1;
    MSG msg;

    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = _T("NuklearTextEditorClass");
    RegisterClass(&wc);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);

    wnd = CreateWindowEx(exstyle, wc.lpszClassName, _T("Nuklear + DirectX9 Custom Text Editor"),
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL);

    if (!wnd) {
        MessageBox(NULL, _T("Err"), _T("Error"), MB_OK | MB_ICONERROR);
        return -1;
    }

    create_d3d9_device(wnd); // ここでDirect3Dデバイス初期化

    ctx = nk_d3d9_init(device, WINDOW_WIDTH, WINDOW_HEIGHT);
    {
        struct nk_font_atlas* atlas;
        nk_d3d9_font_stash_begin(&atlas);

        struct nk_font_config config = nk_font_config(0);
        static const nk_rune jp_ranges[] = {
            0x0020, 0x00FF,
            0x3000, 0x30FF,
            0x31F0, 0x31FF,
            0xFF00, 0xFFEF,
            0x4E00, 0x9FAF,
            0x3400, 0x4DBF,
            0
        };
        config.range = jp_ranges;
        config.oversample_h = 1;
        config.oversample_v = 1;
        config.pixel_snap = 0;

        droid = nk_font_atlas_add_from_file(atlas, "C:\\Windows\\Fonts\\msgothic.ttc", 18, &config);

        if (!droid)
        {
            droid = nk_font_atlas_add_from_file(atlas, "C:\\Windows\\Fonts\\meiryo.ttc", 18, &config);
        }

        if (!droid)
        {
            droid = nk_font_atlas_add_default(atlas, 18, &config);
        }

        nk_style_load_all_cursors(ctx, atlas->cursors);
        if (droid)
        {
            nk_style_set_font(ctx, &droid->handle);
        }
        nk_d3d9_font_stash_end();
    }

    nk_textedit_init_fixed(&editor, editor_buffer, MAX_MEMORY);

    while (running)
    {
        /* Event handling */
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        /* Handle editor type toggle */
        if (toggle_editor_type)
        {
            toggle_editor(wnd);
            toggle_editor_type = 0;
        }

        /* Rendering - Builds UI and renders in one pass */
        if (!rendering_paused && needs_redraw)
        {
            render_frame();
        }

        Sleep(1); /* Reduce CPU usage */
    }

    /* Cleanup */
    TextEdit_Cleanup(&dx9_editor);
    nk_textedit_free(&editor);
    nk_d3d9_shutdown();

    if (g_pD3DXFont)
    {
        g_pD3DXFont->lpVtbl->Release(g_pD3DXFont);
        g_pD3DXFont = NULL;
    }

#if !defined(_MSC_VER) || (_MSC_VER > 1200)
    if (deviceEx)
    {
        IDirect3DDevice9Ex_Release(deviceEx);
    }
    else
    {
        IDirect3DDevice9_Release(device);
    }
#else
    IDirect3DDevice9_Release(device);
#endif

    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 0;
}

/* WinMain - Windows GUI application entry point */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return run_app(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

/* main - Console application entry point */
int main(int argc, char* argv[])
{
    /* Allow launching from console mode */
    return run_app(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOWDEFAULT);
}
