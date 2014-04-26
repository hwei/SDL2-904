//
//  resource.h
//  SDL2-904
//
//  Created by Huang Wei on 14-4-26.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef __SDL2_904__resource__
#define __SDL2_904__resource__

#include <cstddef>

namespace hardrock
{
    int load_resource(const char* path, void* out_buffer, size_t out_size);
}

#endif /* defined(__SDL2_904__resource__) */
