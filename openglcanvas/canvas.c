/************************************************************
 * Check license.txt in project root for license information *
 *********************************************************** */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>

#if !defined(__EMSCRIPTEN__)
#include "include/glad/glad.h"
#include <GLFW/glfw3native.h>
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

#include "../defs.h"

u32 SCREENWIDHT = 860;
u32 SCREENHEIGHT = 640;

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

    "#version 300 es                                                  \n"
        "layout (location = 0) in vec2   pos;                                \n"
        "uniform float time;                                                 \n"
        "uniform mat4 P;                                                     \n"
        
		
		//"out DATA                                                            \n"
        //"{                                                                   \n"
        //"    float    time;                                                  \n"
        //"} vert_out;                                                         \n"
		"out float out_time;"
        
		"void main()                                                         \n"
        "{                                                                   \n"
        "    gl_Position = vec4(pos, 0, 1);                                  \n"
        "    out_time = time;                                           \n"
        "}                                                                   \n"
};


const char* fragment = {

		"#version 300 es                                                    \n"
		"precision highp float;                                                             \n"
        "                                                                   \n"
        "in float out_time;                                                  \n"
        "                                                          \n"
        "out vec4 color;                                                     \n"
        "void main()                                                         \n"
        "{                                                                   \n"
        "    color = vec4(out_time, out_time, out_time, out_time);                           \n"
        "}                                                                   \n"
};

#define STRINGIFY(...) #__VA_ARGS__

const char* star_field_vert = 
#include "shader_star_field_vert.h"
;

const char* star_field_frag = 
#include "shader_star_field_frag.txt"
;

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
            printf("Error compiling shader(%s) :\n%s\n", glenum == GL_VERTEX_SHADER ? "vertex" : "frag", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
    }
    return shader;
}

        // exit(EXIT_FAILURE);

u32 shader_compile(const char* vrx, const char* frag) {
    u32 vID = compile_shader(GL_VERTEX_SHADER,   vrx);
    u32 fID = compile_shader(GL_FRAGMENT_SHADER, frag);
    u32 program = glCreateProgram();
	glAttachShader(program, vID);
	glAttachShader(program, fID);
	glLinkProgram(program);
    glDeleteShader(vID); 
	glDeleteShader(fID);
    return program;
}

static void main_loop();
u32 shader;
u32 shader_time_loc;
GLFWwindow* window;

u32 shader_star;
u32 shader_star_time_loc;

u32 vao = 0;
u32 shader_star_resolution_loc;
// u32 shader_star_time_loc;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(SCREENWIDHT, SCREENHEIGHT, "CTech", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create window\n");
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

#if !defined(__EMSCRIPTEN__)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        exit(EXIT_FAILURE);
        return 0;
    }
#endif 
    glViewport(0, 0, SCREENWIDHT, SCREENHEIGHT);
    glfwSetErrorCallback(error_callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    {
	
        shader = shader_compile(vertex, fragment);
        glUseProgram(shader);
		u32 time_loc = glGetUniformLocation(shader, "time");
		shader_time_loc = time_loc;

		glUseProgram(0);

        shader_star = shader_compile(star_field_vert, star_field_frag);
        glUseProgram(shader_star);
        shader_star_time_loc = glGetUniformLocation(shader_star, "u_time");
        shader_star_resolution_loc = glGetUniformLocation(shader_star, "u_resolution");

		glUseProgram(0);
        // shader_star

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
	
	
#if defined(__EMSCRIPTEN__) 
	emscripten_set_main_loop(main_loop, 0, 0);
#else
	while(1) {
		main_loop();
	}
#endif
	
}

int resolution[2];
/*
void emscripten_resize()
{
    double width, height;
    emscripten_get_element_css_size("canvas", &width, &height);
    emscripten_set_canvas_size((int)width, (int)height);
    // emscripten_set_resize_callback(0, 0, 0, emscWindowSizeChanged);
}
*/
void window_on_resize(int x, int y) {
    printf("hello from c %i %i \n", x, y);
    resolution[0] = x;
    resolution[1] = y;
    glfwSetWindowSize(window, x, y);
    glViewport(0, 0, x, y);
    // emscripten_resize();
}
// void emscWindowSizeChanged(int x, int y) {
// }


float end_time = 0.f;

void main_loop() {
	static u8 init = 1;
    static float time = 0.f;

    float current_time = glfwGetTime();
    float dt = current_time - end_time;
    time += dt * 0.5f;

	if (init) {
		init = 0;
		glGenBuffers(1, &vao);
		glBindVertexArray(vao);
		
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glEnableVertexAttribArray(0);
	    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
		
		float verts[] = {
			0.5f,  0.5f,      // top right
			0.5f, -0.5f,      // bottom right
			-0.5f, -0.5f,     // bottom left
			-0.5f, -0.5f,     // bottom left
			-0.5f,  0.5f,     // top left
			0.5f,  0.5f       // top right
		};
        for (int i = 0; i < 12; i++) {
            verts[i] *= 2;
        }
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glBindVertexArray(vao);
	// glUseProgram(shader);
	// glUniform1f(shader_time_loc, 0.5f);
	// glDrawArrays(GL_TRIANGLES, 0, 6);

    glUseProgram(shader_star);
    glUniform1f(shader_star_time_loc, time);
    
    glUniform2f(shader_star_resolution_loc, (float)resolution[0], (float)resolution[1]);
    // glUniform2f()
	glDrawArrays(GL_TRIANGLES, 0, 6);

    glUseProgram(0);
	
	glfwSwapBuffers (window);
	
#if 0
	printf("%s %i %i\n\n %s\n", star_field_vert, strlen(star_field_vert), 
		strlen("void mainImage( out vec4 fragColor, in vec2 fragCoord ) { vec2 uv=fragCoord.xy/iResolution.xy-.5; uv.y*=iResolution.y/iResolution.x; vec3 dir=vec3(uv*zoom,1.); float time=iTime*speed+.25; float a1=.5+iMouse.x/iResolution.x*2.; float a2=.8+iMouse.y/iResolution.y*2.; mat2 rot1=mat2(cos(a1),sin(a1),-sin(a1),cos(a1)); mat2 rot2=mat2(cos(a2),sin(a2),-sin(a2),cos(a2)); dir.xz*=rot1; dir.xy*=rot2; vec3 from=vec3(1.,.5,0.5); from+=vec3(time*2.,time,-2.); from.xz*=rot1; from.xy*=rot2; float s=0.1,fade=1.; vec3 v=vec3(0.); for (int r=0; r<volsteps; r++) { vec3 p=from+s*dir*.5; p = abs(vec3(tile)-mod(p,vec3(tile*2.))); float pa,a=pa=0.; for (int i=0; i<iterations; i++) { p=abs(p)/dot(p,p)-formuparam; a+=abs(length(p)-pa); pa=length(p); } float dm=max(0.,darkmatter-a*a*.001); a*=a*a; if (r>6) fade*=1.-dm; v+=fade; v+=vec3(s,s*s,s*s*s*s)*a*brightness*fade; fade*=distfading; s+=stepsize; } v=mix(vec3(length(v)),v,saturation); fragColor = vec4(v*.01,1.); }")
			, star_field_frag);
#endif
    end_time = glfwGetTime();
}





















