#include "dock_manager.h"
#ifdef DOCK_UI_USE_NUKLEAR
#include "nuklear.h"
#endif

/* シンプルなタブ UI レンダラ
 * - 呼び出し側が適切なレイアウト領域を確保しておくことを想定
 * - 各タブは水平に並べられ、ボタン押下でアクティブタブを切り替える
 */
static void dock_ui_draw_node(struct DockManager *mgr, DockNode *node, struct nk_context *ctx) {
    if (!node) return;
#ifdef DOCK_UI_USE_NUKLEAR
    if (!ctx) return;
    if (node->type == DOCK_NODE_TAB) {
        int cols = node->tabs_count > 0 ? node->tabs_count : 1;
        float tab_h = 22.0f;
        nk_layout_row_dynamic(ctx, tab_h, cols);
        for (int i = 0; i < node->tabs_count; ++i) {
            const char *label = node->tabs[i] ? node->tabs[i]->title : "(null)";
            if (nk_button_label(ctx, label)) {
                node->active_tab = i;
            }
            /* ドラッグ開始は押下時に判定（ボタン押下と同時にドラッグ開始） */
            if (nk_input_is_mouse_pressed(&ctx->input, NK_BUTTON_LEFT)) {
                if (mgr) {
                    mgr->dragging = 1;
                    mgr->drag_node = node;
                    mgr->drag_index = i;
                    mgr->drag_window = node->tabs[i];
                }
            }
        }
    } else if (node->type == DOCK_NODE_SPLIT) {
        dock_ui_draw_node(mgr, node->child[0], ctx);
        dock_ui_draw_node(mgr, node->child[1], ctx);
    }
#else
    /* Nuklear が使えない環境向けのフォールバック: UI 操作は行わないが
     * active_tab が未設定なら先頭をアクティブにする
     */
    if (node->type == DOCK_NODE_TAB) {
        if (node->active_tab < 0 && node->tabs_count > 0) node->active_tab = 0;
    } else if (node->type == DOCK_NODE_SPLIT) {
        dock_ui_draw_node(NULL, node->child[0], NULL);
        dock_ui_draw_node(NULL, node->child[1], NULL);
    }
#endif
}

void dock_ui_render(struct DockManager *mgr, struct nk_context *ctx) {
    if (!mgr || !mgr->root || !ctx) return;
    dock_ui_draw_node(mgr, mgr->root, ctx);

#ifdef DOCK_UI_USE_NUKLEAR
    /* ドラッグ中の処理: マウス解放でドロップ判定 */
    if (mgr->dragging) {
        const struct nk_input *in = &ctx->input;
        float mx = (float)in->mouse.pos.x;
        float my = (float)in->mouse.pos.y;
        /* リアルタイム候補更新 */
        DockDropCandidate c = dock_manager_get_drop_candidate(mgr, mx, my);
        mgr->last_drop_node = c.node;
        mgr->last_drop_target = c.target;
        mgr->last_drop_tab_index = c.tab_index;

        /* 同一ノード内の並び替えをリアルタイムで行う（プレビュー代わり） */
        if (mgr->drag_node && c.node == mgr->drag_node && c.target == DOCK_TARGET_TAB) {
            int dst = c.tab_index;
            if (dst < 0) dst = mgr->drag_node->tabs_count - 1;
            if (dst != mgr->drag_index) {
                dock_tab_move(mgr->drag_node, mgr->drag_index, dst);
                mgr->drag_index = dst;
            }
        }

        /* 描画プレビュー: ハイライト矩形とゴーストタブ */
        if (mgr->last_drop_node) {
            struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
            struct nk_rect r = mgr->last_drop_node->rect;
            struct nk_rect highlight = r;
            if (mgr->last_drop_target == DOCK_TARGET_CENTER) {
                /* 中央: ノード中心部をハイライト */
                float padw = r.w * 0.1f; float padh = r.h * 0.1f;
                highlight.x += padw; highlight.y += padh;
                highlight.w -= padw * 2.0f; highlight.h -= padh * 2.0f;
            } else if (mgr->last_drop_target == DOCK_TARGET_TAB) {
                /* タブ領域をハイライト（上部固定高さ） */
                highlight.h = 22.0f; highlight.y = r.y; highlight.x = r.x; highlight.w = r.w;
                /* 追加位置が特定できれば縦ラインで指示 */
                if (mgr->last_drop_tab_index >= 0 && mgr->last_drop_node->tabs_count > 0) {
                    int tc = mgr->last_drop_node->tabs_count;
                    float tw = r.w / (float)(tc > 0 ? tc : 1);
                    float ix = r.x + tw * (float)mgr->last_drop_tab_index;
                    struct nk_rect line = { ix - 2.0f, r.y, 4.0f, 22.0f };
                    nk_fill_rect(canvas, line, 0.0f, nk_rgba(0,200,255,200));
                }
            } else if (mgr->last_drop_target == DOCK_TARGET_LEFT) {
                highlight.w = r.w * 0.35f; highlight.h = r.h; highlight.x = r.x; highlight.y = r.y;
            } else if (mgr->last_drop_target == DOCK_TARGET_RIGHT) {
                highlight.w = r.w * 0.35f; highlight.h = r.h; highlight.x = r.x + r.w * 0.65f; highlight.y = r.y;
            } else if (mgr->last_drop_target == DOCK_TARGET_TOP) {
                highlight.h = r.h * 0.35f; highlight.w = r.w; highlight.x = r.x; highlight.y = r.y;
            } else if (mgr->last_drop_target == DOCK_TARGET_BOTTOM) {
                highlight.h = r.h * 0.35f; highlight.w = r.w; highlight.x = r.x; highlight.y = r.y + r.h * 0.65f;
            }
            nk_fill_rect(canvas, highlight, 4.0f, nk_rgba(0,128,255,96));
        }

        /* ゴーストタブ（マウスポインタ付近） */
        if (mgr->drag_window) {
            struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
            struct nk_rect g = { mx + 8.0f, my + 8.0f, 140.0f, 22.0f };
            nk_fill_rect(canvas, g, 4.0f, nk_rgba(50,50,50,200));
            /* テキストは簡易的に nk_label を利用（小さなウィンドウ内に表示） */
            /* 直接テキストを描くために一時ウィンドウを用いる */
            if (nk_begin(ctx, "#ghost", nk_rect(g.x, g.y, g.w, g.h), NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT)) {
                nk_layout_row_dynamic(ctx, g.h, 1);
                nk_label(ctx, mgr->drag_window->title, NK_TEXT_LEFT);
            }
            nk_end(ctx);
        }

        if (nk_input_is_mouse_released(&ctx->input, NK_BUTTON_LEFT)) {
            /* ドロップ確定: 異なるノードへ移動 */
            if (mgr->last_drop_node && mgr->drag_node && mgr->drag_window) {
                if (mgr->last_drop_node != mgr->drag_node) {
                    if (mgr->last_drop_target == DOCK_TARGET_TAB || mgr->last_drop_target == DOCK_TARGET_CENTER) {
                        dock_manager_move_tab(mgr, mgr->drag_node, mgr->drag_index, mgr->last_drop_node, mgr->last_drop_tab_index);
                    } else {
                        /* エッジドロップ: 新しいリーフを作成して split を行う */
                        DockWindow *w = mgr->drag_window;
                        DockNode *new_leaf = dock_node_create_leaf(w);
                        if (new_leaf) {
                            /* ドロップ先ノードを分割して new_leaf を追加 */
                            DockSplitType st = (mgr->last_drop_target == DOCK_TARGET_LEFT || mgr->last_drop_target == DOCK_TARGET_RIGHT) ? DOCK_SPLIT_HORZ : DOCK_SPLIT_VERT;
                            if (mgr->last_drop_target == DOCK_TARGET_LEFT || mgr->last_drop_target == DOCK_TARGET_TOP) {
                                dock_manager_split_node(mgr, mgr->last_drop_node, st, 0.5f, new_leaf);
                            } else {
                                dock_manager_split_node(mgr, mgr->last_drop_node, st, 0.5f, new_leaf);
                            }
                        }
                    }
                }
            }
            mgr->dragging = 0;
            mgr->drag_node = NULL;
            mgr->drag_index = -1;
            mgr->drag_window = NULL;
            mgr->last_drop_node = NULL;
            mgr->last_drop_target = DOCK_TARGET_NONE;
            mgr->last_drop_tab_index = -1;
        }
    }
#endif
}
