//
//  utils.h
//  SDL2-904
//
//  Created by Huang Wei on 14-5-2.
//  Copyright (c) 2014年 hweigame. All rights reserved.
//

#ifndef __SDL2_904__utils__
#define __SDL2_904__utils__

#include <cstdint>
#include <cstddef>

namespace hardrock
{
    void FastSinCos(float r, float* p_out_sin, float* p_out_cos);
    
    class FnvHash
    {
        static const unsigned int FNV_PRIME = 16777619u;
        static const unsigned int OFFSET_BASIS = 2166136261u;
        template <unsigned int N>
        static constexpr unsigned int fnvHashConst(const char (&str)[N], unsigned int I = N)
        {
            return I == 1 ? (OFFSET_BASIS ^ str[0]) * FNV_PRIME : (fnvHashConst(str, I - 1) ^ str[I - 1]) * FNV_PRIME;
        }
        static unsigned int fnvHash(const char* str);
        struct Wrapper
        {
            Wrapper(const char* str) : str (str) { }
            const char* str;
        };
        unsigned int hash_value;
    public:
        // calulate in run-time
        FnvHash(Wrapper wrapper) : hash_value(fnvHash(wrapper.str)) { }
        // calulate in compile-time
        template <unsigned int N>
        constexpr FnvHash(const char (&str)[N]) : hash_value(fnvHashConst(str)) { }
        // output result
        constexpr operator unsigned int() const { return this->hash_value; }
    };
    
    struct TexturePackInput
    {
        std::uint8_t width;
        std::uint8_t height;
    };
    struct TexturePackOutput
    {
        std::uint8_t x;
        std::uint8_t y;
    };
    int TexturePack(std::uint8_t width, std::uint8_t height, std::uint16_t count, const TexturePackInput* sizes, TexturePackOutput* out_positions);
}

#endif /* defined(__SDL2_904__utils__) */
