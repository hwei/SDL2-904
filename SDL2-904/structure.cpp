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

}