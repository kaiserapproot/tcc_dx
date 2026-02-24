#include <stdio.h>
#include <stdlib.h>
#include "Basic.h"
#include "dock_manager.h"

static void test_add_and_tabify(void) {
    DockManager *mgr = dock_manager_create();
    CU_ASSERT_PTR_NOT_NULL(mgr);

    DockWindow *w1 = dock_manager_add_window(mgr, "win1", "Win1");
    CU_ASSERT_PTR_NOT_NULL(w1);

    /* リーフを作成してルートに設定 */
    DockNode *leaf = dock_node_create_leaf(w1);
    CU_ASSERT_PTR_NOT_NULL(leaf);
    leaf->id = mgr->next_node_id++;
    dock_manager_set_root(mgr, leaf);

    DockWindow *w2 = dock_manager_add_window(mgr, "win2", "Win2");
    CU_ASSERT_PTR_NOT_NULL(w2);

    DockNode *tab = dock_manager_tabify_node(mgr, leaf, w2);
    CU_ASSERT_PTR_NOT_NULL(tab);
    CU_ASSERT_EQUAL(tab->type, DOCK_NODE_TAB);
    CU_ASSERT_EQUAL(tab->tabs_count, 2);
    CU_ASSERT_EQUAL(dock_tab_find_index(tab, w1->id), 0);
    CU_ASSERT_EQUAL(dock_tab_find_index(tab, w2->id), 1);

    dock_manager_free(mgr);
}

static void test_remove_window_refs(void) {
    DockManager *mgr = dock_manager_create();
    CU_ASSERT_PTR_NOT_NULL(mgr);

    DockWindow *w1 = dock_manager_add_window(mgr, "a", "A");
    DockWindow *w2 = dock_manager_add_window(mgr, "b", "B");
    DockNode *leaf = dock_node_create_leaf(w1);
    leaf->id = mgr->next_node_id++;
    dock_manager_set_root(mgr, leaf);
    DockNode *tab = dock_manager_tabify_node(mgr, leaf, w2);
    CU_ASSERT_PTR_NOT_NULL(tab);
    CU_ASSERT_EQUAL(tab->tabs_count, 2);

    /* w1 を削除するとタブから除かれるはず */
    int r = dock_manager_remove_window(mgr, w1->id);
    CU_ASSERT_EQUAL(r, 0);
    CU_ASSERT_EQUAL(tab->tabs_count, 1);
    CU_ASSERT_EQUAL(dock_tab_find_index(tab, w1->id), -1);

    dock_manager_free(mgr);
}

static void test_split_and_remove(void) {
    DockManager *mgr = dock_manager_create();
    CU_ASSERT_PTR_NOT_NULL(mgr);

    DockWindow *w1 = dock_manager_add_window(mgr, "l1", "L1");
    DockWindow *w2 = dock_manager_add_window(mgr, "l2", "L2");

    DockNode *leaf1 = dock_node_create_leaf(w1);
    leaf1->id = mgr->next_node_id++;
    dock_manager_set_root(mgr, leaf1);

    DockNode *leaf2 = dock_node_create_leaf(w2);
    leaf2->id = mgr->next_node_id++;

    DockNode *split = dock_manager_split_node(mgr, leaf1, DOCK_SPLIT_HORZ, 0.5f, leaf2);
    CU_ASSERT_PTR_NOT_NULL(split);
    CU_ASSERT_EQUAL(split->type, DOCK_NODE_SPLIT);
    CU_ASSERT_PTR_NOT_NULL(split->child[0]);
    CU_ASSERT_PTR_NOT_NULL(split->child[1]);

    /* 子を削除して親の縮約が起きるか確認 */
    int rr = dock_manager_remove_node(mgr, leaf2);
    CU_ASSERT_EQUAL(rr, 0);
    /* root が残りのリーフ (leaf1) になっていること */
    CU_ASSERT_PTR_NOT_NULL(mgr->root);
    CU_ASSERT_EQUAL(mgr->root->type, DOCK_NODE_LEAF);
    CU_ASSERT_PTR_NOT_NULL(mgr->root->win);
    CU_ASSERT_EQUAL(mgr->root->win->id, w1->id);

    dock_manager_free(mgr);
}

static void test_drop_candidate(void) {
    DockManager *mgr = dock_manager_create();
    CU_ASSERT_PTR_NOT_NULL(mgr);

    DockWindow *w1 = dock_manager_add_window(mgr, "d1", "D1");
    CU_ASSERT_PTR_NOT_NULL(w1);
    DockNode *leaf = dock_node_create_leaf(w1);
    leaf->id = mgr->next_node_id++;
    /* 範囲を設定（幅400x高さ300、左上原点） */
    leaf->rect.x = 0.0f; leaf->rect.y = 0.0f; leaf->rect.w = 400.0f; leaf->rect.h = 300.0f;
    dock_manager_set_root(mgr, leaf);

    /* 左端（LEFT） */
    DockDropCandidate c = dock_manager_get_drop_candidate(mgr, 10.0f, 150.0f);
    CU_ASSERT_PTR_NOT_NULL(c.node);
    CU_ASSERT_EQUAL(c.target, DOCK_TARGET_LEFT);

    /* 右端（RIGHT） */
    c = dock_manager_get_drop_candidate(mgr, 390.0f, 150.0f);
    CU_ASSERT_EQUAL(c.target, DOCK_TARGET_RIGHT);

    /* 上端（TOP） */
    c = dock_manager_get_drop_candidate(mgr, 200.0f, 5.0f);
    CU_ASSERT_EQUAL(c.target, DOCK_TARGET_TOP);

    /* 下端（BOTTOM） */
    c = dock_manager_get_drop_candidate(mgr, 200.0f, 295.0f);
    CU_ASSERT_EQUAL(c.target, DOCK_TARGET_BOTTOM);

    /* 中央（CENTER） */
    c = dock_manager_get_drop_candidate(mgr, 200.0f, 150.0f);
    CU_ASSERT_EQUAL(c.target, DOCK_TARGET_CENTER);

    /* タブノードの判定 */
    DockWindow *w2 = dock_manager_add_window(mgr, "d2", "D2");
    DockNode *tab = dock_manager_tabify_node(mgr, leaf, w2);
    CU_ASSERT_PTR_NOT_NULL(tab);
    /* タブ領域は上部にあるため y 小さい値で TAB を返すはず */
    c = dock_manager_get_drop_candidate(mgr, 10.0f, tab->rect.y + 4.0f);
    /* tab->rect.y は leaf->rect.y を受け継ぐはずだが念のためゼロチェック */
    if (tab->rect.h == 0.0f) tab->rect = leaf->rect;
    c = dock_manager_get_drop_candidate(mgr, 10.0f, tab->rect.y + 4.0f);
    CU_ASSERT_EQUAL(c.target, DOCK_TARGET_TAB);

    dock_manager_free(mgr);
}

static void test_layout_compute(void) {
    DockManager *mgr = dock_manager_create();
    CU_ASSERT_PTR_NOT_NULL(mgr);

    DockWindow *w1 = dock_manager_add_window(mgr, "l1", "L1");
    DockWindow *w2 = dock_manager_add_window(mgr, "l2", "L2");

    DockNode *leaf1 = dock_node_create_leaf(w1);
    leaf1->id = mgr->next_node_id++;
    dock_manager_set_root(mgr, leaf1);

    DockNode *leaf2 = dock_node_create_leaf(w2);
    leaf2->id = mgr->next_node_id++;

    DockNode *split = dock_manager_split_node(mgr, leaf1, DOCK_SPLIT_HORZ, 0.25f, leaf2);
    CU_ASSERT_PTR_NOT_NULL(split);

    struct nk_rect area = { 0.0f, 0.0f, 400.0f, 200.0f };
    /* ここでルートと split の状態を確認 */
    /* ここでは直接 split に対してレイアウトを計算する（テストのため） */
    dock_node_compute_layout(split, area);

    /* 左側幅は 400 * 0.25 = 100 */
    CU_ASSERT_DOUBLE_EQUAL(split->child[0]->rect.w, 100.0f, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(split->child[1]->rect.w, 300.0f, 0.001);
    /* 高さはそのまま */
    CU_ASSERT_DOUBLE_EQUAL(split->child[0]->rect.h, 200.0f, 0.001);

    dock_manager_free(mgr);
}

static void test_move_tab_api(void) {
    DockManager *mgr = dock_manager_create();
    CU_ASSERT_PTR_NOT_NULL(mgr);

    DockWindow *a = dock_manager_add_window(mgr, "a", "A");
    DockWindow *b = dock_manager_add_window(mgr, "b", "B");
    DockWindow *c = dock_manager_add_window(mgr, "c", "C");

    /* src: タブノードに a,b を入れる */
    DockNode *src = dock_node_create_tab();
    dock_tab_add(src, a);
    dock_tab_add(src, b);

    /* dst: リーフに c を置く */
    DockNode *dst_leaf = dock_node_create_leaf(c);

    /* 移動: src のインデックス 0 (a) を dst_leaf にドロップ -> dst がタブ化され a が移動する */
    int r = dock_manager_move_tab(mgr, src, 0, dst_leaf, -1);
    CU_ASSERT_EQUAL(r, 0);
    /* src は 1 要素になっている */
    CU_ASSERT_EQUAL(src->tabs_count, 1);
    /* dst がルートにセットされてタブ化されているはずなので mgr->root を確認 */
    CU_ASSERT_PTR_NOT_NULL(mgr->root);
    CU_ASSERT_EQUAL(mgr->root->type, DOCK_NODE_TAB);
    /* mgr->root に a が含まれていること */
    CU_ASSERT_EQUAL(dock_tab_find_index(mgr->root, a->id) >= 0, 1);

    dock_node_free(src);
    dock_node_free(dst_leaf);
    dock_manager_free(mgr);
}

int main(void) {
    if (CUE_SUCCESS != CU_initialize_registry()) return CU_get_error();
    CU_pSuite suite = CU_add_suite("DockManagerSuite", NULL, NULL);
    if (!suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    CU_add_test(suite, "add_and_tabify", test_add_and_tabify);
    CU_add_test(suite, "remove_window_refs", test_remove_window_refs);
    CU_add_test(suite, "split_and_remove", test_split_and_remove);
    CU_add_test(suite, "drop_candidate", test_drop_candidate);
    CU_add_test(suite, "layout_compute", test_layout_compute);
    CU_add_test(suite, "move_tab_api", test_move_tab_api);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
