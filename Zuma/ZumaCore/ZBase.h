#ifndef ZBase_h__
#define ZBase_h__

#include "TVector2.h"
#include "DebugSystem.h"
#include "FixString.h"

#include <memory>

namespace Zuma
{
	using std::tr1::shared_ptr;

	typedef TVector2< float > Vec2D;


	class IRenderSystem;
	class LevelManager;
	class ResManager;

	class Global
	{
	public:
		static IRenderSystem& getRenderSystem();
		static ResManager&    getResManager();
	};


	float const PI = 3.1415926589793f;

	int const g_ScreenWidth  = 640;
	int const g_ScreenHeight = 480;

	extern int DBG_X;
	extern int DBG_Y;

#define ARRAY_SIZE( AR ) ( sizeof(AR) / sizeof( AR[0]) )
#define BIT( n ) ( 1 << (n) )

	enum ZColor
	{
		zBlue   ,
		zYellow ,
		zRed    ,
		zGreen  ,
		zPurple ,
		zWhite  ,

		zColorNum ,
		zNull ,
	};


	struct GameTime
	{
		unsigned updateTime;
		unsigned curTime;
		unsigned nextTime;
	};

	extern GameTime g_GameTime;

	typedef unsigned char uint8;
	typedef unsigned uint32;

	struct ColorKey
	{
		union
		{
			uint32 value;
			struct
			{
				uint8 r;
				uint8 g;
				uint8 b;
				uint8 a;
			};
		};
	};


}//namespace Zuma

#endif // ZBase_h__
