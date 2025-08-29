#ifdef __ANDROID__
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
//#include <GLES2/gl2.h>
//#include <GLES3/gl3.h>
#include <stdlib.h>
#include <string.h>
#include "NkGLES.h"
#include "overview.c"

/* global screen size for layout (set in InitDisplay) */
static int g_screen_w = 0;
static int g_screen_h = 0;

/* jpedit の簡易実装をここに埋め込む (C リンケージ) */
#define JPEDIT_MAX_TEXT_SIZE 65536

#ifdef __cplusplus
extern "C" {
#endif

/* forward declare jpedit GUI so other functions can call it before its definition */
void jpedit_draw_gui(struct nk_context* ctx);

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

void nk_draw_jpedit(struct nk_context* ctx)
{
    static char internal_text[JPEDIT_MAX_TEXT_SIZE] = "// Japanese editor (Android)\n// type here...\n";
    static int initialized = 0;
    if (!initialized) { initialized = 1; }

    int tabw = 4;
    /* make window size similar to Windows jpedit: use most of screen area above properties */
    int win_w = g_screen_w > 0 ? g_screen_w - 40 : 360;
    int win_h = g_screen_h > 0 ? (g_screen_h - (g_screen_h/4) - 40) : 600; /* leave room for properties */
    struct nk_rect bounds = nk_rect(20, 20, (float)win_w, (float)win_h);

    if (nk_begin(ctx, "Japanese Editor", bounds,
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
    {
        /* toolbar, status, then a large multiline editor area */
        int toolbar_h = 48;
        int status_h = 24;
        int editor_h_calc = win_h - (toolbar_h + status_h + 40);
        if (editor_h_calc < 120) editor_h_calc = 120;

        /* toolbar: Save / Clear */
        nk_layout_row_dynamic(ctx, toolbar_h, 3);
        if (nk_button_label(ctx, "Save")) {
            nk_tooltip(ctx, "Save not implemented on Android sample");
        }
        if (nk_button_label(ctx, "New")) { /* noop for now */ }
        if (nk_button_label(ctx, "Clear")) { internal_text[0] = '\0'; }

        /* status line */
        int lines = 1, chars = (int)strlen(internal_text);
        for (int i = 0; internal_text[i]; ++i) if (internal_text[i] == '\n') ++lines;
        char status[64]; snprintf(status, sizeof(status), "%d lines, %d ch", lines, chars);
        nk_layout_row_dynamic(ctx, status_h, 1);
        nk_label(ctx, status, NK_TEXT_LEFT);

        /* big editor area */
        nk_layout_row_dynamic(ctx, editor_h_calc, 1);
        static char expanded[JPEDIT_MAX_TEXT_SIZE];
        static char temp[JPEDIT_MAX_TEXT_SIZE];
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

#ifdef __cplusplus
}
#endif
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "NativeActivity", __VA_ARGS__))

struct saved_state
{
    float angle;
    int32_t x;
    int32_t y;
};
class Engine;
typedef struct android_app AndroidApp;
int initializedDisplay = false;
static int engine_init_display(Engine* eng);
GLuint generate_shader_program(const char* pvShader, const char* pfShader);
#if 1
// 簡易的なシェーダビルド関数を追加（欠落していたためここで実装）
GLuint generate_shader_program(const char* pvShader, const char* pfShader)
{
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &pvShader, NULL);
    glCompileShader(v);

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &pfShader, NULL);
    glCompileShader(f);

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    return p;
}
#endif
#include <jni.h>
#include <android/log.h>
#include <android/native_activity.h>

void WaitForDebugger(ANativeActivity* activity)
{
    JNIEnv* env;
    activity->vm->AttachCurrentThread(&env, NULL);

    jclass debugClass = env->FindClass("android/os/Debug");
    jmethodID waitForDebuggerMethod = env->GetStaticMethodID(debugClass, "waitForDebugger", "()V");
    env->CallStaticVoidMethod(debugClass, waitForDebuggerMethod);

    activity->vm->DetachCurrentThread();
}
void jpedit_draw_gui(struct nk_context* ctx)
{
    if (!ctx) return;

    /* jpedit の本体ロジックをここに実装して main.cpp 単体で完結させる */
    static char internal_text[JPEDIT_MAX_TEXT_SIZE] = "// Japanese editor (Android)\n// type here...\n";
    static int initialized = 0;
    if (!initialized) { initialized = 1; }

    int tabw = 4;
    /* make window size similar to Windows jpedit: use most of screen area above properties */
    int win_w = g_screen_w > 0 ? g_screen_w - 40 : 360;
    int win_h = g_screen_h > 0 ? (g_screen_h - (g_screen_h/4) - 40) : 600; /* leave room for properties */
    struct nk_rect bounds = nk_rect(20, 20, (float)win_w, (float)win_h);

    if (nk_begin(ctx, "Japanese Editor", bounds,
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE))
    {
        int toolbar_h = 48;
        int status_h = 24;
        int editor_h_calc = win_h - (toolbar_h + status_h + 40);
        if (editor_h_calc < 120) editor_h_calc = 120;

        nk_layout_row_dynamic(ctx, toolbar_h, 3);
        if (nk_button_label(ctx, "Save"))
        {
            nk_tooltip(ctx, "Save not implemented on Android sample");
        }
        if (nk_button_label(ctx, "New")) { /* noop */ }
        if (nk_button_label(ctx, "Clear")) { internal_text[0] = '\0'; }

        int lines = 1, chars = (int)strlen(internal_text);
        for (int i = 0; internal_text[i]; ++i) if (internal_text[i] == '\n') ++lines;
        char status[64]; snprintf(status, sizeof(status), "%d lines, %d ch", lines, chars);
        nk_layout_row_dynamic(ctx, status_h, 1);
        nk_label(ctx, status, NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, editor_h_calc, 1);
        static char expanded[JPEDIT_MAX_TEXT_SIZE];
        static char temp[JPEDIT_MAX_TEXT_SIZE];
        expand_tabs_to_spaces(internal_text, tabw, expanded, sizeof(expanded));
        nk_flags edit_flags = NK_EDIT_FIELD | NK_EDIT_MULTILINE | NK_EDIT_SELECTABLE | NK_EDIT_ALLOW_TAB | NK_EDIT_CLIPBOARD;
        nk_edit_string_zero_terminated(ctx, edit_flags, expanded, sizeof(expanded), nk_filter_default);
        collapse_spaces_to_tabs(expanded, temp, tabw, sizeof(temp));
        if (strcmp(temp, internal_text) != 0)
        {
            strncpy(internal_text, temp, sizeof(internal_text) - 1);
            internal_text[sizeof(internal_text) - 1] = '\0';
        }
    }
    nk_end(ctx);
}

const char* vShader =
"attribute vec4 vPosition;"
"void main() {"
" gl_Position = vPosition;"
"}";

const char* fShader =
"precision mediump float;"
"void main() {"
" gl_FragColor = vec4(1,0,1,1);"
"}";
class Engine
{
    AndroidApp* app;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width, height;
    GLuint hProg;
    NkGLES* nk; // NK
    bool initializedDisplay; // NK
public:
    bool animating;

    void DrawFrame()
    {
        // 初期化が完了していない場合や、ディスプレイが存在しない場合は描画を行わない
        if (!initializedDisplay || !display) return;

        // 頂点データを定義
        const GLfloat vs[] = {
            0.0f, 0.5f, 0.0f,  // 頂点1 (上)
            -0.5f, -0.5f, 0.0f, // 頂点2 (左下)
            0.5f, -0.5f, 0.0f  // 頂点3 (右下)
        };

        // 画面をクリアする色を設定 (R, G, B, A)
        glClearColor(0.2, 0.5, 0.8, 1);
        // 画面をクリア
        glClear(GL_COLOR_BUFFER_BIT);

        // シェーダープログラムを使用
        glUseProgram(hProg);

        // 頂点属性ポインタを設定
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vs);
        // 頂点属性配列を有効化
        glEnableVertexAttribArray(0);

        // 三角形を描画
        glDrawArrays(GL_TRIANGLES, 0, 3);

    /* GUI BEGIN */
    /* Nuklear context */
    nk_context* ctx = &nk->ctx;

    /* Layout: Properties at bottom, editor above it */
    int props_h = g_screen_h > 0 ? g_screen_h / 4 : 200; // properties occupy bottom quarter
    int props_w = g_screen_w > 0 ? g_screen_w - 40 : 360;
    int props_x = 20;
    int props_y = (g_screen_h > 0) ? (g_screen_h - props_h - 20) : 260;

    int editor_h = (g_screen_h > 0) ? (g_screen_h - props_h - 60) : 400; // remaining area minus margins
    int editor_w = g_screen_w > 0 ? g_screen_w - 40 : 360;
    int editor_x = 20;
    int editor_y = 20;

    /* Draw editor first so Properties overlay can be on top */
    jpedit_draw_gui(ctx);

    /* Properties window at bottom */
    if (nk_begin(ctx, "Properties", nk_rect((float)props_x, (float)props_y, (float)props_w, (float)props_h), NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE)) {
        nk_layout_row_dynamic(ctx, 40, 1);
        nk_label(ctx, "Properties", NK_TEXT_LEFT);
        static int prop_enable = 1;
        nk_checkbox_label(ctx, "Enable feature", &prop_enable);
        static int tab_width = 4;
        nk_property_int(ctx, "Tab width:", 1, &tab_width, 8, 1, 1);
    }
    nk_end(ctx);

    /* Render Nuklear */
    nk->Render(NK_ANTI_ALIASING_ON);
    /* GUI END */

        // バッファをスワップして画面に描画
        eglSwapBuffers(display, surface);
    }

    void TermDisplay()
    {
        // ディスプレイが存在する場合のみ処理を行う
        if (display != EGL_NO_DISPLAY)
        {
            // 現在のコンテキストを無効にする
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            // コンテキストが存在する場合は破棄する
            if (context != EGL_NO_CONTEXT)
            {
                eglDestroyContext(display, context);
            }

            // サーフェスが存在する場合は破棄する
            if (surface != EGL_NO_SURFACE)
            {
                eglDestroySurface(display, surface);
            }

            // ディスプレイを終了する
            eglTerminate(display);
        }

        // アニメーションを停止する
        animating = false;

        // ディスプレイ、コンテキスト、サーフェスを無効にする
        display = EGL_NO_DISPLAY;
        context = EGL_NO_CONTEXT;
        surface = EGL_NO_SURFACE;
    }
    static int InitDisplay(Engine* e)
    {
        // EGLの設定属性を定義
        const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT, // ウィンドウサーフェスを使用
            EGL_BLUE_SIZE, 8,                 // 青のサイズを8ビット
            EGL_GREEN_SIZE, 8,                // 緑のサイズを8ビット
            EGL_RED_SIZE, 8,                  // 赤のサイズを8ビット
            EGL_NONE                          // 終端
        };

        // ディスプレイを取得
        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        // ディスプレイを初期化
        eglInitialize(display, 0, 0);
        // OpenGL ES APIをバインド
        eglBindAPI(EGL_OPENGL_ES_API);

        // 使用可能なEGLフレームバッファ構成を選択
        // attribsで指定した属性に一致する構成を取得
        EGLConfig config;
        EGLint numConfigs;
        eglChooseConfig(display, attribs, &config, 1, &numConfigs);
        if (numConfigs == 0)
        {
            LOGW("Unable to find a suitable EGLConfig");
            return -1;
        }

        // ネイティブウィンドウのバッファフォーマットを取得
        // これにより、ANativeWindow_setBuffersGeometryで使用するフォーマットを取得
        EGLint format;
        eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
        // ネイティブウィンドウのバッファのフォーマットを設定
        // これにより、ウィンドウのバッファが適切なフォーマットで設定される
        ANativeWindow_setBuffersGeometry(e->app->window, 0, 0, format);

        // ウィンドウサーフェスを作成
        EGLSurface surface = eglCreateWindowSurface(display, config, e->app->window, NULL);
        // コンテキスト属性を定義
        const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        // コンテキストを作成
        EGLContext context = eglCreateContext(display, config, NULL, contextAttribs);

        // コンテキストを現在のスレッドにバインド
        if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
        {
            LOGW("Unable to eglMakeCurrent");
            return -1;
        }

        // サーフェスの幅と高さを取得
        EGLint w, h;
        eglQuerySurface(display, surface, EGL_WIDTH, &w);
        eglQuerySurface(display, surface, EGL_HEIGHT, &h);

        // エンジンのディスプレイ、コンテキスト、サーフェス、幅、高さを設定
        e->display = display;
        e->context = context;
        e->surface = surface;
        e->width = w;
        e->height = h;

        // store global screen size for layout
        g_screen_w = w;
        g_screen_h = h;

        // 描画のヒントを設定
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
        // ビューポートを設定
        glViewport(0, 0, w, h);
        // シェーダープログラムを作成
        e->hProg = generate_shader_program(vShader, fShader);
        // Nuklearの初期化
        e->nk = new NkGLES(e->display, e->surface, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY); // NK
        // 初期化が完了したことを設定
        e->initializedDisplay = true; // NK

        return 0;
    }


    static void HandleCmd(AndroidApp* app, int32_t cmd)
    {
        Engine* e = (Engine*)app->userData;
        switch (cmd)
        {
        case APP_CMD_INIT_WINDOW:
            if (e->app->window)
            {
                InitDisplay(e);
                e->DrawFrame();
            } break;
        case APP_CMD_TERM_WINDOW:
            e->TermDisplay();
            break;
        case APP_CMD_GAINED_FOCUS:
            break;
        case APP_CMD_LOST_FOCUS:
            //e->animating = false;
            e->DrawFrame();
            break;
        }
    }

    static int32_t HandleInput(AndroidApp* app, AInputEvent* e)
    { // NK
        Engine* eng = (Engine*)app->userData;
        switch (AInputEvent_getType(e))
        {
        case AINPUT_EVENT_TYPE_MOTION: { // Touch event
            float x = AMotionEvent_getX(e, 0);
            float y = AMotionEvent_getY(e, 0);
            nk_context* ctx = &eng->nk->ctx;
            nk_input_begin(ctx);
            switch (AMotionEvent_getAction(e) & AMOTION_EVENT_ACTION_MASK)
            {
            case AMOTION_EVENT_ACTION_DOWN: // WM_LBUTTONDOWN
                ctx->input.mouse.pos = nk_vec2(x, y);
                nk_input_button(ctx, NK_BUTTON_LEFT, x, y, true);
                break;
            case AMOTION_EVENT_ACTION_UP: // WM_LBUTTONUP
                ctx->input.mouse.pos = nk_vec2(0, 0);
                nk_input_button(ctx, NK_BUTTON_LEFT, x, y, false);
                break;
            case AMOTION_EVENT_ACTION_MOVE: // WM_MOUSEMOVE
                nk_input_motion(ctx, x, y);
                break;
            }
            nk_input_end(ctx);
        } return 1;
        case AINPUT_EVENT_TYPE_KEY:
            break;
        }
        return 0;
    }

    Engine(AndroidApp* state_)
    {
        memset(this, 0, sizeof(*this));
        state_->userData = this;
        state_->onAppCmd = Engine::HandleCmd;
        state_->onInputEvent = Engine::HandleInput; // NK
        app = state_;
        animating = true;
    }
    ~Engine() { delete nk; } // NK
};
class engine /* placeholder for alternative engine struct */ {};

#ifdef __cplusplus
extern "C" {
#endif
void android_main(struct android_app* state)
{
    Engine e(state);
    struct android_poll_source* s;
    int events;

    while (true)
    {
        while (ALooper_pollAll(e.animating ? 0 : -1, NULL, &events, (void**)&s) >= 0)
        {
            if (s) s->process(state, s);
            if (state->destroyRequested)
            {
                e.TermDisplay();
                return;
            }
        }

        if (e.animating) e.DrawFrame();
    }
}

#ifdef __cplusplus
}
#endif
#endif /* __ANDROID__ */

