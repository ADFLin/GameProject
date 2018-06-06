#include "ConsoleFrame.h"

ConsoleFrame::~ConsoleFrame()
{

}

void ConsoleFrame::onRender()
{
	BaseClass::onRender();

	Vec2i pos = getWorldPos();
	IGraphics2D& g = ::Global::getIGraphics2D();

	int numLines = getSize().y / 12;
	RenderUtility::SetFont(g, FONT_S8);

	int lineCount = 0;
	for( auto const& line : mLines )
	{
		g.setTextColor(line.color);
		g.drawText(pos, line.content.c_str());
		pos.y += 12;

		++lineCount;
		if( lineCount >= numLines )
			break;
	}
}
