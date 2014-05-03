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
#include "resource.h"


namespace hardrock
{
    class Renderer : public ITileRender
    {
        const int screen_width;
        const int screen_height;
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
        size_t tile_count;
        const float xm, ym, xa, ya;
    public:
        const static size_t MAX_TILE_COUNT = 1024;
        Renderer(int screen_width, int screen_height, IResourceManager& resource_manager);
        bool Valid() const { return this->b_valid; }
        int Begin(size_t count) override;
        int AddTile(const Tile& tile) override;
        int End() override;
    };
}


#endif /* defined(__SDL2_904__renderer__) */
