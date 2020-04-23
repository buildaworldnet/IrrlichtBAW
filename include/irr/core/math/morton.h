#ifndef __IRR_MORTON_H_INCLUDED__
#define __IRR_MORTON_H_INCLUDED__

#include <cstdint>
#include "IrrCompileConfig.h"
#include "irr/macros.h"
#include "irr/static_if.h"

namespace irr {
namespace core
{

namespace impl
{
    template <typename T>
    constexpr T morton2d_mask(uint32_t _n)
    {
        constexpr uint64_t mask[5] =
        {
            0x5555555555555555ull,
            0x3333333333333333ull,
            0x0F0F0F0F0F0F0F0Full,
            0x00FF00FF00FF00FFull,
            0x0000FFFF0000FFFFull
        };
        return static_cast<T>(mask[_n]);
    }

    template <typename T, uint32_t bitDepth>
    inline T morton2d_decode(T x)
    {
        x = x & morton2d_mask<T>(0);
        x = (x | (x >> 1)) & morton2d_mask<T>(1);
        x = (x | (x >> 2)) & morton2d_mask<T>(2);
        IRR_PSEUDO_IF_CONSTEXPR_BEGIN (bitDepth>8u)
        {
        x = (x | (x >> 4)) & morton2d_mask<T>(3);
        } IRR_PSEUDO_IF_CONSTEXPR_END
        IRR_PSEUDO_IF_CONSTEXPR_BEGIN (bitDepth>16u)
        {
        x = (x | (x >> 8)) & morton2d_mask<T>(4);
        } IRR_PSEUDO_IF_CONSTEXPR_END
        IRR_PSEUDO_IF_CONSTEXPR_BEGIN (bitDepth>32u)
        {
        x = (x | (x >> 16));
        } IRR_PSEUDO_IF_CONSTEXPR_END
        return x;
    }

    //! Puts bits on even positions filling gaps with 0s
    template <typename T, uint32_t bitDepth>
    inline T separate_bits_2d(T x)
    {
        IRR_PSEUDO_IF_CONSTEXPR_BEGIN (bitDepth>32u)
        {
        x = (x | (x << 16)) & morton2d_mask<T>(4);
        }
        IRR_PSEUDO_IF_CONSTEXPR_BEGIN (bitDepth>16u)
        {
        x = (x | (x << 8)) & morton2d_mask<T>(3);
        }
        IRR_PSEUDO_IF_CONSTEXPR_BEGIN (bitDepth>8u)
        {
        x = (x | (x << 4)) & morton2d_mask<T>(2);
        }
        x = (x | (x << 2)) & morton2d_mask<T>(1);
        x = (x | (x << 1)) & morton2d_mask<T>(0);

        return x;
    }
}

template<typename T, uint32_t bitDepth=sizeof(T)*8u>
T morton2d_decode_x(T _morton) { return impl::morton2d_decode<T,bitDepth>(_morton); }
template<typename T, uint32_t bitDepth=sizeof(T)*8u>
T morton2d_decode_y(T _morton) { return impl::morton2d_decode<T,bitDepth>(_morton>>1); }

template<typename T, uint32_t bitDepth=sizeof(T)*8u>
T morton2d_encode(T x, T y) { return impl::separate_bits_2d<T,bitDepth>(x) | (impl::separate_bits_2d<T,bitDepth>(y)<<1); }

}}

#endif