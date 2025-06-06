#ifndef CubeBase_h__
#define CubeBase_h__

#include "Core/IntegerType.h"

#include "Math/Vector3.h"
#include "Math/Quaternion.h"

#include "Math/TVector3.h"

#include <cmath>
#include <cassert>

namespace Cube
{
	using Vec3f = ::Math::Vector3;
	using Quat = ::Math::Quaternion;

	using Vec3i = TVector3<int>;
	using Vec2i = TVector2<int>;

	using BlockId = uint8;
	using ItemId = int16;

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

		COUNT,
	};

	inline FaceSide getFaceSide( int idxAxis , bool beN )
	{
		return FaceSide( 2 * idxAxis + ( beN ? 1 : 0 ) );
	}

	FORCEINLINE Vec3i GetFaceOffset(FaceSide face)
	{
		static constexpr Vec3i OffsetMap[] =
		{
			Vec3i(1,0,0),Vec3i(-1,0,0),Vec3i(0,1,0),Vec3i(0,-1,0),Vec3i(0,0,1),Vec3i(0,0,-1)
		};

		return OffsetMap[face];
	}

	enum class EDataState : uint8
	{
		Unintialized,
		Generating,
		Ok,
	};

}//namespace Cube

#endif // CubeBase_h__
