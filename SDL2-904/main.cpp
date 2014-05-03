#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
//#include <OpenGL/glu.h>
#include <iostream>
#include "glm/gtc/constants.hpp"
#include "renderer.h"
#include "scene.h"
#include "algorithm.h"


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

namespace hardrock
{
    class SpriteModel
    {
        const glm::vec2 size;
        const glm::u8vec4 tex;
        const glm::vec2 anchor;
    public:
        SpriteModel(const glm::vec2& size, const glm::u8vec4 & tex, const glm::vec2& anchor)
        : size(size)
        , tex(tex)
        , anchor(anchor)
        {
        }
        void SetTile(const glm::vec2& pos, const glm::vec2& scale, const glm::vec2& norm_dir, Tile* p_out_tile)
        {
            const glm::vec2 size = this->size * scale;
            const glm::mat2 transform(norm_dir.x * size.x, norm_dir.y * size.x, -norm_dir.y * size.y, norm_dir.x * size.y);
            p_out_tile->transform = transform;
            p_out_tile->translate = transform * (-this->anchor) + pos;
            p_out_tile->tex = this->tex;
            p_out_tile->color = { 240, 240, 240, 255 };
        }
    };
}

int main(int argc, char* args[])
{
    hardrock::TexturePackInput pack_input[] =
    {
        {4, 1},
        {4, 1},
        {4, 1},
        {1, 1},
    };
    hardrock::TexturePackOutput pack_output[4];
    int r = hardrock::TexturePack(4, 4, 4, pack_input, pack_output);
    std::cout << r << std::endl;
    for (const auto& output : pack_output)
    {
        std::cout << static_cast<int>(output.x) << ' ' << static_cast<int>(output.y) << std::endl;
    }
    return 0;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
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
        hardrock::Renderer renderer(SCREEN_WIDTH, SCREEN_HEIGHT);
        assert(renderer.Valid());
        hardrock::SpriteModel sprite_modle({128, 128}, {0, 0, 128, 128}, {0.5, 0.5});
        
        hardrock::Scene scene;
        auto layer_idx = scene.LayerAdd();
        scene.LayerAt(layer_idx) = {{0, 0, 0}};
        const float pi = glm::pi<float>();
        const float double_pi = pi * 2.0f;
        const float half_pi = glm::half_pi<float>();
        struct SpriteData
        {
            glm::vec2 pos;
            float radius;
            hardrock::Scene::IndexType tile_idx;
            hardrock::Scene::IndexType padding;
        } sprite_data[64];
        for (int i = 0; i < 64; ++i)
        {
            auto tile_idx = scene.TileAdd(layer_idx);
            int x = i % 8;
            int y = i / 8;
            sprite_data[i] = { { 32 + x * 64, 32 + y * 64 }, (i % 4) * half_pi, tile_idx, 0 };
        }
        
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
            
            for (int i = 0; i < 64; ++i)
            {
                SpriteData& s = sprite_data[i];
                s.radius += 0.01f;
                if (s.radius > pi)
                    s.radius -= double_pi;
                glm::vec2 norm;
                hardrock::FastSinCos(s.radius, &norm.y, &norm.x);
                sprite_modle.SetTile(s.pos, {1, 1}, norm, &scene.TileAt(s.tile_idx));
            }
            

            glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            scene.Render(&renderer);
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