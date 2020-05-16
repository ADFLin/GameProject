#pragma once
#ifndef Base_H_6E7C17E3_03D8_4D00_9413_F9F44FFB3DB6
#define Base_H_6E7C17E3_03D8_4D00_9413_F9F44FFB3DB6

#if 1
#include "LogSystem.h"
#define QA_LOG( ... ) LogMsg( __VA_ARGS__ )
#define QA_ERROR( ... ) LogMsg( __VA_ARGS__ )
#else
#define QA_LOG( ... )
#define QA_ERROR( ... )
#endif

#include "MathCore.h"
#include "Core/IntegerType.h"
#include <cmath>

#include <string>
typedef std::string String;

#include "FixString.h"
typedef FixString< 512 > FString;

#include "Holder.h"

#include "Rect.h"
#include "Core/Color.h"

typedef TRect< float > Rect;

namespace Priv
{
	struct ReleaseFree
	{
		template< class T >
		void operator()( T* ptr ){ ptr->release(); }
	};
}

template< class T >
class FObjectPtr : public TPtrReleaseFuncHolder< T , Priv::ReleaseFree >
{

};

template< class T >
class FPtr : public TPtrHolder< T >
{

};

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

typedef TVector2< int > Vec2i;

#define ARRAY_SIZE( ARRAY ) ( sizeof(ARRAY) / sizeof( ARRAY[0] ) )

float const TICK_TIME = 60 / 1000.0f;
int const BLOCK_SIZE = 64;

class Level;
class LevelObject;
class Light;
class ItemPickup;
class Bullet;
class Block;
class AreaTrigger;

class Texture;

enum RenderPass
{
	RP_DIFFUSE ,
	RP_NORMAL  ,
	RP_GLOW    ,
	NUM_RENDER_PASS ,
};

enum
{
	COL_PLAYER    = BIT(0) ,
	COL_SOILD     = BIT(1) ,
	COL_FLY_SOILD = BIT(2) ,
	COL_TRIGGER   = BIT(3) ,
	COL_BULLET    = BIT(4) ,
	COL_ITEM      = BIT(5) ,

	COL_TERRAIN   = BIT(12) ,
	COL_RENDER    = BIT(13) ,
	COL_VIEW      = BIT(14) , //For terrain

	COL_OBJECT    = COL_PLAYER | COL_SOILD | COL_FLY_SOILD | COL_TRIGGER | COL_BULLET | COL_ITEM ,
	COL_ALL       = 0xffffffff ,
};


#endif // Base_H_6E7C17E3_03D8_4D00_9413_F9F44FFB3DB6
