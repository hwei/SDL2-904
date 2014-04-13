#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <OpenGL/glu.h>
#include <iostream>
#include <string>
#include <CoreFoundation/CFBundle.h>


template <typename T, unsigned int S>
class DelayQueue {
    T arr[S];
    unsigned int current;
public:
    DelayQueue(const T& init_v = T()) : current (0) {
        for (auto p = arr; p < arr + S; ++p) *p = init_v;
    }
    T Head() const {
        return arr[current];
    }
    T PushPop(const T& v) {
        auto r = arr[current];
        arr[current] = v;
        current = (current + 1) % S;
        return r;
    }
};


bool loop() {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            return false;
        }
    }

    static float x = 0;
    x += 0.01;
    if (x >= 1) {
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

int main(int argc, char* args[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    auto main_bundle = CFBundleGetMainBundle();
    auto data_url = CFBundleCopyResourceURL(main_bundle, CFSTR("test"), CFSTR("txt"), nullptr);
    auto data_path = CFURLCopyFileSystemPath(data_url, kCFURLPOSIXPathStyle);
    const char* path = CFStringGetCStringPtr(data_path, CFStringGetSystemEncoding());
    CFRelease(data_path);
    CFRelease(data_url);
    CFRelease(main_bundle);
    
    FILE* file = fopen(path, "rb");
    if (!file) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to open %s\n", path);
        return 1;
    }
    fclose(file);
    
    auto p_window = SDL_CreateWindow(
        "SDL test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        480, 640, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (p_window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return -2;
    }
    
    auto p_context = SDL_GL_CreateContext(p_window);
    if (p_context == nullptr) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return -3;
    }
    
    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glClearColor( 0.f, 0.f, 0.f, 1.f );
    auto error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << error << std::endl;
        return -4;
    }
    
    typedef decltype(SDL_GetTicks()) tick_t;
    tick_t last_fps_tick = SDL_GetTicks();
    tick_t last_fps_frame = 0;
    DelayQueue<unsigned int, 6> tick_time_queue;
    while (loop()) {
        SDL_GL_SwapWindow(p_window);
        tick_t tick_5 = tick_time_queue.Head();
        tick_t len_5_x = SDL_GetTicks() - tick_5;
        if (len_5_x < 90) {
            SDL_Delay(100 - len_5_x);
        }
        tick_t now = SDL_GetTicks();
        tick_time_queue.PushPop(now);
        ++last_fps_frame;
        if (last_fps_frame >= 60) {
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