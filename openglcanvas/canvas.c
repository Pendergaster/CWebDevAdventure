/************************************************************
 * Check license.txt in project root for license information *
 *********************************************************** */
#include <stdlib.h>
#include <stdio.h>
#include "include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <assert.h>

#include "../defs.h"

u32 SCREENWIDHT = 1000;
u32 SCREENHEIGHT = 1000;

GLenum glCheckError_(const char *file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        const char* error = NULL;
        switch (errorCode) {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        printf("GL ERROR file %s line %d error: %s \n", file, line,error);
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

static void error_callback(int e, const char *d) {
    printf("Error %d: %s\n", e, d);
}

const char* vertex = {

    "#version 330 core                                                   \n"
        "layout (location = 1) in vec2   pos;                                \n"
        "uniform float time;                                                 \n"
        "uniform mat4 P;                                                     \n"
        "out DATA                                                            \n"
        "{                                                                   \n"
        "    float    time;                                                  \n"
        "} vert_out;                                                         \n"
        "void main()                                                         \n"
        "{                                                                   \n"
        "    gl_Position = vec4(pos, 0, 1);                                  \n"
        "    vert_out.time = time;                                           \n"
        "}                                                                   \n"
};


const char* fragment = {

    "#version 330 core                                                   \n"
        "in DATA                                                             \n"
        "{                                                                   \n"
        "    float    time;                                                  \n"
        "} vert_in;                                                          \n"
        "out vec4 color;                                                     \n"
        "void main()                                                         \n"
        "{                                                                   \n"
        "    color = vec4(time, time, time, time);                           \n"
        "}                                                                   \n"
};

u32 compile_shader(u32 glenum, const char* source) {
    i32 compilecheck = 0;
    u32 shader = glCreateShader(glenum);
    if (shader == 0) exit(EXIT_FAILURE);

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compilecheck);

    if (!compilecheck) {
        i32 infolen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolen);
        if (infolen > 1) {
            char* infoLog = malloc(sizeof(char) * infolen);
            glGetShaderInfoLog(shader, infolen, NULL, infoLog);
            printf("Error compiling shader :\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        exit(EXIT_FAILURE);
    }
    return shader;
}


int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCREENWIDHT, SCREENHEIGHT, "CTech", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create window\n");
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        exit(EXIT_FAILURE);
        return 0;
    }

    glViewport(0, 0, SCREENWIDHT, SCREENHEIGHT);
    glfwSetErrorCallback(error_callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    {
        u32 vID = compile_shader(GL_VERTEX_SHADER, vertex);
#if 0
        free(vertS);
        char* fragS = load_file(frag, NULL);
        uint fID = compile_shader(GL_FRAGMENT_SHADER, fragS);
        free(fragS);
        shader.progId = glCreateProgram();
        glAttachShader(shader.progId, vID);
        glAttachShader(shader.progId, fID);

        add_attribute(&shader,"uv");
        add_attribute(&shader,"vert");
        add_attribute(&shader,"wdata");
        add_attribute(&shader,"textid");
        add_attribute(&shader, "rotation");

        link_shader(&shader, vID, fID);
        use_shader(&shader);
        unuse_shader(&shader);
#endif
    }
}
