#include "VrApi.h"
#include "VrApi_Helpers.h"
#include "VrApi_Input.h"
#include "VrApi_SystemUtils.h"
#include "android_native_app_glue.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android/window.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <rabbit/opengl/drawer.h>
#include <rabbit/opengl/shader_collection.h>
#include <rabbit/geom/surface.h>
#include <rabbit/mesh.h>
#include <rabbit/util.h>


#include <map>
#include <chrono>

#define error(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#ifndef NDEBUG
#define info(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#endif // NDEBUG

static const char* TAG = "hello_quest";
//ovrPosef PointerPose_;
std::map<int, ovrPosef> PoseMap_;

static const char*
egl_get_error_string(EGLint error)
{
    switch (error)
    {
    case EGL_SUCCESS:
        return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED:
        return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS:
        return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:
        return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:
        return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONTEXT:
        return "EGL_BAD_CONTEXT";
    case EGL_BAD_CONFIG:
        return "EGL_BAD_CONFIG";
    case EGL_BAD_CURRENT_SURFACE:
        return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:
        return "EGL_BAD_DISPLAY";
    case EGL_BAD_SURFACE:
        return "EGL_BAD_SURFACE";
    case EGL_BAD_MATCH:
        return "EGL_BAD_MATCH";
    case EGL_BAD_PARAMETER:
        return "EGL_BAD_PARAMETER";
    case EGL_BAD_NATIVE_PIXMAP:
        return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:
        return "EGL_BAD_NATIVE_WINDOW";
    case EGL_CONTEXT_LOST:
        return "EGL_CONTEXT_LOST";
    default:
        abort();
    }
}

static const char*
gl_get_framebuffer_status_string(GLenum status)
{
    switch (status)
    {
    case GL_FRAMEBUFFER_UNDEFINED:
        return "GL_FRAMEBUFFER_UNDEFINED";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "GL_FRAMEBUFFER_UNSUPPORTED";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
    default:
        abort();
    }
}

struct egl
{
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
};

static void
egl_create(struct egl* egl)
{
    info("get EGL display");
    egl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl->display == EGL_NO_DISPLAY)
    {
        error("can't get EGL display: %s", egl_get_error_string(eglGetError()));
        exit(EXIT_FAILURE);
    }

    info("initialize EGL display");
    if (eglInitialize(egl->display, NULL, NULL) == EGL_FALSE)
    {
        error("can't initialize EGL display: %s",
              egl_get_error_string(eglGetError()));
        exit(EXIT_FAILURE);
    }

    info("get number of EGL configs");
    EGLint num_configs = 0;
    if (eglGetConfigs(egl->display, NULL, 0, &num_configs) == EGL_FALSE)
    {
        error("can't get number of EGL configs: %s",
              egl_get_error_string(eglGetError()));
        exit(EXIT_FAILURE);
    }

    info("allocate EGL configs");
    EGLConfig* configs = (EGLConfig*) malloc(num_configs * sizeof(EGLConfig));
    if (configs == NULL)
    {
        error("cant allocate EGL configs: %s",
              egl_get_error_string(eglGetError()));
        exit(EXIT_FAILURE);
    }

    info("get EGL configs");
    if (eglGetConfigs(egl->display, configs, num_configs, &num_configs) ==
            EGL_FALSE)
    {
        error("can't get EGL configs: %s", egl_get_error_string(eglGetError()));
        exit(EXIT_FAILURE);
    }

    info("choose EGL config");
    static const EGLint CONFIG_ATTRIBS[] =
    {
        EGL_RED_SIZE,   8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE,    8,
        EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0,
        EGL_SAMPLES,    0, EGL_NONE,
    };
    EGLConfig found_config = NULL;
    for (int i = 0; i < num_configs; ++i)
    {
        EGLConfig config = configs[i];

        info("get EGL config renderable type");
        EGLint renderable_type = 0;
        if (eglGetConfigAttrib(egl->display, config, EGL_RENDERABLE_TYPE,
                               &renderable_type) == EGL_FALSE)
        {
            error("can't get EGL config renderable type: %s",
                  egl_get_error_string(eglGetError()));
            exit(EXIT_FAILURE);
        }
        if ((renderable_type & EGL_OPENGL_ES3_BIT_KHR) == 0)
        {
            continue;
        }

        info("get EGL config surface type");
        EGLint surface_type = 0;
        if (eglGetConfigAttrib(egl->display, config, EGL_SURFACE_TYPE,
                               &surface_type) == EGL_FALSE)
        {
            error("can't get EGL config surface type: %s",
                  egl_get_error_string(eglGetError()));
            exit(EXIT_FAILURE);
        }
        if ((renderable_type & EGL_PBUFFER_BIT) == 0)
        {
            continue;
        }
        if ((renderable_type & EGL_WINDOW_BIT) == 0)
        {
            continue;
        }

        const EGLint* attrib = CONFIG_ATTRIBS;
        while (attrib[0] != EGL_NONE)
        {
            info("get EGL config attrib");
            EGLint value = 0;
            if (eglGetConfigAttrib(egl->display, config, attrib[0], &value) ==
                    EGL_FALSE)
            {
                error("can't get EGL config attrib: %s",
                      egl_get_error_string(eglGetError()));
                exit(EXIT_FAILURE);
            }
            if (value != attrib[1])
            {
                break;
            }
            attrib += 2;
        }
        if (attrib[0] != EGL_NONE)
        {
            continue;
        }

        found_config = config;
        break;
    }
    if (found_config == NULL)
    {
        error("can't choose EGL config");
        exit(EXIT_FAILURE);
    }

    info("free EGL configs");
    free(configs);

    info("create EGL context");
    static const EGLint CONTEXT_ATTRIBS[] = { EGL_CONTEXT_CLIENT_VERSION, 3,
                                              EGL_NONE
                                            };
    egl->context = eglCreateContext(egl->display, found_config, EGL_NO_CONTEXT,
                                    CONTEXT_ATTRIBS);
    if (egl->context == EGL_NO_CONTEXT)
    {
        error("can't create EGL context: %s",
              egl_get_error_string(eglGetError()));
        exit(EXIT_FAILURE);
    }

    info("create EGL surface");
    static const EGLint SURFACE_ATTRIBS[] =
    {
        EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE,
    };
    egl->surface =
        eglCreatePbufferSurface(egl->display, found_config, SURFACE_ATTRIBS);
    if (egl->surface == EGL_NO_SURFACE)
    {
        error("can't create EGL pixel buffer surface: %s",
              egl_get_error_string(eglGetError()));
        exit(EXIT_FAILURE);
    }

    info("make EGL context current");
    if (eglMakeCurrent(egl->display, egl->surface, egl->surface,
                       egl->context) == EGL_FALSE)
    {
        error("can't make EGL context current: %s",
              egl_get_error_string(eglGetError()));
    }
}

static void
egl_destroy(struct egl* egl)
{
    info("make EGL context no longer current");
    eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);

    info("destroy EGL surface");
    eglDestroySurface(egl->display, egl->surface);

    info("destroy EGL context");
    eglDestroyContext(egl->display, egl->context);

    info("terminate EGL display");
    eglTerminate(egl->display);
}

struct framebuffer
{
    int swap_chain_index;
    int swap_chain_length;
    GLsizei width;
    GLsizei height;
    ovrTextureSwapChain* color_texture_swap_chain;
    GLuint* depth_renderbuffers;
    GLuint* framebuffers;
};

static void
framebuffer_create(struct framebuffer* framebuffer, GLsizei width,
                   GLsizei height)
{
    framebuffer->swap_chain_index = 0;
    framebuffer->width = width;
    framebuffer->height = height;

    info("create color texture swap chain");
    framebuffer->color_texture_swap_chain = vrapi_CreateTextureSwapChain3(
            VRAPI_TEXTURE_TYPE_2D, GL_RGBA8, width, height, 1, 3);
    if (framebuffer->color_texture_swap_chain == NULL)
    {
        error("can't create color texture swap chain");
        exit(EXIT_FAILURE);
    }

    framebuffer->swap_chain_length =
        vrapi_GetTextureSwapChainLength(framebuffer->color_texture_swap_chain);

    info("allocate depth renderbuffers");
    framebuffer->depth_renderbuffers = (GLuint*)
                                       malloc(framebuffer->swap_chain_length * sizeof(GLuint));
    if (framebuffer->depth_renderbuffers == NULL)
    {
        error("can't allocate depth renderbuffers");
        exit(EXIT_FAILURE);
    }

    info("allocate framebuffers");
    framebuffer->framebuffers = (GLuint*)
                                malloc(framebuffer->swap_chain_length * sizeof(GLuint));
    if (framebuffer->framebuffers == NULL)
    {
        error("can't allocate framebuffers");
        exit(EXIT_FAILURE);
    }

    glGenRenderbuffers(framebuffer->swap_chain_length,
                       framebuffer->depth_renderbuffers);
    glGenFramebuffers(framebuffer->swap_chain_length,
                      framebuffer->framebuffers);
    for (int i = 0; i < framebuffer->swap_chain_length; ++i)
    {
        info("create color texture %d", i);
        GLuint color_texture = vrapi_GetTextureSwapChainHandle(
                                   framebuffer->color_texture_swap_chain, i);
        glBindTexture(GL_TEXTURE_2D, color_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        info("create depth renderbuffer %d", i);
        glBindRenderbuffer(GL_RENDERBUFFER,
                           framebuffer->depth_renderbuffers[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width,
                              height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        info("create framebuffer %d", i);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->framebuffers[i]);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, color_texture, 0);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  framebuffer->depth_renderbuffers[i]);
        GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            error("can't create framebuffer %d: %s", i,
                  gl_get_framebuffer_status_string(status));
            exit(EXIT_FAILURE);
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }
}

static void
framebuffer_destroy(struct framebuffer* framebuffer)
{
    info("destroy framebuffers");
    glDeleteFramebuffers(framebuffer->swap_chain_length,
                         framebuffer->framebuffers);

    info("destroy depth renderbuffers");
    glDeleteRenderbuffers(framebuffer->swap_chain_length,
                          framebuffer->depth_renderbuffers);

    info("free framebuffers");
    free(framebuffer->framebuffers);

    info("free depth renderbuffers");
    free(framebuffer->depth_renderbuffers);

    info("destroy color texture swap chain");
    vrapi_DestroyTextureSwapChain(framebuffer->color_texture_swap_chain);
}

#define ATTRIB_BEGIN 0
#define ATTRIB_POSITION 0
#define ATTRIB_COLOR 1
#define ATTRIB_END 2


#define UNIFORM_BEGIN 0
#define UNIFORM_MODEL_MATRIX 0
#define UNIFORM_VIEW_MATRIX 1
#define UNIFORM_PROJECTION_MATRIX 2
#define UNIFORM_END 3

struct program
{
    GLuint program;
    GLint uniform_locations[UNIFORM_END];
};

static const char* ATTRIB_NAMES[ATTRIB_END] =
{
    "aPosition", "aColor",
};

static const char* UNIFORM_NAMES[UNIFORM_END] =
{
    "uModelMatrix", "uViewMatrix", "uProjectionMatrix",
};

static const char VERTEX_SHADER[] =
    "#version 300 es\n"
    "\n"
    "in vec3 aPosition;\n"
    "in vec3 aColor;\n"
    "uniform mat4 uModelMatrix;\n"
    "uniform mat4 uViewMatrix;\n"
    "uniform mat4 uProjectionMatrix;\n"
    "\n"
    "out vec3 vColor;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = uProjectionMatrix * ( uViewMatrix * ( uModelMatrix * vec4( "
    "aPosition * 0.1, 1.0 ) ) );\n"
    "	vColor = aColor;\n"
    "}\n";

static const char FRAGMENT_SHADER[] = "#version 300 es\n"
                                      "\n"
                                      "in lowp vec3 vColor;\n"
                                      "out lowp vec4 outColor;\n"
                                      "void main()\n"
                                      "{\n"
                                      "	outColor = vec4(vColor, 1.0);\n"
                                      "}\n";

static const char VERTEX_SHADER_2[] =
    "#version 300 es\n"
    "\n"
    "in vec3 aPosition;\n"
    "in vec3 aColor;\n"
    "uniform mat4 uModelMatrix;\n"
    "uniform mat4 uViewMatrix;\n"
    "uniform mat4 uProjectionMatrix;\n"
    "\n"
    "out vec3 vColor;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = uProjectionMatrix * ( uViewMatrix * ( uModelMatrix * vec4( "
    "aPosition * 0.1, 1.0 ) ) );\n"
    "   vColor = aColor;\n"
    "}\n";

static const char FRAGMENT_SHADER_2[] = "#version 300 es\n"
                                        "\n"
                                        "in lowp vec3 vColor;\n"
                                        "out lowp vec4 outColor;\n"
                                        "void main()\n"
                                        "{\n"
                                        " outColor = vec4(vColor, 1.0);\n"
                                        "}\n";


static GLuint
compile_shader(GLenum type, const char* string)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &string, NULL);
    glCompileShader(shader);
    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* log = (char*) malloc(length);
        glGetShaderInfoLog(shader, length, NULL, log);
        error("can't compile shader: %s", log);
        exit(EXIT_FAILURE);
    }
    return shader;
}

static void
program_create(struct program* program)
{
    program->program = glCreateProgram();
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, VERTEX_SHADER);
    glAttachShader(program->program, vertex_shader);
    GLuint fragment_shader =
        compile_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER);
    glAttachShader(program->program, fragment_shader);
    for (uint8_t attrib = ATTRIB_BEGIN; attrib != ATTRIB_END; ++attrib)
    {
        glBindAttribLocation(program->program, attrib, ATTRIB_NAMES[attrib]);
    }
    glLinkProgram(program->program);
    GLint status = 0;
    glGetProgramiv(program->program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length = 0;
        glGetProgramiv(program->program, GL_INFO_LOG_LENGTH, &length);
        char* log = (char*) malloc(length);
        glGetProgramInfoLog(program->program, length, NULL, log);
        error("can't link program: %s", log);
        exit(EXIT_FAILURE);
    }
    for (uint8_t uniform = UNIFORM_BEGIN; uniform != UNIFORM_END;
            ++uniform)
    {
        program->uniform_locations[uniform] =
            glGetUniformLocation(program->program, UNIFORM_NAMES[uniform]);
    }
}

static void
program_destroy(struct program* program)
{
    glDeleteProgram(program->program);
}

struct attrib_pointer
{
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    const GLvoid* pointer;
};

struct vertex
{
    float position[3];
    float color[3];
};

struct geometry
{
    GLuint vertex_array;
    GLuint vertex_buffer;
    GLuint index_buffer;
};

static const struct attrib_pointer ATTRIB_POINTERS[ATTRIB_END] =
{
    {
        3, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
        (const GLvoid*)offsetof(struct vertex, position)
    },
    {
        3, GL_FLOAT, GL_FALSE, sizeof(struct vertex),
        (const GLvoid*)offsetof(struct vertex, color)
    },
};

static void
geometry_create(struct geometry* geometry)
{
    glGenVertexArrays(1, &geometry->vertex_array);
    glBindVertexArray(geometry->vertex_array);
    glGenBuffers(1, &geometry->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, geometry->vertex_buffer);
    for (uint8_t attrib = ATTRIB_BEGIN; attrib != ATTRIB_END; ++attrib)
    {
        struct attrib_pointer attrib_pointer = ATTRIB_POINTERS[attrib];
        glEnableVertexAttribArray(attrib);
        glVertexAttribPointer(attrib, attrib_pointer.size, attrib_pointer.type,
                              attrib_pointer.normalized, attrib_pointer.stride,
                              attrib_pointer.pointer);
    }
    glGenBuffers(1, &geometry->index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->index_buffer);
    glBindVertexArray(0);
}

static void
geometry_destroy(struct geometry* geometry)
{
    glDeleteBuffers(1, &geometry->index_buffer);
    glDeleteBuffers(1, &geometry->vertex_buffer);
    glDeleteVertexArrays(1, &geometry->vertex_array);
}

struct renderer
{
    struct framebuffer framebuffers[VRAPI_FRAME_LAYER_EYE_MAX];
    struct program program;
    struct geometry geometry;

    rabbit::opengl_drawer rabbit_drawer;
};

static void
renderer_create(struct renderer* renderer, GLsizei width, GLsizei height)
{
    for (int i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; ++i)
    {
        framebuffer_create(&renderer->framebuffers[i], width, height);
    }
    program_create(&renderer->program);
    geometry_create(&renderer->geometry);
}

static void
renderer_destroy(struct renderer* renderer)
{
    geometry_destroy(&renderer->geometry);
    program_destroy(&renderer->program);
    for (int i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; ++i)
    {
        framebuffer_destroy(&renderer->framebuffers[i]);
    }
}

double mticks()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (double) tv.tv_usec / 1000 + tv.tv_sec * 1000;
}

auto draw_mesh(rabbit::opengl_drawer & drawer, rabbit::mesh & mesh)
{

    std::vector<std::pair<rabbit::vec3, rabbit::vec3>> vertices;

    for (auto & v : mesh.vertices)
    {
        auto r = (double)std::rand() / (double)(RAND_MAX);
        auto  g = (double)std::rand() / (double)(RAND_MAX);
        auto b = (double)std::rand() / (double)(RAND_MAX);
        vertices.push_back({ v, {r, g, b} });
    }

    return vertices;
}

void draw_mesh_2(rabbit::opengl_drawer & drawer, rabbit::mesh & mesh, std::vector<std::pair<rabbit::vec3, rabbit::vec3>> vertices, rabbit::mat4 model, int loc) 
{
    drawer.uniform_mat4f(loc, model);
    drawer.draw_triangles(
        (float*)vertices.data(), vertices.size(),
        (uint32_t*)mesh.triangles.data(), mesh.triangles.size());
} 

rabbit::opengl_shader_program sprg(
    VERTEX_SHADER_2,
    FRAGMENT_SHADER_2);
bool button_pressed = false;
std::vector<rabbit::pose3> ring_positions;
std::vector<rabbit::pose3> hand_positions;
std::vector<rabbit::pose3> last_hand_positions;
std::vector<std::pair<rabbit::vec3, rabbit::vec3>> vertices2;
std::vector<std::pair<rabbit::vec3, rabbit::vec3>> vertices_sphere;
static ovrLayerProjection2
renderer_render_frame(struct renderer* renderer, ovrTracking2* tracking)
{
    ovrMatrix4f model_matrix = ovrMatrix4f_CreateTranslation(0.0, 0.0, -1);
    model_matrix = ovrMatrix4f_Transpose(&model_matrix);

    ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
    layer.Header.Flags |=
        VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
    layer.HeadPose = tracking->HeadPose;

    for (int i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; ++i)
    {
        ovrMatrix4f view_matrix =
            ovrMatrix4f_Transpose(&tracking->Eye[i].ViewMatrix);
        ovrMatrix4f projection_matrix =
            ovrMatrix4f_Transpose(&tracking->Eye[i].ProjectionMatrix);

        struct framebuffer* framebuffer = &renderer->framebuffers[i];
        layer.Textures[i].ColorSwapChain =
            framebuffer->color_texture_swap_chain;
        layer.Textures[i].SwapChainIndex = framebuffer->swap_chain_index;
        layer.Textures[i].TexCoordsFromTanAngles =
            ovrMatrix4f_TanAngleMatrixFromProjection(
                &tracking->Eye[i].ProjectionMatrix);

        glBindFramebuffer(
            GL_DRAW_FRAMEBUFFER,
            framebuffer->framebuffers[framebuffer->swap_chain_index]);

        //glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glViewport(0, 0, framebuffer->width, framebuffer->height);
        glScissor(0, 0, framebuffer->width, framebuffer->height);

        renderer->rabbit_drawer.clean(0.2f, 0.3f, 0.3f, 1.0f);
        auto & drawer = renderer->rabbit_drawer;

        auto surf_sphere = rabbit::sphere_surface(0.5);
        //auto surf = rabbit::parabolic_surface(0.5, 0.5);
        auto surf = rabbit::torus_surface(2, 0.3);
        auto mesh = rabbit::surface_rubic_mesh(surf, 40, 40);
        auto mesh_sphere = rabbit::surface_rubic_mesh(surf_sphere, 40, 40);


        drawer.set_buffers(
            renderer->geometry.vertex_array,
            renderer->geometry.vertex_buffer,
            renderer->geometry.index_buffer);


        float T = 10;
        GLfloat vertices[] =
        {
            T,  T, 0.99999f * 10, 1., 1., 1., // Top Right
            T, -T, 0.99999f * 10, 1., 1., 1., // Bottom Right
            -T, -T, 0.99999f * 10, 1., 1., 1., // Bottom Left
            -T,  T, 0.99999f * 10, 1., 1., 1., // Top Left
        };
        GLuint indices[] =    // Note that we start from 0!
        {
            0, 1, 3,  // First Triangle
            1, 2, 3   // Second Triangle
        };

        glUseProgram(renderer->program.program);

        int model_matrix_loc = renderer->program.uniform_locations[UNIFORM_MODEL_MATRIX];
        int view_matrix_loc = renderer->program.uniform_locations[UNIFORM_VIEW_MATRIX];
        int proj_matrix_loc = renderer->program.uniform_locations[UNIFORM_PROJECTION_MATRIX];

        drawer.uniform_mat4f(model_matrix_loc, (const GLfloat*)&model_matrix);
        drawer.uniform_mat4f(view_matrix_loc, (const GLfloat*)&view_matrix);
        drawer.uniform_mat4f(proj_matrix_loc, (const GLfloat*)&projection_matrix);


        drawer.set_vertices_stride(6);

        glBindVertexArray(renderer->geometry.vertex_array);
#if 0
        if (vertices2.size() == 0)
        {
            for (auto & v : mesh.vertices)
            {
                auto r = (double)std::rand() / (double)(RAND_MAX);
                auto  g = (double)std::rand() / (double)(RAND_MAX);
                auto b = (double)std::rand() / (double)(RAND_MAX);
                vertices2.push_back({ v, {r, g, b} });
            }


            for (auto & v : mesh_sphere.vertices)
            {
                auto r = (double)std::rand() / (double)(RAND_MAX);
                auto  g = (double)std::rand() / (double)(RAND_MAX);
                auto b = (double)std::rand() / (double)(RAND_MAX);
                vertices_sphere.push_back({ v, {r, g, b} });
            }
        }

        static uint8_t inited = 0;
        if (!inited)
        {

            for (int i = 0; i < 8; ++i)
            {
                float strt = -1;
                float fini = 1;

                float k = (float)i / (float)(8 - 1);
                float pos = strt * k + fini * (1 - k);

                ring_positions.push_back(rabbit::mov3({0, 0, pos}));
            }

            hand_positions.resize(2);
            last_hand_positions.resize(2);
            inited = 1;
        }


        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto time = std::chrono::duration_cast<std::chrono::seconds>(now).count();

        static double start_time = mticks() / 1000;

        linalg::vec<float, 3> ppp = { 0, 0, sin(mticks() / 1000 - start_time) };

        drawer.uniform_mat4f(model_matrix_loc, rabbit::mov3(ppp).to_mat4());

        drawer.draw_triangles(
            (float*)vertices_sphere.data(), vertices_sphere.size(),
            (uint32_t*)mesh_sphere.triangles.data(), mesh_sphere.triangles.size());


        int ii = 0;
        for (auto & p : PoseMap_)
        {
            auto Pose_ = p.second;

            auto _q = Pose_.Orientation;
            auto _p = Pose_.Position;

            rabbit::pose3 grip
            {
                { _q.x, _q.y, _q.z, _q.w },
                { _p.x, _p.y, _p.z }
            };


            info("{%f,%f,%f,%f}, {%f,%f,%f}", _q.x, _q.y, _q.z, _q.w, _p.x, _p.y, _p.z);


            drawer.uniform_mat4f(model_matrix_loc, grip.to_mat4());

            drawer.draw_triangles(
                (float*)vertices_sphere.data(), vertices_sphere.size(),
                (uint32_t*)mesh_sphere.triangles.data(), mesh_sphere.triangles.size());

            if (length(ppp - grip.lin) < 0.1) start_time = mticks() / 1000;

            hand_positions[ii] = grip;
            ii++;
        }



        if (button_pressed)
            for (int i = 0; i < hand_positions.size(); ++i)
                for (int j = 0; j < ring_positions.size(); ++j)
                {
                    if (length(hand_positions[i].lin - ring_positions[j].lin) < 0.2)
                    {
                        auto change = hand_positions[i] * last_hand_positions[i].inverse();
                        ring_positions[j] = change * ring_positions[j];
                        //ring_positions[j] = rabbit::pose3{{0,0,0,1}, {0,0,0.001}} * ring_positions[j];
                        //nodraw=true;
                    }
                }


        for (int i = 0; i < 8; ++i)
        {

            drawer.uniform_mat4f(model_matrix_loc, ring_positions[i].to_mat4());

            drawer.draw_triangles(
                (float*)vertices2.data(), vertices2.size(),
                (uint32_t*)mesh.triangles.data(), mesh.triangles.size());
        }



        /*drawer.uniform_mat4f(model_matrix_loc, rabbit::pose3().to_mat4());
        drawer.uniform_mat4f(view_matrix_loc, rabbit::pose3().to_mat4());
        drawer.uniform_mat4f(proj_matrix_loc, rabbit::pose3().to_mat4());

        drawer.set_vertices_stride(6);
        drawer.draw_triangles(vertices, 4, indices, 2);*/
#endif
        auto surf0 = rabbit::torus_surface(4, 0.2);
        auto surf2 = rabbit::sphere_surface(2);
        auto surf3 = rabbit::torus_surface(7, 0.2);
        auto surf4 = rabbit::torus_surface(8, 0.2);
        auto surf5 = rabbit::sphere_surface(0.6);

        auto mesh0 = rabbit::surface_rubic_mesh(surf0, 30, 20);
        auto mesh2 = rabbit::surface_rubic_mesh(surf2, 30, 20);
        auto mesh3 = rabbit::surface_rubic_mesh(surf3, 30, 20);
        auto mesh4 = rabbit::surface_rubic_mesh(surf4, 30, 20);
        auto mesh5 = rabbit::surface_rubic_mesh(surf5, 30, 20);

        static int inited2 = 0;


        static std::vector<std::pair<rabbit::vec3, rabbit::vec3>> h0;
        static std::vector<std::pair<rabbit::vec3, rabbit::vec3>> h2;
        static std::vector<std::pair<rabbit::vec3, rabbit::vec3>> h3;
        static std::vector<std::pair<rabbit::vec3, rabbit::vec3>> h4;
        static std::vector<std::pair<rabbit::vec3, rabbit::vec3>> h5;
        if (!inited2) {
            h0=draw_mesh(drawer,mesh0);
            h2=draw_mesh(drawer,mesh2);
            h3=draw_mesh(drawer,mesh3);
            h4=draw_mesh(drawer,mesh4);
            h5=draw_mesh(drawer,mesh5);
            inited2 = 1;
        }

        auto RX = rabbit::rot3({1, 0, 0}, rabbit::deg(90));
        double dtime = mticks()/1000;
        auto model = rabbit::rot3({0, 0, 1}, rabbit::deg(dtime * 16));

        draw_mesh_2(drawer,mesh0,h0,(RX*rabbit::rot3(rabbit::vec3{0.3,0.7,0}, rabbit::deg(20)) * model).to_mat4(),model_matrix_loc);
        draw_mesh_2(drawer,mesh2,h2,(RX*model.inverse()).to_mat4(),model_matrix_loc);
        draw_mesh_2(drawer,mesh3,h3,(RX*model).to_mat4(),model_matrix_loc);
        draw_mesh_2(drawer,mesh4,h4,(RX*rabbit::rot3(rabbit::vec3{0.6,0.2,0}, rabbit::deg(20)) * model.inverse()).to_mat4(),model_matrix_loc);
        draw_mesh_2(drawer,mesh5, h5,(RX*rabbit::mov3({0.55*sin(dtime), 0.55*cos(dtime), 0}) * model).to_mat4(),model_matrix_loc);



        last_hand_positions = hand_positions;

        glBindVertexArray(0);



        glUseProgram(0);








        glClearColor(0.0, 0.0, 0.0, 1.0);
        glScissor(0, 0, 1, framebuffer->height);
        glClear(GL_COLOR_BUFFER_BIT);
        glScissor(framebuffer->width - 1, 0, 1, framebuffer->height);
        glClear(GL_COLOR_BUFFER_BIT);
        glScissor(0, 0, framebuffer->width, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glScissor(0, framebuffer->height - 1, framebuffer->width, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        static const GLenum ATTACHMENTS[] = { GL_DEPTH_ATTACHMENT };
        static const GLsizei NUM_ATTACHMENTS =
            sizeof(ATTACHMENTS) / sizeof(ATTACHMENTS[0]);
        glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, NUM_ATTACHMENTS,
                                ATTACHMENTS);
        glFlush();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        framebuffer->swap_chain_index = (framebuffer->swap_chain_index + 1) %
                                        framebuffer->swap_chain_length;

    }
    return layer;
}

struct app
{
    ovrJava* java;
    struct egl egl;
    struct renderer renderer;
    bool resumed;
    ANativeWindow* window;
    ovrMobile* ovr;
    bool back_button_down_previous_frame;
    uint64_t frame_index;
};

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;

static void
app_on_cmd(struct android_app* android_app, int32_t cmd)
{
    struct app* app = (struct app*)android_app->userData;
    switch (cmd)
    {
    case APP_CMD_START:
        info("onStart()");
        break;
    case APP_CMD_RESUME:
        info("onResume()");
        app->resumed = true;
        break;
    case APP_CMD_PAUSE:
        info("onPause()");
        app->resumed = false;
        break;
    case APP_CMD_STOP:
        info("onStop()");
        break;
    case APP_CMD_DESTROY:
        info("onDestroy()");
        app->window = NULL;
        break;
    case APP_CMD_INIT_WINDOW:
        info("surfaceCreated()");
        app->window = android_app->window;
        break;
    case APP_CMD_TERM_WINDOW:
        info("surfaceDestroyed()");
        app->window = NULL;
        break;
    default:
        break;
    }
}

static void
app_update_vr_mode(struct app* app)
{
    if (app->resumed && app->window != NULL)
    {
        if (app->ovr == NULL)
        {
            ovrModeParms mode_parms = vrapi_DefaultModeParms(app->java);
            mode_parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
            mode_parms.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;
            mode_parms.Display = (size_t)app->egl.display;
            mode_parms.WindowSurface = (size_t)app->window;
            mode_parms.ShareContext = (size_t)app->egl.context;

            info("enter vr mode");
            app->ovr = vrapi_EnterVrMode(&mode_parms);
            if (app->ovr == NULL)
            {
                error("can't enter vr mode");
                exit(EXIT_FAILURE);
            }

            vrapi_SetClockLevels(app->ovr, CPU_LEVEL, GPU_LEVEL);
        }
    }
    else
    {
        if (app->ovr != NULL)
        {
            info("leave vr mode");
            vrapi_LeaveVrMode(app->ovr);
            app->ovr = NULL;
        }
    }
}

static void
app_handle_input(struct app* app)
{
    bool back_button_down_current_frame = false;

    int i = 0;
    ovrInputCapabilityHeader capability;
    while (vrapi_EnumerateInputDevices(app->ovr, i, &capability) >= 0)
    {
        info("devtype: %d", capability.Type);

        if (capability.Type == ovrControllerType_TrackedRemote)
        {
            ovrInputStateTrackedRemote input_state;
            input_state.Header.ControllerType = ovrControllerType_TrackedRemote;
            if (vrapi_GetCurrentInputState(app->ovr, capability.DeviceID,
                                           &input_state.Header) == ovrSuccess)
            {
                back_button_down_current_frame |=
                    input_state.Buttons & ovrButton_Back;
                back_button_down_current_frame |=
                    input_state.Buttons & ovrButton_B;
                back_button_down_current_frame |=
                    input_state.Buttons & ovrButton_Y;
            }

            ovrTracking tracking;
            vrapi_GetInputTrackingState(
                app->ovr, capability.DeviceID,
                0,
                &tracking);

            PoseMap_[capability.DeviceID] = tracking.HeadPose.Pose;
        }

        /*if (capability.Type == ovrControllerType_StandardPointer)
        {
            ovrInputStateStandardPointer input_state;

            GripPose_ = input_state.GripPose;
            PointerPose_ = input_state.PointerPose;
        }*/

        ++i;
    }

    //if (app->back_button_down_previous_frame && !back_button_down_current_frame)
    if (back_button_down_current_frame)
    {
        button_pressed = true;
        //vrapi_ShowSystemUI(app->java, VRAPI_SYS_UI_CONFIRM_QUIT_MENU);
    }
    else
        button_pressed = false;

    app->back_button_down_previous_frame = back_button_down_current_frame;
}

static void
app_create(struct app* app, ovrJava* java)
{
    app->java = java;
    egl_create(&app->egl);
    renderer_create(&app->renderer,
                    vrapi_GetSystemPropertyInt(
                        java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
                    vrapi_GetSystemPropertyInt(
                        java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT));
    app->resumed = false;
    app->window = NULL;
    app->ovr = NULL;
    app->back_button_down_previous_frame = false;
    app->frame_index = 0;
}

static void
app_destroy(struct app* app)
{
    egl_destroy(&app->egl);
    renderer_destroy(&app->renderer);
}

void
android_main(struct android_app* android_app)
{
    ANativeActivity_setWindowFlags(android_app->activity,
                                   AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

    info("attach current thread");
    ovrJava java;
    java.Vm = android_app->activity->vm;
    (*java.Vm).AttachCurrentThread(&java.Env, NULL);
    java.ActivityObject = android_app->activity->clazz;

    info("initialize vr api");
    const ovrInitParms init_parms = vrapi_DefaultInitParms(&java);
    if (vrapi_Initialize(&init_parms) != VRAPI_INITIALIZE_SUCCESS)
    {
        info("can't initialize vr api");
        exit(EXIT_FAILURE);
    }

    struct app app;
    app_create(&app, &java);

    app.renderer.rabbit_drawer.init_opengl_context();

    android_app->userData = &app;
    android_app->onAppCmd = app_on_cmd;
    while (!android_app->destroyRequested)
    {
        for (;;)
        {
            int events = 0;
            struct android_poll_source* source = NULL;
            if (ALooper_pollAll(
                        android_app->destroyRequested || app.ovr != NULL ? 0 : -1,
                        NULL, &events, (void**)&source) < 0)
            {
                break;
            }
            if (source != NULL)
            {
                source->process(android_app, source);
            }

            app_update_vr_mode(&app);
        }

        app_handle_input(&app);

        if (app.ovr == NULL)
        {
            continue;
        }
        app.frame_index++;
        const double display_time =
            vrapi_GetPredictedDisplayTime(app.ovr, app.frame_index);
        ovrTracking2 tracking =
            vrapi_GetPredictedTracking2(app.ovr, display_time);
        const ovrLayerProjection2 layer =
            renderer_render_frame(&app.renderer, &tracking);
        const ovrLayerHeader2* layers[] = { &layer.Header };
        ovrSubmitFrameDescription2 frame;
        frame.Flags = 0;
        frame.SwapInterval = 1;
        frame.FrameIndex = app.frame_index;
        frame.DisplayTime = display_time;
        frame.LayerCount = 1;
        frame.Layers = layers;
        vrapi_SubmitFrame2(app.ovr, &frame);
    }

    app_destroy(&app);

    info("shut down vr api");
    vrapi_Shutdown();

    info("detach current thread");
    (*java.Vm).DetachCurrentThread();
}
