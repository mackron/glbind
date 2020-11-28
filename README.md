<h4 align="center">A single file OpenGL header and API loader.</h4>

<p align="center">
    <a href="https://discord.gg/9vpqbjU"><img src="https://img.shields.io/discord/712952679415939085?label=discord&logo=discord" alt="discord"></a>
    <a href="https://twitter.com/mackron"><img src="https://img.shields.io/twitter/follow/mackron?style=flat&label=twitter&color=1da1f2&logo=twitter" alt="twitter"></a>
</p>

glbind includes a full implementation of the OpenGL headers (auto-generated from the OpenGL spec) so there's no need
for the offical headers or SDK. Unlike the official headers, the platform-specific sections are all contained within
the same file.


Usage
=====
glbind is a single file library with no dependencies. There's no need to link to any libraries, nor do you need to
include any other headers. Everything you need is included in `glbind.h`.
```c
#define GLBIND_IMPLEMENTATION
#include "glbind.h"

int main()
{
    GLenum result = glbInit(NULL, NULL);
    if (result != GL_NO_ERROR) {
        printf("Failed to initialize glbind.");
        return -1;
    }
    
    ...
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    ...

    glbUninit();
    return 0;
}
```
The example above binds everything to global scope and uses default settings for the internal rendering context. You
can also initialize glbind like the code below.
```c
GLBapi gl;
GLBconfig config = glbConfigInit();
config.singleBuffered = GL_TRUE;    /* Don't use double-buffering on the internal rendering context. */
GLenum result = glbInit(&gl, &config);
if (result != GL_NO_ERROR) {
    ... error initializing glbind ...
}

#if defined(GLBIND_WGL)
HGLRC hRC = glbGetRC();
... do something with hRC ...
#endif

#if defined(GLBIND_GLX)
GLXContext rc = glbGetRC();
... do something with rc ...
#endif

/* Draw something using local function pointers in the "gl" object instead of global scope. */
gl.glClearColor(0, 0, 0, 0);
gl.glClear(GL_COLOR_BUFFER_BIT);
```
Since OpenGL requires a rendering context in order to retrieve function pointers, it makes sense to give the client
access to it so they can avoid wasting time and memory creating their own rendering context unnecessarily. Therefore,
glbind allows you to configure the internal rendering context and retrieve a handle to it so the application can
make use of it.

You can also initialize a `GLBapi` object against the current context (previously set with wglMakeCurrent or
glXMakeCurrent) using `glbInitContextAPI()` or `glbInitCurrentContextAPI()`. Note, however, that before calling these
functions you must have previously called `glbInit()`. These also do not automatically bind anything to global scope.

You can explicitly bind the function pointers in a `GLBapi` object to global scope by using `glbBindAPI()`.

License
=======
Public domain or MIT-0 (No Attribution). Choose whichever you prefer.