#include <SDL2/SDL.h>
#define GL3_PROTOTYPES 1
#include <OpenGL/gl3.h>
//#include <SDL2/SDL_opengl.h>
//#include <OpenGL/glu.h>
#include <CoreFoundation/CFBundle.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cassert>


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


class TestShader
{
    bool b_valid;
    GLuint vao;
    GLuint vbo;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint shader_program;
    GLuint shader_color;
public:
    TestShader();
    bool Valid() const { return this->b_valid; }
    void Render() const;
};

TestShader::TestShader()
: b_valid(false)
{
    static float vertices[] = {
        0.0f,  0.5f, // Vertex 1 (X, Y)
        0.5f, -0.5f, // Vertex 2 (X, Y)
        -0.5f, -0.5f  // Vertex 3 (X, Y)
    };
    
    do
    {
        glGenVertexArrays(1, &this->vao);
        glBindVertexArray(this->vao);
        
        glGenBuffers(1, &this->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        int r;

        this->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        r = load_resource("test.vert", nullptr, 0);
        assert(r > 0);
        int vertex_shader_size = r;
        std::vector<char> vertex_shader_data(512);
        r = load_resource("test.vert", &vertex_shader_data[0], 512);
        assert(r == vertex_shader_size);
        const GLchar* vertex_shader_list[1];
        vertex_shader_list[0] = &vertex_shader_data[0];
        glShaderSource(this->vertex_shader, 1, vertex_shader_list, &vertex_shader_size);
        glCompileShader(this->vertex_shader);
        glGetShaderiv(this->vertex_shader, GL_COMPILE_STATUS, &r);
        if (r != GL_TRUE)
        {
            char buffer[512];
            glGetShaderInfoLog(this->vertex_shader, 512, NULL, buffer);
            std::cerr << buffer << std::endl;
            break;
        }
        
        this->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        r = load_resource("test.frag", nullptr, 0);
        assert(r > 0);
        int fragment_shader_size = r;
        std::vector<char> fragment_shader_data(fragment_shader_size);
        r = load_resource("test.frag", &fragment_shader_data[0], fragment_shader_size);
        assert(r == fragment_shader_size);
        const GLchar* fragment_shader_list[1];
        fragment_shader_list[0] = &fragment_shader_data[0];
        glShaderSource(this->fragment_shader, 1, fragment_shader_list, &fragment_shader_size);
        glCompileShader(this->fragment_shader);
        glGetShaderiv(this->fragment_shader, GL_COMPILE_STATUS, &r);
        assert(r == GL_TRUE);
        
        this->shader_program = glCreateProgram();
        glAttachShader(this->shader_program, this->vertex_shader);
        glAttachShader(this->shader_program, this->fragment_shader);
        
        glLinkProgram(this->shader_program);
        
        GLint pos_attrib = glGetAttribLocation(this->shader_program, "position");
        glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(pos_attrib);
        
        this->shader_color = glGetUniformLocation(this->shader_program, "triangleColor");
        
        auto error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error: " << error << std::endl;
        }
        
        this->b_valid = true;
    } while (false);
}

void TestShader::Render() const
{
    GLenum error;
    glUseProgram(this->shader_program);
    glBindVertexArray(this->vao);
    glUniform3f(this->shader_color, 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << error << std::endl;
    }
}



bool loop(TestShader* p_shader)
{
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0)
    {
        if (e.type == SDL_QUIT)
            return false;
    }

    static float x = 0;
    x += 0.01;
    if (x >= 1)
    {
        x = -1;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    p_shader->Render();
    auto error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << error << std::endl;
    }
    return true;
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
        480, 640, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
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
    
    
    TestShader test_shader;
    assert(test_shader.Valid());

    typedef decltype(SDL_GetTicks()) tick_t;
    tick_t last_fps_tick = SDL_GetTicks();
    tick_t last_fps_frame = 0;
    DelayQueue<unsigned int, 6> tick_time_queue;
    while (loop(&test_shader))
    {
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

    SDL_DestroyWindow(p_window);
    SDL_Quit();
    
    std::cout << "Quit" << std::endl;
    
    return 0;
}