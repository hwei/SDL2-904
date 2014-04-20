#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
//#include <OpenGL/glu.h>
#include "webp/decode.h"
#include <CoreFoundation/CFBundle.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cassert>
#include <cstddef>
#include <limits>
#include <random>

static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 480;


template <typename T, unsigned int S>
class DelayQueue
{
    T arr[S];
    unsigned int current;
public:
    DelayQueue(const T& init_v = T())
        : current (0)
    {
        for (auto p = arr; p < arr + S; ++p) *p = init_v;
    }
    T Head() const
    {
        return arr[current];
    }
    T PushPop(const T& v)
    {
        auto r = arr[current];
        arr[current] = v;
        current = (current + 1) % S;
        return r;
    }
};

struct CFDel
{
    void operator() (CFTypeRef ref) { CFRelease(ref); }
};

template <typename T>
std::unique_ptr<T, CFDel> create_cfptr(T* ref)
{
    return std::unique_ptr<T, CFDel>(ref);
}

int load_resource(const char* path, void* out_buffer, size_t out_size)
{
    auto system_encoding = CFStringGetSystemEncoding();
    auto cp_path = create_cfptr(CFStringCreateWithCString(kCFAllocatorDefault, path, system_encoding));
    auto main_bundle = CFBundleGetMainBundle();
    auto cp_data_url = create_cfptr(
        CFBundleCopyResourceURL(main_bundle, cp_path.get(), nullptr, nullptr));
    auto cp_data_path = create_cfptr(CFURLCopyFileSystemPath(cp_data_url.get(), kCFURLPOSIXPathStyle));
    const char* data_path = CFStringGetCStringPtr(cp_data_path.get(), system_encoding);
    
    std::ifstream file(data_path, std::ios::binary);
    if (!file)
        return -1;
    file.seekg(0, file.end);
    size_t file_size = file.tellg();
    do
    {
        if (out_buffer == nullptr || out_size < file_size)
            break;
        file.seekg(0, file.beg);
        file.read(static_cast<char*>(out_buffer), file_size);
    } while (false);
    return static_cast<int>(file_size);
}

template<typename OP>
class GlHandles
{
    std::vector<GLuint> handles;
public:
    GlHandles(int n)
    : handles(n)
    {
        OP op;
        op.gen(n, &this->handles[0]);
    }
    ~GlHandles()
    {
        OP op;
        op.del((GLsizei)this->handles.size(), &this->handles[0]);
    }
    GLuint get(int i) const { return this->handles[i]; }
};

template<typename OP>
class GlHandle
{
    GLuint handle;
public:
    template<typename ...Ts>
    GlHandle(Ts... args)
    {
        OP op;
        this->handle = op.gen(args...);
    }
    ~GlHandle()
    {
        OP op;
        op.del(this->handle);
    }
    GLuint get() const { return this->handle; }
    operator GLuint() const { return this->handle; }
};

struct OpGlBuffers
{
    void gen(GLsizei n, GLuint *p) { glGenBuffers(n, p); }
    void del(GLsizei n, GLuint *p) { glDeleteBuffers(n, p); }
};

struct OpGlVertexArrays
{
    void gen(GLsizei n, GLuint *p) { glGenVertexArrays(n, p); }
    void del(GLsizei n, GLuint *p) { glDeleteVertexArrays(n, p); }
};

struct OpGlTextures
{
    void gen(GLsizei n, GLuint *p) { glGenTextures(n, p); }
    void del(GLsizei n, GLuint *p) { glDeleteTextures(n, p); }
};

struct OpGlShader
{
    GLuint gen(GLenum type) { return glCreateShader(type); }
    void del(GLuint h) { glDeleteShader(h); }
};

struct OpGlProgram
{
    GLuint gen() { return glCreateProgram(); }
    void del(GLuint h) { glDeleteShader(h); }
};

struct SurfaceVertex
{
    struct
    {
        GLshort x;
        GLshort y;
    }pos;
    struct
    {
        GLubyte u;
        GLubyte v;
    }tex;
    GLubyte alpha;
    GLbyte light;
};

class TestShader
{
    bool b_valid;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint tex;
    GLuint shader_color;
    GLuint shader_sampler;
    GLuint shader_screen_trans;
    GlHandles<OpGlVertexArrays> h_vertex_arrays;
    GlHandles<OpGlBuffers> h_buffers;
    GlHandles<OpGlTextures> h_textures;
    GlHandle<OpGlProgram> h_program;
public:
    const static size_t MAX_SURFACE_COUNT = 1024;
    TestShader();
    bool Valid() const { return this->b_valid; }
    void Render(const SurfaceVertex *p_vertex_data, size_t count) const;
};

TestShader::TestShader()
    : b_valid(false)
    , h_vertex_arrays(1)
    , h_buffers(2)
    , h_textures(1)
{
    static GLushort elements[] = {
        0, 1, 2,
        2, 3, 0,
    };
    
    int r;
    GLenum error;
    
    do
    {
        r = load_resource("test_tex.webp", nullptr, 0);
        assert(r > 0);
        int test_img_size = r;
        std::vector<uint8_t> test_img_data(test_img_size);
        r = load_resource("test_tex.webp", &test_img_data[0], test_img_size);
        assert(r == test_img_size);
        int tex_width, tex_height;
        r = WebPGetInfo(&test_img_data[0], test_img_size, &tex_width, &tex_height);
        assert(r);
        int tex_stride = tex_width * 4 * sizeof(uint8_t);
        int tex_size = tex_stride * tex_height;
        std::vector<uint8_t> tex_data(tex_size);
        uint8_t* decode_result = WebPDecodeRGBAInto(
            &test_img_data[0], test_img_size, &tex_data[0], tex_size, tex_stride);
        assert(decode_result != nullptr);
        
        this->tex = this->h_textures.get(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, decode_result);

        GlHandle<OpGlShader> h_vertex_shader(GL_VERTEX_SHADER);
        r = load_resource("test.vert", nullptr, 0);
        assert(r > 0);
        int vertex_shader_size = r;
        std::vector<char> vertex_shader_data(vertex_shader_size);
        r = load_resource("test.vert", &vertex_shader_data[0], vertex_shader_size);
        assert(r == vertex_shader_size);
        const GLchar* vertex_shader_list[1];
        vertex_shader_list[0] = &vertex_shader_data[0];
        glShaderSource(h_vertex_shader, 1, vertex_shader_list, &vertex_shader_size);
        glCompileShader(h_vertex_shader);
        glGetShaderiv(h_vertex_shader, GL_COMPILE_STATUS, &r);
        if (r != GL_TRUE)
        {
            char buffer[512];
            glGetShaderInfoLog(h_vertex_shader, 512, NULL, buffer);
            std::cerr << buffer << std::endl;
            break;
        }

        GlHandle<OpGlShader> h_fragment_shader(GL_FRAGMENT_SHADER);
        r = load_resource("test.frag", nullptr, 0);
        assert(r > 0);
        int fragment_shader_size = r;
        std::vector<char> fragment_shader_data(fragment_shader_size);
        r = load_resource("test.frag", &fragment_shader_data[0], fragment_shader_size);
        assert(r == fragment_shader_size);
        const GLchar* fragment_shader_list[1];
        fragment_shader_list[0] = &fragment_shader_data[0];
        glShaderSource(h_fragment_shader, 1, fragment_shader_list, &fragment_shader_size);
        glCompileShader(h_fragment_shader);
        glGetShaderiv(h_fragment_shader, GL_COMPILE_STATUS, &r);
        if (r != GL_TRUE)
        {
            char buffer[512];
            glGetShaderInfoLog(h_fragment_shader, 512, NULL, buffer);
            std::cerr << buffer << std::endl;
            break;
        }
        
        glAttachShader(this->h_program, h_vertex_shader);
        glAttachShader(this->h_program, h_fragment_shader);
        
        glLinkProgram(this->h_program);
        
        this->vbo = this->h_buffers.get(0);
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(SurfaceVertex) * 4 * MAX_SURFACE_COUNT, nullptr, GL_STREAM_DRAW);
        this->ebo = this->h_buffers.get(1);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
        std::vector<GLushort> index_data(MAX_SURFACE_COUNT * 6);
        GLushort* p_index_data = &index_data[0];
        for (int i = 0; i < MAX_SURFACE_COUNT; ++i) {
            GLushort *p = p_index_data + i * 6;
            GLushort base = i * 4;
            p[0] = elements[0] + base;
            p[1] = elements[1] + base;
            p[2] = elements[2] + base;
            p[3] = elements[3] + base;
            p[4] = elements[4] + base;
            p[5] = elements[5] + base;
        }
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * MAX_SURFACE_COUNT * 6, p_index_data, GL_STATIC_DRAW);
        
        this->vao = this->h_vertex_arrays.get(0);
        glBindVertexArray(this->vao);
        GLint pos_attrib = glGetAttribLocation(this->h_program, "position");
        glEnableVertexAttribArray(pos_attrib);
        glVertexAttribPointer(pos_attrib, 2, GL_SHORT, GL_FALSE, sizeof(SurfaceVertex), (void*)offsetof(SurfaceVertex, pos));
        GLint tex_attrib = glGetAttribLocation(this->h_program, "texcoord");
        glEnableVertexAttribArray(tex_attrib);
        glVertexAttribPointer(tex_attrib, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(SurfaceVertex), (void*)offsetof(SurfaceVertex, tex));
        
        this->shader_screen_trans = glGetUniformLocation(this->h_program, "ScreenTransfrom");
        this->shader_color = glGetUniformLocation(this->h_program, "Color");
        this->shader_sampler = glGetUniformLocation(this->h_program, "TexSampler");

        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error: " << error << std::endl;
        }
        else
        {
            this->b_valid = true;
        }
    } while (false);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << error << std::endl;
    }

}

void TestShader::Render(const SurfaceVertex *p_vertex_data, size_t count) const
{
    GLenum error;
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glUseProgram(this->h_program);
    glUniform4f(
        this->shader_screen_trans, 2.0f / SCREEN_WIDTH,
        -2.0f / SCREEN_HEIGHT,
        -1.0f - 0.5f / SCREEN_WIDTH,
        1.0f + 0.5f / SCREEN_HEIGHT);
    glUniform3f(this->shader_color, 1.0f, 1.0f, 1.0f);
    glUniform1i(this->shader_sampler, 0);
    glBindVertexArray(this->vao);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SurfaceVertex) * 4 * MAX_SURFACE_COUNT, nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SurfaceVertex) * count, p_vertex_data);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6 * count), GL_UNSIGNED_SHORT, nullptr);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << error << std::endl;
    }
}

int main(int argc, char* args[])
{
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    std::vector<char> test_chars(512);
    int r = load_resource("test.txt", &test_chars[0], test_chars.size());
    if (r > 0)
    {
        for (int i = 0; i < r; ++i)
        {
            std::cout << test_chars[i];
        }
        std::cout << std::endl;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    
    auto p_window = SDL_CreateWindow(
        "SDL test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (p_window == nullptr)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return -2;
    }
    
    auto p_context = SDL_GL_CreateContext(p_window);
    if (p_context == nullptr)
    {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return -3;
    }
    
    {
        TestShader test_shader;
        assert(test_shader.Valid());
        
        struct Obj
        {
            struct
            {
                float x;
                float y;
            }pos;
            struct
            {
                float x;
                float y;
            }speed;
        };
        
        const int obj_size = 128;
        const int obj_half_size = obj_size / 2;
        const size_t obj_count = TestShader::MAX_SURFACE_COUNT;
        Obj obj_array[obj_count];
        for (Obj& obj : obj_array) {
            obj.pos.x = static_cast<float>(std::rand() % SCREEN_WIDTH) - obj_half_size;
            obj.pos.y = static_cast<float>(std::rand() % SCREEN_HEIGHT) - obj_half_size;
            obj.speed.x = (float)std::rand() / (float)RAND_MAX;
            obj.speed.y = (float)std::rand() / (float)RAND_MAX;
        }
        
        SurfaceVertex vertex_data[obj_count * 4];
        
        typedef decltype(SDL_GetTicks()) tick_t;
        tick_t last_fps_tick = SDL_GetTicks();
        tick_t last_fps_frame = 0;
        DelayQueue<unsigned int, 6> tick_time_queue;
        while (true)
        {
            SDL_Event e;
            bool b_quit = false;
            while (SDL_PollEvent(&e) != 0)
            {
                if (e.type == SDL_QUIT)
                {
                    b_quit = true;
                    break;
                }
            }
            if (b_quit)
            {
                break;
            }
            
            for (Obj& obj : obj_array) {
                obj.pos.x += obj.speed.x;
                obj.pos.y += obj.speed.y;
                if (obj.pos.x > SCREEN_WIDTH - obj_half_size) obj.pos.x = -obj_half_size;
                if (obj.pos.y > SCREEN_HEIGHT - obj_half_size) obj.pos.y = -obj_half_size;
            }
            
            for (int i = 0; i < obj_count; ++i) {
                SurfaceVertex* p_vertex = vertex_data + i * 4;
                Obj* p_obj = obj_array + i;
                p_vertex[0].pos.x = static_cast<GLushort>(p_obj->pos.x);
                p_vertex[0].pos.y = static_cast<GLushort>(p_obj->pos.y);
                p_vertex[0].tex.u = 0;
                p_vertex[0].tex.v = 0;
                p_vertex[0].alpha = 255;
                p_vertex[0].light = 0;
                p_vertex[1].pos.x = static_cast<GLushort>(p_obj->pos.x + obj_size);
                p_vertex[1].pos.y = static_cast<GLushort>(p_obj->pos.y);
                p_vertex[1].tex.u = 128;
                p_vertex[1].tex.v = 0;
                p_vertex[1].alpha = 255;
                p_vertex[1].light = 0;
                p_vertex[2].pos.x = static_cast<GLushort>(p_obj->pos.x + obj_size);
                p_vertex[2].pos.y = static_cast<GLushort>(p_obj->pos.y + obj_size);
                p_vertex[2].tex.u = 128;
                p_vertex[2].tex.v = 128;
                p_vertex[2].alpha = 255;
                p_vertex[2].light = 0;
                p_vertex[3].pos.x = static_cast<GLushort>(p_obj->pos.x);
                p_vertex[3].pos.y = static_cast<GLushort>(p_obj->pos.y + obj_size);
                p_vertex[3].tex.u = 0;
                p_vertex[3].tex.v = 128;
                p_vertex[3].alpha = 255;
                p_vertex[3].light = 0;
            }
            
            glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            test_shader.Render(vertex_data, obj_count);
            glFlush();
            
            SDL_GL_SwapWindow(p_window);
            
            tick_t tick_5 = tick_time_queue.Head();
            tick_t len_5_x = SDL_GetTicks() - tick_5;
            if (len_5_x < 90)
            {
                SDL_Delay(100 - len_5_x);
            }
            tick_t now = SDL_GetTicks();
            tick_time_queue.PushPop(now);
            ++last_fps_frame;
            if (last_fps_frame >= 60)
            {
                std::cout << "fps: " << 60000.0 / (now - last_fps_tick) << std::endl;
                last_fps_tick = now;
                last_fps_frame = 0;
            }
        }
    }
    
    SDL_GL_DeleteContext(p_context);
    SDL_DestroyWindow(p_window);
    SDL_Quit();
    
    std::cout << "Quit" << std::endl;
    
    return 0;
}