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

    std::unique_ptr<ITileSequence> Scene::GetTileSequence()
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
                if (this->layer_data[layer_idx].layer.pos.z < 0)
                    continue;
                this->sorted_layers.push_back(&this->layer_data[layer_idx]);
            }
            const LayerNode* p_layer_data = &this->layer_data[0];
            std::sort(this->sorted_layers.begin(), this->sorted_layers.end(), [p_layer_data](const LayerNode* l0, const LayerNode* l1) { return l0->layer.pos.z < l1->layer.pos.z; });
            this->layers_need_sort = false;
        }
        
        struct TileSequence : public ITileSequence
        {
            const Scene *p_scene;
            const std::size_t count;
            std::vector<const LayerNode*>::const_iterator layer_iter;
            IndexType tile_list_head;
            IndexType tile_idx;
            bool end;
            
            TileSequence(const Scene *p_scene)
            : p_scene (p_scene)
            , count(p_scene->tile_count)
            , layer_iter(p_scene->sorted_layers.begin())
            , tile_list_head(0)
            , tile_idx(0)
            , end(false)
            {
                if (this->layer_iter != p_scene->sorted_layers.end())
                {
                    IndexType head_idx = (*this->layer_iter)->tile_list_head;
                    this->tile_list_head = head_idx;
                    this->tile_idx = head_idx;
                }
                else
                {
                    this->end = true;
                }
            }
            std::size_t Count() const override
            {
                return this->count;
            }
            bool Next(Tile& out_tile) override
            {
                if (this->end)
                    return false;
                while (true)
                {
                    this->tile_idx = this->p_scene->tile_list_pool.Prev(this->tile_idx);
                    if (this->tile_idx == this->tile_list_head)
                    {
                        ++this->layer_iter;
                        if (this->layer_iter == p_scene->sorted_layers.end())
                        {
                            this->end = true;
                            return false;
                        }
                        
                        IndexType head_idx = (*this->layer_iter)->tile_list_head;
                        this->tile_list_head = head_idx;
                        this->tile_idx = head_idx;
                    }
                    else
                    {
                        out_tile = this->p_scene->tile_data[tile_idx];
                        out_tile.translate.x += (*this->layer_iter)->layer.pos.x;
                        out_tile.translate.y += (*this->layer_iter)->layer.pos.y;
                        return true;
                    }
                }
            }
        };
        
        return std::unique_ptr<ITileSequence>(new TileSequence(this));
    }
}
