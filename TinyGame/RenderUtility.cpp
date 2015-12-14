#include "TinyGamePCH.h"
#include "RenderUtility.h"

#include "GameGlobal.h"
#include "GLGraphics2D.h"

#include <algorithm>

namespace
{
	HBRUSH hBrush[3][ Color::Number ];
	HPEN   hCPen[ Color::Number ];
	HFONT  hFont[ FONT_NUM ];
	COLORREF gColorMap[3][ Color::Number ];

	GLFont FontGL[ FONT_NUM ];
}


static HBRUSH getColorBrush( int color , int type = COLOR_NORMAL )
{
	return hBrush[type][color];
}

void RenderUtility::init()
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

	hCPen[ Color::eNull ] = (HPEN)::GetStockObject( NULL_PEN ) ;

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

		hBrush[ COLOR_DEEP ][i] = ::CreateSolidBrush(  gColorMap[COLOR_DEEP][i]);
		hBrush[ COLOR_LIGHT ][i] = ::CreateSolidBrush( gColorMap[COLOR_LIGHT][i] );
		hCPen[ i ] = CreatePen( PS_SOLID , 1 , gColorMap[COLOR_NORMAL][i] );
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


void RenderUtility::release()
{
	for(int i= 1 ; i < Color::Number ;++i)
	{
		::DeleteObject( hBrush[ COLOR_NORMAL ][i] );
		::DeleteObject( hBrush[ COLOR_DEEP ][i] );
		::DeleteObject( hBrush[ COLOR_LIGHT ][i] );

		::DeleteObject( hCPen[i] );
	}

	for( int i = 0 ; i < FONT_NUM ; ++i )
	{
		::DeleteObject( hFont[i] );
	}
}

void RenderUtility::setPen( Graphics2D& g , int color , int type )
{
	g.setPen( hCPen[ color ] );
}

void RenderUtility::setBrush( Graphics2D& g , int color , int type )
{
	g.setBrush( getColorBrush( color , type ) );
}

void RenderUtility::setFont( Graphics2D& g , int fontID )
{
	g.setFont( hFont[fontID] );
}

void RenderUtility::setPen( GLGraphics2D& g , int color , int type )
{
	if ( color == Color::eNull )
	{
		g.enablePen( false );
	}
	else
	{
		COLORREF const& c = gColorMap[type][color];
		g.enablePen( true );
		g.setPen( ColorKey3( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) ) );
	}
}

void RenderUtility::setBrush(GLGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	if ( color == Color::eNull )
	{
		g.enableBrush( false );
	}
	else
	{
		COLORREF const& c = gColorMap[type][color];
		g.enableBrush( true );
		g.setBrush( ColorKey3( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) ) );
	}
}

void RenderUtility::setFont(GLGraphics2D& g , int fontID)
{
	//TODO
	g.setFont( FontGL[ fontID ] );

}

void RenderUtility::setPen(IGraphics2D& g , int color , int type )
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::setPen(  g , color );  }
		virtual void visit(GLGraphics2D& g){  RenderUtility::setPen(  g , color );  }
		int color;
		int type;
	} visitor;
	visitor.color = color;
	visitor.type = type;
	g.accept( visitor );
}

void RenderUtility::setBrush(IGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::setBrush(  g , color , type  );  }
		virtual void visit(GLGraphics2D& g){  RenderUtility::setBrush(  g , color , type );  }
		int color;
		int type;
	} visitor;
	visitor.color = color;
	visitor.type = type;
	g.accept( visitor );
}

void RenderUtility::setFont(IGraphics2D& g , int fontID )
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::setFont(  g , fontID );  }
		virtual void visit(GLGraphics2D& g){  RenderUtility::setFont(  g , fontID );  }
		int fontID;
	} visitor;
	visitor.fontID = fontID;
	g.accept( visitor );

}

void RenderUtility::startOpenGL()
{
	HDC hDC = ::Global::getDrawEngine()->getWindow().getHDC();
	char const* faceName = "新細明體";
	FontGL[ FONT_S8  ].create(  8 , faceName , hDC );
	FontGL[ FONT_S10 ].create( 10 , faceName , hDC );
	FontGL[ FONT_S12 ].create( 12 , faceName , hDC );
	FontGL[ FONT_S16 ].create( 16 , faceName , hDC );
	FontGL[ FONT_S24 ].create( 24 , faceName , hDC );
}

void RenderUtility::stopOpenGL()
{
	for( int i = 0 ; i < FONT_NUM ; ++i)
		FontGL[i].release();
}

void RenderUtility::setFontColor(Graphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	COLORREF const& c = gColorMap[type][color];
	g.setTextColor( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) );
}

void RenderUtility::setFontColor(GLGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	COLORREF const& c = gColorMap[type][color];
	g.setTextColor( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) );
}

void RenderUtility::setFontColor(IGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	COLORREF const& c = gColorMap[type][color];
	g.setTextColor( GetRValue( c ) , GetGValue( c )  , GetBValue( c ) );
}
