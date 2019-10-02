// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_GLSL_FUNCTIONS_TCC_INCLUDED__
#define __IRR_GLSL_FUNCTIONS_TCC_INCLUDED__

#include "irr/core/math/glslFunctions.h"
#include "irr/core/math/floatutil.tcc"
#include "matrix4SIMD.h"

namespace irr
{
namespace core
{

template<>
IRR_FORCE_INLINE vectorSIMDf abs<vectorSIMDf>(const vectorSIMDf& a)
{
    return a&vectorSIMD_32<uint32_t>(0x7FFFFFFFu,0x7FFFFFFFu,0x7FFFFFFFu,0x7FFFFFFFu);
}
template<typename T>
IRR_FORCE_INLINE T abs(const T& a)
{
	auto isneg = a < T(0);
	using bool_type = std::remove_reference<decltype(isneg)>::type;
	return core::mix<T,bool_type>(a,-a,isneg);
}


template<>
IRR_FORCE_INLINE float sqrt<float>(const float& x)
{
	return std::sqrtf(x);
}
template<>
IRR_FORCE_INLINE double sqrt<double>(const double& x)
{
	return std::sqrt(x);
}
template<>
IRR_FORCE_INLINE vectorSIMDf sqrt(const vectorSIMDf& x)
{
#ifdef __IRR_COMPILE_WITH_X86_SIMD_
	return _mm_sqrt_ps(x.getAsRegister());
#else
#error "no implementation"
#endif
}

template<>
IRR_FORCE_INLINE vectorSIMDf inversesqrt<vectorSIMDf>(const vectorSIMDf& x)
{
#ifdef __IRR_COMPILE_WITH_X86_SIMD_
	return _mm_rsqrt_ps(x.getAsRegister());
#else
#error "no implementation"
#endif
}
template<typename T>
IRR_FORCE_INLINE T inversesqrt(const T& x)
{
	return core::reciprocal<T>(core::sqrt<T>(x));
}

template<>
IRR_FORCE_INLINE vectorSIMDf reciprocal_approxim<vectorSIMDf>(const vectorSIMDf& x)
{
#ifdef __IRR_COMPILE_WITH_X86_SIMD_
    return _mm_rcp_ps(x.getAsRegister());
#else
#error "no implementation"
#endif
}
template<>
IRR_FORCE_INLINE float reciprocal_approxim<float>(const float& x)
{
#if defined (__IRR_FAST_MATH) && defined ( __IRR_COMPILE_WITH_X86_SIMD_ )
    float result;
    _mm_store_ss(&result,_mm_rcp_ps(_mm_load_ss(&x)));
    return result;
#else // no fast math
	return reciprocal<T>(x);
#endif
}
template<typename T>
IRR_FORCE_INLINE T reciprocal_approxim(const T& x)
{
    return reciprocal<T>(x);
}




template<>
IRR_FORCE_INLINE float floor<float>(const float& x)
{
	return std::floorf(x);
}
template<>
IRR_FORCE_INLINE double floor<double>(const double& x)
{
	return std::floor(x);
}
template<>
IRR_FORCE_INLINE vectorSIMDf core::floor<vectorSIMDf>(const vectorSIMDf& x)
{
#ifdef __IRR_COMPILE_WITH_SSE3
	return _mm_floor_ps(x.getAsRegister());
#else
#error "no implementation"
#endif
}

/*
//! round towards zero
template<typename T>
IRR_FORCE_INLINE T trunc(const T& x);


//! No idea how this shit works, unimplemented
template<typename T>
IRR_FORCE_INLINE T roundEven(const T& x);
*/

template<>
IRR_FORCE_INLINE float core::ceil<float>(const float& x)
{
	return std::ceilf(x);
}
template<>
IRR_FORCE_INLINE double core::ceil<double>(const double& x)
{
	return std::ceil(x);
}
template<>
IRR_FORCE_INLINE vectorSIMDf core::ceil<vectorSIMDf>(const vectorSIMDf& x)
{
#ifdef __IRR_COMPILE_WITH_SSE3
	return _mm_ceil_ps(x.getAsRegister());
#else
#error "no implementation"
#endif
}

template<>
IRR_FORCE_INLINE vectorSIMDf min<vectorSIMDf>(const vectorSIMDf& a, const vectorSIMDf& b)
{
#ifdef __IRR_COMPILE_WITH_SSE3
	return _mm_min_ps(a.getAsRegister(),b.getAsRegister());
#else
#error "no implementation"
#endif
}
template<class T>
IRR_FORCE_INLINE T min(const T& a, const T& b)
{
	T vb = T(b);
	auto asmaller = a<vb;
	using bool_type = std::remove_reference<decltype(asmaller)>::type;
	return core::mix<T,bool_type>(vb,a,asmaller);
}

template<>
IRR_FORCE_INLINE vectorSIMDf max<vectorSIMDf>(const vectorSIMDf& a, const vectorSIMDf& b)
{
#ifdef __IRR_COMPILE_WITH_SSE3
	return _mm_max_ps(a.getAsRegister(),b.getAsRegister());
#else
#error "no implementation"
#endif
}
template<class T>
IRR_FORCE_INLINE T max(const T& a, const T& b)
{
	T vb = T(b);
	auto asmaller = a < vb;
	using bool_type = std::remove_reference<decltype(asmaller)>::type;
	return core::mix<T,bool_type>(a,vb,asmaller);
}


template<>
IRR_FORCE_INLINE vectorSIMDf dot<vectorSIMDf>(const vectorSIMDf& a, const vectorSIMDf& b)
{
    __m128 xmm0 = a.getAsRegister();
    __m128 xmm1 = b.getAsRegister();
#ifdef __IRR_COMPILE_WITH_SSE3
    xmm0 = _mm_mul_ps(xmm0,xmm1);
    xmm0 = _mm_hadd_ps(xmm0,xmm0);
    return _mm_hadd_ps(xmm0,xmm0);
#else
#error "no implementation"
#endif
}

template<>
IRR_FORCE_INLINE vectorSIMDf cross<vectorSIMDf>(const vectorSIMDf& a, const vectorSIMDf& b)
{
#ifdef __IRR_COMPILE_WITH_X86_SIMD_
    __m128 xmm0 = a.getAsRegister();
    __m128 xmm1 = b.getAsRegister();
    __m128 backslash = _mm_mul_ps(FAST_FLOAT_SHUFFLE(xmm0,_MM_SHUFFLE(3,0,2,1)),FAST_FLOAT_SHUFFLE(xmm1,_MM_SHUFFLE(3,1,0,2)));
    __m128 forwardslash = _mm_mul_ps(FAST_FLOAT_SHUFFLE(xmm0,_MM_SHUFFLE(3,1,0,2)),FAST_FLOAT_SHUFFLE(xmm1,_MM_SHUFFLE(3,0,2,1)));
    return _mm_sub_ps(backslash,forwardslash); //returns 0 in the last component :D
#else
#error "no implementation"
#endif
}


template<>
IRR_FORCE_INLINE int32_t findLSB<uint32_t>(uint32_t x)
{
#ifdef __GNUC__
	return __builtin_ffs(reinterpret_cast<const int32_t&>(x)) - 1;
#elif defined(_MSC_VER)
	unsigned long index;
	if (_BitScanForward(&index, reinterpret_cast<const long&>(x)))
		return index;
	return -1;
#endif
}
template<>
IRR_FORCE_INLINE int32_t findLSB<uint64_t>(uint64_t x)
{
#ifdef __GNUC__
	return __builtin_ffsl(reinterpret_cast<const int64_t&>(x)) - 1;
#elif defined(_MSC_VER)
	unsigned long index;
	if (_BitScanForward64(&index, reinterpret_cast<const __int64&>(x)))
		return index;
	return -1;
#endif
}

template<>
IRR_FORCE_INLINE int32_t findMSB<uint32_t>(uint32_t x)
{
#ifdef __GNUC__
	return x ? (31 - __builtin_clz(x)) : (-1);
#elif defined(_MSC_VER)
	unsigned long index;
	if (_BitScanReverse(&index, reinterpret_cast<const long&>(x)))
		return index;
	return -1;
#endif
}
template<>
IRR_FORCE_INLINE int32_t findMSB<uint64_t>(uint64_t x)
{
#ifdef __GNUC__
	return x ? (63 - __builtin_clzl(x)) : (-1);
#elif defined(_MSC_VER)
	unsigned long index;
	if (_BitScanReverse64(&index, reinterpret_cast<const __int64&>(x)))
		return index;
	return -1;
#endif
}


template<>
IRR_FORCE_INLINE bool equals<vectorSIMDf>(const vectorSIMDf& a, const vectorSIMDf& b, const vectorSIMDf& tolerance)
{
	return ((a + tolerance >= b) && (a - tolerance <= b)).all();
}
template<>
IRR_FORCE_INLINE bool equals<matrix4SIMD>(const matrix4SIMD& a, const matrix4SIMD& b, const matrix4SIMD& tolerance)
{
	for (size_t i = 0u; i<matrix4SIMD::VectorCount; ++i)
		if (!equals<vectorSIMDf>(a.rows[i], b.rows[i], tolerance.rows[i]))
			return false;
	return true;
}
template<>
IRR_FORCE_INLINE bool equals<matrix3x4SIMD>(const matrix3x4SIMD& a, const matrix3x4SIMD& b, const matrix3x4SIMD& tolerance)
{
	for (size_t i = 0u; i<matrix3x4SIMD::VectorCount; ++i)
		if (!equals<vectorSIMDf>(a.rows[i], b.rows[i], tolerance[i]))
			return false;
	return true;
}
template<typename T>
IRR_FORCE_INLINE bool equals(const T& a, const T& b, const T& tolerance)
{
	return (a + tolerance >= b) && (a - tolerance <= b);
}


} // end namespace core
} // end namespace irr

#endif

