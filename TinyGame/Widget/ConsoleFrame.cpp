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

	int const yOffset = 14;
	int numLines = ( getSize().y - 30 ) / yOffset;
	RenderUtility::SetFont(g, FONT_S10);

	int lineCount = 0;
	int startLineIndex = Math::Max(0, mLines.size() - numLines);
	for( int index = startLineIndex; index < mLines.size() ; ++index )
	{
		auto const& line = mLines[index];

		g.setTextColor(line.color);
		g.drawText(pos, line.content.c_str());
		pos.y += yOffset;

	}
}

bool ConsoleFrame::onKeyMsg(KeyMsg const& msg)
{
	if (!msg.isDown())
		return false;

	if( msg.getCode() == EKeyCode::Tab )
	{
		if( mIndexFoundComUsed == INDEX_NONE )
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

		if( mIndexFoundComUsed != INDEX_NONE)
		{
			mComText->setValue(mFoundComs[mIndexFoundComUsed].c_str());
			mComText->appendValue(" ");
		}
		return false;
	}
	else if (msg.getCode() == EKeyCode::Up)
	{
		++mIndexHistoryUsed;
		if (mIndexHistoryUsed < mHistoryComs.size())
		{
			mComText->setValue(mHistoryComs[mIndexHistoryUsed].c_str());
		}
		else
		{
			mIndexHistoryUsed = mHistoryComs.size() - 1;
		}
	}
	else if (msg.getCode() == EKeyCode::Down)
	{
		--mIndexHistoryUsed;
		if (mIndexHistoryUsed >= 0)
		{
			mComText->setValue(mHistoryComs[mIndexHistoryUsed].c_str());
		}
		else
		{
			mIndexHistoryUsed = 0;
		}
	}
	bool result = mComText->onKeyMsg(msg);

	if( msg.getCode() == EKeyCode::Return )
	{

	}

	return false;
}

bool ConsoleFrame::onCharMsg(unsigned code)
{
	//eat tab char and ~
	if( code == 0x09 || code == 0x60 )
	{
		return false;
	}
	mIndexFoundComUsed = INDEX_NONE;
	return mComText->onCharMsg(code);
}

bool ConsoleFrame::onChildEvent(int event, int id, GWidget* ui)
{
	switch( id )
	{
	case UI_COM_TEXT:
		if( event == EVT_TEXTCTRL_COMMITTED )
		{
			char const* comStr = ui->cast<GTextCtrl>()->getValue();
			if( comStr && comStr != 0 )
			{
				bool result = ConsoleSystem::Get().executeCommand(comStr);

				mHistoryComs.push_back(comStr);
				mIndexHistoryUsed = -1;
				ui->cast<GTextCtrl>()->clearValue();
				makeFocus();
			}
		}
		else if( event == EVT_TEXTCTRL_VALUE_CHANGED )
		{


		}
		return false;
	default:
		break;
	}

	return true;
}
