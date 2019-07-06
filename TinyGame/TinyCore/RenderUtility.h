#ifndef RenderUtility_h__
#define RenderUtility_h__

#include "DrawEngine.h"
#include "ColorName.h"

int const BlockSize = 18;

class GLGraphics2D;
namespace Render
{
	class RHIGraphics2D;
}

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
	static TINY_API void Initialize();
	static TINY_API void Finalize();

	static TINY_API Color3ub GetColor(int color, int type = COLOR_NORMAL);
	static TINY_API void SetPen( Graphics2D& g , int color , int type = COLOR_NORMAL );
	static TINY_API void SetBrush( Graphics2D& g , int color , int type = COLOR_NORMAL );
	static TINY_API void SetFont( Graphics2D& g , int fontID );
	static TINY_API void SetFontColor( Graphics2D& g , int color , int type = COLOR_NORMAL );

	static TINY_API void InitializeRHI();
	static TINY_API void ReleaseRHI();
	static TINY_API void SetPen( GLGraphics2D& g , int color , int type = COLOR_NORMAL );
	static TINY_API void SetBrush( GLGraphics2D& g , int color , int type = COLOR_NORMAL );
	static TINY_API void SetFont( GLGraphics2D& g , int fontID );
	static TINY_API void SetFontColor( GLGraphics2D& g , int color , int type = COLOR_NORMAL );

	static TINY_API void SetPen(Render::RHIGraphics2D& g, int color, int type = COLOR_NORMAL);
	static TINY_API void SetBrush(Render::RHIGraphics2D& g, int color, int type = COLOR_NORMAL);
	static TINY_API void SetFont(Render::RHIGraphics2D& g, int fontID);
	static TINY_API void SetFontColor(Render::RHIGraphics2D& g, int color, int type = COLOR_NORMAL);

	static TINY_API void SetPen( IGraphics2D& g , int color , int type = COLOR_NORMAL );
	static TINY_API void SetBrush( IGraphics2D& g , int color , int type = COLOR_NORMAL );
	static TINY_API void SetFont( IGraphics2D& g , int fontID );
	static TINY_API void SetFontColor( IGraphics2D& g , int color , int type = COLOR_NORMAL );

	template< class Graphics2D >
	static void DrawBlock( Graphics2D& g , Vec2i const& pos , Vec2i const& size , int color )
	{
		SetPen( g , EColor::Black );
		SetBrush( g , color , COLOR_DEEP );
		g.drawRoundRect( pos , size , Vec2i( 8 , 8 ) ); 

		SetPen( g , EColor::Null );
		SetBrush( g , color );
		g.drawRoundRect( 
			pos + Vec2i( 3 , 3 ) , Vec2i( size.x - 6 , size.y - 6 ) , Vec2i( 4 , 4 ) );
	}
	template< class Graphics2D >
	static void DrawBlock( Graphics2D& g ,Vec2i const& pos ,  int color )
	{
		DrawBlock( g , pos ,  Vec2i( BlockSize  , BlockSize  ) , color );
	}

	template< class Graphics2D >
	static void DrawBlock( Graphics2D& g ,Vec2i const& pos , int nw , int nh , int color )
	{
		DrawBlock( g , pos , BlockSize * Vec2i( nw , nh ) , color );
	}

	template< class Graphics2D >
	static void DrawCapsuleX( Graphics2D& g , Vec2i const& pos , Vec2i const& size )
	{
		g.drawRoundRect( pos , size , Vec2i( 2 * size.y / 3 , size.y ) );
	}
};


struct SimpleTextLayout
{
	template< class Graphics2D, class ...Args>
	FORCEINLINE void show(Graphics2D& g, char const* format, Args&& ...args)
	{
		FixString< 512 > str;
		str.format(format, args...);
		g.drawText(Vector2( posX, posY ), str);
		posY += offset;
	}
	template< class Graphics2D, class ...Args>
	FORCEINLINE void show(Graphics2D& g, char const* str)
	{
		g.drawText(Vector2(posX, posY), str);
		posY += offset;
	}
	int posX = 100;
	int posY = 10;
	int offset = 15;
};



#endif // RenderUtility_h__