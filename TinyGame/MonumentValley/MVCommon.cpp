#include "MVCommon.h"

namespace MV
{
	Dir FDir::Rotate(Dir axis , Dir dir , int factor)
	{
		assert( 0 <= factor && factor < 4 );
		switch( factor )
		{
		case 0: return dir;
		case 1: return gDirRotateMap[ axis ][ dir ];
		case 2: return ( FDir::Axis( axis ) == FDir::Axis( dir ) ) ? dir : FDir::Inverse( dir );
		case 3: return gDirRotateMap[ FDir::Inverse( axis ) ][ dir ];
		}
		return dir;
	}

	template< class T >
	T roatePosT( Vec3i const& pos , Dir dir , T const& inPos )
	{
		T offset = inPos - T(pos);
		switch( dir )
		{
		case eDirX:    std::swap( offset.y , offset.z ); offset.y *= -1; break;
		case eDirInvX: std::swap( offset.y , offset.z ); offset.z *= -1; break;
		case eDirY:    std::swap( offset.z , offset.x ); offset.z *= -1; break;
		case eDirInvY: std::swap( offset.z , offset.x ); offset.x *= -1; break;
		case eDirZ:    std::swap( offset.x , offset.y ); offset.x *= -1; break;
		case eDirInvZ: std::swap( offset.x , offset.y ); offset.y *= -1; break;
		}
		return T(pos) + offset;
	}

	template< class T >
	T roatePosT(Vec3i const& pos , Dir dir , T const& inPos , int factor)
	{
		assert( 0 <= factor && factor < 4 );
		if ( factor == 0 )
			return inPos;

		if ( factor == 2 )
		{
			T offset = inPos - T(pos);
			switch( dir )
			{
			case eDirX: case eDirInvX: offset.y = -offset.y; offset.z = -offset.z; break;
			case eDirY: case eDirInvY: offset.z = -offset.z; offset.x = -offset.x; break;
			case eDirZ: case eDirInvZ: offset.x = -offset.x; offset.y = -offset.y; break;
			}
			return T(pos) + offset;
		}

		if ( factor == 3 )
			dir = FDir::Inverse( dir );

		return roatePosT( pos , dir , inPos );
	}

	Vec3i roatePos( Vec3i const& pos , Dir dir , Vec3i const& inPos ){ return roatePosT( pos , dir , inPos ); }
	Vec3i roatePos( Vec3i const& pos , Dir dir , Vec3i const& inPos , int factor ){ return roatePosT( pos , dir , inPos , factor ); }
	Vec3f roatePos( Vec3i const& pos , Dir dir , Vec3f const& inPos ){ return roatePosT( pos , dir , inPos ); }
	Vec3f roatePos( Vec3i const& pos , Dir dir , Vec3f const& inPos , int factor ){ return roatePosT( pos , dir , inPos , factor ); }

	void Roataion::rotate( Dir axis , int factor )
	{
		mDir[0] = FDir::Rotate( axis , mDir[0] , factor );
		mDir[1] = FDir::Rotate( axis , mDir[1] , factor );
		mDir[2] = Dir( FDir::Cross( mDir[0] , mDir[1] ) ); 
		assert( mDir[2] != -1 );

	}

	void Roataion::rotate( Dir axis )
	{
		mDir[0] = FDir::Rotate( axis , mDir[0] );
		mDir[1] = FDir::Rotate( axis , mDir[1] );
		mDir[2] = Dir( FDir::Cross( mDir[0] , mDir[1] ) );
		assert( mDir[2] != -1 );
	}

	void Roataion::set(Dir dirX , Dir dirZ)
	{
		int dirCross = FDir::Cross( dirZ , dirX );
		assert( dirCross != -1 );
		mDir[0] = dirX;
		mDir[1] = Dir( dirCross );
		mDir[2] = dirZ;
	}

	Dir Roataion::toWorld(Dir dir)
	{
		Dir outDir = mDir[ dir / 2 ];
		if ( !FDir::isPositive( dir ) )
			outDir = FDir::Inverse( outDir );
		return outDir;
	}

	Dir Roataion::toLocal(Dir dir)
	{
		//TODO: improve
		int axis = FDir::Axis( dir );
		int idx = ( axis == FDir::Axis( mDir[2] ) ) ? 2 :
			( ( axis == FDir::Axis( mDir[1] ) ) ? 1 : 0 ) ;
		assert( axis == FDir::Axis( mDir[idx] ) );
		return Dir( 2 * idx + (( dir == mDir[idx] ) ? 0 : 1 ) );
	}


}//namespace MV