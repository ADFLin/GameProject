#ifndef ZDraw_h__
#define ZDraw_h__

#include "ZBase.h"

namespace Zuma
{
	class IRenderSystem;
	class ZObject;
	class ZConBall;
	class ZPath;
	class ZFrog;
	class ITexture2D;

	enum ToolProp;
	enum ResID;

	struct ZQuakeMap;
	struct ZLevelInfo;

	class ZRenderUtility
	{
	public:
		static bool init();
		static void drawAimArrow ( IRenderSystem& RDSystem , ZFrog const& frog , Vec2f const& aimPos , ZColor aimColor );
		static void drawQuakeMap ( IRenderSystem& RDSystem , ZQuakeMap& map , ITexture2D* texLvBG , Vec2f const pos[] );
		static void drawLevelMask( IRenderSystem& RDSystem , ZLevelInfo const& info );
		static void drawLevelBG  ( IRenderSystem& RDSystem , ZLevelInfo& info );
		static void drawPath     ( IRenderSystem& RDSystem , ZPath& path , float head , float step );
		static void drawHole     ( IRenderSystem& RDSystem , int frame );
		static void drawBall     ( IRenderSystem& RDSystem , ZConBall& ball );
		static void drawBall     ( IRenderSystem& RDSystem , ZColor color , int frame );
		static void drawBall     ( IRenderSystem& RDSystem , ZObject& ball , int frame , ToolProp prop );
	};

}//namespace Zuma

#endif // ZDraw_h__
