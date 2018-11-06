#include "ConsoleFrame.h"

#include "ConsoleSystem.h"

ConsoleFrame::ConsoleFrame(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent) :BaseClass(id, pos, size, parent)
{
	mComText = new GTextCtrl(UI_COM_TEXT, Vec2i(3, size.y - 3 - GTextCtrl::UI_Height ), size.x - 6, this);
	mComText->setRerouteCharMsg();
	mComText->setRerouteKeyMsg();
}

ConsoleFrame::~ConsoleFrame()
{

}

void ConsoleFrame::onRender()
{
	BaseClass::onRender();

	Vec2i pos = getWorldPos();
	pos += Vec2i(5, 5);
	IGraphics2D& g = ::Global::GetIGraphics2D();

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

bool ConsoleFrame::onKeyMsg(unsigned key, bool isDown)
{
	if( key == Keyboard::eTAB )
	{
		if( mIndexFoundComUsed == -1 )
		{
			char const* foundComs[256];
			int numFound = ConsoleSystem::Get().findCommandName2(mComText->getValue(), foundComs, ARRAY_SIZE(foundComs));
			if( numFound != 0 )
			{
				mFoundComs.assign(foundComs, foundComs + numFound);
				mIndexFoundComUsed = 0;
			}
		}
		else
		{
			mIndexFoundComUsed = (mIndexFoundComUsed + 1) % mFoundComs.size();
		}

		if( mIndexFoundComUsed != -1 )
		{
			mComText->setValue(mFoundComs[mIndexFoundComUsed].c_str());
			mComText->appendValue(" ");
		}
		return false;
	}
	bool result = mComText->onKeyMsg(key, isDown);

	if( key == Keyboard::eRETURN )
	{

	}

	return true;
}

bool ConsoleFrame::onCharMsg(unsigned code)
{
	//eat tab char and ~
	if( code == 0x09 || code == 0x60 )
	{
		return false;
	}
	mIndexFoundComUsed = -1;
	return mComText->onCharMsg(code);
}

bool ConsoleFrame::onChildEvent(int event, int id, GWidget* ui)
{
	switch( id )
	{
	case UI_COM_TEXT:
		if( event == EVT_TEXTCTRL_ENTER )
		{
			char const* comStr = ui->cast<GTextCtrl>()->getValue();
			if( comStr && comStr != 0 )
			{
				bool result = ConsoleSystem::Get().executeCommand(comStr);
				ui->cast<GTextCtrl>()->clearValue();
				makeFocus();
			}
		}
		else if( event == EVT_TEXTCTRL_CHANGE )
		{


		}
		return false;
	default:
		break;
	}
}
