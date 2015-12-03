#ifndef MVCommon_h__
#define MVCommon_h__

#include "TVector3.h"
#include "TVector2.h"

#include "Math/Quaternion.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"


namespace MV
{
	typedef TVector3< int >  Vec3i;
	typedef TVector2< int >  Vec2i;

	typedef Math::Quaternion Quat;
	typedef Math::Matrix4    Mat4;

	class Vec3f : public Math::Vector3
	{
	public:
		Vec3f(){}
		Vec3f( Vec3i const& v ):Math::Vector3(v.x , v.y , v.z ){}
		Vec3f( float x , float y , float z ):Math::Vector3(x , y , z ){}
		Vec3f( Math::Vector3 const& v ):Math::Vector3(v.x , v.y , v.z ){}
	};


	float const Sqrt3 =  1.732050807568877f;
	float const Sqrt_2d3 = 0.8164965809277f; // sqrt( 2 / 3 );

	enum Dir
	{
		eDirX    = 0,
		eDirInvX = 1,
		eDirY    = 2,
		eDirInvY = 3,
		eDirZ    = 4,
		eDirInvZ = 5,
	};

	Dir const gDirInvertMap[6] = 
	{ 
		eDirInvX , eDirX , eDirInvY , eDirY , eDirInvZ , eDirZ 
	};

	Dir const gDirRotateMap[6][6] =
	{
		{ eDirX , eDirInvX , eDirZ , eDirInvZ , eDirInvY , eDirY } ,
		{ eDirX , eDirInvX , eDirInvZ , eDirZ , eDirY , eDirInvY } ,
		{ eDirInvZ , eDirZ , eDirY , eDirInvY , eDirX , eDirInvX } ,
		{ eDirZ , eDirInvZ , eDirY , eDirInvY , eDirInvX , eDirX } ,
		{ eDirY , eDirInvY , eDirInvX , eDirX , eDirZ , eDirInvZ } ,
		{ eDirInvY , eDirY , eDirX , eDirInvX , eDirZ , eDirInvZ } ,
	};

	int const gDirCrossMap[6][6] =
	{
		{ -1 , -1 , eDirZ , eDirInvZ , eDirInvY , eDirY } ,
		{ -1 , -1 , eDirInvZ , eDirZ , eDirY , eDirInvY } ,
		{ eDirInvZ , eDirZ , -1 , -1 , eDirX , eDirInvX } ,
		{ eDirZ , eDirInvZ , -1 , -1 , eDirInvX , eDirX } ,
		{ eDirY , eDirInvY , eDirInvX , eDirX , -1 , -1 } ,
		{ eDirInvY , eDirY , eDirX , eDirInvX , -1 , -1 } ,
	};

	int const gDirDotMap[6][6] =
	{
		1, -1, 0, 0, 0, 0,
		-1, 1, 0, 0, 0, 0,
		0, 0, 1, -1, 0, 0,
		0, 0, -1, 1, 0, 0,
		0, 0, 0, 0, 1, -1,
		0, 0, 0, 0, -1, 1, 
	};

	Dir const gDirNeighborMap[3][4] =
	{
		{ eDirY , eDirInvY , eDirZ , eDirInvZ } ,
		{ eDirZ , eDirInvZ , eDirX , eDirInvX } ,
		{ eDirX , eDirInvX , eDirY , eDirInvY } ,
	};

	int const gDirNeighborIndexMap[3][6] = 
	{
		{ -1 , -1 ,  0 ,  1 ,  2 ,  3 } ,
		{  2 ,  3 , -1 , -1 ,  0 ,  1 } ,
		{  0 ,  1 ,  2 ,  3 , -1 , -1 } ,
	};

	Vec3i const gParallaxDirMap[4] =
	{
		Vec3i(1,1,1),Vec3i( 1,-1,1),Vec3i(-1,-1,1),Vec3i(-1,1,1),
	};

	Vec3i const gDirOffsetMap[6] = 
	{ 
		Vec3i(1,0,0),Vec3i(-1,0,0),Vec3i(0,1,0),Vec3i(0,-1,0),Vec3i(0,0,1),Vec3i(0,0,-1) 
	};

	Vec3f const gDirOffsetMapF[6] = 
	{ 
		Vec3f(1,0,0),Vec3f(-1,0,0),Vec3f(0,1,0),Vec3f(0,-1,0),Vec3f(0,0,1),Vec3f(0,0,-1) 
	};


	struct FDir
	{
		static bool isPositive( Dir dir ){ return ( dir & 0x1 ) == 0; }
		static int  Axis( Dir dir ){  return dir / 2;  }
		static int  Cross( Dir dir1 , Dir dir2 ){ return gDirCrossMap[dir1][dir2]; }
		static int  Dot( Dir dir1 , Dir dir2 ){   return gDirDotMap[dir1][dir2];  }
		static Dir  Neighbor( Dir axis , int idx ){  return gDirNeighborMap[ axis / 2 ][ idx ];  }
		static int  NeighborIndex( Dir axis , Dir dir ){  return gDirNeighborIndexMap[ axis / 2 ][ dir ];  }
		static Dir  Inverse( Dir dir ){  return gDirInvertMap[ dir ];  }
		static Dir  Rotate( Dir axis , Dir dir , int factor );
		static Dir  Rotate( Dir axis , Dir dir ){  return gDirRotateMap[ axis ][ dir ];  }
		static Vec3i const& Offset( Dir dir ){  return gDirOffsetMap[dir]; }
		static Vec3f const& OffsetF( Dir dir ){  return gDirOffsetMapF[dir];  }
		static Vec3i const& ParallaxOffset( int idx )
		{ 
			assert( 0 <= idx && idx < 4 );
			return gParallaxDirMap[idx]; 
		}

	};

	Vec3i roatePos( Vec3i const& pos , Dir dir , Vec3i const& inPos );
	Vec3i roatePos( Vec3i const& pos , Dir dir , Vec3i const& inPos , int factor );
	Vec3f roatePos( Vec3i const& pos , Dir dir , Vec3f const& inPos );
	Vec3f roatePos( Vec3i const& pos , Dir dir , Vec3f const& inPos , int factor );

	class Roataion
	{
	public:
		Roataion(){}
		Roataion( Dir dirX , Dir dirZ ){ set( dirX , dirZ ); }

		void  set( Dir dirX , Dir dirZ );

		void  rotate( Dir axis );
		void  rotate( Dir axis , int factor );

		Dir   toWorld( Dir dir );
		Dir   toLocal( Dir dir );

		Dir   operator[] ( int idx ) const { return mDir[idx]; }
		Dir&  operator[] ( int idx ) { return mDir[idx]; }
  
		static Roataion Identity(){ return Roataion( eDirX , eDirZ ); }
	private:
		Dir   mDir[3];
	};


	class ObjectGroup;
	class IGroupModifier;
	class World;
	class BlockSurface;
	class Block;


}

#endif // MVCommon_h__
