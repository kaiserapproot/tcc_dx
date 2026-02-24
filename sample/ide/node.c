#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include <d3d9.h>
// #pragma comment(lib, "d3d9.lib")

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

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

/* グローバル変数 */
static IDirect3D9 *d3d = NULL;
static IDirect3DDevice9 *device = NULL;
static D3DPRESENT_PARAMETERS present = {0};

/* ノード定義 */
enum node_type {
    NODE_INPUT,
    NODE_OUTPUT,
    NODE_ADD,
    NODE_MULTIPLY
};

struct node_socket {
    float value;
    int connected_node_id;
    int connected_socket_id;
    nk_bool is_input;
};

struct node {
    int id;
    enum node_type type;
    struct nk_vec2 pos;
    struct nk_vec2 size;
    char name[64];
    struct node_socket inputs[4];
    struct node_socket outputs[4];
    int input_count;
    int output_count;
    nk_bool selected;
};

struct node_editor {
    struct node nodes[32];
    int node_count;
    int selected_node;
    struct nk_vec2 scrolling;
    nk_bool show_grid;
    int linking_node;
    int linking_socket;
    nk_bool linking_input;
    struct nk_vec2 link_start_pos;
};

/* グローバル変数 */
static struct node_editor editor = {0};
static struct nk_context *ctx = NULL;

/* エディタ初期化 */
static void init_editor(void) {
    editor.linking_node = -1;
    editor.selected_node = -1;
    editor.show_grid = nk_true;
}

/* ノード作成関数 */
static void create_node(enum node_type type, float x, float y) {
    if (editor.node_count >= 32) return;
    
    struct node *node = &editor.nodes[editor.node_count];
    node->id = editor.node_count;
    node->type = type;
    node->pos = nk_vec2(x, y);
    node->size = nk_vec2(120, 80);
    node->selected = nk_false;
    
    /* 接続の初期化 */
    for (int i = 0; i < 4; i++) {
        node->inputs[i].connected_node_id = -1;
        node->inputs[i].connected_socket_id = -1;
        node->inputs[i].is_input = nk_true;
        node->inputs[i].value = 0.0f;
        
        node->outputs[i].connected_node_id = -1;
        node->outputs[i].connected_socket_id = -1;
        node->outputs[i].is_input = nk_false;
        node->outputs[i].value = 0.0f;
    }
    
    switch (type) {
        case NODE_INPUT:
            strcpy(node->name, "Input");
            node->input_count = 0;
            node->output_count = 1;
            node->outputs[0].value = 1.0f;
            break;
        case NODE_OUTPUT:
            strcpy(node->name, "Output");
            node->input_count = 1;
            node->output_count = 0;
            break;
        case NODE_ADD:
            strcpy(node->name, "Add");
            node->input_count = 2;
            node->output_count = 1;
            break;
        case NODE_MULTIPLY:
            strcpy(node->name, "Multiply");
            node->input_count = 2;
            node->output_count = 1;
            break;
    }
    
    editor.node_count++;
}

/* ノード計算 */
static void calculate_node(struct node *node) {
    switch (node->type) {
        case NODE_INPUT:
            /* 入力ノードは手動で値を設定 */
            break;
        case NODE_OUTPUT:
            /* 出力ノードは入力値をそのまま使用 */
            break;
        case NODE_ADD:
            node->outputs[0].value = node->inputs[0].value + node->inputs[1].value;
            break;
        case NODE_MULTIPLY:
            node->outputs[0].value = node->inputs[0].value * node->inputs[1].value;
            break;
    }
}

/* 接続作成 */
static void create_link(int from_node, int from_socket, int to_node, int to_socket) {
    if (from_node < 0 || from_node >= editor.node_count ||
        to_node < 0 || to_node >= editor.node_count) return;
    
    struct node *from = &editor.nodes[from_node];
    struct node *to = &editor.nodes[to_node];
    
    if (from_socket >= from->output_count || to_socket >= to->input_count) return;
    
    /* 接続情報を設定 */
    from->outputs[from_socket].connected_node_id = to_node;
    from->outputs[from_socket].connected_socket_id = to_socket;
    
    to->inputs[to_socket].connected_node_id = from_node;
    to->inputs[to_socket].connected_socket_id = from_socket;
}

/* ソケット位置計算 */
static struct nk_vec2 get_socket_pos(struct node *node, int socket_id, nk_bool is_input) {
    struct nk_vec2 pos = node->pos;
    float socket_height = 20.0f;
    
    if (is_input) {
        pos.x -= 8;
        pos.y += 30 + socket_id * socket_height;
    } else {
        pos.x += node->size.x + 8;
        pos.y += 30 + socket_id * socket_height;
    }
    
    return pos;
}

/* ベクトル長計算関数 (nk_vec2_len の代替) */
static float vec2_length(struct nk_vec2 v) {
    return (float)sqrt(v.x * v.x + v.y * v.y);
}

/* ベジエ曲線計算関数 */
static struct nk_vec2 bezier_cubic(struct nk_vec2 p0, struct nk_vec2 p1, struct nk_vec2 p2, struct nk_vec2 p3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    struct nk_vec2 p;
    p.x = uuu * p0.x + 3 * uu * t * p1.x + 3 * u * tt * p2.x + ttt * p3.x;
    p.y = uuu * p0.y + 3 * uu * t * p1.y + 3 * u * tt * p2.y + ttt * p3.y;
    
    return p;
}

/* 接続線描画 */
static void draw_connection(struct nk_command_buffer *canvas, struct nk_vec2 from, struct nk_vec2 to) {
    struct nk_color line_color = nk_rgb(150, 150, 150);
    
    /* ベジエ曲線で接続線を描画 */
    struct nk_vec2 diff = nk_vec2_sub(to, from);
    float dist = vec2_length(diff);
    float control_dist = NK_MAX(dist * 0.5f, 100.0f); /* 最小距離を100に設定 */
    
    /* 制御点を水平方向に設定してより自然な曲線にする */
    struct nk_vec2 cp1 = nk_vec2(from.x + control_dist, from.y);
    struct nk_vec2 cp2 = nk_vec2(to.x - control_dist, to.y);
    
    /* ベジエ曲線を複数の線分で近似して描画 */
    int segments = 20; /* 曲線の滑らかさ */
    struct nk_vec2 prev_point = from;
    
    for (int i = 1; i <= segments; i++) {
        float t = (float)i / (float)segments;
        struct nk_vec2 current_point = bezier_cubic(from, cp1, cp2, to, t);
        
        nk_stroke_line(canvas, prev_point.x, prev_point.y, 
                      current_point.x, current_point.y, 2.0f, line_color);
        
        prev_point = current_point;
    }
}

/* 接続中の線描画（異なる色で表示） */
static void draw_connecting_line(struct nk_command_buffer *canvas, struct nk_vec2 from, struct nk_vec2 to) {
    struct nk_color line_color = nk_rgb(255, 200, 100); /* オレンジ色で接続中を表示 */
    
    /* ベジエ曲線で接続線を描画 */
    struct nk_vec2 diff = nk_vec2_sub(to, from);
    float dist = vec2_length(diff);
    float control_dist = NK_MAX(dist * 0.5f, 100.0f);
    
    struct nk_vec2 cp1 = nk_vec2(from.x + control_dist, from.y);
    struct nk_vec2 cp2 = nk_vec2(to.x - control_dist, to.y);
    
    /* ベジエ曲線を複数の線分で近似して描画 */
    int segments = 20;
    struct nk_vec2 prev_point = from;
    
    for (int i = 1; i <= segments; i++) {
        float t = (float)i / (float)segments;
        struct nk_vec2 current_point = bezier_cubic(from, cp1, cp2, to, t);
        
        nk_stroke_line(canvas, prev_point.x, prev_point.y, 
                      current_point.x, current_point.y, 3.0f, line_color); /* 太めに描画 */
        
        prev_point = current_point;
    }
}

/* ノードエディタUI */
static void node_editor_ui(struct nk_context *ctx) {
    if (nk_begin(ctx, "Node Editor", nk_rect(0, 0, WINDOW_WIDTH - 240, WINDOW_HEIGHT),
                 NK_WINDOW_BORDER|NK_WINDOW_TITLE)) {
        
        /* ツールバー */
        nk_layout_row_dynamic(ctx, 30, 4);
        if (nk_button_label(ctx, "Add Input")) {
            create_node(NODE_INPUT, 100, 100);
        }
        if (nk_button_label(ctx, "Add Output")) {
            create_node(NODE_OUTPUT, 300, 100);
        }
        if (nk_button_label(ctx, "Add Add")) {
            create_node(NODE_ADD, 200, 200);
        }
        if (nk_button_label(ctx, "Add Multiply")) {
            create_node(NODE_MULTIPLY, 200, 300);
        }
        
        /* ノードエディタ領域 - Propertiesウィンドウ分を除いたサイズを設定 */
        nk_layout_row_dynamic(ctx, WINDOW_HEIGHT - 80, 1);
        struct nk_rect canvas_rect = nk_widget_bounds(ctx);
        if (nk_group_begin(ctx, "node_editor", NK_WINDOW_BORDER)) {
            struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
            struct nk_input *input = &ctx->input;
            
            /* マウスがノードエディタ領域内にある場合のみ処理 */
            nk_bool mouse_in_editor = nk_input_is_mouse_hovering_rect(input, canvas_rect);
            
            /* 背景グリッド */
            if (editor.show_grid) {
                struct nk_color grid_color = nk_rgb(50, 50, 50);
                for (int x = 0; x < (int)canvas_rect.w; x += 20) {
                    nk_stroke_line(canvas, canvas_rect.x + x, canvas_rect.y, 
                                 canvas_rect.x + x, canvas_rect.y + canvas_rect.h, 1, grid_color);
                }
                for (int y = 0; y < (int)canvas_rect.h; y += 20) {
                    nk_stroke_line(canvas, canvas_rect.x, canvas_rect.y + y,
                                 canvas_rect.x + canvas_rect.w, canvas_rect.y + y, 1, grid_color);
                }
            }
            
            /* 接続線描画 */
            for (int i = 0; i < editor.node_count; i++) {
                struct node *node = &editor.nodes[i];
                for (int j = 0; j < node->output_count; j++) {
                    if (node->outputs[j].connected_node_id >= 0) {
                        struct node *target = &editor.nodes[node->outputs[j].connected_node_id];
                        struct nk_vec2 from = get_socket_pos(node, j, nk_false);
                        struct nk_vec2 to = get_socket_pos(target, node->outputs[j].connected_socket_id, nk_true);
                        draw_connection(canvas, from, to);
                    }
                }
            }
            
            /* 接続中の線を描画 */
            if (editor.linking_node >= 0) {
                struct nk_vec2 mouse_pos = nk_vec2(input->mouse.pos.x, input->mouse.pos.y);
                draw_connecting_line(canvas, editor.link_start_pos, mouse_pos);
                
                /* 右クリックで接続キャンセル（エディタ領域内のみ） */
                if (mouse_in_editor && nk_input_is_mouse_pressed(input, NK_BUTTON_RIGHT)) {
                    editor.linking_node = -1;
                }
            }
            
            /* ノード描画 */
            for (int i = 0; i < editor.node_count; i++) {
                struct node *node = &editor.nodes[i];
                
                /* ノード本体 */
                struct nk_rect node_rect = nk_rect(node->pos.x, node->pos.y, node->size.x, node->size.y);
                struct nk_color node_color = node->selected ? nk_rgb(100, 150, 200) : nk_rgb(80, 80, 80);
                nk_fill_rect(canvas, node_rect, 5.0f, node_color);
                nk_stroke_rect(canvas, node_rect, 5.0f, 2.0f, nk_rgb(200, 200, 200));
                
                /* ノード名 */
                nk_draw_text(canvas, nk_rect(node->pos.x + 10, node->pos.y + 5, node->size.x - 20, 20),
                           node->name, (int)strlen(node->name), ctx->style.font, nk_rgb(60, 60, 60), nk_rgb(255, 255, 255));
                
                /* 入力ソケット */
                for (int j = 0; j < node->input_count; j++) {
                    struct nk_vec2 socket_pos = get_socket_pos(node, j, nk_true);
                    struct nk_rect socket_rect = nk_rect(socket_pos.x - 8, socket_pos.y - 8, 16, 16);
                    struct nk_color socket_color = nk_rgb(100, 200, 100);
                    nk_fill_circle(canvas, nk_rect(socket_pos.x - 5, socket_pos.y - 5, 10, 10), socket_color);
                    
                    /* 入力ソケットでの接続処理 */
                    if (mouse_in_editor && nk_input_is_mouse_hovering_rect(input, socket_rect)) {
                        if (editor.linking_node >= 0 && !editor.linking_input && 
                            nk_input_is_mouse_released(input, NK_BUTTON_LEFT)) {
                            /* 出力から入力への接続を作成 */
                            create_link(editor.linking_node, editor.linking_socket, i, j);
                            editor.linking_node = -1;
                        }
                        socket_color = nk_rgb(150, 255, 150); /* ホバー時の色 */
                        nk_fill_circle(canvas, nk_rect(socket_pos.x - 5, socket_pos.y - 5, 10, 10), socket_color);
                    }
                    
                    /* 値表示 */
                    char value_str[32];
                    sprintf(value_str, "%.1f", node->inputs[j].value);
                    nk_draw_text(canvas, nk_rect(socket_pos.x + 15, socket_pos.y - 8, 40, 16),
                               value_str, (int)strlen(value_str), ctx->style.font, nk_rgb(60, 60, 60), nk_rgb(255, 255, 255));
                }
                
                /* 出力ソケット */
                for (int j = 0; j < node->output_count; j++) {
                    struct nk_vec2 socket_pos = get_socket_pos(node, j, nk_false);
                    struct nk_rect socket_rect = nk_rect(socket_pos.x - 8, socket_pos.y - 8, 16, 16);
                    struct nk_color socket_color = nk_rgb(200, 100, 100);
                    nk_fill_circle(canvas, nk_rect(socket_pos.x - 5, socket_pos.y - 5, 10, 10), socket_color);
                    
                    /* 出力ソケットでの接続開始処理 */
                    if (mouse_in_editor && nk_input_is_mouse_hovering_rect(input, socket_rect)) {
                        if (nk_input_is_mouse_pressed(input, NK_BUTTON_LEFT)) {
                            /* 接続開始 */
                            editor.linking_node = i;
                            editor.linking_socket = j;
                            editor.linking_input = nk_false;
                            editor.link_start_pos = socket_pos;
                        }
                        socket_color = nk_rgb(255, 150, 150); /* ホバー時の色 */
                        nk_fill_circle(canvas, nk_rect(socket_pos.x - 5, socket_pos.y - 5, 10, 10), socket_color);
                    }
                    
                    /* 値表示 */
                    char value_str[32];
                    sprintf(value_str, "%.1f", node->outputs[j].value);
                    nk_draw_text(canvas, nk_rect(socket_pos.x - 50, socket_pos.y - 8, 40, 16),
                               value_str, (int)strlen(value_str), ctx->style.font, nk_rgb(255, 255, 255), nk_rgb(60, 60, 60));
                }
                
                /* ノード移動処理 - ソケット以外の領域でのみ */
                nk_bool on_socket = nk_false;
                for (int k = 0; k < node->input_count; k++) {
                    struct nk_vec2 socket_pos = get_socket_pos(node, k, nk_true);
                    struct nk_rect socket_rect = nk_rect(socket_pos.x - 8, socket_pos.y - 8, 16, 16);
                    if (nk_input_is_mouse_hovering_rect(input, socket_rect)) {
                        on_socket = nk_true;
                        break;
                    }
                }
                for (int k = 0; k < node->output_count && !on_socket; k++) {
                    struct nk_vec2 socket_pos = get_socket_pos(node, k, nk_false);
                    struct nk_rect socket_rect = nk_rect(socket_pos.x - 8, socket_pos.y - 8, 16, 16);
                    if (nk_input_is_mouse_hovering_rect(input, socket_rect)) {
                        on_socket = nk_true;
                        break;
                    }
                }
                
                if (mouse_in_editor && !on_socket && nk_input_is_mouse_hovering_rect(input, node_rect) && 
                    nk_input_is_mouse_pressed(input, NK_BUTTON_LEFT) && editor.linking_node < 0) {
                    editor.selected_node = i;
                    node->selected = nk_true;
                } else if (i != editor.selected_node) {
                    node->selected = nk_false;
                }
                
                if (mouse_in_editor && editor.selected_node == i && nk_input_is_mouse_down(input, NK_BUTTON_LEFT) && 
                    editor.linking_node < 0 && !on_socket) {
                    node->pos.x += input->mouse.delta.x;
                    node->pos.y += input->mouse.delta.y;
                }
            }
            
            /* 値の更新と計算 */
            for (int i = 0; i < editor.node_count; i++) {
                struct node *node = &editor.nodes[i];
                
                /* 入力値の更新 */
                for (int j = 0; j < node->input_count; j++) {
                    if (node->inputs[j].connected_node_id >= 0) {
                        struct node *source = &editor.nodes[node->inputs[j].connected_node_id];
                        node->inputs[j].value = source->outputs[node->inputs[j].connected_socket_id].value;
                    }
                }
                
                calculate_node(node);
            }
            
            nk_group_end(ctx);
        }
    }
    nk_end(ctx);
    
    /* サイドパネル */
    if (nk_begin(ctx, "Properties", nk_rect(WINDOW_WIDTH - 230, 0, 230, WINDOW_HEIGHT),
                 NK_WINDOW_BORDER|NK_WINDOW_TITLE)) {
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_checkbox_label(ctx, "Show Grid", &editor.show_grid);
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Selected Node:", NK_TEXT_LEFT);
        
        if (editor.selected_node >= 0 && editor.selected_node < editor.node_count) {
            struct node *node = &editor.nodes[editor.selected_node];
            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, node->name, NK_TEXT_LEFT);
            
            /* 入力ノードの値編集 */
            if (node->type == NODE_INPUT) {
                nk_layout_row_dynamic(ctx, 25, 1);
                nk_property_float(ctx, "Value:", 0.0f, &node->outputs[0].value, 100.0f, 0.1f, 0.1f);
            }
        }
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Instructions:", NK_TEXT_LEFT);
        nk_label(ctx, "- Click to select node", NK_TEXT_LEFT);
        nk_label(ctx, "- Drag to move node", NK_TEXT_LEFT);
        nk_label(ctx, "- Green: Input sockets", NK_TEXT_LEFT);
        nk_label(ctx, "- Red: Output sockets", NK_TEXT_LEFT);
        nk_label(ctx, "- Click red socket to start", NK_TEXT_LEFT);
        nk_label(ctx, "- Click green socket to connect", NK_TEXT_LEFT);
        nk_label(ctx, "- Right click to cancel", NK_TEXT_LEFT);
    }
    nk_end(ctx);
}

/* DirectX 9 初期化 */
static int init_d3d9(HWND hwnd) {
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return 0;
    
    ZeroMemory(&present, sizeof(present));
    present.Windowed = TRUE;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.BackBufferFormat = D3DFMT_UNKNOWN;
    present.EnableAutoDepthStencil = TRUE;
    present.AutoDepthStencilFormat = D3DFMT_D16;
    present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    
    HRESULT hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present, &device);
    
    return SUCCEEDED(hr);
}

/* DirectX 9 終了処理 */
static void cleanup_d3d9(void) {
    if (device) {
        IDirect3DDevice9_Release(device);
        device = NULL;
    }
    if (d3d) {
        IDirect3D9_Release(d3d);
        d3d = NULL;
    }
}

/* デバイスロスト処理 */
static void handle_device_lost(void) {
    HRESULT hr = IDirect3DDevice9_TestCooperativeLevel(device);
    if (hr == D3DERR_DEVICENOTRESET) {
        nk_d3d9_release();
        hr = IDirect3DDevice9_Reset(device, &present);
        if (SUCCEEDED(hr)) {
            nk_d3d9_resize(WINDOW_WIDTH, WINDOW_HEIGHT);
        }
    }
}

/* ウィンドウプロシージャ */
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            if (device) {
                handle_device_lost();
            }
            break;
    }
    
    if (nk_d3d9_handle_event(hwnd, msg, wparam, lparam))
        return 0;
    
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    /* ウィンドウクラス登録 */
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = L"NuklearNodeEditor";
    RegisterClassW(&wc);
    
    /* ウィンドウ作成 */
    RECT rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;
    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    
    HWND hwnd = CreateWindowExW(exstyle, wc.lpszClassName, L"Nuklear Node Editor - DirectX9",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL);
    
    if (!hwnd) {
        MessageBoxA(NULL, "Failed to create window", "Error", MB_OK);
        return -1;
    }
    
    /* DirectX 9 初期化 */
    if (!init_d3d9(hwnd)) {
        MessageBoxA(NULL, "Failed to initialize DirectX9", "Error", MB_OK);
        return -1;
    }
    
    /* Nuklear初期化 */
    ctx = nk_d3d9_init(device, WINDOW_WIDTH, WINDOW_HEIGHT);
    {
        struct nk_font_atlas *atlas;
        nk_d3d9_font_stash_begin(&atlas);
        nk_d3d9_font_stash_end();
    }
    
    /* エディタ初期化 */
    init_editor();
    
    /* 初期ノード作成 */
    create_node(NODE_INPUT, 100, 100);
    create_node(NODE_ADD, 250, 120);
    create_node(NODE_MULTIPLY, 400, 140);
    create_node(NODE_OUTPUT, 550, 160);
    
    /* 初期接続 */
    create_link(0, 0, 1, 0);  /* Input -> Add */
    create_link(1, 0, 2, 0);  /* Add -> Multiply */
    create_link(2, 0, 3, 0);  /* Multiply -> Output */
    
    editor.nodes[0].outputs[0].value = 5.0f;  /* 初期値設定 */
    editor.nodes[1].inputs[1].value = 3.0f;   /* Add の第2入力 */
    editor.nodes[2].inputs[1].value = 2.0f;   /* Multiply の第2入力 */
    
    /* メインループ */
    struct nk_color background = nk_rgb(28, 48, 62);
    MSG msg;
    while (1) {
        nk_input_begin(ctx);
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                goto cleanup;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        nk_input_end(ctx);
        
        /* UI描画 */
        node_editor_ui(ctx);
        
        /* DirectX描画 */
        HRESULT hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
            D3DCOLOR_COLORVALUE(background.r/255.0f, background.g/255.0f, background.b/255.0f, 1.0f), 1.0f, 0);
        
        if (SUCCEEDED(hr)) {
            hr = IDirect3DDevice9_BeginScene(device);
            if (SUCCEEDED(hr)) {
                nk_d3d9_render(NK_ANTI_ALIASING_ON);
                hr = IDirect3DDevice9_EndScene(device);
                if (SUCCEEDED(hr)) {
                    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
                }
            }
        }
        
        if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET) {
            handle_device_lost();
        }
    }
    
cleanup:
    nk_d3d9_shutdown();
    cleanup_d3d9();
    UnregisterClassW(wc.lpszClassName, hInstance);
    return 0;
}