#ifndef CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90
#define CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90

#include "TVector2.h"
#include "CppVersion.h"

#include "CARDefine.h"

#include "CompilerConfig.h"
#include "PlatformConfig.h"

#ifdef CPP_COMPILER_MSVC
#pragma warning( disable : 4482 )
#endif

#include <vector>
#include "StdUtility.h"

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

	inline bool IsInRect( Vec2i const& pos , Vec2i const& min , Vec2i const& max )
	{
		return min.x <= pos.x && pos.x <= max.x &&
			   min.y <= pos.y && pos.y <= max.y;
	}

	struct FBit
	{
		static unsigned Extract( unsigned bits ){ return bits & ~( bits - 1 );} 
		static int ToIndex4( unsigned bit );
		static int ToIndex8( unsigned bit );
		static int ToIndex32( unsigned bit );
#if TARGET_PLATFORM_64BITS
		static int ToIndex64( unsigned bit );
#endif
		static unsigned RotateRight(unsigned bits, unsigned offset, unsigned numBit);
		static unsigned RotateLeft( unsigned bits , unsigned offset , unsigned numBit );

		template< unsigned NumBits >
		static int ToIndex( unsigned bit )
		{ 
#if TARGET_PLATFORM_64BITS
			if( NumBits > 32 )
				return ToIndex64(bit);
			else
				return ToIndex32(bit);
#else
			return ToIndex32( bit ); 
#endif
		}
		template<>
		static int ToIndex<4>( unsigned bit ){ return ToIndex4( bit ); }
		template<>
		static int ToIndex<8>( unsigned bit ){ return ToIndex8( bit ); }

		template< unsigned NumBits >
		static bool MaskIterator( unsigned& mask , int& index )
		{
			if ( mask == 0 )
				return false;
			unsigned bit = FBit::Extract( mask );
			index = FBit::ToIndex< NumBits >( bit );
			mask &= ~bit;
			return true;
		}

	};

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
			return FBit::MaskIterator< NumBits >(mask, index);
		}
		void operator ++(int) {}
		void operator ++(void){}
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
