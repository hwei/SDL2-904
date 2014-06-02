//
//  scene.h
//  SDL2-904
//
//  Created by Huang Wei on 14-4-25.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef SDL2_904_scene_h
#define SDL2_904_scene_h

#include <vector>
#include <memory>
#include "tile.h"
#include "glm/vec3.hpp"
#include "structure.h"

namespace hardrock
{
    class TileSet
    {
    public:
        typedef CircleLinkedListPool::IndexType IndexType;
    private:
        static const IndexType USED_TILE_LIST_HEAD = 0;
        static const IndexType FREE_TILE_LIST_HEAD = 1;
        const std::size_t capacity;
        std::vector<Tile> tile_data;
        CircleLinkedListPool tile_list_pool;
        std::size_t tile_count;
    public:
        TileSet(std::size_t capacity);
        IndexType TileAdd(IndexType insert_after_idx = USED_TILE_LIST_HEAD);
        int TileRemove(IndexType tile_idx);
        Tile& TileAt(IndexType tile_idx);
        class TileSequence : public ITileSequence
        {
            const std::vector<Tile>* const p_tile_data;
            const CircleLinkedListPool* const p_tile_list_pool;
            IndexType next_idx;
        public:
            TileSequence(const std::vector<Tile>* p_tile_data, const CircleLinkedListPool* p_tile_list_pool);
            bool HasNext() const override;
            const Tile& Next() override;
        };
        TileSequence GetTileSequence() const;
    };
}

#endif
