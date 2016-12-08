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

	inline bool isInRange( Vec2i const& pos , Vec2i const& min , Vec2i const& max )
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

		template< unsigned BitNum >
		static int ToIndex( unsigned bit )
		{ 
#if TARGET_PLATFORM_64BITS
			if( BitNum > 32 )
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

		template< unsigned BitNum >
		static bool MaskIterator( unsigned& mask , int& index )
		{
			if ( mask == 0 )
				return false;
			unsigned bit = FBit::Extract( mask );
			index = FBit::ToIndex< BitNum >( bit );
			mask &= ~bit;
			return true;
		}

	};

	static Vec2i const gDirOffset[] =
	{
		Vec2i(1,0) , Vec2i(0,1) , Vec2i(-1,0) , Vec2i(0,-1)
	};

	static Vec2i const gNeighborOffset[] =
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
			return gNeighborOffset[ idx ];
		}
	};

	template< class T >
	bool AddUnique(std::vector< T >& v, T const& val)
	{
		for( int i = 0; i < v.size(); ++i )
		{
			if( v[i] == val )
				return false;
		}
		v.push_back(val);
		return true;
	}

	template < class T >
	bool IsValueUnique(std::vector< T > const& v)
	{
		int size = v.size();

		for( int i = 0; i < size; ++i )
		{
			for( int j = i+1; j < size; ++j )
			{
				if( v[i] == v[j] )
					return false;
			}
		}
		return true;
	}

	template< class T >
	void MakeValueUnique(std::vector< T >& v, int idxStart = 0)
	{
		int idxLast = v.size() - 1;
		for( int i = idxStart; i <= idxLast; ++i )
		{
			for( int j = 0; j < i; ++j )
			{
				if( v[i] == v[j] )
				{
					if( i != idxLast )
					{
						std::swap(v[i], v[idxLast]);
						--i;
					}
					--idxLast;
				}
			}
		}
		if( idxLast + 1 != v.size() )
		{
			v.resize(idxLast + 1);
		}

		assert(IsValueUnique(v));
	}


}//namespace CAR


#endif // CARCommon_h__2777082b_17b5_4cb7_991c_d4b39ae8fe90
