#ifndef CubeBase_h__
#define CubeBase_h__

#include "Math/Vector3.h"
#include "Math/Quaternion.h"

#include "Math/TVector3.h"

#include <cmath>
#include <cassert>

namespace Cube
{
	typedef ::Math::Vector3    Vec3f;
	typedef ::Math::Quaternion Quat;

	typedef TVector3< int >    Vec3i;

	typedef          __int32 int32;
	typedef          __int16 int16;
	typedef unsigned __int8  uint8;
	typedef unsigned __int32 uint32;
	typedef unsigned __int64 uint64;

	typedef uint8 BlockId;
	typedef int16 ItemId;

#define final

#define BLOCK_NULL BlockId( 0 ) 

#define BIT( N ) ( 1 << ( N ) )

	class World;
	class Block;
	class Item;
	class Entity;
	class Random;
	class RenderEngine;

	enum FaceSide
	{
		FACE_X  = 0,
		FACE_NX = 1,
		FACE_Y  = 2,
		FACE_NY = 3,
		FACE_Z  = 4,
		FACE_NZ = 5,
	};

	inline FaceSide getFaceSide( int idxAxis , bool beN )
	{
		return FaceSide( 2 * idxAxis + ( beN ? 1 : 0 ) );
	}


	class Math
	{
	public:
		static float floor( float value ){ return ::floorf( value );  }
		static float ceil( float value ) { return ::ceilf( value ); }
		static float cos( float value )  { return ::cosf( value ); }
		static float sin( float value )  { return ::sinf( value ); }
		static float abs( float value )  { return ::fabs( value ); }
	};

}//namespace Cube

#endif // CubeBase_h__
