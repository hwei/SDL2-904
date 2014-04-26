//
//  renderer.h
//  SDL2-904
//
//  Created by Huang Wei on 14-4-26.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef __SDL2_904__renderer__
#define __SDL2_904__renderer__

#include <vector>
#include <memory>
#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include "glm/mat2x2.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glhandle.h"
#include "tile.h"

namespace hardrock
{
    class Renderer : public ITileRender
    {
        int screen_width;
        int screen_height;
        bool b_valid;
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        GLuint tex;
        GLuint shader_sampler;
        GlHandles<OpGlVertexArrays> h_vertex_arrays;
        GlHandles<OpGlBuffers> h_buffers;
        GlHandles<OpGlTextures> h_textures;
        GlHandle<OpGlProgram> h_program;
        struct TileVertex
        {
            glm::vec2 pos;
            glm::u8vec2 tex;
            glm::u8vec2 padding;
            glm::u8vec4 color;
        };
        std::vector<TileVertex> vertex_buffer;
    public:
        const static size_t MAX_TILE_COUNT = 1024;
        Renderer(int screen_width, int screen_height);
        bool Valid() const { return this->b_valid; }
        // void Render(const SurfaceVertex *p_vertex_data, size_t surface_count) const;
        bool Busy() const override { return false; }
        int RenderTiles(size_t count, const Tile* p_tiles) override;
    };
}


#endif /* defined(__SDL2_904__renderer__) */
