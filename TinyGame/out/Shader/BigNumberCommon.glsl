#pragma once

uint BUint_AddAssign(inout uint4 a, int uint4 b )
{
	uint carry = 0;
	UNROLL 
	for( uint i = 0; i < 4; ++i )
	{
		uint val = a[i];
		a[i] += b[i] + carry;
		carry = (b[i] <= val) ? 1 : 0;
	}
	return carry;
}

uint BUint_MulAssign(inout uint4 a, int uint4 b)
{
	uint carry = 0;
	UNROLL
	for( uint i = 0; i < 4; ++i )
	{
		uint val = a[i];
		a[i] += b[i] + carry;
		carry = (b[i] <= val) ? 1 : 0;
	}
	return carry;
}



uint BIntAdd(uint4 a, uint4 b)
{



}

uint BIntSingal(uint4 v)
{



}
