/*
 * main_cocoa_gl.c - Cocoa window + NSOpenGLContext + CVDisplayLink in plain C.
 *
 * tcc cannot compile Objective-C, but every Objective-C message send is just a
 * call to objc_msgSend() from libobjc, a C API.  This file drives Cocoa that
 * way, so the same structure as an Objective-C renderer
 *     NSApplication / NSWindow / NSOpenGLContext / CVDisplayLink callback
 * builds with tcc (and with clang as plain C).
 *
 * Build: sh build_opengl_mac.sh cocoa [run]
 *
 * Rules for calling objc_msgSend from C (x86-64):
 *  - always cast it to the exact function type of the method
 *    (id/SEL first, then the arguments) - it is not a variadic function;
 *  - a method returning a struct bigger than 16 bytes (NSRect = 32 bytes)
 *    must go through objc_msgSend_stret instead;
 *  - BOOL is signed char.
 *
 * Keys: ESC quit / Up, Down rotation speed.  Option: -frames N quit after N.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <OpenGL/OpenGL.h>   /* CGL: CGLLockContext, CGLFlushDrawable */
#include <OpenGL/gl.h>

/* ---- the few Cocoa / CoreVideo declarations this file needs ------------- */

typedef struct { double x, y, w, h; } NSRect;   /* = CGRect */
typedef unsigned long NSUInteger;

/* CoreVideo/CVDisplayLink.h pulls in CoreGraphics headers that tcc cannot
   parse (static const initialised from another const), so declare the C API
   used here directly. */
typedef struct __CVDisplayLink *CVDisplayLinkRef;
typedef int CVReturn;
typedef unsigned long long CVOptionFlags;
typedef struct CVTimeStamp CVTimeStamp;          /* only used by pointer */
typedef CVReturn (*CVDisplayLinkOutputCallback)(CVDisplayLinkRef,
    const CVTimeStamp *, const CVTimeStamp *, CVOptionFlags, CVOptionFlags *,
    void *);
CVReturn CVDisplayLinkCreateWithActiveCGDisplays(CVDisplayLinkRef *link);
CVReturn CVDisplayLinkSetOutputCallback(CVDisplayLinkRef link,
    CVDisplayLinkOutputCallback cb, void *user);
CVReturn CVDisplayLinkStart(CVDisplayLinkRef link);
CVReturn CVDisplayLinkStop(CVDisplayLinkRef link);
void CVDisplayLinkRelease(CVDisplayLinkRef link);

enum {
    NSWindowStyleMaskTitled = 1, NSWindowStyleMaskClosable = 2,
    NSWindowStyleMaskMiniaturizable = 4, NSWindowStyleMaskResizable = 8,
    NSBackingStoreBuffered = 2,
    NSApplicationActivationPolicyRegular = 0,
    NSOpenGLPFADoubleBuffer = 5, NSOpenGLPFAColorSize = 8,
    NSOpenGLPFADepthSize = 12, NSOpenGLPFAAccelerated = 73,
    NSOpenGLCPSwapInterval = 222
};

/* ---- objc_msgSend helpers ------------------------------------------------ */

#define CLS(name) ((id)objc_getClass(name))
#define SEL_(name) sel_registerName(name)

static id msg(id obj, const char *sel)
{
    return ((id (*)(id, SEL))objc_msgSend)(obj, SEL_(sel));
}
static id msg_id(id obj, const char *sel, id a)
{
    return ((id (*)(id, SEL, id))objc_msgSend)(obj, SEL_(sel), a);
}
static NSRect msg_rect(id obj, const char *sel)  /* struct > 16 bytes: _stret */
{
    NSRect r;
    ((void (*)(NSRect *, id, SEL))objc_msgSend_stret)(&r, obj, SEL_(sel));
    return r;
}
static id nsstring(const char *s)
{
    return ((id (*)(id, SEL, const char *))objc_msgSend)(
        CLS("NSString"), SEL_("stringWithUTF8String:"), s);
}

/* ---- renderer state ------------------------------------------------------ */

static id gl_context;             /* NSOpenGLContext */
static CGLContextObj cgl;         /* its CGL context (lock + flush from C) */
static id window;                 /* NSWindow */
static CVDisplayLinkRef display_link;
static volatile int view_w = 800, view_h = 600, resized = 1;
static float angle, speed = 0.4f;
static int frames, max_frames;

/* ---- drawing (same scene as main_gl.c / main_glut.c) --------------------- */

static void draw_scene(void)
{
    static const float c[6][3] = { {0,1,1}, {1,1,0}, {1,0,1},
                                   {0,0,1}, {0,1,0}, {1,0,0} };
    static const float v[6][4][3] = {
        {{-1,-1,-1},{-1,-1, 1},{-1, 1, 1},{-1, 1,-1}},
        {{-1,-1,-1},{-1, 1,-1},{ 1, 1,-1},{ 1,-1,-1}},
        {{-1,-1,-1},{ 1,-1,-1},{ 1,-1, 1},{-1,-1, 1}},
        {{-1,-1, 1},{ 1,-1, 1},{ 1, 1, 1},{-1, 1, 1}},
        {{-1, 1,-1},{-1, 1, 1},{ 1, 1, 1},{ 1, 1,-1}},
        {{ 1,-1,-1},{ 1, 1,-1},{ 1, 1, 1},{ 1,-1, 1}} };
    int i, j;
    double aspect;

    if (resized) {
        resized = 0;
        glViewport(0, 0, view_w, view_h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        aspect = (double)view_w / (view_h ? view_h : 1);
        glFrustum(-0.1 * aspect, 0.1 * aspect, -0.1, 0.1, 0.25, 100.0);
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);
    }
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0, 0, -6);
    glRotatef(30 + angle, 1, 1, 1);
    glLineWidth(5);
    glBegin(GL_LINES);
    glColor3f(0.6f, 0, 0); glVertex3f(0, 0, 0); glVertex3f(3, 0, 0);
    glColor3f(0, 0.6f, 0); glVertex3f(0, 0, 0); glVertex3f(0, 3, 0);
    glColor3f(0, 0, 0.6f); glVertex3f(0, 0, 0); glVertex3f(0, 0, 3);
    glEnd();
    for (i = 0; i < 6; i++) {
        glBegin(GL_QUADS);
        glColor3fv(c[i]);
        for (j = 0; j < 4; j++)
            glVertex3fv(v[i][j]);
        glEnd();
    }
    angle += speed;
}

/* CVDisplayLink calls this on its own thread at the display refresh rate.
   The GL context is shared with the main thread (resize), hence the lock. */
static CVReturn display_link_callback(CVDisplayLinkRef link,
    const CVTimeStamp *now, const CVTimeStamp *out, CVOptionFlags in_flags,
    CVOptionFlags *out_flags, void *user)
{
    (void)link, (void)now, (void)out, (void)in_flags, (void)out_flags, (void)user;
    CGLLockContext(cgl);
    CGLSetCurrentContext(cgl);
    draw_scene();
    if (max_frames && frames + 1 == max_frames) {
        /* automated check: the cube covers the centre of the view */
        unsigned char px[4];
        glReadPixels(view_w / 2, view_h / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        printf("center pixel: %d %d %d\n", px[0], px[1], px[2]);
    }
    CGLFlushDrawable(cgl);
    CGLUnlockContext(cgl);

    if (max_frames && ++frames == max_frames) {
        printf("GL_VERSION: %s\n", (const char *)glGetString(GL_VERSION));
        printf("GL_RENDERER: %s\n", (const char *)glGetString(GL_RENDERER));
        printf("frames: %d (CVDisplayLink)\n", frames);
        fflush(stdout);
        /* Cocoa must be told to quit from the main thread */
        ((void (*)(id, SEL, SEL, id, signed char))objc_msgSend)(
            msg(CLS("NSApplication"), "sharedApplication"),
            SEL_("performSelectorOnMainThread:withObject:waitUntilDone:"),
            SEL_("terminate:"), (id)0, 0);
    }
    return 0;
}

/* ---- Objective-C classes defined from C --------------------------------- */

/* NSApplicationDelegate: quit when the window is closed */
static signed char app_should_terminate(id self, SEL cmd, id app)
{
    (void)self, (void)cmd, (void)app;
    return 1;
}

static void app_will_terminate(id self, SEL cmd, id note)
{
    (void)self, (void)cmd, (void)note;
    CVDisplayLinkStop(display_link);
    CVDisplayLinkRelease(display_link);
}

/* NSWindowDelegate: the drawable must be updated on the main thread */
static void window_did_resize(id self, SEL cmd, id note)
{
    NSRect r;
    (void)self, (void)cmd, (void)note;
    r = msg_rect(msg(window, "contentView"), "frame");
    CGLLockContext(cgl);
    msg(gl_context, "update");
    view_w = (int)r.w;
    view_h = (int)r.h;
    resized = 1;
    CGLUnlockContext(cgl);
}

/* NSView subclass: accept key events (ESC / arrows) */
static signed char view_accepts_first_responder(id self, SEL cmd)
{
    (void)self, (void)cmd;
    return 1;
}

static void view_key_down(id self, SEL cmd, id event)
{
    unsigned short key;
    (void)self, (void)cmd;
    key = ((unsigned short (*)(id, SEL))objc_msgSend)(event, SEL_("keyCode"));
    switch (key) {
    case 53: /* ESC */
        msg_id(msg(CLS("NSApplication"), "sharedApplication"),
               "terminate:", (id)0);
        break;
    case 126: speed += 0.1f; break;   /* up */
    case 125: speed -= 0.1f; break;   /* down */
    }
}

static Class make_class(const char *name, const char *super)
{
    Class c = objc_allocateClassPair(objc_getClass(super), name, 0);
    return c;
}

/* ---- main ---------------------------------------------------------------- */

int main(int argc, char **argv)
{
    Class delegate_class, view_class;
    id app, delegate, view, pixel_format;
    NSRect frame = { 0, 0, 800, 600 };
    unsigned int attrs[] = {
        NSOpenGLPFADoubleBuffer, NSOpenGLPFAColorSize, 24,
        NSOpenGLPFADepthSize, 16, NSOpenGLPFAAccelerated, 0 };
    int swap_interval = 1, i;

    for (i = 1; i < argc; i++)
        if (strcmp(argv[i], "-frames") == 0 && i + 1 < argc)
            max_frames = atoi(argv[++i]);

    msg(msg(CLS("NSAutoreleasePool"), "alloc"), "init");
    app = msg(CLS("NSApplication"), "sharedApplication");
    ((void (*)(id, SEL, long))objc_msgSend)(app,
        SEL_("setActivationPolicy:"), NSApplicationActivationPolicyRegular);

    /* app + window delegate */
    delegate_class = make_class("TppAppDelegate", "NSObject");
    class_addMethod(delegate_class,
        SEL_("applicationShouldTerminateAfterLastWindowClosed:"),
        (IMP)app_should_terminate, "c@:@");
    class_addMethod(delegate_class, SEL_("applicationWillTerminate:"),
        (IMP)app_will_terminate, "v@:@");
    class_addMethod(delegate_class, SEL_("windowDidResize:"),
        (IMP)window_did_resize, "v@:@");
    objc_registerClassPair(delegate_class);
    delegate = msg(msg((id)delegate_class, "alloc"), "init");
    msg_id(app, "setDelegate:", delegate);

    /* content view that takes the keyboard */
    view_class = make_class("TppGLView", "NSView");
    class_addMethod(view_class, SEL_("acceptsFirstResponder"),
        (IMP)view_accepts_first_responder, "c@:");
    class_addMethod(view_class, SEL_("keyDown:"), (IMP)view_key_down, "v@:@");
    objc_registerClassPair(view_class);

    /* NSWindow: NSRect is passed by value (32 bytes, on the stack) */
    window = ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, signed char))
        objc_msgSend)(msg(CLS("NSWindow"), "alloc"),
        SEL_("initWithContentRect:styleMask:backing:defer:"), frame,
        NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
            | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable,
        NSBackingStoreBuffered, 0);
    view = ((id (*)(id, SEL, NSRect))objc_msgSend)(
        msg((id)view_class, "alloc"), SEL_("initWithFrame:"), frame);
    msg_id(window, "setContentView:", view);
    msg_id(window, "setTitle:", nsstring("tpp: Cocoa + CVDisplayLink (C only)"));
    msg_id(window, "setDelegate:", delegate);
    msg(window, "center");
    msg_id(window, "makeKeyAndOrderFront:", (id)0);
    msg_id(window, "makeFirstResponder:", view);

    /* NSOpenGLContext bound to the view, vsync on */
    pixel_format = ((id (*)(id, SEL, const unsigned int *))objc_msgSend)(
        msg(CLS("NSOpenGLPixelFormat"), "alloc"),
        SEL_("initWithAttributes:"), attrs);
    gl_context = ((id (*)(id, SEL, id, id))objc_msgSend)(
        msg(CLS("NSOpenGLContext"), "alloc"),
        SEL_("initWithFormat:shareContext:"), pixel_format, (id)0);
    msg_id(gl_context, "setView:", view);
    ((void (*)(id, SEL, const int *, long))objc_msgSend)(gl_context,
        SEL_("setValues:forParameter:"), &swap_interval, NSOpenGLCPSwapInterval);
    cgl = ((CGLContextObj (*)(id, SEL))objc_msgSend)(gl_context,
        SEL_("CGLContextObj"));

    /* frame driver: CVDisplayLink (vsync callback on its own thread) */
    CVDisplayLinkCreateWithActiveCGDisplays(&display_link);
    CVDisplayLinkSetOutputCallback(display_link, display_link_callback, 0);
    CVDisplayLinkStart(display_link);

    ((void (*)(id, SEL, signed char))objc_msgSend)(app,
        SEL_("activateIgnoringOtherApps:"), 1);
    msg(app, "run");      /* Cocoa event loop; returns only via terminate: */
    return 0;
}
