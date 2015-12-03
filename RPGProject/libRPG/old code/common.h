#ifndef common_h__
#define common_h__

#ifdef WIN32
#pragma warning(disable : 4819)
#endif // WIN32

#include <Windows.h>
#include <cassert>
#include <map>
#include <vector>
#include <list>


#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "TPhyBody.h"


#include "CFlyCommon.h"
#include "CFVector3.h"
#include "CFTheFlyPort.h"


//#include "TheFlyWin32.h"
//#include "FyFX.h"

#include "debug.h"
#include "Holder.hpp"
#include "singleton.hpp"


#define  SAFE_DELETE( ptr )\
	delete ptr;  ptr = 0;

#ifdef _DEBUG
#define  UPCAST_PTR( type , ptr )\
	dynamic_cast< type* >( ptr )
#else
#define  UPCAST_PTR( type , ptr )\
	(( type*) ptr )
#endif

#ifdef  UNICODE 
#define TSTR( str ) L##str
#else
#define TSTR( str ) str
#endif


#ifdef WIN32
int TCheckHotKeyStatus(BYTE key );
#endif // WIN32

#define BIT(i) ( 1 << (i) )

template< class T , size_t N >
size_t array_size( T (&ar)[N] ){ return N; }
#define ARRAY_SIZE( var ) array_size( var )



typedef btCollisionShape  PhyShape;
typedef btCollisionObject PhyColObj;
typedef btRigidBody       PhyRigidBody;


typedef btScalar          Float;
typedef btVector3         Vec3D;
typedef btMatrix3x3       Mat3x3;
typedef btQuaternion      Quat;
typedef btTransform       XForm;

extern int g_ScreenWidth;
extern int g_ScreenHeight;

Float const PI = 3.1415926f;

extern Float const g_GraphicScale;
extern Vec3D const g_CamFaceDir;
extern Vec3D const g_ActorFaceDir;
extern Vec3D const g_UpAxisDir;

#define RAD2DEG( val )  ( (val / PI)* 180 )
#define DEG2RAD( val )  ( (val * PI)/ 180 )

#include <string>
typedef std::string TString;

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

template < class T >
inline T TMax( T a , T b )
{
	return ( a > b ) ? a : b; 
}

template < class T >
inline T TMin( T a , T b )
{
	return ( a > b ) ? b : a; 
}

template < class T >
inline T TClamp(T val , T min , T max )
{
	return TMax( min , TMin( val , max ) );
}


float TRandom( float s , float e );
int   TRandom();

struct GlobalVal
{
	GlobalVal(int fps);
	int    frameCount;
	int    tickCount;
	float  curtime;
	int    framePeerSec;
	float  frameTick;
	float  frameTime;
	float  nextTime;
	float  gravity;

	void   update();
};


extern GlobalVal g_GlobalVal;

#endif // common_h__