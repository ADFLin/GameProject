#ifndef CFMath_h__
#define CFMath_h__

#include <cmath>
#include <cassert>

#include "Math/Base.h"
//#define CF_USE_SSE

#ifdef CF_USE_SSE
#	include <emmintrin.h>
#	define CF_ALIGNED16(a)  __declspec(align(16)) a
#	define CF_ALIGNED64(a)  __declspec(align(64)) a
#	define CF_ALIGNED128(a) __declspec(align(128)) a
#else
#	define CF_ALIGNED16(a)  a
#	define CF_ALIGNED64(a)  a
#	define CF_ALIGNED128(a) a
#endif //CF_USE_SSE

namespace CFly
{
	namespace Math = ::Math;

}//namespace CFly


#endif // CFMath_h__