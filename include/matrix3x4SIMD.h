#ifndef __MATRIX3X4SIMD_H_INCLUDED__
#define __MATRIX3X4SIMD_H_INCLUDED__

#include "vectorSIMD.h"
#include "quaternion.h"

namespace irr
{
namespace core
{

#define _IRR_MATRIX_ALIGNMENT _IRR_SIMD_ALIGNMENT
static_assert(_IRR_MATRIX_ALIGNMENT>=_IRR_VECTOR_ALIGNMENT,"Matrix must be equally or more aligned than vector!");

//! Equivalent of GLSL's mat4x3
struct matrix3x4SIMD// : private AllocationOverrideBase<_IRR_MATRIX_ALIGNMENT> EBO inheritance problem w.r.t `rows[3]`
{
	_IRR_STATIC_INLINE_CONSTEXPR uint32_t VectorCount = 3u;
	core::vectorSIMDf rows[VectorCount];

	explicit matrix3x4SIMD(	const vectorSIMDf& _r0 = vectorSIMDf(1.f, 0.f, 0.f, 0.f),
							const vectorSIMDf& _r1 = vectorSIMDf(0.f, 1.f, 0.f, 0.f),
							const vectorSIMDf& _r2 = vectorSIMDf(0.f, 0.f, 1.f, 0.f)) : rows{_r0, _r1, _r2}
	{
	}

	matrix3x4SIMD(	float _a00, float _a01, float _a02, float _a03,
					float _a10, float _a11, float _a12, float _a13,
					float _a20, float _a21, float _a22, float _a23)
							: matrix3x4SIMD(vectorSIMDf(_a00, _a01, _a02, _a03),
											vectorSIMDf(_a10, _a11, _a12, _a13),
											vectorSIMDf(_a20, _a21, _a22, _a23))
	{
	}

	explicit matrix3x4SIMD(const float* const _data)
	{
		if (!_data)
			return;
		for (size_t i = 0u; i < VectorCount; ++i)
			rows[i] = vectorSIMDf(_data + 4*i);
	}
	matrix3x4SIMD(const float* const _data, bool ALIGNED)
	{
		if (!_data)
			return;
		for (size_t i = 0u; i < VectorCount; ++i)
			rows[i] = vectorSIMDf(_data + 4*i, ALIGNED);
	}

	inline matrix3x4SIMD& set(const matrix4x3& _retarded);
	inline matrix4x3 getAsRetardedIrrlichtMatrix() const;

	static inline matrix3x4SIMD concatenateBFollowedByA(const matrix3x4SIMD& _a, const matrix3x4SIMD& _b);

	static inline matrix3x4SIMD concatenateBFollowedByAPrecisely(const matrix3x4SIMD& _a, const matrix3x4SIMD& _b);

	inline matrix3x4SIMD& concatenateAfter(const matrix3x4SIMD& _other)
	{
		return *this = concatenateBFollowedByA(*this, _other);
	}

	inline matrix3x4SIMD& concatenateBefore(const matrix3x4SIMD& _other)
	{
		return *this = concatenateBFollowedByA(_other, *this);
	}

	inline matrix3x4SIMD& concatenateAfterPrecisely(const matrix3x4SIMD& _other)
	{
		return *this = concatenateBFollowedByAPrecisely(*this, _other);
	}

	inline matrix3x4SIMD& concatenateBeforePrecisely(const matrix3x4SIMD& _other)
	{
		return *this = concatenateBFollowedByAPrecisely(_other, *this);
	}

	inline bool operator==(const matrix3x4SIMD& _other)
	{
		return !(*this != _other);
	}

	inline bool operator!=(const matrix3x4SIMD& _other);

	inline matrix3x4SIMD& operator+=(const matrix3x4SIMD& _other);
	inline matrix3x4SIMD operator+(const matrix3x4SIMD& _other) const
	{
		matrix3x4SIMD retval(*this);
		return retval += _other;
	}

	inline matrix3x4SIMD& operator-=(const matrix3x4SIMD& _other);
	inline matrix3x4SIMD operator-(const matrix3x4SIMD& _other) const
	{
		matrix3x4SIMD retval(*this);
		return retval -= _other;
	}

	inline matrix3x4SIMD& operator*=(float _scalar);
	inline matrix3x4SIMD operator*(float _scalar) const
	{
		matrix3x4SIMD retval(*this);
		return retval *= _scalar;
	}

	inline matrix3x4SIMD& setTranslation(const core::vectorSIMDf& _translation)
	{
	    // no faster way of doing it?
		rows[0].w = _translation.x;
		rows[1].w = _translation.y;
		rows[2].w = _translation.z;
		return *this;
	}
	inline vectorSIMDf getTranslation() const;
	inline vectorSIMDf getTranslation3D() const;

	inline matrix3x4SIMD& setScale(const core::vectorSIMDf& _scale);

	inline core::vectorSIMDf getScale() const;

	inline void transformVect(vectorSIMDf& _out, const vectorSIMDf& _in) const;
	inline void transformVect(vectorSIMDf& _in_out) const
	{
		transformVect(_in_out, _in_out);
	}

	inline void pseudoMulWith4x1(vectorSIMDf& _out, const vectorSIMDf& _in) const;
	inline void pseudoMulWith4x1(vectorSIMDf& _in_out) const
	{
		pseudoMulWith4x1(_in_out,_in_out);
	}

	inline void mulSub3x3WithNx1(vectorSIMDf& _out, const vectorSIMDf& _in) const;
	inline void mulSub3x3WithNx1(vectorSIMDf& _in_out) const
	{
		mulSub3x3WithNx1(_in_out, _in_out);
	}

	inline static matrix3x4SIMD buildCameraLookAtMatrixLH(
		const core::vectorSIMDf& position,
		const core::vectorSIMDf& target,
		const core::vectorSIMDf& upVector);
	inline static matrix3x4SIMD buildCameraLookAtMatrixRH(
		const core::vectorSIMDf& position,
		const core::vectorSIMDf& target,
		const core::vectorSIMDf& upVector);

	inline matrix3x4SIMD& setRotation(const core::quaternion& _quat);

	inline matrix3x4SIMD& setScaleRotationAndTranslation(	const vectorSIMDf& _scale,
															const core::quaternion& _quat,
															const vectorSIMDf& _translation);

	inline bool getInverse(matrix3x4SIMD& _out) const;
	bool makeInverse()
	{
		matrix3x4SIMD tmp;

		if (getInverse(tmp))
		{
			*this = tmp;
			return true;
		}
		return false;
	}

	//
	inline bool getSub3x3InverseTransposePaddedSIMDColumns(core::vectorSIMDf _out[3]) const;

	//
	inline void setTransformationCenter(const core::vectorSIMDf& _center, const core::vectorSIMDf& _translation);

	//
	static inline matrix3x4SIMD buildAxisAlignedBillboard(
		const core::vectorSIMDf& camPos,
		const core::vectorSIMDf& center,
		const core::vectorSIMDf& translation,
		const core::vectorSIMDf& axis,
		const core::vectorSIMDf& from);


	//
	float& operator()(size_t _i, size_t _j) { return rows[_i].pointer[_j]; }
	const float& operator()(size_t _i, size_t _j) const { return rows[_i].pointer[_j]; }

	//
    inline const vectorSIMDf& operator[](size_t _rown) const { return rows[_rown]; }
    inline vectorSIMDf& operator[](size_t _rown) { return rows[_rown]; }

private:
	static inline vectorSIMDf doJob(const __m128& a, const matrix3x4SIMD& _mtx);

	// really need that dvec<2> or wider
	inline __m128d halfRowAsDouble(size_t _n, bool _0) const;
	static inline __m128d doJob_d(const __m128d& _a0, const __m128d& _a1, const matrix3x4SIMD& _mtx, bool _xyHalf);
};

inline matrix3x4SIMD concatenateBFollowedByA(const matrix3x4SIMD& _a, const matrix3x4SIMD& _b)
{
    return matrix3x4SIMD::concatenateBFollowedByA(_a, _b);
}
/*
inline matrix3x4SIMD concatenateBFollowedByAPrecisely(const matrix3x4SIMD& _a, const matrix3x4SIMD& _b)
{
    return matrix3x4SIMD::concatenateBFollowedByAPrecisely(_a, _b);
}
*/

inline aabbox3df transformBoxEx(const aabbox3df& box, const matrix3x4SIMD& _mat)
{
    vectorSIMDf inMinPt(&box.MinEdge.X);
    vectorSIMDf inMaxPt(box.MaxEdge.X,box.MaxEdge.Y,box.MaxEdge.Z); // TODO: after change to SSE aabbox3df
    inMinPt.makeSafe3D();
    inMaxPt.makeSafe3D();

    vectorSIMDf c0 = _mat.rows[0], c1 = _mat.rows[1], c2 = _mat.rows[2], c3 = vectorSIMDf(0.f, 0.f, 0.f, 1.f);
    transpose4(c0, c1, c2, c3);

    const vectorSIMDf zero;
    vectorSIMDf minPt = c0*mix(inMinPt.xxxw(),inMaxPt.xxxw(),c0<zero)+c1*mix(inMinPt.yyyw(),inMaxPt.yyyw(),c1<zero)+c2*mix(inMinPt.zzzw(),inMaxPt.zzzw(),c2<zero);
    vectorSIMDf maxPt = c0*mix(inMaxPt.xxxw(),inMinPt.xxxw(),c0<zero)+c1*mix(inMaxPt.yyyw(),inMinPt.yyyw(),c1<zero)+c2*mix(inMaxPt.zzzw(),inMinPt.zzzw(),c2<zero);

	minPt += c3;
	maxPt += c3;

	return aabbox3df(minPt.getAsVector3df(),maxPt.getAsVector3df());
}

}}

#endif
