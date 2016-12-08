#ifndef GameGlobal_h__
#define GameGlobal_h__

#include "GameConfig.h"
#include "CoreShare.h"

#include "TVector2.h"
#include "FixString.h"
#include "IntegerType.h"

#define USE_TRANSLATE
#include "Localization.h"

typedef TVector2< int >  Vec2i;
typedef TVector2< float >  Vec2f;

int const MAX_PLAYER_NUM   = 32;
int const gDefaultTickTime = 15;
int const gDefaultGameFPS  = 1000 / gDefaultTickTime;

int const gDefaultScreenWidth  = 800;
int const gDefaultScreenHeight = 600;

float const PI = 3.141592653589793238462643383279f;
#define DEG2RAD( deg ) float( deg * PI / 180.0f )
#define RAD2DEG( rad ) float( rad * 180.0f / PI )

#define ARRAY_SIZE( ar ) ( sizeof(ar)/sizeof(ar[0]) )

#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */
#define B8__( n ) \
	( ( n & 0x0000000fLU)?  1:0 ) + \
	( ( n & 0x000000f0LU)?  2:0 ) + \
	( ( n & 0x00000f00LU)?  4:0 ) + \
	( ( n & 0x0000f000LU)?  8:0 ) + \
	( ( n & 0x000f0000LU)? 16:0 ) + \
	( ( n & 0x00f00000LU)? 32:0 ) + \
	( ( n & 0x0f000000LU)? 64:0 ) + \
	( ( n & 0xf0000000LU)?128:0 )

#define BINARY(d) ( (unsigned)B8__( HEX__(d) ) )
#define BINARY32( a , b , c , d ) ( ( BINARY( a ) << 24) | (BINARY( b ) << 16) | (BINARY( c ) << 8) | BINARY( d ))
#define ARRAY2BIT4( a , b , c , d ) ( (BINARY(a)<<12)|(BINARY(b)<<8)|(BINARY(c)<<4)|BINARY(d) ) 
#define ARRAY2BIT3( a , b , c )     ( (BINARY(a)<< 6)|(BINARY(b)<<3)|BINARY(c) )

#ifndef BIT
#define BIT(i) ( 1 << (i) )
#endif



class DrawEngine;
class Graphics2D;
class IGraphics2D;
class GLGraphics2D;

class GamePackageManager;
class PropertyKey;
class GUISystem;
struct UserProfile;
class NetWorker;

GAME_API uint64 generateRandSeed();

extern GAME_API uint32 gGameThreadId;
GAME_API bool IsInGameThead();

class IGameNetInterface
{
public:
	~IGameNetInterface(){}
	virtual NetWorker*  getNetWorker() = 0;
	virtual NetWorker*  buildNetwork(bool beServer) = 0;
	virtual void        closeNetwork() = 0;
};

class Global
{
public:
	static GAME_API int  RandomNet();
	static GAME_API void RandSeedNet( uint64 seed );
	static GAME_API int  Random();
	static GAME_API void RandSeed(unsigned seed );

	static GAME_API GamePackageManager& GameManager();
	static GAME_API IGameNetInterface&  GameNet();
	static GAME_API PropertyKey&        GameSetting();
	static GAME_API GUISystem&          GUI();
	
	static GAME_API DrawEngine*   getDrawEngine();
	static GAME_API Graphics2D&   getGraphics2D();
	static GAME_API GLGraphics2D& getGLGraphics2D();
	static GAME_API IGraphics2D&  getIGraphics2D();

	static GAME_API UserProfile&  getUserProfile();
	
};

class UnCopiable
{
public:
	UnCopiable(){}
private:
	UnCopiable( UnCopiable const& rhs ){}
};

template< class T >
class TRef
{
public:
	explicit TRef( T& val ):mValue( val ){}
	operator T&      ()       { return mValue; }
	operator T const&() const { return mValue; }
private:
	T& mValue;
};

template< class T >
class TConstRef
{
public:
	explicit TConstRef( T const& val ):mValue( val ){}
	operator T const&() const { return mValue; }

private:
	T const& mValue;
};

template< class T >
TRef< T >      ref( T& val ){ return TRef< T >( val );   }
template< class T >
TConstRef< T > ref( T const& val ){ return TConstRef< T >( val );   }


#endif // GameGlobal_h__