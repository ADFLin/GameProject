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

	template< class TGraphics2D >
	static void DrawDashboardBackground(TGraphics2D& g)
	{
		Vec2i screenSize = Global::GetScreenSize();

		// Draw Background Gradient
		Color3ub color1(10, 15, 25);
		Color3ub color2(25, 35, 55);
		int steps = 15;
		int stepHeight = screenSize.y / steps + 1;
		SetPen(g, EColor::Null);
		for (int i = 0; i < steps; ++i)
		{
			float t = (float)i / (steps - 1);
			Color3ub color(
				color1.r + (int)(t * (color2.r - color1.r)),
				color1.g + (int)(t * (color2.g - color1.g)),
				color1.b + (int)(t * (color2.b - color1.b))
			);
			g.setBrush(color);
			g.drawRect(Vec2i(0, i * stepHeight), Vec2i(screenSize.x, stepHeight));
		}

		// Draw subtle grid
		g.setPen(Color3ub(40, 60, 100), 1);
		int gridSpacing = 40;
		for (int x = 0; x < screenSize.x; x += gridSpacing)
			g.drawLine(Vec2i(x, 0), Vec2i(x, screenSize.y));
		for (int y = 0; y < screenSize.y; y += gridSpacing)
			g.drawLine(Vec2i(0, y), Vec2i(screenSize.x, y));
	}

	enum ArrowDir
	{
		eUp,
		eDown,
		eLeft,
		eRight
	};

	template< class TGraphics2D >
	static void DrawTriangle(TGraphics2D& g, Vec2i const& pos, Vec2i const& size, ArrowDir dir)
	{
		Math::Vector2 p[3];
		switch (dir)
		{
		case eUp:
			p[0] = Math::Vector2(pos.x + size.x / 2.0f, pos.y);
			p[1] = Math::Vector2(pos.x, pos.y + size.y);
			p[2] = Math::Vector2(pos.x + size.x, pos.y + size.y);
			break;
		case eDown:
			p[0] = Math::Vector2(pos.x, pos.y);
			p[1] = Math::Vector2(pos.x + size.x, pos.y);
			p[2] = Math::Vector2(pos.x + size.x / 2.0f, pos.y + size.y);
			break;
		case eLeft:
			p[0] = Math::Vector2(pos.x + size.x, pos.y);
			p[1] = Math::Vector2(pos.x, pos.y + size.y / 2.0f);
			p[2] = Math::Vector2(pos.x + size.x, pos.y + size.y);
			break;
		case eRight:
			p[0] = Math::Vector2(pos.x, pos.y);
			p[1] = Math::Vector2(pos.x + size.x, pos.y + size.y / 2.0f);
			p[2] = Math::Vector2(pos.x, pos.y + size.y);
			break;
		}
		g.drawPolygon(p, 3);
	}
};


struct SimpleTextLayout
{
	template< class TGraphics2D, class ...Args>
	FORCEINLINE void show(TGraphics2D& g, char const* format, Args&& ...args)
	{
		InlineString< 512 > str;
		str.format(format, args...);
		g.drawText(Vec2i( posX, posY ), str);
		posY += offset;
	}
	template< class TGraphics2D, class ...Args>
	FORCEINLINE void show(TGraphics2D& g, char const* str)
	{
		g.drawText(Vec2i(posX, posY), str);
		posY += offset;
	}
	int posX = 100;
	int posY = 10;
	int offset = 15;
};


#endif // RenderUtility_H_7526E438_527C_4167_BD98_6F3E8A83A367