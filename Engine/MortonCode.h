#ifndef MortonCode_h__
#define MortonCode_h__

#include "Core/IntegerType.h"

class FMortonCode
{
public:
	static uint32 Encode(uint32 x, uint32 y){ return (Part1By1(y) << 1) + Part1By1(x); }
	static uint32 Encode(uint32 x, uint32 y, uint32 z){ return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x); }

	static uint32 Decode2D_X(uint32 code){  return Compact1By1(code >> 0);  }
	static uint32 Decode2D_Y(uint32 code){  return Compact1By1(code >> 1);  }

	static uint32 Decode3D_X(uint32 code){  return Compact1By2(code >> 0);  }
	static uint32 Decode3D_Y(uint32 code){  return Compact1By2(code >> 1);  }
	static uint32 Decode3D_Z(uint32 code){  return Compact1By2(code >> 2);  }

private:
	//http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
	// "Insert" a 0 bit after each of the 16 low bits of x
	static uint32 Part1By1(uint32 x)
	{
		x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
		x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
		x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
		x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
		x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
		return x;
	}

	// "Insert" two 0 bits after each of the 10 low bits of x
	static uint32 Part1By2(uint32 x)
	{
		x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
		x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
		x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
		x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
		x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
		return x;
	}


	// Inverse of Part1By1 - "delete" all odd-indexed bits
	static uint32 Compact1By1(uint32 x)
	{
		x &= 0x55555555;                  // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
		x = (x ^ (x >>  1)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
		x = (x ^ (x >>  2)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
		x = (x ^ (x >>  4)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
		x = (x ^ (x >>  8)) & 0x0000ffff; // x = ---- ---- ---- ---- fedc ba98 7654 3210
		return x;
	}

	// Inverse of Part1By2 - "delete" all bits not at positions divisible by 3
	static uint32 Compact1By2(uint32 x)
	{
		x &= 0x09249249;                  // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
		x = (x ^ (x >>  2)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
		x = (x ^ (x >>  4)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
		x = (x ^ (x >>  8)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
		x = (x ^ (x >> 16)) & 0x000003ff; // x = ---- ---- ---- ---- ---- --98 7654 3210
		return x;
	}


};





#endif // MortonCode_h__