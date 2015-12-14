#ifndef RenderUtility_h__
#define RenderUtility_h__

#include "DrawEngine.h"

int const BlockSize = 18;

class GLGraphics2D;

struct Color
{
	enum ColorEnum
	{
		eNull = 0,
		eCyan ,
		eBlue ,
		eOrange ,
		eYellow,
		eGreen,
		ePurple,
		eRed,
		ePink  ,
		eGray ,
		eWhite ,
		eBlack ,
		Number ,
	};
};

enum
{
	COLOR_NORMAL = 0,
	COLOR_LIGHT  = 1,
	COLOR_DEEP   = 2,
};


enum 
{

	FONT_S24 = 0,
	FONT_S12 ,
	FONT_S16 ,
	FONT_S10 ,
	FONT_S8 ,

	FONT_NUM ,
};

class RenderUtility
{
public:
	static GAME_API void init();
	static GAME_API void release();

	static GAME_API void setPen( Graphics2D& g , int color , int type = COLOR_NORMAL );
	static GAME_API void setBrush( Graphics2D& g , int color , int type = COLOR_NORMAL );
	static GAME_API void setFont( Graphics2D& g , int fontID );
	static GAME_API void setFontColor( Graphics2D& g , int color , int type = COLOR_NORMAL );

	static GAME_API void startOpenGL();
	static GAME_API void stopOpenGL();
	static GAME_API void setPen( GLGraphics2D& g , int color , int type = COLOR_NORMAL );
	static GAME_API void setBrush( GLGraphics2D& g , int color , int type = COLOR_NORMAL );
	static GAME_API void setFont( GLGraphics2D& g , int fontID );
	static GAME_API void setFontColor( GLGraphics2D& g , int color , int type = COLOR_NORMAL );

	static GAME_API void setPen( IGraphics2D& g , int color , int type = COLOR_NORMAL );
	static GAME_API void setBrush( IGraphics2D& g , int color , int type = COLOR_NORMAL );
	static GAME_API void setFont( IGraphics2D& g , int fontID );
	static GAME_API void setFontColor( IGraphics2D& g , int color , int type = COLOR_NORMAL );

	template< class Graphics2D >
	static void drawBlock( Graphics2D& g , Vec2i const& pos , Vec2i const& size , int color )
	{
		setPen( g , Color::eBlack );
		setBrush( g , color , COLOR_DEEP );
		g.drawRoundRect( pos , size , Vec2i( 8 , 8 ) ); 

		setPen( g , Color::eNull );
		setBrush( g , color );
		g.drawRoundRect( 
			pos + Vec2i( 3 , 3 ) , Vec2i( size.x - 6 , size.y - 6 ) , Vec2i( 4 , 4 ) );
	}
	template< class Graphics2D >
	static void drawBlock( Graphics2D& g ,Vec2i const& pos ,  int color )
	{
		drawBlock( g , pos ,  Vec2i( BlockSize  , BlockSize  ) , color );
	}

	template< class Graphics2D >
	static void drawBlock( Graphics2D& g ,Vec2i const& pos , int nw , int nh , int color )
	{
		drawBlock( g , pos , BlockSize * Vec2i( nw , nh ) , color );
	}

	template< class Graphics2D >
	static void drawCapsuleX( Graphics2D& g , Vec2i const& pos , Vec2i const& size )
	{
		g.drawRoundRect( pos , size , Vec2i( 2 * size.y / 3 , size.y ) );
	}
};


#endif // RenderUtility_h__