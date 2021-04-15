/*
OpenGL API loader. Choice of public domain or MIT-0. See license statements at the end of this file.
glbind - v<<opengl_version>>.<<revision>> - <<date>>

David Reid - davidreidsoftware@gmail.com
*/

#ifndef GLBIND_H
#define GLBIND_H

#ifdef __cplusplus
extern "C" {
#endif

/* For platform detection, I'm just assuming GLX if it's not Win32. Happy to look at making this more flexible, especially when it comes to GLES. */
#if defined(_WIN32)
    #define GLBIND_WGL
#else
    #define GLBIND_GLX
#endif

/*
The official OpenGL headers have a dependency on a header called khrplatform.h. From what I can see it's mainly just for sized types. Since glbind is a
single header, and that we can't just copy-and-paste the contents of khrplatform.h due to licensing, we need to do our own sized type declarations.
*/
#ifndef __khrplatform_h_
#include <stddef.h> /* For size_t. */
#ifdef _MSC_VER
    #if defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wlanguage-extension-token"
        #pragma GCC diagnostic ignored "-Wc++11-long-long"
    #endif
    typedef   signed __int8  khronos_int8_t;
    typedef unsigned __int8  khronos_uint8_t;
    typedef   signed __int16 khronos_int16_t;
    typedef unsigned __int16 khronos_uint16_t;
    typedef   signed __int32 khronos_int32_t;
    typedef unsigned __int32 khronos_uint32_t;
    typedef   signed __int64 khronos_int64_t;
    typedef unsigned __int64 khronos_uint64_t;
    #if defined(__clang__)
        #pragma GCC diagnostic pop
    #endif
#else
    #define MA_HAS_STDINT
    #include <stdint.h>
    typedef int8_t   khronos_int8_t;
    typedef uint8_t  khronos_uint8_t;
    typedef int16_t  khronos_int16_t;
    typedef uint16_t khronos_uint16_t;
    typedef int32_t  khronos_int32_t;
    typedef uint32_t khronos_uint32_t;
    typedef int64_t  khronos_int64_t;
    typedef uint64_t khronos_uint64_t;
#endif

#ifdef MA_HAS_STDINT
    typedef uintptr_t khronos_uintptr_t;
    typedef intptr_t  khronos_intptr_t;
    typedef uintptr_t khronos_usize_t;
    typedef intptr_t  khronos_ssize_t;
#else
    #if defined(_WIN32)
        #if defined(_WIN64)
            typedef khronos_uint64_t khronos_uintptr_t;
            typedef khronos_int64_t  khronos_intptr_t;
            typedef khronos_uint64_t khronos_usize_t;
            typedef khronos_int64_t  khronos_ssize_t;
        #else
            typedef khronos_uint32_t khronos_uintptr_t;
            typedef khronos_int32_t  khronos_intptr_t;
            typedef khronos_uint32_t khronos_usize_t;
            typedef khronos_int32_t  khronos_ssize_t;
        #endif
    #elif defined(__GNUC__)
        #if defined(__LP64__)
            typedef khronos_uint64_t khronos_uintptr_t;
            typedef khronos_int64_t  khronos_intptr_t;
            typedef khronos_uint64_t khronos_usize_t;
            typedef khronos_int64_t  khronos_ssize_t;
        #else
            typedef khronos_uint32_t khronos_uintptr_t;
            typedef khronos_int32_t  khronos_intptr_t;
            typedef khronos_uint32_t khronos_usize_t;
            typedef khronos_int32_t  khronos_ssize_t;
        #endif
    #else
        typedef khronos_uint64_t khronos_uintptr_t;
        typedef khronos_int64_t  khronos_intptr_t;
        typedef khronos_uint64_t khronos_usize_t;   /* Fallback. */
        typedef khronos_int64_t  khronos_ssize_t;
    #endif
#endif
typedef float khronos_float_t;
#endif  /* __khrplatform_h_ */

/* Platform headers. */
#if defined(GLBIND_WGL)
#include <windows.h>    /* Can we remove this dependency? */
#endif
#if defined(GLBIND_GLX)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

/*
The official OpenGL headers have traditionally defined their APIs with APIENTRY, APIENTRYP and GLAPI. I'm including these just in case
some program wants to use them.
*/
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif



/*<<opengl_main>>*/

/*<<opengl_funcpointers_decl_global>>*/

typedef struct
{
/*<<opengl_funcpointers_decl_global:4>>*/
} GLBapi;

typedef struct
{
    GLboolean singleBuffered;
#if defined(GLBIND_WGL)
    HWND hWnd;
#endif
#if defined(GLBIND_GLX)
    Display* pDisplay;
#endif
} GLBconfig;

/*
Initializes a config object which can later be passed to glbInit() to configure the rendering context that's created by glbInit().
*/
GLBconfig glbConfigInit();

/*
Initializes glbind and attempts to load APIs statically.

pAPI is optional. On output it will contain pointers to all OpenGL APIs found by the loader.

This will initialize a dummy rendering context and make it current. It will also bind API's to global scope. If you want to load
APIs based on a specific rendering context, use glbInitContextAPI(). Then you can, optionally, call glbBindAPI() to bind those
APIs to global scope.

This is not thread-safe. You can call this multiple times, but each call must be matched with a call to glbUninit(). The first
time this is called it will bind the APIs to global scope.

The internal rendering context can be used like normal. It will be created in double-buffered mode. You can also create your own
context, but you may want to consider calling glbInitContextAPI() or glbInitCurrentContextAPI() after the fact to ensure function
pointers are valid for that context.

You can configure the internal rendering context by specifying a GLBconfig object. This can NULL in which case it will use
defaults. Initialize the config object with glbConfigInit(). The default config creates a context with 32-bit color, 24-bit depth,
8-bit stencil and double-buffered.
*/
GLenum glbInit(GLBapi* pAPI, GLBconfig* pConfig);

/*
Loads context-specific APIs into the specified API object.

This does not bind these APIs to global scope. Use glbBindAPI() for this.
*/
#if defined(GLBIND_WGL)
GLenum glbInitContextAPI(HDC dc, HGLRC rc, GLBapi* pAPI);
#endif
#if defined(GLBIND_GLX)
GLenum glbInitContextAPI(Display *dpy, GLXDrawable drawable, GLXContext rc, GLBapi* pAPI);
#endif

/*
Loads context-specific APIs from the current context into the specified API object.

This does not bind these APIs to global scope. Use glbBindAPI() for this.
*/
GLenum glbInitCurrentContextAPI(GLBapi* pAPI);

/*
Uninitializes glbind.

Each call to glbInit() must be matched up with a call to glbUninit().
*/
void glbUninit();

/*
Binds the function pointers in pAPI to global scope.
*/
GLenum glbBindAPI(const GLBapi* pAPI);

/* Platform-specific APIs. */
#if defined(GLBIND_WGL)
/*
Retrieves the rendering context that was created on the first call to glbInit().
*/
HGLRC glbGetRC();

/*
Retrieves the pixel format that's being used by the rendering context that was created on the first call to glbInit().
*/
int glbGetPixelFormat();

/*
Retrieves the pixel format descriptor being used by the rendering context that was created on the first call to glbInit().
*/
PIXELFORMATDESCRIPTOR* glbGetPFD();
#endif

#if defined(GLBIND_GLX)
/*
Retrieves a reference to the global Display that was created with the first call to glbInit(). If the display was set
in the config object, that Display will be returned.
*/
Display* glbGetDisplay();

/*
Retrieves the rendering context that was created on the first call to glbInit().
*/
GLXContext glbGetRC();

/*
Retrieves the color map that was created on the first call to glbInit().
*/
Colormap glbGetColormap();

/*
Retrieves the framebuffer visual info that was created on the first call to glbInit().
*/
XVisualInfo* glbGetFBVisualInfo();
#endif

#ifdef __cplusplus
}
#endif

/*
Helper API for checking if an extension is supported based on the current rendering context.

This checks cross-platform extensions, WGL extensions and GLX extensions (in that order).

pAPI is optional. If non-null, this relevant APIs from this object will be used. Otherwise, whatever is bound to global
scope will be used.
*/
GLboolean glbIsExtensionSupported(GLBapi* pAPI, const char* extensionName);

#endif  /* GLBIND_H */


/******************************************************************************
 ******************************************************************************

 IMPLEMENTATION

 ******************************************************************************
 ******************************************************************************/
#ifdef GLBIND_IMPLEMENTATION
#if defined(GLBIND_WGL)
#endif
#if defined(GLBIND_GLX)
    #include <unistd.h>
    #include <dlfcn.h>
#endif

typedef void* GLBhandle;
typedef void (* GLBproc)(void);

void glbZeroMemory(void* p, size_t sz)
{
    size_t i;
    for (i = 0; i < sz; ++i) {
        ((GLbyte*)p)[i] = 0;
    }
}

#define glbZeroObject(p) glbZeroMemory((p), sizeof(*(p)));

GLBhandle glb_dlopen(const char* filename)
{
#ifdef _WIN32
    return (GLBhandle)LoadLibraryA(filename);
#else
    return (GLBhandle)dlopen(filename, RTLD_NOW);
#endif
}

void glb_dlclose(GLBhandle handle)
{
#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else
    dlclose((void*)handle);
#endif
}

GLBproc glb_dlsym(GLBhandle handle, const char* symbol)
{
#ifdef _WIN32
    return (GLBproc)GetProcAddress((HMODULE)handle, symbol);
#else
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
#endif
    return (GLBproc)dlsym((void*)handle, symbol);
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
    #pragma GCC diagnostic pop
#endif
#endif
}


static unsigned int g_glbInitCount = 0;
static GLBhandle g_glbOpenGLSO = NULL;

#if defined(GLBIND_WGL)
HWND  glbind_DummyHWND = 0;
HDC   glbind_DC   = 0;
HGLRC glbind_RC   = 0;
PIXELFORMATDESCRIPTOR glbind_PFD;
int glbind_PixelFormat;

static LRESULT GLBIND_DummyWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif
#if defined(GLBIND_GLX)
Display*     glbind_pDisplay      = 0;
Window       glbind_DummyWindow   = 0;
GLXContext   glbind_RC            = 0;
Colormap     glbind_Colormap      = 0;
XVisualInfo* glbind_pFBVisualInfo = 0;
GLboolean    glbind_OwnsDisplay   = GL_FALSE;
#endif

#if defined(GLBIND_WGL)
PFNWGLCREATECONTEXTPROC         glbind_wglCreateContext;
PFNWGLDELETECONTEXTPROC         glbind_wglDeleteContext;
PFNWGLGETCURRENTCONTEXTPROC     glbind_wglGetCurrentContext;
PFNWGLGETCURRENTDCPROC          glbind_wglGetCurrentDC;
PFNWGLGETPROCADDRESSPROC        glbind_wglGetProcAddress;
PFNWGLMAKECURRENTPROC           glbind_wglMakeCurrent;
#endif
#if defined(GLBIND_GLX)
PFNGLXCHOOSEVISUALPROC          glbind_glXChooseVisual;
PFNGLXCREATECONTEXTPROC         glbind_glXCreateContext;
PFNGLXDESTROYCONTEXTPROC        glbind_glXDestroyContext;
PFNGLXMAKECURRENTPROC           glbind_glXMakeCurrent;
PFNGLXSWAPBUFFERSPROC           glbind_glXSwapBuffers;
PFNGLXGETCURRENTCONTEXTPROC     glbind_glXGetCurrentContext;
PFNGLXQUERYEXTENSIONSSTRINGPROC glbind_glXQueryExtensionsString;
PFNGLXGETCURRENTDISPLAYPROC     glbind_glXGetCurrentDisplay;
PFNGLXGETCURRENTDRAWABLEPROC    glbind_glXGetCurrentDrawable;
PFNGLXCHOOSEFBCONFIGPROC        glbind_glXChooseFBConfig;
PFNGLXGETVISUALFROMFBCONFIGPROC glbind_glXGetVisualFromFBConfig;
PFNGLXGETPROCADDRESSPROC        glbind_glXGetProcAddress;

static GLBhandle g_glbX11SO = NULL;
typedef Display* (* GLB_PFNXOPENDISPLAYPROC)   (char* pDisplayName);
typedef int      (* GLB_PFNXCLOSEDISPLAYPROC)  (Display* pDisplay);
typedef Window   (* GLB_PFNXCREATEWINDOWPROC)  (Display *pDisplay, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int borderWidth, int depth, unsigned int class, Visual* pVisual, unsigned long valueMask, XSetWindowAttributes* pAttributes);
typedef int      (* GLB_PFNXDESTROYWINDOWPROC) (Display* pDisplay, Window window);
typedef Colormap (* GLB_PFNXCREATECOLORMAPPROC)(Display *pDisplay, Window window, Visual* pVisual, int alloc);
typedef int      (* GLB_PFNXFREECOLORMAPPROC)  (Display *pDisplay, Colormap colormap);
typedef int      (* GLB_PFNXDEFAULTSCREENPROC) (Display* pDisplay);

GLB_PFNXOPENDISPLAYPROC    glbind_XOpenDisplay;
GLB_PFNXCLOSEDISPLAYPROC   glbind_XCloseDisplay;
GLB_PFNXCREATEWINDOWPROC   glbind_XCreateWindow;
GLB_PFNXDESTROYWINDOWPROC  glbind_XDestroyWindow;
GLB_PFNXCREATECOLORMAPPROC glbind_XCreateColormap;
GLB_PFNXFREECOLORMAPPROC   glbind_XFreeColormap;
GLB_PFNXDEFAULTSCREENPROC  glbind_XDefaultScreen;
#endif

GLBproc glbGetProcAddress(const char* name)
{
    GLBproc func = NULL;
#if defined(GLBIND_WGL)
    if (glbind_wglGetProcAddress) {
        func = (GLBproc)glbind_wglGetProcAddress(name);
    }
#endif
#if defined(GLBIND_GLX)
    if (glbind_glXGetProcAddress) {
        func = (GLBproc)glbind_glXGetProcAddress((const GLubyte*)name);
    }
#endif

    if (func == NULL) {
        func = glb_dlsym(g_glbOpenGLSO, name);
    }

    return func;
}

GLenum glbLoadOpenGLSO()
{
    GLenum result;
    size_t i;

    const char* openGLSONames[] = {
#if defined(_WIN32)
        "OpenGL32.dll"
#elif defined(__APPLE__)
#else
        "libGL.so.1",
        "libGL.so"
#endif
    };

    result = GL_INVALID_OPERATION;
    for (i = 0; i < sizeof(openGLSONames)/sizeof(openGLSONames[0]); ++i) {
        GLBhandle handle = glb_dlopen(openGLSONames[i]);
        if (handle != NULL) {
            g_glbOpenGLSO = handle;
            result = GL_NO_ERROR;
            break;
        }
    }

    if (result != GL_NO_ERROR) {
        return result;
    }

    /* Runtime linking for platform-specific libraries. */
    {
    #if defined(_WIN32)
        /* Win32 */
    #elif defined(__APPLE_)
        /* Apple */
    #else
        /* X11 */
        const char* x11SONames[] = {
            "libX11.so",
            "libX11.so.6"
        };

        result = GL_INVALID_OPERATION;
        for (i = 0; i < sizeof(openGLSONames)/sizeof(openGLSONames[0]); ++i) {
            GLBhandle handle = glb_dlopen(x11SONames[i]);
            if (handle != NULL) {
                g_glbX11SO = handle;
                result = GL_NO_ERROR;
                break;
            }
        }
    #endif
    }

    if (result != GL_NO_ERROR) {
        glb_dlclose(g_glbOpenGLSO);
        g_glbOpenGLSO = NULL;
    }

    return result;
}

GLBconfig glbConfigInit()
{
    GLBconfig config;
    glbZeroObject(&config);

    return config;
}

GLenum glbInit(GLBapi* pAPI, GLBconfig* pConfig)
{
    GLenum result;

    if (g_glbInitCount == 0) {
        result = glbLoadOpenGLSO();
        if (result != GL_NO_ERROR) {
            return result;
        }

        /* Here is where we need to initialize some core APIs. We need these to initialize dummy objects and whatnot. */
#if defined(GLBIND_WGL)
        glbind_wglCreateContext         = (PFNWGLCREATECONTEXTPROC    )glb_dlsym(g_glbOpenGLSO, "wglCreateContext");
        glbind_wglDeleteContext         = (PFNWGLDELETECONTEXTPROC    )glb_dlsym(g_glbOpenGLSO, "wglDeleteContext");
        glbind_wglGetCurrentContext     = (PFNWGLGETCURRENTCONTEXTPROC)glb_dlsym(g_glbOpenGLSO, "wglGetCurrentContext");
        glbind_wglGetCurrentDC          = (PFNWGLGETCURRENTDCPROC     )glb_dlsym(g_glbOpenGLSO, "wglGetCurrentDC");
        glbind_wglGetProcAddress        = (PFNWGLGETPROCADDRESSPROC   )glb_dlsym(g_glbOpenGLSO, "wglGetProcAddress");
        glbind_wglMakeCurrent           = (PFNWGLMAKECURRENTPROC      )glb_dlsym(g_glbOpenGLSO, "wglMakeCurrent");

        if (glbind_wglCreateContext     == NULL ||
            glbind_wglDeleteContext     == NULL ||
            glbind_wglGetCurrentContext == NULL ||
            glbind_wglGetCurrentDC      == NULL ||
            glbind_wglGetProcAddress    == NULL ||
            glbind_wglMakeCurrent       == NULL) {
            glb_dlclose(g_glbOpenGLSO);
            g_glbOpenGLSO = NULL;
            return GL_INVALID_OPERATION;
        }
#endif
#if defined(GLBIND_GLX)
        glbind_glXChooseVisual          = (PFNGLXCHOOSEVISUALPROC         )glb_dlsym(g_glbOpenGLSO, "glXChooseVisual");
        glbind_glXCreateContext         = (PFNGLXCREATECONTEXTPROC        )glb_dlsym(g_glbOpenGLSO, "glXCreateContext");
        glbind_glXDestroyContext        = (PFNGLXDESTROYCONTEXTPROC       )glb_dlsym(g_glbOpenGLSO, "glXDestroyContext");
        glbind_glXMakeCurrent           = (PFNGLXMAKECURRENTPROC          )glb_dlsym(g_glbOpenGLSO, "glXMakeCurrent");
        glbind_glXSwapBuffers           = (PFNGLXSWAPBUFFERSPROC          )glb_dlsym(g_glbOpenGLSO, "glXSwapBuffers");
        glbind_glXGetCurrentContext     = (PFNGLXGETCURRENTCONTEXTPROC    )glb_dlsym(g_glbOpenGLSO, "glXGetCurrentContext");
        glbind_glXQueryExtensionsString = (PFNGLXQUERYEXTENSIONSSTRINGPROC)glb_dlsym(g_glbOpenGLSO, "glXQueryExtensionsString");
        glbind_glXGetCurrentDisplay     = (PFNGLXGETCURRENTDISPLAYPROC    )glb_dlsym(g_glbOpenGLSO, "glXGetCurrentDisplay");
        glbind_glXGetCurrentDrawable    = (PFNGLXGETCURRENTDRAWABLEPROC   )glb_dlsym(g_glbOpenGLSO, "glXGetCurrentDrawable");
        glbind_glXChooseFBConfig        = (PFNGLXCHOOSEFBCONFIGPROC       )glb_dlsym(g_glbOpenGLSO, "glXChooseFBConfig");
        glbind_glXGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)glb_dlsym(g_glbOpenGLSO, "glXGetVisualFromFBConfig");
        glbind_glXGetProcAddress        = (PFNGLXGETPROCADDRESSPROC       )glb_dlsym(g_glbOpenGLSO, "glXGetProcAddress");

        if (glbind_glXChooseVisual          == NULL ||
            glbind_glXCreateContext         == NULL ||
            glbind_glXDestroyContext        == NULL ||
            glbind_glXMakeCurrent           == NULL ||
            glbind_glXSwapBuffers           == NULL ||
            glbind_glXGetCurrentContext     == NULL ||
            glbind_glXQueryExtensionsString == NULL ||
            glbind_glXGetCurrentDisplay     == NULL ||
            glbind_glXGetCurrentDrawable    == NULL ||
            glbind_glXChooseFBConfig        == NULL ||
            glbind_glXGetVisualFromFBConfig == NULL ||
            glbind_glXGetProcAddress        == NULL) {
            glb_dlclose(g_glbOpenGLSO);
            g_glbOpenGLSO = NULL;
            return GL_INVALID_OPERATION;
        }

        glbind_XOpenDisplay    = (GLB_PFNXOPENDISPLAYPROC   )glb_dlsym(g_glbX11SO, "XOpenDisplay");
        glbind_XCloseDisplay   = (GLB_PFNXCLOSEDISPLAYPROC  )glb_dlsym(g_glbX11SO, "XCloseDisplay");
        glbind_XCreateWindow   = (GLB_PFNXCREATEWINDOWPROC  )glb_dlsym(g_glbX11SO, "XCreateWindow");
        glbind_XDestroyWindow  = (GLB_PFNXDESTROYWINDOWPROC )glb_dlsym(g_glbX11SO, "XDestroyWindow");
        glbind_XCreateColormap = (GLB_PFNXCREATECOLORMAPPROC)glb_dlsym(g_glbX11SO, "XCreateColormap");
        glbind_XFreeColormap   = (GLB_PFNXFREECOLORMAPPROC  )glb_dlsym(g_glbX11SO, "XFreeColormap");
        glbind_XDefaultScreen  = (GLB_PFNXDEFAULTSCREENPROC )glb_dlsym(g_glbX11SO, "XDefaultScreen");

        if (glbind_XOpenDisplay    == NULL ||
            glbind_XCloseDisplay   == NULL ||
            glbind_XCreateWindow   == NULL ||
            glbind_XDestroyWindow  == NULL ||
            glbind_XCreateColormap == NULL ||
            glbind_XFreeColormap   == NULL ||
            glbind_XDefaultScreen  == NULL) {
            glb_dlclose(g_glbX11SO);
            g_glbX11SO = NULL;
            glb_dlclose(g_glbOpenGLSO);
            g_glbOpenGLSO = NULL;
            return GL_INVALID_OPERATION;
        }
#endif

        /* Here is where we need to initialize our dummy objects so we can get a context and retrieve some API pointers. */
#if defined(GLBIND_WGL)
        {
            HWND hWnd = NULL;

            if (pConfig != NULL) {
                hWnd = pConfig->hWnd;
            }

            /* Create a dummy window if we haven't passed in an explicit window. */
            if (hWnd == NULL) {
                WNDCLASSEXW dummyWC;
                memset(&dummyWC, 0, sizeof(dummyWC));
                dummyWC.cbSize        = sizeof(dummyWC);
                dummyWC.lpfnWndProc   = (WNDPROC)GLBIND_DummyWindowProcWin32;
                dummyWC.lpszClassName = L"GLBIND_DummyHWND";
                dummyWC.style         = CS_OWNDC;
                if (!RegisterClassExW(&dummyWC)) {
                    glb_dlclose(g_glbOpenGLSO);
                    g_glbOpenGLSO = NULL;
                    return GL_INVALID_OPERATION;
                }

                hWnd = CreateWindowExW(0, L"GLBIND_DummyHWND", L"", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
                glbind_DummyHWND = hWnd;
            }

            glbind_DC = GetDC(hWnd);

            memset(&glbind_PFD, 0, sizeof(glbind_PFD));
            glbind_PFD.nSize        = sizeof(glbind_PFD);
            glbind_PFD.nVersion     = 1;
            glbind_PFD.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | ((pConfig == NULL || pConfig->singleBuffered == GL_FALSE) ? PFD_DOUBLEBUFFER : 0);
            glbind_PFD.iPixelType   = PFD_TYPE_RGBA;
            glbind_PFD.cStencilBits = 8;
            glbind_PFD.cDepthBits   = 24;
            glbind_PFD.cColorBits   = 32;
            glbind_PixelFormat = ChoosePixelFormat(glbind_DC, &glbind_PFD);
            if (glbind_PixelFormat == 0) {
                DestroyWindow(hWnd);
                glb_dlclose(g_glbOpenGLSO);
                g_glbOpenGLSO = NULL;
                return GL_INVALID_OPERATION;
            }

            if (!SetPixelFormat(glbind_DC, glbind_PixelFormat, &glbind_PFD)) {
                DestroyWindow(hWnd);
                glb_dlclose(g_glbOpenGLSO);
                g_glbOpenGLSO = NULL;
                return GL_INVALID_OPERATION;
            }

            glbind_RC = glbind_wglCreateContext(glbind_DC);
            if (glbind_RC == NULL) {
                DestroyWindow(hWnd);
                glb_dlclose(g_glbOpenGLSO);
                g_glbOpenGLSO = NULL;
                return GL_INVALID_OPERATION;
            }

            glbind_wglMakeCurrent(glbind_DC, glbind_RC);
        }
#endif

#if defined(GLBIND_GLX)
        {
            static int attribs[] = {
                GLX_RGBA,
                GLX_RED_SIZE,      8,
                GLX_GREEN_SIZE,    8,
                GLX_BLUE_SIZE,     8,
                GLX_ALPHA_SIZE,    8,
                GLX_DEPTH_SIZE,    24,
                GLX_STENCIL_SIZE,  8,
                GLX_DOUBLEBUFFER,
                None, None
            };

            if (pConfig != NULL) {
                if (!pConfig->singleBuffered) {
                    attribs[13] = None;
                }
            }
    
            glbind_OwnsDisplay = GL_TRUE;
            glbind_pDisplay = glbind_XOpenDisplay(NULL);
            if (glbind_pDisplay == NULL) {
                glb_dlclose(g_glbOpenGLSO);
                g_glbOpenGLSO = NULL;
                return GL_INVALID_OPERATION;
            }

            glbind_pFBVisualInfo = glbind_glXChooseVisual(glbind_pDisplay, DefaultScreen(glbind_pDisplay), attribs);
            if (glbind_pFBVisualInfo == NULL) {
                glb_dlclose(g_glbOpenGLSO);
                g_glbOpenGLSO = NULL;
                return GL_INVALID_OPERATION;
            }

            glbind_Colormap = glbind_XCreateColormap(glbind_pDisplay, RootWindow(glbind_pDisplay, glbind_pFBVisualInfo->screen), glbind_pFBVisualInfo->visual, AllocNone);

            glbind_RC = glbind_glXCreateContext(glbind_pDisplay, glbind_pFBVisualInfo, NULL, GL_TRUE);
            if (glbind_RC == NULL) {
                glb_dlclose(g_glbOpenGLSO);
                g_glbOpenGLSO = NULL;
                return GL_INVALID_OPERATION;
            }

            /* We cannot call any OpenGL APIs until a context is made current. In order to make a context current we will need a window. We just use a dummy window for this. */
            XSetWindowAttributes wa;
            wa.colormap = glbind_Colormap;
            wa.border_pixel = 0;

            /* Window's can not have dimensions of 0 in X11. We stick with dimensions of 1. */
            glbind_DummyWindow = glbind_XCreateWindow(glbind_pDisplay, RootWindow(glbind_pDisplay, glbind_pFBVisualInfo->screen), 0, 0, 1, 1, 0, glbind_pFBVisualInfo->depth, InputOutput, glbind_pFBVisualInfo->visual, CWBorderPixel | CWColormap, &wa);
            if (glbind_DummyWindow == 0) {
                glb_dlclose(g_glbOpenGLSO);
                g_glbOpenGLSO = NULL;
                return GL_INVALID_OPERATION;
            }

            glbind_glXMakeCurrent(glbind_pDisplay, glbind_DummyWindow, glbind_RC);
        }
#endif
    }

    if (pAPI != NULL) {
#if defined(GLBIND_WGL)
        result = glbInitContextAPI(glbind_DC, glbind_RC, pAPI);
#endif
#if defined(GLBIND_GLX)
        result = glbInitContextAPI(glbind_pDisplay, glbind_DummyWindow, glbind_RC, pAPI);
#endif
        if (result == GL_NO_ERROR) {
            if (g_glbInitCount == 0) {
                result = glbBindAPI(pAPI);
            }
        }
    } else {
        GLBapi tempAPI;
#if defined(GLBIND_WGL)
        result = glbInitContextAPI(glbind_DC, glbind_RC, &tempAPI);
#endif
#if defined(GLBIND_GLX)
        result = glbInitContextAPI(glbind_pDisplay, glbind_DummyWindow, glbind_RC, &tempAPI);
#endif
        if (result == GL_NO_ERROR) {
            if (g_glbInitCount == 0) {
                result = glbBindAPI(pAPI);
            }
        }
    }

    /* If at this point we have an error we need to uninitialize the global objects (if this is the initial initialization) and return. */
    if (result != GL_NO_ERROR) {
        if (g_glbInitCount == 0) {
#if defined(GLBIND_WGL)
            if (glbind_RC) {
                glbind_wglDeleteContext(glbind_RC);
                glbind_RC = 0;
            }
            if (glbind_DummyHWND) {
                DestroyWindow(glbind_DummyHWND);
                glbind_DummyHWND = 0;
            }
            glbind_DC = 0;
#endif
#if defined(GLBIND_GLX)
            if (glbind_RC) {
                glbind_glXDestroyContext(glbind_pDisplay, glbind_RC);
                glbind_RC = 0;
            }
            if (glbind_DummyWindow) {
                glbind_XDestroyWindow(glbind_pDisplay, glbind_DummyWindow);
                glbind_DummyWindow = 0;
            }
            if (glbind_pDisplay && glbind_OwnsDisplay) {
                glbind_XCloseDisplay(glbind_pDisplay);
                glbind_pDisplay    = 0;
                glbind_OwnsDisplay = GL_FALSE;
            }
#endif

            glb_dlclose(g_glbOpenGLSO);
            g_glbOpenGLSO = NULL;
        }

        return result;
    }

    g_glbInitCount += 1;    /* <-- Only increment the init counter on success. */
    return GL_NO_ERROR;
}

#if defined(GLBIND_WGL)
GLenum glbInitContextAPI(HDC dc, HGLRC rc, GLBapi* pAPI)
{
    GLenum result;
    HDC dcPrev;
    HGLRC rcPrev;
    
    dcPrev = glbind_wglGetCurrentDC();
    rcPrev = glbind_wglGetCurrentContext();

    if (dcPrev != dc && rcPrev != rc) {
        glbind_wglMakeCurrent(dc, rc);
    }
    
    result = glbInitCurrentContextAPI(pAPI);
    
    if (dcPrev != dc && rcPrev != rc) {
        glbind_wglMakeCurrent(dcPrev, rcPrev);
    }

    return result;
}
#endif
#if defined(GLBIND_GLX)
GLenum glbInitContextAPI(Display *dpy, GLXDrawable drawable, GLXContext rc, GLBapi* pAPI)
{
    GLenum result;
    GLXContext rcPrev = 0;
    GLXDrawable drawablePrev = 0;
    Display* dpyPrev = NULL;

    if (glbind_glXGetCurrentContext && glbind_glXGetCurrentDrawable && glbind_glXGetCurrentDisplay) {
        rcPrev       = glbind_glXGetCurrentContext();
        drawablePrev = glbind_glXGetCurrentDrawable();
        dpyPrev      = glbind_glXGetCurrentDisplay();
    }

    glbind_glXMakeCurrent(dpy, drawable, rc);
    result = glbInitCurrentContextAPI(pAPI);
    glbind_glXMakeCurrent(dpyPrev, drawablePrev, rcPrev);

    return result;
}
#endif

GLenum glbInitCurrentContextAPI(GLBapi* pAPI)
{
    if (pAPI == NULL) {
        return GL_INVALID_OPERATION;
    }

    glbZeroObject(pAPI);

/*<<init_current_context_api>>*/

    return GL_NO_ERROR;
}

void glbUninit()
{
    if (g_glbInitCount == 0) {
        return;
    }

    g_glbInitCount -= 1;
    if (g_glbInitCount == 0) {
#if defined(GLBIND_WGL)
        if (glbind_RC) {
            glbind_wglDeleteContext(glbind_RC);
            glbind_RC = 0;
        }
        if (glbind_DummyHWND) {
            DestroyWindow(glbind_DummyHWND);
            glbind_DummyHWND = 0;
        }
        glbind_DC = 0;
#endif
#if defined(GLBIND_GLX)
        if (glbind_RC) {
            glbind_glXDestroyContext(glbind_pDisplay, glbind_RC);
            glbind_RC = 0;
        }
        if (glbind_DummyWindow) {
            glbind_XDestroyWindow(glbind_pDisplay, glbind_DummyWindow);
            glbind_DummyWindow = 0;
        }
        if (glbind_pDisplay && glbind_OwnsDisplay) {
            glbind_XCloseDisplay(glbind_pDisplay);
            glbind_pDisplay    = 0;
            glbind_OwnsDisplay = GL_FALSE;
        }
#endif

        glb_dlclose(g_glbOpenGLSO);
        g_glbOpenGLSO = NULL;
    }
}

GLenum glbBindAPI(const GLBapi* pAPI)
{
    GLenum result;

    if (pAPI == NULL) {
        GLBapi tempAPI;
#if defined(GLBIND_WGL)
        result = glbInitContextAPI(glbind_DC, glbind_RC, &tempAPI);
#endif
#if defined(GLBIND_GLX)
        result = glbInitContextAPI(glbind_pDisplay, glbind_DummyWindow, glbind_RC, &tempAPI);
#endif
        if (result != GL_NO_ERROR) {
            return result;
        }

        return glbBindAPI(&tempAPI);
    }

/*<<set_global_api_from_struct>>*/

    return GL_NO_ERROR;
}

#if defined(GLBIND_WGL)
HGLRC glbGetRC()
{
    return glbind_RC;
}

int glbGetPixelFormat()
{
    return glbind_PixelFormat;
}

PIXELFORMATDESCRIPTOR* glbGetPFD()
{
    return &glbind_PFD;
}
#endif

#if defined(GLBIND_GLX)
Display* glbGetDisplay()
{
    return glbind_pDisplay;
}

GLXContext glbGetRC()
{
    return glbind_RC;
}

Colormap glbGetColormap()
{
    return glbind_Colormap;
}

XVisualInfo* glbGetFBVisualInfo()
{
    return glbind_pFBVisualInfo;
}
#endif


int glb_strcmp(const char* s1, const char* s2)
{
    while ((*s1) && (*s1 == *s2)) {
        ++s1;
        ++s2;
    }

    return (*(unsigned char*)s1 - *(unsigned char*)s2);
}

int glb_strncmp(const char* s1, const char* s2, size_t n)
{
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }

    if (n == 0) {
        return 0;
    } else {
        return (*(unsigned char*)s1 - *(unsigned char*)s2);
    }
}

GLboolean glbIsExtensionInString(const char* ext, const char* str)
{
    const char* ext2beg;
    const char* ext2end;

    if (ext == NULL || str == NULL) {
        return GL_FALSE;
    }

    ext2beg = str;
    ext2end = ext2beg;

    for (;;) {
        while (ext2end[0] != ' ' && ext2end[0] != '\0') {
            ext2end += 1;
        }

        if (glb_strncmp(ext, ext2beg, ext2end - ext2beg) == 0) {
            return GL_TRUE;
        }

        /* Break if we've reached the end. Otherwise, just move to start fo the next extension. */
        if (ext2end[0] == '\0') {
            break;
        } else {
            ext2beg = ext2end + 1;
            ext2end = ext2beg;
        }
    }

    return GL_FALSE;
}

#if defined(GLBIND_WGL)
GLboolean glbIsExtensionSupportedWGL(GLBapi* pAPI, const char* extensionName)
{
    PFNWGLGETEXTENSIONSSTRINGARBPROC _wglGetExtensionsStringARB = (pAPI != NULL) ? pAPI->wglGetExtensionsStringARB : wglGetExtensionsStringARB;
    PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = (pAPI != NULL) ? pAPI->wglGetExtensionsStringEXT : wglGetExtensionsStringEXT;
    PFNWGLGETCURRENTDCPROC           _wglGetCurrentDC           = (pAPI != NULL) ? pAPI->wglGetCurrentDC           : glbind_wglGetCurrentDC;

    if (_wglGetExtensionsStringARB) {
        return glbIsExtensionInString(extensionName, _wglGetExtensionsStringARB(_wglGetCurrentDC()));
    }
    if (_wglGetExtensionsStringEXT) {
        return glbIsExtensionInString(extensionName, _wglGetExtensionsStringEXT());
    }

    return GL_FALSE;
}
#endif

#if defined(GLBIND_GLX)
GLboolean glbIsExtensionSupportedGLX(GLBapi* pAPI, const char* extensionName)
{
    PFNGLXQUERYEXTENSIONSSTRINGPROC _glXQueryExtensionsString = (pAPI != NULL) ? pAPI->glXQueryExtensionsString : glbind_glXQueryExtensionsString;

    if (_glXQueryExtensionsString) {
        return glbIsExtensionInString(extensionName, _glXQueryExtensionsString(glbGetDisplay(), glbind_XDefaultScreen(glbGetDisplay())));
    }

    return GL_FALSE;
}
#endif

GLboolean glbIsExtensionSupported(GLBapi* pAPI, const char* extensionName)
{
    GLboolean isSupported = GL_FALSE;
    PFNGLGETSTRINGIPROC  _glGetStringi  = (pAPI != NULL) ? pAPI->glGetStringi  : glGetStringi;
    PFNGLGETSTRINGPROC   _glGetString   = (pAPI != NULL) ? pAPI->glGetString   : glGetString;
    PFNGLGETINTEGERVPROC _glGetIntegerv = (pAPI != NULL) ? pAPI->glGetIntegerv : glGetIntegerv;

    /* Try the new way first. */
    if (_glGetStringi && _glGetIntegerv) {
        GLint iExtension;
        GLint supportedExtensionCount = 0;
        _glGetIntegerv(GL_NUM_EXTENSIONS, &supportedExtensionCount);

        for (iExtension = 0; iExtension < supportedExtensionCount; ++iExtension) {
            const char* pSupportedExtension = (const char*)_glGetStringi(GL_EXTENSIONS, iExtension);
            if (pSupportedExtension != NULL) {
                if (glb_strcmp(pSupportedExtension, extensionName) == 0) {
                    return GL_TRUE;
                }
            }
        }

        /* It's not a core extension. Check platform-specific extensions. */
        isSupported = GL_FALSE;
#if defined(GLBIND_WGL)
        isSupported = glbIsExtensionSupportedWGL(pAPI, extensionName);
#endif
#if defined(GLBIND_GLX)
        isSupported = glbIsExtensionSupportedGLX(pAPI, extensionName);
#endif
        return isSupported;
    }

    /* Fall back to old style. */
    if (_glGetString) {
        isSupported = glbIsExtensionInString(extensionName, (const char*)_glGetString(GL_EXTENSIONS));
        if (!isSupported) {
#if defined(GLBIND_WGL)
            isSupported = glbIsExtensionSupportedWGL(pAPI, extensionName);
#endif
#if defined(GLBIND_GLX)
            isSupported = glbIsExtensionSupportedGLX(pAPI, extensionName);
#endif
        }
    }

    return isSupported;
}

#endif  /* GLBIND_IMPLEMENTATION */

/*
This software is available as a choice of the following licenses. Choose
whichever you prefer.

===============================================================================
ALTERNATIVE 1 - Public Domain (www.unlicense.org)
===============================================================================
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

===============================================================================
ALTERNATIVE 2 - MIT No Attribution
===============================================================================
Copyright 2019 David Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
