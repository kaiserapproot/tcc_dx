/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_D3D9_H_
#define NK_D3D9_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>
/* fallback for environments where <stdint.h> isn't providing UINT16_MAX */
#ifndef UINT16_MAX
typedef unsigned short uint16_t;
#define UINT16_MAX 65535
#endif

typedef struct IDirect3DDevice9 IDirect3DDevice9;

NK_API struct nk_context *nk_d3d9_init(IDirect3DDevice9 *device, int width, int height);
NK_API void nk_d3d9_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_d3d9_font_stash_end(void);
NK_API int nk_d3d9_handle_event(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
NK_API void nk_d3d9_render(enum nk_anti_aliasing);
NK_API void nk_d3d9_release(void);
NK_API void nk_d3d9_resize(int width, int height);
NK_API void nk_d3d9_shutdown(void);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_D3D9_IMPLEMENTATION

#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <d3d9.h>

/* Fallbacks for environments where these might not be defined */
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

struct nk_d3d9_vertex {
    /* D3d9 FFP requires three coordinate position, but nuklear writes only 2 elements
       projection matrix doesn't use z coordinate => so it can be any value.
       Member order here is important! Do not rearrange them! */
    float position[3];
    nk_uchar col[4];
    float uv[2];
};

static struct {
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_buffer cmds;

    struct nk_draw_null_texture tex_null;

    D3DVIEWPORT9 viewport;
    D3DMATRIX projection;
    IDirect3DDevice9 *device;
    IDirect3DTexture9 *texture;
    IDirect3DStateBlock9 *state;
    HWND window;
} d3d9;

NK_API void
nk_d3d9_create_state()
{
    HRESULT hr;

    hr = IDirect3DDevice9_BeginStateBlock(d3d9.device);
    NK_ASSERT(SUCCEEDED(hr));

    /* vertex format */
    IDirect3DDevice9_SetFVF(d3d9.device, D3DFVF_XYZ + D3DFVF_DIFFUSE + D3DFVF_TEX1);

    /* blend state */
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_ALPHABLENDENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_BLENDOP, D3DBLENDOP_ADD);

    /* render state */
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_ZENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_ZWRITEENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(d3d9.device, D3DRS_SCISSORTESTENABLE, TRUE);

    /* sampler state */
    IDirect3DDevice9_SetSamplerState(d3d9.device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    IDirect3DDevice9_SetSamplerState(d3d9.device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    /* Use point filtering for crisp UI text and disable mipmapping */
    IDirect3DDevice9_SetSamplerState(d3d9.device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    IDirect3DDevice9_SetSamplerState(d3d9.device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    IDirect3DDevice9_SetSamplerState(d3d9.device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

    /* texture stage state */
    IDirect3DDevice9_SetTextureStageState(d3d9.device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(d3d9.device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(d3d9.device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    IDirect3DDevice9_SetTextureStageState(d3d9.device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(d3d9.device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(d3d9.device, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

    hr = IDirect3DDevice9_EndStateBlock(d3d9.device, &d3d9.state);
    NK_ASSERT(SUCCEEDED(hr));
}

NK_API void
nk_d3d9_render(enum nk_anti_aliasing AA)
{
    HRESULT hr;

    nk_d3d9_create_state();

    hr = IDirect3DStateBlock9_Apply(d3d9.state);
    NK_ASSERT(SUCCEEDED(hr));

    /* projection matrix */
    IDirect3DDevice9_SetTransform(d3d9.device, D3DTS_PROJECTION, &d3d9.projection);

    /* viewport */
    IDirect3DDevice9_SetViewport(d3d9.device, &d3d9.viewport);

    /* convert from command queue into draw list and draw to screen */
    {
        struct nk_buffer vbuf, ebuf;
        const struct nk_draw_command *cmd;
        const nk_draw_index *offset = NULL;
        UINT vertex_count;

        /* fill converting configuration */
        struct nk_convert_config config;
        NK_STORAGE const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT,    NK_OFFSETOF(struct nk_d3d9_vertex, position)},
            {NK_VERTEX_COLOR,    NK_FORMAT_B8G8R8A8, NK_OFFSETOF(struct nk_d3d9_vertex, col)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT,    NK_OFFSETOF(struct nk_d3d9_vertex, uv)},
            {NK_VERTEX_LAYOUT_END}
        };
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_d3d9_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_d3d9_vertex);
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.tex_null = d3d9.tex_null;

        /* convert shapes into vertexes */
        nk_buffer_init_default(&vbuf);
        nk_buffer_init_default(&ebuf);
        nk_convert(&d3d9.ctx, &d3d9.cmds, &vbuf, &ebuf, &config);

        /* iterate over and execute each draw command */
        offset = (const nk_draw_index *)nk_buffer_memory_const(&ebuf);
        vertex_count = (UINT)vbuf.needed / sizeof(struct nk_d3d9_vertex);

        nk_draw_foreach(cmd, &d3d9.ctx, &d3d9.cmds)
        {
            RECT scissor;
            if (!cmd->elem_count) continue;

            hr = IDirect3DDevice9_SetTexture(d3d9.device, 0, (IDirect3DBaseTexture9 *)cmd->texture.ptr);
            NK_ASSERT(SUCCEEDED(hr));

            scissor.left = (LONG)cmd->clip_rect.x;
            scissor.right = (LONG)(cmd->clip_rect.x + cmd->clip_rect.w);
            scissor.top = (LONG)cmd->clip_rect.y;
            scissor.bottom = (LONG)(cmd->clip_rect.y + cmd->clip_rect.h);

            hr = IDirect3DDevice9_SetScissorRect(d3d9.device, &scissor);
            NK_ASSERT(SUCCEEDED(hr));

            /* Ensure draw index size matches 16-bit index used by D3D9 backend */
            NK_ASSERT(sizeof(nk_draw_index) == sizeof(unsigned short));
            hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(d3d9.device, D3DPT_TRIANGLELIST,
                0, vertex_count, cmd->elem_count/3, offset, D3DFMT_INDEX16,
                nk_buffer_memory_const(&vbuf), sizeof(struct nk_d3d9_vertex));
            NK_ASSERT(SUCCEEDED(hr));
            offset += cmd->elem_count;
        }

        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }

    nk_clear(&d3d9.ctx);
    nk_buffer_clear(&d3d9.cmds);

    IDirect3DStateBlock9_Apply(d3d9.state);
    IDirect3DStateBlock9_Release(d3d9.state);
}

static void
nk_d3d9_get_projection_matrix(int width, int height, float *result)
{
    const float L = 0.5f;
    const float R = (float)width + 0.5f;
    const float T = 0.5f;
    const float B = (float)height + 0.5f;
    float matrix[4][4] = {
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
    };
    matrix[0][0] = 2.0f / (R - L);
    matrix[1][1] = 2.0f / (T - B);
    matrix[3][0] = (R + L) / (L - R);
    matrix[3][1] = (T + B) / (B - T);
    memcpy(result, matrix, sizeof(matrix));
}

NK_API void
nk_d3d9_release(void)
{
    IDirect3DTexture9_Release(d3d9.texture);
}

static void
nk_d3d9_create_font_texture()
{
    int w, h, y;
    const void *image;

    HRESULT hr;
    D3DLOCKED_RECT locked;

    image = nk_font_atlas_bake(&d3d9.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

    hr = IDirect3DDevice9_CreateTexture(d3d9.device, w, h, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &d3d9.texture, NULL);
    NK_ASSERT(SUCCEEDED(hr));

    hr = IDirect3DTexture9_LockRect(d3d9.texture, 0, &locked, NULL, 0);
    NK_ASSERT(SUCCEEDED(hr));

    for (y = 0; y < h; y++) {
        void *src = (char *)image + y * w * 4;
        void *dst = (char *)locked.pBits + y * locked.Pitch;
        memcpy(dst, src, w * 4);
    }

    hr = IDirect3DTexture9_UnlockRect(d3d9.texture, 0);
    NK_ASSERT(SUCCEEDED(hr));

    nk_font_atlas_end(&d3d9.atlas, nk_handle_ptr(d3d9.texture), &d3d9.tex_null);
}

NK_API void
nk_d3d9_resize(int width, int height)
{
    if (d3d9.texture) {
        nk_d3d9_create_font_texture();
    }

    nk_d3d9_create_state();

    nk_d3d9_get_projection_matrix(width, height, &d3d9.projection.m[0][0]);
    d3d9.viewport.Width = width;
    d3d9.viewport.Height = height;
}

NK_API int
nk_d3d9_handle_event(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        int down = !((lparam >> 31) & 1);
        int ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

        switch (wparam)
        {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            nk_input_key(&d3d9.ctx, NK_KEY_SHIFT, down);
            return 1;

        case VK_DELETE:
            nk_input_key(&d3d9.ctx, NK_KEY_DEL, down);
            return 1;

        case VK_RETURN:
        case VK_SEPARATOR:
            nk_input_key(&d3d9.ctx, NK_KEY_ENTER, down);
            return 1;

        case VK_TAB:
            nk_input_key(&d3d9.ctx, NK_KEY_TAB, down);
            return 1;

        case VK_LEFT:
            if (ctrl)
                nk_input_key(&d3d9.ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else
                nk_input_key(&d3d9.ctx, NK_KEY_LEFT, down);
            return 1;

        case VK_RIGHT:
            if (ctrl)
                nk_input_key(&d3d9.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else
                nk_input_key(&d3d9.ctx, NK_KEY_RIGHT, down);
            return 1;

        case VK_UP:
            nk_input_key(&d3d9.ctx, NK_KEY_UP, down);
            return 1;

        case VK_DOWN:
            nk_input_key(&d3d9.ctx, NK_KEY_DOWN, down);
            return 1;

        case VK_BACK:
            nk_input_key(&d3d9.ctx, NK_KEY_BACKSPACE, down);
            return 1;

        case VK_HOME:
            nk_input_key(&d3d9.ctx, NK_KEY_TEXT_START, down);
            nk_input_key(&d3d9.ctx, NK_KEY_SCROLL_START, down);
            return 1;

        case VK_END:
            nk_input_key(&d3d9.ctx, NK_KEY_TEXT_END, down);
            nk_input_key(&d3d9.ctx, NK_KEY_SCROLL_END, down);
            return 1;

        case VK_NEXT:
            nk_input_key(&d3d9.ctx, NK_KEY_SCROLL_DOWN, down);
            return 1;

        case VK_PRIOR:
            nk_input_key(&d3d9.ctx, NK_KEY_SCROLL_UP, down);
            return 1;

        case 'C':
            if (ctrl) {
                nk_input_key(&d3d9.ctx, NK_KEY_COPY, down);
                return 1;
            }
            break;

        case 'V':
            if (ctrl) {
                nk_input_key(&d3d9.ctx, NK_KEY_PASTE, down);
                return 1;
            }
            break;

        case 'X':
            if (ctrl) {
                nk_input_key(&d3d9.ctx, NK_KEY_CUT, down);
                return 1;
            }
            break;

        case 'Z':
            if (ctrl) {
                nk_input_key(&d3d9.ctx, NK_KEY_TEXT_UNDO, down);
                return 1;
            }
            break;

        case 'R':
            if (ctrl) {
                nk_input_key(&d3d9.ctx, NK_KEY_TEXT_REDO, down);
                return 1;
            }
            break;
        }
        return 0;
    }

    case WM_CHAR:
        if (wparam >= 32)
        {
            nk_input_unicode(&d3d9.ctx, (nk_rune)wparam);
            return 1;
        }
        break;

    case WM_LBUTTONDOWN:
        nk_input_button(&d3d9.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
    /* event debug prints removed */
        return 1;

    case WM_LBUTTONUP:
        nk_input_button(&d3d9.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        nk_input_button(&d3d9.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
    /* event debug prints removed */
        return 1;

    case WM_RBUTTONDOWN:
        nk_input_button(&d3d9.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_RBUTTONUP:
        nk_input_button(&d3d9.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_MBUTTONDOWN:
        nk_input_button(&d3d9.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_MBUTTONUP:
        nk_input_button(&d3d9.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_MOUSEWHEEL:
        nk_input_scroll(&d3d9.ctx, nk_vec2(0,(float)(short)HIWORD(wparam) / WHEEL_DELTA));
        return 1;

    case WM_MOUSEMOVE:
    /* event debug prints removed */
        nk_input_motion(&d3d9.ctx, (short)LOWORD(lparam), (short)HIWORD(lparam));
        return 1;

    case WM_LBUTTONDBLCLK:
        nk_input_button(&d3d9.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        return 1;
    }

    return 0;
}

static void
nk_d3d9_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    HGLOBAL mem;
    LPCWSTR wstr;
    int wlen;
    int utf8size;

    (void)usr;
    /* If clipboard doesn't contain unicode text or cannot be opened, bail out */
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(NULL)) {
        return;
    }

    mem = GetClipboardData(CF_UNICODETEXT);
    if (!mem) {
        CloseClipboard();
        return;
    }

    wstr = (LPCWSTR)GlobalLock(mem);
    if (!wstr) {
        CloseClipboard();
        return;
    }

    /* use actual wide-string length (safe) */
    wlen = lstrlenW(wstr);
    if (wlen <= 0) {
        GlobalUnlock(mem);
        CloseClipboard();
        return;
    }

    utf8size = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, NULL, 0, NULL, NULL);
    if (utf8size) {
        char *utf8 = (char *)malloc((size_t)utf8size + 1);
        if (utf8) {
            WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, utf8, utf8size, NULL, NULL);
            utf8[utf8size] = '\0';
            /* sanitize: remove CR (0x0D) which often appears in Windows CRLF and
             * can become visible as replacement glyphs in the editor. */
            {
                int i, j = 0;
                for (i = 0; i < utf8size; ++i) {
                    unsigned char c = (unsigned char)utf8[i];
                    if (c != '\r') utf8[j++] = utf8[i];
                }
                utf8size = j;
                utf8[utf8size] = '\0';
            }

            /* small debug trace to observe payload size on paste (will print to console)
             * Keep this lightweight; user can remove later if unnecessary. */
            /* paste debug print removed */

            nk_textedit_paste(edit, utf8, utf8size);
            free(utf8);
            /* force redraw after paste to ensure new text is rendered */
            if (d3d9.window) {
                InvalidateRect(d3d9.window, NULL, FALSE);
                UpdateWindow(d3d9.window);
            } else {
                HWND aw = GetActiveWindow();
                if (aw) { InvalidateRect(aw, NULL, FALSE); UpdateWindow(aw); }
            }
        }
    }

    GlobalUnlock(mem);
    CloseClipboard();
}

static void
nk_d3d9_clipboard_copy(nk_handle usr, const char *text, int len)
{
    int wsize;
    int use_len;

    (void)usr;
    if (!OpenClipboard(NULL)) {
        return;
    }

    /* determine actual byte length to use (len may be -1 or not reliable) */
    use_len = len;
    if (use_len < 0) {
        use_len = (int)strlen(text);
    } else {
        /* if strlen differs from provided len, the provided len is likely a glyph count
         * so decode UTF-8 to find the byte length covering that many glyphs */
        int actual = (int)strlen(text);
        if (actual != use_len) {
            int cur = 0;
            int pos = 0;
            int byte_len = 0;
            while (cur < use_len && pos < actual) {
                nk_rune rune = 0;
                int clen = nk_utf_decode(text + pos, &rune, actual - pos);
                if (clen <= 0) break;
                pos += clen;
                cur++;
            }
            if (cur == use_len) {
                byte_len = pos;
                use_len = byte_len;
            } else {
                /* fallback to entire strlen if decoding failed */
                /* copy debug print removed */
                use_len = actual;
            }
        }
    }

    /* copy: debug preview removed in production */

            wsize = MultiByteToWideChar(CP_UTF8, 0, text, use_len, NULL, 0);
            if (wsize == 0 && use_len > 0) {
                /* conversion failed for provided length; try null-terminated fallback */
                int actual = (int)strlen(text);
                if (actual != use_len) {
                    use_len = actual;
                    wsize = MultiByteToWideChar(CP_UTF8, 0, text, use_len, NULL, 0);
                }
            }
    if (wsize) {
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (wsize + 1) * sizeof(wchar_t));
        if (mem) {
            wchar_t *wstr = (wchar_t*)GlobalLock(mem);
            if (wstr) {
                MultiByteToWideChar(CP_UTF8, 0, text, use_len, wstr, wsize);
                wstr[wsize] = 0;
                GlobalUnlock(mem);
                EmptyClipboard();
                SetClipboardData(CF_UNICODETEXT, mem);
            }
        }
    }

    CloseClipboard();
}

NK_API struct nk_context*
nk_d3d9_init(IDirect3DDevice9 *device, int width, int height)
{
    d3d9.device = device;
    IDirect3DDevice9_AddRef(device);

    /* store current active window for redraws triggered by clipboard operations */
    d3d9.window = GetActiveWindow();

    nk_init_default(&d3d9.ctx, 0);
    d3d9.state = NULL;
    d3d9.texture = NULL;
    d3d9.ctx.clip.copy = nk_d3d9_clipboard_copy;
    d3d9.ctx.clip.paste = nk_d3d9_clipboard_paste;
    d3d9.ctx.clip.userdata = nk_handle_ptr(0);

    nk_buffer_init_default(&d3d9.cmds);

    /* viewport */
    d3d9.viewport.X = 0;
    d3d9.viewport.Y = 0;
    d3d9.viewport.MinZ = 0.0f;
    d3d9.viewport.MaxZ = 1.0f;

    nk_d3d9_resize(width, height);

    return &d3d9.ctx;
}

NK_API void
nk_d3d9_font_stash_begin(struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&d3d9.atlas);
    nk_font_atlas_begin(&d3d9.atlas);
    *atlas = &d3d9.atlas;
}

NK_API void
nk_d3d9_font_stash_end(void)
{
    nk_d3d9_create_font_texture();

    if (d3d9.atlas.default_font)
        nk_style_set_font(&d3d9.ctx, &d3d9.atlas.default_font->handle);
}

NK_API
void nk_d3d9_shutdown(void)
{
    nk_d3d9_release();

    nk_font_atlas_clear(&d3d9.atlas);
    nk_buffer_free(&d3d9.cmds);
    nk_free(&d3d9.ctx);
}

#endif
