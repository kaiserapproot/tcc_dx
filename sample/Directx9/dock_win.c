#include <windows.h>
#include <stdio.h>

#ifndef _STDBOOL_H
#define _STDBOOL_H
#define bool int
#define true 1
#define false 0
#endif // _STDBOOL_H

// ----- ウィジェットのデータ定義 -----
// ウィジェットはウィンドウプロシージャを持たない純粋なデータ構造
typedef struct {
    RECT rect;              // 位置とサイズ
    bool visible;           // 表示状態
    bool dragging;          // ドラッグ状態
    POINT dragOffset;       // ドラッグ時のオフセット
    COLORREF color;         // 色
    int viewportId;         // 配置されているビューポートのID (0=メインウィンドウ, 1=外部ビューポート)
} Widget;

// ----- ビューポート管理 -----
// ビューポートはウィンドウプロシージャを持つOSウィンドウ
typedef struct {
    HWND hwnd;              // ウィンドウハンドル
    int id;                 // ビューポートID
    RECT rect;              // ビューポートのスクリーン座標上の位置とサイズ
    bool active;            // アクティブ状態
} Viewport;

// グローバル変数
HINSTANCE g_hInstance;
Widget g_Square = { {100, 100, 200, 200}, true, false, {0, 0}, RGB(255, 0, 0), 0 };
Viewport g_Viewports[2] = { {0} }; // 0=メインウィンドウ, 1=外部ビューポート

// 関数プロトタイプ
LRESULT CALLBACK ViewportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateExternalViewport(int x, int y, int width, int height);
void DestroyExternalViewport();
void UpdateUI();
void ProcessInput();
void RenderUI();
bool IsWidgetInsideViewport(Widget* widget, Viewport* viewport);
void OutputDebugLog(const char* format, ...);
Viewport* FindViewportByHWND(HWND hwnd);

// メイン関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;
    
    // ウィンドウクラスを登録（ビューポート用の1種類だけ）
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = ViewportWndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName = TEXT("ImGuiStyleViewport");
    RegisterClassEx(&wcex);
    
    // メインビューポート（メインウィンドウ）を作成
    g_Viewports[0].hwnd = CreateWindow(
        TEXT("ImGuiStyleViewport"),
        TEXT("ImGuiスタイルサンプル"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL, hInstance, NULL);
        
    if (!g_Viewports[0].hwnd) {
        MessageBox(NULL, TEXT("ウィンドウの作成に失敗しました"), TEXT("エラー"), MB_ICONERROR);
        return 1;
    }
    
    g_Viewports[0].id = 0;
    g_Viewports[0].active = true;
    GetClientRect(g_Viewports[0].hwnd, &g_Viewports[0].rect);
    
    ShowWindow(g_Viewports[0].hwnd, nCmdShow);
    UpdateWindow(g_Viewports[0].hwnd);
    
    // メインループ（ImGui風）
    MSG msg;
    bool running = true;
    while (running) {
        // メッセージ処理
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        UpdateUI();      // UI状態の更新（ImGui::NewFrame相当）
        ProcessInput();   // 入力処理
        RenderUI();      // UI描画（ImGui::Render相当）
        
        Sleep(10);
    }
    
    return (int)msg.wParam;
}

// UI状態の更新
void UpdateUI() {
    // ビューポートのスクリーン位置を更新
    if (g_Viewports[0].active) {
        RECT rect;
        POINT pt = {0, 0};
        GetClientRect(g_Viewports[0].hwnd, &rect);
        ClientToScreen(g_Viewports[0].hwnd, &pt);
        g_Viewports[0].rect.left = pt.x;
        g_Viewports[0].rect.top = pt.y;
        g_Viewports[0].rect.right = pt.x + rect.right;
        g_Viewports[0].rect.bottom = pt.y + rect.bottom;
    }
    
    if (g_Viewports[1].active) {
        RECT rect;
        GetWindowRect(g_Viewports[1].hwnd, &rect);
        g_Viewports[1].rect = rect;
    }
}

// 入力処理
void ProcessInput() {
    // マウスが押されていない場合、ドラッグ状態を解除
    if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000) && g_Square.dragging) {
        g_Square.dragging = false;
        ReleaseCapture();
    }
    
    // ドラッグ中の処理
    if (g_Square.dragging) {
        POINT mousePos;
        GetCursorPos(&mousePos);
        
        int width = g_Square.rect.right - g_Square.rect.left;
        int height = g_Square.rect.bottom - g_Square.rect.top;
        
        if (g_Square.viewportId == 0) {
            // メインビューポート内
            POINT clientPos = mousePos;
            ScreenToClient(g_Viewports[0].hwnd, &clientPos);
            
            g_Square.rect.left = clientPos.x - g_Square.dragOffset.x;
            g_Square.rect.top = clientPos.y - g_Square.dragOffset.y;
            g_Square.rect.right = g_Square.rect.left + width;
            g_Square.rect.bottom = g_Square.rect.top + height;
            
            // ウィンドウ外に出たかチェック
            RECT clientRect;
            GetClientRect(g_Viewports[0].hwnd, &clientRect);
            if (g_Square.rect.left < 0 || g_Square.rect.top < 0 ||
                g_Square.rect.right > clientRect.right || g_Square.rect.bottom > clientRect.bottom) {
                
                OutputDebugLog("四角形がメインビューポートから外に出ました");
                
                // 外部ビューポートを作成
                g_Square.viewportId = 1;
                CreateExternalViewport(mousePos.x - g_Square.dragOffset.x, 
                                       mousePos.y - g_Square.dragOffset.y,
                                       width, height);
                
                // ドラッグオフセットを再設定
                g_Square.dragOffset.x = g_Square.dragOffset.x;
                g_Square.dragOffset.y = g_Square.dragOffset.y;
                
                // メインビューポートの再描画
                InvalidateRect(g_Viewports[0].hwnd, NULL, TRUE);
            }
        } else if (g_Square.viewportId == 1 && g_Viewports[1].active) {
            // 外部ビューポート - ウィンドウ自体を移動
            int newX = mousePos.x - g_Square.dragOffset.x;
            int newY = mousePos.y - g_Square.dragOffset.y;
            SetWindowPos(g_Viewports[1].hwnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            
            // メインウィンドウ内に入ったか確認
            RECT vpRect;
            GetWindowRect(g_Viewports[1].hwnd, &vpRect);
            
            // メインビューポートのスクリーン座標を取得
            if (vpRect.left >= g_Viewports[0].rect.left && 
                vpRect.right <= g_Viewports[0].rect.right &&
                vpRect.top >= g_Viewports[0].rect.top && 
                vpRect.bottom <= g_Viewports[0].rect.bottom) {
                
                OutputDebugLog("四角形がメインビューポート内に戻りました");
                
                // メインビューポートのクライアント座標に変換
                POINT ptClient = {vpRect.left, vpRect.top};
                ScreenToClient(g_Viewports[0].hwnd, &ptClient);
                
                // 四角形の位置を設定
                g_Square.rect.left = ptClient.x;
                g_Square.rect.top = ptClient.y;
                g_Square.rect.right = g_Square.rect.left + width;
                g_Square.rect.bottom = g_Square.rect.top + height;
                
                // メインビューポートに戻す
                g_Square.viewportId = 0;
                DestroyExternalViewport();
                
                // メインビューポートを再描画
                InvalidateRect(g_Viewports[0].hwnd, NULL, TRUE);
            }
        }
    }
}

// UI描画
void RenderUI() {
    // 各ビューポートを再描画
    if (g_Viewports[0].active) {
        InvalidateRect(g_Viewports[0].hwnd, NULL, TRUE);
    }
    if (g_Viewports[1].active) {
        InvalidateRect(g_Viewports[1].hwnd, NULL, TRUE);
    }
}

// ビューポートウィンドウプロシージャ（すべてのビューポート共通）
LRESULT CALLBACK ViewportWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // hwndからこのビューポートを見つける
    Viewport* viewport = FindViewportByHWND(hwnd);
    
    switch (msg) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // メインビューポートの場合
            if (viewport && viewport->id == 0) {
                // 四角形がメインビューポートにあれば描画
                if (g_Square.viewportId == 0 && g_Square.visible) {
                    HBRUSH hBrush = CreateSolidBrush(g_Square.color);
                    SelectObject(hdc, hBrush);
                    Rectangle(hdc, 
                             g_Square.rect.left, 
                             g_Square.rect.top, 
                             g_Square.rect.right, 
                             g_Square.rect.bottom);
                    DeleteObject(hBrush);
                }
            }
            // 外部ビューポート（四角形ウィンドウ）の場合
            else if (viewport && viewport->id == 1) {
                // 四角形全体を描画（ウィンドウ全体）
                HBRUSH hBrush = CreateSolidBrush(g_Square.color);
                SelectObject(hdc, hBrush);
                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                Rectangle(hdc, 0, 0, clientRect.right, clientRect.bottom);
                DeleteObject(hBrush);
            }
            
            EndPaint(hwnd, &ps);
        }
        return 0;
        
    case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            
            // メインビューポートでの処理
            if (viewport && viewport->id == 0) {
                // 四角形がメインビューポートにあり、クリックされたか
                if (g_Square.viewportId == 0 &&
                    x >= g_Square.rect.left && x <= g_Square.rect.right &&
                    y >= g_Square.rect.top && y <= g_Square.rect.bottom) {
                    
                    g_Square.dragging = true;
                    g_Square.dragOffset.x = x - g_Square.rect.left;
                    g_Square.dragOffset.y = y - g_Square.rect.top;
                    SetCapture(hwnd); // マウスキャプチャ開始
                }
            }
            // 外部ビューポートでの処理
            else if (viewport && viewport->id == 1) {
                g_Square.dragging = true;
                g_Square.dragOffset.x = x;
                g_Square.dragOffset.y = y;
                SetCapture(hwnd); // マウスキャプチャ開始
            }
        }
        return 0;
        
    // 外部ビューポートのヒットテスト
    case WM_NCHITTEST:
        if (viewport && viewport->id == 1) {
            return HTCLIENT;
        }
        break;
        
    case WM_SETCURSOR:
        if (viewport && viewport->id == 1) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        break;
        
    case WM_DESTROY:
        if (viewport && viewport->id == 0) {
            PostQuitMessage(0);
        }
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 外部ビューポートを作成
void CreateExternalViewport(int x, int y, int width, int height) {
    if (g_Viewports[1].active) {
        DestroyExternalViewport();
    }
    
    // CreateWindowの代わりにCreateWindowExを使用
    g_Viewports[1].hwnd = CreateWindowEx(
        WS_EX_COMPOSITED,  // 合成ウィンドウとして描画
        TEXT("ImGuiStyleViewport"),
        TEXT(""),
        WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, g_hInstance, NULL);
        
    if (g_Viewports[1].hwnd) {
        g_Viewports[1].id = 1;
        g_Viewports[1].active = true;
        GetWindowRect(g_Viewports[1].hwnd, &g_Viewports[1].rect);
        
        OutputDebugLog("外部ビューポートが作成されました (HWND: %p)", g_Viewports[1].hwnd);
        ShowWindow(g_Viewports[1].hwnd, SW_SHOW);
        UpdateWindow(g_Viewports[1].hwnd);
    }
}

// 外部ビューポートを破棄
void DestroyExternalViewport() {
    if (g_Viewports[1].active) {
        OutputDebugLog("外部ビューポートが破棄されます (HWND: %p)", g_Viewports[1].hwnd);
        DestroyWindow(g_Viewports[1].hwnd);
        g_Viewports[1].hwnd = NULL;
        g_Viewports[1].active = false;
    }
}

// ウィジェットがビューポート内にあるかチェック
bool IsWidgetInsideViewport(Widget* widget, Viewport* viewport) {
    if (viewport->id == 0) {
        // メインビューポート用の判定
        RECT clientRect;
        GetClientRect(viewport->hwnd, &clientRect);
        return (widget->rect.left >= 0 && widget->rect.right <= clientRect.right &&
                widget->rect.top >= 0 && widget->rect.bottom <= clientRect.bottom);
    }
    return false;
}

// HWNDからビューポートを検索
Viewport* FindViewportByHWND(HWND hwnd) {
    for (int i = 0; i < 2; i++) {
        if (g_Viewports[i].hwnd == hwnd) {
            return &g_Viewports[i];
        }
    }
    return NULL;
}

// デバッグログ出力関数
void OutputDebugLog(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}