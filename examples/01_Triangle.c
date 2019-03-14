#include <stdio.h>

#define GLBIND_IMPLEMENTATION
#include "../glbind.h"

void Render(GLBapi* pGL)
{
    pGL->glClearColor(0.2f, 0.5f, 0.8f, 0);
    pGL->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    pGL->glBegin(GL_TRIANGLES);
    {
        pGL->glColor3f(1, 0, 0);
        pGL->glVertex2f( 0.0f, +0.5f);
        pGL->glColor3f(0, 1, 0);
        pGL->glVertex2f(-0.5f, -0.5f);
        pGL->glColor3f(0, 0, 1);
        pGL->glVertex2f(+0.5f, -0.5f);
        
    }
    pGL->glEnd();
}

int main(int argc, char** argv)
{
    GLBapi gl;
    GLenum result = glbInit(&gl, NULL);
    if (result != GL_NO_ERROR) {
        printf("Failed to initialize glbind.\n");
        return result;
    }

    glbBindAPI(&gl);    /* Bind the API to global scope. */

    /* Create the window and show something on the screen. */
    {
#if defined(GLBIND_GLX)
        XSetWindowAttributes attr;
        Display* pDisplay;
        Window windowX11;
        XVisualInfo* pVisualInfo = glbGetFBVisualInfo();
        Atom g_WM_DELETE_WINDOW = 0;

        pDisplay = glbGetDisplay();
        pVisualInfo = glbGetFBVisualInfo();
        g_WM_DELETE_WINDOW = XInternAtom(pDisplay, "WM_DELETE_WINDOW", False);

        attr.colormap   = glbGetColormap();
        attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | FocusChangeMask | VisibilityChangeMask;

        windowX11 = XCreateWindow(pDisplay, XRootWindow(pDisplay, pVisualInfo->screen), 0, 0, 640, 480, 0, pVisualInfo->depth, InputOutput, pVisualInfo->visual, CWColormap | CWEventMask, &attr);
        if (windowX11 == 0) {
            printf("Failed to create X11 window.\n");
            return -1;
        }

        XSetWMProtocols(pDisplay, windowX11, &g_WM_DELETE_WINDOW, 1);
        XStoreName(pDisplay, windowX11, "glbind 01_Triangle");
        XMapRaised(pDisplay, windowX11);    /* <-- Show the window. */

        gl.glXMakeCurrent(pDisplay, windowX11, glbGetRC());

        /* Loop. */
        for (;;) {
            if (XPending(pDisplay) > 0) {
                XEvent x11Event;
                XNextEvent(pDisplay, &x11Event);

                if (x11Event.type == ClientMessage) {
                    if ((Atom)x11Event.xclient.data.l[0] == g_WM_DELETE_WINDOW) {
                        return 0;   /* Received a quit message. */
                    }
                };

                /* Handle events here. */
            } else {
                Render(&gl);
                gl.glXSwapBuffers(pDisplay, windowX11);
            }
        }

        /* Shutdown. */
        XDestroyWindow(pDisplay, windowX11);
#endif
    }

    glbUninit();

    (void)argc;
    (void)argv;
    return 0;
}