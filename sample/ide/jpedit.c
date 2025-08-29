/*
 * 単一ファイル化された Nuklear D3D9 エディタ（簡素化版）。
 * ワークスペース内の既存の nuklear_d3d9.h バックエンドを使用します（存在が必要）。
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <imm.h>
#include <commdlg.h>
#include <wincon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <limits.h>

#if defined(__TINYC__) || (defined(_MSC_VER) && (_MSC_VER <= 1200))
/* VC6 互換: snprintf が無い場合は _vsnprintf を使った簡易実装を提供
   また GetConsoleWindow / AttachConsole が古い SDK に無いことがあるため
   ランタイムで Resolve する小さなラッパーを用意します。 */
#include <stdarg.h>
static int snprintf(char* buf, size_t len, const char* fmt, ...)
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = _vsnprintf(buf, len, fmt, ap);
	if (ret < 0 && len > 0) buf[len - 1] = '\0';
	va_end(ap);
	return ret;
}

/* typedef を先に定義しておく */
typedef HWND (WINAPI *PFN_GetConsoleWindow)(void);
typedef BOOL (WINAPI *PFN_AttachConsole)(DWORD);

static HWND GetConsoleWindow(void)
{
	HMODULE h;
	PFN_GetConsoleWindow p;
	h = GetModuleHandleA("kernel32.dll");
	if (!h) return NULL;
	p = (PFN_GetConsoleWindow)GetProcAddress(h, "GetConsoleWindow");
	if (p) return p();
	return NULL;
}

static BOOL AttachConsole(DWORD dwProcessId)
{
	PFN_AttachConsole p;
	HMODULE h = GetModuleHandleA("kernel32.dll");
	if (!h) return FALSE;
	p = (PFN_AttachConsole)GetProcAddress(h, "AttachConsole");
	if (p) return p(dwProcessId);
	return FALSE;
}
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

#ifndef ATTACH_PARENT_PROCESS
#define ATTACH_PARENT_PROCESS ((DWORD)-1)
#endif
//#define MOBILE_PORTRAIT
/*****
 * ウィンドウサイズの条件付き定義
 * デスクトップ用とスマホ縦長（MOBILE_PORTRAIT）を切り替え可能にします。
 * スマホ用に切り替えるにはビルド時に -DMOBILE_PORTRAIT を渡すか、下の
 * コメントを外してソース内で有効化してください（例: #define MOBILE_PORTRAIT）。
 */

/* #define MOBILE_PORTRAIT */

#ifdef MOBILE_PORTRAIT
/* スマホ縦長の既定サイズ（必要なら調整してください） */
#define WINDOW_WIDTH 360
#define WINDOW_HEIGHT 800
#else
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#endif
#define MAX_TEXT_SIZE 65536
/* 編集可能なドキュメントの最大数（必要ならここを変更） */
#ifndef MAX_EDITORS
#define MAX_EDITORS 16
#endif
static void open_file_dialog_and_load(void);
static void build_current_doc_with_tcc(const char* filename);
static void run_current_doc(const char* exe_name);

/* 設定変数（ここで定義） */
char cfg_tcc_path[MAX_PATH] = "";
char cfg_fonts_path[MAX_PATH] = "";
char cfg_src_path[MAX_PATH] = "";
char cfg_compiler_opts[512] = "";
int cfg_tab_width = 4;

/* 注意: Nuklear に渡す前にタブをスペースへ展開しない方法（フォント幅コールバックの上書き）
	と、編集時にタブをスペースへ展開して編集後に戻す方法の二通りがあります。
	ここでは実装を簡潔にするために後者（表示用にタブをスペースへ展開し、編集後に元に戻す）を採用しています。 */

/* 表示用にタブをスペースへ展開するヘルパー関数 */
static char* expand_tabs_to_spaces(const char* text, int tab_width, char* buffer, int buffer_size)
{
	if (!text || !buffer) return NULL;
	
	int out_pos = 0;
	int column = 0;
	int len = (int)strlen(text);
	
	for (int i = 0; i < len && out_pos < buffer_size - 1; i++) {
		if (text[i] == '\t') {
			/* 次のタブストップに到達するために必要なスペース数を計算 */
			int next_tab_stop = ((column / tab_width) + 1) * tab_width;
			int spaces_needed = next_tab_stop - column;
			
			/* タブストップまでスペースを追加 */
			for (int j = 0; j < spaces_needed && out_pos < buffer_size - 1; j++) {
				buffer[out_pos++] = ' ';
			}
			column = next_tab_stop;
		} else if (text[i] == '\n' || text[i] == '\r') {
			buffer[out_pos++] = text[i];
			column = 0;  /* 改行でカラムをリセット */
		} else {
			buffer[out_pos++] = text[i];
			column++;
		}
	}
	
	buffer[out_pos] = '\0';
	return buffer;
}

/* 表示バッファのスペースをタブへ戻すヘルパー関数 */
static void collapse_spaces_to_tabs(const char* expanded_text, char* internal_text, int tab_width, int buffer_size)
{
	if (!expanded_text || !internal_text) return;
	
	int in_pos = 0, out_pos = 0;
	int column = 0;
	int len = (int)strlen(expanded_text);
	
	while (in_pos < len && out_pos < buffer_size - 1) {
		if (expanded_text[in_pos] == ' ') {
			/* 連続するスペースを数える */
			int space_count = 0;
			int temp_pos = in_pos;
			
			while (temp_pos < len && expanded_text[temp_pos] == ' ') {
				space_count++;
				temp_pos++;
			}
			
			/* スペースがタブストップに揃っているか確認 */
			int spaces_to_tab_stop = tab_width - (column % tab_width);
			if (space_count >= spaces_to_tab_stop && (column % tab_width) != 0) {
				/* タブストップに達したらタブに変換 */
				internal_text[out_pos++] = '\t';
				in_pos += spaces_to_tab_stop;
				column += spaces_to_tab_stop;
				
				/* 残りのスペースを処理 */
				space_count -= spaces_to_tab_stop;
				while (space_count >= tab_width) {
					internal_text[out_pos++] = '\t';
					in_pos += tab_width;
					column += tab_width;
					space_count -= tab_width;
				}
				/* 残りのスペースをそのまま追加 */
				while (space_count > 0 && out_pos < buffer_size - 1) {
					internal_text[out_pos++] = ' ';
					in_pos++;
					column++;
					space_count--;
				}
			} else {
				/* スペースのまま保持 */
				internal_text[out_pos++] = expanded_text[in_pos++];
				column++;
			}
		} else if (expanded_text[in_pos] == '\n' || expanded_text[in_pos] == '\r') {
			internal_text[out_pos++] = expanded_text[in_pos++];
			column = 0;
		} else {
			internal_text[out_pos++] = expanded_text[in_pos++];
			column++;
		}
	}
	
	internal_text[out_pos] = '\0';
}
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
#include <stdarg.h>
static int sprintf_s(char* buf, size_t bufsize, const char* fmt, ...)
{
	int ret;
	va_list args;
	va_start(args, fmt);
	ret = _vsnprintf(buf, bufsize - 1, fmt, args);
	va_end(args);
	buf[bufsize - 1] = '\0'; // 念のため終端
	return ret;
}
#endif
/* ファイル属性管理用の列挙型 */
typedef enum {
	ENC_UNKNOWN = 0,
	ENC_UTF8,
	ENC_UTF16_LE,
	ENC_UTF16_BE,
	ENC_SJIS,
	ENC_EUC_JP
} EncodingType;

typedef enum {
	NEWLINE_UNKNOWN = 0,
	NEWLINE_CRLF,
	NEWLINE_LF
} NewlineType;

typedef enum {
	FILETYPE_UNKNOWN = 0,
	FILETYPE_C,
	FILETYPE_H,
	FILETYPE_CS,   /* C# .cs */
	FILETYPE_PY,   /* Python .py */
	FILETYPE_PHP,  /* PHP .php */
	FILETYPE_INI,  /* INI .ini */
	FILETYPE_LOG,  /* Log .log */
	FILETYPE_SQL,  /* SQL .sql */
	FILETYPE_BAT,  /* Batch .bat */
	FILETYPE_PS1,  /* PowerShell .ps1 */
	FILETYPE_HTML, /* HTML .html .htm */
	FILETYPE_CSS,  /* CSS .css */
	FILETYPE_TXT,
	FILETYPE_OTHER
} FileType;

typedef struct
{
	char text[MAX_TEXT_SIZE];
	int text_len;
	char title[64];
	int modified;
	int checked;
	/* 追加属性 */
	EncodingType encoding; /* 文字コード */
	NewlineType newline;   /* 改行コード */
	int has_bom;           /* BOM が付いているか（主に UTF-8/UTF-16） */
	FileType file_type;    /* 拡張子などによるファイル種別 */
} JapaneseTextEditor;
typedef struct
{
	JapaneseTextEditor editors[MAX_EDITORS];
	int current_editor;
	int editor_count;
	struct nk_color bg_color;
	char new_doc_title[64];
	int is_interpreter_mode; /* 0 = compiler mode, 1 = interpreter mode */
} AppState;

static struct nk_context* ctx = NULL;
static struct nk_font_atlas* atlas_ptr = NULL;
static IDirect3D9* d3d = NULL;
static IDirect3DDevice9* device = NULL;
static IDirect3DDevice9Ex* deviceEx = NULL;
static D3DPRESENT_PARAMETERS present;
static AppState app;
/* 設定保存用グローバル */

static void load_or_create_ini(void);
/* デバッグ用フラグ: スクロールバーをドラッグしている間に立つフラグ（実体は nuklear.h で定義） */

static int dbg_prev_select_start = -1;
static int dbg_prev_select_end = -1;

/* IME 用のグローバルとヘルパ関数（old_edit.c から移植） */
static HWND main_window = NULL;
static int global_frame_counter = 0;
static float last_ime_x = 0.0f;
static float last_ime_y = 0.0f;

struct detailed_cursor_info
{
	int cursor_byte_pos;
	int line_number;
	int column_in_line;
	int line_start_byte;
	int cursor_byte_in_line;
	float text_width_to_cursor;
	float actual_line_height;
	struct nk_vec2 scroll_offset;
};

int byte_pos_to_char_pos(const char* text, int byte_pos)
{
	int char_count = 0;
	int len;
	int i = 0;
	if (!text || byte_pos <= 0) return 0;
	len = (int)strlen(text);
	while (i < byte_pos && i < len)
	{
		nk_rune rune;
		int char_len = nk_utf_decode(text + i, &rune, len - i);
		if (char_len <= 0) break;
		i += char_len;
		char_count++;
	}
	return char_count;
}

int char_pos_to_byte_pos(const char* text, int char_pos)
{
	int i = 0;
	int cur = 0;
	int len;
	if (!text || char_pos <= 0) return 0;
	len = (int)strlen(text);
	while (cur < char_pos && i < len)
	{
		nk_rune rune;
		int char_len = nk_utf_decode(text + i, &rune, len - i);
		if (char_len <= 0) break;
		i += char_len;
		cur++;
	}
	return i;
}

int find_line_start_byte(const char* text, int line_number)
{
	int line = 0;
	int i = 0;
	int len;
	if (!text || line_number <= 0) return 0;
	len = (int)strlen(text);
	if (line_number == 0) return 0;
	while (i < len)
	{
		if (text[i] == '\n')
		{
			line++;
			if (line == line_number) return i + 1;
		}
		i++;
	}
	return len;
}

struct detailed_cursor_info get_detailed_cursor_info_fixed(struct nk_context* ctx, const char* text, int is_multiline)
{
	struct detailed_cursor_info info = { 0 };
	static int cached_frame = -1;
	static const char* cached_text = NULL;
	static struct detailed_cursor_info cached_info;
	struct nk_text_edit* edit;
	const struct nk_user_font* font;
	int nuklear_cursor_char_pos;
	int current_char_index;
	int text_len;
	int char_byte_len;
	struct nk_style_edit* style;
	int pos;
	if (!ctx || !text) return info;
	if (cached_frame == global_frame_counter && cached_text == text) return cached_info;
	edit = &ctx->text_edit;
	font = ctx->style.font;
	if (!font) return info;
	nuklear_cursor_char_pos = edit->cursor;
	text_len = (int)strlen(text);
	info.cursor_byte_pos = 0;
	current_char_index = 0;
	while (current_char_index < nuklear_cursor_char_pos && info.cursor_byte_pos < text_len)
	{
		nk_rune rune;
		char_byte_len = nk_utf_decode(text + info.cursor_byte_pos, &rune, text_len - info.cursor_byte_pos);
		if (char_byte_len <= 0) break;
		info.cursor_byte_pos += char_byte_len;
		current_char_index++;
	}
	if (info.cursor_byte_pos > text_len) info.cursor_byte_pos = text_len;
	if (info.cursor_byte_pos < 0) info.cursor_byte_pos = 0;
	while (info.cursor_byte_pos > 0 && (text[info.cursor_byte_pos] & 0xC0) == 0x80) info.cursor_byte_pos--;
	style = &ctx->style.edit;
	info.actual_line_height = font->height + style->row_padding;
	if (is_multiline)
	{
		int i;
		info.line_number = 0;
		info.column_in_line = 0;
		info.line_start_byte = 0;
		for (i = 0; i < info.cursor_byte_pos && i < text_len; i++)
		{
			if (text[i] == '\n')
			{
				info.line_number++;
				info.column_in_line = 0;
				info.line_start_byte = i + 1;
			}
			else
			{
				info.column_in_line++;
			}
		}
		info.cursor_byte_in_line = info.cursor_byte_pos - info.line_start_byte;
	}
	else
	{
		info.line_number = 0;
		info.line_start_byte = 0;
		info.cursor_byte_in_line = info.cursor_byte_pos;
	}
	info.text_width_to_cursor = 0.0f;
	pos = info.line_start_byte;
	while (pos < info.cursor_byte_pos && text[pos] != '\0' && text[pos] != '\n')
	{
		float char_width;
		int char_len = 1;
		unsigned char c = (unsigned char)text[pos];
		if ((c & 0x80) == 0x00) char_len = 1;
		else if ((c & 0xE0) == 0xC0) char_len = 2;
		else if ((c & 0xF0) == 0xE0) char_len = 3;
		else if ((c & 0xF8) == 0xF0) char_len = 4;
		if (pos + char_len > info.cursor_byte_pos) break;
		char_width = font->width(font->userdata, font->height, text + pos, char_len);
		info.text_width_to_cursor += char_width;
		pos += char_len;
	}
	info.scroll_offset.x = 0;
	info.scroll_offset.y = 0;
	cached_frame = global_frame_counter;
	cached_text = text;
	cached_info = info;
	return cached_info;
}

static void ime_set_position_from_info(struct nk_context* ctx, HWND hwnd, const struct detailed_cursor_info* info, struct nk_rect edit_bounds, int is_multiline)
{
	struct nk_style_edit* style;
	float content_x, content_y, ime_x, ime_y, max_y, max_x;
	HIMC himc;
	if (!ctx || !hwnd || !info) return;
	style = &ctx->style.edit;
	content_x = edit_bounds.x + style->border + style->padding.x;
	content_y = edit_bounds.y + style->border + style->padding.y;
	if (is_multiline)
	{
		ime_x = content_x + info->text_width_to_cursor - info->scroll_offset.x;
		ime_y = content_y + (info->line_number * info->actual_line_height) - info->scroll_offset.y;
		max_y = edit_bounds.y + edit_bounds.h - info->actual_line_height - style->padding.y - style->border;
		if (ime_y > max_y) ime_y = max_y;
	}
	else
	{
		ime_x = content_x + info->text_width_to_cursor - info->scroll_offset.x;
		ime_y = content_y - info->scroll_offset.y;
	}
	max_x = edit_bounds.x + edit_bounds.w - style->padding.x - style->border - 20;
	if (ime_x > max_x) ime_x = max_x;
	if (ime_x < content_x) ime_x = content_x;
	if (ime_y < content_y) ime_y = content_y;
	himc = ImmGetContext(hwnd);
	if (himc)
	{
		COMPOSITIONFORM cf = { 0 };
		CANDIDATEFORM cand = { 0 };
		/* 最後に取得した IME 座標を記録し、WM_IME_* ハンドラが必要時に再適用できるようにする */
		last_ime_x = ime_x;
		last_ime_y = ime_y;
		cf.dwStyle = CFS_POINT;
		cf.ptCurrentPos.x = (LONG)ime_x;
		cf.ptCurrentPos.y = (LONG)ime_y;
		ImmSetCompositionWindow(himc, &cf);
		cand.dwIndex = 0;
		cand.dwStyle = CFS_CANDIDATEPOS;
		cand.ptCurrentPos.x = (LONG)ime_x;
		cand.ptCurrentPos.y = (LONG)(ime_y + info->actual_line_height + 2);
		ImmSetCandidateWindow(himc, &cand);
		ImmReleaseContext(hwnd, himc);
	}
}

/* 保存してある last_ime_x/last_ime_y から IME 位置を再設定する - WM_IME_* イベントで使用 */
static void update_ime_position_simple(HWND wnd, float x, float y)
{
	HIMC himc = ImmGetContext(wnd);
	COMPOSITIONFORM cf = { 0 };
	CANDIDATEFORM cand = { 0 };
	if (!himc) return;
	cf.dwStyle = CFS_POINT;
	cf.ptCurrentPos.x = (LONG)x;
	cf.ptCurrentPos.y = (LONG)y;
	ImmSetCompositionWindow(himc, &cf);
	cand.dwIndex = 0;
	cand.dwStyle = CFS_CANDIDATEPOS;
	cand.ptCurrentPos.x = (LONG)x;
	cand.ptCurrentPos.y = (LONG)(y + 30);
	ImmSetCandidateWindow(himc, &cand);
	ImmReleaseContext(wnd, himc);
}

void update_ime_position_multiline_line_fixed(struct nk_context* ctx, HWND hwnd, const char* text, struct nk_rect edit_bounds)
{
	struct detailed_cursor_info cursor_info;
	if (!ctx || !hwnd || !text) return;
	cursor_info = get_detailed_cursor_info_fixed(ctx, text, 1);
	ime_set_position_from_info(ctx, hwnd, &cursor_info, edit_bounds, 1);
}

void update_ime_position_single_line_line_fixed(struct nk_context* ctx, HWND hwnd, const char* text, struct nk_rect edit_bounds)
{
	struct detailed_cursor_info cursor_info;
	if (!ctx || !hwnd || !text) return;
	cursor_info = get_detailed_cursor_info_fixed(ctx, text, 0);
	ime_set_position_from_info(ctx, hwnd, &cursor_info, edit_bounds, 0);
}


/* ヘルパ関数 */
static void init_text_editor(JapaneseTextEditor* editor, const char* title)
{
	memset(editor->text, 0, sizeof(editor->text));
	editor->text_len = 0;
	strncpy(editor->title, title, sizeof(editor->title) - 1);
	editor->title[sizeof(editor->title) - 1] = '\0';
	editor->modified = 0;
	editor->checked = 0;
	/* 追加属性のデフォルト初期化 */
	editor->encoding = ENC_UNKNOWN;
	editor->newline = NEWLINE_UNKNOWN;
	editor->has_bom = 0;
	editor->file_type = FILETYPE_UNKNOWN;
}

static void init_app(void)
{
	const char* sample =
		"Welcome to Japanese Text Editor!\n\n"
		"This editor supports:\n"
		"- Multiple text editing\n"
		"- File save and load\n"
		"- Japanese input with IME\n\n"
		"Programming example:\n"
		"#include <stdio.h>\n"
		"int main() {\n"
		"    printf(\"Hello, World!\\n\");\n"
		"    return 0;\n"
		"}\n\n"
		"Test your Japanese input here:\n"
		"(Enable IME and type in Japanese)\n";    app.current_editor = 0;
	app.editor_count = 1;
	app.bg_color.r = 40;
	app.bg_color.g = 44;
	app.bg_color.b = 52;
	app.bg_color.a = 255;
	app.is_interpreter_mode = 0;

	init_text_editor(&app.editors[0], "Document");
	app.new_doc_title[0] = '\0';



	strncpy(app.editors[0].text, sample, sizeof(app.editors[0].text) - 1);
	app.editors[0].text_len = (int)strlen(app.editors[0].text);
}

static void add_new_editor(void)
{
	/* 後方互換のため: テンプレート作成関数へ委譲 */
	extern void add_new_editor_with_title(void);
	add_new_editor_with_title();
}

/* app.new_doc_title があればそれをタイトルに使い、なければフォールバック名を使用して新規エディタを作成 */
static void add_new_editor_with_title(void)
{
	if (app.editor_count < MAX_EDITORS)
	{
		char title[64];
		if (app.new_doc_title[0])
		{
			strncpy(title, app.new_doc_title, sizeof(title) - 1);
			title[sizeof(title) - 1] = '\0';
		}
		else
		{
			sprintf(title, "Document %d", app.editor_count + 1);
		}
		init_text_editor(&app.editors[app.editor_count], title);
		app.current_editor = app.editor_count;
		app.editor_count++;
		app.new_doc_title[0] = '\0';
	}
}

static void save_file(JapaneseTextEditor* editor)
{
	char filename[256];
	FILE* file;
	sprintf(filename, "%s.txt", editor->title);

	file = fopen(filename, "wb");
	if (file)
	{
		unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
		fwrite(bom, 1, 3, file);
		fwrite(editor->text, 1, editor->text_len, file);
		fclose(file);
		editor->modified = 0;
		MessageBoxA(NULL, "File saved successfully", "Save Complete", MB_OK);
	}
	else
	{
		MessageBoxA(NULL, "Failed to save file", "Error", MB_OK | MB_ICONERROR);
	}
}

/* GUI */
static void draw_gui(void)
{
	struct nk_rect bounds;
	/* レイアウト用の矩形を条件付きで設定：モバイル縦長では Properties を下に配置 */
	struct nk_rect editor_bounds;
	struct nk_rect props_bounds;
#ifdef MOBILE_PORTRAIT
	{
		int props_h = WINDOW_HEIGHT / 3; /* 下部に置く Properties の高さ */
		editor_bounds = nk_rect(10, 10, WINDOW_WIDTH - 20, WINDOW_HEIGHT - props_h - 20);
		props_bounds = nk_rect(10, WINDOW_HEIGHT - props_h - 10, WINDOW_WIDTH - 20, props_h);
	}
#else
	{
		/* デスクトップ既存レイアウト */
		editor_bounds = nk_rect(310, 10, WINDOW_WIDTH - 320, WINDOW_HEIGHT - 20);
		props_bounds = nk_rect(10, 10, 290, WINDOW_HEIGHT - 20);
	}
#endif
	int i;
	// メイン編集領域
	bounds = editor_bounds;
	if (nk_begin(ctx, "Japanese Multi Text Editor", bounds,
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
	{
		JapaneseTextEditor* current;
		JapaneseTextEditor* editor;
		char status[128];
		int still_active, became_active, cursor_changed, old_len, line_count = 1;
		static int prev_cursor = -1;
		static int active_before = 0;
		nk_flags edit_flags;
		struct nk_rect edit_bounds;
		nk_flags edit_result;

		nk_layout_row_begin(ctx, NK_STATIC, 25, 4);
		nk_layout_row_push(ctx, 60);
		if (nk_button_label(ctx, "Save")) save_file(&app.editors[app.current_editor]);
		nk_layout_row_push(ctx, 60);
		if (nk_button_label(ctx, "New")) add_new_editor();
		nk_layout_row_push(ctx, 80);
		if (nk_button_label(ctx, "Clear"))
		{
			memset(app.editors[app.current_editor].text, 0, sizeof(app.editors[app.current_editor].text));
			app.editors[app.current_editor].text_len = 0;
			app.editors[app.current_editor].modified = 1;
		}
		nk_layout_row_push(ctx, 150);
		current = &app.editors[app.current_editor];
		for (i = 0; i < current->text_len; i++) if (current->text[i] == '\n') line_count++;
		sprintf(status, "%d lines, %d chars", line_count, current->text_len);
		nk_label(ctx, status, NK_TEXT_LEFT);
		nk_layout_row_end(ctx);

		nk_layout_row_dynamic(ctx, WINDOW_HEIGHT - 120, 1);
		editor = &app.editors[app.current_editor];
		edit_flags = NK_EDIT_FIELD | NK_EDIT_MULTILINE | NK_EDIT_SELECTABLE |
			NK_EDIT_CLIPBOARD | NK_EDIT_ALLOW_TAB;
		old_len = editor->text_len;
		/* track cursor/active state to update IME only when needed */
		edit_bounds = nk_widget_bounds(ctx);
		/* use tab expansion approach for consistent rendering */
		{
			static char expanded_buffer[MAX_TEXT_SIZE];
			char temp_buffer[MAX_TEXT_SIZE];
			int tabw = cfg_tab_width > 0 ? cfg_tab_width : 4;
			
			/* Expand tabs to spaces for Nuklear editing */
			expand_tabs_to_spaces(editor->text, tabw, expanded_buffer, sizeof(expanded_buffer));
			
			/* Edit the expanded text */
			edit_result = nk_edit_string_zero_terminated(ctx, edit_flags, expanded_buffer, sizeof(expanded_buffer), nk_filter_default);
			
			/* Convert back to tab format and update editor */
			if (edit_result & (NK_EDIT_COMMITED | NK_EDIT_ACTIVE)) {
				collapse_spaces_to_tabs(expanded_buffer, temp_buffer, tabw, sizeof(temp_buffer));
				
				/* Update only if text actually changed */
				if (strcmp(temp_buffer, editor->text) != 0) {
					strncpy(editor->text, temp_buffer, sizeof(editor->text) - 1);
					editor->text[sizeof(editor->text) - 1] = '\0';
					editor->text_len = (int)strlen(editor->text);
					editor->modified = 1;
				}
			}
		}

		cursor_changed = (prev_cursor != ctx->text_edit.cursor);
		became_active = ((edit_result & NK_EDIT_ACTIVE) && !active_before) ? 1 : 0;
		still_active = ((edit_result & NK_EDIT_ACTIVE) && active_before) ? 1 : 0;
		active_before = (edit_result & NK_EDIT_ACTIVE) ? 1 : 0;
		if (main_window && (became_active || (still_active && cursor_changed)))
		{
			update_ime_position_multiline_line_fixed(ctx, main_window, editor->text, edit_bounds);
		}
		prev_cursor = ctx->text_edit.cursor;
	}
	nk_end(ctx);

	// Properties（プロパティ領域）
	bounds = props_bounds;
	if (nk_begin(ctx, "Properties", bounds,
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
	{
		int i, line_count = 1;
		char stats[256];
		JapaneseTextEditor* current;

		/* Mode toggle + Run/Build: モバイルでは縦に積む、デスクトップは横並びを維持 */
#ifdef MOBILE_PORTRAIT
		/* モバイル：モード表示とトグルを縦に並べる */
		nk_layout_row_dynamic(ctx, 26, 1);
		if (app.is_interpreter_mode) nk_label(ctx, "Mode: Interpreter", NK_TEXT_LEFT);
		else nk_label(ctx, "Mode: Compiler", NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 32, 2);
		if (nk_button_label(ctx, "Compiler")) app.is_interpreter_mode = 0;
		if (nk_button_label(ctx, "Interpreter")) app.is_interpreter_mode = 1;

		/* Run / Build ボタンを大きめに横並び */
		nk_layout_row_dynamic(ctx, 40, 2);
		if (nk_button_label(ctx, "Run"))
		{
			/* Run behavior depends on mode */
			if (app.is_interpreter_mode)
			{
				/* call pcc.exe with the current title (assumed script) */
				char inner_cmd[1024]; char cmdline[2048];
				const char* title = app.editors[app.current_editor].title;
				if (strchr(title, ' ')) snprintf(inner_cmd, sizeof(inner_cmd), "pcc.exe \"%s\"", title);
				else snprintf(inner_cmd, sizeof(inner_cmd), "pcc.exe %s", title);
				snprintf(cmdline, sizeof(cmdline), "cmd.exe  %s", inner_cmd);
				/* start in new console */
				STARTUPINFOA si; PROCESS_INFORMATION pi; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si); ZeroMemory(&pi, sizeof(pi));
				if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
				{
					MessageBoxA(NULL, "Failed to start interpreter (pcc.exe)", "Run Error", MB_OK | MB_ICONERROR);
				}
				else { CloseHandle(pi.hProcess); CloseHandle(pi.hThread); }
			}
			else
			{
				char exe_name[512];
				char* dot;
				const char* title = app.editors[app.current_editor].title;
				strncpy(exe_name, title, sizeof(exe_name) - 1);
				exe_name[sizeof(exe_name) - 1] = '\0';
				dot = strrchr(exe_name, '.');
				if (dot && dot[1] && dot[2] == '\0')
				{
					char ch = dot[1];
					if (ch == 'c' || ch == 'C' || ch == 'h' || ch == 'H') *dot = '\0';
				}
				run_current_doc(exe_name);
			}
		}
		if (!app.is_interpreter_mode)
		{
			if (nk_button_label(ctx, "Build"))
			{
				char filename[512];
				const char* title = app.editors[app.current_editor].title;
				const char* ext = strrchr(title, '.');
				if (ext && strlen(ext) > 1) snprintf(filename, sizeof(filename), "%s", title);
				else snprintf(filename, sizeof(filename), "%s.c", title);
				build_current_doc_with_tcc(filename);
			}
		}
#else
		nk_layout_row_begin(ctx, NK_STATIC, 30, 3);
		nk_layout_row_push(ctx, 80);
		/* Mode display */
		if (app.is_interpreter_mode) nk_label(ctx, "Mode: Interpreter", NK_TEXT_LEFT);
		else nk_label(ctx, "Mode: Compiler", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, 60);
		/* Toggle buttons: Compiler / Interpreter */
		if (nk_button_label(ctx, "Compiler")) app.is_interpreter_mode = 0;
		nk_layout_row_push(ctx, 60);
		if (nk_button_label(ctx, "Interpreter")) app.is_interpreter_mode = 1;
		nk_layout_row_end(ctx);

		nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
		nk_layout_row_push(ctx, 130);
		if (nk_button_label(ctx, "Run"))
		{
			/* Run behavior depends on mode */
			if (app.is_interpreter_mode)
			{
				/* call pcc.exe with the current title (assumed script) */
				char inner_cmd[1024]; char cmdline[2048];
				const char* title = app.editors[app.current_editor].title;
				if (strchr(title, ' ')) snprintf(inner_cmd, sizeof(inner_cmd), "pcc.exe \"%s\"", title);
				else snprintf(inner_cmd, sizeof(inner_cmd), "pcc.exe %s", title);
				snprintf(cmdline, sizeof(cmdline), "cmd.exe  %s", inner_cmd);
				/* start in new console */
				STARTUPINFOA si; PROCESS_INFORMATION pi; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si); ZeroMemory(&pi, sizeof(pi));
				if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
				{
					MessageBoxA(NULL, "Failed to start interpreter (pcc.exe)", "Run Error", MB_OK | MB_ICONERROR);
				}
				else { CloseHandle(pi.hProcess); CloseHandle(pi.hThread); }
			}
			else
			{
				char exe_name[512];
				char* dot;
				const char* title = app.editors[app.current_editor].title;
				strncpy(exe_name, title, sizeof(exe_name) - 1);
				exe_name[sizeof(exe_name) - 1] = '\0';
				dot = strrchr(exe_name, '.');
				if (dot && dot[1] && dot[2] == '\0')
				{
					char ch = dot[1];
					if (ch == 'c' || ch == 'C' || ch == 'h' || ch == 'H') *dot = '\0';
				}
				run_current_doc(exe_name);
			}
		}
		nk_layout_row_push(ctx, 130);
		/* Build ボタンはインタープリタモードでは表示しない */
		if (!app.is_interpreter_mode)
		{
			if (nk_button_label(ctx, "Build"))
			{
				char filename[512];
				const char* title = app.editors[app.current_editor].title;
				const char* ext = strrchr(title, '.');
				if (ext && strlen(ext) > 1) snprintf(filename, sizeof(filename), "%s", title);
				else snprintf(filename, sizeof(filename), "%s.c", title);
				build_current_doc_with_tcc(filename);
			}
		}
		nk_layout_row_end(ctx);
#endif

	/* Tab width control */
#ifdef MOBILE_PORTRAIT
	nk_layout_row_dynamic(ctx, 28, 1);
	{
	    char tbstr[32];
	    snprintf(tbstr, sizeof(tbstr), "Tab width: %d", cfg_tab_width);
	    nk_label(ctx, tbstr, NK_TEXT_LEFT);
	}
	nk_layout_row_dynamic(ctx, 36, 3);
	if (nk_button_label(ctx, "-")) { if (cfg_tab_width > 1) cfg_tab_width--; }
	if (nk_button_label(ctx, "+")) { cfg_tab_width++; }
#else
	nk_layout_row_begin(ctx, NK_STATIC, 26, 3);
	nk_layout_row_push(ctx, 90);
	{
	    char tbstr[32];
	    snprintf(tbstr, sizeof(tbstr), "Tab width: %d", cfg_tab_width);
	    nk_label(ctx, tbstr, NK_TEXT_LEFT);
	}
	 nk_layout_row_push(ctx, 40);
	 if (nk_button_label(ctx, "-")) { if (cfg_tab_width > 1) cfg_tab_width--; }
	 nk_layout_row_push(ctx, 40);
	 if (nk_button_label(ctx, "+")) { cfg_tab_width++; }
	 nk_layout_row_end(ctx);
#endif
		/* Save tab width to ini immediately if changed */
		{
			char ini_path[MAX_PATH]; char curdir[MAX_PATH]; char tb[16];
			GetCurrentDirectoryA(MAX_PATH, curdir);
			snprintf(ini_path, sizeof(ini_path), "%s\\ide.ini", curdir);
			snprintf(tb, sizeof(tb), "%d", cfg_tab_width);
			WritePrivateProfileStringA("ide", "tab_width", tb, ini_path);
		}

		/* レイアウトの余白確保: 有効なレイアウト行が存在することを保証 */
		nk_layout_row_dynamic(ctx, 8, 1);

		/* ドキュメント選択: チェックボックスとボタンを縦に並べたリスト */
		for (i = 0; i < app.editor_count; ++i)
		{
			char tab_title[80];
			sprintf(tab_title, "%s%s", app.editors[i].title, app.editors[i].modified ? " *" : "");
			/* 2 列構成の行: 左に小さなチェックボックス、右にドキュメントボタン */
			nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
			nk_layout_row_push(ctx, 30); /* チェックボックス列 */
			nk_checkbox_label(ctx, "", &app.editors[i].checked);
			nk_layout_row_push(ctx, 250); /* ボタン列 */
			/* チェックが ON の場合はボタン背景をオレンジにしてわかりやすくする */
			if (app.editors[i].checked) {
				nk_style_push_style_item(ctx, &ctx->style.button.normal, nk_style_item_color(nk_rgb(255,165,0)));
				nk_style_push_style_item(ctx, &ctx->style.button.hover, nk_style_item_color(nk_rgb(255,195,100)));
				nk_style_push_style_item(ctx, &ctx->style.button.active, nk_style_item_color(nk_rgb(255,140,0)));
			}
			if (nk_button_label(ctx, tab_title)) app.current_editor = i;
			if (app.editors[i].checked) {
				nk_style_pop_style_item(ctx);
				nk_style_pop_style_item(ctx);
				nk_style_pop_style_item(ctx);
			}
			nk_layout_row_end(ctx);
		}
		/* 新規ドキュメントタイトル入力と + ボタン */
		nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
		nk_layout_row_push(ctx, 200);
		nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, app.new_doc_title, sizeof(app.new_doc_title), nk_filter_default);
		nk_layout_row_push(ctx, 60);
		if (nk_button_label(ctx, "+")) add_new_editor();
		nk_layout_row_end(ctx);

		/* 新規ドキュメント入力欄の下に配置された Open ボタン */
		nk_layout_row_begin(ctx, NK_STATIC, 30, 1);
		nk_layout_row_push(ctx, 260);
		if (nk_button_label(ctx, "Open"))
		{
			open_file_dialog_and_load();
		}
		nk_layout_row_end(ctx);

		/* セクション分離のための余白 */
		nk_layout_row_dynamic(ctx, 8, 1);

		nk_layout_row_dynamic(ctx, 25, 1);
		nk_label(ctx, "File Information", NK_TEXT_CENTERED);
		current = &app.editors[app.current_editor];

		for (i = 0; i < current->text_len; i++) if (current->text[i] == '\n') line_count++;
		nk_layout_row_dynamic(ctx, 20, 1);
		sprintf(stats, "Lines: %d", line_count);
		nk_label(ctx, stats, NK_TEXT_LEFT);
		sprintf(stats, "Characters: %d", current->text_len);
		nk_label(ctx, stats, NK_TEXT_LEFT);
		sprintf(stats, "Status: %s", current->modified ? "Modified" : "Saved");
		nk_label(ctx, stats, NK_TEXT_LEFT);
		nk_spacer(ctx);
		nk_label(ctx, "Keyboard Shortcuts", NK_TEXT_CENTERED);
		nk_layout_row_dynamic(ctx, 15, 1);
		nk_label(ctx, "Ctrl+S: Save", NK_TEXT_LEFT);
		nk_label(ctx, "Ctrl+N: New", NK_TEXT_LEFT);
		nk_label(ctx, "Ctrl+C: Copy", NK_TEXT_LEFT);
		nk_label(ctx, "Ctrl+V: Paste", NK_TEXT_LEFT);
		nk_label(ctx, "Ctrl+X: Cut", NK_TEXT_LEFT);
	}
	nk_end(ctx);

}

/* Win32 + D3D9 setup (demo style) */
static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY: PostQuitMessage(0); return 0;
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
	case WM_ENTERSIZEMOVE:
		/* ウィンドウのサイズ変更ドラッグ開始をメインループへポスト */
		PostMessageW(wnd, WM_APP + 1, 0, 0);
		return DefWindowProcW(wnd, msg, wparam, lparam);
	case WM_EXITSIZEMOVE:
		/* ドラッグ終了をメインループへポスト */
		PostMessageW(wnd, WM_APP + 2, 0, 0);
		return DefWindowProcW(wnd, msg, wparam, lparam);
	case WM_SIZE:
		/* WM_SIZE は SendMessage される場合があり、メッセージキューに入らないことがある。
		   そのためメインループで処理できるようキューへポストする。
		   WM_APP+3 をサイズ変更通知用に使う。*/
		PostMessageW(wnd, WM_APP + 3, wparam, lparam);
		break;

#else

	case WM_SIZE:
		if (device)
		{
			UINT width = LOWORD(lparam);
			UINT height = HIWORD(lparam);
			if (width != 0 && height != 0 && (width != present.BackBufferWidth || height != present.BackBufferHeight))
			{
				HRESULT hr;
				nk_d3d9_release();
				present.BackBufferWidth = width;
				present.BackBufferHeight = height;
				hr = IDirect3DDevice9_Reset(device, &present);
				NK_ASSERT(SUCCEEDED(hr));
				nk_d3d9_resize(width, height);
			}
		}
		break;
#endif
	}
	/* let backend handle input first; if not handled, provide fallback for arrow keys */
	if (nk_d3d9_handle_event(wnd, msg, wparam, lparam)) return 0;
	if (msg == WM_IME_STARTCOMPOSITION || msg == WM_IME_COMPOSITION)
	{
		if (main_window && (last_ime_x > 0 || last_ime_y > 0))
		{
			update_ime_position_simple(main_window, last_ime_x, last_ime_y);
		}
		return DefWindowProcW(wnd, msg, wparam, lparam);
	}
	if (msg == WM_KEYDOWN || msg == WM_KEYUP)
	{
		int down = (msg == WM_KEYDOWN) ? 1 : 0;
		switch ((UINT)wparam)
		{
		case VK_UP:
			nk_input_key(ctx, NK_KEY_UP, down);
			return 0;
		case VK_DOWN:
			nk_input_key(ctx, NK_KEY_DOWN, down);
			return 0;
		default:
			break;
		}
	}
	return DefWindowProcW(wnd, msg, wparam, lparam);
}

static void create_d3d9_device(HWND wnd)
{
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
	/* VC6: use the simpler initialization matching Nuklear_edit_main.c::init_d3d */
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d) return;

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
		/* failure - leave device NULL */
		return;
	}
#else
	HRESULT hr;
	memset(&present, 0, sizeof(present));
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

	{ /* try D3D9Ex if available */
		void* pProc = (void*)GetProcAddress(GetModuleHandleA("d3d9.dll"), "Direct3DCreate9Ex");
		if (pProc)
		{
			IDirect3D9Ex* d3d9ex = NULL;
			HRESULT(WINAPI * pDirect3DCreate9Ex)(UINT, IDirect3D9Ex**) = (HRESULT(WINAPI*)(UINT, IDirect3D9Ex**))pProc;
			if (SUCCEEDED(pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex)) && d3d9ex)
			{
				/* prefer hardware vertex processing first (avoid PUREDEVICE) */
				hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
					D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
					&present, NULL, &deviceEx);
				if (SUCCEEDED(hr))
				{
					device = (IDirect3DDevice9*)deviceEx;
				}
				else
				{
					/* fallback to software vertex processing */
					hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
						D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
						&present, NULL, &deviceEx);
					if (SUCCEEDED(hr)) device = (IDirect3DDevice9*)deviceEx;
				}
				IDirect3D9Ex_Release(d3d9ex);
			}
		}
	}

	if (!device)
	{
		IDirect3D9* d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
		if (!d3d9) return; /* cannot continue without d3d */

		/* try hardware vertex processing first */
		hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
			D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
			&present, &device);
		if (FAILED(hr))
		{
			/* fallback to software vertex processing */
			hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE,
				&present, &device);
			NK_ASSERT(SUCCEEDED(hr));
		}
		IDirect3D9_Release(d3d9);
	}
#endif
}
static int need_reset = 0; // リサイズ後のリソース再作成フラグ
int main(void)
{
	WNDCLASSW wc;
	RECT rect = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };
	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD exstyle = WS_EX_APPWINDOW;
	HWND wnd;
	int running = 1;
	/* VC6 互換: 関数先頭で変数を宣言 */
#if defined(_MSC_VER) && (_MSC_VER <= 1200)

	int in_sizemove = 0;
	RECT rc;
	UINT width;
	UINT height;
#endif

	/* initialize window class and register it */
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandleW(NULL);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"NuklearD3D9Japanese";
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	if (!RegisterClassW(&wc))
	{
		MessageBoxA(NULL, "RegisterClassW failed", "Error", MB_OK);
		return -1;
	}

	AdjustWindowRectEx(&rect, style, FALSE, exstyle);
	wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"Japanese Text Editor", style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, wc.hInstance, NULL);
	main_window = wnd;
	/* startup tcc_set removed: environment will be set at Build time instead */

	create_d3d9_device(wnd);

	/* Nuklear init */
	ctx = nk_d3d9_init(device, WINDOW_WIDTH, WINDOW_HEIGHT);
	if (!ctx) { MessageBoxA(NULL, "Failed to initialize Nuklear D3D9", "Error", MB_OK); return -1; }

	{
		/* 日本語レンジ指定 */
		static const nk_rune jp_ranges[] = {
			0x0020, 0x00FF, /* 基本ラテン文字 */
			0x3000, 0x30FF, /* ひらがな・カタカナ */
			0x31F0, 0x31FF, /* カタカナ音声拡張 */
			0xFF00, 0xFFEF, /* 全角ローマン + 半角カタカナ */
			0x4E00, 0x9FAF, /* CJK 統合漢字 */
			0
		};
		struct nk_font_config config;
		struct nk_font_atlas* atlas_local;
		struct nk_font* jp_font;
		FILE* fp;
		nk_d3d9_font_stash_begin(&atlas_local);

		config = nk_font_config(20.0f);
		config.range = jp_ranges;
		config.oversample_h = 2; /* サブピクセルレンダリングを改善 */
		config.oversample_v = 2;
		config.pixel_snap = 1;

		jp_font = NULL;
		{
			char fontpath[MAX_PATH];
			if (cfg_fonts_path[0]) {
				snprintf(fontpath, sizeof(fontpath), "%s\\MPLUS1p-Light.ttf", cfg_fonts_path);
			} else {
				snprintf(fontpath, sizeof(fontpath), "..\\..\\fonts\\MPLUS1p-Light.ttf");
			}
			fp = fopen(fontpath, "rb");
		}
		if (fp)
		{
			long font_size;
			void* font_data = NULL;
			if (fseek(fp, 0, SEEK_END) == 0)
			{
				font_size = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				font_data = malloc((size_t)font_size);
				if (font_data && fread(font_data, 1, (size_t)font_size, fp) == (size_t)font_size)
				{
					jp_font = nk_font_atlas_add_from_memory(atlas_local, font_data, (int)font_size, 20.0f, &config);
					/* 必要なら nk_d3d9_font_stash_end の後まで font_data を保持 */
				}
				else
				{
					free(font_data);
					font_data = NULL;
				}
			}
			fclose(fp);
		}

		if (!jp_font)
		{
			MessageBoxA(NULL, "日本語フォント MPLUS1p-Light.ttf のロードに失敗しました。\n日本語表示できません。", "Font Error", MB_ICONERROR | MB_OK);
			/* フォントが無ければ終了するかフォールバックを使う。ここでは終了する */
			exit(1);
		}

		nk_style_load_all_cursors(ctx, atlas_local->cursors);
		nk_style_set_font(ctx, &jp_font->handle);
		nk_d3d9_font_stash_end();
		/* No custom font wrapper needed - use standard font with tab expansion */
		atlas_ptr = atlas_local;
		/* 文字色だけ白にする（背景やボタン色は変更しない） */
		{
			/* グローバル/単純テキスト */
			ctx->style.text.color = nk_white;

			/* ウィンドウヘッダのラベル */
			ctx->style.window.header.label_normal = nk_white;
			ctx->style.window.header.label_hover = nk_white;
			ctx->style.window.header.label_active = nk_white;

			/* ボタン */
			ctx->style.button.text_normal = nk_white;
			ctx->style.button.text_hover = nk_white;
			ctx->style.button.text_active = nk_white;

			/* トグル / オプション / チェックボックス */
			ctx->style.option.text_normal = nk_white;
			ctx->style.option.text_hover = nk_white;
			ctx->style.option.text_active = nk_white;
			ctx->style.checkbox.text_normal = nk_white;
			ctx->style.checkbox.text_hover = nk_white;
			ctx->style.checkbox.text_active = nk_white;

			/* 選択可能 */
			ctx->style.selectable.text_normal = nk_white;
			ctx->style.selectable.text_hover = nk_white;
			ctx->style.selectable.text_pressed = nk_white;
			ctx->style.selectable.text_normal_active = nk_white;
			ctx->style.selectable.text_hover_active = nk_white;
			ctx->style.selectable.text_pressed_active = nk_white;

			/* プロパティラベル */
			ctx->style.property.label_normal = nk_white;
			ctx->style.property.label_hover = nk_white;
			ctx->style.property.label_active = nk_white;

			/* 編集 / テキストエディタ */
			ctx->style.edit.text_normal = nk_white;
			ctx->style.edit.text_hover = nk_white;
			ctx->style.edit.text_active = nk_white;
			ctx->style.edit.selected_text_normal = nk_white;
			ctx->style.edit.selected_text_hover = nk_white;
			ctx->style.edit.cursor_text_normal = nk_white;
			ctx->style.edit.cursor_text_hover = nk_white;

			/* コンボとタブのラベル */
			ctx->style.combo.label_normal = nk_white;
			ctx->style.combo.label_hover = nk_white;
			ctx->style.combo.label_active = nk_white;
			ctx->style.tab.text = nk_white;
		}
	}

	/* ini の読み込みまたは作成 */
	load_or_create_ini();

	init_app();

	while (running)
	{
		HRESULT hr;
		MSG msg;
		nk_input_begin(ctx);
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = 0;
			}
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
			else if (msg.message == WM_ENTERSIZEMOVE)
			{
				in_sizemove = 1; // ユーザによるサイズ変更（ドラッグ）開始
			}
			/* WindowProc からポストされたサイズ系メッセージを受け取る */
			else if (msg.message == (WM_APP + 1))
			{
				in_sizemove = 1; // ドラッグ開始（ポスト経由）
			}
			else if (msg.message == WM_EXITSIZEMOVE)
			{
				in_sizemove = 0; // ドラッグ終了。ここで最終サイズを取得してリセットを行う
				if (device && main_window)
				{
					GetClientRect(main_window, &rc);
					width = (UINT)(rc.right - rc.left);
					height = (UINT)(rc.bottom - rc.top);
					if (width != 0 && height != 0 && (width != present.BackBufferWidth || height != present.BackBufferHeight))
					{
						// Reset 前にGPUリソースを解放
						nk_d3d9_release();
						present.BackBufferWidth = width;
						present.BackBufferHeight = height;
						need_reset = 1;
					}
				}
			}
			else if (msg.message == (WM_APP + 2))
			{
				/* ドラッグ終了（ポスト経由）*/
				in_sizemove = 0;
				if (device && main_window)
				{
					GetClientRect(main_window, &rc);
					width = (UINT)(rc.right - rc.left);
					height = (UINT)(rc.bottom - rc.top);
					if (width != 0 && height != 0 && (width != present.BackBufferWidth || height != present.BackBufferHeight))
					{
						nk_d3d9_release();
						present.BackBufferWidth = width;
						present.BackBufferHeight = height;
						need_reset = 1;
					}
				}
			}
			else if (msg.message == WM_SIZE)
			{
				// プログラムからのサイズ変更や最大化など、
				// ドラッグ中でなければ即時処理する
				if (device)
				{
					width = LOWORD(msg.lParam);
					height = HIWORD(msg.lParam);
					if (width == 0 || height == 0)
					{
						// 最小化等は無視
					}
					else if (!in_sizemove)
					{
						if (width != present.BackBufferWidth || height != present.BackBufferHeight)
						{
							nk_d3d9_release();
							present.BackBufferWidth = width;
							present.BackBufferHeight = height;
							need_reset = 1;
						}
					}
					else
					{
						// ドラッグ中は最終サイズを待つために単に記録する
						present.BackBufferWidth = width;
						present.BackBufferHeight = height;
					}
				}
			}
			else if (msg.message == (WM_APP + 3))
			{
				/* WindowProc からポストされた WM_SIZE の代替 */
				if (device)
				{
					width = LOWORD(msg.wParam);
					height = HIWORD(msg.wParam);
					if (width == 0 || height == 0)
					{
						/* ignore */
					}
					else if (!in_sizemove)
					{
						if (width != present.BackBufferWidth || height != present.BackBufferHeight)
						{
							nk_d3d9_release();
							present.BackBufferWidth = width;
							present.BackBufferHeight = height;
							need_reset = 1;
						}
					}
					else
					{
						present.BackBufferWidth = width;
						present.BackBufferHeight = height;
					}
				}
			}
#endif
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		nk_input_end(ctx);
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
		// リサイズ後のリソース再作成
		if (need_reset)
		{
			hr = IDirect3DDevice9_Reset(device, &present);
			if (SUCCEEDED(hr))
			{

				nk_d3d9_resize(present.BackBufferWidth, present.BackBufferHeight);
				need_reset = 0;
			}
			else
			{
				// 失敗時は描画スキップ
				continue;
			}
		}
#endif
		/* debug selection logging removed */

		/* advance frame counter for IME caching */
		global_frame_counter++;

		if (GetAsyncKeyState(VK_CONTROL) & 0x8000) { static int save_pressed = 0, new_pressed = 0; if (GetAsyncKeyState('S') & 0x8000) { if (!save_pressed) { save_file(&app.editors[app.current_editor]); save_pressed = 1; } } else save_pressed = 0; if (GetAsyncKeyState('N') & 0x8000) { if (!new_pressed) { add_new_editor(); new_pressed = 1; } } else new_pressed = 0; }

		draw_gui();

		hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_COLORVALUE(app.bg_color.r / 255.0f, app.bg_color.g / 255.0f, app.bg_color.b / 255.0f, 1.0f), 1.0f, 0);
		NK_ASSERT(SUCCEEDED(hr));
		hr = IDirect3DDevice9_BeginScene(device); NK_ASSERT(SUCCEEDED(hr));
		nk_d3d9_render(NK_ANTI_ALIASING_ON);
		hr = IDirect3DDevice9_EndScene(device); NK_ASSERT(SUCCEEDED(hr));

		if (deviceEx) hr = IDirect3DDevice9Ex_PresentEx(deviceEx, NULL, NULL, NULL, NULL, 0); else hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
		if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICEHUNG || hr == D3DERR_DEVICEREMOVED) { MessageBoxW(NULL, L"D3D9 device is lost or removed!", L"Error", 0); break; }
		else if (hr == S_PRESENT_OCCLUDED) Sleep(10);
	}

	nk_d3d9_shutdown(); if (deviceEx) IDirect3DDevice9Ex_Release(deviceEx); else IDirect3DDevice9_Release(device); UnregisterClassW(wc.lpszClassName, wc.hInstance);
	return 0;
}

/* ファイルを開くダイアログを表示し、現在のエディタに読み込む（現在のエディタに読み込む想定） */
static void open_file_dialog_and_load(void)
{
	char filename[MAX_PATH];
	OPENFILENAMEA ofn;
	memset(filename, 0, sizeof(filename));
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = main_window;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrFilter = "c h Files\0*.c\0*.h\0All Files\0*.*\0";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	if (GetOpenFileNameA(&ofn))
	{
		unsigned char* buf;
		size_t r;
		int start = 0;
		int copy;
		char* p1;
		char* p2;
		char* base;
		FILE* f = fopen(filename, "rb");
		if (!f) { MessageBoxA(NULL, "Failed to open file", "Error", MB_OK | MB_ICONERROR); return; }
		buf = (unsigned char*)malloc(MAX_TEXT_SIZE);
		if (!buf) { fclose(f); MessageBoxA(NULL, "Out of memory", "Error", MB_OK | MB_ICONERROR); return; }
		r = fread(buf, 1, MAX_TEXT_SIZE - 1, f);
		fclose(f);

	/* 初期値 */
	app.editors[app.current_editor].encoding = ENC_UNKNOWN;
	app.editors[app.current_editor].has_bom = 0;
	app.editors[app.current_editor].newline = NEWLINE_UNKNOWN;
	app.editors[app.current_editor].file_type = FILETYPE_UNKNOWN;
	/* Open直後はchecked=1にする（自動的にビルド対象） */
	app.editors[app.current_editor].checked = 1;

		/* BOM / 文字コードの簡易判定と変換（必要なら UTF-16 -> UTF-8 に変換） */
		if (r >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF)
		{
			/* UTF-8 BOM */
			start = 3;
			app.editors[app.current_editor].encoding = ENC_UTF8;
			app.editors[app.current_editor].has_bom = 1;
			copy = (int)(r - start);
			if (copy < 0) copy = 0;
			if (copy > MAX_TEXT_SIZE - 1) copy = MAX_TEXT_SIZE - 1;
			memcpy(app.editors[app.current_editor].text, buf + start, copy);
			app.editors[app.current_editor].text[copy] = '\0';
			app.editors[app.current_editor].text_len = copy;
			/* tabs preserved in internal buffer */
		}
		else if (r >= 2 && buf[0] == 0xFF && buf[1] == 0xFE)
		{
			int wchar_count;
			wchar_t* wbuf;
			/* UTF-16 LE BOM */
			start = 2;
			app.editors[app.current_editor].encoding = ENC_UTF16_LE;
			app.editors[app.current_editor].has_bom = 1;
			/* UTF-16LE を UTF-8 に変換して格納 */
			wchar_count = (int)((r - start) / 2);
			if (wchar_count < 0) wchar_count = 0;
			wbuf = (wchar_t*)malloc((wchar_count + 1) * sizeof(wchar_t));
			if (wbuf)
			{
				int i,needed;
				for (i = 0; i < wchar_count; ++i)
				{
					unsigned char lo = buf[start + i * 2];
					unsigned char hi = buf[start + i * 2 + 1];
					wbuf[i] = (wchar_t)((hi << 8) | lo);
				}
				wbuf[wchar_count] = 0;
				/* WideChar -> UTF-8 */
				needed = WideCharToMultiByte(CP_UTF8, 0, wbuf, wchar_count, NULL, 0, NULL, NULL);
				if (needed > 0)
				{
					if (needed > MAX_TEXT_SIZE - 1) needed = MAX_TEXT_SIZE - 1;
					WideCharToMultiByte(CP_UTF8, 0, wbuf, wchar_count, app.editors[app.current_editor].text, needed, NULL, NULL);
					app.editors[app.current_editor].text[needed] = '\0';
					app.editors[app.current_editor].text_len = needed;
					/* tabs preserved in internal buffer */
				}
				else
				{
					app.editors[app.current_editor].text[0] = '\0';
					app.editors[app.current_editor].text_len = 0;
				}
				free(wbuf);
			}
		}
		else if (r >= 2 && buf[0] == 0xFE && buf[1] == 0xFF)
		{
			int wchar_count,needed;
			wchar_t* wbuf;
			/* UTF-16 BE BOM */
			start = 2;
			app.editors[app.current_editor].encoding = ENC_UTF16_BE;
			app.editors[app.current_editor].has_bom = 1;
			wchar_count = (int)((r - start) / 2);
			if (wchar_count < 0) wchar_count = 0;
			wbuf = (wchar_t*)malloc((wchar_count + 1) * sizeof(wchar_t));
			if (wbuf)
			{
				int i;
				for (i = 0; i < wchar_count; ++i)
				{
					unsigned char hi = buf[start + i * 2];
					unsigned char lo = buf[start + i * 2 + 1];
					/* バイト順を反転して LE に直す */
					wbuf[i] = (wchar_t)((lo) | (hi << 8));
				}
				wbuf[wchar_count] = 0;
				needed = WideCharToMultiByte(CP_UTF8, 0, wbuf, wchar_count, NULL, 0, NULL, NULL);
				if (needed > 0)
				{
					if (needed > MAX_TEXT_SIZE - 1) needed = MAX_TEXT_SIZE - 1;
					WideCharToMultiByte(CP_UTF8, 0, wbuf, wchar_count, app.editors[app.current_editor].text, needed, NULL, NULL);
					app.editors[app.current_editor].text[needed] = '\0';
					app.editors[app.current_editor].text_len = needed;
					/* tabs preserved in internal buffer */
				}
				else
				{
					app.editors[app.current_editor].text[0] = '\0';
					app.editors[app.current_editor].text_len = 0;
				}
				free(wbuf);
			}
		}
		else
		{
			/* BOM が無い場合はそのままコピー（UTF-8 か ANSI の想定）。エンコーディングは不明のままにする。 */
			copy = (int)r;
			if (copy < 0) copy = 0;
			if (copy > MAX_TEXT_SIZE - 1) copy = MAX_TEXT_SIZE - 1;
			memcpy(app.editors[app.current_editor].text, buf, copy);
			app.editors[app.current_editor].text[copy] = '\0';
			app.editors[app.current_editor].text_len = copy;
			/* tabs preserved in internal buffer */
		}
		app.editors[app.current_editor].modified = 0;

		/* 改行種別の検出（UTF-8 に変換済みのバッファを走査） */
		{
			const char* t = app.editors[app.current_editor].text;
			int i;
			int found_crlf = 0;
			int found_lf = 0;
			for (i = 0; t[i] != '\0' && i < app.editors[app.current_editor].text_len; ++i)
			{
				if (t[i] == '\r' && t[i+1] == '\n') { found_crlf = 1; break; }
				if (t[i] == '\n') { found_lf = 1; }
			}
			if (found_crlf) app.editors[app.current_editor].newline = NEWLINE_CRLF;
			else if (found_lf) app.editors[app.current_editor].newline = NEWLINE_LF;
			else app.editors[app.current_editor].newline = NEWLINE_UNKNOWN;
		}

		/* ファイル拡張子から file_type を設定 */
		{
			const char* ext = strrchr(filename, '.');
			if (ext)
			{
				if (_stricmp(ext, ".c") == 0) app.editors[app.current_editor].file_type = FILETYPE_C;
				else if (_stricmp(ext, ".h") == 0) app.editors[app.current_editor].file_type = FILETYPE_H;
				else if (_stricmp(ext, ".cs") == 0) app.editors[app.current_editor].file_type = FILETYPE_CS;
				else if (_stricmp(ext, ".py") == 0) app.editors[app.current_editor].file_type = FILETYPE_PY;
				else if (_stricmp(ext, ".php") == 0) app.editors[app.current_editor].file_type = FILETYPE_PHP;
				else if (_stricmp(ext, ".ini") == 0) app.editors[app.current_editor].file_type = FILETYPE_INI;
				else if (_stricmp(ext, ".log") == 0) app.editors[app.current_editor].file_type = FILETYPE_LOG;
				else if (_stricmp(ext, ".sql") == 0) app.editors[app.current_editor].file_type = FILETYPE_SQL;
				else if (_stricmp(ext, ".bat") == 0) app.editors[app.current_editor].file_type = FILETYPE_BAT;
				else if (_stricmp(ext, ".ps1") == 0) app.editors[app.current_editor].file_type = FILETYPE_PS1;
				else if (_stricmp(ext, ".html") == 0 || _stricmp(ext, ".htm") == 0) app.editors[app.current_editor].file_type = FILETYPE_HTML;
				else if (_stricmp(ext, ".css") == 0) app.editors[app.current_editor].file_type = FILETYPE_CSS;
				else if (_stricmp(ext, ".txt") == 0) app.editors[app.current_editor].file_type = FILETYPE_TXT;
				else app.editors[app.current_editor].file_type = FILETYPE_OTHER;
			}
		}
		/* set title to basename */
		p1 = strrchr(filename, '\\');
		p2 = strrchr(filename, '/');
		base = (p1 > p2) ? p1 : p2;
		if (base) base++; else base = filename;
		strncpy(app.editors[app.current_editor].title, base, sizeof(app.editors[app.current_editor].title) - 1);
		app.editors[app.current_editor].title[sizeof(app.editors[app.current_editor].title) - 1] = '\0';
		free(buf);
	}
}

/* dev サブフォルダにある tcc_set.bat を起動時に実行する処理（元の説明）。
	カレント作業ディレクトリを保存して dev に移動し、バッチを実行して元に戻す想定でした。 */
	/* run_startup_tcc_set はユーザの要望で削除済み。環境はビルド時に設定されます。 */

	/* 指定したファイル名を tcc でビルドする関数。
		戻り値はなく、filename はソースファイルへのフルパスまたは相対パスで指定します。 */
static void build_current_doc_with_tcc(const char* filename)
{
	char cmdline[1024];
	char inner_cmd[1024];
	int i;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	/* チェックボックスが ON のエディタを全てビルドする */
	for (i = 0; i < app.editor_count; ++i)
	{
		const char* title;
		if (!app.editors[i].checked) continue;
		title = app.editors[i].title;
		/* title はファイル名かもしれないのでそのままコマンドに使う。空白があれば引用する。 */
		if (!title || !title[0]) continue;
		if (!cfg_compiler_opts[0]) {
			strncpy(cfg_compiler_opts, "-lopengl32 -lglut32 -lgdi32 -luser32 -lkernel32 -lgdi32 -lmsvcrt -ld3d9 -limm32 -lcomdlg32 -g -ld3dcompiler_47", sizeof(cfg_compiler_opts)-1);
			cfg_compiler_opts[sizeof(cfg_compiler_opts)-1] = '\0';
		}
		if (strchr(title, ' '))
			snprintf(inner_cmd, sizeof(inner_cmd), "tcc.exe \"%s\" %s", title, cfg_compiler_opts);
		else
			snprintf(inner_cmd, sizeof(inner_cmd), "tcc.exe %s %s", title, cfg_compiler_opts);

		{
			char tcc_dir[MAX_PATH];
			char src_dir[MAX_PATH];
			/* use cfg paths if present, otherwise fallback to dev and current */
			if (cfg_tcc_path[0]) snprintf(tcc_dir, sizeof(tcc_dir), "%s", cfg_tcc_path);
			else snprintf(tcc_dir, sizeof(tcc_dir), "dev");
			if (cfg_src_path[0]) snprintf(src_dir, sizeof(src_dir), "%s", cfg_src_path);
			else snprintf(src_dir, sizeof(src_dir), "%%CD%%");

			/* Construct command: cd /d "tcc_dir" & call tcc_set.bat & cd /d "src_dir" & <inner_cmd> */
			snprintf(cmdline, sizeof(cmdline), "cmd.exe /C cd /d \"%s\" & call tcc_set.bat & cd /d \"%s\" & %s", tcc_dir, src_dir, inner_cmd);
		}

		/* 既存のコンソールを使うか親へアタッチ、なければ新規作成 */
		{
			int attached = 0;
			int attached_here = 0;
			if (GetConsoleWindow()) attached = 1;
			else if (AttachConsole(ATTACH_PARENT_PROCESS)) { attached = 1; attached_here = 1; }
			if (attached)
			{
				HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
				if (hOut && hOut != INVALID_HANDLE_VALUE)
				{
					DWORD written; char showbuf[2048]; snprintf(showbuf, sizeof(showbuf), "%s\r\n", inner_cmd); WriteFile(hOut, showbuf, (DWORD)strlen(showbuf), &written, NULL);
				}
				if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
				{
					CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
				}
				else
				{
					DWORD err = GetLastError();
					char emsg[256];
					snprintf(emsg, sizeof(emsg), "Failed to start tcc build for %s\nGetLastError=%lu", title, (unsigned long)err);
					MessageBoxA(NULL, emsg, "Build Error", MB_OK | MB_ICONERROR);
				}
				if (attached_here) FreeConsole();
			}
				else
				{
					char newcmd[4096];
					/* same composition as cmdline but keep shell open (/K) */
					char tcc_dir[MAX_PATH]; char src_dir[MAX_PATH];
					if (cfg_tcc_path[0]) snprintf(tcc_dir, sizeof(tcc_dir), "%s", cfg_tcc_path);
					else snprintf(tcc_dir, sizeof(tcc_dir), "dev");
					if (cfg_src_path[0]) snprintf(src_dir, sizeof(src_dir), "%s", cfg_src_path);
					else snprintf(src_dir, sizeof(src_dir), "%%CD%%");
					snprintf(newcmd, sizeof(newcmd), "cmd.exe /K cd /d \"%s\" & call tcc_set.bat & cd /d \"%s\" & %s", tcc_dir, src_dir, inner_cmd);
					if (CreateProcessA(NULL, newcmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
					{
						CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
					}
					else
					{
						DWORD err = GetLastError();
						char emsg[256];
						snprintf(emsg, sizeof(emsg), "Failed to start tcc build for %s\nGetLastError=%lu", title, (unsigned long)err);
						MessageBoxA(NULL, emsg, "Build Error", MB_OK | MB_ICONERROR);
					}
				}
		}
	}
}

/* Run the given executable name (without extension) by changing to dev, calling tcc_set.bat, then returning and running the exe. */
static void run_current_doc(const char* exe_name)
{
	char inner_cmd[1024];
	char cmdline[2048];
	PROCESS_INFORMATION pi;
	STARTUPINFOA si;
	if (!exe_name || !exe_name[0]) return;
	ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	/* run <exe_name>.exe */
	snprintf(inner_cmd, sizeof(inner_cmd), "%s.exe", exe_name);
	snprintf(cmdline, sizeof(cmdline), "cmd.exe /C %s", inner_cmd);

	{
		int attached = 0;
		int attached_here = 0;
		if (GetConsoleWindow()) attached = 1;
		else if (AttachConsole(ATTACH_PARENT_PROCESS)) { attached = 1; attached_here = 1; }
		if (attached)
		{
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			if (hOut && hOut != INVALID_HANDLE_VALUE)
			{
				DWORD written; char showbuf[1024]; snprintf(showbuf, sizeof(showbuf), "%s\r\n", inner_cmd); WriteFile(hOut, showbuf, (DWORD)strlen(showbuf), &written, NULL);
			}
			if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			{
				CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
			}
			else
			{
				MessageBoxA(NULL, "Failed to start run process in existing console", "Run Error", MB_OK | MB_ICONERROR);
			}
			if (attached_here) FreeConsole();
		}
		else
		{
			char newcmd[2048];
			snprintf(newcmd, sizeof(newcmd), "cmd.exe /K %s", inner_cmd);
			if (CreateProcessA(NULL, newcmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
			{
				CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
			}
			else
			{
				MessageBoxA(NULL, "Failed to start run process", "Run Error", MB_OK | MB_ICONERROR);
			}
		}
	}
}

/* ini を読み込み、無ければ現在のカレントフォルダを使って新規作成する。
   keys: tcc_path, fonts_path, src_path
*/
static void load_or_create_ini(void)
{
	char ini_path[MAX_PATH];
	char curdir[MAX_PATH];
	char tmp[MAX_PATH];
	DWORD r;

	/* ide.ini は実行ファイルのカレントディレクトリに置く */
	GetCurrentDirectoryA(MAX_PATH, curdir);
	snprintf(ini_path, sizeof(ini_path), "%s\\ide.ini", curdir);

	/* 既にファイルがあるかをチェック */
	r = GetFileAttributesA(ini_path);
	if (r != INVALID_FILE_ATTRIBUTES)
	{
		/* 読み込み: 存在する -> 読み出しだけ行う */
		GetPrivateProfileStringA("ide", "tcc_path", "", cfg_tcc_path, sizeof(cfg_tcc_path), ini_path);
		GetPrivateProfileStringA("ide", "fonts_path", "", cfg_fonts_path, sizeof(cfg_fonts_path), ini_path);
		GetPrivateProfileStringA("ide", "src_path", "", cfg_src_path, sizeof(cfg_src_path), ini_path);
		GetPrivateProfileStringA("ide", "compiler_opts", "", cfg_compiler_opts, sizeof(cfg_compiler_opts), ini_path);
		/* tab width (整数) */
		{
			char tb[16] = "4";
			GetPrivateProfileStringA("ide", "tab_width", "4", tb, sizeof(tb), ini_path);
			cfg_tab_width = atoi(tb);
			if (cfg_tab_width <= 0) cfg_tab_width = 4;
		}
	}
	else
	{
		/* 無ければカレントディレクトリを各パスに設定して保存する */
		/* tcc_path: カレント */
		strncpy(cfg_tcc_path, curdir, sizeof(cfg_tcc_path)-1); cfg_tcc_path[sizeof(cfg_tcc_path)-1] = '\0';
		/* fonts_path: ../..\\fonts 相当を探す。存在しなければ curdir を使う */
		snprintf(tmp, sizeof(tmp), "%s\\..\\..\\fonts", curdir);
		/* normalize - not strictly necessary */
		GetFullPathNameA(tmp, sizeof(tmp), cfg_fonts_path, NULL);
		if (GetFileAttributesA(cfg_fonts_path) == INVALID_FILE_ATTRIBUTES) {
			/* フォルダが無ければカレントを設定 */
			strncpy(cfg_fonts_path, curdir, sizeof(cfg_fonts_path)-1); cfg_fonts_path[sizeof(cfg_fonts_path)-1] = '\0';
		}
		/* src_path: カレント */
		strncpy(cfg_src_path, curdir, sizeof(cfg_src_path)-1); cfg_src_path[sizeof(cfg_src_path)-1] = '\0';

		/* 保存 */
		WritePrivateProfileStringA("ide", "tcc_path", cfg_tcc_path, ini_path);
		WritePrivateProfileStringA("ide", "fonts_path", cfg_fonts_path, ini_path);
		WritePrivateProfileStringA("ide", "src_path", cfg_src_path, ini_path);
		/* tab width */
		{
			char tb[16];
			snprintf(tb, sizeof(tb), "%d", cfg_tab_width);
			WritePrivateProfileStringA("ide", "tab_width", tb, ini_path);
		}
		/* compiler options */
		if (!cfg_compiler_opts[0]) {
			strncpy(cfg_compiler_opts, "-lopengl32 -lglut32 -lgdi32 -luser32 -lkernel32 -lgdi32 -lmsvcrt -ld3d9 -limm32 -lcomdlg32 -g -ld3dcompiler_47", sizeof(cfg_compiler_opts)-1);
			cfg_compiler_opts[sizeof(cfg_compiler_opts)-1] = '\0';
		}
		WritePrivateProfileStringA("ide", "compiler_opts", cfg_compiler_opts, ini_path);
	}
}

/* Helper: expand internal text (with '\t') into display buffer (tabs->spaces) */
static void internal_to_display(const char* internal, char* display, int tabw)
{
	int i = 0, o = 0;
	if (!internal || !display) return;
	while (internal[i]) {
		if (internal[i] == '\t') {
			int k;
			for (k = 0; k < tabw; ++k) display[o++] = ' ';
			i++;
		} else {
			display[o++] = internal[i++];
		}
	}
	display[o] = '\0';
}

/* Helper: convert display buffer back to internal (collapse runs of spaces to tabs where appropriate).
   This is a conservative mapping: sequences of exactly tabw spaces become a single '\t'. */
static void display_to_internal(const char* display, char* internal, int tabw)
{
	int i = 0, o = 0;
	if (!display || !internal) return;
	while (display[i]) {
		if (display[i] == ' ') {
			int j = i; int count = 0;
			while (display[j] == ' ' && count < tabw) { j++; count++; }
			if (count == tabw) { internal[o++] = '\t'; i += tabw; }
			else internal[o++] = display[i++];
		} else {
			internal[o++] = display[i++];
		}
	}
	internal[o] = '\0';
}

/* 内部テキストと表示上のカーサ位置（文字数）から対応する内部インデックスを返す。
	タブは tabw 個のスペースに展開されることを想定。 */
static int display_to_internal_index_using_internal(const char* internal, int disp_idx, int tabw)
{
	int i = 0, disp = 0;
	if (!internal) return 0;
	while (internal[i]) {
		if (internal[i] == '\t') {
			if (disp + tabw > disp_idx) return i; /* cursor inside tab -> return tab char index */
			disp += tabw;
			i++;
		} else {
			if (disp == disp_idx) return i;
			disp++;
			i++;
		}
	}
	return i; /* end */
}

/* 内部インデックスを表示上のカーソル位置（文字数）に変換する */
static int internal_index_to_display_cursor(const char* internal, int internal_idx, int tabw)
{
	int i = 0, disp = 0;
	if (!internal) return 0;
	while (i < internal_idx && internal[i]) {
		if (internal[i] == '\t') { disp += tabw; i++; }
		else { disp++; i++; }
	}
	return disp;
}
