//
//  scene.cpp
//  SDL2-904
//
//  Created by Huang Wei on 14-4-25.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#include "scene.h"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>

namespace hardrock
{
    TileSet::TileSet(std::size_t capacity)
    : capacity(capacity)
    , tile_list_pool(capacity + 2)
    , tile_data(capacity + 2)
    , tile_count(0)
    {
        this->tile_list_pool.MoveTo(USED_TILE_LIST_HEAD, USED_TILE_LIST_HEAD);
    }
    
    TileSet::IndexType TileSet::TileAdd(IndexType insert_after_idx)
    {
        IndexType tile_idx = this->tile_list_pool.Next(FREE_TILE_LIST_HEAD);
        if (tile_idx == FREE_TILE_LIST_HEAD)
            return 0;
        this->tile_list_pool.MoveTo(tile_idx, insert_after_idx);
        ++this->tile_count;
        return tile_idx;
    }
    
    int TileSet::TileRemove(IndexType tile_idx)
    {
        if (tile_idx == 0 || tile_idx >= this->tile_data.size())
            return 1;
        this->tile_list_pool.MoveTo(tile_idx, FREE_TILE_LIST_HEAD);
        --this->tile_count;
        return 0;
    }
    
    Tile& TileSet::TileAt(IndexType tile_idx)
    {
        return this->tile_data[tile_idx];
    }
    
    TileSet::TileSequence::TileSequence(const std::vector<Tile>* p_tile_data, const CircleLinkedListPool* p_tile_list_pool)
    : p_tile_data(p_tile_data)
    , p_tile_list_pool(p_tile_list_pool)
    {
        this->next_idx = p_tile_list_pool->Next(USED_TILE_LIST_HEAD);
    }
    
    bool TileSet::TileSequence::HasNext() const
    {
        return this->next_idx != USED_TILE_LIST_HEAD;
    }
    
    const Tile& TileSet::TileSequence::Next()
    {
        IndexType c = this->next_idx;
        this->next_idx = p_tile_list_pool->Next(c);
        return this->p_tile_data->at(c);
    }
    
    TileSet::TileSequence TileSet::GetTileSequence() const
    {
        return TileSequence(&this->tile_data, &this->tile_list_pool);
    }
}
