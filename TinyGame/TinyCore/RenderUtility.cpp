#include "TinyGamePCH.h"
#include "RenderUtility.h"

#include "GameGlobal.h"
#include "GLGraphics2D.h"

#include <algorithm>

namespace
{
	HBRUSH hBrush[3][ Color::Number ];
	HPEN   hCPen[3][ Color::Number ];
	HFONT  hFont[ FONT_NUM ];
	COLORREF gColorMap[3][ Color::Number ];

	GLFont FontGL[ FONT_NUM ];
}


static HBRUSH getColorBrush( int color , int type = COLOR_NORMAL )
{
	return hBrush[type][color];
}

void RenderUtility::Initialize()
{
	using std::min;

	gColorMap[COLOR_NORMAL][ Color::eCyan   ] = RGB( 0 , 255 , 255 );
	gColorMap[COLOR_NORMAL][ Color::eBlue   ] = RGB( 0 , 0 , 255 );
	gColorMap[COLOR_NORMAL][ Color::eOrange ] = RGB( 255 , 165 , 0 );
	gColorMap[COLOR_NORMAL][ Color::eYellow ] = RGB( 255 , 255 , 0 );
	gColorMap[COLOR_NORMAL][ Color::eGreen  ] = RGB( 0 , 255 , 0 );
	gColorMap[COLOR_NORMAL][ Color::ePurple ] = RGB( 160 , 32 , 240 );
	gColorMap[COLOR_NORMAL][ Color::eRed    ] = RGB( 255 , 0 , 0 );
	gColorMap[COLOR_NORMAL][ Color::eGray   ] = RGB( 100 , 100 , 100 );
	gColorMap[COLOR_NORMAL][ Color::ePink   ] = RGB( 255 , 0 , 255 );
	gColorMap[COLOR_NORMAL][ Color::eWhite  ] = RGB( 255 , 255 , 255 );
	gColorMap[COLOR_NORMAL][ Color::eBlack  ] = RGB( 0, 0, 0 );

	hBrush[ COLOR_NORMAL ][ Color::eNull ] = 
	hBrush[ COLOR_DEEP   ][ Color::eNull ] =
	hBrush[ COLOR_LIGHT  ][ Color::eNull ] = (HBRUSH)::GetStockObject( NULL_BRUSH );

	hCPen[COLOR_NORMAL][ Color::eNull ] = hCPen[COLOR_DEEP][Color::eNull] = hCPen[COLOR_LIGHT][Color::eNull] = (HPEN)::GetStockObject( NULL_PEN ) ;

	for(int i = 1; i < Color::Number ; ++i )
	{
		hBrush[ COLOR_NORMAL ][i] = ::CreateSolidBrush( gColorMap[COLOR_NORMAL][i] );
		gColorMap[COLOR_DEEP][i] = RGB( 
			GetRValue( gColorMap[COLOR_NORMAL][i] ) * 60 / 100 , 
			GetGValue( gColorMap[COLOR_NORMAL][i] ) * 60 / 100 ,
			GetBValue( gColorMap[COLOR_NORMAL][i] ) * 60 / 100 );
		gColorMap[ COLOR_LIGHT ][i] = RGB( 
			min( GetRValue( gColorMap[COLOR_NORMAL][i] ) + 180 , 255 )  , 
			min( GetGValue( gColorMap[COLOR_NORMAL][i] ) + 180 , 255 )  ,
			min( GetBValue( gColorMap[COLOR_NORMAL][i] ) + 180 , 255 )  );

		hBrush[ COLOR_DEEP ][i] = ::CreateSolidBrush(  gColorMap[COLOR_DEEP][i] );
		hBrush[ COLOR_LIGHT ][i] = ::CreateSolidBrush( gColorMap[COLOR_LIGHT][i] );
		hCPen[COLOR_NORMAL][ i ] = CreatePen( PS_SOLID , 1 , gColorMap[COLOR_NORMAL][i] );
		hCPen[COLOR_DEEP][i] = CreatePen(PS_SOLID, 1, gColorMap[COLOR_DEEP][i]);
		hCPen[COLOR_LIGHT][i] = CreatePen(PS_SOLID, 1, gColorMap[COLOR_LIGHT][i]);
	}

	DrawEngine* de = Global::getDrawEngine();

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
	for(int i= 1 ; i < Color::Number ;++i)
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
	if ( color == Color::eNull )
	{
		g.enablePen( false );
	}
	else
	{
		COLORREF const& c = gColorMap[type][color];
		g.enablePen( true );
		g.setPen( Color3ub( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) ) );
	}
}

void RenderUtility::SetBrush(GLGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	if ( color == Color::eNull )
	{
		g.enableBrush( false );
	}
	else
	{
		COLORREF const& c = gColorMap[type][color];
		g.enableBrush( true );
		g.setBrush( Color3ub( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) ) );
	}
}

void RenderUtility::SetFont(GLGraphics2D& g , int fontID)
{
	//#TODO
	g.setFont( FontGL[ fontID ] );

}

void RenderUtility::SetPen(IGraphics2D& g , int color , int type )
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::SetPen(  g , color );  }
		virtual void visit(GLGraphics2D& g){  RenderUtility::SetPen(  g , color );  }
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
		int fontID;
	} visitor;
	visitor.fontID = fontID;
	g.accept( visitor );

}

void RenderUtility::StartOpenGL()
{
	HDC hDC = ::Global::getDrawEngine()->getWindow().getHDC();
	char const* faceName = "新細明體";
	FontGL[ FONT_S8  ].create(  8 , faceName , hDC );
	FontGL[ FONT_S10 ].create( 10 , faceName , hDC );
	FontGL[ FONT_S12 ].create( 12 , faceName , hDC );
	FontGL[ FONT_S16 ].create( 16 , faceName , hDC );
	FontGL[ FONT_S24 ].create( 24 , faceName , hDC );
}

void RenderUtility::StopOpenGL()
{
	for( int i = 0 ; i < FONT_NUM ; ++i)
		FontGL[i].cleanup();
}

void RenderUtility::SetFontColor(Graphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	COLORREF const& c = gColorMap[type][color];
	g.setTextColor( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) );
}

void RenderUtility::SetFontColor(GLGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	COLORREF const& c = gColorMap[type][color];
	g.setTextColor( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) );
}

void RenderUtility::SetFontColor(IGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	COLORREF const& c = gColorMap[type][color];
	g.setTextColor( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) );
}
