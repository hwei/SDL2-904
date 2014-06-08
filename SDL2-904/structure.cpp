//
//  structure.cpp
//  SDL2-904
//
//  Created by Huang Wei on 14-4-27.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#include "structure.h"
#include <cassert>
#include <limits>

namespace hardrock
{
    CircleLinkedListPool::CircleLinkedListPool(size_t size)
    : data_list(size)
    {
        assert(size <= std::numeric_limits<IndexType>::max() + 1);
        LinkedData* p_base = &this->data_list[0];
        IndexType i_prev = size - 1;
        LinkedData* p_prev = p_base + i_prev;
        for (IndexType i = 0; i < size; ++i) {
            LinkedData* p_curr = p_base + i;
            p_curr->prev = i_prev;
            p_prev->next = i;
            i_prev = i;
            p_prev = p_curr;
        }
    }
    
    CircleLinkedListPool::IndexType CircleLinkedListPool::MoveTo(IndexType idx_curr, IndexType idx_head)
    {
        assert(static_cast<size_t>(idx_curr) < this->data_list.size());
        assert(static_cast<size_t>(idx_head) < this->data_list.size());

        LinkedData* p_base = &this->data_list[0];
        LinkedData* p_curr = p_base + idx_curr;
        
        // remove
        IndexType idx_prev = p_curr->prev;
        LinkedData* p_prev = p_base + idx_prev;
        IndexType idx_next = p_curr->next;
        LinkedData* p_next = p_base + idx_next;
        p_curr->next = idx_curr;
        p_curr->prev = idx_curr;
        p_prev->next = idx_next;
        p_next->prev = idx_prev;
        
        // add
        LinkedData* p_head = p_base + idx_head;
        IndexType idx_tail = p_head->next;
        LinkedData* p_tail = p_base + idx_tail;
        p_curr->prev = idx_head;
        p_curr->next = idx_tail;
        p_head->next = idx_curr;
        p_tail->prev = idx_curr;
        
        return idx_next;
    }

    void CircleLinkedListPool::Cross(IndexType idx_head0, IndexType idx_head1)
    {
        assert(static_cast<size_t>(idx_head0) < this->data_list.size());
        assert(static_cast<size_t>(idx_head1) < this->data_list.size());

        LinkedData* p_base = &this->data_list[0];
        LinkedData* p_head0 = p_base + idx_head0;
        LinkedData* p_head1 = p_base + idx_head1;
        IndexType idx_tail0 = p_head0->next;
        LinkedData* p_tail0 = p_base + idx_tail0;
        IndexType idx_tail1 = p_head1->next;
        LinkedData* p_tail1 = p_base + idx_tail1;
        
        p_head0->next = idx_tail1;
        p_tail1->prev = idx_head0;
        p_head1->next = idx_tail0;
        p_tail0->prev = idx_head1;
    }

    CircleLinkedListPool::IndexType CircleLinkedListPool::Next(IndexType idx_curr) const
    {
        assert(static_cast<size_t>(idx_curr) < this->data_list.size());
        const LinkedData* p_base = &this->data_list[0];
        const LinkedData* p_curr = p_base + idx_curr;
        return p_curr->next;
    }

    CircleLinkedListPool::IndexType CircleLinkedListPool::Prev(IndexType idx_curr) const
    {
        assert(static_cast<size_t>(idx_curr) < this->data_list.size());
        const LinkedData* p_base = &this->data_list[0];
        const LinkedData* p_curr = p_base + idx_curr;
        return p_curr->prev;
    }

    SimpleMemoryAllocator::SimpleMemoryAllocator(std::size_t size)
    {
        free_list.push_back({0, size});
    }
    
    int SimpleMemoryAllocator::Allocate(std::size_t size, std::size_t& out_pos)
    {
        auto cmp = [](const FreeSpace& free_space, std::size_t s)
        {
            return free_space.size < s;
        };
        auto iter = std::lower_bound(this->free_list.begin(), this->free_list.end(), size, cmp);
        if (iter == this->free_list.end())
            return 1;
        out_pos = iter->pos;
        if (iter->size == size)
        {
            this->free_list.erase(iter);
        }
        else
        {
            iter->pos += size;
            iter->size -= size;
        }
        return 0;
    }
    
    int SimpleMemoryAllocator::Free(std::size_t pos, std::size_t size)
    {
        if (this->free_list.size() == 0)
        {
            this->free_list.push_back({pos, size});
            return 0;
        }
        auto cmp = [](std::size_t p, const FreeSpace& free_space)
        {
            return p < free_space.pos;
        };
        auto iter = std::upper_bound(this->free_list.begin(), this->free_list.end(), pos, cmp);
        if (iter == this->free_list.begin())
        {
            // free block is at the front of the list
            std::size_t end = pos + size;
            if (end < iter->pos)
            {
                this->free_list.insert(iter, {pos, size});
                return 0;
            }
            else if (end == iter->pos)
            {
                iter->pos = pos;
                iter->size += size;
                return 0;
            }
            else
            {
                return 1;
            }
        }
        if (iter == this->free_list.end())
        {
            // free block is at the end of the list
            --iter;
            std::size_t free_end = iter->pos + iter->size;
            if (free_end < pos)
            {
                this->free_list.push_back({pos, size});
                return 0;
            }
            else if (free_end == pos)
            {
                iter->size += size;
                return 0;
            }
            else
            {
                return 1;
            }
        }
        {
            std::size_t end = pos + size;
            if (end > iter->pos)
                return 1;
            auto prev_iter = iter - 1;
            std::size_t free_end = prev_iter->pos + prev_iter->size;
            if (pos < free_end)
                return 1;
            if (end == iter->pos && pos == free_end)
            {
                prev_iter->size += size + iter->size;
                this->free_list.erase(iter);
                return 0;
            }
            if (end == iter->pos)
            {
                iter->pos = pos;
                iter->size += size;
                return 0;
            }
            if (pos == free_end)
            {
                prev_iter->size += size;
                return 0;
            }
            {
                this->free_list.insert(iter, {pos, size});
                return 0;
            }
        }
    }
    
    void SimpleMemoryAllocator::DebugPrint() const
    {
        printf("SimpleMemoryPool\n");
        for (auto free_space : this->free_list)
        {
            printf("%lu %lu\n", free_space.pos, free_space.size);
        }
    }
    
    ObjRef::ObjRef()
    : ref_data(0)
    {
    }
    
    ObjRef::ObjRef(std::uint32_t ref_data)
    : ref_data(ref_data)
    {
    }
    
    ObjRef::ModifyHandle::ModifyHandle(ObjRef* p_obj_ref)
    : p_obj_ref(p_obj_ref)
    {
    }
    
    ObjRef::ModifyHandle::Pool::Pool(std::size_t size)
    : ref_item_pool(size)
    , ref_item_buffer(size)
    {
        this->ref_item_pool.MoveTo(USED_LIST_HEAD, USED_LIST_HEAD);
    }
    
    std::unique_ptr<ObjRef, ObjRef::Deleter> ObjRef::ModifyHandle::Pool::CreateObjRef(std::uint32_t ref_data, ModifyHandle* p_modify_handle)
    {
        const auto next_free_idx = this->ref_item_pool.Next(FREE_LIST_HEAD);
        if (next_free_idx == FREE_LIST_HEAD)
            return nullptr;
        this->ref_item_pool.MoveTo(next_free_idx, USED_LIST_HEAD);
        auto p_ref_item = &this->ref_item_buffer[next_free_idx];
        p_ref_item->ref_data = ref_data;
        p_modify_handle->p_obj_ref = p_ref_item;
        return std::unique_ptr<ObjRef, ObjRef::Deleter>(p_ref_item);
    }
    
    void ObjRef::ModifyHandle::Pool::Remove(ModifyHandle modify_handle)
    {
        const auto ref_idx =  modify_handle.p_obj_ref - &this->ref_item_buffer[0];
        this->ref_item_pool.MoveTo(ref_idx, FREE_LIST_HEAD);
    }
}