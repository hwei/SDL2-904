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
#include "resource.h"
#include "glm/gtc/matrix_access.hpp"


namespace hardrock
{
    Renderer::Renderer(int screen_width, int screen_height)
    : screen_width(screen_width)
    , screen_height(screen_height)
    , b_valid(false)
    , h_vertex_arrays(1)
    , h_buffers(2)
    , h_textures(1)
    , vertex_buffer(MAX_TILE_COUNT * 4)
    {
        static GLushort elements[] = {
            0, 1, 2,
            2, 3, 0,
        };
        
        int r;
        GLenum error;
        
        do
        {
            r = load_resource("test_tex.webp", nullptr, 0);
            assert(r > 0);
            int test_img_size = r;
            std::vector<uint8_t> test_img_data(test_img_size);
            r = load_resource("test_tex.webp", &test_img_data[0], test_img_size);
            assert(r == test_img_size);
            int tex_width, tex_height;
            r = WebPGetInfo(&test_img_data[0], test_img_size, &tex_width, &tex_height);
            assert(r);
            int tex_stride = tex_width * 4 * sizeof(uint8_t);
            int tex_size = tex_stride * tex_height;
            std::vector<uint8_t> tex_data(tex_size);
            uint8_t* decode_result = WebPDecodeRGBAInto(&test_img_data[0], test_img_size, &tex_data[0], tex_size, tex_stride);
            assert(decode_result != nullptr);
            
            this->tex = this->h_textures.get(0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, decode_result);
            
            GlHandle<OpGlShader> h_vertex_shader(GL_VERTEX_SHADER);
            r = load_resource("test.vert", nullptr, 0);
            assert(r > 0);
            int vertex_shader_size = r;
            std::vector<char> vertex_shader_data(vertex_shader_size);
            r = load_resource("test.vert", &vertex_shader_data[0], vertex_shader_size);
            assert(r == vertex_shader_size);
            const GLchar* vertex_shader_list[1];
            vertex_shader_list[0] = &vertex_shader_data[0];
            glShaderSource(h_vertex_shader, 1, vertex_shader_list, &vertex_shader_size);
            glCompileShader(h_vertex_shader);
            glGetShaderiv(h_vertex_shader, GL_COMPILE_STATUS, &r);
            if (r != GL_TRUE)
            {
                char buffer[512];
                glGetShaderInfoLog(h_vertex_shader, 512, NULL, buffer);
                std::cerr << buffer << std::endl;
                break;
            }
            
            GlHandle<OpGlShader> h_fragment_shader(GL_FRAGMENT_SHADER);
            r = load_resource("test.frag", nullptr, 0);
            assert(r > 0);
            int fragment_shader_size = r;
            std::vector<char> fragment_shader_data(fragment_shader_size);
            r = load_resource("test.frag", &fragment_shader_data[0], fragment_shader_size);
            assert(r == fragment_shader_size);
            const GLchar* fragment_shader_list[1];
            fragment_shader_list[0] = &fragment_shader_data[0];
            glShaderSource(h_fragment_shader, 1, fragment_shader_list, &fragment_shader_size);
            glCompileShader(h_fragment_shader);
            glGetShaderiv(h_fragment_shader, GL_COMPILE_STATUS, &r);
            if (r != GL_TRUE)
            {
                char buffer[512];
                glGetShaderInfoLog(h_fragment_shader, 512, NULL, buffer);
                std::cerr << buffer << std::endl;
                break;
            }
            
            glAttachShader(this->h_program, h_vertex_shader);
            glAttachShader(this->h_program, h_fragment_shader);
            glLinkProgram(this->h_program);
            
            this->vbo = this->h_buffers.get(0);
            glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * 4 * MAX_TILE_COUNT, nullptr, GL_STREAM_DRAW);
            this->ebo = this->h_buffers.get(1);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
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
    
    int Renderer::RenderTiles(size_t count, const Tile* p_tiles)
    {
        assert(count < MAX_TILE_COUNT);
        float xm = 2.0f / this->screen_width;
        float ym = -2.0f / this->screen_height;
        float xa = -1.0f - 0.5f / this->screen_width;
        float ya = 1.0f + 0.5f / this->screen_height;
        TileVertex* p_vertex_buffer = &this->vertex_buffer[0];
        for (size_t i = 0; i < count; ++i)
        {
            TileVertex* pv = p_vertex_buffer + (i << 2);
            TileVertex* pv0 = pv;
            TileVertex* pv1 = pv + 1;
            TileVertex* pv2 = pv + 2;
            TileVertex* pv3 = pv + 3;
            const Tile* pt = p_tiles + i;
            glm::vec2 translate(pt->translate.x * xm + xa, pt->translate.y * ym + ya);
            glm::u8vec4 tex = pt->tex;
            glm::u8vec4 color = pt->color;
            glm::vec2 vec_x(pt->transform[0].x * xm, pt->transform[0].y * ym);
            glm::vec2 vec_y(pt->transform[1].x * xm, pt->transform[1].y * ym);
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
        }
        
        GLenum error;
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glUseProgram(this->h_program);
        glUniform1i(this->shader_sampler, 0);
        glBindVertexArray(this->vao);
        glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * 4 * MAX_TILE_COUNT, nullptr, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TileVertex) * count * 4, p_vertex_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(6 * count), GL_UNSIGNED_SHORT, nullptr);
        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error: " << error << std::endl;
        }
        return 0;
    }
}