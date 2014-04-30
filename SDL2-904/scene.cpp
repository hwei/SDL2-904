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
    Scene::Scene()
    : tile_data(MAX_TILE_COUNT + 1)
    , tile_list_pool(MAX_TILE_COUNT + 1)
    , layer_data(MAX_LAYER_COUNT + 2)
    , layer_list_pool(MAX_LAYER_COUNT + 2)
    , tile_count(0)
    , layers_need_sort(true)
    {
        // make layer_list_pool[1] a new circle head
        this->layer_list_pool.MoveTo(1, 1);
    }
    
    Scene::IndexType Scene::LayerAdd()
    {
        IndexType layer_idx = this->layer_list_pool.Next(FREE_LAYER_LIST_HEAD);
        if (layer_idx == 0)
            return 0;
        IndexType tile_list_head = this->tile_list_pool.Next(FREE_TILE_LIST_HEAD);
        if (tile_list_head == 0)
            return 0;
        // make a circle
        this->tile_list_pool.MoveTo(tile_list_head, tile_list_head);
        this->layer_list_pool.MoveTo(layer_idx, LAYER_LIST_HEAD);
        this->layer_data[layer_idx].tile_list_head = tile_list_head;
        this->layers_need_sort = true;
        return layer_idx;
    }

    int Scene::LayerDelete(IndexType layer_idx)
    {
        if (layer_idx == 0 || layer_idx >= this->layer_data.size())
            return -1;
        // delete tiles
        IndexType tile_list_head = this->layer_data[layer_idx].tile_list_head;
        this->tile_list_pool.Cross(FREE_TILE_LIST_HEAD, tile_list_head);
        // delete layer
        this->layer_list_pool.MoveTo(layer_idx, FREE_LAYER_LIST_HEAD);
        this->layers_need_sort = true;
        return 0;
    }
    
    Scene::Layer& Scene::LayerAt(IndexType layer_idx)
    {
        this->layers_need_sort = true;
        return this->layer_data[layer_idx].layer;
    }
    
    Scene::IndexType Scene::TileAdd(IndexType layer_idx)
    {
        if (layer_idx == 0 || layer_idx >= this->layer_data.size())
            return 0;
        IndexType tile_idx = this->tile_list_pool.Next(FREE_TILE_LIST_HEAD);
        if (tile_idx == 0)
            return 0;
        IndexType tile_list_head = this->layer_data[layer_idx].tile_list_head;
        this->tile_list_pool.MoveTo(tile_idx, tile_list_head);
        ++this->tile_count;
        return tile_idx;
    }
    
    int Scene::TileDelete(IndexType tile_idx)
    {
        if (tile_idx == 0 || tile_idx >= this->tile_data.size())
            return -1;
        this->tile_list_pool.MoveTo(tile_idx, FREE_TILE_LIST_HEAD);
        --this->tile_count;
        return 0;
    }
    
    Tile& Scene::TileAt(IndexType tile_idx)
    {
        return this->tile_data[tile_idx];
    }
    
    int Scene::Render(ITileRender* p_tile_render)
    {
        if (this->layers_need_sort)
        {
            this->sorted_layers.resize(0);
            IndexType layer_idx = LAYER_LIST_HEAD;
            while (true)
            {
                layer_idx = this->layer_list_pool.Next(layer_idx);
                if (layer_idx == LAYER_LIST_HEAD)
                    break;
                this->sorted_layers.push_back(&this->layer_data[layer_idx]);
            }
            const LayerNode* p_layer_data = &this->layer_data[0];
            std::sort(this->sorted_layers.begin(), this->sorted_layers.end(), [p_layer_data](const LayerNode* l0, const LayerNode* l1) { return l0->layer.pos.z < l1->layer.pos.z; });
            this->layers_need_sort = false;
        }
        
        int r = p_tile_render->Begin(this->tile_count);
        if (r != 0)
            return r;

        for (const LayerNode* p_layer : this->sorted_layers)
        {
            if (p_layer->layer.pos.z < 0)
                continue;
            const IndexType tile_list_head = p_layer->tile_list_head;
            IndexType tile_idx = tile_list_head;
            while (true)
            {
                tile_idx = this->tile_list_pool.Prev(tile_idx);
                if (tile_idx == tile_list_head)
                    break;
                p_tile_render->AddTile(this->tile_data[tile_idx]);
            }
        }

        return p_tile_render->End();
    }
}
