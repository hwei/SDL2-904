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

    class Renderer
    {
        const int screen_width;
        const int screen_height;
        bool b_valid;
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        GLuint shader_sampler;
        GlHandles<OpGlVertexArrays> h_vertex_arrays;
        GlHandles<OpGlBuffers> h_buffers;
        GlHandle<OpGlProgram> h_program;

        struct TileVertex
        {
            glm::vec2 pos;
            glm::u8vec2 tex;
            glm::u8vec2 padding;
            glm::u8vec4 color;
        };
        
        class TextureAtlasRender : public ITileRender
        {
            std::vector<glm::u8vec4> rect_list;
            GlHandles<OpGlTextures> h_textures;
            GLuint tex;
            std::vector<TileVertex> vertex_buffer;
            Renderer* p_render;
            TextureAtlasRender(Renderer* p_render);
        public:
            static std::unique_ptr<TextureAtlasRender> Create(Renderer* p_render, const IResourceDataSet& data_set, std::uint16_t unit_length, std::uint8_t width, std::uint8_t height, int& out_error_code);
            int Render(std::unique_ptr<ITileSequence>&& tile_sequence) override;
        };
        
        int render(const TileVertex* p_vertex_buffer, std::size_t tile_count, GLuint tex_id);
    public:
        const static size_t MAX_TILE_COUNT = 1024;
        Renderer(int screen_width, int screen_height, const std::uint8_t* p_vert_shader_data, std::size_t vert_shader_data_size, const std::uint8_t* p_frag_shader_data, std::size_t frag_shader_data_size);
        bool Valid() const { return this->b_valid; }
        std::unique_ptr<ITileRender> GetTextureAtlasRender(const IResourceDataSet& data_set, std::uint16_t unit_length, std::uint8_t width, std::uint8_t height, int& out_error_code);
    };
}


#endif /* defined(__SDL2_904__renderer__) */
