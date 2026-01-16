#include "TinyGamePCH.h"
#include "GameWidget.h"

#include "GameGUISystem.h"
#include "GameGlobal.h"
#include "StageBase.h"
#include "RenderUtility.h"

#include "FileSystem.h"

#include <cmath>
#include "ProfileSystem.h"

DefaultWidgetRenderer gDefaultRenderer;
UnrealWidgetRenderer   gUnrealRenderer;
WidgetRenderer*  gActiveWidgetRenderer = &gUnrealRenderer;

void DefaultWidgetRenderer::drawButton(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* title, int fontType, ButtonState state, WidgetColor const& color, bool beEnable)
{
	Vec2i renderPos = pos;
	if (state == BS_PRESS)
		renderPos += Vec2i(1, 1);

	if (state == BS_HIGHLIGHT)
		RenderUtility::SetPen(g, EColor::White);
	else
		RenderUtility::SetPen(g, EColor::Black);

	if (beEnable)
	{
		color.setupBrush(g, COLOR_NORMAL);
	}
	else
	{
		RenderUtility::SetBrush(g, EColor::Gray, COLOR_NORMAL);
	}
#define DRAW_CAPSULE 1

#if DRAW_CAPSULE
	RenderUtility::DrawCapsuleX(g, renderPos, size);
#else
	g.drawRect(renderPos, size);
#endif
	RenderUtility::SetPen(g, EColor::Null);
	RenderUtility::SetBrush(g, EColor::White, COLOR_NORMAL);
#if DRAW_CAPSULE
	RenderUtility::DrawCapsuleX(g, renderPos + Vec2i(3, 2), size - Vec2i(4, 6));
#else
	g.drawRect(renderPos + Vec2i(3, 2), size - Vec2i(4, 6));
#endif
	if (beEnable)
	{
		color.setupBrush(g, COLOR_DEEP);
	}
	else
	{
		RenderUtility::SetBrush(g, EColor::White, COLOR_DEEP);
	}
#if DRAW_CAPSULE
	RenderUtility::DrawCapsuleX(g, renderPos + Vec2i(3, 4), size - Vec2i(4, 6));
#else
	g.drawRect(renderPos + Vec2i(3, 4), size - Vec2i(4, 6));
#endif

	if (title && title[0] != '\0')
	{
		RenderUtility::SetFont(g, fontType);
		g.setBrush(Color3ub(255, 255, 255));

		if (state == BS_PRESS)
		{
			g.setTextColor(Color3ub(255, 0, 0));
			renderPos.x += 2;
			renderPos.y += 2;
		}
		else
		{
			g.setTextColor(Color3ub(255, 255, 0));
		}
		g.drawText(renderPos, size, title, EHorizontalAlign::Center, false);
	}
}

void DefaultWidgetRenderer::drawButton2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , ButtonState state , WidgetColor const& color, bool beEnable /*= true */ )
{

}

void DefaultWidgetRenderer::drawPanel( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , WidgetColor const& color)
{
	RenderUtility::SetBrush( g ,  EColor::White  );
	RenderUtility::SetPen( g , EColor::Black );
	g.drawRoundRect( pos , size , Vec2i( 10 , 10 ) );
	color.setupBrush(g);
	g.drawRoundRect( pos + Vec2i( 2 , 2 ) ,size - Vec2i( 4, 4 ) , Vec2i( 20 , 20 ) );

}

void DefaultWidgetRenderer::drawPanel( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , WidgetColor const& color, float alpha )
{
	g.beginBlend( pos , size , alpha );
	drawPanel( g , pos , size , color );
	g.endBlend();
}

void DefaultWidgetRenderer::drawPanel2( IGraphics2D& g , Vec2i const& pos , Vec2i const& size  , WidgetColor const& color)
{

	int border = 2;
	RenderUtility::SetPen( g , EColor::Black );
	RenderUtility::SetBrush( g , EColor::White );
	g.drawRect( pos , size );
	color.setupBrush(g);
	g.drawRect( pos + Vec2i( border ,border) , size - 2 * Vec2i( border , border ) );
}

void DefaultWidgetRenderer::drawSilder( IGraphics2D& g , Vec2i const& pos , Vec2i const& size , Vec2i const& tipPos , Vec2i const& tipSize )
{
	RenderUtility::SetPen(g, EColor::Black);
	RenderUtility::SetBrush( g , EColor::Blue );
	g.drawRoundRect( pos ,size , Vec2i( 3 , 3 ) );
	RenderUtility::SetBrush( g , EColor::Yellow , COLOR_DEEP );
	g.drawRoundRect( tipPos , tipSize , Vec2i( 3 , 3 ) );
	RenderUtility::SetBrush( g , EColor::Yellow );
	g.drawRoundRect( tipPos + Vec2i( 2 , 2 ) , tipSize - Vec2i( 4 , 4 ) , Vec2i( 1 , 1 ) );
}

void DefaultWidgetRenderer::drawCheckBox(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* title, int fontType, bool bChecked, ButtonState state)
{
	int brushColor;
	int colorType[2];
	if ( bChecked )
	{
		brushColor = EColor::Red;
		colorType[0] = COLOR_NORMAL;
		colorType[1] = COLOR_DEEP;
		RenderUtility::SetPen( g , (state == BS_HIGHLIGHT) ?  EColor::Yellow : EColor::White );
	}
	else
	{
		brushColor = EColor::Blue;
		colorType[0] = COLOR_NORMAL;
		colorType[1] = COLOR_DEEP;
		RenderUtility::SetPen( g , (state == BS_HIGHLIGHT) ? EColor::Yellow : EColor::Black );
	}
	RenderUtility::SetBrush(g, brushColor, colorType[0]);
	g.drawRect( pos , size );

	RenderUtility::SetBrush(g, brushColor, colorType[1]);
	RenderUtility::SetPen(g, brushColor);
	g.drawRect(pos + Vec2i(3, 3), size - 2 * Vec2i(3, 3));

	if (title && title[0] != '\0')
	{
		RenderUtility::SetFont(g, fontType);
		g.setTextColor(Color3ub(255, 255, 0));
		g.drawText((bChecked) ? pos : pos + Vec2i(1, 1), size, title);
	}
}

void DefaultWidgetRenderer::drawChoice(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* selectValue, ButtonState state, EHorizontalAlign alignment)
{
	int border = 2;
	RenderUtility::SetPen( g , EColor::Black );
	RenderUtility::SetBrush( g , EColor::White );
	g.drawRoundRect( pos , size ,Vec2i( 5 , 5 ) );

	RenderUtility::SetBrush(g, EColor::Blue, COLOR_DEEP);
	g.drawRoundRect( pos + Vec2i( border ,border) , size - 2 * Vec2i( border , border ) , Vec2i( 2 , 2 ) );

	if ( selectValue )
	{
		RenderUtility::SetFont( g , FONT_S8 );
		g.setTextColor( Color3ub(255 , 255 , 0) );
		if (alignment == EHorizontalAlign::Center)
		{
			g.drawText(pos, size, selectValue);
		}
		else if (alignment == EHorizontalAlign::Left)
		{
			g.drawText(pos + Vec2i(6, 0), selectValue);
		}
		else
		{
			// Right-align dummy logic or implement properly
			g.drawText(pos, size, selectValue);
		}
	}
}

void DefaultWidgetRenderer::drawChoiceItem(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* value, bool bSelected)
{
	g.beginBlend( pos , size , 0.8f );
	RenderUtility::SetPen( g , EColor::Black );
	RenderUtility::SetBrush( g , EColor::Blue , COLOR_LIGHT );
	g.drawRoundRect( pos + Vec2i(4,1), size - Vec2i(8,2) , Vec2i( 5 , 5 ) );
	g.endBlend();

	if ( bSelected )
		g.setTextColor(Color3ub(255 , 255 , 255) );
	else
		g.setTextColor(Color3ub(0, 0 , 0) );

	RenderUtility::SetFont( g , FONT_S8 );
	g.drawText( pos , size , value );
}

void DefaultWidgetRenderer::drawChoiceMenuBG(IGraphics2D& g, Vec2i const& pos, Vec2i const& size)
{
	g.beginBlend( pos , size , 0.8f );
	RenderUtility::SetPen( g , EColor::Black );
	RenderUtility::SetBrush( g , EColor::Blue  );
	g.drawRoundRect( pos + Vec2i(2,1), size - Vec2i(4,2) , Vec2i( 5 , 5 ) );
	g.endBlend();
}

void DefaultWidgetRenderer::drawListItem(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* value, bool bSelected, WidgetColor const& color)
{
	if (bSelected)
	{
		g.beginBlend( pos , size , 0.8f );
		RenderUtility::SetPen( g , EColor::Black );
		color.setupBrush( g , COLOR_NORMAL );
		g.drawRoundRect( pos + Vec2i(4,1), size - Vec2i(8,2) , Vec2i( 5 , 5 ) );
		g.endBlend();

		g.setTextColor(Color3ub(255 , 255 , 255) );
	}
	else
		g.setTextColor(Color3ub(255 , 255 , 0) );

	RenderUtility::SetFont( g , FONT_S10 );
	g.drawText( pos , size , value );
}

void DefaultWidgetRenderer::drawListBackground(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, WidgetColor const& color)
{
	RenderUtility::SetPen( g , EColor::Black );
	color.setupBrush(g, COLOR_LIGHT);
	g.drawRoundRect( pos , size , Vec2i( 12 , 12 ) );
}

void DefaultWidgetRenderer::drawTextCtrl(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* value, bool bFocus, bool bEnable, EHorizontalAlign alignment)
{
	RenderUtility::SetPen(g, EColor::Black);

	RenderUtility::SetBrush( g , EColor::Gray  );
	g.drawRect( pos , size );
	RenderUtility::SetBrush( g , EColor::Gray , COLOR_LIGHT );

	Vec2i inPos = pos + Vec2i( 2 , 2 );
	Vec2i inSize = size - Vec2i( 4 , 4 );
	g.drawRect( inPos , inSize );

	if ( bEnable )
		g.setTextColor( Color3ub(0 , 0 , 0) );
	else
		g.setTextColor( Color3ub(125 , 125 , 125) );

	RenderUtility::SetFont( g , FONT_S10 );

	if (alignment == EHorizontalAlign::Center)
	{
		g.drawText(pos, size, value, EHorizontalAlign::Center, false);
	}
	else
	{
		Vec2i textPos = pos + Vec2i(6, 3);
		g.drawText(textPos, value);
	}
}

void DefaultWidgetRenderer::drawText(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* text, int fontType)
{
	RenderUtility::SetFont(g, fontType);
	g.setTextColor(Color3ub(255, 255, 0));
	g.drawText(pos, text);
}

void DefaultWidgetRenderer::drawNoteBookButton(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* title, bool bSelected, ButtonState state)
{
	int color = bSelected ? (EColor::Green) : (EColor::Blue);
	RenderUtility::DrawBlock(g, pos, size, color);

	if (title && title[0] != '\0')
	{
		RenderUtility::SetFont(g, FONT_S8);
		if (bSelected)
			g.setTextColor(Color3ub(255, 255, 255));
		else
			g.setTextColor(Color3ub(255, 255, 0));

		g.drawText(pos, size, title);
	}
}

void DefaultWidgetRenderer::drawNoteBookPage(IGraphics2D& g, Vec2i const& pos, Vec2i const& size)
{
	RenderUtility::SetBrush(g, EColor::Null);
	g.setPen(Color3ub(255, 255, 255), 0);
	g.drawLine(pos, pos + Vec2i(size.x - 1, 0));
}

void DefaultWidgetRenderer::drawNoteBookBackground(IGraphics2D& g, Vec2i const& pos, Vec2i const& size)
{
	WidgetColor color;
	color.setValue(Color3ub(50, 100, 150));
	drawPanel(g, pos, size, color, 0.9f);
}

void DefaultWidgetRenderer::drawScrollBar(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, bool bHorizontal, Vec2i const& thumbPos, Vec2i const& thumbSize, ButtonState minusState, ButtonState plusState, ButtonState thumbState)
{
	int btnSize = 16;
	// Track
	RenderUtility::SetBrush(g, EColor::Blue, COLOR_DEEP);
	RenderUtility::SetPen(g, EColor::Black);
	g.drawRect(pos, size);

	// Buttons
	Vec2i btn1Pos = pos;
	Vec2i btn2Pos = bHorizontal ? pos + Vec2i(size.x - btnSize, 0) : pos + Vec2i(0, size.y - btnSize);
	Vec2i btnSizeVec(btnSize, btnSize);

	auto drawBtn = [&](Vec2i const& p, ButtonState state) {
		RenderUtility::SetBrush(g, EColor::Blue, (state == BS_PRESS) ? COLOR_DEEP : COLOR_NORMAL);
		RenderUtility::SetPen(g, (state == BS_HIGHLIGHT) ? EColor::White : EColor::Black);
		g.drawRect(p, btnSizeVec);
	};
	drawBtn(btn1Pos, minusState);
	drawBtn(btn2Pos, plusState);

	// Thumb
	RenderUtility::SetBrush(g, EColor::Yellow, (thumbState == BS_PRESS) ? COLOR_DEEP : COLOR_NORMAL);
	RenderUtility::SetPen(g, (thumbState == BS_HIGHLIGHT) ? EColor::White : EColor::Black);
	g.drawRoundRect(pos + thumbPos, thumbSize, Vec2i(4, 4));
}

void UnrealWidgetRenderer::drawButton(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* title, int fontType, ButtonState state, WidgetColor const& color, bool beEnable)
{
	Color3ub colorBG = beEnable ? Color3ub(65, 65, 65) : Color3ub(45, 45, 45);
	
	if (!color.bUseName)
	{
		colorBG = color.value;
	}

	Color3ub colorBorder = Color3ub(30, 30, 30);
	Color3ub colorHighlight = Color3ub(90, 90, 90);

	if (state == BS_HIGHLIGHT)
	{
		if (!color.bUseName)
		{
			// Make custom color brighter on highlight
			colorBG = Color3ub(std::min(255, colorBG.r + 20), std::min(255, colorBG.g + 20), std::min(255, colorBG.b + 20));
		}
		else
		{
			colorBG = Color3ub(85, 85, 85);
		}
		colorHighlight = Color3ub(120, 120, 120);
	}
	else if (state == BS_PRESS)
	{
		colorBG = Color3ub(40, 40, 40);
		colorHighlight = Color3ub(30, 30, 30);
	}

	// Shadow / Outer Border
	g.setBrush(colorBorder);
	RenderUtility::SetPen(g, EColor::Null);
	g.drawRect(pos, size);

	// Main Body
	g.setBrush(colorBG);
	g.drawRect(pos + Vec2i(1, 1), size - Vec2i(2, 2));

	// Top Bevel
	if (state != BS_PRESS)
	{
		g.setPen(colorHighlight);
		g.drawLine(pos + Vec2i(1, 1), pos + Vec2i(size.x - 2, 1));
	}

	// Bottom Accent Line
	if (state == BS_HIGHLIGHT || state == BS_PRESS)
	{
		g.setBrush(Color3ub(0, 120, 215));
		g.drawRect(pos + Vec2i(1, size.y - 3), Vec2i(size.x - 2, 2));
	}

	if (title && title[0] != '\0')
	{
		RenderUtility::SetFont(g, fontType);
		Vec2i textPos = pos;
		if (state == BS_PRESS) textPos += Vec2i(1, 1);

		if (beEnable)
			g.setTextColor(state == BS_HIGHLIGHT ? Color3ub(255, 255, 255) : Color3ub(230, 230, 230));
		else
			g.setTextColor(Color3ub(100, 100, 100));

		// Use the correct rectangle to ensure center alignment
		g.drawText(textPos, size, title);
	}
}

void UnrealWidgetRenderer::drawButton2(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, ButtonState state, WidgetColor const& color, bool beEnable /*= true */)
{
}


void UnrealWidgetRenderer::drawPanel(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, WidgetColor const& color)
{
	// Base layer (Unreal Dark Gray)
	g.setBrush(Color3ub(35, 35, 35));
	RenderUtility::SetPen(g, EColor::Null);
	g.drawRect(pos, size);

	// Subtle Outer Shadow/Border
	g.setPen(Color3ub(15, 15, 15));
	g.drawRect(pos, size);

	// UE-style header/top line (slightly lighter)
	g.setBrush(Color3ub(50, 50, 50));
	g.drawRect(pos + Vec2i(1, 1), Vec2i(size.x - 2, 2));

	// Internal Highlight Line (Bevel)
	g.setPen(Color3ub(45, 45, 45));
	g.drawLine(pos + Vec2i(1, 4), pos + Vec2i(1, size.y - 2));
}

void UnrealWidgetRenderer::drawPanel(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, WidgetColor const& color, float alpha)
{
	// Even with alpha, we want a solid base feel to avoid "muddy" black transparency
	g.beginBlend(pos, size, alpha);
	drawPanel(g, pos, size, color);
	g.endBlend();
}

void UnrealWidgetRenderer::drawPanel2(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, WidgetColor const& color)
{
	drawPanel(g, pos, size, color);
}

void UnrealWidgetRenderer::drawSilder(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, Vec2i const& tipPos, Vec2i const& tipSize)
{
	// Track
	g.setBrush(Color3ub(25, 27, 30));
	g.setPen(Color3ub(45, 45, 45));
	g.drawRect(pos, size);

	// Thumb (Tip)
	g.setBrush(Color3ub(60, 65, 70));
	RenderUtility::SetPen(g, EColor::Null);
	g.drawRoundRect(tipPos, tipSize, Vec2i(4, 4));
}

void UnrealWidgetRenderer::drawCheckBox(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* title, int fontType, bool bChecked, ButtonState state)
{
	Vec2i boxSize(16, 16);
	// Center the box vertically within the widget height
	Vec2i renderPos = pos + Vec2i(2, (size.y - boxSize.y) / 2);

	// Box background (inset look)
	g.setBrush(Color3ub(15, 15, 15));
	g.setPen(Color3ub(60, 60, 60));
	if (state == BS_HIGHLIGHT) g.setPen(Color3ub(100, 100, 100));
	g.drawRect(renderPos, boxSize);

	if (bChecked)
	{
		g.setBrush(Color3ub(0, 120, 215));
		RenderUtility::SetPen(g, EColor::Null);
		g.drawRect(Vector2(renderPos) + Vector2(2.0, 2.0), Vec2i(11, 11));
	}

	if (title && title[0] != '\0')
	{
		RenderUtility::SetFont(g, fontType);
		g.setTextColor(Color3ub(200, 200, 200));
		g.drawText(pos + Vec2i(boxSize.x + 10, 0), size - Vec2i(boxSize.x + 10, 0), title);
	}
}

void UnrealWidgetRenderer::drawChoice(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* selectValue, ButtonState state, EHorizontalAlign alignment)
{
	// Input area
	Color3ub colorBG(18, 18, 18);
	if (state == BS_HIGHLIGHT) colorBG = Color3ub(28, 28, 28);
	
	g.setBrush(colorBG);
	g.setPen(Color3ub(80, 80, 80)); 
	g.drawRect(pos, size);

	// Top intent
	g.setPen(Color3ub(10, 10, 10));
	g.drawLine(pos + Vec2i(1, 1), pos + Vec2i(size.x - 2, 1));

	// Side arrow section
	int arrowWidth = 20;
	Vec2i arrowAreaPos = pos + Vec2i(size.x - arrowWidth, 1);
	Vec2i arrowAreaSize = Vec2i(arrowWidth - 1, size.y - 2);
	g.setBrush(Color3ub(50, 50, 50));
	g.setPen(Color3ub(30, 30, 30));
	g.drawRect(arrowAreaPos, arrowAreaSize);

	// Draw Center Arrow (Triangle) - Precisely Balanced for RHI
	g.setBrush(Color3ub(200, 200, 200));
	RenderUtility::SetPen(g, EColor::Null);
	Vec2i cp = arrowAreaPos + arrowAreaSize / 2;
	Vector2 pts[3] = { cp + Vec2i(-4, -2), cp + Vec2i(4, -2), cp + Vec2i(0, 2) };
	g.drawPolygon(pts, 3);

	if (selectValue)
	{
		RenderUtility::SetFont(g, FONT_S8);
		g.setTextColor(Color3ub(240, 240, 240));
		if (alignment == EHorizontalAlign::Center)
		{
			g.drawText(pos, size, selectValue, EHorizontalAlign::Center, false);
		}
		else
		{
			g.drawText(pos + Vec2i(6, 0), Vec2i(size.x - arrowWidth - 6, size.y), selectValue, EHorizontalAlign::Left, false);
		}
	}
}

void UnrealWidgetRenderer::drawChoiceItem(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* value, bool bSelected)
{
	if (bSelected)
	{
		g.setBrush(Color3ub(0, 120, 215));
		RenderUtility::SetPen(g, EColor::Null);
		g.drawRect(pos, size);
		g.setTextColor(Color3ub(255, 255, 255));
	}
	else
	{
		g.setBrush(Color3ub(40, 40, 40));
		RenderUtility::SetPen(g, EColor::Null);
		g.drawRect(pos, size);
		g.setTextColor(Color3ub(200, 200, 200));
	}

	RenderUtility::SetFont(g, FONT_S8);
	g.drawText(pos, size, value);
}

void UnrealWidgetRenderer::drawChoiceMenuBG(IGraphics2D& g, Vec2i const& pos, Vec2i const& size)
{
	g.setBrush(Color3ub(30, 30, 30));
	g.setPen(Color3ub(80, 80, 80));
	g.drawRect(pos, size);
}

void UnrealWidgetRenderer::drawListItem(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* value, bool bSelected, WidgetColor const& color)
{
	if (bSelected)
	{
		g.setBrush(Color3ub(0, 120, 215));
		RenderUtility::SetPen(g, EColor::Null);
		g.drawRect(pos, size);
		g.setTextColor(Color3ub(255, 255, 255));
	}
	else
	{
		g.setTextColor(Color3ub(180, 180, 180));
	}

	RenderUtility::SetFont(g, FONT_S10);
	g.drawText(pos, size, value);
}

void UnrealWidgetRenderer::drawListBackground(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, WidgetColor const& color)
{
	g.setBrush(Color3ub(20, 20, 20));
	g.setPen(Color3ub(10, 10, 10));
	g.drawRect(pos, size);
}

void UnrealWidgetRenderer::drawTextCtrl(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* value, bool bFocus, bool bEnable, EHorizontalAlign alignment)
{
	g.setBrush(Color3ub(12, 12, 12));
	g.setPen(bFocus ? Color3ub(0, 150, 255) : Color3ub(75, 75, 75)); 
	g.drawRect(pos, size);

	if (bEnable)
		g.setTextColor(Color3ub(240, 240, 240));
	else
		g.setTextColor(Color3ub(100, 100, 100));

	RenderUtility::SetFont(g, FONT_S10);
	if (alignment == EHorizontalAlign::Center)
	{
		g.drawText(pos, size, value, EHorizontalAlign::Center, false);
	}
	else
	{
		// Left-aligned with vertical centering for text controls
		g.drawText(pos + Vec2i(6, 0), Vec2i(size.x - 6, size.y), value, EHorizontalAlign::Left, false);
	}
}

void UnrealWidgetRenderer::drawText(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* text, int fontType)
{
	RenderUtility::SetFont(g, fontType);
	g.setTextColor(Color3ub(225, 225, 225));
	// Use 4-argument call to enable centering within size
	g.drawText(pos, size, text);
}

void UnrealWidgetRenderer::drawNoteBookButton(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, char const* title, bool bSelected, ButtonState state)
{
	Color3ub colorBG = bSelected ? Color3ub(40, 40, 40) : Color3ub(25, 25, 25);
	if (state == BS_HIGHLIGHT) colorBG = Color3ub(50, 50, 50);

	g.setBrush(colorBG);
	g.setPen(Color3ub(20, 20, 20));
	g.drawRect(pos, size);

	if (bSelected)
	{
		g.setBrush(Color3ub(255, 255, 255));
		RenderUtility::SetPen(g, EColor::Null);
		g.drawRect(pos + Vec2i(2, size.y - 2), Vec2i(size.x - 4, 2));
	}

	if (title && title[0] != '\0')
	{
		RenderUtility::SetFont(g, FONT_S8);
		g.setTextColor(bSelected ? Color3ub(255, 255, 255) : Color3ub(160, 160, 160));
		g.drawText(pos, size, title);
	}
}

void UnrealWidgetRenderer::drawNoteBookPage(IGraphics2D& g, Vec2i const& pos, Vec2i const& size)
{
	g.setPen(Color3ub(40, 40, 40));
	RenderUtility::SetBrush(g, EColor::Null);
	g.drawLine(pos, pos + Vec2i(size.x - 1, 0));
}

void UnrealWidgetRenderer::drawNoteBookBackground(IGraphics2D& g, Vec2i const& pos, Vec2i const& size)
{
	g.setBrush(Color3ub(35, 35, 35));
	g.setPen(Color3ub(20, 20, 20));
	g.drawRect(pos, size);
}

void UnrealWidgetRenderer::drawScrollBar(IGraphics2D& g, Vec2i const& pos, Vec2i const& size, bool bHorizontal, Vec2i const& thumbPos, Vec2i const& thumbSize, ButtonState minusState, ButtonState plusState, ButtonState thumbState)
{
	int btnSize = 16;
	Color3ub colorTrack(45, 45, 48);
	Color3ub colorThumb(62, 62, 66);
	Color3ub colorThumbHover(104, 104, 104);
	Color3ub colorThumbPress(0, 122, 204);
	Color3ub colorArrow(200, 200, 200);

	// Track
	g.setBrush(colorTrack);
	RenderUtility::SetPen(g, EColor::Null);
	g.drawRect(pos, size);

	// Thumb
	Color3ub colorT = colorThumb;
	if (thumbState == BS_PRESS) colorT = colorThumbPress;
	else if (thumbState == BS_HIGHLIGHT) colorT = colorThumbHover;
	g.setBrush(colorT);
	g.drawRect(pos + thumbPos, thumbSize);

	// Arrows (simplified)
	auto drawArrow = [&](Vec2i p, int dir, ButtonState state) {
		if (state == BS_HIGHLIGHT) g.setBrush(Color3ub(80, 80, 80));
		else if (state == BS_PRESS) g.setBrush(Color3ub(100, 100, 100));
		else return;
		g.drawRect(p, Vec2i(btnSize, btnSize));
	};
	drawArrow(pos, 0, minusState);
	Vec2i btn2Pos = bHorizontal ? pos + Vec2i(size.x - btnSize, 0) : pos + Vec2i(0, size.y - btnSize);
	drawArrow(btn2Pos, 1, plusState);
}

GWidget::GWidget( Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
:WidgetCoreT< GWidget >( pos , size , parent )
{
	mID = UI_ANY;
	callback   = NULL;
	motionTask = NULL; 
	mFontType  = FONT_S8;
	useHotKey  = false;
}

GWidget::~GWidget()
{
	cleanupTweens();
	removeMotionTask();
	if ( callback )
		callback->release();

	if( useHotKey )
		::Global::GUI().removeHotkey( this );
}

void GWidget::sendEvent( int eventID )
{
	if (!preSendEvent(eventID))
		return;

	bool bKeep = true;
	if( onEvent )
	{
		bKeep = onEvent(eventID, this);
	}

	if ( mID != UI_ANY && bKeep )
	{
		GUI::Core* ui = this;
		while( ui != nullptr )
		{
			if( ui->isTop() )
				break;

			ui = ui->getParent();
			GWidget* widget = static_cast<GWidget*> (ui);
			if( !widget->onChildEvent(eventID, mID, this) )
				return;
		}

		if( mID < UI_WIDGET_ID )
			::Global::GUI().sendMessage(eventID, mID, this);
	}

}

void GWidget::removeMotionTask()
{
	if ( motionTask )
		motionTask->stop();
}

void GWidget::addTween(void* tween)
{
	mTweens.push_back(tween);
}

void GWidget::removeTween(void* tween)
{
	auto iter = std::find(mTweens.begin(), mTweens.end(), tween);
	if (iter != mTweens.end())
	{
		mTweens.erase(iter);
	}
}

void GWidget::cleanupTweens()
{
	for (auto tween : mTweens)
	{
		::Global::GUI().getTweener().remove(static_cast<Tween::IComponentT<float>*>(tween));
	}
	mTweens.clear();
}

void GWidget::refresh()
{
	if (canRefresh())
	{
		if (onRefresh)
			onRefresh(this);

		for (auto child = createChildrenIterator(); child; ++child)
		{
			child->refresh();
		}
	}
}

void GWidget::setRenderCallback( RenderCallBack* cb )
{
	if ( callback ) 
		callback->release(); 

	callback = cb;
}

void GWidget::setHotkey( ControlAction key )
{
	::Global::GUI().addHotkey( this , key );
	useHotKey = true;
}


GWidget* GWidget::findChild( int id , GWidget* start )
{
	for( auto child = createChildrenIterator(); child; ++child )
	{
		if( child->mID == id )
			return &(*child);
	}
	return NULL;
}

void GWidget::onPostRender()
{
	if ( callback )
		callback->postRender( this );
}

bool GWidget::doClipTest()
{
	if ( isTop() )
		return true;
	return true;
}

void GWidget::SetStyle(Style style)
{
	switch (style)
	{
	case Style::Default: gActiveWidgetRenderer = &gDefaultRenderer; break;
	case Style::Unreal: gActiveWidgetRenderer = &gUnrealRenderer; break;
	}
}

WidgetRenderer& GWidget::getRenderer()
{
	return *gActiveWidgetRenderer;
}

Vec2i const GMsgBox::BoxSize( 300 , 150 );

GMsgBox::GMsgBox( int _id , Vec2i const& pos , GWidget* parent , EMessageButton::Type buttonType)
	:GPanel( _id , pos , BoxSize , parent )
{
	GButton* button;

	switch( buttonType )
	{
	case EMessageButton::YesNo:
		button = new GButton( UI_NO , BoxSize - Vec2i( 50 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LOCTEXT("No") );
		button->setHotkey( ACT_BUTTON0 );
		button = new GButton( UI_YES , BoxSize - Vec2i( 100 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LOCTEXT("Yes") );
		button->setHotkey( ACT_BUTTON1 );
		break;
	case EMessageButton::Ok:
		button = new GButton( UI_OK , BoxSize - Vec2i( 50 , 30 ) , Vec2i( 40 , 20 ) , this );
		button->setTitle( LOCTEXT("OK") );
		button->setHotkey( ACT_BUTTON0 );
		break;
	}
}


bool GMsgBox::onChildEvent( int event , int id , GWidget* ui )
{
	if ( event != EVT_BUTTON_CLICK )
		return true;

	switch ( id )
	{
	case UI_OK  : sendEvent( EVT_BOX_OK  ); destroy(); return true;
	case UI_YES : sendEvent( EVT_BOX_YES ); destroy(); return true;
	case UI_NO :  sendEvent( EVT_BOX_NO  ); destroy(); return true;
	}
	return false;
}

void GMsgBox::onRender()
{
	GPanel::onRender();

	IGraphics2D& g = Global::GetIGraphics2D();

	Vec2i pos = getWorldPos();
	getRenderer().drawText(g, pos, Vec2i(BoxSize.x, BoxSize.y - 30), mTitle.c_str(), FONT_S12);
}

void GText::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawText(g, getWorldPos(), getSize(), mText.c_str(), mFontType);
}

UIMotionTask::UIMotionTask( GWidget* _ui , Vec2i const& _ePos , long time , WidgetMotionType type ) 
	:LifeTimeTask( time )
	,ui(_ui),ePos(_ePos),tParam( time ),mType( type )
{
	sPos = ui->getPos();
}

void UIMotionTask::release()
{
	delete this;
}

void UIMotionTask::LineMotion()
{
	Vec2i dir = ePos - sPos;

	//float len = sqrt( (float)dir.length2() );

	int x = int( ePos.x - float( getLifeTime() ) * dir.x / ( tParam ));
	int y = int( ePos.y - float( getLifeTime() ) * dir.y / ( tParam ));

	ui->setPos( Vec2i(x,y) );
}

void UIMotionTask::ShakeMotion()
{
	float const PI = 3.1415926f;
	if ( getLifeTime() == 0 )
		setLiftTime( tParam );

	Vec2i dir = ePos - sPos;

	float st =  sin( 2 * PI * float( getLifeTime() ) / tParam );

	int x = sPos.x + int( st * dir.x );
	int y = sPos.y + int( st * dir.y );

	ui->setPos( Vec2i(x,y) );
}

void UIMotionTask::onEnd( bool beComplete )
{
	ui->motionTask = NULL;

	switch( mType )
	{
	case WMT_LINE_MOTION: ui->setPos( ePos ); break;
	case WMT_SHAKE_MOTION: ui->setPos( sPos ); break;
	}
}

bool UIMotionTask::onUpdate( long time )
{
	if ( !LifeTimeTask::onUpdate( time ) )
		setLiftTime( 0 );

	switch( mType )
	{
	case WMT_LINE_MOTION: LineMotion(); break;
	case WMT_SHAKE_MOTION: ShakeMotion(); break;
	}

	return getLifeTime() > 0;
}

void UIMotionTask::onStart()
{
	if ( ui->motionTask )
		ui->motionTask->skip();

	 ui->motionTask = this;
}

GButton::GButton( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( id , pos , size , parent )
{
	setColorName( EColor::Blue );
}

void GButton::onMouse( bool beInside )
{
	BaseClass::onMouse( beInside );

	if ( beInside )
	{
		if ( !motionTask )
		{
			TaskBase* task = new UIMotionTask( this , getPos() + Vec2i( 0 , 2 ) , 500 , WMT_SHAKE_MOTION );
			::Global::GUI().addTask( task );
		}
	}
	else
	{
		UIMotionTask* task = dynamic_cast< UIMotionTask* >( motionTask );
		if ( task  && task->mType == WMT_SHAKE_MOTION )
		{
			motionTask->stop();
			motionTask = NULL;
		}
	}
}

void GButton::onRender()
{
	PROFILE_FUNCTION();
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawButton(g, getWorldPos(), getSize(), mTitle.c_str(), mFontType, getButtonState(), mColor, isEnable());
}

GNoteBook::GNoteBook( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:GUI::NoteBookT< GNoteBook >(  pos , size , parent )
{
	mID = id;
}

void GNoteBook::doRenderButton( PageButton* button )
{
	IGraphics2D&  g = Global::GetIGraphics2D();
	getRenderer().drawNoteBookButton(g, button->getWorldPos(), button->getSize(), button->title.c_str(), button->page == getCurPage(), button->getButtonState());
}

void GNoteBook::doRenderPage( Page* page )
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawNoteBookPage(g, page->getWorldPos(), page->getSize());
}

void GNoteBook::doRenderBackground()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawNoteBookBackground(g, getWorldPos(), getSize());
}

GTextCtrl::GTextCtrl( int id , Vec2i const& pos , int length , GWidget* parent ) 
	:BaseClass( pos , Vec2i( length , UI_Height ) , parent )
{
	mID = id;
}

void GTextCtrl::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawTextCtrl(g, getWorldPos(), getSize(), getValue(), isFocus(), isEnable(), mAlignment);
}


void GChoice::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawChoice(g, getWorldPos(), getSize(), getSelectValue(), BS_NORMAL, mAlignment);
}

void GChoice::doRenderItem( Vec2i const& pos , Item& item , bool bSelected )
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i size( getSize().x , getMenuItemHeight() );
	getRenderer().drawChoiceItem(g, pos, size, item.value.c_str(), bSelected);
}

void GChoice::doRenderMenuBG( Menu* menu )
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawChoiceMenuBG(g, menu->getWorldPos(), menu->getSize());
}

GChoice::GChoice( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	: BaseClass( pos , size , parent )
{
	mID = id;
	setColorName( EColor::Blue );
}

GPanel::GPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) :BaseClass( pos , size , parent )
	,mAlpha( 0.5f )
	,mRenderType( eRoundRectType )
{
	mID = id;
	setColor( Color3ub( 50 , 100 , 150 ) );
}

GPanel::~GPanel()
{

}

void GPanel::onRender()
{
	Vec2i pos = getWorldPos();

	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i size = getSize();

	switch ( mRenderType )
	{
	case eRoundRectType:
		g.beginBlend( pos , size , mAlpha );
		gActiveWidgetRenderer->drawPanel( g , pos , size  , mColor );
		g.endBlend();
		break;
	case eRectType:
		g.beginBlend( pos , size , mAlpha );
		gActiveWidgetRenderer->drawPanel2( g , pos , size , mColor );
		g.endBlend();
		break;
	}
}

MsgReply GPanel::onMouseMsg(MouseMsg const& msg)
{
	if( msg.onLeftDown() )
	{
		setTop();
	}
	return BaseClass::onMouseMsg(msg);
}

GListCtrl::GListCtrl( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( pos , size , parent )
{
	mID = id;
	setColorName( EColor::Blue );
}

void GListCtrl::doRenderItem( Vec2i const& pos , Item& item , bool bSelected )
{
	IGraphics2D& g = Global::GetIGraphics2D();
	Vec2i size( getSize().x , getItemHeight() );
	getRenderer().drawListItem(g, pos, size, item.value.c_str(), bSelected, mColor);
}

void GListCtrl::doRenderBackground( Vec2i const& pos , Vec2i const& size )
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawListBackground(g, pos, size, mColor);
}

GFrame::GFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) 
	:BaseClass( id , pos , size , parent )
{

}

// Helper enum for resize edge detection
enum ResizeEdge
{
	RESIZE_NONE   = 0,
	RESIZE_LEFT   = BIT(0),
	RESIZE_RIGHT  = BIT(1),
	RESIZE_TOP    = BIT(2),
	RESIZE_BOTTOM = BIT(3),
	RESIZE_TOP_LEFT = RESIZE_TOP | RESIZE_LEFT,
	RESIZE_TOP_RIGHT = RESIZE_TOP | RESIZE_RIGHT,
	RESIZE_BOTTOM_LEFT = RESIZE_BOTTOM | RESIZE_LEFT,
	RESIZE_BOTTOM_RIGHT = RESIZE_BOTTOM | RESIZE_RIGHT,
};

SUPPORT_ENUM_FLAGS_OPERATION(ResizeEdge);

#ifdef SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif


enum class ECursorIcon
{
	SizeWE,
	SizeNS,
	SizeNWSE,
	sizeNESW,
};

class CursorManager
{
public:
	static void SetCursor(ECursorIcon icon)
	{
#ifdef SYS_PLATFORM_WIN
		LPCTSTR cursorName = IDC_ARROW;
		switch (icon)
		{
		case ECursorIcon::SizeWE:
			cursorName = IDC_SIZEWE;
			break;
		case ECursorIcon::SizeNS:
			cursorName = IDC_SIZENS;
			break;
		case ECursorIcon::SizeNWSE:
			cursorName = IDC_SIZENWSE;
			break;
		case ECursorIcon::sizeNESW:
			cursorName = IDC_SIZENESW;
			break;
		}
		::SetCursor(::LoadCursor(NULL, cursorName));
#endif
	}
};

struct ResizeOperation
{
	static const int BorderWidth = 8;

	static ECursorIcon GetCursorIcon(ResizeEdge edge)
	{
		switch (edge)
		{
		case RESIZE_LEFT:
		case RESIZE_RIGHT:
			return ECursorIcon::SizeWE;
		case RESIZE_TOP:
		case RESIZE_BOTTOM:
			return ECursorIcon::SizeNS;
		case RESIZE_TOP_LEFT:
		case RESIZE_BOTTOM_RIGHT:
			return ECursorIcon::SizeNWSE;
		case RESIZE_TOP_RIGHT:
		case RESIZE_BOTTOM_LEFT:
			return ECursorIcon::sizeNESW;
		}
		return ECursorIcon::SizeWE;
	}
	
	ResizeEdge detectEdge(Vec2i const& localPos, Vec2i const& widgetSize)
	{
		ResizeEdge edge = RESIZE_NONE;
		
		if (localPos.x < BorderWidth)
			edge |= RESIZE_LEFT;
		else if (localPos.x >= widgetSize.x - BorderWidth)
			edge |= RESIZE_RIGHT;
			
		if (localPos.y < BorderWidth)
			edge |= RESIZE_TOP;
		else if (localPos.y >= widgetSize.y - BorderWidth)
			edge |= RESIZE_BOTTOM;
			
		return edge;
	}
	
	void start(ResizeEdge edge, Vec2i const& startWorldPos, Vec2i const& widgetPos, Vec2i const& widgetSize)
	{
		resizingEdge = edge;
		startMouseWorldPos = startWorldPos;
		startWidgetPos = widgetPos;
		startWidgetSize = widgetSize;
	}
	
	void end()
	{
		resizingEdge = RESIZE_NONE;
	}
	
	bool isResizing() const
	{
		return resizingEdge != RESIZE_NONE;
	}
	
	void updateResize(Vec2i const& currentWorldPos, Vec2i& outPos, Vec2i& outSize)
	{
		if (!isResizing())
			return;
			
		Vec2i delta = currentWorldPos - startMouseWorldPos;
		Vec2i newPos = startWidgetPos;
		Vec2i newSize = startWidgetSize;

		const int minWidth = 50;
		const int minHeight = 30;
		
		// Handle horizontal resizing
		if (resizingEdge & RESIZE_LEFT)
		{
			int newWidth = startWidgetSize.x - delta.x;
			// Clamp to minimum width
			if (newWidth < minSize.x)
			{
				newWidth = minSize.x;
				// Adjust position to maintain right edge
				newPos.x = startWidgetPos.x + startWidgetSize.x - minSize.x;
			}
			else
			{
				newPos.x = startWidgetPos.x + delta.x;
			}
			newSize.x = newWidth;
		}
		else if (resizingEdge & RESIZE_RIGHT)
		{
			int newWidth = startWidgetSize.x + delta.x;
			// Clamp to minimum width
			newSize.x = (newWidth >= minSize.x) ? newWidth : minSize.x;
		}
		
		// Handle vertical resizing
		if (resizingEdge & RESIZE_TOP)
		{
			int newHeight = startWidgetSize.y - delta.y;
			// Clamp to minimum height
			if (newHeight < minSize.y)
			{
				newHeight = minSize.y;
				// Adjust position to maintain bottom edge
				newPos.y = startWidgetPos.y + startWidgetSize.y - minSize.y;
			}
			else
			{
				newPos.y = startWidgetPos.y + delta.y;
			}
			newSize.y = newHeight;
		}
		else if (resizingEdge & RESIZE_BOTTOM)
		{
			int newHeight = startWidgetSize.y + delta.y;
			// Clamp to minimum height
			newSize.y = (newHeight >= minSize.y) ? newHeight : minSize.y;
		}
		
		outPos = newPos;
		outSize = newSize;
	}
	
	ResizeEdge resizingEdge = RESIZE_NONE;
	Vec2i startMouseWorldPos;  // Use world coordinates instead of local
	Vec2i startWidgetPos;
	Vec2i startWidgetSize;

	Vec2i minSize = Vec2i(50, 30);
};

struct DragOperation
{

	void start(Vec2i const& startPos)
	{
		bStart = true;
		pos = startPos;
	}
	void end()
	{
		bStart = false;
	}
	bool update(Vec2i const& inPos , Vec2i& outOffset )
	{
		if( bStart )
		{
			outOffset = inPos - pos;
			pos = inPos;
		}
		return bStart;
	}

	Vec2i pos;
	bool  bStart;
};

MsgReply GFrame::onMouseMsg( MouseMsg const& msg )
{
	BaseClass::onMouseMsg( msg );

	static ResizeOperation resizeOp;
	static DragOperation dragOp;



	if ( msg.onLeftUp() )
	{
		getManager()->releaseMouse();
		resizeOp.end();
		dragOp.end();
	}
	else if ( msg.onLeftDown() )
	{
		setTop();
		
		ResizeEdge edge = RESIZE_NONE;
		if (bCanResize)
		{
			Vec2i localPos = msg.getPos() - getWorldPos();
			edge = resizeOp.detectEdge(localPos, getSize());
		}
		if (edge != RESIZE_NONE)
		{
			getManager()->captureMouse( this );
			resizeOp.start(edge, msg.getPos(), getPos(), getSize());
			resizeOp.minSize = mMinSize;
			CursorManager::SetCursor(ResizeOperation::GetCursorIcon(edge));
		}
		else
		{
			getManager()->captureMouse( this );
			dragOp.start(msg.getPos());
		}
	}	
	else if (msg.onMoving())
	{
		if (bCanResize)
		{
			if (resizeOp.isResizing())
			{
				CursorManager::SetCursor(ResizeOperation::GetCursorIcon(resizeOp.resizingEdge));
			}
			else if (!dragOp.bStart)
			{
				Vec2i localPos = msg.getPos() - getWorldPos();
				ResizeEdge edge = resizeOp.detectEdge(localPos, getSize());
				if (edge != RESIZE_NONE)
				{
					CursorManager::SetCursor(ResizeOperation::GetCursorIcon(edge));
				}
			}
		}
	}


	if ( msg.isLeftDown() && msg.isDraging() )
	{
		if (resizeOp.isResizing())
		{
			Vec2i newPos, newSize;
			resizeOp.updateResize(msg.getPos(), newPos, newSize);
			setPos(newPos);
			setSize(newSize);
		}
		else if (dragOp.bStart)
		{
			Vec2i offset;
			if( dragOp.update(msg.getPos(),offset) )
			{
				setPos(getPos() + offset);
			}
		}
	}

	return MsgReply::Handled();
}

Vec2i GSlider::TipSize( 10, 10 );

GSlider::GSlider( int id , Vec2i const& pos , int length , bool beH , GWidget* parent ) 
	:BaseClass( pos , length , TipSize , beH , 0 , 1000 , parent )
{
	mID = id;
}

void GSlider::renderValue( GWidget* widget )
{
	Vec2i pos = getWorldPos();
	Vec2i size = getSize();
	IGraphics2D& g = Global::GetIGraphics2D();

	if (onGetShowValue)
	{
		std::string value = onGetShowValue();
		getRenderer().drawText(g, pos, size, value.c_str(), mFontType);
	}
	else
	{
		InlineString< 256 > str;
		str.format("%d", getValue());
		getRenderer().drawText(g, pos, size, str, FONT_S10);
	}
}

void GSlider::showValue()
{
	setRenderCallback( RenderCallBack::Create( this , &GSlider::renderValue ) );
}

void GSlider::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawSilder( g , getWorldPos() , getSize() , getTipWidget()->getWorldPos() , getTipWidget()->getSize() );
}

GCheckBox::GCheckBox(int id , Vec2i const& pos , Vec2i const& size , GWidget* parent) 
	:BaseClass( id , pos , size , parent )
{
	bChecked = false;
	mID = id;
}

void GCheckBox::onClick()
{
	bChecked = !bChecked; 
	sendEvent(EVT_CHECK_BOX_CHANGE);
	updateMotionState(true);
}

void GCheckBox::onMouse(bool beInside)
{
	BaseClass::onMouse(beInside);
	updateMotionState(beInside);
}

void GCheckBox::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawCheckBox(g, getWorldPos(), getSize(), mTitle.c_str(), mFontType, bChecked, getButtonState());
}



void GCheckBox::updateMotionState( bool bInside )
{
	if( bInside && bChecked == false )
	{
		if( !motionTask )
		{
			TaskBase* task = new UIMotionTask(this, getPos() + Vec2i(0, 2), 500, WMT_SHAKE_MOTION);
			::Global::GUI().addTask(task);
		}
	}
	else
	{
		UIMotionTask* task = dynamic_cast<UIMotionTask*>(motionTask);
		if( task  && task->mType == WMT_SHAKE_MOTION )
		{
			motionTask->stop();
			motionTask = NULL;
		}
	}
}

GFileListCtrl::GFileListCtrl(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent) 
	:BaseClass(id, pos, size, parent)
{

}

String GFileListCtrl::getSelectedFilePath() const
{
	char const* name = getSelectValue();
	if( name )
		return mCurDir + "/" + name;

	return String();
}

void GFileListCtrl::deleteSelectdFile()
{
	if( getSelection() == INDEX_NONE )
		return;
	String path = getSelectedFilePath();
	FFileSystem::DeleteFile(path.c_str());
	removeItem(getSelection());
}

void GFileListCtrl::setDir(char const* dir)
{
	mCurDir = dir;
	refreshFiles();
}

void GFileListCtrl::refreshFiles()
{
	removeAllItem();

	FileIterator fileIter;
	if( !FFileSystem::FindFiles(mCurDir.c_str(), mSubFileName.c_str(), fileIter) )
		return;

	for( ; fileIter.haveMore(); fileIter.goNext() )
	{
		addItem(fileIter.getFileName());
	}
}

GText::GText( Vec2i const& pos, Vec2i const& size, GWidget* parent) 
	:BaseClass(  pos, size, parent)
{
	mID = UI_ANY;
	setRerouteMouseMsgUnhandled();
}


// GScrollBar Implementation

GScrollBar::GScrollBar(int id, Vec2i const& pos, int length, bool beH, GWidget* parent)
	:BaseClass(pos, beH ? Vec2i(length, 16) : Vec2i(16, length), parent)
{
	mID = id;
	mbHorizontal = beH;

	// No child buttons created

	updateThumbSize();
	updateThumbPos();
}

void GScrollBar::setRange(int range, int pageSize)
{
	mRange = range;
	mPageSize = pageSize;
	updateThumbSize();
	setValue(mValue);
}

void GScrollBar::setValue(int value)
{
	int maxVal = std::max(0, mRange);
	int oldVal = mValue;
	mValue = std::max(0, std::min(value, maxVal));
	
	if (oldVal != mValue)
	{
		updateThumbPos();
		onScrollChange(mValue);
	}
}

void GScrollBar::updateThumbSize()
{
	int btnSize = 16;
	int trackLen = (mbHorizontal ? getSize().x : getSize().y) - 2 * btnSize;
	
	if (trackLen <= 0) 
		return;

	int total = mRange + mPageSize;
	if (total <= 0) 
		total = 1;

	int thumbLen = Math::Clamp(trackLen * mPageSize / total, 10, trackLen);
	mThumbSize = mbHorizontal ? Vec2i(thumbLen, 16) : Vec2i(16, thumbLen);
}

void GScrollBar::updateThumbPos()
{
	int btnSize = 16;
	int trackLen = (mbHorizontal ? getSize().x : getSize().y) - 2 * btnSize;
	int thumbLen = mbHorizontal ? mThumbSize.x : mThumbSize.y;
	int movableLen = trackLen - thumbLen;

	if (movableLen <= 0)
	{
		if (mbHorizontal) mThumbPos = Vec2i(btnSize, 0);
		else mThumbPos = Vec2i(0, btnSize);
		return;
	}

	int offset = 0;
	if (mRange > 0)
	{
		offset = mValue * movableLen / mRange;
	}

	if (mbHorizontal)
		mThumbPos = Vec2i(btnSize + offset, 0);
	else
		mThumbPos = Vec2i(0, btnSize + offset);
}

int GScrollBar::calcValueFromPos(Vec2i const& pos)
{
	int btnSize = 16;
	int trackLen = (mbHorizontal ? getSize().x : getSize().y) - 2 * btnSize;
	int thumbLen = mbHorizontal ? mThumbSize.x : mThumbSize.y;
	int movableLen = trackLen - thumbLen;

	if (movableLen <= 0) return 0;

	// Calculate center of thumb relative to track start
	int relativePos = (mbHorizontal ? pos.x : pos.y) - btnSize - thumbLen / 2;
	
	int val = relativePos * mRange / movableLen;
	return Math::Clamp(val, 0, mRange);
}

void GScrollBar::onRender()
{
	IGraphics2D& g = Global::GetIGraphics2D();
	getRenderer().drawScrollBar(g, getWorldPos(), getSize(), mbHorizontal, mThumbPos, mThumbSize, mPressPart == PART_MINUS ? BS_PRESS : (mHoverPart == PART_MINUS ? BS_HIGHLIGHT : BS_NORMAL), mPressPart == PART_PLUS ? BS_PRESS : (mHoverPart == PART_PLUS ? BS_HIGHLIGHT : BS_NORMAL), mPressPart == PART_THUMB ? BS_PRESS : (mHoverPart == PART_THUMB ? BS_HIGHLIGHT : BS_NORMAL));
}

MsgReply GScrollBar::onMouseMsg(MouseMsg const& msg)
{
	Vec2i localPos = msg.getPos() - getWorldPos();
	Vec2i size = getSize();
	int btnSize = 16;

	// Hit testing
	EPart hitPart = PART_NONE;
	if (localPos.x >= 0 && localPos.y >= 0 && localPos.x < size.x && localPos.y < size.y)
	{
		if (mbHorizontal)
		{
			if (localPos.x < btnSize) hitPart = PART_MINUS;
			else if (localPos.x >= size.x - btnSize) hitPart = PART_PLUS;
			else if (localPos.x >= mThumbPos.x && localPos.x < mThumbPos.x + mThumbSize.x) hitPart = PART_THUMB;
			else hitPart = PART_TRACK;
		}
		else
		{
			if (localPos.y < btnSize) hitPart = PART_MINUS;
			else if (localPos.y >= size.y - btnSize) hitPart = PART_PLUS;
			else if (localPos.y >= mThumbPos.y && localPos.y < mThumbPos.y + mThumbSize.y) hitPart = PART_THUMB;
			else hitPart = PART_TRACK;
		}
	}

	if (msg.onMoving())
	{
		if (mHoverPart != hitPart)
		{
			mHoverPart = hitPart;
			// Redraw? GWidget doesn't have invalidate. But onRender is called every frame usually?
			// If not, we might need to trigger update.
		}
	}
	else if (msg.onLeftDown() || msg.onLeftDClick())
	{
		mPressPart = hitPart;
		getManager()->captureMouse(this);

		if (hitPart == PART_THUMB)
		{
			mbDragging = true;
			mDragStartPos = msg.getPos();
			mDragStartValue = mValue;
		}
		else if (hitPart == PART_MINUS)
		{
			setValue(mValue - 1);
		}
		else if (hitPart == PART_PLUS)
		{
			setValue(mValue + 1);
		}
		else if (hitPart == PART_TRACK)
		{
			setValue(calcValueFromPos(localPos));
		}
		return MsgReply::Handled();
	}
	else if (msg.onLeftUp())
	{
		if (mPressPart != PART_NONE)
		{
			mPressPart = PART_NONE;
			mbDragging = false;
			getManager()->releaseMouse();
			return MsgReply::Handled();
		}
	}
	
	if (msg.isLeftDown() && mbDragging)
	{
		// Drag logic (same as before)
		int trackLen = (mbHorizontal ? getSize().x : getSize().y) - 2 * btnSize;
		int thumbLen = mbHorizontal ? mThumbSize.x : mThumbSize.y;
		int movableLen = trackLen - thumbLen;

		if (movableLen > 0)
		{
			int deltaPix = mbHorizontal ? (msg.x() - mDragStartPos.x) : (msg.y() - mDragStartPos.y);
			int deltaVal = deltaPix * mRange / movableLen;
			setValue(mDragStartValue + deltaVal);
		}
		return MsgReply::Handled();
	}

	return BaseClass::onMouseMsg(msg);
}

bool GScrollBar::onChildEvent(int event, int id, GWidget* ui)
{
	if (event == EVT_BUTTON_CLICK)
	{
		if (id == 0) // Minus
		{
			setValue(mValue - 1);
			return false;
		}
		else if (id == 1) // Plus
		{
			setValue(mValue + 1);
			return false;
		}
	}
	return true;
}

void GScrollBar::onResize(Vec2i const& size)
{
	BaseClass::onResize(size);
	updateThumbSize();
	updateThumbPos();
}
