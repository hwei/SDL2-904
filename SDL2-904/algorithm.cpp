//
//  utils.cpp
//  SDL2-904
//
//  Created by Huang Wei on 14-5-2.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#include "algorithm.h"
#include <vector>
#include <iostream>
#include "glm/gtx/fast_trigonometry.hpp"
#include "glm/gtc/constants.hpp"

namespace hardrock
{
    void FastSinCos(float r, float* p_out_sin, float* p_out_cos)
    {
        static const float half_pi = glm::half_pi<float>();
        static const float pi = glm::pi<float>();
        static const float one_half_pi = pi + half_pi;
        static const float double_pi = pi + pi;
        r = glm::mod(r, double_pi);
        if (r < half_pi)
        {
            *p_out_sin = glm::fastSin(r);
            *p_out_cos = glm::fastCos(r);
        }
        else if (r < pi)
        {
            r = pi - r;
            *p_out_sin = glm::fastSin(r);
            *p_out_cos = -glm::fastCos(r);
        }
        else if (r < one_half_pi)
        {
            r = r - pi;
            *p_out_sin = -glm::fastSin(r);
            *p_out_cos = -glm::fastCos(r);
        }
        else
        {
            r = double_pi - r;
            *p_out_sin = -glm::fastSin(r);
            *p_out_cos = glm::fastCos(r);
        }
    }
    
    unsigned int FnvHash::fnvHash(const char* str)
    {
        unsigned int hash = OFFSET_BASIS;
        while (true)
        {
            const char c = *str;
            hash ^= c;
            hash *= FNV_PRIME;
            if (c == 0)
                break;
            ++str;
        }
        return hash;
    }

    int TexturePack(std::uint8_t width, std::uint8_t height, std::uint16_t count, const TexturePackInput* sizes, TexturePackOutput* out_positions)
    {
        if (width > 128) return -1;
        if (height > 128) return -1;
        if (sizes == nullptr || out_positions == nullptr) return -1;
        if (count == 0) return -2;
        
        typedef std::pair<std::uint16_t, std::uint16_t> InnerNode;
        static const std::uint16_t NODE_FULL = -2;
        static const std::uint16_t NODE_EMPTY = -1;
        
        std::vector<std::uint16_t> wide_to_narrow_idx_list(count);
        for (size_t i = 0; i < wide_to_narrow_idx_list.size(); ++i)
        {
            wide_to_narrow_idx_list[i] = i;
        }
        std::sort(wide_to_narrow_idx_list.begin(), wide_to_narrow_idx_list.end(),
            [sizes](std::uint16_t a, std::uint16_t b)
        {
            return sizes[a].width > sizes[b].width;
        });
        
        const std::uint8_t max_inner_node_count = count << 1;
        std::vector<InnerNode> inner_node_list(max_inner_node_count);
        std::vector<std::uint8_t> split_list;
        split_list.reserve(max_inner_node_count);
        
        struct VisitStackData
        {
            std::uint8_t x, y, width, height;
            std::uint16_t* p_node_idx;
            std::uint16_t vertical_split;
        };
        std::vector<VisitStackData> visit_stack;
        visit_stack.reserve(max_inner_node_count);
        std::uint16_t root_node_idx = NODE_EMPTY;
        for (std::uint16_t i = 0; i < count; ++i)
        {
            const std::uint16_t tex_idx = wide_to_narrow_idx_list[i];
            const TexturePackInput* p_size = sizes + tex_idx;
            printf("size: %d %d\n", p_size->width, p_size->height);

            if (root_node_idx == NODE_FULL)
                return -3;
            visit_stack.resize(0);
            visit_stack.push_back({0, 0, width, height, &root_node_idx, true});
            printf("visit_stack.push_back %d %d %d %d ? %d\n", 0, 0, width, height, true);
            bool inserted = false;
            while (visit_stack.size())
            {
                const VisitStackData visit_data = visit_stack.back();
                visit_stack.pop_back();
                printf("visit_stack.pop_back %d %d %d %d ? %d\n", visit_data.x, visit_data.y, visit_data.width, visit_data.height, visit_data.vertical_split);
                if (p_size->width > visit_data.width || p_size->height > visit_data.height)
                {
                    std::cout << "too large" << std::endl;
                    continue;
                }
                if (*visit_data.p_node_idx == NODE_EMPTY)
                {
                    const bool same_width = p_size->width == visit_data.width;
                    const bool same_height = p_size->height == visit_data.height;
                    if (same_width && same_height)
                    {
                        *visit_data.p_node_idx = NODE_FULL;
                    }
                    else if (visit_data.vertical_split)
                    {
                        const std::uint16_t new_v_node_idx = split_list.size();
                        *visit_data.p_node_idx = new_v_node_idx;
                        split_list.push_back(p_size->width);
                        std::cout << "add v node " << static_cast<int>(p_size->width) << std::endl;
                        InnerNode& new_v_inner_node = inner_node_list[new_v_node_idx];
                        if (same_height)
                        {
                            new_v_inner_node.second = NODE_EMPTY;
                            new_v_inner_node.first = NODE_FULL;
                        }
                        else
                        {
                            if (same_width)
                                new_v_inner_node.second = NODE_FULL;
                            else
                                new_v_inner_node.second = NODE_EMPTY;
                            const std::uint16_t new_h_node_idx = split_list.size();
                            split_list.push_back(p_size->height);
                            std::cout << "add h node " << static_cast<int>(p_size->height) << std::endl;
                            new_v_inner_node.first = new_h_node_idx;
                            InnerNode& new_h_inner_node = inner_node_list[new_h_node_idx];
                            new_h_inner_node.first = NODE_FULL;
                            new_h_inner_node.second = NODE_EMPTY;
                        }
                    }
                    else
                    {
                        const std::uint16_t new_h_node_idx = split_list.size();
                        *visit_data.p_node_idx = new_h_node_idx;
                        split_list.push_back(p_size->height);
                        std::cout << "add h node " << static_cast<int>(p_size->height) << std::endl;
                        InnerNode& new_h_inner_node = inner_node_list[new_h_node_idx];
                        if (same_width)
                        {
                            new_h_inner_node.second = NODE_EMPTY;
                            new_h_inner_node.first = NODE_FULL;
                        }
                        else
                        {
                            if (same_height)
                                new_h_inner_node.second = NODE_FULL;
                            else
                                new_h_inner_node.second = NODE_EMPTY;
                            const std::uint16_t new_v_node_idx = split_list.size();
                            split_list.push_back(p_size->width);
                            std::cout << "add v node " << static_cast<int>(p_size->width) << std::endl;
                            new_h_inner_node.first = new_v_node_idx;
                            InnerNode& new_v_inner_node = inner_node_list[new_v_node_idx];
                            new_v_inner_node.first = NODE_FULL;
                            new_v_inner_node.second = NODE_EMPTY;
                        }
                    }
                    
                    auto p_out = out_positions + tex_idx;
                    p_out->x = visit_data.x;
                    p_out->y = visit_data.y;
                    printf("put tex %d %d\n", visit_data.x, visit_data.y);
                    inserted = true;
                    break;
                }
                else // must not be NODE_FULL
                {
                    InnerNode& inner_node = inner_node_list[*visit_data.p_node_idx];
                    const std::uint8_t split = split_list[*visit_data.p_node_idx];
                    bool is_full = true;
                    if (visit_data.vertical_split)
                    {
                        if (inner_node.second != NODE_FULL)
                        {
                            const std::uint8_t right_width = visit_data.width - split;
                            if (right_width >= p_size->width)
                            {
                                const std::uint8_t right_x = visit_data.x + split;
                                visit_stack.push_back({right_x, visit_data.y, right_width, visit_data.height, &inner_node.second, false});
                                printf("visit_stack.push_back1 %d %d %d %d ? false\n", right_x, visit_data.y, right_width, visit_data.height);
                            }
                            is_full = false;
                        }
                        if (inner_node.first != NODE_FULL)
                        {
                            if (split >= p_size->width)
                            {
                                visit_stack.push_back({visit_data.x, visit_data.y, split, visit_data.height, &inner_node.first, false});
                                printf("visit_stack.push_back2 %d %d %d %d ? false\n", visit_data.x, visit_data.y, split, visit_data.height);
                            }
                            is_full = false;
                        }
                    }
                    else
                    {
                        if (inner_node.second != NODE_FULL)
                        {
                            const std::uint8_t bottom_height = visit_data.height - split;
                            if (bottom_height >= p_size->height)
                            {
                                const std::uint8_t bottom_y = visit_data.y + split;
                                visit_stack.push_back({visit_data.x, bottom_y, visit_data.width, bottom_height, &inner_node.second, true});
                                printf("visit_stack.push_back3 %d %d %d %d ? true\n", visit_data.x, bottom_y, visit_data.width, bottom_height);
                            }
                            is_full = false;
                        }
                        if (inner_node.first != NODE_FULL)
                        {
                            if (split >= p_size->height)
                            {
                                visit_stack.push_back({visit_data.x, visit_data.y, visit_data.width, split, &inner_node.first, true});
                                printf("visit_stack.push_back4 %d %d %d %d ? true\n", visit_data.x, visit_data.y, visit_data.width, split);
                            }
                            is_full = false;
                        }
                    }
                    if (is_full)
                    {
                        *visit_data.p_node_idx = NODE_FULL;
                    }
                }
            }
            if (!inserted)
                return -3;
        }
        return 0;
    }
}