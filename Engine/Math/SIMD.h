#pragma once
#ifndef SIMD_H_A48E9490_F46C_48AF_8AB6_98AE7574890B
#define SIMD_H_A48E9490_F46C_48AF_8AB6_98AE7574890B

#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#define USE_MATH_SIMD 1
#else
#define USE_MATH_SIMD 0
#endif

#if USE_MATH_SIMD
#include <xmmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>


#include "MathFun/avx_mathfun.h"
#include "DirectXMath.h"

enum class EAligned { Value, };

namespace SIMD
{
	struct alignas(16) SBase
	{
		SBase() = default;
		FORCEINLINE SBase(float x, float y, float z, float w)
		{
			reg = _mm_set_ps(w, z, y, x);
		}
		FORCEINLINE SBase(float const* v)
		{
			reg = _mm_loadu_ps(v);
		}

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
				float r, i, t0, t1;
			};

			float v[4];
			__m128 reg;
		};
	};

	struct SScalar : SBase
	{
		SScalar() = default;
		SScalar(float value) { reg = _mm_set_ss(value); }

		SScalar(__m128 val) : SBase(val) {}

		FORCEINLINE SScalar operator * (SScalar const& rhs) const { return _mm_mul_ps(reg, rhs.reg); }
		FORCEINLINE SScalar operator / (SScalar const& rhs) const { return _mm_div_ps(reg, rhs.reg); }
		FORCEINLINE SScalar operator + (SScalar const& rhs) const { return _mm_add_ps(reg, rhs.reg); }
		FORCEINLINE SScalar operator - (SScalar const& rhs) const { return _mm_sub_ps(reg, rhs.reg); }

		operator float() const { return _mm_cvtss_f32(reg); }
	};

	struct SVectorBase : SBase
	{
		SVectorBase() = default;
		FORCEINLINE SVectorBase(float x, float y, float z, float w)
			:SBase(x, y, z, w) {}

		FORCEINLINE SVectorBase(float x)
		{
			reg = _mm_set_ps1(x);
		}
		FORCEINLINE SVectorBase operator * (SVectorBase const& rhs) const { return _mm_mul_ps(reg, rhs.reg); }
		FORCEINLINE SVectorBase operator / (SVectorBase const& rhs) const { return _mm_div_ps(reg, rhs.reg); }
		FORCEINLINE SVectorBase operator + (SVectorBase const& rhs) const { return _mm_add_ps(reg, rhs.reg); }
		FORCEINLINE SVectorBase operator - (SVectorBase const& rhs) const { return _mm_sub_ps(reg, rhs.reg); }

		FORCEINLINE SVectorBase(__m128 val) :SBase(val) {}
	};

	struct SVector4 : SVectorBase
	{
	public:
		SVector4() {}
		SVector4(float x, float y, float z, float w)
			:SVectorBase(x, y, z, w) { }
		SVector4(float const* v)
		{
			reg = _mm_loadu_ps(v);
		}


		FORCEINLINE SScalar dot(SVector4 const& rhs) const
		{
			return _mm_dp_ps(*this, rhs, 0xf1);
		}
		FORCEINLINE SScalar length(SVector4 const& rhs) const
		{
			return _mm_sqrt_ss(_mm_dp_ps(*this, rhs, 0xf1));
		}
		FORCEINLINE SScalar lengthInv(SVector4 const& rhs) const
		{
			return _mm_rsqrt_ss(_mm_dp_ps(*this, rhs, 0xf1));
		}

		SVector4(SBase const& rhs) :SVectorBase(rhs) {}
	};

	struct SVector3 : SVectorBase
	{
	public:
		SVector3() {}
		SVector3(float x, float y, float z)
			:SVectorBase(x, y, z, 0)
		{
		}
		SVector3(float const* v)
		{
			reg = _mm_loadu_ps(v);
		}
		FORCEINLINE SScalar dot(SVector3 const& rhs) const
		{
			return _mm_dp_ps(reg, rhs.reg, 0x71);
		}
		FORCEINLINE SScalar lengthSquare() const
		{
			return _mm_dp_ps(reg, reg, 0x71);
		}
		FORCEINLINE SScalar length() const
		{
			return _mm_sqrt_ss(_mm_dp_ps(reg, reg, 0x71));
		}
		FORCEINLINE SScalar lengthInv() const
		{
			return _mm_rsqrt_ss(_mm_dp_ps(reg, reg, 0x71));
		}
		FORCEINLINE SVector3 cross(SVector3 const& rhs) const
		{
			return _mm_sub_ps(
				_mm_mul_ps(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(rhs.reg, rhs.reg, _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_ps(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(rhs.reg, rhs.reg, _MM_SHUFFLE(3, 0, 2, 1)))
			);
		}

		FORCEINLINE SScalar normalize()
		{
			SScalar len = length();
			reg = _mm_div_ps(reg, _mm_set1_ps(len));
			return len;
		}

		SVector3(SBase const& rhs) :SVectorBase(rhs) {}
	};

	struct SVector2 : SVectorBase
	{
	public:
		SVector2() {}
		explicit SVector2(float x)
			:SVectorBase(x)
		{
		}
		SVector2(float x, float y)
			:SVectorBase(x, y, 0, 0)
		{
		}
		SVector2(float const* v)
		{
			reg = _mm_loadu_ps(v);
		}
		FORCEINLINE SScalar dot(SVector2 const& rhs) const
		{
			return _mm_dp_ps(reg, rhs.reg, 0x31);
		}
		FORCEINLINE SScalar lengthSquare() const
		{
			return _mm_dp_ps(reg, reg, 0x31);
		}
		FORCEINLINE SScalar length() const
		{
			return _mm_sqrt_ss(_mm_dp_ps(reg, reg, 0x31));
		}
		FORCEINLINE SScalar lengthInv() const
		{
			return _mm_rsqrt_ss(_mm_dp_ps(reg, reg, 0x31));
		}
#if 0
		FORCEINLINE SVector2 cross(SVector2 const& rhs) const
		{
			return _mm_sub_ps(
				_mm_mul_ps(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(rhs.reg, rhs.reg, _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_ps(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(rhs.reg, rhs.reg, _MM_SHUFFLE(3, 0, 2, 1)))
			);
		}
#endif
		SVector2(SBase const& rhs) :SVectorBase(rhs) {}
	};

	struct SCompolex : SBase
	{
		SCompolex() {}
		SCompolex(float r, float i)
			:SBase(r, i, 0, 0)
		{
		}
		FORCEINLINE float lengthSquare() const
		{
			return _mm_cvtss_f32(_mm_dp_ps(reg, reg, 0x31));
		}
		FORCEINLINE float length() const
		{
			return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(reg, reg, 0x31)));
		}
		FORCEINLINE float lengthInv() const
		{
			return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_dp_ps(reg, reg, 0x31)));
		}
		FORCEINLINE SCompolex operator + (SCompolex const& rhs) const { return _mm_add_ps(reg, rhs.reg); }
		FORCEINLINE SCompolex operator - (SCompolex const& rhs) const { return _mm_sub_ps(reg, rhs.reg); }
		FORCEINLINE SCompolex operator * (SCompolex const& rhs) const
		{
			__m128 aa = _mm_moveldup_ps(reg);
			__m128 bb = _mm_movehdup_ps(reg);
			__m128 x0 = _mm_mul_ps(aa, rhs.reg);   //( ac , ad )
			__m128 dc = _mm_shuffle_ps(rhs.reg, rhs.reg, _MM_SHUFFLE(2, 3, 0, 1));
			__m128 x1 = _mm_mul_ps(bb, dc);    //( bd , bc )
			return _mm_addsub_ps(x0, x1);
		}
		FORCEINLINE SCompolex operator / (SCompolex const& rhs) const
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

	template< int Size >
	struct TFloatVector {};

	template< int Size >
	struct TIntVector {};


	template<>
	struct alignas(32) TFloatVector<8>
	{
		TFloatVector() = default;

		operator __m256 () const { return reg; }

		FORCEINLINE TFloatVector(float x, float y, float z, float w, float p, float q, float r, float s)
		{
			reg = _mm256_set_ps(s, r, q, p, w, z, y, x);
		}

		FORCEINLINE TFloatVector(std::initializer_list<float> elements)
		{
			reg = _mm256_set_ps(
				elements.begin()[7], elements.begin()[6], elements.begin()[5], elements.begin()[4],
				elements.begin()[3], elements.begin()[2], elements.begin()[1], elements.begin()[0]);
		}
		FORCEINLINE TFloatVector(__m256 val) : reg(val) {}

		FORCEINLINE TFloatVector(float const* v) { reg = _mm256_loadu_ps(v); }
		FORCEINLINE TFloatVector(float const* v, EAligned) { reg = _mm256_load_ps(v); }
		FORCEINLINE TFloatVector(float val) { reg = _mm256_set1_ps(val); }

		static TFloatVector Zero() { return TFloatVector(0.0); }

		float  operator [](int index) const { return reg.m256_f32[index]; }
		float& operator [](int index) { return reg.m256_f32[index]; }

		FORCEINLINE TFloatVector operator * (TFloatVector const& rhs) const { return _mm256_mul_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator / (TFloatVector const& rhs) const { return _mm256_div_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator + (TFloatVector const& rhs) const { return _mm256_add_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator - (TFloatVector const& rhs) const { return _mm256_sub_ps(reg, rhs.reg); }

		FORCEINLINE TFloatVector operator * (float rhs) const { return _mm256_mul_ps(reg, _mm256_set1_ps(rhs)); }
		FORCEINLINE TFloatVector operator / (float rhs) const { return _mm256_div_ps(reg, _mm256_set1_ps(rhs)); }
		FORCEINLINE TFloatVector operator + (float rhs) const { return _mm256_add_ps(reg, _mm256_set1_ps(rhs)); }
		FORCEINLINE TFloatVector operator - (float rhs) const { return _mm256_sub_ps(reg, _mm256_set1_ps(rhs)); }

		FORCEINLINE TFloatVector operator <  (TFloatVector const& rhs) const { return _mm256_cmp_ps(reg, rhs.reg, _CMP_LT_OQ); }
		FORCEINLINE TFloatVector operator <= (TFloatVector const& rhs) const { return _mm256_cmp_ps(reg, rhs.reg, _CMP_LE_OQ); }
		FORCEINLINE TFloatVector operator >  (TFloatVector const& rhs) const { return _mm256_cmp_ps(reg, rhs.reg, _CMP_GT_OQ); }
		FORCEINLINE TFloatVector operator >= (TFloatVector const& rhs) const { return _mm256_cmp_ps(reg, rhs.reg, _CMP_GE_OQ); }
		FORCEINLINE TFloatVector operator == (TFloatVector const& rhs) const { return _mm256_cmp_ps(reg, rhs.reg, _CMP_EQ_OQ); }
		FORCEINLINE TFloatVector operator != (TFloatVector const& rhs) const { return _mm256_cmp_ps(reg, rhs.reg, _CMP_NEQ_OQ); }

		FORCEINLINE TFloatVector operator & (TFloatVector const& rhs) const { return _mm256_and_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator | (TFloatVector const& rhs) const { return _mm256_or_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator ^ (TFloatVector const& rhs) const { return _mm256_xor_ps(reg, rhs.reg); }

		FORCEINLINE float sum() const
		{
			__m128 vlow = _mm256_castps256_ps128(reg);
			__m128 vhigh = _mm256_extractf128_ps(reg, 1);
			vlow = _mm_add_ps(vlow, vhigh);
			__m128 shuf = _mm_movehdup_ps(vlow);
			__m128 sums = _mm_add_ps(vlow, shuf);
			shuf = _mm_movehl_ps(shuf, sums);
			sums = _mm_add_ss(sums, shuf);
			return _mm_cvtss_f32(sums);
		}

		template< int Scale >
		FORCEINLINE static TFloatVector gather(float const* base, __m256i const& indices)
		{
			return _mm256_i32gather_ps(base, indices, Scale);
		}

		FORCEINLINE bool isAnyActive() const
		{
			return !_mm256_testz_si256(_mm256_castps_si256(reg), _mm256_castps_si256(reg));
		}

		static int constexpr Size = 8;
		__m256 reg;
	};

	template<>
	struct alignas(32) TIntVector<8>
	{
		TIntVector() = default;
		FORCEINLINE TIntVector(int val) { reg = _mm256_set1_epi32(val); }
		FORCEINLINE TIntVector(__m256i val) : reg(val) {}
		FORCEINLINE static TIntVector load(int const* p) { return _mm256_loadu_si256((__m256i const*)p); }
		operator __m256i () const { return reg; }
		__m256i reg;
	};

	FORCEINLINE TFloatVector<8> operator * (float lhs, TFloatVector<8> const& rhs) { return _mm256_mul_ps(_mm256_set1_ps(lhs), rhs.reg); }
	FORCEINLINE TFloatVector<8> operator / (float lhs, TFloatVector<8> const& rhs) { return _mm256_div_ps(_mm256_set1_ps(lhs), rhs.reg); }
	FORCEINLINE TFloatVector<8> operator + (float lhs, TFloatVector<8> const& rhs) { return _mm256_add_ps(_mm256_set1_ps(lhs), rhs.reg); }
	FORCEINLINE TFloatVector<8> operator - (float lhs, TFloatVector<8> const& rhs) { return _mm256_sub_ps(_mm256_set1_ps(lhs), rhs.reg); }

	FORCEINLINE TFloatVector<8> operator-(TFloatVector<8> const& a)
	{
		return TFloatVector<8>(_mm256_xor_ps(a.reg, _mm256_set1_ps(-0.f)));
	}

#define SIMD_FUNC_FALLBACK(FUNC, V)\
		TFloatVector<8>{ FUNC(V[0]),FUNC(V[1]),FUNC(V[2]),FUNC(V[3]),FUNC(V[4]),FUNC(V[5]),FUNC(V[6]),FUNC(V[7]) }

	FORCEINLINE TFloatVector<8> exp(TFloatVector<8> const& value)
	{
		return exp256_ps(value.reg);
	}

	FORCEINLINE TFloatVector<8> log(TFloatVector<8> const& value)
	{
		return log256_ps(value.reg);
	}

	FORCEINLINE TFloatVector<8> sin(TFloatVector<8> const& value)
	{
#if 0
		return sin256_ps(value.reg);
#else
		return SIMD_FUNC_FALLBACK(::sin, value);
#endif
	}

	FORCEINLINE TFloatVector<8> cos(TFloatVector<8> const& value)
	{
		return cos256_ps(value.reg);
	}

	FORCEINLINE TFloatVector<8> tan(TFloatVector<8> const& value)
	{
		__m256 s, c;
		sincos256_ps(value.reg, &s, &c);
		return _mm256_div_ps(s, c);
	}

	FORCEINLINE TFloatVector<8> sqrt(TFloatVector<8> const& value)
	{
		return _mm256_sqrt_ps(value.reg);
	}

	FORCEINLINE TFloatVector<8> rsqrt(TFloatVector<8> const& value)
	{
		return _mm256_rsqrt_ps(value.reg);
	}

	FORCEINLINE TFloatVector<8> min(TFloatVector<8> const& a, TFloatVector<8> const& b)
	{
		return _mm256_min_ps(a.reg, b.reg);
	}

	FORCEINLINE TFloatVector<8> max(TFloatVector<8> const& a, TFloatVector<8> const& b)
	{
		return _mm256_max_ps(a.reg, b.reg);
	}

	FORCEINLINE TFloatVector<8> select(TFloatVector<8> const& mask, TFloatVector<8> const& vTrue, TFloatVector<8> const& vFalse)
	{
		return _mm256_blendv_ps(vFalse.reg, vTrue.reg, mask.reg);
	}

	FORCEINLINE TFloatVector<8> abs(TFloatVector<8> const& v)
	{
		return _mm256_andnot_ps(_mm256_set1_ps(-0.0f), v.reg);
	}


	template<>
	struct alignas(16) TFloatVector<4>
	{
		TFloatVector() = default;

		operator __m128 () const { return reg; }

		FORCEINLINE TFloatVector(float x, float y, float z, float w) { reg = _mm_set_ps(w, z, y, x); }

		FORCEINLINE TFloatVector(std::initializer_list<float> elements)
		{
			reg = _mm_set_ps(elements.begin()[3], elements.begin()[2], elements.begin()[1], elements.begin()[0]);
		}

		FORCEINLINE TFloatVector(__m128 val) : reg(val) {}
		FORCEINLINE TFloatVector(float const* v) { reg = _mm_loadu_ps(v); }
		FORCEINLINE TFloatVector(float const* v, EAligned) { reg = _mm_load_ps(v); }
		FORCEINLINE TFloatVector(float val) { reg = _mm_set1_ps(val); }

		static TFloatVector Zero() { return TFloatVector(0.0); }

		float  operator [](int index) const { return reg.m128_f32[index]; }
		float& operator [](int index){ return reg.m128_f32[index]; }

		FORCEINLINE TFloatVector operator * (TFloatVector const& rhs) const { return _mm_mul_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator / (TFloatVector const& rhs) const { return _mm_div_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator + (TFloatVector const& rhs) const { return _mm_add_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator - (TFloatVector const& rhs) const { return _mm_sub_ps(reg, rhs.reg); }

		FORCEINLINE TFloatVector operator * (float rhs) const { return _mm_mul_ps(reg, _mm_set1_ps(rhs)); }
		FORCEINLINE TFloatVector operator / (float rhs) const { return _mm_div_ps(reg, _mm_set1_ps(rhs)); }
		FORCEINLINE TFloatVector operator + (float rhs) const { return _mm_add_ps(reg, _mm_set1_ps(rhs)); }
		FORCEINLINE TFloatVector operator - (float rhs) const { return _mm_sub_ps(reg, _mm_set1_ps(rhs)); }

		FORCEINLINE TFloatVector operator <  (TFloatVector const& rhs) const { return _mm_cmplt_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator <= (TFloatVector const& rhs) const { return _mm_cmple_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator >  (TFloatVector const& rhs) const { return _mm_cmpgt_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator >= (TFloatVector const& rhs) const { return _mm_cmpge_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator == (TFloatVector const& rhs) const { return _mm_cmpeq_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator != (TFloatVector const& rhs) const { return _mm_cmpneq_ps(reg, rhs.reg); }

		FORCEINLINE TFloatVector operator & (TFloatVector const& rhs) const { return _mm_and_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator | (TFloatVector const& rhs) const { return _mm_or_ps(reg, rhs.reg); }
		FORCEINLINE TFloatVector operator ^ (TFloatVector const& rhs) const { return _mm_xor_ps(reg, rhs.reg); }

		FORCEINLINE float sum() const
		{
			__m128 shuf = _mm_movehdup_ps(reg);
			__m128 sums = _mm_add_ps(reg, shuf);
			shuf = _mm_movehl_ps(shuf, sums);
			sums = _mm_add_ss(sums, shuf);
			return _mm_cvtss_f32(sums);
		}

		template< int Scale >
		FORCEINLINE static TFloatVector gather(float const* base, __m128i const& indices)
		{
			// SSE doesn't have native gather, implement an optimized emulation
			int const* pIdx = (int const*)&indices;
			return _mm_setr_ps(
				*(float const*)((uint8 const*)base + pIdx[0] * Scale),
				*(float const*)((uint8 const*)base + pIdx[1] * Scale),
				*(float const*)((uint8 const*)base + pIdx[2] * Scale),
				*(float const*)((uint8 const*)base + pIdx[3] * Scale)
			);
		}

		FORCEINLINE bool isAnyActive() const
		{
			return _mm_movemask_ps(reg) != 0;
		}

		static int constexpr Size = 4;
		__m128 reg;
	};

	template<>
	struct alignas(16) TIntVector<4>
	{
		TIntVector() = default;
		FORCEINLINE TIntVector(int val) { reg = _mm_set1_epi32(val); }
		FORCEINLINE TIntVector(__m128i val) : reg(val) {}
		FORCEINLINE static TIntVector load(int const* p) { return _mm_loadu_si128((__m128i const*)p); }
		operator __m128i () const { return reg; }
		__m128i reg;
	};


	FORCEINLINE TFloatVector<4> operator * (float lhs, TFloatVector<4> const& rhs) { return _mm_mul_ps(_mm_set1_ps(lhs), rhs.reg); }
	FORCEINLINE TFloatVector<4> operator / (float lhs, TFloatVector<4> const& rhs) { return _mm_div_ps(_mm_set1_ps(lhs), rhs.reg); }
	FORCEINLINE TFloatVector<4> operator + (float lhs, TFloatVector<4> const& rhs) { return _mm_add_ps(_mm_set1_ps(lhs), rhs.reg); }
	FORCEINLINE TFloatVector<4> operator - (float lhs, TFloatVector<4> const& rhs) { return _mm_sub_ps(_mm_set1_ps(lhs), rhs.reg); }

	FORCEINLINE TFloatVector<4> operator-(TFloatVector<4> const& a)
	{
		return TFloatVector<4>(_mm_xor_ps(a.reg, _mm_set1_ps(-0.f)));
	}

	FORCEINLINE TFloatVector<4> exp(TFloatVector<4> const& value)
	{
		return DirectX::XMVectorExpE(value);
	}

	FORCEINLINE TFloatVector<4> log(TFloatVector<4> const& value)
	{
		return DirectX::XMVectorLogE(value);
	}

	FORCEINLINE TFloatVector<4> sin(TFloatVector<4> const& value)
	{
		return DirectX::XMVectorSin(value);
	}

	FORCEINLINE TFloatVector<4> cos(TFloatVector<4> const& value)
	{
		return DirectX::XMVectorCos(value);
	}

	FORCEINLINE TFloatVector<4> tan(TFloatVector<4> const& value)
	{
		return DirectX::XMVectorTan(value);
	}

	FORCEINLINE TFloatVector<4> sqrt(TFloatVector<4> const& value)
	{
		return _mm_sqrt_ps(value.reg);
	}

	FORCEINLINE TFloatVector<4> rsqrt(TFloatVector<4> const& value)
	{
		return _mm_rsqrt_ps(value.reg);
	}

	FORCEINLINE TFloatVector<4> min(TFloatVector<4> const& a, TFloatVector<4> const& b)
	{
		return _mm_min_ps(a.reg, b.reg);
	}

	FORCEINLINE TFloatVector<4> max(TFloatVector<4> const& a, TFloatVector<4> const& b)
	{
		return _mm_max_ps(a.reg, b.reg);
	}

	FORCEINLINE TFloatVector<4> select(TFloatVector<4> const& mask, TFloatVector<4> const& vTrue, TFloatVector<4> const& vFalse)
	{
#if SYS_PLATFORM_WIN
		return _mm_blendv_ps(vFalse.reg, vTrue.reg, mask.reg);
#else
		return _mm_or_ps(_mm_and_ps(mask.reg, vTrue.reg), _mm_andnot_ps(mask.reg, vFalse.reg));
#endif
	}

	FORCEINLINE TFloatVector<4> abs(TFloatVector<4> const& v)
	{
		return _mm_andnot_ps(_mm_set1_ps(-0.0f), v.reg);
	}
}//namespace SIMD

#endif

#endif // SIMD_H_A48E9490_F46C_48AF_8AB6_98AE7574890B