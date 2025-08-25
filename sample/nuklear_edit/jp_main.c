#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <assert.h>
#include <imm.h>
#include <stdlib.h>
#define NK_ASSERT(expr) assert(expr)


#ifndef IDirect3D9_CreateDevice
#define IDirect3D9_CreateDevice(p,a,b,c,d,e,f) (p)->lpVtbl->CreateDevice((p),(a),(b),(c),(d),(e),(f))
#endif
#ifndef IDirect3DDevice9_Clear
#define IDirect3DDevice9_Clear(p,a,b,c,d,e,f) (p)->lpVtbl->Clear((p),(a),(b),(c),(d),(e),(f))
#endif
#ifndef IDirect3DDevice9_BeginScene
#define IDirect3DDevice9_BeginScene(p) (p)->lpVtbl->BeginScene((p))
#endif
#ifndef IDirect3DDevice9_EndScene
#define IDirect3DDevice9_EndScene(p) (p)->lpVtbl->EndScene((p))
#endif
#ifndef IDirect3DDevice9_Present
#define IDirect3DDevice9_Present(p,a,b,c,d) (p)->lpVtbl->Present((p),(a),(b),(c),(d))
#endif
#ifndef IDirect3DDevice9_Reset
#define IDirect3DDevice9_Reset(p,a) (p)->lpVtbl->Reset((p),(a))
#endif
#ifndef IDirect3DDevice9_Release
#define IDirect3DDevice9_Release(p) (p)->lpVtbl->Release((p))
#endif
#ifndef IDirect3D9_Release
#define IDirect3D9_Release(p) (p)->lpVtbl->Release((p))
#endif

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
 char text_buffer_sjis[256] = "日本語入力テスト";
static char multiline_buffer_sjis[1024] = "複数行テキストエディタ\n日本語入力テスト\n複数行での日本語入力が可能です。";
char text_buffer_utf8[512] = "";
char multiline_buffer_utf8[2048] = "";
// ウィンドウの設定
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// デバッグ出力を矢印キー押下時のみ有効にするためのフラグとマクロ
static int debug_log_on_keypress = 0;
#// OSレベルの矢印キー状態（フォールバック）
static int os_key_up = 0;
static int os_key_down = 0;
static int os_key_left = 0;
static int os_key_right = 0;
// 前フレームのキー状態（エッジ検出用）
static int prev_key_up = 0;
static int prev_key_down = 0;
static int prev_key_left = 0;
static int prev_key_right = 0;
#ifndef DBG_PRINTF
#define DBG_PRINTF(...) do { if (debug_log_on_keypress) printf(__VA_ARGS__); } while(0)
#endif

// グローバル変数
static IDirect3D9* d3d = NULL;
static IDirect3DDevice9* device = NULL;
static D3DPRESENT_PARAMETERS present = { 0 };
static struct nk_context* ctx;
static struct nk_font_atlas* atlas;
// フレームカウンタ（同フレームの重複計算抑制用）
static int global_frame_counter = 0;

// 日本語テキストサンプル
static const char* japanese_text = "こんにちは、世界！";
static const char* mixed_text = "Hello こんにちは 世界 World!";
static const char* sample_texts[] = {
    "日本語テキストの表示テスト",
    "ひらがな、カタカナ、漢字",
    "プログラミング言語：C言語",
    "Nuklearライブラリ使用例",
    "文字コード：UTF-8"
};
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

// フォントの設定
static struct nk_font* japanese_font = NULL;

// IME位置制御用の変数
static HWND main_window = NULL;
static float last_ime_x = 0;
static float last_ime_y = 0;

// 複数行カーソル位置情報の構造体
typedef struct
{
    int line;
    int column_chars;
    int line_start_byte;
    int cursor_byte_in_line;
    float line_width;
} cursor_position_info;




// 修正版IME位置更新関数
void update_ime_position_simple(HWND wnd, float x, float y)
{
    HIMC himc = ImmGetContext(wnd);
    if (!himc) return;

    // 位置を記録（他の場所からも使用するため）
    last_ime_x = x;
    last_ime_y = y;

    // IME変換ウィンドウの位置を設定（テキスト入力位置）
    COMPOSITIONFORM cf = { 0 };
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = (LONG)x;
    cf.ptCurrentPos.y = (LONG)y;
    ImmSetCompositionWindow(himc, &cf);

    // 候補ウィンドウの位置を設定（テキストフィールドの下に確実に配置）
    CANDIDATEFORM cand = { 0 };
    cand.dwIndex = 0;
    cand.dwStyle = CFS_CANDIDATEPOS;
    cand.ptCurrentPos.x = (LONG)x;
    cand.ptCurrentPos.y = (LONG)(y + 30); // テキストフィールドの高さ(25) + マージン(5)
    ImmSetCandidateWindow(himc, &cand);

    // デバッグ用：位置情報を出力
    printf("[IME設定] 変換ウィンドウ(%d, %d) 候補ウィンドウ(%d, %d)\n",
        (int)x, (int)y, (int)x, (int)(y + 30));

    ImmReleaseContext(wnd, himc);
}
static int need_reset = 0; // リサイズ後のリソース再作成フラグ
// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
        // IMEからの日本語入力をUTF-8に変換してテキストエディタに挿入
        extern char text_buffer_utf8[512];
        extern char multiline_buffer_utf8[2048];
        static int single_line_active = 0;
        static int multi_line_active = 0;
        // 1行・複数行どちらがアクティブかはdraw_gui_line_fixedで更新される

    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (device) {
            UINT width = LOWORD(lparam);
            UINT height = HIWORD(lparam);
            // ウィンドウが最小化されている場合は何もしない
            if (width == 0 || height == 0) {
                break;
            }
            if (width != present.BackBufferWidth || height != present.BackBufferHeight) {
                nk_d3d9_release(); // テクスチャ等を解放
                present.BackBufferWidth = width;
                present.BackBufferHeight = height;
                need_reset = 1; // mainループでリソース再作成を指示
            }
        }
        break;
    case WM_KEYDOWN:
        switch (wparam) {
        case VK_UP: os_key_up = 1; break;
        case VK_DOWN: os_key_down = 1; break;
        case VK_LEFT: os_key_left = 1; break;
        case VK_RIGHT: os_key_right = 1; break;
        }
        break;
    case WM_KEYUP:
        switch (wparam) {
        case VK_UP: os_key_up = 0; break;
        case VK_DOWN: os_key_down = 0; break;
        case VK_LEFT: os_key_left = 0; break;
        case VK_RIGHT: os_key_right = 0; break;
        }
        break;
    case WM_CHAR:
        {
            if (single_line_active) {
                if ((unsigned char)wparam >= 0x20 && (unsigned char)wparam <= 0x7E) {
                    size_t len = strlen(text_buffer_utf8);
                    if (len < sizeof(text_buffer_utf8) - 1) {
                        text_buffer_utf8[len] = (char)wparam;
                        text_buffer_utf8[len + 1] = '\0';
                    }
                } else {
                    char sjis_char[3] = {0};
                    sjis_char[0] = (char)wparam;
                    char utf8_char[8];
                    sjis_to_utf8(sjis_char, utf8_char, sizeof(utf8_char));
                    strncat(text_buffer_utf8, utf8_char, sizeof(text_buffer_utf8) - strlen(text_buffer_utf8) - 1);
                }
            } else if (multi_line_active) {
                if ((unsigned char)wparam >= 0x20 && (unsigned char)wparam <= 0x7E) {
                    size_t len = strlen(multiline_buffer_utf8);
                    if (len < sizeof(multiline_buffer_utf8) - 1) {
                        multiline_buffer_utf8[len] = (char)wparam;
                        multiline_buffer_utf8[len + 1] = '\0';
                    }
                } else {
                    char sjis_char[3] = {0};
                    sjis_char[0] = (char)wparam;
                    char utf8_char[8];
                    sjis_to_utf8(sjis_char, utf8_char, sizeof(utf8_char));
                    strncat(multiline_buffer_utf8, utf8_char, sizeof(multiline_buffer_utf8) - strlen(multiline_buffer_utf8) - 1);
                }
            }
        }
        break;
    case WM_IME_STARTCOMPOSITION:
        DBG_PRINTF("[IME] 開始\n");
        if (last_ime_x > 0 || last_ime_y > 0)
        {
            update_ime_position_simple(wnd, last_ime_x, last_ime_y);
        }
        return DefWindowProcW(wnd, msg, wparam, lparam);
    case WM_IME_COMPOSITION:
        DBG_PRINTF("[IME] 変換中\n");
        if (last_ime_x > 0 || last_ime_y > 0)
        {
            update_ime_position_simple(wnd, last_ime_x, last_ime_y);
        }
        return DefWindowProcW(wnd, msg, wparam, lparam);
    }

    if (nk_d3d9_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

// Direct3D初期化
int init_d3d(HWND wnd)
{
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return 0;

    ZeroMemory(&present, sizeof(present));
    present.Windowed = TRUE;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.BackBufferFormat = D3DFMT_UNKNOWN;
    present.EnableAutoDepthStencil = TRUE;
    present.AutoDepthStencilFormat = D3DFMT_D16;
    present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    if (FAILED(IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &present, &device)))
    {
        return 0;
    }

    return 1;
}

// 日本語フォントの読み込み
int load_japanese_font()
{
    FILE* debugFile = fopen("debug_font.txt", "w");
    if (debugFile)
    {
        fprintf(debugFile, "フォント読み込み処理を開始します\n");
        fflush(debugFile);
    }

    struct nk_font_atlas* local_atlas = NULL;
    nk_d3d9_font_stash_begin(&local_atlas);
    if (!local_atlas)
    {
        printf("フォントアトラスの初期化に失敗しました\n");
        if (debugFile)
        {
            fprintf(debugFile, "フォントアトラスの初期化に失敗しました\n");
            fclose(debugFile);
        }
        return 0;
    }

    atlas = local_atlas;

    char cwd[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, cwd))
    {
        printf("現在の作業ディレクトリ: %s\n", cwd);
        if (debugFile) fprintf(debugFile, "現在の作業ディレクトリ: %s\n", cwd);
    }

    struct nk_font* default_font = nk_font_atlas_add_default(atlas, 14, NULL);
    if (!default_font)
    {
        printf("デフォルトフォントの追加に失敗しました\n");
        if (debugFile)
        {
            fprintf(debugFile, "デフォルトフォントの追加に失敗しました\n");
            fclose(debugFile);
        }
        nk_d3d9_font_stash_end();
        atlas = NULL;
        return 0;
    }

    struct nk_font_config config = nk_font_config(14);

    static const nk_rune japanese_ranges[] = {
        0x0020, 0x00FF,  // 基本ラテン文字
        0x3000, 0x30FF,  // 句読点、ひらがな、カタカナ
        0x4E00, 0x9FBF,  // CJK統合漢字の一部
        0x5200, 0x52FF,  // 刀, 力 など
        0x5300, 0x53FF,  // 匕, 北 など  
        0x5400, 0x54FF,  // 半, 協 など
        0x5900, 0x59FF,  // 変, 収 など
        0x6E00, 0x6EFF,  // 測, 海 など
        0x6F00, 0x6FFF,  // 準, 漢 など
        0x7000, 0x70FF,  // 無, 焼 など
        0x7400, 0x74FF,  // 球, 理 など 
        0x8000, 0x80FF,  // 般, 船 など
        0x8100, 0x81FF,  // 色, 花 など
        0xFF00, 0xFFEF,  // 全角英数字
        0
    };

    config.range = japanese_ranges;
    config.oversample_h = 1;
    config.oversample_v = 1;
    config.pixel_snap = 0;

    const char* font_paths[] = {
        "./MPLUS1p-Light.ttf",
        "./SawarabiGothic-Regular.ttf",
        "C:\\Windows\\Fonts\\msgothic.ttc",
        "C:\\Windows\\Fonts\\meiryo.ttc"
    };

    for (int i = 0; i < sizeof(font_paths) / sizeof(font_paths[0]); i++)
    {
        printf("フォントファイルを開こうとしています: %s\n", font_paths[i]);
        if (debugFile)
        {
            fprintf(debugFile, "フォントファイルを開こうとしています: %s\n", font_paths[i]);
            fflush(debugFile);
        }

        FILE* font_file = fopen(font_paths[i], "rb");
        if (font_file)
        {
            fseek(font_file, 0, SEEK_END);
            long font_size = ftell(font_file);
            fseek(font_file, 0, SEEK_SET);

            if (font_size > 0)
            {
                printf("フォントファイルを開きました: %s (サイズ: %ld バイト)\n", font_paths[i], font_size);
                if (debugFile)
                {
                    fprintf(debugFile, "フォントファイルを開きました: %s (サイズ: %ld バイト)\n", font_paths[i], font_size);
                    fflush(debugFile);
                }

                void* font_data = malloc(font_size);
                if (font_data && fread(font_data, 1, font_size, font_file) == font_size)
                {
                    printf("フォントデータを読み込みました\n");

                    japanese_font = nk_font_atlas_add_from_memory(local_atlas, font_data, font_size, 12, &config);
                    fclose(font_file);

                    if (japanese_font)
                    {
                        printf("日本語フォントを正常に読み込みました: %s\n", font_paths[i]);
                        if (debugFile)
                        {
                            fprintf(debugFile, "日本語フォントを正常に読み込みました: %s\n", font_paths[i]);
                            fflush(debugFile);
                        }
                        break;
                    }
                    else
                    {
                        printf("nk_font_atlas_add_from_memory が失敗しました\n");
                        if (debugFile)
                        {
                            fprintf(debugFile, "nk_font_atlas_add_from_memory が失敗しました\n");
                            fflush(debugFile);
                        }
                        free(font_data);
                    }
                }
                else
                {
                    printf("フォントデータの読み込みに失敗しました\n");
                    if (font_data) free(font_data);
                }
            }
            fclose(font_file);
        }
    }

    if (!japanese_font)
    {
        printf("日本語フォントの読み込みに失敗しました。デフォルトフォントを使用します。\n");
        if (debugFile)
        {
            fprintf(debugFile, "日本語フォントの読み込みに失敗しました。デフォルトフォントを使用します。\n");
            fflush(debugFile);
        }
        japanese_font = default_font;
    }

    printf("nk_d3d9_font_stash_end を呼び出します\n");
    nk_d3d9_font_stash_end();
    printf("nk_d3d9_font_stash_end 呼び出し完了\n");

    if (japanese_font)
    {
        printf("フォントを設定します\n");
        nk_style_set_font(ctx, &japanese_font->handle);
    }

    if (debugFile)
    {
        fprintf(debugFile, "フォント読み込み処理を完了しました\n");
        fclose(debugFile);
    }

    return japanese_font != NULL;
}

// Nuklearの内部テキスト編集状態をより詳しく取得する関数
struct detailed_cursor_info
{
    int cursor_byte_pos;        // バイト単位でのカーソル位置
    int line_number;           // 行番号（0から開始）
    int column_in_line;        // 行内での文字位置
    int line_start_byte;       // 行の開始バイト位置
    int cursor_byte_in_line;   // 行内でのバイト位置
    float text_width_to_cursor;// カーソル位置までのピクセル幅
    float actual_line_height;  // 実際の行の高さ
    struct nk_vec2 scroll_offset; // スクロールオフセット
};

// デバッグ用：テキスト内容を詳細に表示する関数
void debug_text_content(const char* text)
{
    DBG_PRINTF("=== テキスト内容詳細分析 ===\n");
    if (!text) {
        DBG_PRINTF("(null)\n");
        DBG_PRINTF("=== テキスト内容分析完了 ===\n");
        return;
    }
    int len = (int)strlen(text);
    // 単一行の要約として、各文字の範囲と表示をカンマ区切りで作成（長過ぎたら切る）
    char out[4096];
    int out_used = 0;
    out[0] = '\0';
    for (int i = 0; i < len; ) {
        if (out_used > (int)sizeof(out) - 100) break; // バッファ安全策
        unsigned char uc = (unsigned char)text[i];
        if (uc == '\n') {
            int n = snprintf(out + out_used, sizeof(out) - out_used, "[%d:'\\n'],",
                i);
            out_used += (n > 0 ? n : 0);
            i++;
            continue;
        }
        if (uc >= 32 && uc <= 126) {
            int n = snprintf(out + out_used, sizeof(out) - out_used, "[%d:'%c'],",
                i, text[i]);
            out_used += (n > 0 ? n : 0);
            i++;
            continue;
        }
        int char_len = 1;
        if ((uc & 0xE0) == 0xC0) char_len = 2;
        else if ((uc & 0xF0) == 0xE0) char_len = 3;
        else if ((uc & 0xF8) == 0xF0) char_len = 4;
        if (i + char_len <= len) {
            // 安全に部分文字列を表示
            int n = snprintf(out + out_used, sizeof(out) - out_used, "[%d-%d:'%.*s'(%d)],",
                i, i + char_len - 1, char_len, text + i, char_len);
            out_used += (n > 0 ? n : 0);
            i += char_len;
        } else {
            break;
        }
    }
    DBG_PRINTF("%s\n", out);
    DBG_PRINTF("=== テキスト内容分析完了 ===\n");
}

// 修正版：正確な行・列計算
struct detailed_cursor_info get_detailed_cursor_info_fixed(struct nk_context* ctx, const char* text, int is_multiline)
{
    struct detailed_cursor_info info = { 0 };

    if (!ctx || !text) return info;

    // キャッシュ：同一フレームかつ同一テキストへの再呼び出しは前回結果を返す
    static int cached_frame = -1;
    static const char* cached_text = NULL;
    static struct detailed_cursor_info cached_info;
    if (cached_frame == global_frame_counter && cached_text == text) {
        return cached_info;
    }

    struct nk_text_edit* edit = &ctx->text_edit;
    const struct nk_user_font* font = ctx->style.font;

    if (!font) return info;

    // *** 重要な修正: Nuklearの文字数カーソル位置をバイト位置に変換 ***
    int nuklear_cursor_char_pos = edit->cursor;  // Nuklearのカーソル位置（文字数）
    int text_len = (int)strlen(text);
    
    DBG_PRINTF("DEBUG: Nuklearカーソル位置(文字数) = %d\n", nuklear_cursor_char_pos);
    DBG_PRINTF("DEBUG: テキスト長(バイト) = %d\n", text_len);
    
    // 文字数をバイト位置に変換
    info.cursor_byte_pos = 0;
    int current_char_index = 0;
    
    while (current_char_index < nuklear_cursor_char_pos && info.cursor_byte_pos < text_len) {
        nk_rune rune;
        int char_byte_len = nk_utf_decode(text + info.cursor_byte_pos, &rune, text_len - info.cursor_byte_pos);
        if (char_byte_len <= 0) break;
        
        info.cursor_byte_pos += char_byte_len;
        current_char_index++;
    }
    
    DBG_PRINTF("DEBUG: 変換結果: 文字位置 %d -> バイト位置 %d\n", nuklear_cursor_char_pos, info.cursor_byte_pos);

    // バイト位置を範囲内に制限
    if (info.cursor_byte_pos > text_len) info.cursor_byte_pos = text_len;
    if (info.cursor_byte_pos < 0) info.cursor_byte_pos = 0;

    // UTF-8文字境界に調整
    while (info.cursor_byte_pos > 0 && (text[info.cursor_byte_pos] & 0xC0) == 0x80)
    {
        info.cursor_byte_pos--;
    }

    DBG_PRINTF("DEBUG: 最終カーソル位置 = %d\n", info.cursor_byte_pos);

    // デバッグ用：テキスト内容を表示（初回のみ）
    static int first_call = 1;
    if (first_call && is_multiline && debug_log_on_keypress)
    {
        debug_text_content(text);
        first_call = 0;
    }

    // 実際の行高を計算
    struct nk_style_edit* style = &ctx->style.edit;
    info.actual_line_height = font->height + style->row_padding;

    DBG_PRINTF("DEBUG: フォント高さ=%.2f, row_padding=%.2f, 実際行高=%.2f\n",
        font->height, style->row_padding, info.actual_line_height);

    if (is_multiline)
    {
        // 修正版：正確な行・列計算
        info.line_number = 0;
        info.column_in_line = 0;
        info.line_start_byte = 0;

    DBG_PRINTF("DEBUG: 行計算開始 - カーソル位置 %d まで走査\n", info.cursor_byte_pos);

        // カーソル位置まで1バイトずつ走査
        for (int i = 0; i < info.cursor_byte_pos && i < text_len; i++)
        {
            if (text[i] == '\n')
            {
                info.line_number++;
                info.column_in_line = 0;
                info.line_start_byte = i + 1;
                DBG_PRINTF("DEBUG: 改行発見 - バイト位置 %d, 新しい行=%d, 行開始=%d\n",
                    i, info.line_number, info.line_start_byte);
            }
            else
            {
                info.column_in_line++;
            }
        }

        info.cursor_byte_in_line = info.cursor_byte_pos - info.line_start_byte;

        DBG_PRINTF("DEBUG: 最終結果 - 行=%d, 行内列=%d, 行開始バイト=%d, 行内バイト位置=%d\n",
            info.line_number, info.column_in_line, info.line_start_byte, info.cursor_byte_in_line);
    }
    else
    {
        // 1行テキストの場合
        info.line_number = 0;
        info.line_start_byte = 0;
        info.cursor_byte_in_line = info.cursor_byte_pos;
    }

    // カーソル位置までの正確な幅を計算
    info.text_width_to_cursor = 0.0f;
    int pos = info.line_start_byte;

    DBG_PRINTF("DEBUG: 幅計算開始 - 行開始=%d から カーソル=%d まで\n", info.line_start_byte, info.cursor_byte_pos);
    // 対象テキストの要約を作成（長すぎる場合は切り詰め）
    {
        int seg_len = info.cursor_byte_pos - info.line_start_byte;
        if (seg_len > 64) seg_len = 64;
        DBG_PRINTF("DEBUG: 対象テキスト: '%.*s'\n", seg_len, text + info.line_start_byte);
    }

    while (pos < info.cursor_byte_pos && text[pos] != '\0' && text[pos] != '\n')
    {
        // UTF-8文字の長さを判定
        unsigned char c = (unsigned char)text[pos];
        int char_len = 1;

        if ((c & 0x80) == 0x00) char_len = 1;      // ASCII
        else if ((c & 0xE0) == 0xC0) char_len = 2; // 2バイト
        else if ((c & 0xF0) == 0xE0) char_len = 3; // 3バイト（日本語）
        else if ((c & 0xF8) == 0xF0) char_len = 4; // 4バイト

        // 文字境界を確認
        if (pos + char_len > info.cursor_byte_pos)
        {
            DBG_PRINTF("DEBUG: 文字境界で停止 - pos=%d, char_len=%d, cursor=%d\n",
                pos, char_len, info.cursor_byte_pos);
            break;
        }

        float char_width = font->width(font->userdata, font->height, text + pos, char_len);
        info.text_width_to_cursor += char_width;
        pos += char_len;
    }

    DBG_PRINTF("DEBUG: 計算された幅 = %.2f\n", info.text_width_to_cursor);

    // スクロールオフセット（現在は0）
    info.scroll_offset.x = 0;
    info.scroll_offset.y = 0;

    // キャッシュに保存して同フレーム内の重複呼び出しを防止
    cached_frame = global_frame_counter;
    cached_text = text;
    cached_info = info;

    return cached_info;
}

// 修正版：複数行テキスト用IME位置計算
void update_ime_position_multiline_line_fixed(struct nk_context* ctx, HWND hwnd,
    const char* text, struct nk_rect edit_bounds)
{
    if (!ctx || !hwnd || !text) return;
    DBG_PRINTF("\n=== 複数行テキスト IME位置計算開始（行修正版） ===\n");

    struct detailed_cursor_info cursor_info = get_detailed_cursor_info_fixed(ctx, text, 1);
    struct nk_style_edit* style = &ctx->style.edit;

    // 実際のテキストコンテンツ領域の計算
    float content_x = edit_bounds.x + style->border + style->padding.x;
    float content_y = edit_bounds.y + style->border + style->padding.y;

    DBG_PRINTF("DEBUG: ウィジェット境界 x=%.2f, y=%.2f, w=%.2f, h=%.2f\n",
        edit_bounds.x, edit_bounds.y, edit_bounds.w, edit_bounds.h);
    DBG_PRINTF("DEBUG: border=%.2f, padding.x=%.2f, padding.y=%.2f\n",
        style->border, style->padding.x, style->padding.y);
    DBG_PRINTF("DEBUG: コンテンツ領域 x=%.2f, y=%.2f\n", content_x, content_y);

    // IME位置の計算（行を正しく考慮）
    float ime_x = content_x + cursor_info.text_width_to_cursor - cursor_info.scroll_offset.x;
    float ime_y = content_y + (cursor_info.line_number * cursor_info.actual_line_height) - cursor_info.scroll_offset.y;

    DBG_PRINTF("DEBUG: IME計算 - content_y=%.2f + (行%d × 行高%.2f) = %.2f\n",
        content_y, cursor_info.line_number, cursor_info.actual_line_height, ime_y);

    // テキストボックスの境界内に調整
    float max_y = edit_bounds.y + edit_bounds.h - cursor_info.actual_line_height - style->padding.y - style->border;
    if (ime_y > max_y)
    {
    DBG_PRINTF("DEBUG: IME Y位置を調整 %.2f -> %.2f\n", ime_y, max_y);
        ime_y = max_y;
    }

    float max_x = edit_bounds.x + edit_bounds.w - style->padding.x - style->border - 20; // マージン
    if (ime_x > max_x)
    {
    DBG_PRINTF("DEBUG: IME X位置を調整 %.2f -> %.2f\n", ime_x, max_x);
        ime_x = max_x;
    }

    // 最小位置制限
    if (ime_x < content_x) ime_x = content_x;
    if (ime_y < content_y) ime_y = content_y;

    DBG_PRINTF("DEBUG: 最終IME位置 x=%.2f, y=%.2f (行=%d, 行内幅=%.2f)\n",
        ime_x, ime_y, cursor_info.line_number, cursor_info.text_width_to_cursor);

    // IME設定
    HIMC himc = ImmGetContext(hwnd);
    if (himc)
    {
        COMPOSITIONFORM cf = { 0 };
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = (LONG)ime_x;
        cf.ptCurrentPos.y = (LONG)ime_y;

        BOOL comp_result = ImmSetCompositionWindow(himc, &cf);
        DBG_PRINTF("DEBUG: ImmSetCompositionWindow 位置(%ld, %ld) 結果 = %s\n",
            cf.ptCurrentPos.x, cf.ptCurrentPos.y, comp_result ? "成功" : "失敗");

        CANDIDATEFORM cand = { 0 };
        cand.dwIndex = 0;
        cand.dwStyle = CFS_CANDIDATEPOS;
        cand.ptCurrentPos.x = (LONG)ime_x;
        cand.ptCurrentPos.y = (LONG)(ime_y + cursor_info.actual_line_height + 2);

        BOOL cand_result = ImmSetCandidateWindow(himc, &cand);
        DBG_PRINTF("DEBUG: ImmSetCandidateWindow 位置(%ld, %ld) 結果 = %s\n",
            cand.ptCurrentPos.x, cand.ptCurrentPos.y, cand_result ? "成功" : "失敗");

        ImmReleaseContext(hwnd, himc);
    }

    DBG_PRINTF("=== 複数行テキスト IME位置計算完了（行修正版） ===\n\n");
}

// 上で計算済みの cursor_info を再利用する版（重複計算と重複ログを避ける）
void update_ime_position_multiline_with_info(struct nk_context* ctx, HWND hwnd,
    const char* text, struct nk_rect edit_bounds, const struct detailed_cursor_info* cursor_info)
{
    if (!ctx || !hwnd || !text || !cursor_info) return;

    DBG_PRINTF("\n=== 複数行テキスト IME位置計算開始（行修正版・既計算情報） ===\n");

    const struct detailed_cursor_info* info = cursor_info;
    struct nk_style_edit* style = &ctx->style.edit;

    // 実際のテキストコンテンツ領域の計算
    float content_x = edit_bounds.x + style->border + style->padding.x;
    float content_y = edit_bounds.y + style->border + style->padding.y;

    DBG_PRINTF("DEBUG: コンテンツ領域 x=%.2f, y=%.2f\n", content_x, content_y);

    // IME位置の計算（行を正しく考慮）
    float ime_x = content_x + info->text_width_to_cursor - info->scroll_offset.x;
    float ime_y = content_y + (info->line_number * info->actual_line_height) - info->scroll_offset.y;

    // テキストボックスの境界内に調整
    float max_y = edit_bounds.y + edit_bounds.h - info->actual_line_height - style->padding.y - style->border;
    if (ime_y > max_y) ime_y = max_y;

    float max_x = edit_bounds.x + edit_bounds.w - style->padding.x - style->border - 20; // マージン
    if (ime_x > max_x) ime_x = max_x;

    if (ime_x < content_x) ime_x = content_x;
    if (ime_y < content_y) ime_y = content_y;

    DBG_PRINTF("DEBUG: 最終IME位置 x=%.2f, y=%.2f (行=%d, 行内幅=%.2f)\n",
        ime_x, ime_y, info->line_number, info->text_width_to_cursor);

    // IME設定
    HIMC himc = ImmGetContext(hwnd);
    if (himc)
    {
        COMPOSITIONFORM cf = { 0 };
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = (LONG)ime_x;
        cf.ptCurrentPos.y = (LONG)ime_y;

        BOOL comp_result = ImmSetCompositionWindow(himc, &cf);
        DBG_PRINTF("DEBUG: ImmSetCompositionWindow 位置(%ld, %ld) 結果 = %s\n",
            cf.ptCurrentPos.x, cf.ptCurrentPos.y, comp_result ? "成功" : "失敗");

        CANDIDATEFORM cand = { 0 };
        cand.dwIndex = 0;
        cand.dwStyle = CFS_CANDIDATEPOS;
        cand.ptCurrentPos.x = (LONG)ime_x;
        cand.ptCurrentPos.y = (LONG)(ime_y + info->actual_line_height + 2);

        BOOL cand_result = ImmSetCandidateWindow(himc, &cand);
        DBG_PRINTF("DEBUG: ImmSetCandidateWindow 位置(%ld, %ld) 結果 = %s\n",
            cand.ptCurrentPos.x, cand.ptCurrentPos.y, cand_result ? "成功" : "失敗");

        ImmReleaseContext(hwnd, himc);
    }

    DBG_PRINTF("=== 複数行テキスト IME位置計算完了（行修正版・既計算情報） ===\n\n");
}

// 修正版：1行テキスト用IME位置計算
void update_ime_position_single_line_line_fixed(struct nk_context* ctx, HWND hwnd,
    const char* text, struct nk_rect edit_bounds)
{
    if (!ctx || !hwnd || !text) return;
    DBG_PRINTF("\n=== 1行テキスト IME位置計算開始（行修正版） ===\n");

    struct detailed_cursor_info cursor_info = get_detailed_cursor_info_fixed(ctx, text, 0);
    struct nk_style_edit* style = &ctx->style.edit;

    // 実際のテキストコンテンツ領域の計算
    float content_x = edit_bounds.x + style->border + style->padding.x;
    float content_y = edit_bounds.y + style->border + style->padding.y;

    DBG_PRINTF("DEBUG: ウィジェット境界 x=%.2f, y=%.2f, w=%.2f, h=%.2f\n",
        edit_bounds.x, edit_bounds.y, edit_bounds.w, edit_bounds.h);
    DBG_PRINTF("DEBUG: コンテンツ領域 x=%.2f, y=%.2f\n", content_x, content_y);

    // IME位置の計算
    float ime_x = content_x + cursor_info.text_width_to_cursor - cursor_info.scroll_offset.x;
    float ime_y = content_y - cursor_info.scroll_offset.y;

    DBG_PRINTF("DEBUG: IME位置 x=%.2f, y=%.2f\n", ime_x, ime_y);

    // IME設定
    HIMC himc = ImmGetContext(hwnd);
    if (himc)
    {
        COMPOSITIONFORM cf = { 0 };
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = (LONG)ime_x;
        cf.ptCurrentPos.y = (LONG)ime_y;

        BOOL comp_result = ImmSetCompositionWindow(himc, &cf);
        DBG_PRINTF("DEBUG: ImmSetCompositionWindow 結果 = %s\n", comp_result ? "成功" : "失敗");

        CANDIDATEFORM cand = { 0 };
        cand.dwIndex = 0;
        cand.dwStyle = CFS_CANDIDATEPOS;
        cand.ptCurrentPos.x = (LONG)ime_x;
        cand.ptCurrentPos.y = (LONG)(ime_y + cursor_info.actual_line_height + 2);

        BOOL cand_result = ImmSetCandidateWindow(himc, &cand);
        DBG_PRINTF("DEBUG: ImmSetCandidateWindow 結果 = %s\n", cand_result ? "成功" : "失敗");

        ImmReleaseContext(hwnd, himc);
    }

    DBG_PRINTF("=== 1行テキスト IME位置計算完了（行修正版） ===\n\n");
}

// 既計算 cursor_info を使う版（1行）
void update_ime_position_single_with_info(struct nk_context* ctx, HWND hwnd,
    const char* text, struct nk_rect edit_bounds, const struct detailed_cursor_info* cursor_info)
{
    if (!ctx || !hwnd || !text || !cursor_info) return;

    struct nk_style_edit* style = &ctx->style.edit;
    float content_x = edit_bounds.x + style->border + style->padding.x;
    float content_y = edit_bounds.y + style->border + style->padding.y;

    float ime_x = content_x + cursor_info->text_width_to_cursor - cursor_info->scroll_offset.x;
    float ime_y = content_y - cursor_info->scroll_offset.y;

    HIMC himc = ImmGetContext(hwnd);
    if (himc)
    {
        COMPOSITIONFORM cf = { 0 };
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = (LONG)ime_x;
        cf.ptCurrentPos.y = (LONG)ime_y;
        ImmSetCompositionWindow(himc, &cf);

        CANDIDATEFORM cand = { 0 };
        cand.dwIndex = 0;
        cand.dwStyle = CFS_CANDIDATEPOS;
        cand.ptCurrentPos.x = (LONG)ime_x;
        cand.ptCurrentPos.y = (LONG)(ime_y + cursor_info->actual_line_height + 2);
        ImmSetCandidateWindow(himc, &cand);

        ImmReleaseContext(hwnd, himc);
    }
}

// 行修正版：draw_gui関数
void draw_gui_line_fixed()
{
    // グローバルUTF-8バッファを直接使用（初期化時のみSJIS→UTF-8変換）
    extern char text_buffer_utf8[512];
    extern char multiline_buffer_utf8[2048];
    // 初期化時のみsjis_to_utf8で変換し、以降は上書きしない
    // （main関数の初期化直後に1回だけ変換するようにしてください）

    // 前回の状態を追跡する変数
    static int prev_single_cursor = -1;
    static int prev_multi_cursor = -1;
    static int single_line_active = 0;
    static int multi_line_active = 0;

    // メインウィンドウ
    char window_title_sjis[] = "日本語表示サンプル (行修正版)";
    char window_title_utf8[128];
    sjis_to_utf8(window_title_sjis, window_title_utf8, sizeof(window_title_utf8));
    // 毎フレームウィンドウサイズを強制的に固定化
    nk_window_set_size(ctx, window_title_utf8, nk_vec2(700, 550));
    if (nk_begin(ctx, window_title_utf8, nk_rect(50, 50, 700, 550),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
       // NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
    // タイトル
    char demo_label_sjis[] = "Nuklear D3D9 日本語表示デモ (行修正版)";
    char demo_label_utf8[128];
    sjis_to_utf8(demo_label_sjis, demo_label_utf8, sizeof(demo_label_utf8));
    nk_layout_row_static(ctx, 30, 680, 1);
    nk_label(ctx, demo_label_utf8, NK_TEXT_CENTERED);

    // デバッグログは矢印キーの押下エッジでのみ出力する（押しっぱなしは抑制）
        int curr_key_up = nk_input_is_key_pressed(&ctx->input, NK_KEY_UP) || os_key_up;
        int curr_key_down = nk_input_is_key_pressed(&ctx->input, NK_KEY_DOWN) || os_key_down;
        int curr_key_left = nk_input_is_key_pressed(&ctx->input, NK_KEY_LEFT) || os_key_left;
        int curr_key_right = nk_input_is_key_pressed(&ctx->input, NK_KEY_RIGHT) || os_key_right;

        debug_log_on_keypress = (curr_key_up && !prev_key_up) || (curr_key_down && !prev_key_down) || (curr_key_left && !prev_key_left) || (curr_key_right && !prev_key_right);

        // 基本的な日本語テキスト表示
    char hello_sjis[] = "こんにちは、世界！";
    char hello_utf8[64];
    sjis_to_utf8(hello_sjis, hello_utf8, sizeof(hello_utf8));
    char mixed_sjis[] = "Hello こんにちは 世界 World!";
    char mixed_utf8[128];
    sjis_to_utf8(mixed_sjis, mixed_utf8, sizeof(mixed_utf8));
    nk_layout_row_static(ctx, 25, 680, 1);
    nk_label(ctx, hello_utf8, NK_TEXT_LEFT);
    nk_label(ctx, mixed_utf8, NK_TEXT_LEFT);

        // 1行テキスト入力
    char single_label_sjis[] = "1行テキスト入力 (行修正版):";
    char single_label_utf8[64];
    sjis_to_utf8(single_label_sjis, single_label_utf8, sizeof(single_label_utf8));
    nk_layout_row_static(ctx, 25, 680, 1);
    nk_label(ctx, single_label_utf8, NK_TEXT_LEFT);
        nk_layout_row_static(ctx, 25, 680, 1);

        struct nk_rect edit_bounds = nk_widget_bounds(ctx);
    int edit_result = nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, text_buffer_utf8, sizeof(text_buffer_utf8), nk_filter_default);

        int cursor_changed = (prev_single_cursor != ctx->text_edit.cursor);
        int became_active = (edit_result & NK_EDIT_ACTIVE) && !single_line_active;
        int still_active = (edit_result & NK_EDIT_ACTIVE) && single_line_active;

        single_line_active = (edit_result & NK_EDIT_ACTIVE) ? 1 : 0;

        if (became_active || (still_active && cursor_changed))
        {
            update_ime_position_single_line_line_fixed(ctx, main_window, text_buffer_utf8, edit_bounds);
        }

        prev_single_cursor = ctx->text_edit.cursor;

        // 複数行テキスト入力
    char multi_label_sjis[] = "複数行テキスト入力 (行修正版):";
    char multi_label_utf8[64];
    sjis_to_utf8(multi_label_sjis, multi_label_utf8, sizeof(multi_label_utf8));
    nk_layout_row_static(ctx, 25, 680, 1);
    nk_label(ctx, multi_label_utf8, NK_TEXT_LEFT);
        nk_layout_row_static(ctx, 200, 680, 1);

        struct nk_rect multiline_bounds = nk_widget_bounds(ctx);
    int multiline_result = nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_MULTILINE, multiline_buffer_utf8, sizeof(multiline_buffer_utf8), nk_filter_default);

        int multi_cursor_changed = (prev_multi_cursor != ctx->text_edit.cursor);
        int multi_became_active = (multiline_result & NK_EDIT_ACTIVE) && !multi_line_active;
        int multi_still_active = (multiline_result & NK_EDIT_ACTIVE) && multi_line_active;

        multi_line_active = (multiline_result & NK_EDIT_ACTIVE) ? 1 : 0;

        if (multi_became_active || (multi_still_active && multi_cursor_changed))
        {
            update_ime_position_multiline_line_fixed(ctx, main_window, multiline_buffer_utf8, multiline_bounds);
        }

        prev_multi_cursor = ctx->text_edit.cursor;

        // デバッグ情報表示
        nk_layout_row_static(ctx, 25, 680, 1);
        nk_label(ctx, "=== 行修正版：詳細情報 ===", NK_TEXT_CENTERED);

        if (single_line_active || multi_line_active)
        {
            const char* active_text = single_line_active ? text_buffer_utf8 : multiline_buffer_utf8;
            struct detailed_cursor_info info = get_detailed_cursor_info_fixed(ctx, active_text, multi_line_active);

            // ↑/↓キーの押下を処理（Nuklear の入力状態 + OS フォールバック）。
            // エッジ検出で一回だけ動かす。
            int curr_key_up = nk_input_is_key_pressed(&ctx->input, NK_KEY_UP) || os_key_up;
            int curr_key_down = nk_input_is_key_pressed(&ctx->input, NK_KEY_DOWN) || os_key_down;
            int up_pressed_edge = (curr_key_up && !prev_key_up);
            int down_pressed_edge = (curr_key_down && !prev_key_down);

            if (up_pressed_edge || down_pressed_edge) {
                // 現在の行と列情報
                int cur_line = info.line_number;
                int cur_column_in_line = info.column_in_line; // 文字単位のカウント

                // 現在の列を文字数で取得するため、行先頭からカーソルバイト位置までを文字数に変換
                int line_start_byte = info.line_start_byte;
                int desired_column_chars = 0;
                if (info.cursor_byte_pos > line_start_byte) {
                    desired_column_chars = byte_pos_to_char_pos(active_text + line_start_byte, info.cursor_byte_pos - line_start_byte);
                } else {
                    desired_column_chars = 0;
                }

                int target_line = cur_line;
                if (up_pressed_edge) {
                    if (cur_line > 0) target_line = cur_line - 1;
                } else if (down_pressed_edge) {
                    // 次の行が存在するか確認
                    // 次の行の先頭バイト位置を取得
                    int next_line_start = find_line_start_byte(active_text, cur_line + 1);
                    if (next_line_start < (int)strlen(active_text)) {
                        target_line = cur_line + 1;
                    } else {
                        // 既に最終行なら動かない
                        target_line = cur_line;
                    }
                }

                // 目標行の開始バイトを取得
                int target_line_start = find_line_start_byte(active_text, target_line);
                int target_line_end = find_line_start_byte(active_text, target_line + 1);
                if (target_line_end > (int)strlen(active_text)) target_line_end = (int)strlen(active_text);

                // 目標行の文字数を取得
                int target_line_chars = byte_pos_to_char_pos(active_text + target_line_start, target_line_end - target_line_start);

                int new_column_chars = desired_column_chars;
                if (new_column_chars > target_line_chars) new_column_chars = target_line_chars; // 行の長さを超えてたら末尾に

                // 目標のバイト位置を求め、Nuklearのカーソル（文字数）に変換して設定
                int target_byte_pos = char_pos_to_byte_pos(active_text + target_line_start, new_column_chars) + target_line_start;

                // 目標の文字数（先頭からの文字数）を計算
                int total_char_pos = byte_pos_to_char_pos(active_text, target_byte_pos);

                // Nuklearのカーソルにセット
                // 注意: nk_edit_string / nk_edit_buffer の内部では
                // ウィンドウ固有の win->edit.cursor が編集開始時に
                // text_edit にコピーされ、描画後に再び win->edit に
                // 書き戻されます。したがってプログラムから即時に
                // カーソルを反映させるには両方を更新します。
                ctx->text_edit.cursor = total_char_pos;
                if (ctx && ctx->current) {
                    ctx->current->edit.cursor = total_char_pos;
                    ctx->current->edit.sel_start = total_char_pos;
                    ctx->current->edit.sel_end = total_char_pos;
                }

                // IME位置を更新（既計算のinfoを利用）
                if (single_line_active)
                    update_ime_position_single_with_info(ctx, main_window, text_buffer_utf8, edit_bounds, &info);
                else
                    update_ime_position_multiline_with_info(ctx, main_window, multiline_buffer_utf8, multiline_bounds, &info);
            }

            // 既存のデバッグ表示
            char debug_text[512];
            sprintf_s(debug_text, sizeof(debug_text),
                "%s - カーソル:%dB 行:%d 列:%d 幅:%.1fpx 行高:%.1fpx",
                single_line_active ? "1行" : "複数行",
                info.cursor_byte_pos, info.line_number, info.column_in_line,
                info.text_width_to_cursor, info.actual_line_height);

            nk_layout_row_static(ctx, 20, 680, 1);
            nk_label(ctx, debug_text, NK_TEXT_LEFT);

            // 追加デバッグ情報
            sprintf_s(debug_text, sizeof(debug_text),
                "行開始バイト:%d 行内バイト:%d",
                info.line_start_byte, info.cursor_byte_in_line);
            nk_layout_row_static(ctx, 20, 680, 1);
            nk_label(ctx, debug_text, NK_TEXT_LEFT);
        }
        else
        {
            nk_layout_row_static(ctx, 20, 680, 1);
            nk_label(ctx, "テキストフィールドがアクティブではありません", NK_TEXT_LEFT);
        }
    }
    nk_end(ctx);
    // フレーム終端で現在のキー状態を前フレームとして保存（エッジ検出用）
    prev_key_up = nk_input_is_key_pressed(&ctx->input, NK_KEY_UP) || os_key_up;
    prev_key_down = nk_input_is_key_pressed(&ctx->input, NK_KEY_DOWN) || os_key_down;
    prev_key_left = nk_input_is_key_pressed(&ctx->input, NK_KEY_LEFT) || os_key_left;
    prev_key_right = nk_input_is_key_pressed(&ctx->input, NK_KEY_RIGHT) || os_key_right;
}

// 補助: バイト位置 -> 文字数変換
int byte_pos_to_char_pos(const char* text, int byte_pos)
{
    if (!text || byte_pos <= 0) return 0;
    int len = (int)strlen(text);
    int char_count = 0;
    int i = 0;
    while (i < byte_pos && i < len) {
        nk_rune rune;
        int char_len = nk_utf_decode(text + i, &rune, len - i);
        if (char_len <= 0) break;
        i += char_len;
        char_count++;
    }
    return char_count;
}

// 補助: 文字数 -> バイト位置変換（安全な版）
int char_pos_to_byte_pos(const char* text, int char_pos)
{
    if (!text || char_pos <= 0) return 0;
    int len = (int)strlen(text);
    int i = 0;
    int cur = 0;
    while (cur < char_pos && i < len) {
        nk_rune rune;
        int char_len = nk_utf_decode(text + i, &rune, len - i);
        if (char_len <= 0) break;
        i += char_len;
        cur++;
    }
    return i;
}

// 補助: 行の開始バイト位置を取得（行番号が0ベース）
int find_line_start_byte(const char* text, int line_number)
{
    if (!text || line_number <= 0) return 0;
    int len = (int)strlen(text);
    int line = 0;
    int i = 0;
    if (line_number == 0) return 0;
    while (i < len) {
        if (text[i] == '\n') {
            line++;
            if (line == line_number) return i + 1; // 次のバイトが行開始
        }
        i++;
    }
    // 指定行が存在しない場合は末尾
    return len;
}

// メイン関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HRESULT hr;
    // コンソールウィンドウを割り当て（デバッグ用）
    AllocConsole();
    
    // コンソールのコードページをUTF-8に設定
    //SetConsoleOutputCP(65001);   // 出力をUTF-8に設定
    //SetConsoleCP(65001);         // 入力をUTF-8に設定
    
    // コマンドプロンプト自体のコードページもUTF-8に変更
    //system("chcp 65001 >nul 2>&1");
    
    // ファイルハンドルをリダイレクト
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
    
    // コンソールタイトルを設定
    SetConsoleTitleA("Nuklear D3D9 Japanese IME Debug Console");
    
    printf("=== Nuklear D3D9 日本語IMEサンプル ===\n");
    printf("コンソール設定: UTF-8 (CP65001) 対応完了\n");
    printf("デバッグ情報がここに表示されます。\n");
    printf("文字化けテスト: あいうえお カタカナ 漢字テスト\n\n");

    // ウィンドウクラスの登録
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NuklearD3D9Japanese";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc))
    {
        printf("ウィンドウクラスの登録に失敗しました\n");
        return -1;
    }

    // ウィンドウの作成
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

    HWND wnd = CreateWindowW(wc.lpszClassName, L"Nuklear D3D9 日本語表示サンプル",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL);

    if (!wnd)
    {
        printf("ウィンドウの作成に失敗しました\n");
        return -1;
    }

    // ウィンドウハンドルを保存（IME位置制御用）
    main_window = wnd;

    // Direct3D初期化
    if (!init_d3d(wnd))
    {
        printf("Direct3Dの初期化に失敗しました\n");
        return -1;
    }

    // Nuklear初期化
    ctx = nk_d3d9_init(device, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!ctx)
    {
        printf("Nuklearの初期化に失敗しました\n");
        return -1;
    }
    if (!load_japanese_font())
    {
        printf("日本語フォントの読み込みに失敗しました\n");
        return -1;
    }

    // メインループ
    int running = 1;
    while (running)
    {
    // フレームカウンタを更新
    global_frame_counter++;

        MSG msg;
        nk_input_begin(ctx);

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        nk_input_end(ctx);

        // リサイズ後のリソース再作成
        if (need_reset) {
            hr = IDirect3DDevice9_Reset(device, &present);
            if (SUCCEEDED(hr)) {
                nk_d3d9_resize(present.BackBufferWidth, present.BackBufferHeight);
                need_reset = 0;
            } else {
                // 失敗時は描画スキップ
                continue;
            }
        }

        // リソース未作成時は描画スキップ
        if (need_reset) continue;
        // GUI描画
        draw_gui_line_fixed();
       // draw_gui_fixed();
        // レンダリング
        IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            D3DCOLOR_XRGB(40, 40, 40), 1.0f, 0);

        if (SUCCEEDED(IDirect3DDevice9_BeginScene(device)))
        {
            nk_d3d9_render(NK_ANTI_ALIASING_ON);
            IDirect3DDevice9_EndScene(device);
        }

        HRESULT result = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        if (result == D3DERR_DEVICELOST)
        {
            nk_d3d9_release();
            result = IDirect3DDevice9_Reset(device, &present);
            if (result == D3DERR_INVALIDCALL)
            {
                running = 0;
            }
            else
            {
                //nk_d3d9_resize(WINDOW_WIDTH, WINDOW_HEIGHT);
            }
        }
    }

    // クリーンアップ
    if (ctx) {
        nk_d3d9_shutdown();
        ctx = NULL;
    }
    
    if (device) {
        IDirect3DDevice9_Release(device);
        device = NULL;
    }
    
    if (d3d) {
        IDirect3D9_Release(d3d);
        d3d = NULL;
    }
    
    // フォントアトラスは nk_d3d9_shutdown() で既に解放されているため、
    // 追加のクリーンアップは不要
    atlas = NULL;
    
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    FreeConsole();
    return 0;
}

