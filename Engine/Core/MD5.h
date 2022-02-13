#pragma once
#pragma once
#ifndef MD5_H_36CCD097_E4AA_4F7D_8EC1_032DA68843C6
#define MD5_H_36CCD097_E4AA_4F7D_8EC1_032DA68843C6

/* MD5
converted to C++ class by Frank Thilo (thilo@unix-ag.org)
for bzflag (http://www.bzflag.org)

based on:

md5.h and md5.c
reference implementation of RFC 1321

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


#include "Core/IntegerType.h"

#include <cstring>
#include <iostream>

// a small class for calculating MD5 hashes of strings or byte arrays
// it is not meant to be fast or secure
//
// usage: 1) feed it blocks of uchars with update()
//      2) finalize()
//      3) get hexdigest() string
//      or
//      MD5(std::string).hexdigest()
//
// assumes that char is 8 bit and int is 32 bit
class MD5
{
public:
	typedef uint32 size_type; // must be 32bit

	MD5();
	MD5(const std::string& text);
	void update(uint8 const buf[], size_type length);
	void update(int8 const buf[], size_type length);
	uint8 const* getResult() const { return digest; }
	MD5& finalize();
	std::string hexdigest() const;
	friend std::ostream& operator<<(std::ostream&, MD5 const& md5);

private:
	void init();
	typedef unsigned char uint8; //  8bit
	typedef unsigned int uint32;  // 32bit
	enum { blocksize = 64 }; // VC6 won't eat a const static int here

	void transform(const uint8 block[blocksize]);
	static void decode(uint32 output[], const uint8 input[], size_type len);
	static void encode(uint8 output[], const uint32 input[], size_type len);

	bool   finalized;
	uint8  buffer[blocksize]; // bytes that didn't fit in last 64 byte chunk
	uint32 count[2];   // 64bit counter for number of bits (lo, hi)
	uint32 state[4];   // digest so far
	uint8  digest[16]; // the result

};

#endif // MD5_H_36CCD097_E4AA_4F7D_8EC1_032DA68843C6