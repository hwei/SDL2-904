//
//  scene.cpp
//  SDL2-904
//
//  Created by Huang Wei on 14-4-25.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#include "scene.h"
#include "glm/gtc/matrix_transform.hpp"

namespace hardrock
{
    Scene::Scene()
        : tile_buffer(MAX_TILE_COUNT)
    {
    }

    int Scene::Process(int scene_modify_list, ITileRender* p_tile_renderer)
    {
        if (p_tile_renderer->Busy())
            return 1;
        int tile_count = 128;
        Tile* p_tile_buffer = &this->tile_buffer[0];
        for (int i = 0; i < tile_count; ++i)
        {
            Tile* p = p_tile_buffer + i;
            p->translate = glm::vec2(i * 2, i * 2);
            p->transform = glm::mat2(128, 0, 0, 128);
            p->tex = glm::u8vec4(0, 0, 128, 128);
            p->color = glm::u8vec4(240, 240, 240, 255);
        }
        return p_tile_renderer->RenderTiles(tile_count, p_tile_buffer);
    }
}
