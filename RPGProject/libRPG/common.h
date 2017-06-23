#ifndef common_h__
#define common_h__

#define  USE_PROFILE
#include "ProfileSystem.h"
#define  TPROFILE  PROFILE_ENTRY

#include "TVector2.h"
typedef TVector2< int > Vec2i;

#include "CppVersion.h"
#include "LogSystem.h"
#include "CommonMarco.h"
#include "FastDelegate/FastDelegate.h"

#include <vector>
#include <list>
#include <map>

#include "CFVector3.h"
#include "CFMatrix4.h"
#include "CFQuaternion.h"
#include "CFMath.h"
#include "CFColor.h"

namespace CFly
{
	class World;    class Scene;     class Object; class Actor;
	class Material; class SceneNode; class Texture;
	class Sprite;   class Camera;    class Viewport; 
	class RenderWindow;
}

using CFly::Viewport;
using CFly::Material;
using CFly::SceneNode;
using CFly::Texture;
using CFly::Sprite;

typedef CFly::World  CFWorld;
typedef CFly::Scene  CFScene;
typedef CFly::Object CFObject;
typedef CFly::Actor  CFActor;
typedef CFly::Camera CFCamera;

using CFly::Color3ub;
using CFly::Color3f;
using CFly::Color4ub;
using CFly::Color4f;

typedef CFly::Vector3    Vec3D;
typedef CFly::Matrix4  Mat4x4;
typedef CFly::Quaternion Quat;

struct TEvent;

#include <string>
#ifdef _DEBUG
#define TASSERT( expr , str )\
	if ( !( expr ) ) { assert( str == 0 ); }
#else
#define TASSERT( expr , str )
#endif


#ifdef  UNICODE 
#define TSTR( str ) L##str
typedef std::wstring String;
#else
#define TSTR( str ) str
typedef std::string String;
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
float TRandomFloat();

extern Vec3D const g_CamFaceDir;
extern Vec3D const g_ActorFaceDir;
extern Vec3D const g_UpAxisDir;


class EntitySystem;
class GameObjectSystem;
class GameFramework;
class RenderSystem;
class ScriptSystem;
class IGameMod;
#include "WindowsHeader.h"


extern struct SGlobalEnv* gEnv;

struct GlobalVal
{
	GlobalVal(int fps);
	int    frameCount;
	int    tickCount;
	float  curtime;
	int    framePerSec;
	float  frameTick;
	float  frameTime;
	float  nextTime;

	void   updateFrame();
};

extern GlobalVal g_GlobalVal;

#define DESCRIBLE_CLASS( thisClass , baseClass )\
private:\
	typedef thisClass  ThisClass;\
	typedef baseClass  BaseClass;

#endif // common_h__