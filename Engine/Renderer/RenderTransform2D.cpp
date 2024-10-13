#include "RenderTransform2D.h"

namespace Render
{

	RenderTransform2D RenderTransform2D::inverse() const
	{
		//[ M  0 ] [ M(-1) 0 ]  =  [ M*M(-1)        0 ] = [ I  0 ]
		//[ P  1 ] [ P(-1) 1 ]     [ P*M(-1)+P(-1)  1 ]   [ 0  1 ]
		RenderTransform2D result;
		float det;
		M.inverse(result.M, det);
		result.P = -P * result.M;

		return result;
	}

	Vector2 RenderTransform2D::removeScale()
	{
		Vector2 xAxis = Vector2(M[0], M[1]);
		float xScale = xAxis.normalize();
		Vector2 yAxis = Vector2(M[2], M[3]);
		float yScale = yAxis.normalize();

		M = Matrix2(xAxis, yAxis);
		return Vector2(xScale, yScale);
	}

	void RenderTransform2D::removeRotation()
	{
		Vector2 scale = getScale();
		M = Matrix2::Scale(scale);
	}

	void RenderTransform2D::removeScaleAndRotation()
	{
		M = Matrix2::Identity();
	}

	Vector2 RenderTransform2D::MulOffset(Vector2 const& v, Matrix2 const& m, Vector2 const& offset)
	{
#if USE_MATH_SIMD
		__m128 rv = _mm_setr_ps(v.x, v.x, v.y, v.y);
		__m128 mv = _mm_loadu_ps(m);
		__m128 xv = _mm_dp_ps(rv, mv, 0x51);
		__m128 yv = _mm_dp_ps(rv, mv, 0xa2);
		__m128 resultV = _mm_add_ps(_mm_add_ps(xv, yv), _mm_setr_ps(offset.x, offset.y, 0, 0));
		return Vector2(resultV.m128_f32[0], resultV.m128_f32[1]);
#else
		return v * m + offset;
#endif
	}
}

