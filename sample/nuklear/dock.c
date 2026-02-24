#include "dock.h"
#include <string.h>
#include <stdio.h>

/* 補助: ゼロ初期化されたメモリを確保 */
static void *xcalloc(size_t n, size_t sz) {
    void *p = calloc(n, sz);
    return p;
}

/* ---------------- DockWindow ---------------- */
/* 新しい DockWindow を作成して初期化して返す */
DockWindow* dock_window_create(int id, const char *name, const char *title) {
    DockWindow *w = (DockWindow*)xcalloc(1, sizeof(DockWindow));
    if (!w) return NULL;
    w->id = id;
    if (name) strncpy(w->name, name, sizeof(w->name)-1);
    if (title) strncpy(w->title, title, sizeof(w->title)-1);
    w->is_floating = 0;
    w->visible = 1;
    w->closable = 1;
    w->float_rect.x = w->float_rect.y = w->float_rect.w = w->float_rect.h = 0.0f;
    w->last_bounds.x = w->last_bounds.y = w->last_bounds.w = w->last_bounds.h = 0.0f;
    w->user_data = NULL;
    return w;
}

/* DockWindow を破棄する（user_data や外部のウィンドウは呼び出し側で管理） */
void dock_window_free(DockWindow *w) {
    if (!w) return;
    free(w);
}

/* ---------------- DockNode ---------------- */
/* 内部: DockNode の割当と初期値設定 */
static DockNode* dock_node_alloc(void) {
    DockNode *n = (DockNode*)xcalloc(1, sizeof(DockNode));
    if (n) {
        n->split_ratio = 0.5f;
        n->min_w = 32;
        n->min_h = 32;
        n->active_tab = -1;
    }
    return n;
}

/* タブノードを作成 */
DockNode* dock_node_create_tab(void) {
    DockNode *n = dock_node_alloc();
    if (!n) return NULL;
    n->type = DOCK_NODE_TAB;
    n->tabs = NULL;
    n->tabs_count = 0;
    n->tabs_capacity = 0;
    n->active_tab = -1;
    return n;
}

/* リーフノードを作成し、DockWindow を紐付ける */
DockNode* dock_node_create_leaf(DockWindow *win) {
    DockNode *n = dock_node_alloc();
    if (!n) return NULL;
    n->type = DOCK_NODE_LEAF;
    n->win = win;
    return n;
}

/* 分割ノードを作成し、子ノードを接続する */
DockNode* dock_node_create_split(DockSplitType type, float ratio, DockNode *a, DockNode *b) {
    DockNode *n = dock_node_alloc();
    if (!n) return NULL;
    n->type = DOCK_NODE_SPLIT;
    n->split_type = type;
    n->split_ratio = ratio;
    n->child[0] = a;
    n->child[1] = b;
    if (a) a->parent = n;
    if (b) b->parent = n;
    return n;
}

/* ノードツリーを再帰的に解放する。
 * 注意: DockWindow ポインタ自体はここでは解放しない（呼び出し側の責任）。
 */
void dock_node_free(DockNode *node) {
    if (!node) return;
    if (node->type == DOCK_NODE_SPLIT) {
        dock_node_free(node->child[0]);
        dock_node_free(node->child[1]);
    } else if (node->type == DOCK_NODE_TAB) {
        if (node->tabs) {
            /* DockWindow は所有しないため pointer 配列のみ解放 */
            free(node->tabs);
            node->tabs = NULL;
        }
    }
    free(node);
}

/* ノード本体のみを解放する（子ノードや DockWindow ポインタは解放しない） */
void dock_node_free_shallow(DockNode *node) {
    if (!node) return;
    /* タブ配列はノード本体が保持しているため解放しておく */
    if (node->type == DOCK_NODE_TAB) {
        if (node->tabs) {
            free(node->tabs);
            node->tabs = NULL;
            node->tabs_count = 0;
            node->tabs_capacity = 0;
            node->active_tab = -1;
        }
    }
    /* 子は解放しない */
    free(node);
}

/* ---------------- Tab operations ---------------- */
/* タブを追加する。成功で追加インデックス、失敗で -1 を返す */
int dock_tab_add(DockNode *tab_node, DockWindow *win) {
    if (!tab_node || tab_node->type != DOCK_NODE_TAB || !win) return -1;
    if (tab_node->tabs_count + 1 > tab_node->tabs_capacity) {
        int newcap = tab_node->tabs_capacity ? tab_node->tabs_capacity * 2 : 4;
        DockWindow **n = (DockWindow**)realloc(tab_node->tabs, newcap * sizeof(DockWindow*));
        if (!n) return -1;
        tab_node->tabs = n;
        tab_node->tabs_capacity = newcap;
    }
    tab_node->tabs[tab_node->tabs_count] = win;
    int idx = tab_node->tabs_count;
    tab_node->tabs_count++;
    if (tab_node->active_tab < 0) tab_node->active_tab = 0;
    return idx;
}

/* 指定インデックスのタブを削除する。成功で0、失敗で-1 */
int dock_tab_remove(DockNode *tab_node, int index) {
    if (!tab_node || tab_node->type != DOCK_NODE_TAB) return -1;
    if (index < 0 || index >= tab_node->tabs_count) return -1;
    /* 要素を詰める */
    for (int i = index; i + 1 < tab_node->tabs_count; ++i) tab_node->tabs[i] = tab_node->tabs[i+1];
    tab_node->tabs_count--;
    if (tab_node->tabs_count == 0) {
        free(tab_node->tabs);
        tab_node->tabs = NULL;
        tab_node->tabs_capacity = 0;
        tab_node->active_tab = -1;
    } else {
        if (tab_node->active_tab >= tab_node->tabs_count) tab_node->active_tab = tab_node->tabs_count - 1;
    }
    return 0;
}

/* window_id を持つタブのインデックスを探す。見つからなければ -1 */
int dock_tab_find_index(DockNode *tab_node, int window_id) {
    if (!tab_node || tab_node->type != DOCK_NODE_TAB) return -1;
    for (int i = 0; i < tab_node->tabs_count; ++i) {
        if (tab_node->tabs[i] && tab_node->tabs[i]->id == window_id) return i;
    }
    return -1;
}

/* タブ挿入: index が負なら末尾に追加する */
int dock_tab_insert(DockNode *tab_node, int index, DockWindow *win) {
    if (!tab_node || tab_node->type != DOCK_NODE_TAB || !win) return -1;
    if (index < 0) index = tab_node->tabs_count;
    if (index > tab_node->tabs_count) index = tab_node->tabs_count;
    if (tab_node->tabs_count + 1 > tab_node->tabs_capacity) {
        int newcap = tab_node->tabs_capacity ? tab_node->tabs_capacity * 2 : 4;
        DockWindow **n = (DockWindow**)realloc(tab_node->tabs, newcap * sizeof(DockWindow*));
        if (!n) return -1;
        tab_node->tabs = n;
        tab_node->tabs_capacity = newcap;
    }
    /* 挿入位置以降をシフト */
    for (int i = tab_node->tabs_count; i > index; --i) tab_node->tabs[i] = tab_node->tabs[i-1];
    tab_node->tabs[index] = win;
    tab_node->tabs_count++;
    if (tab_node->active_tab < 0) tab_node->active_tab = 0;
    return index;
}

/* タブ内での要素移動（同一ノード内） */
int dock_tab_move(DockNode *tab_node, int src_index, int dst_index) {
    if (!tab_node || tab_node->type != DOCK_NODE_TAB) return -1;
    int n = tab_node->tabs_count;
    if (src_index < 0 || src_index >= n) return -1;
    if (dst_index < 0) dst_index = n - 1;
    if (dst_index >= n) dst_index = n - 1;
    if (src_index == dst_index) return 0;
    DockWindow *w = tab_node->tabs[src_index];
    if (src_index < dst_index) {
        for (int i = src_index; i < dst_index; ++i) tab_node->tabs[i] = tab_node->tabs[i+1];
        tab_node->tabs[dst_index] = w;
    } else {
        for (int i = src_index; i > dst_index; --i) tab_node->tabs[i] = tab_node->tabs[i-1];
        tab_node->tabs[dst_index] = w;
    }
    if (tab_node->active_tab == src_index) tab_node->active_tab = dst_index;
    else if (tab_node->active_tab > src_index && tab_node->active_tab <= dst_index) tab_node->active_tab--;
    else if (tab_node->active_tab < src_index && tab_node->active_tab >= dst_index) tab_node->active_tab++;
    return 0;
}
