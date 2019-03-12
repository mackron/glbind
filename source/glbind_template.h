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
*/
GLenum glbInit(GLBapi* pAPI);

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

GLenum glbBindGlobalAPI()
{
    /*<<load_global_api_funcpointers>>*/


    /*<<load_safe_global_api>>*/

    return GL_NO_ERROR;
}

GLenum glbInit(GLBapi* pAPI)
{
    if (g_glbInitCount == 0) {
        GLenum result = glbLoadOpenGLSO();
        if (result != GL_NO_ERROR) {
            return result;
        }

        result = glbBindGlobalAPI();
        if (result != GL_NO_ERROR) {
            return result;
        }
    }

    if (pAPI != NULL) {
        /*<<set_struct_api_from_global>>*/
    }

    g_glbInitCount += 1;    /* <-- Only increment the init counter on success. */
    return GL_NO_ERROR;
}

void glbUninit()
{
    if (g_glbInitCount == 0) {
        return;
    }

    g_glbInitCount -= 1;
    if (g_glbInitCount == 0) {
        glb_dlclose(g_glbOpenGLSO);
        g_glbOpenGLSO = NULL;
    }
}

GLenum glbBindAPI(const GLBapi* pAPI)
{
    if (g_glbInitCount == 0) {
        return GL_INVALID_OPERATION;  /* glbind not initialized. */
    }

    if (pAPI == NULL) {
        return glbBindGlobalAPI();
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
