//
//  tile.h
//  SDL2-904
//
//  Created by Huang Wei on 14-4-26.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef SDL2_904_tile_h
#define SDL2_904_tile_h

#include "glm/mat2x2.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace hardrock
{
    struct Tile
    {
        glm::mat2 transform;
        glm::vec2 translate;
        glm::u8vec4 tex;
        glm::u8vec4 color;
    };
    
    struct ITileRender
    {
        virtual ~ITileRender() { }
        virtual bool Busy() const = 0;
        virtual int RenderTiles(size_t count, const Tile* p_tiles) = 0;
    };

}

#endif