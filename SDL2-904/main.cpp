#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <OpenGL/glu.h>
#include <CoreFoundation/CFBundle.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>


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
    auto cp_main_bundle = create_cfptr(CFBundleGetMainBundle());
    auto cp_data_url = create_cfptr(
        CFBundleCopyResourceURL(cp_main_bundle.get(), cp_path.get(), nullptr, nullptr));
    auto cp_data_path = create_cfptr(CFURLCopyFileSystemPath(cp_data_url.get(), kCFURLPOSIXPathStyle));
    const char* data_path = CFStringGetCStringPtr(cp_data_path.get(), system_encoding);
    
    std::ifstream file(data_path, std::ios::binary);
    if (!file)
        return -1;
    file.seekg(0, file.end);
    size_t file_size = file.tellg();
    do
    {
        if (out_size < file_size)
            break;
        file.seekg(0, file.beg);
        file.read(static_cast<char*>(out_buffer), file_size);
    } while (false);
    return static_cast<int>(file_size);
}


bool loop()
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
    glBegin(GL_QUADS);
    glVertex2f( x-0.5f, -0.5f );
    glVertex2f( x+0.5f, -0.5f );
    glVertex2f( x+0.5f, 0.5f );
    glVertex2f( x-0.5f, 0.5f );
    glEnd();
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
    
    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glClearColor( 0.f, 0.f, 0.f, 1.f );
    auto error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << error << std::endl;
        return -4;
    }
    
    typedef decltype(SDL_GetTicks()) tick_t;
    tick_t last_fps_tick = SDL_GetTicks();
    tick_t last_fps_frame = 0;
    DelayQueue<unsigned int, 6> tick_time_queue;
    while (loop())
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