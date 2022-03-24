#include "MD5.h"

#include "Core/Memory.h"

/* MD5
converted to C++ class by Frank Thilo (thilo@unix-ag.org)
for bzflag (http://www.bzflag.org)

based on:

md5.h and md5.c
reference implemantion of RFC 1321

Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

*/

/* system implementation headers */
#include <cstdio>

///////////////////////////////////////////////

// F, G, H and I are basic MD5 functions.
inline uint32 MD5_F(uint32 x, uint32 y, uint32 z)
{
	return x&y | ~x&z;
}

inline uint32 MD5_G(uint32 x, uint32 y, uint32 z)
{
	return x&z | y&~z;
}

inline uint32 MD5_H(uint32 x, uint32 y, uint32 z)
{
	return x^y^z;
}

inline uint32 MD5_I(uint32 x, uint32 y, uint32 z)
{
	return y ^ (x | ~z);
}

// rotate_left rotates x left n bits.
inline uint32  MD5_rotate_left(uint32 x, int n)
{
	return (x << n) | (x >> (32 - n));
}

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
inline void  MD5_FF(uint32 &a, uint32 b, uint32 c, uint32 d, uint32 x, uint32 s, uint32 ac)
{
	a = MD5_rotate_left(a + MD5_F(b, c, d) + x + ac, s) + b;
}

inline void  MD5_GG(uint32 &a, uint32 b, uint32 c, uint32 d, uint32 x, uint32 s, uint32 ac)
{
	a = MD5_rotate_left(a + MD5_G(b, c, d) + x + ac, s) + b;
}

inline void  MD5_HH(uint32 &a, uint32 b, uint32 c, uint32 d, uint32 x, uint32 s, uint32 ac)
{
	a = MD5_rotate_left(a + MD5_H(b, c, d) + x + ac, s) + b;
}

inline void  MD5_II(uint32 &a, uint32 b, uint32 c, uint32 d, uint32 x, uint32 s, uint32 ac)
{
	a = MD5_rotate_left(a + MD5_I(b, c, d) + x + ac, s) + b;
}

//////////////////////////////////////////////

// default ctor, just initailize
MD5::MD5()
{
	init();
}

//////////////////////////////////////////////

// nifty shortcut ctor, compute MD5 for string and finalize it right away
MD5::MD5(const std::string &text)
{
	init();
	update((uint8 const*)text.c_str(), text.length());
	finalize();
}

//////////////////////////////

void MD5::init()
{
	finalized = false;

	count[0] = 0;
	count[1] = 0;

	// load magic initialization constants.
	state[0] = 0x67452301;
	state[1] = 0xefcdab89;
	state[2] = 0x98badcfe;
	state[3] = 0x10325476;
}

//////////////////////////////

// decodes input (unsigned char) into output (uint4). Assumes len is a multiple of 4.
void MD5::decode(uint32 output[], const uint8 input[], size_type len)
{
	for( unsigned int i = 0, j = 0; j < len; i++, j += 4 )
		output[i] = ((uint32)input[j]) | (((uint32)input[j + 1]) << 8) |
		(((uint32)input[j + 2]) << 16) | (((uint32)input[j + 3]) << 24);
}

//////////////////////////////

// encodes input (uint4) into output (unsigned char). Assumes len is
// a multiple of 4.
void MD5::encode(uint8 output[], const uint32 input[], size_type len)
{
	for( size_type i = 0, j = 0; j < len; i++, j += 4 )
	{
		output[j] = input[i] & 0xff;
		output[j + 1] = (input[i] >> 8) & 0xff;
		output[j + 2] = (input[i] >> 16) & 0xff;
		output[j + 3] = (input[i] >> 24) & 0xff;
	}
}

//////////////////////////////

// apply MD5 algo on a block
void MD5::transform(const uint8 block[blocksize])
{
	// Constants for MD5Transform routine.
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

	uint32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];
	decode(x, block, blocksize);

	/* Round 1 */
	MD5_FF(a, b, c, d, x[0], S11, 0xd76aa478); /* 1 */
	MD5_FF(d, a, b, c, x[1], S12, 0xe8c7b756); /* 2 */
	MD5_FF(c, d, a, b, x[2], S13, 0x242070db); /* 3 */
	MD5_FF(b, c, d, a, x[3], S14, 0xc1bdceee); /* 4 */
	MD5_FF(a, b, c, d, x[4], S11, 0xf57c0faf); /* 5 */
	MD5_FF(d, a, b, c, x[5], S12, 0x4787c62a); /* 6 */
	MD5_FF(c, d, a, b, x[6], S13, 0xa8304613); /* 7 */
	MD5_FF(b, c, d, a, x[7], S14, 0xfd469501); /* 8 */
	MD5_FF(a, b, c, d, x[8], S11, 0x698098d8); /* 9 */
	MD5_FF(d, a, b, c, x[9], S12, 0x8b44f7af); /* 10 */
	MD5_FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	MD5_FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	MD5_FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	MD5_FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	MD5_FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	MD5_FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

											/* Round 2 */
	MD5_GG(a, b, c, d, x[1], S21, 0xf61e2562); /* 17 */
	MD5_GG(d, a, b, c, x[6], S22, 0xc040b340); /* 18 */
	MD5_GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	MD5_GG(b, c, d, a, x[0], S24, 0xe9b6c7aa); /* 20 */
	MD5_GG(a, b, c, d, x[5], S21, 0xd62f105d); /* 21 */
	MD5_GG(d, a, b, c, x[10], S22, 0x2441453); /* 22 */
	MD5_GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	MD5_GG(b, c, d, a, x[4], S24, 0xe7d3fbc8); /* 24 */
	MD5_GG(a, b, c, d, x[9], S21, 0x21e1cde6); /* 25 */
	MD5_GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	MD5_GG(c, d, a, b, x[3], S23, 0xf4d50d87); /* 27 */
	MD5_GG(b, c, d, a, x[8], S24, 0x455a14ed); /* 28 */
	MD5_GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	MD5_GG(d, a, b, c, x[2], S22, 0xfcefa3f8); /* 30 */
	MD5_GG(c, d, a, b, x[7], S23, 0x676f02d9); /* 31 */
	MD5_GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

											/* Round 3 */
	MD5_HH(a, b, c, d, x[5], S31, 0xfffa3942); /* 33 */
	MD5_HH(d, a, b, c, x[8], S32, 0x8771f681); /* 34 */
	MD5_HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	MD5_HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	MD5_HH(a, b, c, d, x[1], S31, 0xa4beea44); /* 37 */
	MD5_HH(d, a, b, c, x[4], S32, 0x4bdecfa9); /* 38 */
	MD5_HH(c, d, a, b, x[7], S33, 0xf6bb4b60); /* 39 */
	MD5_HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	MD5_HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	MD5_HH(d, a, b, c, x[0], S32, 0xeaa127fa); /* 42 */
	MD5_HH(c, d, a, b, x[3], S33, 0xd4ef3085); /* 43 */
	MD5_HH(b, c, d, a, x[6], S34, 0x4881d05); /* 44 */
	MD5_HH(a, b, c, d, x[9], S31, 0xd9d4d039); /* 45 */
	MD5_HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	MD5_HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	MD5_HH(b, c, d, a, x[2], S34, 0xc4ac5665); /* 48 */

										   /* Round 4 */
	MD5_II(a, b, c, d, x[0], S41, 0xf4292244); /* 49 */
	MD5_II(d, a, b, c, x[7], S42, 0x432aff97); /* 50 */
	MD5_II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	MD5_II(b, c, d, a, x[5], S44, 0xfc93a039); /* 52 */
	MD5_II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	MD5_II(d, a, b, c, x[3], S42, 0x8f0ccc92); /* 54 */
	MD5_II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	MD5_II(b, c, d, a, x[1], S44, 0x85845dd1); /* 56 */
	MD5_II(a, b, c, d, x[8], S41, 0x6fa87e4f); /* 57 */
	MD5_II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	MD5_II(c, d, a, b, x[6], S43, 0xa3014314); /* 59 */
	MD5_II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	MD5_II(a, b, c, d, x[4], S41, 0xf7537e82); /* 61 */
	MD5_II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	MD5_II(c, d, a, b, x[2], S43, 0x2ad7d2bb); /* 63 */
	MD5_II(b, c, d, a, x[9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	memset(x, 0, sizeof x);

#undef S11
#undef S12
#undef S13
#undef S14
#undef S21
#undef S22
#undef S23
#undef S24
#undef S31
#undef S32
#undef S33
#undef S34
#undef S41
#undef S42
#undef S43
#undef S44
}

//////////////////////////////

// MD5 block update operation. Continues an MD5 message-digest
// operation, processing another message block
void MD5::update(uint8 const input[], size_type length)
{
	// compute number of bytes mod 64
	size_type index = count[0] / 8 % blocksize;

	// Update number of bits
	if( (count[0] += (length << 3)) < (length << 3) )
		count[1]++;
	count[1] += (length >> 29);

	// number of bytes we need to fill in buffer
	size_type firstpart = 64 - index;

	size_type i;

	// transform as many times as possible.
	if( length >= firstpart )
	{
		// fill buffer first, transform
		FMemory::Copy(&buffer[index], input, firstpart);
		transform(buffer);

		// transform chunks of blocksize (64 bytes)
		for( i = firstpart; i + blocksize <= length; i += blocksize )
			transform(&input[i]);

		index = 0;
	}
	else
		i = 0;

	// buffer remaining input
	FMemory::Copy(&buffer[index], &input[i], length - i);
}

//////////////////////////////

// for convenience provide a verson with signed char
void MD5::update(int8 const input[], size_type length)
{
	update((uint8 const*)input, length);
}

//////////////////////////////

// MD5 finalization. Ends an MD5 message-digest operation, writing the
// the message digest and zeroizing the context.
MD5& MD5::finalize()
{
	static unsigned char padding[64] = 
	{
		0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	if( !finalized )
	{
		// Save number of bits
		uint8 bits[8];
		encode(bits, count, 8);

		// pad out to 56 mod 64.
		size_type index = count[0] / 8 % 64;
		size_type padLen = (index < 56) ? (56 - index) : (120 - index);
		update(padding, padLen);

		// Append length (before padding)
		update(bits, 8);

		// Store state in digest
		encode(digest, state, 16);

		// Zeroize sensitive information.
		memset(buffer, 0, sizeof buffer);
		memset(count, 0, sizeof count);

		finalized = true;
	}

	return *this;
}

//////////////////////////////

// return hex representation of digest as string
std::string MD5::hexdigest() const
{
	if( !finalized )
		return "";

	char buf[33];
	for( int i = 0; i<16; i++ )
		sprintf(buf + i * 2, "%02x", digest[i]);
	buf[32] = 0;

	return std::string(buf);
}

//////////////////////////////

std::ostream& operator<<(std::ostream& out, MD5 const& md5)
{
	return out << md5.hexdigest();
}

//////////////////////////////

