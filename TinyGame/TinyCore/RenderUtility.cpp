#include "TinyGamePCH.h"
#include "RenderUtility.h"

#include "GameGlobal.h"
#include "GLGraphics2D.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/Font.h"

#include <algorithm>

using namespace Render;

namespace
{
	HBRUSH hBrush[3][EColor::Number];
	HPEN   hCPen[3][EColor::Number];
	HFONT  hFont[FONT_NUM];
	GLFont FontGL[FONT_NUM];

	Color3ub gColorMap[3][EColor::Number];
}


static HBRUSH getColorBrush( int color , int type = COLOR_NORMAL )
{
	return hBrush[type][color];
}

void RenderUtility::Initialize()
{
	using std::min;

	gColorMap[COLOR_NORMAL][ EColor::Cyan   ] = Color3ub( 0 , 255 , 255 );
	gColorMap[COLOR_NORMAL][ EColor::Blue   ] = Color3ub( 0 , 0 , 255 );
	gColorMap[COLOR_NORMAL][ EColor::Orange ] = Color3ub( 255 , 165 , 0 );
	gColorMap[COLOR_NORMAL][ EColor::Yellow ] = Color3ub( 255 , 255 , 0 );
	gColorMap[COLOR_NORMAL][ EColor::Green  ] = Color3ub( 0 , 255 , 0 );
	gColorMap[COLOR_NORMAL][ EColor::Purple ] = Color3ub( 160 , 32 , 240 );
	gColorMap[COLOR_NORMAL][ EColor::Red    ] = Color3ub( 255 , 0 , 0 );
	gColorMap[COLOR_NORMAL][ EColor::Gray   ] = Color3ub( 100 , 100 , 100 );
	gColorMap[COLOR_NORMAL][ EColor::Pink   ] = Color3ub( 255 , 0 , 255 );
	gColorMap[COLOR_NORMAL][ EColor::White  ] = Color3ub( 255 , 255 , 255 );
	gColorMap[COLOR_NORMAL][ EColor::Black  ] = Color3ub( 0, 0, 0 );

	hBrush[ COLOR_NORMAL ][ EColor::Null ] = 
	hBrush[ COLOR_DEEP   ][ EColor::Null ] =
	hBrush[ COLOR_LIGHT  ][ EColor::Null ] = (HBRUSH)::GetStockObject( NULL_BRUSH );

	hCPen[COLOR_NORMAL][ EColor::Null ] = hCPen[COLOR_DEEP][EColor::Null] = hCPen[COLOR_LIGHT][EColor::Null] = (HPEN)::GetStockObject( NULL_PEN ) ;

	for(int i = 1; i < EColor::Number ; ++i )
	{
		hBrush[ COLOR_NORMAL ][i] = ::CreateSolidBrush( gColorMap[COLOR_NORMAL][i].toXBGR() );
		gColorMap[COLOR_DEEP][i] = Color3ub(
			int(gColorMap[COLOR_NORMAL][i].r) * 60 / 100 , 
			int(gColorMap[COLOR_NORMAL][i].g) * 60 / 100 ,
			int(gColorMap[COLOR_NORMAL][i].b) * 60 / 100 );
		gColorMap[ COLOR_LIGHT ][i] = Color3ub(
			min( gColorMap[COLOR_NORMAL][i].r + 180 , 255 )  , 
			min( gColorMap[COLOR_NORMAL][i].g + 180 , 255 )  ,
			min( gColorMap[COLOR_NORMAL][i].b + 180 , 255 )  );

		hBrush[ COLOR_DEEP ][i] = ::CreateSolidBrush(  gColorMap[COLOR_DEEP][i].toXBGR() );
		hBrush[ COLOR_LIGHT ][i] = ::CreateSolidBrush( gColorMap[COLOR_LIGHT][i].toXBGR() );
		hCPen[COLOR_NORMAL][ i ] = CreatePen( PS_SOLID , 1 , gColorMap[COLOR_NORMAL][i].toXBGR() );
		hCPen[COLOR_DEEP][i] = CreatePen(PS_SOLID, 1, gColorMap[COLOR_DEEP][i].toXBGR() );
		hCPen[COLOR_LIGHT][i] = CreatePen(PS_SOLID, 1, gColorMap[COLOR_LIGHT][i].toXBGR() );
	}

	DrawEngine* de = Global::GetDrawEngine();

	HFONT badFont = de->createFont( 20 , TEXT("華康中圓體") , true , false );
	DeleteObject( badFont );

	hFont[ FONT_S8 ]  = de->createFont(  8 , TEXT("華康中圓體") , true , false );
	hFont[ FONT_S10 ] = de->createFont( 10 , TEXT("華康中圓體") , true , false );
	hFont[ FONT_S12 ] = de->createFont( 12 , TEXT("華康中圓體") , true , false );
	hFont[ FONT_S16 ] = de->createFont( 16 , TEXT("華康中圓體") , true , false );
	hFont[ FONT_S24 ] = de->createFont( 24 , TEXT("華康中圓體") , true , false );

}


void RenderUtility::Finalize()
{
	for(int i= 1 ; i < EColor::Number ;++i)
	{
		::DeleteObject( hBrush[ COLOR_NORMAL ][i] );
		::DeleteObject( hBrush[ COLOR_DEEP ][i] );
		::DeleteObject( hBrush[ COLOR_LIGHT ][i] );

		::DeleteObject( hCPen[COLOR_NORMAL][i] );
		::DeleteObject(hCPen[COLOR_DEEP][i]);
		::DeleteObject(hCPen[COLOR_LIGHT][i]);
	}

	for( int i = 0 ; i < FONT_NUM ; ++i )
	{
		::DeleteObject( hFont[i] );
	}
}

Color3ub RenderUtility::GetColor(int color, int type /*= COLOR_NORMAL*/)
{
	return gColorMap[type][color];
}

void RenderUtility::SetPen( Graphics2D& g , int color , int type )
{
	g.setPen( hCPen[type][ color ] );
}

void RenderUtility::SetBrush( Graphics2D& g , int color , int type )
{
	g.setBrush( getColorBrush( color , type ) );
}

void RenderUtility::SetFont( Graphics2D& g , int fontID )
{
	g.setFont( hFont[fontID] );
}

void RenderUtility::SetPen( GLGraphics2D& g , int color , int type )
{
	if ( color == EColor::Null )
	{
		g.enablePen( false );
	}
	else
	{
		g.enablePen( true );
		g.setPen(gColorMap[type][color]);
	}
}

void RenderUtility::SetBrush(GLGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	if ( color == EColor::Null )
	{
		g.enableBrush( false );
	}
	else
	{
		g.enableBrush( true );
		g.setBrush(gColorMap[type][color]);
	}
}

void RenderUtility::SetFont(GLGraphics2D& g , int fontID)
{
	//#TODO
	g.setFont( FontGL[ fontID ] );
}

void RenderUtility::SetPen(Render::RHIGraphics2D& g, int color, int type)
{
	if( color == EColor::Null )
	{
		g.enablePen(false);
	}
	else
	{
		g.enablePen(true);
		g.setPen(gColorMap[type][color]);
	}
}

void RenderUtility::SetBrush(Render::RHIGraphics2D& g, int color, int type /*= COLOR_NORMAL */)
{
	if( color == EColor::Null )
	{
		g.enableBrush(false);
	}
	else
	{
		g.enableBrush(true);
		g.setBrush(gColorMap[type][color]);
	}
}

void RenderUtility::SetFont(Render::RHIGraphics2D& g, int fontID)
{
	//#TODO
	g.setFont(FontGL[fontID]);
}

void RenderUtility::SetPen(IGraphics2D& g , int color , int type )
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::SetPen(  g , color );  }
		virtual void visit(GLGraphics2D& g){  RenderUtility::SetPen(  g , color );  }
		virtual void visit(Render::RHIGraphics2D& g) { RenderUtility::SetPen(g, color); }
		int color;
		int type;
	} visitor;
	visitor.color = color;
	visitor.type = type;
	g.accept( visitor );
}

void RenderUtility::SetBrush(IGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::SetBrush(  g , color , type  );  }
		virtual void visit(GLGraphics2D& g){  RenderUtility::SetBrush(  g , color , type );  }
		virtual void visit(Render::RHIGraphics2D& g) { RenderUtility::SetBrush(g, color, type); }
		int color;
		int type;
	} visitor;
	visitor.color = color;
	visitor.type = type;
	g.accept( visitor );
}

void RenderUtility::SetFont(IGraphics2D& g , int fontID )
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::SetFont(  g , fontID );  }
		virtual void visit(GLGraphics2D& g){  RenderUtility::SetFont(  g , fontID );  }
		virtual void visit(Render::RHIGraphics2D& g) { RenderUtility::SetFont(g, fontID); }
		int fontID;
	} visitor;
	visitor.fontID = fontID;
	g.accept( visitor );

}

void RenderUtility::InitializeRHI()
{
	using namespace Render;

	HDC hDC = ::Global::GetDrawEngine()->getWindow().getHDC();
	FontCharCache::Get().hDC = hDC;
	FontCharCache::Get().initialize();

	char const* faceName = "新細明體";
	FontGL[ FONT_S8  ].initialize(FontFaceInfo(faceName, 8 , true));
	FontGL[ FONT_S10 ].initialize(FontFaceInfo(faceName, 10, true));
	FontGL[ FONT_S12 ].initialize(FontFaceInfo(faceName, 12, true));
	FontGL[ FONT_S16 ].initialize(FontFaceInfo(faceName, 16, true));
	FontGL[ FONT_S24 ].initialize(FontFaceInfo(faceName, 24, true));
}

void RenderUtility::ReleaseRHI()
{
	for( int i = 0 ; i < FONT_NUM ; ++i)
		FontGL[i].cleanup();

	FontCharCache::Get().releaseRHI();
}

void RenderUtility::SetFontColor(Graphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	g.setTextColor(gColorMap[type][color]);
}

void RenderUtility::SetFontColor(GLGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	g.setTextColor(gColorMap[type][color]);
}

void RenderUtility::SetFontColor(IGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	g.setTextColor(gColorMap[type][color]);
}
