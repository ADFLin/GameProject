#pragma once
#ifndef RenderUtility_H_7526E438_527C_4167_BD98_6F3E8A83A367
#define RenderUtility_H_7526E438_527C_4167_BD98_6F3E8A83A367

#include "ColorName.h"
#include "GameConfig.h"
#include "Core/Color.h"
#include "Math/TVector2.h"
#include "Renderer/RenderTransform2D.h"

int const BlockSize = 18;

class Graphics2D;
class RHIGraphics2D;
class IGraphics2D;

typedef TVector2< int >  Vec2i;

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

#define ADD_PEN_WIDTH 0

namespace  Render
{
	class FontDrawer;
}

class RenderUtility
{
public:
	static TINY_API void Initialize();
	static TINY_API void Finalize();

	static TINY_API Render::FontDrawer& GetFontDrawer(int fontID);

	static TINY_API Color3ub GetColor(int color, int type = COLOR_NORMAL);
#if ADD_PEN_WIDTH
	static TINY_API void SetPen(Graphics2D& g, int color, int type = COLOR_NORMAL, int width = 1);
#else
	static TINY_API void SetPen(Graphics2D& g, int color, int type = COLOR_NORMAL);
#endif
	static TINY_API void SetBrush( Graphics2D& g , int color , int type = COLOR_NORMAL );
	static TINY_API void SetFont( Graphics2D& g , int fontID );
	static TINY_API void SetFontColor( Graphics2D& g , int color , int type = COLOR_NORMAL );

	static TINY_API void InitializeRHI();
	static TINY_API void ReleaseRHI();

#if ADD_PEN_WIDTH
	static TINY_API void SetPen( RHIGraphics2D& g , int color , int type = COLOR_NORMAL, int width = 1);
#else
	static TINY_API void SetPen(RHIGraphics2D& g, int color, int type = COLOR_NORMAL);
#endif
	static TINY_API void SetBrush( RHIGraphics2D& g , int color , int type = COLOR_NORMAL );
	static TINY_API void SetFont( RHIGraphics2D& g , int fontID );
	static TINY_API void SetFontColor( RHIGraphics2D& g , int color , int type = COLOR_NORMAL );
#if ADD_PEN_WIDTH
	static TINY_API void SetPen(IGraphics2D& g, int color, int type = COLOR_NORMAL, int width = 1);
#else
	static TINY_API void SetPen(IGraphics2D& g, int color, int type = COLOR_NORMAL);
#endif
	static TINY_API void SetBrush( IGraphics2D& g, int color, int type = COLOR_NORMAL);
	static TINY_API void SetFont( IGraphics2D& g , int fontID );
	static TINY_API void SetFontColor( IGraphics2D& g , int color , int type = COLOR_NORMAL );

	template< class TGraphics2D >
	static void DrawBlock(TGraphics2D& g, Vec2i const& pos, Vec2i const& size, int color, int outlineColor = EColor::Black)
	{
		SetPen(g, outlineColor);
		SetBrush(g, color, COLOR_DEEP);
		g.drawRoundRect(pos, size, Vec2i(8, 8));

		SetPen(g, EColor::Null);
		SetBrush(g, color);
		g.drawRoundRect(pos + Vec2i(3, 3), Vec2i(size.x - 6, size.y - 6), Vec2i(4, 4));
	}
	template< class TGraphics2D >
	static void DrawBlock(TGraphics2D& g ,Vec2i const& pos , int color, int outlineColor = EColor::Black)
	{
		DrawBlock( g , pos ,  Vec2i( BlockSize  , BlockSize  ) , color , outlineColor);
	}

	template< class TGraphics2D >
	static void DrawBlock(TGraphics2D& g ,Vec2i const& pos , int nw , int nh , int color )
	{
		DrawBlock( g , pos , BlockSize * Vec2i( nw , nh ) , color );
	}

	template< class TGraphics2D >
	static void DrawCapsuleX(TGraphics2D& g , Vec2i const& pos , Vec2i const& size )
	{
		g.drawRoundRect( pos , size , Vec2i( 2 * size.y / 3 , size.y ) );
	}
};


struct SimpleTextLayout
{
	template< class TGraphics2D, class ...Args>
	FORCEINLINE void show(TGraphics2D& g, char const* format, Args&& ...args)
	{
		InlineString< 512 > str;
		str.format(format, args...);
		g.drawText(Vector2( posX, posY ), str);
		posY += offset;
	}
	template< class TGraphics2D, class ...Args>
	FORCEINLINE void show(TGraphics2D& g, char const* str)
	{
		g.drawText(Vector2(posX, posY), str);
		posY += offset;
	}
	int posX = 100;
	int posY = 10;
	int offset = 15;
};


#endif // RenderUtility_H_7526E438_527C_4167_BD98_6F3E8A83A367