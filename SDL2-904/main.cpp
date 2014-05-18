#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
//#include <OpenGL/glu.h>
#include <iostream>
#include <array>
#include <algorithm>
#include <cassert>
#include "glm/gtc/constants.hpp"
#include "renderer.h"
#include "scene.h"
#include "algorithm.h"
#include "resource.h"


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
        const glm::vec2 anchor;
        const std::uint16_t tex_id;
        const std::uint16_t padding;
    public:
        SpriteModel(const glm::vec2& size, const glm::vec2& anchor, std::uint16_t tex_id)
        : size(size)
        , anchor(anchor)
        , tex_id(tex_id)
        , padding(0)
        {
        }
        void SetTileWithPosScaleDir(const glm::vec2& pos, const glm::vec2& scale, const glm::vec2& norm_dir, Tile* p_out_tile)
        {
            const glm::vec2 size = this->size * scale;
            const glm::mat2 transform(norm_dir.x * size.x, norm_dir.y * size.x, -norm_dir.y * size.y, norm_dir.x * size.y);
            p_out_tile->transform = transform;
            p_out_tile->translate = pos - transform * this->anchor;
            p_out_tile->tex_id = this->tex_id;
            p_out_tile->color = { 240, 240, 240, 255 };
        }
        void SetTileWithPos(const glm::vec2& pos, Tile* p_out_tile)
        {
            p_out_tile->transform = glm::mat2();
            p_out_tile->translate = pos - this->anchor;
            p_out_tile->tex_id = this->tex_id;
            p_out_tile->color = { 240, 240, 240, 255 };
        }
    };
    
    struct IControlTarget
    {
        virtual ~IControlTarget() { }
        virtual void Move(float x, float y) = 0;
    };
    
    class KeyboardControl
    {
        int left, right, up, down;
    public:
        KeyboardControl() : left(0), right(0), up(0), down(0) { }
        void KeyStatus(int scan_code, bool pressed)
        {
            int v = pressed ? 1 : 0;
            switch (scan_code)
            {
                case SDL_SCANCODE_UP:
                    this->up = v;
                    break;
                case SDL_SCANCODE_DOWN:
                    this->down = v;
                    break;
                case SDL_SCANCODE_LEFT:
                    this->left = v;
                    break;
                case SDL_SCANCODE_RIGHT:
                    this->right = v;
                    break;
                default:
                    return;
            }
        }
        void Control(IControlTarget* p_target)
        {
            int x = this->right - this->left;
            int y = this->down - this->up;
            if (x && y)
                p_target->Move(x * glm::one_over_root_two<float>(), y * glm::one_over_root_two<float>());
            else
                p_target->Move(static_cast<float>(x), static_cast<float>(y));
        }
    };
}

int main(int argc, char* args[])
{
    hardrock::PackResourceManager resource_manager("res.pack");

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
        auto up_vert_shader_data = std::move(resource_manager.LoadResource(hardrock::FnvHash("test.vert")));
        assert(up_vert_shader_data);
        auto up_frag_shader_data = std::move(resource_manager.LoadResource(hardrock::FnvHash("test.frag")));
        assert(up_frag_shader_data);
        auto up_render_device = hardrock::RenderDevice::Create(SCREEN_WIDTH, SCREEN_HEIGHT, &up_vert_shader_data->at(0), up_vert_shader_data->size(), &up_frag_shader_data->at(0), up_frag_shader_data->size());
        assert(up_render_device);
        std::array<std::uint32_t, 4> tex_res_id_list =
        {
            hardrock::FnvHash("self_l.webp"),
            hardrock::FnvHash("self_m.webp"),
            hardrock::FnvHash("self_r.webp"),
            hardrock::FnvHash("bullet_0.webp"),
        };
        std::sort(tex_res_id_list.begin(), tex_res_id_list.end());
        auto up_tex_res_bundle = resource_manager.LoadResourceBatch(&tex_res_id_list[0], tex_res_id_list.size());
        assert(up_tex_res_bundle);
        int r;
        hardrock::RenderDevice::AtlasIdType atlas_id;
        r = up_render_device->CreateTextureAtlas(*up_tex_res_bundle, 16, 16, 16, atlas_id);
        assert(r == 0);
        hardrock::RenderDevice::BatchIdType batch_id;
        r = up_render_device->CreateBatch(64, atlas_id, batch_id);
        std::array<hardrock::RenderDevice::RenderQuest, 1> render_quest_list =
        {
            { nullptr, {}, {}, batch_id },
        };
        
        assert(r == 0);
        auto tex_self_m_find_iter = std::lower_bound(tex_res_id_list.begin(), tex_res_id_list.end(), hardrock::FnvHash("self_m.webp"));
        assert(*tex_self_m_find_iter == hardrock::FnvHash("self_m.webp"));
        hardrock::SpriteModel sprite_model({128, 128}, {0.5, 0.5}, static_cast<hardrock::TileSet::IndexType>(tex_self_m_find_iter - tex_res_id_list.begin()));
        
        hardrock::TileSet tile_set(64);
        
        const float pi = glm::pi<float>();
        const float double_pi = pi * 2.0f;
        const float half_pi = glm::half_pi<float>();
        const size_t sprite_count = 16;
        struct SpriteData
        {
            glm::vec2 pos;
            float radius;
            hardrock::TileSet::IndexType tile_idx;
            hardrock::TileSet::IndexType padding;
        } sprite_data[sprite_count];
        for (int i = 0; i < sprite_count; ++i)
        {
            auto tile_idx = tile_set.TileAdd();
            int x = i % 4;
            int y = i / 4;
            sprite_data[i] = { { 64 + x * 128, 64 + y * 128 }, (i % 4) * half_pi, tile_idx, 0 };
        }
        class ControlTranslate : public hardrock::IControlTarget
        {
            glm::vec2* p_translate;
            float x, y;
        public:
            ControlTranslate(glm::vec2* p_translate)
            : p_translate(p_translate)
            , x(0), y(0)
            {
            }
            void Move(float x, float y) override
            {
                this->x += x;
                this->y += y;
                p_translate->x = this->x;
                p_translate->y = this->y;
            }
        };
        ControlTranslate control_translate(&render_quest_list[0].translate);
        hardrock::KeyboardControl keyboard_control;

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
                switch(e.type)
                {
                    case SDL_QUIT:
                        b_quit = true;
                        break;
                    case SDL_KEYDOWN:
                        if (!e.key.repeat)
                        {
                            keyboard_control.KeyStatus(e.key.keysym.scancode, e.key.state);
                        }
                        break;
                    case SDL_KEYUP:
                        keyboard_control.KeyStatus(e.key.keysym.scancode, e.key.state);
                        break;
                }
            }
            if (b_quit)
            {
                break;
            }

            keyboard_control.Control(&control_translate);

            for (int i = 0; i < sprite_count; ++i)
            {
                SpriteData& s = sprite_data[i];
                s.radius += 0.01f;
                if (s.radius > pi)
                    s.radius -= double_pi;
                glm::vec2 norm;
                hardrock::FastSinCos(s.radius, &norm.y, &norm.x);
                sprite_model.SetTileWithPosScaleDir(s.pos, {1, 1}, norm, &tile_set.TileAt(s.tile_idx));
            }

            glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            {
                auto tile_seq = tile_set.GetTileSequence();
                render_quest_list[0].p_tile_seq = &tile_seq;
                up_render_device->Render(render_quest_list.begin(), render_quest_list.end());
            }
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