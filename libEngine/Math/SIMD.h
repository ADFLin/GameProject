#pragma once
#ifndef SIMD_H_A48E9490_F46C_48AF_8AB6_98AE7574890B
#define SIMD_H_A48E9490_F46C_48AF_8AB6_98AE7574890B

#include "xmmintrin.h"
#include "smmintrin.h"

namespace SIMD
{
	struct alignas(16) SBase
	{
		SBase() {}
		SBase(float x, float y, float z, float w)
			:x(x),y(y),z(z),w(w){}

		FORCEINLINE SBase operator + (SBase const& rhs) const { return _mm_add_ps(reg, rhs.reg); }
		FORCEINLINE SBase operator - (SBase const& rhs) const { return _mm_sub_ps(reg, rhs.reg); }

		SBase(__m128 val) :reg(val) {}

		operator __m128 () const { return reg; }
		union
		{
			struct
			{
				float x, y, z, w;
				
			};

			struct
			{
				float r, i , t0 , t1;
			};

			float v[4];
			__m128 reg;
		};
	};

	struct SVectorBase : SBase
	{
		SVectorBase() {}
		SVectorBase(float x, float y, float z, float w)
			:SBase(x,y,z,w){}

		SVectorBase operator * (SVectorBase const& rhs) const { return _mm_mul_ps(reg, rhs.reg); }
		SVectorBase operator / (SVectorBase const& rhs) const { return _mm_div_ps(reg, rhs.reg); }

		SVectorBase(__m128 val) :SBase(val) {}
	};

	struct SVector4 : SVectorBase
	{
	public:
		SVector4() {}
		SVector4(float x, float y, float z, float w)
			:SVectorBase(x,y,z,w){ }
		FORCEINLINE float dot(SVector4 const& rhs) const
		{
			return _mm_cvtss_f32(_mm_dp_ps(*this, rhs, 0xf1) );
		}
		FORCEINLINE float length(SVector4 const& rhs) const
		{
			return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(*this, rhs, 0xf1)));
		}
		FORCEINLINE float lengthInv(SVector4 const& rhs) const
		{
			return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_dp_ps(*this, rhs, 0xf1)));
		}

		SVector4( SBase const& rhs ):SVectorBase( rhs ){}
	};

	struct SVector3 : SVectorBase
	{
	public:
		SVector3() {}
		SVector3(float x, float y, float z)
			:SVectorBase(x, y, z, 0)
		{
		}
		FORCEINLINE float dot(SVector3 const& rhs) const
		{
			return _mm_cvtss_f32(_mm_dp_ps(reg, rhs.reg, 0x71));
		}
		FORCEINLINE float length() const
		{
			return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(reg, reg, 0x71)));
		}
		FORCEINLINE float lengthInv() const
		{
			return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_dp_ps(reg, reg, 0x71)));
		}
		FORCEINLINE SVector3 cross(SVector3 const& rhs) const
		{
			return _mm_sub_ps(
				_mm_mul_ps(_mm_shuffle_ps( reg , reg, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(rhs.reg, rhs.reg, _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_ps(_mm_shuffle_ps( reg , reg, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(rhs.reg, rhs.reg, _MM_SHUFFLE(3, 0, 2, 1)))
			);
		}
		SVector3(SBase const& rhs) :SVectorBase(rhs) {}
	};

	struct SCompolex : SBase
	{
		SCompolex() {}
		SCompolex(float r, float i)
			:SBase(r, i, 0, 0)
		{
		}

		FORCEINLINE float length() const
		{
			return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(reg, reg, 0x31)));
		}
		FORCEINLINE float lengthInv() const
		{
			return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_dp_ps(reg, reg, 0x31)));
		}
		FORCEINLINE SCompolex operator * (SCompolex const& rhs) const
		{
			__m128 aa = _mm_moveldup_ps(reg);
			__m128 bb = _mm_movehdup_ps(reg);
			__m128 x0 = _mm_mul_ps(aa, rhs.reg);   //( ac , ad )
			__m128 dc = _mm_shuffle_ps(rhs.reg, rhs.reg, _MM_SHUFFLE(2, 3, 0, 1));
			__m128 x1 = _mm_mul_ps(bb, dc);    //( bd , bc )
			return _mm_addsub_ps(x0, x1);
		}
		FORCEINLINE SCompolex operator / (SCompolex const& rhs ) const
		{
			__m128 c = { 1 , -1 , 0 , 0 };
			__m128 rhsC = _mm_mul_ps(rhs.reg, c);
			__m128 aa = _mm_moveldup_ps(reg);
			__m128 bb = _mm_movehdup_ps(reg);
			__m128 x0 = _mm_mul_ps(aa, rhsC); //( ac , -ad )
			__m128 dc = _mm_shuffle_ps(rhsC, rhsC, _MM_SHUFFLE(2, 3, 0, 1)); //( -d , c )
			__m128 x1 = _mm_mul_ps(bb, dc);    //( -bd , bc )
			return _mm_div_ps(_mm_addsub_ps(x0, x1), _mm_dp_ps(rhsC, rhsC, 0x33));
		}
		SCompolex(__m128 val) :SBase(val) {}
		SCompolex(SBase const& rhs) :SBase(rhs) {}
	};

	struct alignas(16) SMatrix4
	{
	public:
		SVector4 operator * (SVector4 const& rhs) const
		{


		}

		SVector4 Basis[4];
	};


	template< int ChannelNum >
	struct LaneScale
	{



	};

}//namespace SIMD

#endif // SIMD_H_A48E9490_F46C_48AF_8AB6_98AE7574890B