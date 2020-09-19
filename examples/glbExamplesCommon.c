#define GLBIND_IMPLEMENTATION
#include "../glbind.h"

#include <stdio.h>

typedef struct GLBexample GLBexample;
typedef GLenum (* GLBOnInitProc)  (GLBexample* pExample);
typedef void   (* GLBOnUninitProc)(GLBexample* pExample);
typedef void   (* GLBOnDrawProc)  (GLBexample* pExample);
typedef void   (* GLBOnSizeProc)  (GLBexample* pExample, GLsizei sizeX, GLsizei sizeY);

typedef struct
{
    GLBconfig* pGLBConfig;
    const char* pWindowTitle;
    void* pUserData;
    GLBOnInitProc   onInit;
    GLBOnUninitProc onUninit;
    GLBOnDrawProc   onDraw;
    GLBOnSizeProc   onSize;
} GLBexampleconfig;

struct GLBexample
{
    GLBapi gl;
    GLBexampleconfig config;
#if defined(GLBIND_WGL)
    struct
    {
        HWND hWnd;
    } win32;
#else
    struct
    {
        Display* pDisplay;
        Window window;
    } x11;
#endif
};

#if defined(GLBIND_WGL)
static LRESULT glbExample_DefaultWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GLBexample* pExample = (GLBexample*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
    if (pExample != NULL) {
        switch (msg)
        {
            case WM_CLOSE:
            {
                PostQuitMessage(0);
            } break;

            case WM_SIZE:
            {
                if (pExample->config.onSize) {
                    pExample->config.onSize(pExample, LOWORD(lParam), HIWORD(lParam));
                }
            }

            default: break;
        }
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}
#endif

static GLenum glbExampleInitWindow(GLBexample* pExample, GLsizei sizeX, GLsizei sizeY, const char* pTitle)
{
    if (pExample == NULL) {
        return GL_INVALID_VALUE;
    }

#if defined(GLBIND_WGL)
    {
        WNDCLASSEXA wc;
        DWORD dwExStyle = 0;
        DWORD dwStyle = WS_OVERLAPPEDWINDOW;
        
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize        = sizeof(wc);
        wc.cbWndExtra    = sizeof(void*);
        wc.lpfnWndProc   = (WNDPROC)glbExample_DefaultWindowProcWin32;
        wc.lpszClassName = "GLBIND_Example";
        wc.hCursor       = LoadCursorA(NULL, MAKEINTRESOURCEA(32512));
        wc.style         = CS_OWNDC | CS_DBLCLKS;
        if (!RegisterClassExA(&wc)) {
            printf("Failed to register window class.\n");
            return -1;
        }
        
        pExample->win32.hWnd = CreateWindowExA(dwExStyle, "GLBIND_Example", pTitle, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, sizeX, sizeY, NULL, NULL, NULL, NULL);
        if (pExample->win32.hWnd == NULL) {
            printf("Failed to create Win32 window.\n");
            return -1;
        }

        SetWindowLongPtrA(pExample->win32.hWnd, GWLP_USERDATA, (LONG_PTR)pExample);

        SetPixelFormat(GetDC(pExample->win32.hWnd), glbGetPixelFormat(), glbGetPFD());
    }
#else
    {
        XSetWindowAttributes attr;
        XVisualInfo* pVisualInfo = glbGetFBVisualInfo();
        Atom g_WM_DELETE_WINDOW = 0;

        pExample->x11.pDisplay = glbGetDisplay();
        pVisualInfo = glbGetFBVisualInfo();
        g_WM_DELETE_WINDOW = XInternAtom(pExample->x11.pDisplay, "WM_DELETE_WINDOW", False);

        attr.colormap   = glbGetColormap();
        attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | FocusChangeMask | VisibilityChangeMask;

        pExample->x11.window = XCreateWindow(pExample->x11.pDisplay, XRootWindow(pExample->x11.pDisplay, pVisualInfo->screen), 0, 0, sizeX, sizeY, 0, pVisualInfo->depth, InputOutput, pVisualInfo->visual, CWColormap | CWEventMask, &attr);
        if (pExample->x11.window == 0) {
            printf("Failed to create X11 window.\n");
            return -1;
        }

        XSetWMProtocols(pExample->x11.pDisplay, pExample->x11.window, &g_WM_DELETE_WINDOW, 1);
        XStoreName(pExample->x11.pDisplay, pExample->x11.window, pTitle);
    }
#endif

    return GL_NO_ERROR;
}

static void glbExampleUninitWindow(GLBexample* pExample)
{
    if (pExample == NULL) {
        return;
    }

#if defined(GLBIND_WGL)
    DestroyWindow(pExample->win32.hWnd);
#else
    XDestroyWindow(pExample->x11.pDisplay, pExample->x11.window);
#endif
}

static void glbExampleShowWindowAndMakeCurrent(GLBexample* pExample)
{
    if (pExample == NULL) {
        return;
    }

#if defined(GLBIND_WGL)
    ShowWindow(pExample->win32.hWnd, SW_SHOWNORMAL);
    pExample->gl.wglMakeCurrent(GetDC(pExample->win32.hWnd), glbGetRC()); /* Using the local API to avoid the need to link to OpenGL32.dll. */
#else
    XMapRaised(pExample->x11.pDisplay, pExample.x11.window);    /* <-- Show the window. */
    pExample->gl.glXMakeCurrent(pExample->x11.pDisplay, pExample->x11.window, glbGetRC());
#endif
}

GLenum glbExampleInit(GLBexample* pExample, GLBexampleconfig* pConfig)
{
    GLenum result;

    if (pExample == NULL || pConfig == NULL) {
        return GL_INVALID_VALUE;
    }

    glbZeroObject(pExample);
    pExample->config = *pConfig;

    result = glbInit(&pExample->gl, pConfig->pGLBConfig);
    if (result != GL_NO_ERROR) {
        return result;
    }

    /* Create the window. */
    result = glbExampleInitWindow(pExample, 640, 480, (pConfig->pWindowTitle != NULL) ? pConfig->pWindowTitle : "glbind");
    if (result != GL_NO_ERROR) {
        glbUninit();
        return result;
    }

    /* Now show the window. */
    glbExampleShowWindowAndMakeCurrent(pExample);

    /* We need to wait for the context to be made current before calling the init callback to ensure resources can be loaded without an explicit MakeCurrent(). */
    if (pExample->config.onInit) {
        result = pExample->config.onInit(pExample);
        if (result != GL_NO_ERROR) {
            glbExampleUninitWindow(pExample);
            glbUninit();
            return result;
        }
    }

    return GL_NO_ERROR;
}

void glbExampleUninit(GLBexample* pExample)
{
    if (pExample == NULL) {
        return;
    }

    glbExampleUninitWindow(pExample);
    glbUninit(&pExample->gl);
}

int glbExampleRun(GLBexample* pExample)
{
#if defined(GLBIND_WGL)
    for (;;) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return (int)msg.wParam;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (pExample->config.onDraw) {
            pExample->config.onDraw(pExample);
        }

        SwapBuffers(GetDC(pExample->win32.hWnd));
    }
#else
    for (;;) {
        if (XPending(pDisplay) > 0) {
            XEvent x11Event;
            XNextEvent(pExample->x11.pDisplay, &x11Event);

            if (x11Event.type == ClientMessage) {
                if ((Atom)x11Event.xclient.data.l[0] == g_WM_DELETE_WINDOW) {
                    return 0;   /* Received a quit message. */
                }
            };

            /* Handle events here. */
        } else {
            pExample->gl.glXSwapBuffers(pExample->x11.pDisplay, pExample->x11.window);
        }
    }
#endif
}
