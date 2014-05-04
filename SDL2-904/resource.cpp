//
//  resource.cpp
//  SDL2-904
//
//  Created by Huang Wei on 14-4-26.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#include "resource.h"
#include <CoreFoundation/CFBundle.h>
#include <fstream>
#include <algorithm>
#include "webp/decode.h"
#include "algorithm.h"

namespace
{
    struct CFDel
    {
        void operator() (CFTypeRef ref) { CFRelease(ref); }
    };

    template <typename T>
    std::unique_ptr<T, CFDel> create_cfptr(T* ref)
    {
        return std::unique_ptr<T, CFDel>(ref);
    }

    std::string FindResource(const char* path)
    {
        auto system_encoding = CFStringGetSystemEncoding();
        auto cp_path = create_cfptr(CFStringCreateWithCString(kCFAllocatorDefault, path, system_encoding));
        auto main_bundle = CFBundleGetMainBundle();
        auto cp_data_url = create_cfptr(
                                        CFBundleCopyResourceURL(main_bundle, cp_path.get(), nullptr, nullptr));
        auto cp_data_path = create_cfptr(CFURLCopyFileSystemPath(cp_data_url.get(), kCFURLPOSIXPathStyle));
        const char* data_path = CFStringGetCStringPtr(cp_data_path.get(), system_encoding);
        return data_path;
    }
}

namespace hardrock
{
    PackResourceManager::PackResourceManager(const char* path)
    : pack_path(FindResource("res.pack"))
    {
        Header header;
        std::ifstream file(pack_path);
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (header.identifier != std::array<char, 4>({'P', 'A', 'C', 'K'}))
            return;
        file.seekg(header.index_pos);
        this->index_list.resize(header.count);
        file.read(reinterpret_cast<char*>(&this->index_list[0]), header.count * sizeof(this->index_list[0]));
    }

    int PackResourceManager::LoadResource(std::uint32_t rid, void* out_buffer, size_t out_size) const
    {
        auto iter = std::lower_bound(this->index_list.begin(), this->index_list.end(), rid, IndexSearchCmp());
        if (iter == this->index_list.end() || iter->rid != rid)
        {
            return 0;
        }
        if (out_buffer != nullptr && out_size >= iter->size)
        {
            std::ifstream file(this->pack_path);
            file.seekg(iter->pos);
            file.read(reinterpret_cast<char*>(out_buffer), iter->size);
        }
        return static_cast<int>(iter->size);
    }

    std::unique_ptr<std::vector<std::uint8_t>> PackResourceManager::LoadResource(std::uint32_t rid) const
    {
        auto iter = std::lower_bound(this->index_list.begin(), this->index_list.end(), rid, IndexSearchCmp());
        if (iter == this->index_list.end() || iter->rid != rid)
        {
            return 0;
        }
        std::unique_ptr<std::vector<std::uint8_t>> up_buffer(new std::vector<std::uint8_t>(iter->size));
        std::ifstream file(this->pack_path);
        file.seekg(iter->pos);
        file.read(reinterpret_cast<char*>(&up_buffer->at(0)), iter->size);
        return up_buffer;
    }
    
    std::unique_ptr<std::vector<std::uint8_t>> PackResourceManager::LoadResourceBatch(
        const std::uint32_t* p_sorted_rid_list, size_t count, size_t* p_out_size_list) const
    {
        auto search_iter = this->index_list.begin();
        auto end_iter = this->index_list.end();
        std::vector<const Index*> index_list(count);
        size_t size = 0;
        for (size_t i = 0; i < count; ++i) {
            std::uint32_t rid = p_sorted_rid_list[i];
            auto result_iter = std::lower_bound(search_iter, end_iter, rid, IndexSearchCmp());
            if (result_iter->rid != rid)
                return nullptr;
            index_list[i] = &*result_iter;
            p_out_size_list[i] = result_iter->size;
            size += result_iter->size;
        }
        std::unique_ptr<std::vector<std::uint8_t>> up_data(new std::vector<std::uint8_t>(size));
        std::uint8_t* p = &up_data->at(0);
        std::ifstream file(this->pack_path);
        for (const Index* p_index : index_list)
        {
            file.seekg(p_index->pos);
            file.read(reinterpret_cast<char*>(p), p_index->size);
            p += p_index->size;
        }
        return up_data;
    }
    
    std::unique_ptr<std::vector<std::uint8_t>> WebpToPackedTexture(
        std::uint16_t unit_length, std::uint8_t width, std::uint8_t height,
        const std::uint8_t* webp_data_stream, const size_t* data_size_list, size_t data_count,
        PackedTextureRect* p_out_rect_list)
    {
        std::vector<TexturePackInput> pack_input(data_count);
        std::vector<TexturePackOutput> pack_output(data_count);
        const int int_unit_length = static_cast<int>(unit_length);
        const std::uint8_t* p_data_stream = webp_data_stream;
        for (size_t i = 0; i < data_count; ++i)
        {
            int w, h;
            int ok = WebPGetInfo(p_data_stream, data_size_list[i], &w, &h);
            if (!ok) return nullptr;
            p_data_stream += data_size_list[i];
            if (w % int_unit_length) return nullptr;
            if (h % int_unit_length) return nullptr;
            w /= int_unit_length;
            if (w > 128) return nullptr;
            h /= int_unit_length;
            if (h > 128) return nullptr;
            pack_input[i].width = static_cast<std::uint8_t>(w);
            pack_input[i].height = static_cast<std::uint8_t>(h);
            p_out_rect_list[i].width = pack_input[i].width;
            p_out_rect_list[i].height = pack_input[i].height;
        }
        int r = TexturePack(width, height, static_cast<std::uint16_t>(data_count), &pack_input[0], &pack_output[0]);
        if (r != 0) return nullptr;
        const size_t size_t_unit_length = static_cast<size_t>(unit_length);
        const size_t tex_width = static_cast<size_t>(width) * size_t_unit_length;
        const size_t tex_height = static_cast<size_t>(height) * size_t_unit_length;
        const size_t tex_stride = tex_width * 4 * sizeof(std::uint8_t);
        const int int_tex_stride = static_cast<int>(tex_stride);
        std::unique_ptr<std::vector<std::uint8_t>> up_image_data(new std::vector<std::uint8_t>(tex_height * tex_stride));
        p_data_stream = webp_data_stream;
        for (size_t i = 0; i < data_count; ++i)
        {
            const size_t x = static_cast<size_t>(pack_output[i].x) * size_t_unit_length;
            const size_t y = static_cast<size_t>(pack_output[i].y) * size_t_unit_length;
            const size_t sub_tex_height = static_cast<size_t>(pack_input[i].height) * size_t_unit_length;
            std::uint8_t * const p = &up_image_data->at(0) + y * tex_stride + x * 4 * sizeof(std::uint8_t);
            const uint8_t* decode_result = WebPDecodeRGBAInto(p_data_stream, data_size_list[i], p, sub_tex_height * tex_stride, int_tex_stride);
            if (decode_result == nullptr) return nullptr;
            p_data_stream += data_size_list[i];
            p_out_rect_list[i].x = pack_output[i].x;
            p_out_rect_list[i].y = pack_output[i].y;
        }
        return up_image_data;
    }
}