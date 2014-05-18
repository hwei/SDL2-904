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
#include "structure.h"


namespace hardrock
{

    class RenderDevice
    {
    public:
        const static std::size_t MAX_TILE_COUNT = 1024;
        const static std::size_t MAX_BATCH_COUNT = 256;
        typedef std::uint8_t BatchIdType;
        typedef std::uint8_t AtlasIdType;
        struct RenderQuest
        {
            ITileSequence* p_tile_seq;
            glm::vec2 translate;
            glm::mat2 transform;
            BatchIdType batch_id;
            BatchIdType padding[3];
        };
    private:
        const int screen_width;
        const int screen_height;
        const float xm, ym, xa, ya;
        
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
        GLuint shader_sampler;
        GLuint shader_translate;
        GLuint shader_transform;
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
        std::vector<TileVertex> vertex_buffer;
        SimpleMemoryAllocator buffer_allocator;
        struct TileBatch
        {
            std::size_t offset;
            std::size_t capacity;
            std::size_t count;
            AtlasIdType atlas_id;
            AtlasIdType padding[3];
        };
        std::vector<TileBatch> tile_batch_list;
        
        class TextureAtlas
        {
            std::vector<glm::u8vec4> rect_list;
            GlHandles<OpGlTextures> h_textures;
            GLuint tex;
            TextureAtlas();
        public:
            static std::unique_ptr<TextureAtlas> Create(const IResourceDataSet& data_set, std::uint16_t unit_length, std::uint8_t width, std::uint8_t height, int& out_error_code);
            GLuint GetGlTexureId() const { return this->tex; }
            glm::u8vec4 GetRect(std::size_t tex_id) const { return this->rect_list[tex_id]; }
        };
        std::vector<std::unique_ptr<TextureAtlas>> up_texture_atlas_list;

        RenderDevice(int screen_width, int screen_height);
        int beginRender();
        int updateBatch(BatchIdType batch_id, ITileSequence* p_tile_seq);
        int render(BatchIdType batch_id, const glm::vec2& translate, const glm::mat2& transform);
    public:
        static std::unique_ptr<RenderDevice> Create(int screen_width, int screen_height, const std::uint8_t* p_vert_shader_data, std::size_t vert_shader_data_size, const std::uint8_t* p_frag_shader_data, std::size_t frag_shader_data_size);
        
        int CreateTextureAtlas(const IResourceDataSet& data_set, std::uint16_t unit_length, std::uint8_t width, std::uint8_t height, AtlasIdType& out_atlas_id);
        int RemoveTextureAtlas(AtlasIdType atlas_id);
        int CreateBatch(std::size_t capacity, AtlasIdType atlas_id, BatchIdType& out_batch_id);
        int RemoveBatch(BatchIdType batch_id);
        
        template<typename Iterator>
        int Render(Iterator begin, Iterator end)
        {
            int r;
            r = this->beginRender();
            if (r) return r;
            for (Iterator i = begin; i < end; ++i)
            {
                auto p_tile_seq = i->p_tile_seq;
                if (p_tile_seq)
                {
                    r = this->updateBatch(i->batch_id, p_tile_seq);
                    if (r) return r;
                }
            }
            for (Iterator i = begin; i < end; ++i)
            {
                r = this->render(i->batch_id, i->translate, i->transform);
                if (r) return r;
            }
            return 0;
        }
    };
}


#endif /* defined(__SDL2_904__renderer__) */
