#ifndef NUKLEAR_DOCK_MANAGER_H
#define NUKLEAR_DOCK_MANAGER_H

#include "dock.h"
/* ドラッグ＆ドロップ判定: ドロップターゲット種別 */
typedef enum {
    DOCK_TARGET_NONE,
    DOCK_TARGET_CENTER,
    DOCK_TARGET_TOP,
    DOCK_TARGET_LEFT,
    DOCK_TARGET_RIGHT,
    DOCK_TARGET_BOTTOM,
    DOCK_TARGET_TAB
} DockDropTarget;

/* ドロップ候補情報 */
typedef struct DockDropCandidate {
    DockNode *node;        /* 対象ノード（NULL なら候補なし） */
    DockDropTarget target; /* ターゲット種別 */
    int tab_index;         /* タブ挿入先インデックス（TAB 時有効、-1 は末尾） */
} DockDropCandidate;

/* ドッキングマネージャ構造体 */
typedef struct DockManager {
    DockNode *root;           /* ドッキングツリーのルートノード */
    int next_node_id;         /* ノードID発行用 */
    int next_window_id;       /* ウィンドウID発行用 */
    int window_count;         /* 登録ウィンドウ数 */
    DockWindow **windows;     /* 全ウィンドウ配列（動的） */
    int windows_capacity;
    /* ドラッグ状態（UI 実装が管理するための補助） */
    int dragging;             /* 0: 非ドラッグ, 1: ドラッグ中 */
    DockNode *drag_node;      /* ドラッグ元ノード */
    int drag_index;           /* ドラッグ元インデックス */
    DockWindow *drag_window;  /* ドラッグ中のウィンドウポインタ */
    /* 最後に検出したドロップ候補（UI 表示/プレビュー用） */
    DockNode *last_drop_node;
    DockDropTarget last_drop_target;
    int last_drop_tab_index;
} DockManager;

/* マネージャ生成・破棄 */
DockManager* dock_manager_create(void);
void dock_manager_free(DockManager *mgr);

/* ウィンドウ登録・削除 */
DockWindow* dock_manager_add_window(DockManager *mgr, const char *name, const char *title);
int dock_manager_remove_window(DockManager *mgr, int window_id);
DockWindow* dock_manager_find_window(DockManager *mgr, int window_id);

/* ツリー操作API */
DockNode* dock_manager_split_node(DockManager *mgr, DockNode *node, DockSplitType split_type, float ratio, DockNode *new_node);
DockNode* dock_manager_tabify_node(DockManager *mgr, DockNode *node, DockWindow *win);
int dock_manager_remove_node(DockManager *mgr, DockNode *node);

/* 指定座標に対するドロップ候補を返す（mgr が NULL なら DOCK_TARGET_NONE を返す） */
DockDropCandidate dock_manager_get_drop_candidate(DockManager *mgr, float x, float y);

/* ルートノード設定 */
void dock_manager_set_root(DockManager *mgr, DockNode *root);

/* レイアウト計算: 指定矩形に対してノード矩形を再帰的に計算する */
void dock_manager_compute_layout(DockManager *mgr, struct nk_rect area);

#endif /* NUKLEAR_DOCK_MANAGER_H */
