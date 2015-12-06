#ifndef CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90
#define CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90

#include "TVector2.h"
#include "CppVersion.h"

#include "CARDefine.h"

namespace CAR
{
	int const MaxPlayerNum = 6;
	unsigned const AllPlayerMask = 0xfffffff;

	class FeatureBase;
	class CityFeature;
	class FarmFeature;
	class RoadFeature;

	class PlayerBase;


	typedef TVector2<int> Vec2i;
	struct FBit
	{
		static unsigned Extract( unsigned bits ){ return bits & ~( bits - 1 );} 
		static int ToIndex4( unsigned bit );
		static int ToIndex8( unsigned bit );
		static unsigned RotateRight( unsigned bits , unsigned offset , unsigned numBit );
		static unsigned RotateLeft( unsigned bits , unsigned offset , unsigned numBit );
		static bool MaskIterator4( unsigned& mask , int& index )
		{
			if ( mask == 0 )
				return false;
			unsigned bit = FBit::Extract( mask );
			index = FBit::ToIndex4( bit );
			mask &= ~bit;
			return true;
		}
		static bool MaskIterator8( unsigned& mask , int& index )
		{
			if ( mask == 0 )
				return false;
			unsigned bit = FBit::Extract( mask );
			index = FBit::ToIndex8( bit );
			mask &= ~bit;
			return true;
		}
	};

	static Vec2i const gDirOffset[] =
	{
		Vec2i(1,0) , Vec2i(0,1) , Vec2i(-1,0) , Vec2i(0,-1)
	};

	struct FDir
	{
		static int const TotalNum = 4;
		static int Inverse( int dir ){ return ( dir + TotalNum / 2 ) % TotalNum; }
		static int ToLocal( int dir , int rotation )
		{
			assert( 0 <= rotation && rotation < TotalNum );
			return ( dir - rotation + TotalNum ) % TotalNum;
		}

		static int ToWorld( int lDir , int rotation )		
		{
			assert( 0 <= rotation && rotation < TotalNum );
			return ( lDir + rotation ) % TotalNum;
		}

		static Vec2i const& LinkOffset( int dir )
		{
			return gDirOffset[dir];
		}

		static Vec2i LinkPos( Vec2i const& pos , int dir )
		{
			return pos + gDirOffset[dir];
		}
	};


}//namespace CAR


#endif // CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90
