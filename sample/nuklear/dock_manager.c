#include "dock_manager.h"
#include <stdlib.h>
#include <string.h>

/* 内部ヘルパ: parent の child ポインタを old -> nw に置き換える */
static void replace_child(DockNode *parent, DockNode *old, DockNode *nw) {
    if (!parent) return;
    if (parent->child[0] == old) parent->child[0] = nw;
    else if (parent->child[1] == old) parent->child[1] = nw;
}

/* 内部ヘルパ: ツリー内で window_id を参照するノード（leaf または tab）を探して削除処理を行う。
 * 見つけて削除した場合は 1 を返す。複数箇所に参照があればすべて削除する。
 */
static int node_remove_window_refs_recursive(DockManager *mgr, DockNode *node, int window_id) {
    if (!node) return 0;
    int removed = 0;
    if (node->type == DOCK_NODE_LEAF) {
        if (node->win && node->win->id == window_id) {
            /* リーフを丸ごと削除 */
            dock_manager_remove_node(mgr, node);
            return 1;
        }
    } else if (node->type == DOCK_NODE_TAB) {
        int idx = dock_tab_find_index(node, window_id);
        if (idx >= 0) {
            dock_tab_remove(node, idx);
            removed = 1;
            /* タブが空になったらノード自体を削除 */
            if (node->tabs_count == 0) {
                dock_manager_remove_node(mgr, node);
            }
        }
    } else if (node->type == DOCK_NODE_SPLIT) {
        if (node->child[0]) removed += node_remove_window_refs_recursive(mgr, node->child[0], window_id);
        if (node->child[1]) removed += node_remove_window_refs_recursive(mgr, node->child[1], window_id);
    }
    return removed;
}

/* ドッキングマネージャ生成 */
DockManager* dock_manager_create(void) {
    DockManager *mgr = (DockManager*)calloc(1, sizeof(DockManager));
    if (!mgr) return NULL;
    mgr->root = NULL;
    mgr->next_node_id = 1;
    mgr->next_window_id = 1;
    mgr->window_count = 0;
    mgr->windows = NULL;
    mgr->windows_capacity = 0;
    return mgr;
}

/* ドッキングマネージャ破棄 */
void dock_manager_free(DockManager *mgr) {
    if (!mgr) return;
    if (mgr->root) dock_node_free(mgr->root);
    if (mgr->windows) {
        for (int i = 0; i < mgr->window_count; ++i) {
            dock_window_free(mgr->windows[i]);
        }
        free(mgr->windows);
    }
    free(mgr);
}

/* ウィンドウ追加 */
DockWindow* dock_manager_add_window(DockManager *mgr, const char *name, const char *title) {
    if (!mgr) return NULL;
    if (mgr->window_count + 1 > mgr->windows_capacity) {
        int newcap = mgr->windows_capacity ? mgr->windows_capacity * 2 : 8;
        DockWindow **n = (DockWindow**)realloc(mgr->windows, newcap * sizeof(DockWindow*));
        if (!n) return NULL;
        mgr->windows = n;
        mgr->windows_capacity = newcap;
    }
    int id = mgr->next_window_id++;
    DockWindow *w = dock_window_create(id, name, title);
    mgr->windows[mgr->window_count++] = w;
    return w;
}

/* ウィンドウ削除 */
int dock_manager_remove_window(DockManager *mgr, int window_id) {
    if (!mgr || !mgr->windows) return -1;
    /* まずツリー中の参照を除去 */
    if (mgr->root) node_remove_window_refs_recursive(mgr, mgr->root, window_id);

    /* 次に管理配列から削除してメモリ解放 */
    for (int i = 0; i < mgr->window_count; ++i) {
        if (mgr->windows[i]->id == window_id) {
            dock_window_free(mgr->windows[i]);
            for (int j = i; j + 1 < mgr->window_count; ++j) mgr->windows[j] = mgr->windows[j+1];
            mgr->window_count--;
            return 0;
        }
    }
    return -1;
}

/* ウィンドウ検索 */
DockWindow* dock_manager_find_window(DockManager *mgr, int window_id) {
    if (!mgr || !mgr->windows) return NULL;
    for (int i = 0; i < mgr->window_count; ++i) {
        if (mgr->windows[i]->id == window_id) return mgr->windows[i];
    }
    return NULL;
}

/* ノード分割: nodeをsplit_typeで分割し、new_nodeを追加 */
DockNode* dock_manager_split_node(DockManager *mgr, DockNode *node, DockSplitType split_type, float ratio, DockNode *new_node) {
    if (!mgr || !node || !new_node) return NULL;
    DockNode *split = dock_node_create_split(split_type, ratio, NULL, NULL);
    split->id = mgr->next_node_id++;
    if (split_type == DOCK_SPLIT_HORZ || split_type == DOCK_SPLIT_VERT) {
        /* 元の親を保持してから子を再配列する（node->parent を上書くため） */
        DockNode *orig_parent = node->parent;
        split->child[0] = node;
        split->child[1] = new_node;
        node->parent = split;
        new_node->parent = split;

        /* 親ノードの置き換え（orig_parent を使う） */
        if (orig_parent) {
            if (orig_parent->child[0] == node) orig_parent->child[0] = split;
            else if (orig_parent->child[1] == node) orig_parent->child[1] = split;
            split->parent = orig_parent;
        } else {
            mgr->root = split;
            split->parent = NULL;
        }
    }
    return split;
}

/* ノードをタブ化: nodeにwinを追加 */
DockNode* dock_manager_tabify_node(DockManager *mgr, DockNode *node, DockWindow *win) {
    if (!mgr || !node || !win) return NULL;
    /* 既にタブノードなら単純追加（重複チェックあり） */
    if (node->type == DOCK_NODE_TAB) {
        if (dock_tab_find_index(node, win->id) >= 0) return node; /* 重複追加を無視 */
        dock_tab_add(node, win);
        return node;
    }

    /* リーフノードのみタブ化をサポートする（split ノードを直接タブ化するのは未対応） */
    if (node->type != DOCK_NODE_LEAF) return NULL;

    DockNode *tab = dock_node_create_tab();
    if (!tab) return NULL;
    tab->id = mgr->next_node_id++;
    /* 既存のリーフに紐付くウィンドウを先に追加 */
    if (node->win) dock_tab_add(tab, node->win);
    dock_tab_add(tab, win);

    /* 親ノードの置き換え */
    if (node->parent) {
        DockNode *parent = node->parent;
        replace_child(parent, node, tab);
        tab->parent = parent;
    } else {
        mgr->root = tab;
        tab->parent = NULL;
    }

    /* 古いリーフノード本体を浅く解放（DockWindow は解放しない） */
    dock_node_free_shallow(node);
    return tab;
}

/* ノード削除（ツリーから除去、メモリ解放） */
int dock_manager_remove_node(DockManager *mgr, DockNode *node) {
    if (!mgr || !node) return -1;

    DockNode *parent = node->parent;

    /* ルートノードを削除する場合 */
    if (!parent) {
        /* node を再帰的に解放 */
        if (mgr->root == node) mgr->root = NULL;
        dock_node_free(node);
        return 0;
    }

    /* 親が split の場合、兄弟を持ち上げて親を取り除く（ツリーの縮約） */
    if (parent->type == DOCK_NODE_SPLIT) {
        DockNode *sibling = (parent->child[0] == node) ? parent->child[1] : parent->child[0];
        DockNode *grand = parent->parent;

        if (grand) {
            /* 親の親の参照を sibling に差し替える */
            replace_child(grand, parent, sibling);
            if (sibling) sibling->parent = grand;
        } else {
            /* parent が root の場合は sibling を root にする */
            mgr->root = sibling;
            if (sibling) sibling->parent = NULL;
        }

        /* parent 構造体のみ解放（子は保持されているため浅い解放） */
        dock_node_free_shallow(parent);

        /* node を再帰的に解放 */
        dock_node_free(node);
        return 0;
    }

    /* 親がタブノードやその他の場合は、単純に親の child を NULL にして node を解放 */
    if (parent->child[0] == node) parent->child[0] = NULL;
    else if (parent->child[1] == node) parent->child[1] = NULL;
    dock_node_free(node);
    return 0;
}

/* ルートノード設定 */
void dock_manager_set_root(DockManager *mgr, DockNode *root) {
    if (!mgr) return;
    mgr->root = root;
    if (root) root->parent = NULL;
}

/* 深さ優先で座標が含まれる最下位ノードを探す（子があれば優先して掘る） */
static DockNode* find_deepest_node_at(DockNode *node, float x, float y) {
    if (!node) return NULL;
    struct nk_rect r = node->rect;
    if (x < r.x || x > r.x + r.w || y < r.y || y > r.y + r.h) return NULL;
    if (node->type == DOCK_NODE_SPLIT) {
        DockNode *c0 = find_deepest_node_at(node->child[0], x, y);
        if (c0) return c0;
        DockNode *c1 = find_deepest_node_at(node->child[1], x, y);
        if (c1) return c1;
        /* 子に当たらない場合は親ノードを返す */
        return node;
    }
    return node;
}

/* タブノード内のタブインデックスを座標から推定する。見つからなければ -1 を返す。
 * 単純化のため均等幅で分割して計算する。
 */
static int tab_index_from_x(DockNode *tab_node, float x) {
    if (!tab_node || tab_node->type != DOCK_NODE_TAB || tab_node->tabs_count <= 0) return -1;
    struct nk_rect r = tab_node->rect;
    float relx = x - r.x;
    if (relx < 0) return 0;
    if (relx > r.w) return tab_node->tabs_count - 1;
    float w = r.w / (float)tab_node->tabs_count;
    int idx = (int)(relx / w);
    if (idx < 0) idx = 0;
    if (idx >= tab_node->tabs_count) idx = tab_node->tabs_count - 1;
    return idx;
}

/* ドロップ候補判定の実装 */
DockDropCandidate dock_manager_get_drop_candidate(DockManager *mgr, float x, float y) {
    DockDropCandidate c;
    c.node = NULL; c.target = DOCK_TARGET_NONE; c.tab_index = -1;
    if (!mgr || !mgr->root) return c;

    DockNode *node = find_deepest_node_at(mgr->root, x, y);
    if (!node) return c;

    /* タブバー領域の判定: ノード矩形の上端から高さの小領域をタブ領域とする */
    struct nk_rect r = node->rect;
    float tabbar_h = (r.h * 0.12f > 16.0f) ? (r.h * 0.12f) : 16.0f; /* 最低 16px または高さの12% */
    if (node->type == DOCK_NODE_TAB && y >= r.y && y <= r.y + tabbar_h) {
        c.node = node;
        c.target = DOCK_TARGET_TAB;
        c.tab_index = tab_index_from_x(node, x);
        return c;
    }

    /* エッジ／中央判定: 相対座標比で判定 */
    float nx = (x - r.x) / r.w;
    float ny = (y - r.y) / r.h;
    const float T = 0.25f; /* エッジ閾値 */
    if (nx < T) {
        c.node = node; c.target = DOCK_TARGET_LEFT; return c;
    }
    if (nx > 1.0f - T) {
        c.node = node; c.target = DOCK_TARGET_RIGHT; return c;
    }
    if (ny < T) {
        c.node = node; c.target = DOCK_TARGET_TOP; return c;
    }
    if (ny > 1.0f - T) {
        c.node = node; c.target = DOCK_TARGET_BOTTOM; return c;
    }
    c.node = node; c.target = DOCK_TARGET_CENTER; return c;
}

/* ノード矩形を再帰的に計算するヘルパ */
static void dock_node_calc_layout(DockNode *node, struct nk_rect rect) {
    const unsigned int LAYOUT_VISIT_FLAG = 0x80000000u;
    if (!node) return;
    if (node->flags & LAYOUT_VISIT_FLAG) {
        return; /* サイクル検出: 静かに抜ける */
    }
    node->flags &= ~LAYOUT_VISIT_FLAG; /* clear just in case */
    node->flags |= LAYOUT_VISIT_FLAG;
    node->rect = rect;
    if (node->type == DOCK_NODE_SPLIT) {
        /* 水平方向分割 = 左/右, 垂直分割 = 上/下 */
        if (node->split_type == DOCK_SPLIT_HORZ) {
            float w0 = rect.w * node->split_ratio;
            struct nk_rect r0 = { rect.x, rect.y, w0, rect.h };
            struct nk_rect r1 = { rect.x + w0, rect.y, rect.w - w0, rect.h };
            if (node->child[0]) dock_node_calc_layout(node->child[0], r0);
            if (node->child[1]) dock_node_calc_layout(node->child[1], r1);
        } else {
            float h0 = rect.h * node->split_ratio;
            struct nk_rect r0 = { rect.x, rect.y, rect.w, h0 };
            struct nk_rect r1 = { rect.x, rect.y + h0, rect.w, rect.h - h0 };
            if (node->child[0]) dock_node_calc_layout(node->child[0], r0);
            if (node->child[1]) dock_node_calc_layout(node->child[1], r1);
        }
    } else if (node->type == DOCK_NODE_TAB) {
        /* タブノードは自身の矩形を持つだけ。内容描画領域は下部を使う想定 */
        (void)node; /* 何もしない（rect は既に設定済み） */
    } else {
        /* リーフは自身の矩形を持つ */
    }
    /* clear visit mark */
    node->flags &= ~LAYOUT_VISIT_FLAG;
}

/* マネージャ向け公開 API: ルートに対して矩形を適用して再帰的に計算する */
void dock_manager_compute_layout(DockManager *mgr, struct nk_rect area) {
    if (!mgr) return;
    if (!mgr->root) return;
    dock_node_calc_layout(mgr->root, area);
}

/* 任意ノードに対するレイアウト計算（公開） */
void dock_node_compute_layout(DockNode *node, struct nk_rect area) {
    if (!node) return;
    dock_node_calc_layout(node, area);
}

/* タブ移動: src_node の src_index を dst_node の dst_index に移動する */
int dock_manager_move_tab(DockManager *mgr, DockNode *src_node, int src_index, DockNode *dst_node, int dst_index) {
    if (!mgr || !src_node || !dst_node) return -1;
    DockWindow *w = NULL;
    if (src_node->type == DOCK_NODE_TAB) {
        if (src_index < 0 || src_index >= src_node->tabs_count) return -1;
        w = src_node->tabs[src_index];
    } else if (src_node->type == DOCK_NODE_LEAF) {
        if (src_index != 0) return -1;
        w = src_node->win;
        if (!w) return -1;
    } else return -1;

    /* 挿入先 */
    if (dst_node->type == DOCK_NODE_TAB) {
        if (dock_tab_insert(dst_node, dst_index, w) < 0) return -1;
    } else if (dst_node->type == DOCK_NODE_LEAF) {
        /* リーフをタブ化して追加 */
        DockNode *tab = dock_manager_tabify_node(mgr, dst_node, w);
        if (!tab) return -1;
        dst_node = tab;
    } else {
        return -1;
    }

    /* 元から削除 */
    if (src_node->type == DOCK_NODE_TAB) {
        dock_tab_remove(src_node, src_index);
        if (src_node->tabs_count == 0) {
            dock_manager_remove_node(mgr, src_node);
        }
    } else if (src_node->type == DOCK_NODE_LEAF) {
        /* リーフから移動した場合はリーフを削除 */
        src_node->win = NULL;
        dock_manager_remove_node(mgr, src_node);
    }

    return 0;
}
