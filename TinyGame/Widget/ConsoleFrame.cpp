#include "ConsoleFrame.h"

#include "ConsoleSystem.h"
#include <unordered_set>

ConsoleFrame::ConsoleFrame(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent) :BaseClass(id, pos, size, parent)
{
	setRenderType( GPanel::eRectType );

	mCmdText = new GTextCtrl(UI_COM_TEXT, Vec2i(3, size.y - 3 - GTextCtrl::UI_Height ), size.x - 6, this);
	mCmdText->setRerouteCharMsg();
	mCmdText->setRerouteKeyMsg();
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
		if( mIndexFoundCmdUsed == INDEX_NONE )
		{
			char const* foundCmds[256];
			int numFound = ConsoleSystem::Get().findCommandName2(mCmdText->getValue(), foundCmds, ARRAY_SIZE(foundCmds));
			if( numFound != 0 )
			{
				bool bNeedAddNamespace = *FCString::FindChar(mCmdText->getValue(), '.') == 0;
				if (bNeedAddNamespace)
				{
					mFoundCmds.clear();
					std::vector< std::string > namespaceUsed;

					for (int index = 0; index < numFound; ++index)
					{
						char const* nsEnd = FCString::FindChar(foundCmds[index], '.');
						if (*nsEnd)
						{
							std::string ns = { foundCmds[index] , nsEnd + 1 };
							auto iter = std::find( namespaceUsed.begin(), namespaceUsed.end(), ns );
							if (iter == namespaceUsed.end())
							{
								mFoundCmds.push_back(ns);
								namespaceUsed.push_back(ns);
							}
						}

						mFoundCmds.push_back(foundCmds[index]);
					}
				}
				else
				{
					mFoundCmds.assign(foundCmds, foundCmds + numFound);
				}

				mIndexFoundCmdUsed = 0;
			}
		}
		else
		{
			mIndexFoundCmdUsed = (mIndexFoundCmdUsed + 1) % mFoundCmds.size();
		}

		if( mIndexFoundCmdUsed != INDEX_NONE)
		{
			mCmdText->setValue(mFoundCmds[mIndexFoundCmdUsed].c_str());
			if (mFoundCmds[mIndexFoundCmdUsed].back() != '.')
				mCmdText->appendValue(" ");
		}
		return false;
	}
	else if (msg.getCode() == EKeyCode::Up)
	{
		++mIndexHistoryUsed;
		if (mIndexHistoryUsed < mHistoryCmds.size())
		{
			mCmdText->setValue(mHistoryCmds[mIndexHistoryUsed].c_str());
		}
		else
		{
			mIndexHistoryUsed = mHistoryCmds.size() - 1;
		}
	}
	else if (msg.getCode() == EKeyCode::Down)
	{
		--mIndexHistoryUsed;
		if (mIndexHistoryUsed >= 0)
		{
			mCmdText->setValue(mHistoryCmds[mIndexHistoryUsed].c_str());
		}
		else
		{
			mIndexHistoryUsed = 0;
		}
	}
	bool result = mCmdText->onKeyMsg(msg);

	if( msg.getCode() == EKeyCode::Return )
	{

	}

	return false;
}

bool ConsoleFrame::onCharMsg(unsigned code)
{
	//eat tab char and ~
	if( code == EKeyCode::Tab || code == 0x60 )
	{
		return false;
	}
	mIndexFoundCmdUsed = INDEX_NONE;
	return mCmdText->onCharMsg(code);
}

bool ConsoleFrame::onChildEvent(int event, int id, GWidget* ui)
{
	switch( id )
	{
	case UI_COM_TEXT:
		if( event == EVT_TEXTCTRL_COMMITTED )
		{
			char const* comStr = ui->cast<GTextCtrl>()->getValue();
			if( comStr && *comStr != 0 )
			{
				bool result = ConsoleSystem::Get().executeCommand(comStr);

				if (mIndexHistoryUsed != INDEX_NONE && mHistoryCmds[mIndexHistoryUsed] == comStr)
				{
					--mIndexHistoryUsed;
				}
				else
				{
					mHistoryCmds.push_back(comStr);
					mIndexHistoryUsed = INDEX_NONE;
				}

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
