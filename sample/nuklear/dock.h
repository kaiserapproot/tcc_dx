#ifndef NUKLEAR_DOCK_H
#define NUKLEAR_DOCK_H

#include <stdlib.h>
#include <stdint.h>

/* Nuklear の固定幅型定義を有効にする（nk_size 等の typedef を安定させる） */
#ifndef NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_FIXED_TYPES
#endif

#include "nuklear.h"

typedef enum {
    DOCK_NODE_SPLIT,
    DOCK_NODE_TAB,
    DOCK_NODE_LEAF
} DockNodeType;

typedef enum {
    DOCK_SPLIT_HORZ,
    DOCK_SPLIT_VERT
} DockSplitType;

typedef struct DockWindow {
    int id;                    /* 一意なウィンドウID */
    char name[64];             /* Nuklear識別子（nk_begin_titled の name） */
    char title[64];            /* 表示タイトル（ui上に表示する文字列） */
    struct nk_rect float_rect; /* フローティング時の矩形（位置・サイズ） */
    struct nk_rect last_bounds;/* 最後に適用した矩形（レイアウト同期用） */
    int is_floating;           /* フローティング状態か */
    int visible;               /* 表示フラグ */
    int closable;              /* クローズ可能フラグ */
    void *user_data;           /* 任意のペイロード */
} DockWindow;

typedef struct DockNode {
    int id;                   /* ノードID（デバッグ／シリアライズ用） */
    DockNodeType type;        /* ノード種別（分割/タブ/リーフ） */
    struct DockNode *parent;  /* 親ノードへのポインタ */

    /* 分割ノード用 */
    DockSplitType split_type; /* 分割方向（横/縦） */
    float split_ratio;        /* 分割比率（0.0 - 1.0） */
    struct DockNode *child[2];

    /* タブノード用 */
    DockWindow **tabs;        /* 動的配列（DockWindow*） */
    int tabs_count;           /* 要素数 */
    int tabs_capacity;        /* 割当容量 */
    int active_tab;           /* アクティブタブのインデックス */

    /* リーフ用 */
    DockWindow *win;          /* リーフが保有するウィンドウ */

    /* 共通 */
    struct nk_rect rect;      /* ノードが占める画面矩形（計算結果） */
    int min_w, min_h;         /* 最小サイズ制約 */
    unsigned int flags;       /* 拡張フラグ */
} DockNode;

/* DockWindow 管理関数 */
DockWindow* dock_window_create(int id, const char *name, const char *title);
void dock_window_free(DockWindow *w);

/* DockNode 管理関数 */
DockNode* dock_node_create_tab(void);
DockNode* dock_node_create_leaf(DockWindow *win);
DockNode* dock_node_create_split(DockSplitType type, float ratio, DockNode *a, DockNode *b);
void dock_node_free(DockNode *node); /* ノードツリーを解放（DockWindowポインタは解放しない） */
/* ノード本体のみを解放する（子ノードや持たれる DockWindow は解放しない） */
void dock_node_free_shallow(DockNode *node);

/* タブ配列操作（DOCK_NODE_TAB 向け） */
int dock_tab_add(DockNode *tab_node, DockWindow *win); /* 成功時は追加インデックス、失敗で-1 */
int dock_tab_remove(DockNode *tab_node, int index); /* 成功で0、失敗で-1 */
int dock_tab_find_index(DockNode *tab_node, int window_id); /* 見つからなければ-1 */

/* タブ挿入: 指定インデックスに挿入する（index が負なら末尾） */
int dock_tab_insert(DockNode *tab_node, int index, DockWindow *win);

/* タブ内での要素移動（同一ノード内）: src_index の要素を dst_index に移動する。
 * dst_index が負なら末尾扱い。成功で0、失敗で-1。
 */
int dock_tab_move(DockNode *tab_node, int src_index, int dst_index);

/* タブ移動 API: src_node の src_index のタブを dst_node の dst_index に移動する
 * index が負なら末尾。成功で0、失敗で-1。
 */
int dock_manager_move_tab(struct DockManager *mgr, DockNode *src_node, int src_index, DockNode *dst_node, int dst_index);

/* レイアウト計算: 指定領域に対してツリー全体の矩形を計算して各ノードの rect を設定する */
void dock_manager_compute_layout(struct DockManager *mgr, struct nk_rect area);

/* シンプルなタブUIレンダラ: Nuklear のコンテキスト内で呼び出すことで
 * 現在のタブ群を水平にボタンとして描画し、ボタン押下でアクティブ切替する。
 * 注意: 描画位置は呼び出し側のレイアウトに依存します。より厳密な位置制御は
 * ノードの rect を用いてユーザが適切なスペースを確保してから呼んでください。
 */
void dock_ui_render(struct DockManager *mgr, struct nk_context *ctx);

/* 任意ノードに対して矩形を適用してレイアウト計算を行う（テスト/部分レイアウト用） */
void dock_node_compute_layout(DockNode *node, struct nk_rect area);

#endif /* NUKLEAR_DOCK_H */
