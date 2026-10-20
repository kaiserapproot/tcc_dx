/* X11 window + GLX context from C++ (tpp, Linux).
   Same Xlib calls as amateras platform/linux (XOpenDisplay, XSelectInput,
   WM_DELETE_WINDOW, XNextEvent loop) plus a GLX context.  Non-interactive:
   clears to a known colour, reads a pixel back, then closes the window.
   Exit 0 = OK, 77 = no display (skip), other = failure. */
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>

class GlWindow {
public:
    Display *dpy;
    Window win;
    GLXContext ctx;
    Atom wm_delete;
    int width, height;

    GlWindow(int w, int h) : dpy(0), win(0), ctx(0), width(w), height(h) {}
    ~GlWindow() { close(); }

    int open(const char *title)
    {
        int attr[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8,
                       GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, None };
        XVisualInfo *vi;
        XSetWindowAttributes swa;
        int screen;

        dpy = XOpenDisplay(NULL);
        if (!dpy)
            return 77;
        screen = DefaultScreen(dpy);
        vi = glXChooseVisual(dpy, screen, attr);
        if (!vi)
            return 2;
        memset(&swa, 0, sizeof swa);
        swa.colormap = XCreateColormap(dpy, RootWindow(dpy, vi->screen),
                                       vi->visual, AllocNone);
        swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
        win = XCreateWindow(dpy, RootWindow(dpy, vi->screen), 0, 0,
                            width, height, 0, vi->depth, InputOutput,
                            vi->visual, CWColormap | CWEventMask, &swa);
        XStoreName(dpy, win, title);
        wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy, win, &wm_delete, 1);
        ctx = glXCreateContext(dpy, vi, NULL, True);
        XFree(vi);
        if (!ctx)
            return 3;
        XMapWindow(dpy, win);
        wait_mapped();
        if (!glXMakeCurrent(dpy, win, ctx))
            return 4;
        return 0;
    }

    void wait_mapped()
    {
        XEvent ev;
        for (;;) {
            XNextEvent(dpy, &ev);
            if (ev.type == MapNotify)
                break;
        }
    }

    void close()
    {
        if (ctx) {
            glXMakeCurrent(dpy, None, NULL);
            glXDestroyContext(dpy, ctx);
            ctx = 0;
        }
        if (win) {
            XDestroyWindow(dpy, win);
            win = 0;
        }
        if (dpy) {
            XCloseDisplay(dpy);
            dpy = 0;
        }
    }

    virtual void draw()
    {
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
};

class TriWindow : public GlWindow {
public:
    TriWindow(int w, int h) : GlWindow(w, h) {}
    virtual void draw()
    {
        GlWindow::draw();
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(-1.0f, -1.0f);
        glVertex2f(3.0f, -1.0f);
        glVertex2f(-1.0f, 3.0f);
        glEnd();
    }
};

int main()
{
    TriWindow tw(64, 48);
    GlWindow *w = &tw;
    unsigned char px[4];
    int rc;

    rc = w->open("tpp x11 glx");
    if (rc == 77) {
        printf("x11_glx: no display, skipped\n");
        return 77;
    }
    if (rc)
        return rc;
    printf("GL_VENDOR=%s\nGL_RENDERER=%s\nGL_VERSION=%s\n",
           (const char *)glGetString(GL_VENDOR),
           (const char *)glGetString(GL_RENDERER),
           (const char *)glGetString(GL_VERSION));
    w->draw();              /* virtual: TriWindow covers the viewport red */
    glFinish();
    memset(px, 0, sizeof px);
    glReadBuffer(GL_BACK);
    glReadPixels(10, 10, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glXSwapBuffers(w->dpy, w->win);
    printf("pixel=%d,%d,%d\n", px[0], px[1], px[2]);
    if (px[0] < 200 || px[1] > 50 || px[2] > 50)
        return 5;
    w->close();
    return 0;
}
