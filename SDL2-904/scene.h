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
#include "tile.h"
#include "glm/vec3.hpp"
#include "structure.h"

namespace hardrock
{
    typedef std::uint16_t ObjId;
    
    struct SceneModify
    {
        std::vector<ObjId> layer_add_list;
        std::vector<ObjId> layer_del_list;
        std::vector<ObjId> layer_change_list;
        std::vector<glm::dvec3> layer_change_data;
        struct TileAndLayer
        {
            ObjId tile_id;
            ObjId layer_id;
        };
        std::vector<TileAndLayer> tile_add_or_move_list;
        std::vector<ObjId> tile_del_list;
        std::vector<ObjId> tile_change_list;
        std::vector<Tile> tile_change_data;
    };

    class Scene
    {
    public:
        typedef CircleLinkedListPool::IndexType IndexType;
        struct Layer
        {
            glm::vec3 pos;
        };
    private:
        struct LayerNode
        {
            Layer layer;
            IndexType tile_list_head;
            IndexType padding;
        };
        static const size_t MAX_TILE_COUNT = 1000;
        static const size_t MAX_LAYER_COUNT = 20;
        static const IndexType FREE_TILE_LIST_HEAD = 0;
        static const IndexType FREE_LAYER_LIST_HEAD = 0;
        static const IndexType LAYER_LIST_HEAD = 1;

        std::vector<Tile> tile_data;
        CircleLinkedListPool tile_list_pool;
        size_t tile_count;
        
        std::vector<LayerNode> layer_data;
        CircleLinkedListPool layer_list_pool;

        std::vector<const LayerNode*> sorted_layers; // sorted by layer z
        bool layers_need_sort;
    public:
        Scene();
        IndexType LayerAdd();
        int LayerDelete(IndexType layer_idx);
        Layer& LayerAt(IndexType layer_idx);
        IndexType TileAdd(IndexType layer_idx);
        int TileDelete(IndexType tile_idx);
        Tile& TileAt(IndexType layer_idx);
        int Render(ITileRender* p_tile_render);
    };

}

#endif
