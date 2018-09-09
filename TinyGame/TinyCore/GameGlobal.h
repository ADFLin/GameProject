#ifndef GameGlobal_h__
#define GameGlobal_h__

#include "GameConfig.h"
#include "GameShare.h"

#include "TVector2.h"
#include "Math/Vector2.h"
#include "FixString.h"
#include "Core/IntegerType.h"
#include "MarcoCommon.h"
#include "TemplateMisc.h"

#define USE_TRANSLATE
#include "Localization.h"

typedef TVector2< int >  Vec2i;

using ::Math::Vector2;

int const MAX_PLAYER_NUM   = 32;
int const gDefaultTickTime = 15;
int const gDefaultGameFPS  = 1000 / gDefaultTickTime;

int const gDefaultScreenWidth  = 800;
int const gDefaultScreenHeight = 600;

float const PI = 3.141592653589793238462643383279f;
#define DEG2RAD( deg ) float( deg * PI / 180.0f )
#define RAD2DEG( rad ) float( rad * 180.0f / PI )

#define COMBINE_HEX(n) 0x##n##LU

/* 8-bit conversion function */
#define HEX_TO_BINARY8( n ) \
	( ( n & 0x0000000fLU)?  1:0 ) + \
	( ( n & 0x000000f0LU)?  2:0 ) + \
	( ( n & 0x00000f00LU)?  4:0 ) + \
	( ( n & 0x0000f000LU)?  8:0 ) + \
	( ( n & 0x000f0000LU)? 16:0 ) + \
	( ( n & 0x00f00000LU)? 32:0 ) + \
	( ( n & 0x0f000000LU)? 64:0 ) + \
	( ( n & 0xf0000000LU)?128:0 )

#define BINARY8(d) ( (unsigned)HEX_TO_BINARY8( COMBINE_HEX(d) ) )
#define BINARY32( a , b , c , d ) ( ( BINARY8( a ) << 24) | (BINARY8( b ) << 16) | (BINARY8( c ) << 8) | BINARY8( d ))
#define ARRAY2BIT4( a , b , c , d ) ( (BINARY8(a)<<12)|(BINARY8(b)<<8)|(BINARY8(c)<<4)|BINARY8(d) ) 
#define ARRAY2BIT3( a , b , c )     ( (BINARY8(a)<< 6)|(BINARY8(b)<<3)|BINARY8(c) )


class DrawEngine;
class Graphics2D;
class IGraphics2D;
class GLGraphics2D;

class GameModuleManager;
class AssetManager;
class IGameInstance;
class PropertyKey;
class GUISystem;
struct UserProfile;
class NetWorker;

TINY_API uint64 generateRandSeed();

TINY_API bool IsInGameThead();

class IGameNetInterface
{
public:
	~IGameNetInterface(){}
	virtual NetWorker*  getNetWorker() = 0;
	virtual NetWorker*  buildNetwork(bool beServer) = 0;
	virtual void        closeNetwork() = 0;
};

class IDebugInterface
{
public:
	~IDebugInterface(){}
	virtual void clearDebugMsg() = 0;
};

class Global
{
public:
	static TINY_API int  RandomNet();
	static TINY_API void RandSeedNet( uint64 seed );
	static TINY_API int  Random();
	static TINY_API void RandSeed(unsigned seed );

	static TINY_API GameModuleManager& GameManager();
	static TINY_API AssetManager&      GetAssetManager();

	static TINY_API IGameInstance*     GameInstacne();


	static TINY_API IGameNetInterface&   GameNet();
	static TINY_API IDebugInterface&     Debug();
	static TINY_API PropertyKey&         GameConfig();
	static TINY_API GUISystem&           GUI();
	
	static TINY_API DrawEngine*   GetDrawEngine();
	static TINY_API Graphics2D&   GetGraphics2D();
	static TINY_API GLGraphics2D& GetRHIGraphics2D();
	static TINY_API IGraphics2D&  GetIGraphics2D();

	static TINY_API UserProfile&  GetUserProfile();

};



#endif // GameGlobal_h__