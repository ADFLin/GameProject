#ifndef GameGlobal_h__
#define GameGlobal_h__

#include "GameShare.h"

#include "Math/TVector2.h"
#include "Math/Vector2.h"
#include "InlineString.h"
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

#define DEG2RAD( deg ) Math::DegToRad( deg )
#define RAD2DEG( rad ) Math::RadToDeg( rad )

class DrawEngine;
class Graphics2D;
class IGraphics2D;
class RHIGraphics2D;

class GameModuleManager;
class AssetManager;
class IGameInstance;
class PropertySet;
class GUISystem;
struct UserProfile;
class NetWorker;
class DataCacheInterface;

class IEditor;

TINY_API uint64 GenerateRandSeed();



class IGameNetInterface
{
public:
	~IGameNetInterface(){}
	virtual bool        haveServer() = 0;
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
	static TINY_API void Initialize();
	static TINY_API void Finalize();

	static TINY_API int  RandomNet();
	static TINY_API void RandSeedNet( uint64 seed );
	static TINY_API int  Random();
	static TINY_API void RandSeed(unsigned seed );


	static TINY_API Vec2i GetScreenSize();

	static TINY_API GameModuleManager& ModuleManager();
	static TINY_API AssetManager&      GetAssetManager();

	static TINY_API IGameInstance*     GameInstacne();
	static TINY_API IEditor*           Editor();

	static TINY_API IGameNetInterface&   GameNet();
	static TINY_API IDebugInterface&     Debug();
	static TINY_API PropertySet&         GameConfig();

	static TINY_API GUISystem&           GUI();
	
	static TINY_API DrawEngine&    GetDrawEngine();
	static TINY_API Graphics2D&    GetGraphics2D();
	static TINY_API RHIGraphics2D& GetRHIGraphics2D();
	static TINY_API IGraphics2D&   GetIGraphics2D();

	static TINY_API UserProfile&  GetUserProfile();

	static TINY_API DataCacheInterface&  DataCache();

};



#endif // GameGlobal_h__