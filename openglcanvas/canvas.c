/************************************************************
 * Check license.txt in project root for license information *
 *********************************************************** */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#if !defined(__EMSCRIPTEN__)
#include "include/glad/glad.h"
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>

static GLFWwindow* window = 0;
#endif

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/key_codes.h>
#include <GLES3/gl3.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

typedef uint64_t    u64;
typedef int64_t     i64;
typedef uint32_t    u32;
typedef int32_t     i32;
typedef uint16_t    u16;
typedef int16_t     i16;
typedef uint8_t     u8;
typedef int8_t      i8;

u32 SCREENWIDHT  = 860;
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

    "#version 300 es\n"
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
        "    color = vec4(1.0, out_time, out_time, 1.0);                           \n"
        "}                                                                   \n"
};

#define STRINGIFY(...) #__VA_ARGS__

const char* star_field_vert = 
#include "shader_star_field_vert.h"
;

const char* star_field_frag = 
#include "shader_star_field_frag.txt"
;

const char* image_vert_src =
#if defined(__EMSCRIPTEN__)
"#version 300 es\n"
#else
"#version 330 core\n"
#endif
#include "image.vert"
;
const char* image_frag_src =
#if defined(__EMSCRIPTEN__)
"#version 300 es\n"
#else
"#version 330 core\n"
#endif
#include "image.frag"
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
static void motocross_debug_render();
u32 shader;
u32 shader_time_loc;

u32 shader_star;
u32 shader_star_time_loc;
u32 shader_star_resolution_loc;

u32 shader_image;
u32 shader_image_sampler_loc;

u32 vao = -1;
u32 image_vao = -1;
// u32 shader_star_time_loc;

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}
static void motocross_game_render();

typedef enum {
    Key_W, Key_S, Key_A, Key_D, Key_Space, 
    Key_Max
} Keycode;


#if !defined (__EMSCRIPTEN__)
static void init_gl_desktop() {
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
    }
#endif 
    glfwSetErrorCallback(error_callback);

}

// quick and dirty inputs
static u8 isKeyDown(Keycode key) {
    int scancode;
    switch (key) {
        case Key_W: 
            scancode = GLFW_KEY_W;
            break;
        case Key_S: 
            scancode = GLFW_KEY_S;
			break;
        case Key_A: 
            scancode = GLFW_KEY_A;
			break;
        case Key_D: 
            scancode = GLFW_KEY_D;
			break;
        case Key_Space:
            scancode = GLFW_KEY_SPACE;
			break;
        default:
            return;
    }

    return glfwGetKey(window, scancode) == GLFW_PRESS;
}
#endif

#if defined(__EMSCRIPTEN__)
u8 keys_down[Key_Max] = { 0 };
EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent* e, void* userData)
{
    // int dom_pk_code = emscripten_compute_dom_pk_code(e->code);

    Keycode code = Key_Max;
    switch (e->keyCode)
    {
    case DOM_VK_SPACE:
        code = Key_Space;
        break;
    case DOM_VK_A:
        code = Key_A;
        break;
    case DOM_VK_D:
        code = Key_D;
        break;
    case DOM_VK_W:
        code = Key_W;
        break;
    case DOM_VK_S:
        code = Key_S;
        break;
    default:
        return 0;  
    }

    if (eventType  == EMSCRIPTEN_EVENT_KEYUP) {
        printf("%i up\n", code);
        keys_down[code] = 0;
    } else if (eventType  == EMSCRIPTEN_EVENT_KEYDOWN) {
        printf("%i down\n", code);
        keys_down[code] = 1;
    }

    return 0;
}

// quick and dirty inputs
static u8 isKeyDown(Keycode key) {
    if (key >= 0 && key < Key_Max) {
        return keys_down[key];
    }
    return 0;
}

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
static void init_webgl() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = 0;
#if MAX_WEBGL_VERSION >= 2
#endif
    attrs.majorVersion = 2;

    glContext = emscripten_webgl_create_context("canvas", &attrs);
    if (!glContext) {
        printf("gl context cretion failed\n");
    }

    emscripten_webgl_make_context_current(glContext);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, key_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, key_callback);
    emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 1, key_callback);

}

#endif


int main() {

#if defined(__EMSCRIPTEN__)
    init_webgl();
#else
    init_gl_desktop();
#endif

    glViewport(0, 0, SCREENWIDHT, SCREENHEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(1.0, 1.0, 1.0, 1.0);

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

        shader_image = shader_compile(image_vert_src, image_frag_src);
        shader_image_sampler_loc = glGetUniformLocation(shader_image, "image");

		glUseProgram(0);
    }
	
	
#if defined(__EMSCRIPTEN__) 
	emscripten_set_main_loop(main_loop, 0, 0);
#else
	while(1) {
		main_loop();
	}
#endif
    
    // TODO: clean up mby
    return 0;
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

#if defined(__EMSCRIPTEN__)
EM_JS(int, canvas_x, (), {
    return window.innerWidth;
});

EM_JS(int, canvas_y, (), {
    return window.innerHeight;
});

    // Module.canvas.width = x;
    // Module.canvas.height = y;
EM_JS(void, canvas_set_size, (int x, int y), {
    canvas.width = x;
    canvas.height = y;
});
#endif

void window_on_resize(int x, int y) {
    // printf("hello from c %i %i \n", x, y);
    resolution[0] = x;
    resolution[1] = y;
    
#if defined(_WIN32)
    glfwSetWindowSize(window, x, y);
#else
    canvas_set_size(x, y);
#endif
    glViewport(0, 0, x, y);
}

typedef enum {
    Background_motocross,
    Background_star,
    Background_max
} Background;
float last_time = 0.f;
static void motocross_update_and_render(float dt);
Background background = Background_star;

void main_loop() {
	static u8 init = 1;
    static float timer = 0.f;

#if defined(__EMSCRIPTEN__)
    float current_time = emscripten_performance_now() / 1000.f;
#else
    glfwPollEvents();
    float current_time = glfwGetTime();
#endif


    float dt = current_time - last_time;
    last_time = current_time;
    timer += dt * 0.5f;



    // printf("dt %f\n", dt);

	if (init) {
#if defined(__EMSCRIPTEN__)
        int x = canvas_x();
        int y = canvas_y();
        window_on_resize(x, y);
        printf("error %i %i\n", x, y);
#endif

        srand(time(NULL));
        background = rand() % Background_max;
        background = Background_motocross;
		init = 0;
		glGenVertexArrays(1, &vao);
		GLuint vbo;
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		
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

        glBindVertexArray(0);

        glGenVertexArrays(1, &image_vao);
        glBindVertexArray(image_vao);
        u32 vbo2;
		glGenBuffers(1, &vbo2);
		glBindBuffer(GL_ARRAY_BUFFER, vbo2);
		glEnableVertexAttribArray(0);
	    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);


		float uvs[] = {
			1.0f,  1.0f,     // top right
		    1.0f,  0.0f,     // bottom right
			0.0f,  0.0f,     // bottom left
			0.0f,  0.0f,     // bottom left
			0.0f,  1.0f,     // top left
			1.0f,  1.0f      // top right
		};
        u32 vbo_image;
        glGenBuffers(1, &vbo_image);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_image);
		glEnableVertexAttribArray(1);
	    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
	}
	
    glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (background == Background_motocross) {
        glBindVertexArray(image_vao);
        motocross_update_and_render(dt * 100.f);
	    glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else if (background == Background_star) {
        glUseProgram(shader_star);
        glUniform1f(shader_star_time_loc, timer);
#if defined _WIN32
		resolution[0] = SCREENWIDHT;
		resolution[1] = SCREENHEIGHT;
#endif
		glUniform2f(shader_star_resolution_loc, (float)resolution[0], (float)resolution[1]);
		glDrawArrays(GL_TRIANGLES, 0, 6);
    }

#if 0 // test cody
	glUseProgram(shader);
	glUniform1f(shader_time_loc, 0.5f);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
#endif
  
	
#if defined(_WIN32)
	glfwSwapBuffers (window);
    glfwSwapInterval(1);
#endif
	
#if 0
	printf("%s %i %i\n\n %s\n", star_field_vert, strlen(star_field_vert), 
		strlen("void mainImage( out vec4 fragColor, in vec2 fragCoord ) { vec2 uv=fragCoord.xy/iResolution.xy-.5; uv.y*=iResolution.y/iResolution.x; vec3 dir=vec3(uv*zoom,1.); float time=iTime*speed+.25; float a1=.5+iMouse.x/iResolution.x*2.; float a2=.8+iMouse.y/iResolution.y*2.; mat2 rot1=mat2(cos(a1),sin(a1),-sin(a1),cos(a1)); mat2 rot2=mat2(cos(a2),sin(a2),-sin(a2),cos(a2)); dir.xz*=rot1; dir.xy*=rot2; vec3 from=vec3(1.,.5,0.5); from+=vec3(time*2.,time,-2.); from.xz*=rot1; from.xy*=rot2; float s=0.1,fade=1.; vec3 v=vec3(0.); for (int r=0; r<volsteps; r++) { vec3 p=from+s*dir*.5; p = abs(vec3(tile)-mod(p,vec3(tile*2.))); float pa,a=pa=0.; for (int i=0; i<iterations; i++) { p=abs(p)/dot(p,p)-formuparam; a+=abs(length(p)-pa); pa=length(p); } float dm=max(0.,darkmatter-a*a*.001); a*=a*a; if (r>6) fade*=1.-dm; v+=fade; v+=vec3(s,s*s,s*s*s*s)*a*brightness*fade; fade*=distfading; s+=stepsize; } v=mix(vec3(length(v)),v,saturation); fragColor = vec4(v*.01,1.); }")
			, star_field_frag);
#endif
}

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y;
} Vec2;

static Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2) { a.x + b.x, a.y + b.y };
}
static Vec2 vec2_mul_scl(Vec2 a, float scalar) {
    return (Vec2) { a.x* scalar, a.y* scalar };
}
static Vec2 vec2_norm(Vec2 a) {
    if ( (a.x * a.x + a.y * a.y) <= 0.f) {
        return (Vec2) { 0.f, 0.f };
    }
    float len = sqrt(a.x * a.x + a.y * a.y);
    return (Vec2) { len* a.x, len* a.y };
}
float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.x * b.y;
}

typedef struct Rgb {
    u8 r, g, b;
} Rgb;

Rgb* height_map = 0;
Rgb* color_map = 0;
#define SCREEN_H 640  
#define SCREEN_W 860  
u8* screen = 0;
int h_map_width = 0;
int h_map_fmt = 0;
int c_map_width = 0;
int c_map_fmt = 0;


void draw_vertical_line(int x, int height_on_screen, int screen_h, Rgb col);

static Rgb lerp_col(Rgb a, Rgb b, float t) {
    return  (Rgb) { lerp(a.r, b.r, t), lerp(a.g, b.g, t), lerp(a.b, b.b, t) };
}

static int debug_frame = 0;
static void render_rot(Vec2 p, float phi, int h, int horizon,
    int scale_height, int distance, int screen_w, int screen_h) {


    int iters = 0;

    float sinphi = sin(phi);
    float cosphi = cos(phi);

    // clear screen
    // TODO: Add sky
    memset(screen, 0, SCREEN_H * SCREEN_W * 3);
    int ybuffer[SCREEN_W] = { 0 };

    for (int i = 0; i < screen_w; i++) {
        ybuffer[i] = screen_h;
    }
    float dz = 1.f;
    float z = 1.f;


    while (z < distance) {
        Vec2 pleft = {
            (-cosphi * z - sinphi * z) + p.x,
            (sinphi * z - cosphi * z) + p.y
        };
        Vec2 pright = {
            (cosphi * z - sinphi * z) + p.x,
            (-sinphi * z - cosphi * z) + p.y
        };

        float dx = (pright.x - pleft.x) / screen_w;
        float dy = (pright.y - pleft.y) / screen_w;


        for (int i = 0; i < screen_w; i++) {
            int x = (int)pleft.x;
            int y = (int)pleft.y;                                          

            // 10^2 == 1024
            // 1024-1
            int index = ((y & 1023) << 10) + (x & 1023);


            int ind = index;
            float height_on_screen = (h - height_map[ind].r) / z * scale_height + horizon;

            draw_vertical_line(i, height_on_screen, (int)ybuffer[i], color_map[ind]);

            if (height_on_screen < ybuffer[i]) {
                  ybuffer[i] = height_on_screen;
            }
            pleft.x += dx;
            pleft.y += dy;
        }

        z  += dz;
        dz += 0.01;

        if (++iters == debug_frame)
            return;
    }


#if 0
    Rgb start = { 189.f / 255.f, 222.f / 255.f, 255.f/255.f };
    Rgb end   = { 0.f, 0.f, 255.f };
    for (int j = 0; j < SCREEN_H; j++) {
        for (int i = 0; i < SCREEN_W; i++) {

            if (ybuffer[i] < j)
                continue;

            float t = (float)j / SCREEN_H;
            Rgb c = lerp_col(start, end, t);
            screen[(i * 3) + (j * SCREEN_W * 3) + 0] = c.r;
            screen[(i * 3) + (j * SCREEN_W * 3) + 1] = c.g;
            screen[(i * 3) + (j * SCREEN_W * 3) + 2] = c.b;
        }
    }
#endif
}

#include "motocross.h"

void draw_vertical_line(int x, int top, int bot, Rgb col) {

    if (top < 0) {
        top = 0;
    }

    for (int i = top; i < bot; i++) {
        screen[(x * 3) + (i * SCREEN_W * 3) + 0] = col.r;
        screen[(x * 3) + (i * SCREEN_W * 3) + 1] = col.g;
        screen[(x * 3) + (i * SCREEN_W * 3) + 2] = col.b;
    }
}

static void* png_load(const char* filename, int* x_out, int* y_out, int* fmt_out);
static void motocross_debug_render();
static void motocross_update_and_render(float dt) {
   

    static u8 load_image = 1;
    if (load_image) {
        int y, fmt;
        color_map  = png_load("C6.png",  &c_map_width, &y, &c_map_fmt);
	    height_map = png_load("D62.png",  &h_map_width, &y, &h_map_fmt);

		load_image = 0;
        screen = malloc(SCREEN_W * SCREEN_H * 3 * sizeof(u8));
    }

    motocross_update(dt);
    motocross_render();
    motocross_debug_render();
}

static void motocross_debug_render() {

    static u32 texture;
    static u8 init = 1;
   
    if (init) {
        window_on_resize(SCREENWIDHT, SCREENHEIGHT);
        init = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_W, SCREEN_H, 0, GL_RGB, GL_UNSIGNED_BYTE,
            screen);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {

        if (motocross.rerender) {
            glTexSubImage2D(GL_TEXTURE_2D,
                0,
                0,
                0,
                SCREEN_W,
                SCREEN_H,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                screen);
        }
    }

    glUseProgram(shader_image);
    glUniform1i(shader_image_sampler_loc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
}



static void* png_load(const char* filename, int* x_out, int* y_out, int* fmt_out) {
    FILE* f = fopen(filename, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        int size = ftell(f);
        fseek(f, 0, SEEK_SET);
        void* mem = malloc(size);
        fread(mem, size, 1, f);

        void* rst = stbi_load_from_memory(mem, size, x_out, y_out, fmt_out, 0);
        printf("loaded image %s: x: %i, y: %i, fmt: %i\n", filename, *x_out, *y_out, *fmt_out);

        fclose(f);
        return rst;

    }
    return 0;
}












