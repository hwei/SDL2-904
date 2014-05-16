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
        std::uint16_t tex_id;
        std::uint16_t palette_id;
        glm::u8vec4 color;
    };
    
    struct ITileSequence
    {
        virtual ~ITileSequence() { }
        virtual std::size_t Count() const = 0;
        virtual bool Next(Tile& out_tile) = 0;
    };

    struct ITileRender
    {
        virtual ~ITileRender() { }
        virtual int Render(std::unique_ptr<ITileSequence>&& tile_sequence) = 0;
    };
}

#endif
