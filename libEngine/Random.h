#ifndef Random_h__
#define Random_h__

#include "Core/IntegerType.h"

namespace Random
{

	//http://stackoverflow.com/questions/1046714/what-is-a-good-random-number-generator-for-a-game
	class Well512
	{
	public:
		Well512()
		{
			for( int i = 0 ; i < 16 ; ++i )
				state[i] = i;
		}
		void init( uint32 s[16] )
		{
			index = 0;
			for( int i = 0 ; i < 16 ; ++i )
				state[i] = s[i];
		}

		uint32 rand()
		{
			unsigned long a, b, c, d;
			a = state[index];
			c = state[(index+13)&15];
			b = a^c^(a<<16)^(c<<15);
			c = state[(index+9)&15];
			c ^= (c>>11);
			a = state[index] = b^c;
			d = a^((a<<5)&0xDA442D20UL);
			index = (index + 15)&15;
			a = state[index];
			state[index] = a^b^d^(a<<2)^(b<<18)^(c<<28);
			return state[index];
		}

		int    index;
		uint32 state[16];
	};

	class LFSR113
	{
	public:

		void init( uint32 s[4] )
		{
			z1 = s[0] + (( s[0] > 1 ) ? 0 : ( 1 + 1 ));
			z2 = s[1] + (( s[1] > 7 ) ? 0 : ( 7 + 1 ));
			z3 = s[2] + (( s[2] > 15 ) ? 0 : ( 15 + 1 ));
			z4 = s[3] + (( s[3] > 127 ) ? 0 : ( 127 + 1 ));
		}

		uint32 rand() 
		{ /* Generates random 32 bit numbers.    */ 
			uint32 b; 
			b  = (((z1 << 6) ^ z1)   >> 13); 
			z1 = (((z1 & 4294967294) << 18) ^ b); 
			b  = (((z2 << 2) ^ z2)   >> 27); 
			z2 = (((z2 & 4294967288) <<  2) ^ b); 
			b  = (((z3 << 13) ^ z3)  >> 21); 
			z3 = (((z3 & 4294967280) <<  7) ^ b); 
			b  = (((z4 << 3) ^ z4)   >> 12); 
			z4 = (((z4 & 4294967168) << 13) ^ b); 
			return (z1 ^ z2 ^ z3 ^ z4); 
		}

		/* the state  */
		/* NOTE: the seed MUST satisfy 
		z1 > 1, z2 > 7, z3 > 15, and z4 > 127 */
		uint32 z1, z2, z3, z4;
	};

	class Microsoft
	{
	public:
		Microsoft( long seed ):mSeed( seed ){}
		void init( long seed ){ mSeed = seed; }
		int rand()
		{
			mSeed = mSeed * 214013 + 2531011;
			return ( mSeed >> 16) & 0x7fff; 
		}
	private:
		long mSeed;
	};


}//namespace Random



#endif // Random_h__
