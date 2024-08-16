#ifndef MVCommon_h__
#define MVCommon_h__

#include "Math/TVector3.h"
#include "Math/TVector2.h"

#include "Math/Quaternion.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"


namespace MV
{
	using Vec3i = TVector3< int >;
	using Vec2i = TVector2< int >;

	using Quat = Math::Quaternion;
	using Mat4 = Math::Matrix4;

	using Vec2f = Math::Vector2;
	using Vec3f = Math::Vector3;

	float constexpr Sqrt3 =  1.732050807568877f;
	float constexpr Sqrt_2d3 = 0.8164965809277f; // sqrt( 2 / 3 );

	enum Dir
	{
		X    = 0,
		InvX = 1,
		Y    = 2,
		InvY = 3,
		Z    = 4,
		InvZ = 5,
	};

	Dir constexpr gDirInvertMap[6] =
	{ 
		Dir::InvX , Dir::X , Dir::InvY , Dir::Y , Dir::InvZ , Dir::Z 
	};

	Dir constexpr gDirRotateMap[6][6] =
	{
		{ Dir::X , Dir::InvX , Dir::Z , Dir::InvZ , Dir::InvY , Dir::Y } ,
		{ Dir::X , Dir::InvX , Dir::InvZ , Dir::Z , Dir::Y , Dir::InvY } ,
		{ Dir::InvZ , Dir::Z , Dir::Y , Dir::InvY , Dir::X , Dir::InvX } ,
		{ Dir::Z , Dir::InvZ , Dir::Y , Dir::InvY , Dir::InvX , Dir::X } ,
		{ Dir::Y , Dir::InvY , Dir::InvX , Dir::X , Dir::Z , Dir::InvZ } ,
		{ Dir::InvY , Dir::Y , Dir::X , Dir::InvX , Dir::Z , Dir::InvZ } ,
	};

	int constexpr gDirCrossMap[6][6] =
	{
		{ -1 , -1 , Dir::Z , Dir::InvZ , Dir::InvY , Dir::Y } ,
		{ -1 , -1 , Dir::InvZ , Dir::Z , Dir::Y , Dir::InvY } ,
		{ Dir::InvZ , Dir::Z , -1 , -1 , Dir::X , Dir::InvX } ,
		{ Dir::Z , Dir::InvZ , -1 , -1 , Dir::InvX , Dir::X } ,
		{ Dir::Y , Dir::InvY , Dir::InvX , Dir::X , -1 , -1 } ,
		{ Dir::InvY , Dir::Y , Dir::X , Dir::InvX , -1 , -1 } ,
	};

	int constexpr gDirDotMap[6][6] =
	{
		1, -1, 0, 0, 0, 0,
		-1, 1, 0, 0, 0, 0,
		0, 0, 1, -1, 0, 0,
		0, 0, -1, 1, 0, 0,
		0, 0, 0, 0, 1, -1,
		0, 0, 0, 0, -1, 1, 
	};

	Dir constexpr gDirNeighborMap[3][4] =
	{
		{ Dir::Y , Dir::InvY , Dir::Z , Dir::InvZ } ,
		{ Dir::Z , Dir::InvZ , Dir::X , Dir::InvX } ,
		{ Dir::X , Dir::InvX , Dir::Y , Dir::InvY } ,
	};

	int constexpr gDirNeighborIndexMap[3][6] = 
	{
		{ -1 , -1 ,  0 ,  1 ,  2 ,  3 } ,
		{  2 ,  3 , -1 , -1 ,  0 ,  1 } ,
		{  0 ,  1 ,  2 ,  3 , -1 , -1 } ,
	};

	Vec3i constexpr gParallaxDirMap[4] =
	{
		Vec3i(1,1,1),Vec3i( 1,-1,1),Vec3i(-1,-1,1),Vec3i(-1,1,1),
	};

	Vec3i constexpr gDirOffsetMap[6] =
	{ 
		Vec3i(1,0,0),Vec3i(-1,0,0),Vec3i(0,1,0),Vec3i(0,-1,0),Vec3i(0,0,1),Vec3i(0,0,-1) 
	};

	Vec3f constexpr gDirOffsetMapF[6] =
	{ 
		Vec3f(1,0,0),Vec3f(-1,0,0),Vec3f(0,1,0),Vec3f(0,-1,0),Vec3f(0,0,1),Vec3f(0,0,-1) 
	};


	struct FDir
	{
		static bool IsPositive( Dir dir ){ return ( dir & 0x1 ) == 0; }
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

	Vec3i RotatePos( Vec3i const& pos , Dir dir , Vec3i const& inPos );
	Vec3i RotatePos( Vec3i const& pos , Dir dir , Vec3i const& inPos , int factor );
	Vec3f RotatePos( Vec3i const& pos , Dir dir , Vec3f const& inPos );
	Vec3f RotatePos( Vec3i const& pos , Dir dir , Vec3f const& inPos , int factor );

	class AxisRoataion
	{
	public:
		AxisRoataion(){}
		AxisRoataion( Dir dirX , Dir dirZ ){ set( dirX , dirZ ); }

		void  set( Dir dirX , Dir dirZ );

		void  rotate( Dir axis );
		void  rotate( Dir axis , int factor );

		Dir   toWorld( Dir dir );
		Dir   toLocal( Dir dir );

		Dir   operator[] ( int idx ) const { return mDir[idx]; }
		Dir&  operator[] ( int idx ) { return mDir[idx]; }
  
		static AxisRoataion Identity(){ return AxisRoataion( Dir::X , Dir::Z ); }
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
