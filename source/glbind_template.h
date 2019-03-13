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
The office OpenGL headers have a dependency on a header called khrplatform.h. From what I can see it's mainly just for sized types. Since glbind is a
single header, and that we can't just copy-and-paste the contents of khrplatform.h due to licensing, we need to do our own sized type declarations.
*/
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

/* Platform headers. */
#if defined(GLBIND_WGL)
#include <windows.h>    /* Can we remove this dependency? */
#endif
#if defined(GLBIND_GLX)
/* TODO: Include X headers. */
#endif

/*
The office OpenGL headers have traditionally defined their APIs with APIENTRY, APIENTRYP and GLAPI. I'm including these just in case
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
*/
GLenum glbInit(GLBapi* pAPI);

/*
Loads context-specific APIs into the specified API object.

This this not bind these APIs to global scope. Use glbBindAPI() for this.
*/
#if defined(GLBIND_WGL)
GLenum glbInitContextAPI(HDC dc, HGLRC rc, GLBapi* pAPI);
#endif
#if defined(GLBIND_GLX)
GLenum glbInitContextAPI(Display *dpy, GLXDrawable drawable, GLXContext rc, GLBapi* pAPI);
#endif

/*
Loads context-specific APIs from the current context into the specified API object.

This this not bind these APIs to global scope. Use glbBindAPI() for this.
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

#ifdef __cplusplus
}
#endif
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
HDC   glbind_DummyDC   = 0;
HGLRC glbind_DummyRC   = 0;
PIXELFORMATDESCRIPTOR glbind_DummyPFD;
int glbind_DummyPixelFormat;

static LRESULT GLBIND_DummyWindowProcWin32(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif
#if defined(GLBIND_GLX)
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
PFNGLXCHOOSEVISUALPROC          glbind_glxChooseVisual;
PFNGLXCREATECONTEXTPROC         glbind_glxCreateContext;
PFNGLXDESTROYCONTEXTPROC        glbind_glxDestroyContext;
PFNGLXMAKECURRENTPROC           glbind_glxMakeCurrent;
PFNGLXSWAPBUFFERSPROC           glbind_glxSwapBuffers;
PFNGLXGETCURRENTCONTEXTPROC     glbind_glxGetCurrentContext;
PFNGLXQUERYEXTENSIONSSTRINGPROC glbind_glxQueryExtensionsString;
PFNGLXGETCURRENTDISPLAYPROC     glbind_glxGetCurrentDisplay;
PFNGLXCHOOSEFBCONFIGPROC        glbind_glxChooseFBConfig;
PFNGLXGETVISUALFROMFBCONFIGPROC glbind_glxGetVisualFromFBConfig;
PFNGLXGETPROCADDRESSPROC        glbind_glxGetProcAddress;
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
    if (glbind_glxGetProcAddress) {
        func = (GLBproc)glbind_glxGetProcAddress(name);
    }
#endif

    if (func == NULL) {
        func = glb_dlsym(g_glbOpenGLSO, name);
    }

    return func;
}

GLenum glbLoadOpenGLSO()
{
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

    for (i = 0; i < sizeof(openGLSONames)/sizeof(openGLSONames[0]); ++i) {
        GLBhandle handle = glb_dlopen(openGLSONames[i]);
        if (handle != NULL) {
            g_glbOpenGLSO = handle;
            return GL_NO_ERROR;
        }
    }

    return GL_INVALID_OPERATION;
}

GLenum glbInit(GLBapi* pAPI)
{
    GLenum result;

    if (g_glbInitCount == 0) {
        GLenum result = glbLoadOpenGLSO();
        if (result != GL_NO_ERROR) {
            return result;
        }

        /* Here is where we need to initialize some core APIs. We need these to initialize dummy objects and whatnot. */
#if defined(GLBIND_WGL)
        glbind_wglCreateContext         = (PFNWGLCREATECONTEXTPROC        )glb_dlsym(g_glbOpenGLSO, "wglCreateContext");
        glbind_wglDeleteContext         = (PFNWGLDELETECONTEXTPROC        )glb_dlsym(g_glbOpenGLSO, "wglDeleteContext");
        glbind_wglGetCurrentContext     = (PFNWGLGETCURRENTCONTEXTPROC    )glb_dlsym(g_glbOpenGLSO, "wglGetCurrentContext");
        glbind_wglGetCurrentDC          = (PFNWGLGETCURRENTDCPROC         )glb_dlsym(g_glbOpenGLSO, "wglGetCurrentDC");
        glbind_wglGetProcAddress        = (PFNWGLGETPROCADDRESSPROC       )glb_dlsym(g_glbOpenGLSO, "wglGetProcAddress");
        glbind_wglMakeCurrent           = (PFNWGLMAKECURRENTPROC          )glb_dlsym(g_glbOpenGLSO, "wglMakeCurrent");
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
        glbind_glxChooseVisual          = (PFNGLXCHOOSEVISUALPROC         )glb_dlsym(g_glbOpenGLSO, "glXChooseVisual");
        glbind_glxCreateContext         = (PFNGLXCREATECONTEXTPROC        )glb_dlsym(g_glbOpenGLSO, "glXCreateContext");
        glbind_glxDestroyContext        = (PFNGLXDESTROYCONTEXTPROC       )glb_dlsym(g_glbOpenGLSO, "glXDestroyContext");
        glbind_glxMakeCurrent           = (PFNGLXMAKECURRENTPROC          )glb_dlsym(g_glbOpenGLSO, "glXMakeCurrent");
        glbind_glxSwapBuffers           = (PFNGLXSWAPBUFFERSPROC          )glb_dlsym(g_glbOpenGLSO, "glXSwapBuffers");
        glbind_glxGetCurrentContext     = (PFNGLXGETCURRENTCONTEXTPROC    )glb_dlsym(g_glbOpenGLSO, "glXGetCurrentContext");
        glbind_glxQueryExtensionsString = (PFNGLXQUERYEXTENSIONSSTRINGPROC)glb_dlsym(g_glbOpenGLSO, "glXQueryExtensionsString");
        glbind_glxGetCurrentDisplay     = (PFNGLXGETCURRENTDISPLAYPROC    )glb_dlsym(g_glbOpenGLSO, "glXGetCurrentDisplay");
        glbind_glxChooseFBConfig        = (PFNGLXCHOOSEFBCONFIGPROC       )glb_dlsym(g_glbOpenGLSO, "glXChooseFBConfig");
        glbind_glxGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)glb_dlsym(g_glbOpenGLSO, "glXGetVisualFromFBConfig");
        glbind_glxGetProcAddress        = (PFNGLXGETPROCADDRESSPROC       )glb_dlsym(g_glbOpenGLSO, "glXGetProcAddress");

        if (glbind_glxChooseVisual          == NULL ||
            glbind_glxCreateContext         == NULL ||
            glbind_glxDestroyContext        == NULL ||
            glbind_glxMakeCurrent           == NULL ||
            glbind_glxSwapBuffers           == NULL ||
            glbind_glxGetCurrentContext     == NULL ||
            glbind_glxQueryExtensionsString == NULL ||
            glbind_glxGetCurrentDisplay     == NULL ||
            glbind_glxChooseFBConfig        == NULL ||
            glbind_glxGetVisualFromFBConfig == NULL ||
            glbind_glxGetProcAddress        == NULL) {
            glb_dlclose(g_glbOpenGLSO);
            g_glbOpenGLSO = NULL;
            return GL_INVALID_OPERATION;
        }
#endif
    }


    /* Here is where we need to initialize our dummy objects so we can get a context and retrieve some API pointers. */
#if defined(GLBIND_WGL)
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

    glbind_DummyHWND = CreateWindowExW(0, L"GLBIND_DummyHWND", L"", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
    glbind_DummyDC   = GetDC(glbind_DummyHWND);

    memset(&glbind_DummyPFD, 0, sizeof(glbind_DummyPFD));
    glbind_DummyPFD.nSize        = sizeof(glbind_DummyPFD);
    glbind_DummyPFD.nVersion     = 1;
    glbind_DummyPFD.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    glbind_DummyPFD.iPixelType   = PFD_TYPE_RGBA;
    glbind_DummyPFD.cStencilBits = 8;
    glbind_DummyPFD.cDepthBits   = 24;
    glbind_DummyPFD.cColorBits   = 32;
    glbind_DummyPixelFormat = ChoosePixelFormat(glbind_DummyDC, &glbind_DummyPFD);
    if (glbind_DummyPixelFormat == 0) {
        DestroyWindow(glbind_DummyHWND);
        glb_dlclose(g_glbOpenGLSO);
        g_glbOpenGLSO = NULL;
        return GL_INVALID_OPERATION;
    }

    if (!SetPixelFormat(glbind_DummyDC, glbind_DummyPixelFormat, &glbind_DummyPFD)) {
        DestroyWindow(glbind_DummyHWND);
        glb_dlclose(g_glbOpenGLSO);
        g_glbOpenGLSO = NULL;
        return GL_INVALID_OPERATION;
    }

    glbind_DummyRC = glbind_wglCreateContext(glbind_DummyDC);
    if (glbind_DummyRC == NULL) {
        DestroyWindow(glbind_DummyHWND);
        glb_dlclose(g_glbOpenGLSO);
        g_glbOpenGLSO = NULL;
        return GL_INVALID_OPERATION;
    }

    glbind_wglMakeCurrent(glbind_DummyDC, glbind_DummyRC);
#endif

#if defined(GLBIND_GLX)

#endif


    if (pAPI != NULL) {
#if defined(GLBIND_WGL)
        result = glbInitContextAPI(glbind_DummyDC, glbind_DummyRC, pAPI);
#endif
#if defined(GLBIND_GLX)
        result = glbInitContextAPI(glbind_DummyDisplay, glbind_DummyDrawable, glbind_DummyRC, pAPI);
#endif
        if (result == GL_NO_ERROR) {
            if (g_glbInitCount == 0) {
                result = glbBindAPI(pAPI);
            }
        }
    } else {
        GLBapi tempAPI;
#if defined(GLBIND_WGL)
        result = glbInitContextAPI(glbind_DummyDC, glbind_DummyRC, &tempAPI);
#endif
#if defined(GLBIND_GLX)
        result = glbInitContextAPI(glbind_DummyDisplay, glbind_DummyDrawable, glbind_DummyRC, &tempAPI);
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
            if (glbind_DummyRC) {
                glbind_wglDeleteContext(glbind_DummyRC);
                glbind_DummyRC = 0;
            }
            if (glbind_DummyHWND) {
                DestroyWindow(glbind_DummyHWND);
                glbind_DummyHWND = 0;
                glbind_DummyDC   = 0;
            }
#endif
#if defined(GLBIND_GLX)
            /* TODO: Uninitialize dummy objects. */
#endif

            glb_dlclose(g_glbOpenGLSO);
            g_glbOpenGLSO = NULL;
        }
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
    Display* displayPrev = NULL;

    if (glbind_glxGetCurrentContext && glbind_glxGetCurrentDrawable && glbind_glxGetCurrentDisplay) {
        rcPrev       = glbind_glxGetCurrentContext();
        drawablePrev = glbind_glxGetCurrentDrawable();
        displayPrev  = glbind_glxGetCurrentDisplay();
    }

    glbind_glxMakeCurrent(dpy, drawable, rc);
    result = glbInitCurrentContextAPI(pAPI);
    glbind_glxMakeCurrent(dpyPrev, drawablePrev, rcPrev);

    return result;
}
#endif

GLenum glbInitCurrentContextAPI(GLBapi* pAPI)
{
    if (pAPI == NULL) {
        return GL_INVALID_OPERATION;
    }

    memset(pAPI, 0, sizeof(*pAPI));

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
        if (glbind_DummyRC) {
            glbind_wglDeleteContext(glbind_DummyRC);
            glbind_DummyRC = 0;
        }
        if (glbind_DummyHWND) {
            DestroyWindow(glbind_DummyHWND);
            glbind_DummyHWND = 0;
            glbind_DummyDC   = 0;
        }
#endif
#if defined(GLBIND_GLX)

#endif

        glb_dlclose(g_glbOpenGLSO);
        g_glbOpenGLSO = NULL;
    }
}

GLenum glbBindAPI(const GLBapi* pAPI)
{
    GLenum result;

    if (g_glbInitCount == 0) {
        return GL_INVALID_OPERATION;  /* glbind not initialized. */
    }

    if (pAPI == NULL) {
        GLBapi tempAPI;
#if defined(GLBIND_WGL)
        result = glbInitContextAPI(glbind_DummyDC, glbind_DummyRC, &tempAPI);
#endif
#if defined(GLBIND_GLX)
        result = glbInitContextAPI(glbind_DummyDisplay, glbind_DummyDrawable, glbind_DummyRC, &tempAPI);
#endif
        if (result != GL_NO_ERROR) {
            return result;
        }

        return glbBindAPI(&tempAPI);
    }

/*<<set_global_api_from_struct>>*/

    return GL_NO_ERROR;
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
