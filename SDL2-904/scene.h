//
//  scene.h
//  SDL2-904
//
//  Created by Huang Wei on 14-4-25.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef SDL2_904_scene_h
#define SDL2_904_scene_h

#include <vector>
#include "tile.h"

namespace hardrock
{
    class Scene
    {
        static const size_t MAX_TILE_COUNT = 1024;
        std::vector<Tile> tile_buffer;
    public:
        Scene();
        int Process(int scene_modify_list, ITileRender* p_tile_renderer);
    };
}



#endif
