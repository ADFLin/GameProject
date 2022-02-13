#include "FFT.h"

#include "BitUtility.h"

#include <complex>
#include <vector>

namespace kissfft_utils {

	template <typename T_scalar>
	struct traits
	{
		typedef T_scalar scalar_type;
		typedef std::complex<scalar_type> cpx_type;
		void fill_twiddles(std::complex<T_scalar> * dst, int nfft, bool inverse)
		{
			T_scalar phinc = (inverse ? 2 : -2)* acos((T_scalar)-1) / nfft;
			for( int i = 0; i < nfft; ++i )
				dst[i] = exp(std::complex<T_scalar>(0, i*phinc));
		}

		void prepare(
			std::vector< std::complex<T_scalar> > & dst,
			int nfft, bool inverse,
			std::vector<int> & stageRadix,
			std::vector<int> & stageRemainder)
		{
			_twiddles.resize(nfft);
			fill_twiddles(&_twiddles[0], nfft, inverse);
			dst = _twiddles;

			//factorize
			//start factoring out 4's, then 2's, then 3,5,7,9,...
			int n = nfft;
			int p = 2;
			do
			{
				while( n % p )
				{
					switch( p )
					{
						//case 4: p = 2; break;
					case 2: p = 3; break;
					default: p += 2; break;
					}
					if( p*p > n )
						p = n;// no more factors
				}
				n /= p;
				stageRadix.push_back(p);
				stageRemainder.push_back(n);
			} while( n > 1 );
		}
		std::vector<cpx_type> _twiddles;


		const cpx_type twiddle(int i) { return _twiddles[i]; }
	};

}

template <typename T_Scalar,
	typename T_traits = kissfft_utils::traits<T_Scalar>
>
class kissfft
{
public:
	typedef T_traits traits_type;
	typedef typename traits_type::scalar_type scalar_type;
	typedef typename traits_type::cpx_type cpx_type;

	kissfft(int nfft, bool inverse, const traits_type & traits = traits_type())
		:_nfft(nfft), _inverse(inverse), _traits(traits)
	{
		_traits.prepare(_twiddles, _nfft, _inverse, _stageRadix, _stageRemainder);
	}

	void transform(const cpx_type * src, cpx_type * dst)
	{
		kf_work(0, dst, src, 1, 1);
	}

private:
	void kf_work(int stage, cpx_type * Fout, const cpx_type * f, size_t fstride, size_t in_stride)
	{
		int p = _stageRadix[stage];
		int m = _stageRemainder[stage];
		cpx_type * Fout_beg = Fout;
		cpx_type * Fout_end = Fout + p*m;

		if( m == 1 )
		{
			do
			{
				*Fout = *f;
				f += fstride*in_stride;
			} while( ++Fout != Fout_end );
		}
		else
		{
			do
			{
				// recursive call:
				// DFT of size m*p performed by doing
				// p instances of smaller DFTs of size m, 
				// each one takes a decimated version of the input
				kf_work(stage + 1, Fout, f, fstride*p, in_stride);
				f += fstride*in_stride;
			} while( (Fout += m) != Fout_end );
		}

		Fout = Fout_beg;

		// recombine the p smaller DFTs 
		switch( p )
		{
		case 2: kf_bfly2(Fout, fstride, m); break;
		case 3: kf_bfly3(Fout, fstride, m); break;
		case 4: kf_bfly4(Fout, fstride, m); break;
		case 5: kf_bfly5(Fout, fstride, m); break;
		default: kf_bfly_generic(Fout, fstride, m, p); break;
		}
	}

	// these were #define macros in the original kiss_fft
	static void C_ADD(cpx_type & c, const cpx_type & a, const cpx_type & b) { c = a + b; }
	static void C_MUL(cpx_type & c, const cpx_type & a, const cpx_type & b) { c = a*b; }
	static void C_SUB(cpx_type & c, const cpx_type & a, const cpx_type & b) { c = a - b; }
	static void C_ADDTO(cpx_type & c, const cpx_type & a) { c += a; }
	static void C_FIXDIV(cpx_type &, int) {} // NO-OP for float types
	static scalar_type S_MUL(const scalar_type & a, const scalar_type & b) { return a*b; }
	static scalar_type HALF_OF(const scalar_type & a) { return a*.5; }
	void C_MULBYSCALAR(cpx_type & c, const scalar_type & a) { c *= a; }

	void kf_bfly2(cpx_type * Fout, const size_t fstride, int m)
	{
		for( int k = 0; k < m; ++k )
		{
			cpx_type t = Fout[m + k] * _traits.twiddle(k*fstride);
			Fout[m + k] = Fout[k] - t;
			Fout[k] += t;
		}
	}

	void kf_bfly4(cpx_type * Fout, const size_t fstride, const size_t m)
	{
		cpx_type scratch[7];
		int negative_if_inverse = _inverse * -2 + 1;
		for( size_t k = 0; k < m; ++k )
		{
			scratch[0] = Fout[k + m] * _traits.twiddle(k*fstride);
			scratch[1] = Fout[k + 2 * m] * _traits.twiddle(k*fstride * 2);
			scratch[2] = Fout[k + 3 * m] * _traits.twiddle(k*fstride * 3);
			scratch[5] = Fout[k] - scratch[1];

			Fout[k] += scratch[1];
			scratch[3] = scratch[0] + scratch[2];
			scratch[4] = scratch[0] - scratch[2];
			scratch[4] = cpx_type(scratch[4].imag()*negative_if_inverse, -scratch[4].real()* negative_if_inverse);

			Fout[k + 2 * m] = Fout[k] - scratch[3];
			Fout[k] += scratch[3];
			Fout[k + m] = scratch[5] + scratch[4];
			Fout[k + 3 * m] = scratch[5] - scratch[4];
		}
	}

	void kf_bfly3(cpx_type * Fout, const size_t fstride, const size_t m)
	{
		size_t k = m;
		const size_t m2 = 2 * m;
		cpx_type *tw1, *tw2;
		cpx_type scratch[5];
		cpx_type epi3;
		epi3 = _twiddles[fstride*m];

		tw1 = tw2 = &_twiddles[0];

		do
		{
			C_FIXDIV(*Fout, 3); C_FIXDIV(Fout[m], 3); C_FIXDIV(Fout[m2], 3);

			C_MUL(scratch[1], Fout[m], *tw1);
			C_MUL(scratch[2], Fout[m2], *tw2);

			C_ADD(scratch[3], scratch[1], scratch[2]);
			C_SUB(scratch[0], scratch[1], scratch[2]);
			tw1 += fstride;
			tw2 += fstride * 2;

			Fout[m] = cpx_type(Fout->real() - HALF_OF(scratch[3].real()), Fout->imag() - HALF_OF(scratch[3].imag()));

			C_MULBYSCALAR(scratch[0], epi3.imag());

			C_ADDTO(*Fout, scratch[3]);

			Fout[m2] = cpx_type(Fout[m].real() + scratch[0].imag(), Fout[m].imag() - scratch[0].real());

			C_ADDTO(Fout[m], cpx_type(-scratch[0].imag(), scratch[0].real()));
			++Fout;
		} while( --k );
	}

	void kf_bfly5(cpx_type * Fout, const size_t fstride, const size_t m)
	{
		cpx_type *Fout0, *Fout1, *Fout2, *Fout3, *Fout4;
		size_t u;
		cpx_type scratch[13];
		cpx_type * twiddles = &_twiddles[0];
		cpx_type *tw;
		cpx_type ya, yb;
		ya = twiddles[fstride*m];
		yb = twiddles[fstride * 2 * m];

		Fout0 = Fout;
		Fout1 = Fout0 + m;
		Fout2 = Fout0 + 2 * m;
		Fout3 = Fout0 + 3 * m;
		Fout4 = Fout0 + 4 * m;

		tw = twiddles;
		for( u = 0; u < m; ++u )
		{
			C_FIXDIV(*Fout0, 5); C_FIXDIV(*Fout1, 5); C_FIXDIV(*Fout2, 5); C_FIXDIV(*Fout3, 5); C_FIXDIV(*Fout4, 5);
			scratch[0] = *Fout0;

			C_MUL(scratch[1], *Fout1, tw[u*fstride]);
			C_MUL(scratch[2], *Fout2, tw[2 * u*fstride]);
			C_MUL(scratch[3], *Fout3, tw[3 * u*fstride]);
			C_MUL(scratch[4], *Fout4, tw[4 * u*fstride]);

			C_ADD(scratch[7], scratch[1], scratch[4]);
			C_SUB(scratch[10], scratch[1], scratch[4]);
			C_ADD(scratch[8], scratch[2], scratch[3]);
			C_SUB(scratch[9], scratch[2], scratch[3]);

			C_ADDTO(*Fout0, scratch[7]);
			C_ADDTO(*Fout0, scratch[8]);

			scratch[5] = scratch[0] + cpx_type(
				S_MUL(scratch[7].real(), ya.real()) + S_MUL(scratch[8].real(), yb.real()),
				S_MUL(scratch[7].imag(), ya.real()) + S_MUL(scratch[8].imag(), yb.real())
			);

			scratch[6] = cpx_type(
				S_MUL(scratch[10].imag(), ya.imag()) + S_MUL(scratch[9].imag(), yb.imag()),
				-S_MUL(scratch[10].real(), ya.imag()) - S_MUL(scratch[9].real(), yb.imag())
			);

			C_SUB(*Fout1, scratch[5], scratch[6]);
			C_ADD(*Fout4, scratch[5], scratch[6]);

			scratch[11] = scratch[0] +
				cpx_type(
					S_MUL(scratch[7].real(), yb.real()) + S_MUL(scratch[8].real(), ya.real()),
					S_MUL(scratch[7].imag(), yb.real()) + S_MUL(scratch[8].imag(), ya.real())
				);

			scratch[12] = cpx_type(
				-S_MUL(scratch[10].imag(), yb.imag()) + S_MUL(scratch[9].imag(), ya.imag()),
				S_MUL(scratch[10].real(), yb.imag()) - S_MUL(scratch[9].real(), ya.imag())
			);

			C_ADD(*Fout2, scratch[11], scratch[12]);
			C_SUB(*Fout3, scratch[11], scratch[12]);

			++Fout0; ++Fout1; ++Fout2; ++Fout3; ++Fout4;
		}
	}

	/* perform the butterfly for one stage of a mixed radix FFT */
	void kf_bfly_generic(
		cpx_type * Fout,
		const size_t fstride,
		int m,
		int p)
	{
		int u, k, q1, q;
		cpx_type * twiddles = &_twiddles[0];
		cpx_type t;
		int Norig = _nfft;
		cpx_type* scratchbuf = (cpx_type*)alloca(sizeof(cpx_type)*p);

		for( u = 0; u < m; ++u )
		{
			k = u;
			for( q1 = 0; q1 < p; ++q1 )
			{
				scratchbuf[q1] = Fout[k];
				C_FIXDIV(scratchbuf[q1], p);
				k += m;
			}

			k = u;
			for( q1 = 0; q1 < p; ++q1 )
			{
				int twidx = 0;
				Fout[k] = scratchbuf[0];
				for( q = 1; q < p; ++q )
				{
					twidx += fstride * k;
					if( twidx >= Norig ) twidx -= Norig;
					C_MUL(t, scratchbuf[q], twiddles[twidx]);
					C_ADDTO(Fout[k], t);
				}
				k += m;
			}
		}
	}

	int _nfft;
	bool _inverse;
	std::vector<cpx_type> _twiddles;
	std::vector<int> _stageRadix;
	std::vector<int> _stageRemainder;
	traits_type _traits;
};
#include <complex.h>

void FFT::Transform(float data[], int numData, Complex outData[])
{
#if FFT_METHOD == 0
	if( IsPowOf2(numData) )
	{
		Context context;
		context.factors.resize(numData);
		for( int i = 0; i < numData; ++i )
		{
			context.factors[i] = Complex::Expi(-2 * Math::PI * i / numData);
		}

		std::vector< Complex > tempData(numData);
		for( uint32 i = 0; i < numData; ++i )
		{
			tempData[i].r = data[i];
			tempData[i].i = 0;
		}
		Transform2_R(context, &tempData[0], numData, outData, 1);
	}
	else
	{
		Transform_G(data, numData, outData);
	}
#elif FFT_METHOD == 1
	if( IsPowOf2(numData) )
	{
		Context context;
		context.factors.resize(numData);
		context.reverseIndices.resize(numData);
		context.sampleSize = numData;
		uint32 count = FBitUtility::CountTrailingZeros(numData);
		uint32 offset = 32 - FBitUtility::CountTrailingZeros(numData);
		for( int i = 0; i < numData; ++i )
		{
			context.factors[i] = Complex::Expi(-2 * Math::PI * i / numData);
			context.reverseIndices[i] = FBitUtility::Reverse(uint32(i)) >> offset;
		}

		for( uint32 i = 0; i < numData; ++i )
		{
			outData[i].r = data[context.reverseIndices[i]];
			outData[i].i = 0;
		}
		Transform2_RA(context, numData, outData, 1);
	}
	else
	{
		Transform_G(data, numData, outData);
	}
#elif FFT_METHOD == 2
	std::vector< Complex > tempData(numData);
	for( int i = 0; i < numData; ++i )
	{
		tempData[i].r = data[i];
		tempData[i].i = 0;

		outData[i].r = data[i];
		outData[i].i = 0;
	}

	_fft(outData, &tempData[0], numData, 1);
#else
	kissfft< float > fft{ numData , false };

	typedef std::complex< float > ctype;
	std::vector< ctype > inputs;
	inputs.resize(numData);
	for( int i = 0; i < numData; ++i )
	{
		inputs[i] = ctype(data[i], 0);
	}

	std::vector< ctype > outputs;
	outputs.resize(numData);
	fft.transform(&inputs[0], &outputs[0]);

	for( int i = 0; i < numData; ++i )
	{
		outData[i].r = outputs[i].real();
		outData[i].i = outputs[i].imag();
	}
#endif
}

#define PLATFORM_CACHE_LINE_SIZE 64
FORCEINLINE static void Prefetch(void const* x, int32 offset = 0)
{
	_mm_prefetch((char const*)(x)+offset, _MM_HINT_T0);
}

void FFT::Transform2_RA(Context& context, int numData, Complex outData[], uint stride)
{
	assert(IsPowOf2(numData));
#if 1

#if 1
	int  curStride = context.sampleSize / 2;
	int  curNum = 1;

	while( curNum < context.sampleSize )
	{
		int offset = 2 * curNum;
		for( int n = 0; n < context.sampleSize; n += offset )
		{
			Complex* RESTRICT pData1 = outData + n;
			Complex* RESTRICT pData2 = pData1 + curNum;
			Complex* RESTRICT pFactor = context.factors.data();
			for( int i = 0; i < curNum; ++i )
			{
#if 0
				Prefetch(pData1 + PLATFORM_CACHE_LINE_SIZE);
				Prefetch(pData2 + PLATFORM_CACHE_LINE_SIZE);
#endif

				Complex factor = *pFactor * (*pData2);
				Complex temp = *pData1;

				*pData1 = temp + factor;
				*pData2 = temp - factor;
				++pData1;
				++pData2;
				pFactor += curStride;
			}
		}

		curStride /= 2;
		curNum *= 2;
	}

#else

	int numSubData = numData / 2;
	if( numSubData > 1 )
	{
		Transform2_RA(context, numSubData, outData, stride * 2);
	}

	for( int n = 0; n < context.sampleSize; n += numData )
	{
		Complex* RESTRICT pData1 = outData + n;
		Complex* RESTRICT pData2 = pData1  + numSubData;
		for( int i = 0; i < numSubData; ++i )
		{
#if 0
			Prefetch(pData1 + PLATFORM_CACHE_LINE_SIZE);
			Prefetch(pData2 + PLATFORM_CACHE_LINE_SIZE);
#endif

			Complex factor = context.factors[i * stride] * (*pData2);
			Complex temp = *pData1;

			*pData1 = temp + factor;
			*pData2 = temp - factor;
			++pData1;
			++pData2;
		}
	}
#endif

#elif 0

	int numSubData = numData / 2;
	if( numSubData > 1 )
	{
		Transform2_RA(context, numSubData, outData, stride * 2);
	}

	for( int n = 0; n < stride; ++n )
	{
		Complex* RESTRICT pData1 = outData + n * numData;
		Complex* RESTRICT pData2 = pData1 + numSubData;
		for( int i = 0; i < numSubData; ++i )
		{
#if 0
			Prefetch(pData1 + PLATFORM_CACHE_LINE_SIZE);
			Prefetch(pData2 + PLATFORM_CACHE_LINE_SIZE);
#endif

			Complex factor = context.factors[i * stride] * (*pData2);
			Complex temp = *pData1;

			*pData1 = temp + factor;
			*pData2 = temp - factor;
			++pData1;
			++pData2;
		}
	}

#else

	int numSubData = numData / 2;
	if( numSubData > 1 )
	{
		Transform2_RA(context, numSubData, outData, stride * 2);
		Transform2_RA(context, numSubData, outData + numSubData, stride * 2);
	}


	Complex* RESTRICT pData1 = outData;
	Complex* RESTRICT pData2 = pData1 + numSubData;
	for( int i = 0; i < numSubData; ++i )
	{
		Complex factor = context.factors[i * stride] * (*pData2);
		Complex temp = *pData1;
		*pData1 = temp + factor;
		*pData2 = temp - factor;
		++pData1;
		++pData2;
	}

#endif
}

void FFT::Transform2_R(Context& context, Complex data[], int numData, Complex outData[], uint stride)
{
	assert(IsPowOf2(numData));
	if( numData == 1 )
	{
		outData[0] = data[0];
	}
	else
	{
		int numSubData = numData / 2;
		Transform2_R(context, data, numSubData, outData, stride * 2);
		Transform2_R(context, data + stride, numSubData, outData + numSubData, stride * 2);

		Complex* RESTRICT pData1 = outData;
		Complex* RESTRICT pData2 = outData + numSubData;
		//Complex* RESTRICT pFactor = &context.factors[0];
		for( int i = 0; i < numSubData; ++i )
		{
			Complex factor = context.factors[i * stride] * (*pData2);
			Complex temp = *pData1;
			*pData1 = temp + factor;
			*pData2 = temp - factor;
			++pData1;
			++pData2;
			//pFactor += stride;
		}
	}
}

void FFT::Transform_G(float data[], int numData, Complex outData[])
{
	for( int n = 0; n < numData; ++n )
	{
		outData[n] = Complex{ 0 ,0 };

		for( int k = 0; k < numData; ++k )
		{
			float theta = 2 * Math::PI * k * n / numData;
			float c, s;
			Math::SinCos(theta, s, c);
			outData[n].r += data[k] * c;
			outData[n].i -= data[k] * s;
		}
	}
}
