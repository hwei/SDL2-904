//
//  structure.h
//  SDL2-904
//
//  Created by Huang Wei on 14-4-27.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef __SDL2_904__structure__
#define __SDL2_904__structure__

#include <cstdint>
#include <vector>

namespace hardrock
{
    class CircleLinkedListPool
    {
    public:
        typedef std::uint16_t IndexType;
    private:
        struct LinkedData
        {
            IndexType prev;
            IndexType next;
        };
        std::vector<LinkedData> data_list;
    public:
        CircleLinkedListPool(size_t size);
        size_t Size() const { return this->data_list.size(); }
        // Move a element to another circle.
        // If it move to it self, a new circle is created.
        // return next node in origin cirle.
        IndexType MoveTo(IndexType idx_curr, IndexType idx_head);
        // If the 2 heads are in different circle lists, the two lists are combined into one.
        // If the 2 heads are in a same list, the list are splited into two lists.
        void Cross(IndexType idx_head0, IndexType idx_head1);
        IndexType Next(IndexType idx_curr) const;
        IndexType Prev(IndexType idx_curr) const;
    };
    
    class SimpleMemoryAllocator
    {
        struct FreeSpace
        {
            std::size_t pos;
            std::size_t size;
        };
        std::vector<FreeSpace> free_list;
    public:
        SimpleMemoryAllocator(std::size_t size);
        int Allocate(std::size_t size, std::size_t& out_pos);
        int Free(std::size_t pos, std::size_t size);
        void DebugPrint() const;
    };
}

#endif /* defined(__SDL2_904__structure__) */
