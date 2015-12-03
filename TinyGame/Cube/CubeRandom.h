#ifndef CubeRandom_h__
#define CubeRandom_h__


namespace Cube
{

	class Random
	{
	public:
		Random()
		{
			setSeed( 0 );
		}
		void   setSeed( uint64 seed )
		{
			mSeed = ( seed ^ uint64( 0x5DEECE66D ) ) & ( (uint64(1) << 48)  - 1 );
		}

		uint32 getInt()
		{ 
			return uint32( 32 * uint64( next(31) ) >> 31 );
		}

	private:
		uint32 next(int bits) 
		{
			mSeed = ( mSeed * uint64( 0x5DEECE66D ) + 0xB ) & ( (uint64(1) << 48)  - 1 );
			return uint32( mSeed >> (48 - bits) );
		}
		uint64 mSeed;
	};


}//namespace Cube

#endif // CubeRandom_h__
