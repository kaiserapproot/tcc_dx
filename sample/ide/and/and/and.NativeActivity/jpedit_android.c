#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "NkGLES.h"
#include "nuklear.h"

#define MAX_TEXT_SIZE 65536
#if 0 /* jpedit_android.c disabled: main.cpp provides the implementation */

/* シンプルなタブ展開/復元（デスクトップ版と同様のロジックを採用） */
static char* expand_tabs_to_spaces(const char* text, int tab_width, char* buffer, int buffer_size)
{
    if (!text || !buffer) return NULL;
    int out_pos = 0;
    int column = 0;
    int len = (int)strlen(text);
    for (int i = 0; i < len && out_pos < buffer_size - 1; ++i) {
        if (text[i] == '\t') {
            int next_tab_stop = ((column / tab_width) + 1) * tab_width;
            int spaces_needed = next_tab_stop - column;
            for (int j = 0; j < spaces_needed && out_pos < buffer_size - 1; ++j) buffer[out_pos++] = ' ';
            column = next_tab_stop;
        } else if (text[i] == '\n' || text[i] == '\r') {
            buffer[out_pos++] = text[i];
            column = 0;
        } else {
            buffer[out_pos++] = text[i];
            column++;
        }
    }
    buffer[out_pos] = '\0';
    return buffer;
}

static void collapse_spaces_to_tabs(const char* expanded_text, char* internal_text, int tab_width, int buffer_size)
{
    if (!expanded_text || !internal_text) return;
    int in_pos = 0, out_pos = 0;
    int column = 0;
    int len = (int)strlen(expanded_text);
    while (in_pos < len && out_pos < buffer_size - 1) {
        if (expanded_text[in_pos] == ' ') {
            int space_count = 0;
            int temp_pos = in_pos;
            while (temp_pos < len && expanded_text[temp_pos] == ' ') { space_count++; temp_pos++; }
            int spaces_to_tab_stop = tab_width - (column % tab_width);
            if ((column % tab_width) != 0 && space_count >= spaces_to_tab_stop) {
                internal_text[out_pos++] = '\t';
                in_pos += spaces_to_tab_stop;
                column += spaces_to_tab_stop;
                space_count -= spaces_to_tab_stop;
                while (space_count >= tab_width && out_pos < buffer_size - 1) {
                    internal_text[out_pos++] = '\t';
                    in_pos += tab_width;
                    column += tab_width;
                    space_count -= tab_width;
                }
                while (space_count > 0 && out_pos < buffer_size - 1) {
                    internal_text[out_pos++] = ' ';
                    in_pos++; column++; space_count--;
                }
            } else {
                internal_text[out_pos++] = expanded_text[in_pos++]; column++;
            }
        } else if (expanded_text[in_pos] == '\n' || expanded_text[in_pos] == '\r') {
            internal_text[out_pos++] = expanded_text[in_pos++]; column = 0;
        } else {
            internal_text[out_pos++] = expanded_text[in_pos++]; column++;
        }
    }
    internal_text[out_pos] = '\0';
}

/* 外部から呼べる描画関数 */
void nk_draw_jpedit(struct nk_context* ctx)
{
    static char internal_text[MAX_TEXT_SIZE] = "// Japanese editor (Android)\n// type here...\n";
    static int initialized = 0;
    if (!initialized) { initialized = 1; }

    int tabw = 4;
    struct nk_rect bounds = nk_rect(10, 10, 300, 400);

    if (nk_begin(ctx, "Japanese Editor", bounds,
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 28, 1);
        if (nk_button_label(ctx, "Save")) {
            /* Android 上の保存はここでは簡易ログ出力に留める */
            nk_show_tooltip(ctx, "Save not implemented on Android sample");
        }
        if (nk_button_label(ctx, "Clear")) { internal_text[0] = '\0'; }

        /* タブ展開 -> 編集 -> 変換戻し のワークフロー */
        static char expanded[MAX_TEXT_SIZE];
        static char temp[MAX_TEXT_SIZE];
        expand_tabs_to_spaces(internal_text, tabw, expanded, sizeof(expanded));
        nk_flags edit_flags = NK_EDIT_FIELD | NK_EDIT_MULTILINE | NK_EDIT_SELECTABLE | NK_EDIT_ALLOW_TAB | NK_EDIT_CLIPBOARD;
        nk_edit_string_zero_terminated(ctx, edit_flags, expanded, sizeof(expanded), nk_filter_default);
        collapse_spaces_to_tabs(expanded, temp, tabw, sizeof(temp));
        if (strcmp(temp, internal_text) != 0) {
            strncpy(internal_text, temp, sizeof(internal_text)-1);
            internal_text[sizeof(internal_text)-1] = '\0';
        }
    }
    nk_end(ctx);
}

/* ヘッダ代わりの簡易エクスポート */
#ifdef __cplusplus
extern "C" {
#endif
void nk_draw_jpedit(struct nk_context* ctx);
#ifdef __cplusplus
}
#endif
