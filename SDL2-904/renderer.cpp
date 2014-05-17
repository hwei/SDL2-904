//
//  renderer.cpp
//  SDL2-904
//
//  Created by Huang Wei on 14-4-26.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#include "renderer.h"
#include <iostream>
#include <cassert>
#include "webp/decode.h"
#include "glm/gtc/matrix_access.hpp"
#include "algorithm.h"


namespace hardrock
{
    RenderDevice::TextureAtlasRender::TextureAtlasRender(const std::weak_ptr<RenderDevice>& wp_render_device)
    : h_textures(1)
    , wp_render_device(wp_render_device)
    , vertex_buffer(MAX_TILE_COUNT * 4)
    {
        this->tex = this->h_textures.get(0);
    }
    
    std::unique_ptr<RenderDevice::TextureAtlasRender> RenderDevice::TextureAtlasRender::Create(const std::weak_ptr<RenderDevice>& wp_render_device, const IResourceDataSet& data_set, std::uint16_t unit_length, std::uint8_t width, std::uint8_t height, int& out_error_code)
    {
        assert((width & (width - 1)) == 0);
        assert((height & (height - 1)) == 0);
        assert(width <= 128 && height <= 128);
        int r;
        std::unique_ptr<TextureAtlasRender> up_render(new TextureAtlasRender(wp_render_device));
        auto data_count = data_set.Count();
        up_render->rect_list.resize(data_count);
        std::vector<TexturePackInput> pack_input(data_count);
        std::vector<TexturePackOutput> pack_output(data_count);
        const int int_unit_length = static_cast<int>(unit_length);
        for (std::size_t i = 0; i < data_count; ++i)
        {
            int w, h;
            const std::uint8_t* p_webp_data;
            std::size_t size;
            r = data_set.GetDataByIdx(i, p_webp_data, size);
            assert(r == 0);
            r = WebPGetInfo(p_webp_data, size, &w, &h);
            if (r == 0)
            {
                out_error_code = 1;
                return nullptr;
            }
            if (w % int_unit_length)
            {
                out_error_code = 2;
                return nullptr;
            }
            if (h % int_unit_length)
            {
                out_error_code = 2;
                return nullptr;
            }
            w /= int_unit_length;
            if (w > width)
            {
                out_error_code = 3;
                return nullptr;
            }
            h /= int_unit_length;
            if (h > height)
            {
                out_error_code = 3;
                return nullptr;
            }
            pack_input[i].width = static_cast<std::uint8_t>(w);
            pack_input[i].height = static_cast<std::uint8_t>(h);
        }
        r = TexturePack(width, height, static_cast<std::uint16_t>(data_count), &pack_input[0], &pack_output[0]);
        if (r != 0)
        {
            out_error_code = 0x100 | r;
            return nullptr;
        }
        const size_t size_t_unit_length = static_cast<size_t>(unit_length);
        const size_t tex_width = static_cast<size_t>(width) * size_t_unit_length;
        const size_t tex_height = static_cast<size_t>(height) * size_t_unit_length;
        const size_t tex_stride = tex_width * 4 * sizeof(std::uint8_t);
        const int int_tex_stride = static_cast<int>(tex_stride);
        std::unique_ptr<std::vector<std::uint8_t>> up_image_data(new std::vector<std::uint8_t>(tex_height * tex_stride));
        const std::uint8_t coord_w_scale = 128 / width;
        const std::uint8_t coord_h_scale = 128 / height;
        for (std::size_t i = 0; i < data_count; ++i)
        {
            const size_t x = static_cast<size_t>(pack_output[i].x) * size_t_unit_length;
            const size_t y = static_cast<size_t>(pack_output[i].y) * size_t_unit_length;
            const size_t sub_tex_height = static_cast<size_t>(pack_input[i].height) * size_t_unit_length;
            std::uint8_t * const p = &up_image_data->at(0) + y * tex_stride + x * 4 * sizeof(std::uint8_t);
            const std::uint8_t* p_webp_data;
            std::size_t size;
            r = data_set.GetDataByIdx(i, p_webp_data, size);
            assert(r == 0);
            const uint8_t* decode_result = WebPDecodeRGBAInto(p_webp_data, size, p, sub_tex_height * tex_stride, int_tex_stride);
            if (decode_result == nullptr)
            {
                out_error_code = 4;
                return nullptr;
            }
            up_render->rect_list[i].x = pack_output[i].x * coord_w_scale;
            up_render->rect_list[i].y = pack_output[i].y * coord_h_scale;
            up_render->rect_list[i].z = (pack_output[i].x + pack_input[i].width) * coord_w_scale;
            up_render->rect_list[i].w = (pack_output[i].y + pack_input[i].height) * coord_h_scale;
        }
        
        glBindTexture(GL_TEXTURE_2D, up_render->tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(tex_width), static_cast<GLsizei>(tex_height), 0, GL_RGBA, GL_UNSIGNED_BYTE, &up_image_data->at(0));

        return up_render;
    }
    
    int RenderDevice::TextureAtlasRender::Render(std::unique_ptr<ITileSequence>&& tile_sequence)
    {
        if (tile_sequence->Count() > MAX_TILE_COUNT)
            return 1;
        
        auto sp_render_device = this->wp_render_device.lock();
        assert(sp_render_device);
        
        Tile tile;
        std::size_t tile_count = 0;
        TileVertex* p = &this->vertex_buffer[0];
        const float screen_width = sp_render_device->screen_width;
        const float screen_height = sp_render_device->screen_height;
        const float xm = 2.0f / screen_width;
        const float ym = -2.0f / screen_height;
        const float xa = -1.0f - 0.5f / screen_width;
        const float ya = 1.0f + 0.5f / screen_height;
        while (tile_sequence->Next(tile))
        {
            TileVertex* pv0 = p + (tile_count << 2);
            TileVertex* pv1 = pv0 + 1;
            TileVertex* pv2 = pv0 + 2;
            TileVertex* pv3 = pv0 + 3;
            glm::vec2 translate(tile.translate.x * xm + xa, tile.translate.y * ym + ya);
            glm::u8vec4 tex = this->rect_list[tile.tex_id];
            glm::u8vec4 color = tile.color;
            glm::vec2 vec_x(tile.transform[0].x * xm, tile.transform[0].y * ym);
            glm::vec2 vec_y(tile.transform[1].x * xm, tile.transform[1].y * ym);
            pv0->pos = translate;
            pv1->pos = translate + vec_x;
            pv2->pos = translate + vec_x + vec_y;
            pv3->pos = translate + vec_y;
            pv0->tex.x = tex.x;
            pv1->tex.x = tex.z;
            pv2->tex.x = tex.z;
            pv3->tex.x = tex.x;
            pv0->tex.y = tex.y;
            pv1->tex.y = tex.y;
            pv2->tex.y = tex.w;
            pv3->tex.y = tex.w;
            pv0->color = color;
            pv1->color = color;
            pv2->color = color;
            pv3->color = color;
            
            ++tile_count;
            if (tile_count >= MAX_TILE_COUNT)
                break;
        }
        
        return sp_render_device->render(p, tile_count, this->tex);
    }
    
    RenderDevice::RenderDevice(int screen_width, int screen_height)
    : screen_width(screen_width)
    , screen_height(screen_height)
    , h_vertex_arrays(1)
    , h_buffers(2)
    {
        this->vao = this->h_vertex_arrays.get(0);
        this->vbo = this->h_buffers.get(0);
        this->ebo = this->h_buffers.get(1);
    }
    
    std::shared_ptr<RenderDevice> RenderDevice::Create(int screen_width, int screen_height, const std::uint8_t* p_vert_shader_data, std::size_t vert_shader_data_size, const std::uint8_t* p_frag_shader_data, std::size_t frag_shader_data_size)
    {
        static GLushort elements[] =
        {
            0, 1, 2,
            2, 3, 0,
        };
        
        auto sp_render_device = std::shared_ptr<RenderDevice>(new RenderDevice(screen_width, screen_height));
        
        int r;
        GLenum error;
        do
        {
            GlHandle<OpGlShader> h_vertex_shader(GL_VERTEX_SHADER);
            {
                const GLchar* vertex_shader_data_list[1] =
                {
                    reinterpret_cast<const GLchar*>(p_vert_shader_data)
                };
                GLint vertex_shader_size_list[1] =
                {
                    static_cast<GLint>(vert_shader_data_size)
                };
                glShaderSource(h_vertex_shader, 1, vertex_shader_data_list, vertex_shader_size_list);
                glCompileShader(h_vertex_shader);
                glGetShaderiv(h_vertex_shader, GL_COMPILE_STATUS, &r);
                if (r != GL_TRUE)
                {
                    char buffer[512];
                    glGetShaderInfoLog(h_vertex_shader, 512, NULL, buffer);
                    std::cerr << buffer << std::endl;
                    break;
                }
            }
            
            GlHandle<OpGlShader> h_fragment_shader(GL_FRAGMENT_SHADER);
            {
                const GLchar* frag_shader_data_list[1] =
                {
                    reinterpret_cast<const GLchar*>(p_frag_shader_data)
                };
                GLint frag_shader_size_list[1] =
                {
                    static_cast<GLint>(frag_shader_data_size)
                };
                glShaderSource(h_fragment_shader, 1, frag_shader_data_list, frag_shader_size_list);
                glCompileShader(h_fragment_shader);
                glGetShaderiv(h_fragment_shader, GL_COMPILE_STATUS, &r);
                if (r != GL_TRUE)
                {
                    char buffer[512];
                    glGetShaderInfoLog(h_fragment_shader, 512, NULL, buffer);
                    std::cerr << buffer << std::endl;
                    break;
                }
            }
            
            glAttachShader(sp_render_device->h_program, h_vertex_shader);
            glAttachShader(sp_render_device->h_program, h_fragment_shader);
            glLinkProgram(sp_render_device->h_program);
            
            
            glBindBuffer(GL_ARRAY_BUFFER, sp_render_device->vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * 4 * MAX_TILE_COUNT, nullptr, GL_STREAM_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sp_render_device->ebo);
            {
                std::vector<GLushort> index_data(MAX_TILE_COUNT * 6);
                GLushort* p_index_data = &index_data[0];
                for (int i = 0; i < MAX_TILE_COUNT; ++i) {
                    GLushort *p = p_index_data + i * 6;
                    GLushort base = i * 4;
                    p[0] = elements[0] + base;
                    p[1] = elements[1] + base;
                    p[2] = elements[2] + base;
                    p[3] = elements[3] + base;
                    p[4] = elements[4] + base;
                    p[5] = elements[5] + base;
                }
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * MAX_TILE_COUNT * 6, p_index_data, GL_STATIC_DRAW);
            }
            
            glBindVertexArray(sp_render_device->vao);
            GLint pos_attrib = glGetAttribLocation(sp_render_device->h_program, "position");
            glEnableVertexAttribArray(pos_attrib);
            glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, pos));
            GLint tex_attrib = glGetAttribLocation(sp_render_device->h_program, "texcoord");
            glEnableVertexAttribArray(tex_attrib);
            glVertexAttribPointer(tex_attrib, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, tex));
            GLint color_attrib = glGetAttribLocation(sp_render_device->h_program, "color");
            glEnableVertexAttribArray(color_attrib);
            glVertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, color));
            sp_render_device->shader_sampler = glGetUniformLocation(sp_render_device->h_program, "TexSampler");
            glBindVertexArray(0);
            
            error = glGetError();
            if (error != GL_NO_ERROR)
            {
                std::cerr << "OpenGL error: " << error << std::endl;
                return nullptr;
            }
        } while (false);
        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error: " << error << std::endl;
            return nullptr;
        }
        else
        {
            sp_render_device->wp_self = sp_render_device;
            return sp_render_device;
        }
    }

    std::unique_ptr<ITileRender> RenderDevice::CreateTextureAtlasRender(const IResourceDataSet& data_set, std::uint16_t unit_length, std::uint8_t width, std::uint8_t height, int& out_error_code)
    {
        return std::move(TextureAtlasRender::Create(this->wp_self, data_set, unit_length, width, height, out_error_code));
    }
    
    int RenderDevice::render(const TileVertex* p_vertex_buffer, std::size_t tile_count, GLuint tex_id)
    {
        GLenum error;
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glUseProgram(this->h_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glUniform1i(this->shader_sampler, 0);
        glBindVertexArray(this->vao);
        glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * 4 * MAX_TILE_COUNT, nullptr, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TileVertex) * tile_count * 4, p_vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6 * tile_count), GL_UNSIGNED_SHORT, nullptr);
        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error: " << error << std::endl;
            return 1;
        }
        return 0;
    }
}