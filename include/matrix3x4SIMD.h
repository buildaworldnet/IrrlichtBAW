#ifndef __MATRIX3X4SIMD_H_INCLUDED__
#define __MATRIX3X4SIMD_H_INCLUDED__

#include <quaternion.h>

namespace irr { namespace core
{
#ifdef _IRR_WINDOWS_
__declspec(align(SIMD_ALIGNMENT))
#endif
//! Equivalent of GLSL's mat4x3
struct matrix3x4SIMD
{
	core::vectorSIMDf rows[3];

#define BUILD_MASKF(_x_, _y_, _z_, _w_) _mm_castsi128_ps(_mm_setr_epi32(_x_*0xffffffff, _y_*0xffffffff, _z_*0xffffffff, _w_*0xffffffff))
	explicit matrix3x4SIMD(const vectorSIMDf& _r0 = vectorSIMDf(1.f, 0.f, 0.f, 0.f), const vectorSIMDf& _r1 = vectorSIMDf(0.f, 1.f, 0.f, 0.f), const vectorSIMDf& _r2 = vectorSIMDf(0.f, 0.f, 1.f, 0.f)) 
		: rows{_r0, _r1, _r2} {}

	matrix3x4SIMD(
		float _a00, float _a01, float _a02, float _a03,
		float _a10, float _a11, float _a12, float _a13,
		float _a20, float _a21, float _a22, float _a23) 
	: matrix3x4SIMD(vectorSIMDf(_a00, _a01, _a02, _a03), vectorSIMDf(_a10, _a11, _a12, _a13), vectorSIMDf(_a20, _a21, _a22, _a23))
	{
	}

	explicit matrix3x4SIMD(const float* const _data)
	{
		if (!_data)
			return;
		for (size_t i = 0u; i < 3u; ++i)
			rows[i] = vectorSIMDf(_data + 4*i);
	}
	matrix3x4SIMD(const float* const _data, bool ALIGNED)
	{
		if (!_data)
			return;
		for (size_t i = 0u; i < 3u; ++i)
			rows[i] = vectorSIMDf(_data + 4*i, ALIGNED);
	}

	static inline matrix3x4SIMD concatenateBFollowedByA(const matrix3x4SIMD& _a, const matrix3x4SIMD& _b)
	{
		__m128 r0 = _a.rows[0].getAsRegister();
		__m128 r1 = _a.rows[1].getAsRegister();
		__m128 r2 = _a.rows[2].getAsRegister();

		matrix3x4SIMD out;
		_mm_store_ps(out.rows[0].pointer, matrix3x4SIMD::doJob(r0, _b));
		_mm_store_ps(out.rows[1].pointer, matrix3x4SIMD::doJob(r1, _b));
		_mm_store_ps(out.rows[2].pointer, matrix3x4SIMD::doJob(r2, _b));

		return out;
	}

	static inline matrix3x4SIMD concatenateBFollowedByAPrecisely(const matrix3x4SIMD& _a, const matrix3x4SIMD& _b)
	{
		__m128d r00 = _a.halfRowAsDouble(0u, true);
		__m128d r01 = _a.halfRowAsDouble(0u, false);
		__m128d r10 = _a.halfRowAsDouble(1u, true);
		__m128d r11 = _a.halfRowAsDouble(1u, false);
		__m128d r20 = _a.halfRowAsDouble(2u, true);
		__m128d r21 = _a.halfRowAsDouble(2u, false);

		matrix3x4SIMD out;

		const __m128 mask0011 = BUILD_MASKF(0, 0, 1, 1);

		__m128 second = _mm_cvtpd_ps(matrix3x4SIMD::doJob_d(r00, r01, _b, false));
		out.rows[0] = vectorSIMDf(_mm_cvtpd_ps(matrix3x4SIMD::doJob_d(r00, r01, _b, true))) | _mm_and_ps(_mm_movelh_ps(second, second), mask0011);

		second = _mm_cvtpd_ps(matrix3x4SIMD::doJob_d(r10, r11, _b, false));
		out.rows[1] = vectorSIMDf(_mm_cvtpd_ps(matrix3x4SIMD::doJob_d(r10, r11, _b, true))) | _mm_and_ps(_mm_movelh_ps(second, second), mask0011);

		second = _mm_cvtpd_ps(matrix3x4SIMD::doJob_d(r20, r21, _b, false));
		out.rows[2] = vectorSIMDf(_mm_cvtpd_ps(matrix3x4SIMD::doJob_d(r20, r21, _b, true))) | _mm_and_ps(_mm_movelh_ps(second, second), mask0011);

		return out;
	}

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
		for (size_t i = 0u; i < 3u; ++i)
			if (!((rows[i] == _other.rows[i]).all()))
				return false;
		return true;
	}

	inline bool operator!=(const matrix3x4SIMD& _other)
	{
		return !(*this == _other);
	}

	inline matrix3x4SIMD& operator+=(const matrix3x4SIMD& _other)
	{
		for (size_t i = 0u; i < 3u; ++i)
			rows[i] += _other.rows[i];
		return *this;
	}
	inline matrix3x4SIMD operator+(const matrix3x4SIMD& _other) const
	{
		matrix3x4SIMD retval(*this);
		return retval += _other;
	}

	inline matrix3x4SIMD& operator-=(const matrix3x4SIMD& _other)
	{
		for (size_t i = 0u; i < 3u; ++i)
			rows[i] -= _other.rows[i];
		return *this;
	}
	inline matrix3x4SIMD operator-(const matrix3x4SIMD& _other) const
	{
		matrix3x4SIMD retval(*this);
		return retval -= _other;
	}

	inline matrix3x4SIMD& operator*=(float _scalar)
	{
		for (size_t i = 0u; i < 3u; ++i)
			rows[i] *= _scalar;
		return *this;
	}
	inline matrix3x4SIMD operator*(float _scalar) const
	{
		matrix3x4SIMD retval(*this);
		return retval *= _scalar;
	}

	inline matrix3x4SIMD& setTranslation(const core::vectorSIMDf& _translation)
	{
		rows[0].w = _translation.x;
		rows[1].w = _translation.y;
		rows[2].w = _translation.z;
		return *this;
	}
	inline vectorSIMDf getTranslation() const
	{
		__m128 xmm0 = _mm_unpackhi_ps(rows[0].getAsRegister(), rows[1].getAsRegister()); // (0z,1z,0w,1w)
		__m128 xmm1 = _mm_unpackhi_ps(rows[2].getAsRegister(), _mm_setr_ps(0.f, 0.f, 0.f, 1.f)); // (2z,3z,2w,3w)
		__m128 xmm2 = _mm_movehl_ps(xmm1, xmm0);// (0w,1w,2w,3w)

		return xmm2;
	}
	inline vectorSIMDf getTranslation3D() const
	{
		__m128 xmm0 = _mm_unpackhi_ps(rows[0].getAsRegister(), rows[1].getAsRegister()); // (0z,1z,0w,1w)
		__m128 xmm1 = _mm_unpackhi_ps(rows[2].getAsRegister(), _mm_setzero_ps()); // (2z,0,2w,0)
		__m128 xmm2 = _mm_movehl_ps(xmm1, xmm0);// (0w,1w,2w,0)

		return xmm2;
	}

	inline matrix3x4SIMD& setScale(const core::vectorSIMDf& _scale)
	{
		const vectorSIMDf mask0001 = BUILD_MASKF(0, 0, 0, 1);

		rows[0] = (_scale & BUILD_MASKF(1, 0, 0, 0)) | (rows[0] & mask0001);
		rows[1] = (_scale & BUILD_MASKF(0, 1, 0, 0)) | (rows[1] & mask0001);
		rows[2] = (_scale & BUILD_MASKF(0, 0, 1, 0)) | (rows[2] & mask0001);

		return *this;
	}

	inline core::vectorSIMDf getScale() const
	{
		// xmm4-7 will now become columuns of B
		__m128 xmm4 = rows[0].getAsRegister();
		__m128 xmm5 = rows[1].getAsRegister();
		__m128 xmm6 = rows[2].getAsRegister();
		__m128 xmm7 = _mm_setzero_ps();
		// g==0
		__m128 xmm0 = _mm_unpacklo_ps(xmm4, xmm5);
		__m128 xmm1 = _mm_unpacklo_ps(xmm6, xmm7); // (2x,g,2y,g)
		__m128 xmm2 = _mm_unpackhi_ps(xmm4, xmm5);
		__m128 xmm3 = _mm_unpackhi_ps(xmm6, xmm7); // (2z,g,2w,g)
		xmm4 = _mm_movelh_ps(xmm1, xmm0); //(0x,1x,2x,g)
		xmm5 = _mm_movehl_ps(xmm1, xmm0);
		xmm6 = _mm_movelh_ps(xmm3, xmm2); //(0z,1z,2z,g)

		// See http://www.robertblum.com/articles/2005/02/14/decomposing-matrices
		// We have to do the full calculation.
		xmm0 = _mm_mul_ps(xmm4, xmm4);// column 0 squared
		xmm1 = _mm_mul_ps(xmm5, xmm5);// column 1 squared
		xmm2 = _mm_mul_ps(xmm6, xmm6);// column 2 squared
		xmm4 = _mm_hadd_ps(xmm0, xmm1);
		xmm5 = _mm_hadd_ps(xmm2, xmm7);
		xmm6 = _mm_hadd_ps(xmm4, xmm5);

		return _mm_sqrt_ps(xmm6);
	}

	inline void transformVect(float* _out, const float* _in) const
	{
		vectorSIMDf vec(_in);
		vectorSIMDf r0 = rows[0] * vec, 
			r1 = rows[1] * vec, 
			r2 = rows[2] * vec, 
			r3;

		float res[4];
		_mm_storeu_ps(res,
		_mm_hadd_ps(
			_mm_hadd_ps(r0.getAsRegister(), r1.getAsRegister()),
			_mm_hadd_ps(r2.getAsRegister(), r3.getAsRegister())
		));

		memcpy(_out, res, 3 * 4);
	}
	inline void transformVect(float* _in_out) const
	{
		float out[3];
		transformVect(out, _in_out);
		memcpy(_in_out, out, 3*4);
	}

	inline void pseudoMulWith4x1(float* _out, const float* _in) const
	{
		transformVect(_out, _in);
	}
	inline void pseudoMulWith4x1(float* _in_out) const
	{
		transformVect(_in_out);
	}

	inline void mulSub3x3With3x1(float* _out, const float* _in) const
	{
		vectorSIMDf mask1110 = BUILD_MASKF(1, 1, 1, 0);
		vectorSIMDf vec(_in);
		vectorSIMDf r0 = (rows[0] * vec) & mask1110,
			r1 = (rows[1] * vec) & mask1110,
			r2 = (rows[2] * vec) & mask1110,
			r3 = _mm_setzero_ps();

		float res[4];
		_mm_storeu_ps(res,
			_mm_hadd_ps(
				_mm_hadd_ps(r0.getAsRegister(), r1.getAsRegister()),
				_mm_hadd_ps(r2.getAsRegister(), r3.getAsRegister())
		));

		memcpy(_out, res, 3 * 4);
	}
	inline void mulSub3x3With3x1(float* _in_out) const
	{
		mulSub3x3With3x1(_in_out, _in_out);
	}

	inline matrix3x4SIMD& buildCameraLookAtMatrixLH(
		const core::vectorSIMDf& position,
		const core::vectorSIMDf& target,
		const core::vectorSIMDf& upVector)
	{
		const core::vectorSIMDf zaxis = core::normalize(target - position);
		const core::vectorSIMDf xaxis = core::normalize(upVector.crossProduct(zaxis));
		const core::vectorSIMDf yaxis = zaxis.crossProduct(xaxis);

		matrix3x4SIMD r;
		r.rows[0] = xaxis;
		r.rows[1] = yaxis;
		r.rows[2] = zaxis;
		r.rows[0].w = -xaxis.dotProductAsFloat(position);
		r.rows[1].w = -yaxis.dotProductAsFloat(position);
		r.rows[2].w = -zaxis.dotProductAsFloat(position);

		return r;
	}
	inline matrix3x4SIMD& buildCameraLookAtMatrixRH(
		const core::vectorSIMDf& position,
		const core::vectorSIMDf& target,
		const core::vectorSIMDf& upVector)
	{
		const core::vectorSIMDf zaxis = core::normalize(position - target);
		const core::vectorSIMDf xaxis = core::normalize(upVector.crossProduct(zaxis));
		const core::vectorSIMDf yaxis = zaxis.crossProduct(xaxis);

		matrix3x4SIMD r;
		r.rows[0] = xaxis;
		r.rows[1] = yaxis;
		r.rows[2] = zaxis;
		r.rows[0].w = -xaxis.dotProductAsFloat(position);
		r.rows[1].w = -yaxis.dotProductAsFloat(position);
		r.rows[2].w = -zaxis.dotProductAsFloat(position);

		return r;
	}

	static inline matrix3x4SIMD mix(const matrix3x4SIMD& _a, const matrix3x4SIMD& _b, const float& _x)
	{
		matrix3x4SIMD mat;

		mat.rows[0] = core::mix(_a.rows[0], _b.rows[0], vectorSIMDf(_x));
		mat.rows[1] = core::mix(_a.rows[1], _b.rows[1], vectorSIMDf(_x));
		mat.rows[2] = core::mix(_a.rows[2], _b.rows[2], vectorSIMDf(_x));

		return mat;
	}

	inline matrix3x4SIMD& setRotation(const core::quaternion& _quat)
	{
		const __m128 mask0001 = _mm_castsi128_ps(_mm_setr_epi32(0, 0, 0, 0xffffffff));
		const __m128 mask1110 = _mm_castsi128_ps(_mm_setr_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0));

		const core::vectorSIMDf& quat = reinterpret_cast<const core::vectorSIMDf&>(_quat);
		rows[0] = ((quat.yyyy() * ((quat.yxwx() & mask1110) * vectorSIMDf(2.f))) + (quat.zzzz() * (quat.zwxx() & mask1110) * vectorSIMDf(2.f, -2.f, 2.f, 0.f))) | (rows[0] & mask0001);
		rows[0].x = 1.f - rows[0].x;

		rows[1] = ((quat.zzzz() * ((quat.wzyx() & mask1110) * vectorSIMDf(2.f))) + (quat.xxxx() * (quat.yxwx() & mask1110) * vectorSIMDf(2.f, 2.f, -2.f, 0.f))) | (rows[1] & mask0001);
		rows[1].y = 1.f - rows[1].y;

		rows[2] = ((quat.xxxx() * ((quat.zwxx() & mask1110) * vectorSIMDf(2.f))) + (quat.yyyy() * (quat.wzyx() & mask1110) * vectorSIMDf(-2.f, 2.f, 2.f, 0.f))) | (rows[2] & mask0001);
		rows[2].z = 1.f - rows[2].z;

		return *this;
	}

	inline bool getInverse(matrix3x4SIMD& _out) const
	{
		vectorSIMDf c0 = rows[0], c1 = rows[1], c2 = rows[2], c3 = vectorSIMDf(0.f, 0.f, 0.f, 1.f);
		core::transpose4(c0, c1, c2, c3);

		const vectorSIMDf c1crossc2 = c1.crossProduct(c2);

		const vectorSIMDf d = c0.dotProduct(c1crossc2);

		if (core::iszero(d.x, FLT_MIN))
			return false;

		_out.rows[0] = c1crossc2 / d;
		_out.rows[1] = (c2.crossProduct(c0)) / d;
		_out.rows[2] = (c0.crossProduct(c1)) / d;

		vectorSIMDf outC3 = vectorSIMDf(0.f, 0.f, 0.f, 1.f);
		core::transpose4(_out.rows[0], _out.rows[1], _out.rows[2], outC3);
		mulSub3x3With3x1(outC3.pointer, c3.pointer);
		outC3 = -outC3;
		core::transpose4(_out.rows[0], _out.rows[1], _out.rows[2], outC3);

		return true;
	}
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

	inline bool getSub3x3Inverse(float* _out) const
	{
		vectorSIMDf c0 = rows[0], c1 = rows[1], c2 = rows[2], c3 = vectorSIMDf(0.f, 0.f, 0.f, 1.f);
		core::transpose4(c0, c1, c2, c3);

		const vectorSIMDf c1crossc2 = c1.crossProduct(c2);

		const vectorSIMDf d = c0.dotProduct(c1crossc2);

		if (core::iszero(d.x, FLT_MIN))
			return false;

		vectorSIMDf tmp = c1crossc2 / d;
		memcpy(_out, tmp.pointer, 3*4);
		tmp = (c2.crossProduct(c0)) / d;
		memcpy(_out+3, tmp.pointer, 3*4);
		tmp = (c0.crossProduct(c1)) / d;
		memcpy(_out+6, tmp.pointer, 3*4);

		return true;
	}

	inline void setRotationCenter(const core::vectorSIMDf& _center, const core::vectorSIMDf& _translation)
	{
		core::vectorSIMDf r0 = rows[0] * _center;
		core::vectorSIMDf r1 = rows[1] * _center;
		core::vectorSIMDf r2 = rows[2] * _center;
		core::vectorSIMDf r3(0.f, 0.f, 0.f, 1.f);

		__m128 col3 = _mm_hadd_ps(_mm_hadd_ps(r0.getAsRegister(), r1.getAsRegister()), _mm_hadd_ps(r2.getAsRegister(), r3.getAsRegister()));
		const vectorSIMDf vcol3 = _center - _translation - col3;

		for (size_t i = 0u; i < 3u; ++i)
			rows[i].w = vcol3.pointer[i];
	}

	inline void buildAxisAlignedBillboard(
		const core::vectorSIMDf& camPos,
		const core::vectorSIMDf& center,
		const core::vectorSIMDf& translation,
		const core::vectorSIMDf& axis,
		const core::vectorSIMDf& from)
	{
		// axis of rotation
		const core::vectorSIMDf up = core::normalize(axis);
		const core::vectorSIMDf forward = core::normalize(camPos - center);
		const core::vectorSIMDf right = core::normalize(up.crossProduct(forward));

		// correct look vector
		const core::vectorSIMDf look = right.crossProduct(up);

		// rotate from to
		// axis multiplication by sin
		const core::vectorSIMDf vs = look.crossProduct(from);

		// cosinus angle
		const core::vectorSIMDf ca = from.dotProduct(look);

		const core::vectorSIMDf vt(up * (core::vectorSIMDf(1.f) - ca));
		const core::vectorSIMDf wt = vt * up.yzxx();
		const core::vectorSIMDf vtuppca = vt * up + ca;

		core::vectorSIMDf& row0 = rows[0];
		core::vectorSIMDf& row1 = rows[1];
		core::vectorSIMDf& row2 = rows[2];

		row0 = vtuppca & BUILD_MASKF(1, 0, 0, 0);
		row1 = vtuppca & BUILD_MASKF(0, 1, 0, 0);
		row2 = vtuppca & BUILD_MASKF(0, 0, 1, 0);

		row0 += (wt.xxzx() + vs.xzyx()*core::vectorSIMDf(1.f, 1.f, -1.f, 1.f)) & BUILD_MASKF(0, 1, 1, 0);
		row1 += (wt.xxyx() + vs.zxxx()*core::vectorSIMDf(-1.f, 1.f, 1.f, 1.f)) & BUILD_MASKF(1, 0, 1, 0);
		row2 += (wt.zyxx() + vs.yxxx()*core::vectorSIMDf(1.f, -1.f, 1.f, 1.f)) & BUILD_MASKF(1, 1, 0, 0);

		setRotationCenter(center, translation);
	}

	//! @bug Something's wrong with row[2].x
	inline matrix3x4SIMD& buildRotateFromTo(const core::vectorSIMDf& from, const core::vectorSIMDf& to)
	{
		// unit vectors
		const core::vectorSIMDf f = core::normalize(from);
		const core::vectorSIMDf t = core::normalize(to);

		// axis multiplication by sin
		const core::vectorSIMDf vs(t.crossProduct(f));

		// axis of rotation
		const core::vectorSIMDf v = core::normalize(vs);

		// cosinus angle
		const core::vectorSIMDf ca = f.dotProduct(t);

		const core::vectorSIMDf vt(v * (core::vectorSIMDf(1.f) - ca));
		const core::vectorSIMDf wt = vt * v.yzxx();
		const core::vectorSIMDf vtuppca = vt * v + ca;

		core::vectorSIMDf& row0 = rows[0];
		core::vectorSIMDf& row1 = rows[1];
		core::vectorSIMDf& row2 = rows[2];

		const core::vectorSIMDf mask0001 = BUILD_MASKF(0, 0, 0, 1);
		row0 = (row0 & mask0001) + (vtuppca & BUILD_MASKF(1, 0, 0, 0));
		row1 = (row1 & mask0001) + (vtuppca & BUILD_MASKF(0, 1, 0, 0));
		row2 = (row2 & mask0001) + (vtuppca & BUILD_MASKF(0, 0, 1, 0));

		row0 += (wt.xxzx() + vs.xzyx()*core::vectorSIMDf(1.f, 1.f, -1.f, 1.f)) & BUILD_MASKF(0, 1, 1, 0);
		row1 += (wt.xxyx() + vs.zxxx()*core::vectorSIMDf(-1.f, 1.f, 1.f, 1.f)) & BUILD_MASKF(1, 0, 1, 0);
		row2 += (wt.zyxx() + vs.yxxx()*core::vectorSIMDf(1.f, -1.f, 1.f, 1.f)) & BUILD_MASKF(1, 1, 0, 0);

		return *this;
	}
#undef BUILD_MASKF
	float& operator()(size_t _i, size_t _j) { return rows[_i].pointer[_j]; }
	const float& operator()(size_t _i, size_t _j) const { return rows[_i].pointer[_j]; }

private:
#define BROADCAST32(fpx) _MM_SHUFFLE(fpx, fpx, fpx, fpx)
	static inline __m128 doJob(const __m128& a, const matrix3x4SIMD& _mtx)
	{
		__m128 r0 = _mtx.rows[0].getAsRegister();
		__m128 r1 = _mtx.rows[1].getAsRegister();
		__m128 r2 = _mtx.rows[2].getAsRegister();

		const __m128 mask = _mm_castsi128_ps(_mm_setr_epi32(0, 0, 0, 0xffffffff));

		__m128 res;
		res = _mm_mul_ps(_mm_shuffle_ps(a, a, BROADCAST32(0)), r0);
		res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(a, a, BROADCAST32(1)), r1));
		res = _mm_add_ps(res, _mm_mul_ps(_mm_shuffle_ps(a, a, BROADCAST32(2)), r2));
		res = _mm_add_ps(res, _mm_and_ps(a, mask)); // always 0 0 0 a3 -- no shuffle needed
		return res;
	}

	inline __m128d halfRowAsDouble(size_t _n, bool _0) const
	{
		return _mm_cvtps_pd(_0 ? rows[_n].xyxx().getAsRegister() : rows[_n].zwxx().getAsRegister());
	}
	static inline __m128d doJob_d(const __m128d& _a0, const __m128d& _a1, const matrix3x4SIMD& _mtx, bool _xyHalf)
	{
		__m128d r0 = _mtx.halfRowAsDouble(0u, _xyHalf);
		__m128d r1 = _mtx.halfRowAsDouble(1u, _xyHalf);
		__m128d r2 = _mtx.halfRowAsDouble(2u, _xyHalf);

		const __m128d mask01 = _mm_castsi128_pd(_mm_setr_epi32(0, 0, 0xffffffff, 0xffffffff));

		__m128d res;
		res = _mm_mul_pd(_mm_shuffle_pd(_a0, _a0, 0), r0);
		res = _mm_add_pd(res, _mm_mul_pd(_mm_shuffle_pd(_a0, _a0, 3), r1));
		res = _mm_add_pd(res, _mm_mul_pd(_mm_shuffle_pd(_a1, _a1, 0), r2));
		if (!_xyHalf)
			res = _mm_add_pd(res, _mm_and_pd(_a1, mask01));
		return res;
	}
#undef BROADCAST32
}
#ifndef _IRR_WINDOWS_
__attribute__((__aligned__(SIMD_ALIGNMENT)));
#endif
;

inline void transformPlane(const plane3df& _in, plane3df& _out, const matrix3x4SIMD& _mat)
{
	matrix3x4SIMD inv;
	_mat.getInverse(inv);

	vectorSIMDf& c0 = inv.rows[0], &c1 = inv.rows[1], &c2 = inv.rows[2];
	vectorSIMDf c3(0.f, 0.f, 0.f, 1.f);
	core::transpose4(c0, c1, c2, c3);

	const vectorSIMDf normal(_in.Normal.X, _in.Normal.Y, _in.Normal.Z, 0.f);

	_out.Normal.X = normal.dotProductAsFloat(c0);
	_out.Normal.Y = normal.dotProductAsFloat(c1);
	_out.Normal.Z = normal.dotProductAsFloat(c2);
	_out.D = normal.dotProductAsFloat(c3) + _in.D;
}

inline void transformBoxEx(aabbox3df& box, const matrix3x4SIMD& _mat)
{
	float min[4]{ 0.f, 0.f, 0.f, 1.f }, max[4]{ 0.f, 0.f, 0.f, 1.f };
	memcpy(min, &box.MinEdge.X, 3 * 4);
	memcpy(max, &box.MaxEdge.X, 3 * 4);

	_mat.transformVect(min);
	_mat.transformVect(max);

	memcpy(&box.MinEdge.X, min, 3 * 4);
	memcpy(&box.MaxEdge.Y, max, 3 * 4);
}

}}

#endif