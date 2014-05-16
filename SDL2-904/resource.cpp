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
    
    std::unique_ptr<IResourceDataSet> PackResourceManager::LoadResourceBatch(const std::uint32_t* p_sorted_rid_list, size_t count) const
    {
        struct ResourceDataSet : public IResourceDataSet
        {
            std::vector<std::uint8_t> data;
            std::vector<Index> index_list;
            std::size_t Count() const override
            {
                return index_list.size();
            }
            int GetDataByRid(std::uint32_t rid, const std::uint8_t*& out_p_data, std::size_t& out_size) const override
            {
                auto result_iter = std::lower_bound(this->index_list.begin(), this->index_list.end(), rid, IndexSearchCmp());
                if (result_iter->rid != rid)
                {
                    out_p_data = nullptr;
                    out_size = 0;
                    return -1;
                }
                else
                {
                    out_p_data = &this->data[result_iter->pos];
                    out_size = result_iter->size;
                    return 0;
                }
            }
            int GetDataByIdx(std::size_t idx, const std::uint8_t*& out_p_data, std::size_t& out_size) const override
            {
                if (idx < this->index_list.size())
                {
                    const Index& index = this->index_list[idx];
                    out_p_data = &this->data[index.pos];
                    out_size = index.size;
                    return 0;
                }
                else
                {
                    out_p_data = nullptr;
                    out_size = 0;
                    return -1;
                }
            }
        };
        std::unique_ptr<ResourceDataSet> up_resourece_data_set(new ResourceDataSet());
        up_resourece_data_set->index_list.reserve(count);
        auto search_iter = this->index_list.begin();
        auto end_iter = this->index_list.end();
        std::vector<const Index*> index_list(count);
        std::uint32_t size = 0;
        for (size_t i = 0; i < count; ++i) {
            std::uint32_t rid = p_sorted_rid_list[i];
            auto result_iter = std::lower_bound(search_iter, end_iter, rid, IndexSearchCmp());
            if (result_iter->rid != rid)
                return nullptr;
            index_list[i] = &*result_iter;
            up_resourece_data_set->index_list.push_back({rid, size, result_iter->size});
            size += result_iter->size;
        }
        up_resourece_data_set->data.resize(size);
        std::uint8_t* p = &up_resourece_data_set->data[0];
        std::ifstream file(this->pack_path);
        for (const Index* p_index : index_list)
        {
            file.seekg(p_index->pos);
            file.read(reinterpret_cast<char*>(p), p_index->size);
            p += p_index->size;
        }
        return std::move(up_resourece_data_set);
    }
}