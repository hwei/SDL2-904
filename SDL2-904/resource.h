//
//  resource.h
//  SDL2-904
//
//  Created by Huang Wei on 14-4-26.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef __SDL2_904__resource__
#define __SDL2_904__resource__

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <array>
#include <memory>

namespace hardrock
{
    struct IResourceDataSet
    {
        virtual std::size_t Count() const = 0;
        virtual int GetDataByRid(std::uint32_t rid, const std::uint8_t*& out_p_data, std::size_t& out_size) const = 0;
        virtual int GetDataByIdx(std::size_t idx, const std::uint8_t*& out_p_data, std::size_t& out_size) const = 0;
    };
    
    struct IResourceManager
    {
        virtual ~IResourceManager() { }
        virtual int LoadResource(std::uint32_t rid, void* out_buffer, size_t out_size) const = 0;
        virtual std::unique_ptr<std::vector<std::uint8_t>> LoadResource(std::uint32_t rid) const = 0;
        virtual std::unique_ptr<IResourceDataSet> LoadResourceBatch(const std::uint32_t* p_sorted_rid_list, size_t count) const = 0;
    };

    class PackResourceManager : public IResourceManager
    {
        struct Header
        {
            std::array<char, 4> identifier;
            std::uint32_t count;
            std::uint32_t index_pos;
        };
        struct Index
        {
            std::uint32_t rid;
            std::uint32_t pos;
            std::uint32_t size;
        };
        struct IndexSearchCmp
        {
            bool operator()(const Index& index, std::uint32_t rid) const { return index.rid < rid; }
        };
        std::vector<Index> index_list;
        std::string pack_path;
    public:
        PackResourceManager(const char* path);
        int LoadResource(std::uint32_t rid, void* out_buffer, size_t out_size) const override;
        std::unique_ptr<std::vector<std::uint8_t>> LoadResource(std::uint32_t rid) const override;
        std::unique_ptr<IResourceDataSet> LoadResourceBatch(const std::uint32_t* p_sorted_rid_list, size_t count) const override;
    };
}

#endif /* defined(__SDL2_904__resource__) */
