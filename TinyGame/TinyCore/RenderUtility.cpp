#include "TinyGamePCH.h"
#include "RenderUtility.h"

#include "GameGlobal.h"
#include "GameGraphics2D.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/Font.h"

#include <algorithm>
#include "DrawEngine.h"

using namespace Render;

namespace
{
	HBRUSH hBrush[3][EColor::Number];
	HPEN   hCPen[3][EColor::Number];
	HFONT  hFont[FONT_NUM];
	FontDrawer FontRHI[FONT_NUM];
	Color3ub gColorMap[3][EColor::Number];
	TCHAR const* FontName = "µÿ±d§§∂Í≈È";
}

static HBRUSH GetColorBrush( int color , int type = COLOR_NORMAL )
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
#if 1
		gColorMap[COLOR_DEEP][i] = Color3ub(
			int(gColorMap[COLOR_NORMAL][i].r) * 60 / 100 , 
			int(gColorMap[COLOR_NORMAL][i].g) * 60 / 100 ,
			int(gColorMap[COLOR_NORMAL][i].b) * 60 / 100 );
		gColorMap[ COLOR_LIGHT ][i] = Color3ub(
			min( gColorMap[COLOR_NORMAL][i].r + 180 , 255 )  , 
			min( gColorMap[COLOR_NORMAL][i].g + 180 , 255 )  ,
			min( gColorMap[COLOR_NORMAL][i].b + 180 , 255 )  );
#else
		Vector3 hsv = FColorConv::RGBToHSV(Color3f(gColorMap[COLOR_NORMAL][i]));
		Vector3 hsvLight = hsv; hsvLight.y = Math::Clamp<float>(2 * hsvLight.y, 0, 1);
		Vector3 hsvDark = hsv; hsvDark.y = Math::Clamp<float>(2 * hsvDark.y, 0, 1);

		gColorMap[COLOR_DEEP][i] = Color3ub(FColorConv::HSVToRGB(hsvDark));
		gColorMap[COLOR_LIGHT][i] = Color3ub( FColorConv::HSVToRGB(hsvLight));
#endif
		hBrush[COLOR_NORMAL][i] = FWindowsGDI::CreateBrush(gColorMap[COLOR_NORMAL][i]);
		hBrush[COLOR_DEEP][i]   = FWindowsGDI::CreateBrush(gColorMap[COLOR_DEEP][i]);
		hBrush[COLOR_LIGHT][i]  = FWindowsGDI::CreateBrush(gColorMap[COLOR_LIGHT][i]);
		hCPen[COLOR_NORMAL][i]  = FWindowsGDI::CreatePen(gColorMap[COLOR_NORMAL][i], 1);
		hCPen[COLOR_DEEP][i]    = FWindowsGDI::CreatePen(gColorMap[COLOR_DEEP][i], 1 );
		hCPen[COLOR_LIGHT][i]   = FWindowsGDI::CreatePen(gColorMap[COLOR_LIGHT][i], 1 );
	}

	DrawEngine& de = Global::GetDrawEngine();
	HDC hDC = de.getPlatformGraphics().getRenderDC();


	HFONT badFont = FWindowsGDI::CreateFont( hDC, FontName, 20, true , false );
	DeleteObject( badFont );
	hFont[ FONT_S8 ]  = FWindowsGDI::CreateFont(hDC, FontName, 8 , true , false );
	hFont[ FONT_S10 ] = FWindowsGDI::CreateFont(hDC, FontName, 10, true , false );
	hFont[ FONT_S12 ] = FWindowsGDI::CreateFont(hDC, FontName, 12, true , false );
	hFont[ FONT_S16 ] = FWindowsGDI::CreateFont(hDC, FontName, 16, true , false );
	hFont[ FONT_S24 ] = FWindowsGDI::CreateFont(hDC, FontName, 24, true , false );

}


void RenderUtility::Finalize()
{
	for(int i= 1 ; i < EColor::Number ;++i)
	{
		::DeleteObject(hBrush[ COLOR_NORMAL ][i]);
		::DeleteObject(hBrush[ COLOR_DEEP ][i]);
		::DeleteObject(hBrush[ COLOR_LIGHT ][i]);

		::DeleteObject(hCPen[COLOR_NORMAL][i]);
		::DeleteObject(hCPen[COLOR_DEEP][i]);
		::DeleteObject(hCPen[COLOR_LIGHT][i]);
	}

	for( int i = 0 ; i < FONT_NUM ; ++i )
	{
		::DeleteObject( hFont[i] );
	}
}

Render::FontDrawer& RenderUtility::GetFontDrawer(int fontID)
{
	return FontRHI[fontID];
}

Color3ub RenderUtility::GetColor(int color, int type /*= COLOR_NORMAL*/)
{
	return gColorMap[type][color];
}

#if ADD_PEN_WIDTH
void RenderUtility::SetPen( Graphics2D& g , int color , int type, int width)
{
	if (width == 1)
	{
		g.setPen(hCPen[type][color]);
	}
	else
	{
		g.setPen(gColorMap[type][color], width);
	}
}
#else
void RenderUtility::SetPen(Graphics2D& g, int color, int type)
{
	g.setPen(hCPen[type][color]);
}
#endif

void RenderUtility::SetBrush( Graphics2D& g , int color , int type )
{
	g.setBrush( GetColorBrush( color , type ) );
}

void RenderUtility::SetFont( Graphics2D& g , int fontID )
{
	g.setFont( hFont[fontID] );
}

#if ADD_PEN_WIDTH
void RenderUtility::SetPen( RHIGraphics2D& g , int color , int type, int width)
{
	if ( color == EColor::Null )
	{
		g.enablePen( false );
	}
	else
	{
		g.setPen(gColorMap[type][color], width);
	}
}
#else
void RenderUtility::SetPen(RHIGraphics2D& g, int color, int type)
{
	if (color == EColor::Null)
	{
		g.enablePen(false);
	}
	else
	{
		g.setPen(gColorMap[type][color]);
	}
}
#endif

void RenderUtility::SetBrush(RHIGraphics2D& g, int color, int type /*= COLOR_NORMAL */)
{
	if( color == EColor::Null )
	{
		g.enableBrush(false);
	}
	else
	{
		g.setBrush(gColorMap[type][color]);
	}
}

void RenderUtility::SetFont(RHIGraphics2D& g, int fontID)
{
	//#TODO
	g.setFont(FontRHI[fontID]);
}

#if ADD_PEN_WIDTH
void RenderUtility::SetPen(IGraphics2D& g , int color , int type, int width)
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::SetPen(  g , color , width);  }
		virtual void visit(RHIGraphics2D& g){  RenderUtility::SetPen(  g , color , width );  }
		int color;
		int type;
		int width;
	} visitor;
	visitor.color = color;
	visitor.type = type;
	visitor.width = width;
	g.accept( visitor );
}
#else
void RenderUtility::SetPen(IGraphics2D& g, int color, int type)
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g) { RenderUtility::SetPen(g, color); }
		virtual void visit(RHIGraphics2D& g) { RenderUtility::SetPen(g, color); }
		int color;
		int type;
	} visitor;
	visitor.color = color;
	visitor.type = type;
	g.accept(visitor);
}
#endif

void RenderUtility::SetBrush(IGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	struct MyVisistor : IGraphics2D::Visitor
	{
		virtual void visit(Graphics2D& g){  RenderUtility::SetBrush(  g , color , type  );  }
		virtual void visit(RHIGraphics2D& g){  RenderUtility::SetBrush(  g , color , type );  }
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
		virtual void visit(RHIGraphics2D& g){  RenderUtility::SetFont(  g , fontID );  }
		int fontID;
	} visitor;
	visitor.fontID = fontID;
	g.accept( visitor );

}

static bool IsRHISupport()
{
	if (GRHISystem->getName() == RHISystemName::OpenGL ||
		GRHISystem->getName() == RHISystemName::D3D11 ||
		GRHISystem->getName() == RHISystemName::D3D12 )
		return true;
	return false;
}

void RenderUtility::InitializeRHI()
{
	using namespace Render;
	if (!IsRHISupport())
		return;

	HDC hDC = ::Global::GetDrawEngine().getWindow().getHDC();
	FontCharCache::Get().hDC = hDC;
	FontCharCache::Get().initialize();

	FontRHI[ FONT_S8  ].initialize(FontFaceInfo(FontName, 8 , true));
	FontRHI[ FONT_S10 ].initialize(FontFaceInfo(FontName, 10, true));
	FontRHI[ FONT_S12 ].initialize(FontFaceInfo(FontName, 12, true));
	FontRHI[ FONT_S16 ].initialize(FontFaceInfo(FontName, 16, true));
	FontRHI[ FONT_S24 ].initialize(FontFaceInfo(FontName, 24, true));
}

void RenderUtility::ReleaseRHI()
{
	if (!IsRHISupport())
		return;

	for( int i = 0 ; i < FONT_NUM ; ++i)
		FontRHI[i].cleanup();

	FontCharCache::Get().releaseRHI();
}

void RenderUtility::SetFontColor(Graphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	g.setTextColor(gColorMap[type][color]);
}

void RenderUtility::SetFontColor(RHIGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	g.setTextColor(gColorMap[type][color]);
}

void RenderUtility::SetFontColor(IGraphics2D& g , int color , int type /*= COLOR_NORMAL */)
{
	g.setTextColor(gColorMap[type][color]);
}
