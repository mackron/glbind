#define GLBIND_IMPLEMENTATION
#include "../glbind.h"

int main(int argc, char** argv)
{
    GLBapi gl;
    GLenum result = glbInit(&gl);
    if (result != GL_NO_ERROR) {
        return result;
    }

    glbBindAPI(&gl);    /* Bind the API to global scope. */


    /* Do stuff. */


    glbUninit();

    (void)argc;
    (void)argv;
    return 0;
}