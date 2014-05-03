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
        auto iter = std::lower_bound(this->index_list.begin(), this->index_list.end(), rid,
            [](const Index& index, std::uint32_t rid) { return index.rid < rid; });
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
        auto iter = std::lower_bound(this->index_list.begin(), this->index_list.end(), rid,
            [](const Index& index, std::uint32_t rid) { return index.rid < rid; });
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
}