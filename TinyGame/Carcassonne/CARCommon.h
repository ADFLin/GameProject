#ifndef CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90
#define CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90

#include "Math/TVector2.h"
#include "CppVersion.h"

#include "CARDefine.h"

#include "CompilerConfig.h"
#include "PlatformConfig.h"

#ifdef CPP_COMPILER_MSVC
#pragma warning( disable : 4482 )
#endif

#include <vector>
#include "StdUtility.h"
#include "BitUtility.h"

namespace CAR
{
	int const MaxPlayerNum = 6;
	unsigned const AllPlayerMask = 0xfffffff;

	class FeatureBase;
	class CityFeature;
	class FarmFeature;
	class RoadFeature;

	class PlayerBase;


	using Vec2i = TVector2<int>;

	inline bool IsInRect( Vec2i const& pos , Vec2i const& min , Vec2i const& max )
	{
		return min.x <= pos.x && pos.x <= max.x &&
			   min.y <= pos.y && pos.y <= max.y;
	}

	template< unsigned NumBits >
	struct TBitMaskIterator
	{
		unsigned mask;
		int      index;

		TBitMaskIterator(unsigned inMask)
			:mask(inMask)
			,index(-1)
		{

		}
		operator bool()
		{ 
			return FBitUtility::MaskIterator< NumBits >(mask, index);
		}
		void operator ++(int) {}
		void operator ++(){}
	};

	static Vec2i const gDirOffset[] =
	{
		Vec2i(1,0) , Vec2i(0,1) , Vec2i(-1,0) , Vec2i(0,-1)
	};

	static Vec2i const gAdjacentOffset[] =
	{
		Vec2i(1,0) , Vec2i(1,1) , Vec2i(0,1) , Vec2i(-1,1),
		Vec2i(-1,0) , Vec2i(-1,-1) , Vec2i(0,-1) , Vec2i(1,-1),
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

		static int const NeighborNum = 8;
		static Vec2i const& NeighborOffset( int idx )
		{
			return gAdjacentOffset[ idx ];
		}

		static Vec2i ToLocal(Vec2i const& offset, int rotation)
		{
			assert(0 <= rotation && rotation < TotalNum);
			switch( rotation )
			{
			case 1: return Vec2i(offset.y, -offset.x); 
			case 2: return -offset;
			case 3: return Vec2i(-offset.y, offset.x);
			}
			return offset;
		}

		static Vec2i ToWorld(Vec2i const& offset, int rotation)
		{
			assert(0 <= rotation && rotation < TotalNum);
			switch( rotation )
			{
			case 1: return Vec2i(-offset.y, offset.x);
			case 2: return -offset;
			case 3: return Vec2i( offset.y, -offset.x);
			}
			return offset;
		}
	};


}//namespace CAR


#endif // CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90
