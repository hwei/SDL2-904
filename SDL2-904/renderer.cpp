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
    Renderer::Renderer(int screen_width, int screen_height, IResourceManager& resource_manager)
    : screen_width(screen_width)
    , screen_height(screen_height)
    , b_valid(false)
    , h_vertex_arrays(1)
    , h_buffers(2)
    , h_textures(1)
    , vertex_buffer(MAX_TILE_COUNT * 4)
    , xm(2.0f / screen_width)
    , ym(-2.0f / screen_height)
    , xa(-1.0f - 0.5f / screen_width)
    , ya(1.0f + 0.5f / screen_height)
    {
        static GLushort elements[] = {
            0, 1, 2,
            2, 3, 0,
        };

        int r;
        GLenum error;
        do
        {
            this->tex = this->h_textures.get(0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            {
                auto up_test_img_data = std::move(resource_manager.LoadResource(FnvHash("test_tex.webp")));
                assert(up_test_img_data.get() != nullptr);
                int tex_width, tex_height;
                r = WebPGetInfo(&up_test_img_data->at(0), up_test_img_data->size(), &tex_width, &tex_height);
                assert(r);
                int tex_stride = tex_width * 4 * sizeof(uint8_t);
                int tex_size = tex_stride * tex_height;
                std::vector<uint8_t> tex_data(tex_size);
                const uint8_t* decode_result = WebPDecodeRGBAInto(&up_test_img_data->at(0), up_test_img_data->size(), &tex_data[0], tex_size, tex_stride);
                assert(decode_result != nullptr);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, decode_result);
            }

            GlHandle<OpGlShader> h_vertex_shader(GL_VERTEX_SHADER);
            {
                auto up_vertex_shader_data = std::move(resource_manager.LoadResource(FnvHash("test.vert")));
                assert(up_vertex_shader_data.get() != nullptr);
                const GLchar* vertex_shader_data_list[1];
                vertex_shader_data_list[0] = reinterpret_cast<GLchar*>(&up_vertex_shader_data->at(0));
                GLint vertex_shader_size_list[1];
                vertex_shader_size_list[0] = static_cast<GLint>(up_vertex_shader_data->size());
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
                auto up_frag_shader_data = std::move(resource_manager.LoadResource(FnvHash("test.frag")));
                assert(up_frag_shader_data.get() != nullptr);
                const GLchar* frag_shader_data_list[1];
                frag_shader_data_list[0] = reinterpret_cast<GLchar*>(&up_frag_shader_data->at(0));
                GLint frag_shader_size_list[1];
                frag_shader_size_list[0] = static_cast<GLint>(up_frag_shader_data->size());
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

            glAttachShader(this->h_program, h_vertex_shader);
            glAttachShader(this->h_program, h_fragment_shader);
            glLinkProgram(this->h_program);

            this->vbo = this->h_buffers.get(0);
            glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * 4 * MAX_TILE_COUNT, nullptr, GL_STREAM_DRAW);
            this->ebo = this->h_buffers.get(1);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
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

            this->vao = this->h_vertex_arrays.get(0);
            glBindVertexArray(this->vao);
            GLint pos_attrib = glGetAttribLocation(this->h_program, "position");
            glEnableVertexAttribArray(pos_attrib);
            glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, pos));
            GLint tex_attrib = glGetAttribLocation(this->h_program, "texcoord");
            glEnableVertexAttribArray(tex_attrib);
            glVertexAttribPointer(tex_attrib, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, tex));
            GLint color_attrib = glGetAttribLocation(this->h_program, "color");
            glEnableVertexAttribArray(color_attrib);
            glVertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, color));

            this->shader_sampler = glGetUniformLocation(this->h_program, "TexSampler");

            glBindVertexArray(0);

            error = glGetError();
            if (error != GL_NO_ERROR)
            {
                std::cerr << "OpenGL error: " << error << std::endl;
            }
            else
            {
                this->b_valid = true;
            }
        } while (false);
        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error: " << error << std::endl;
        }
    }

    int Renderer::Begin(size_t count)
    {
        if (count > MAX_TILE_COUNT)
            return -1;
        this->tile_count = 0;
        return 0;
    }

    int Renderer::AddTile(const Tile& tile)
    {
        if (this->tile_count >= MAX_TILE_COUNT - 1)
            return -1;
        TileVertex* pv0 = &this->vertex_buffer[this->tile_count * 4];
        TileVertex* pv1 = pv0 + 1;
        TileVertex* pv2 = pv0 + 2;
        TileVertex* pv3 = pv0 + 3;
        glm::vec2 translate(tile.translate.x * this->xm + this->xa, tile.translate.y * this->ym + this->ya);
        glm::u8vec4 tex = tile.tex;
        glm::u8vec4 color = tile.color;
        glm::vec2 vec_x(tile.transform[0].x * this->xm, tile.transform[0].y * this->ym);
        glm::vec2 vec_y(tile.transform[1].x * this->xm, tile.transform[1].y * this->ym);
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
        ++this->tile_count;
        return 0;
    }

    int Renderer::End()
    {
        GLenum error;
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glUseProgram(this->h_program);
        glUniform1i(this->shader_sampler, 0);
        glBindVertexArray(this->vao);
        glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * 4 * MAX_TILE_COUNT, nullptr, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TileVertex) * this->tile_count * 4, &this->vertex_buffer[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6 * this->tile_count), GL_UNSIGNED_SHORT, nullptr);
        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error: " << error << std::endl;
        }
        return 0;
    }
}