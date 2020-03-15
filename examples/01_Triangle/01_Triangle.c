#include <stdio.h>

#define GLBIND_IMPLEMENTATION
#include "../../glbind.h"

void Render()
{
    glClearColor(0.2f, 0.5f, 0.8f, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
    {
        glColor3f(1, 0, 0);
        glVertex2f( 0.0f, +0.5f);
        glColor3f(0, 1, 0);
        glVertex2f(-0.5f, -0.5f);
        glColor3f(0, 0, 1);
        glVertex2f(+0.5f, -0.5f);
    }
    glEnd();
}

#if defined(GLBIND_WGL)
static LRESULT DefaultWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;

        case WM_SIZE:
        {
            glViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
        }

        default: break;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}
#endif

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
#if defined(GLBIND_WGL)
        WNDCLASSEXA wc;
        DWORD dwExStyle = 0;
        DWORD dwStyle = WS_OVERLAPPEDWINDOW;
        HWND hWnd;
        
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize        = sizeof(wc);
        wc.cbWndExtra    = sizeof(void*);
        wc.lpfnWndProc   = (WNDPROC)DefaultWindowProcWin32;
        wc.lpszClassName = "GLBIND_01_Triangle";
        wc.hCursor       = LoadCursorA(NULL, MAKEINTRESOURCEA(32512));
        wc.style         = CS_OWNDC | CS_DBLCLKS;
        if (!RegisterClassExA(&wc)) {
            printf("Failed to register window class.\n");
            return -1;
        }
        
        hWnd = CreateWindowExA(dwExStyle, "GLBIND_01_Triangle", "glbind 01_Triangle", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, NULL, NULL);
        if (hWnd == NULL) {
            printf("Failed to create Win32 window.\n");
            return -1;
        }

        SetPixelFormat(GetDC(hWnd), glbGetPixelFormat(), glbGetPFD());

        ShowWindow(hWnd, SW_SHOWNORMAL);

        gl.wglMakeCurrent(GetDC(hWnd), glbGetRC()); /* Using the local API to avoid the need to link to OpenGL32.dll. */

        for (;;) {
            MSG msg;
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    return (int)msg.wParam;
                }

                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }

            Render();
            SwapBuffers(GetDC(hWnd));
        }

        DestroyWindow(hWnd);
#endif

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
                Render();
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