#include "ConsoleFrame.h"

#include "ConsoleSystem.h"
#include <unordered_set>

ConsoleFrame::ConsoleFrame(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent) :BaseClass(id, pos, size, parent)
{
	setRenderType( GPanel::eRectType );
	mCmdText = new ConsoleCmdTextCtrl(UI_COM_TEXT, Vec2i(3, size.y - 3 - GTextCtrl::UI_Height), size.x - 6, this);
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

MsgReply ConsoleFrame::onKeyMsg(KeyMsg const& msg)
{
	return mCmdText->onKeyMsg(msg);
}

MsgReply ConsoleFrame::onCharMsg(unsigned code)
{
	return mCmdText->onCharMsg(code);
}

bool ConsoleFrame::onChildEvent(int event, int id, GWidget* ui)
{
	switch( id )
	{
	case UI_COM_TEXT:
		if( event == EVT_TEXTCTRL_COMMITTED )
		{

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

MsgReply ConsoleCmdTextCtrl::onCharMsg(unsigned code)
{
	//eat tab char and ~
	if (code == EKeyCode::Tab || code == 0x60)
	{
		return MsgReply::Handled();
	}
	mIndexFoundCmdUsed = INDEX_NONE;
	return BaseClass::onCharMsg(code);
}


MsgReply ConsoleCmdTextCtrl::onKeyMsg(KeyMsg const& msg)
{
	if (!msg.isDown())
		return MsgReply::Handled();

	if (msg.getCode() == EKeyCode::Tab)
	{
		if (mIndexFoundCmdUsed == INDEX_NONE)
		{
			char const* foundCmds[256];
			int numFound;
			if (mNamespace.empty())
			{
				numFound = IConsoleSystem::Get().findCommandName2(getValue(), foundCmds, ARRAY_SIZE(foundCmds));
			}
			else
			{
				std::string findText = mNamespace + '.' + getValue();
				numFound = IConsoleSystem::Get().findCommandName2(findText.c_str(), foundCmds, ARRAY_SIZE(foundCmds));
			}

			if (numFound != 0)
			{
				bool bNeedAddNamespace = *FCString::FindChar(getValue(), '.') == 0 && mNamespace.empty();
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
							auto iter = std::find(namespaceUsed.begin(), namespaceUsed.end(), ns);
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

		if (mIndexFoundCmdUsed != INDEX_NONE)
		{
			setFoundCmdToText();
		}
		return MsgReply::Handled();
	}
	else if (msg.getCode() == EKeyCode::Up)
	{
		++mIndexHistoryUsed;
		if (mIndexHistoryUsed < mHistoryCmds.size())
		{
			setValue(mHistoryCmds[mIndexHistoryUsed].c_str());
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
			setValue(mHistoryCmds[mIndexHistoryUsed].c_str());
		}
		else
		{
			mIndexHistoryUsed = 0;
		}
	}
	MsgReply reply = BaseClass::onKeyMsg(msg);

	if (msg.getCode() == EKeyCode::Return)
	{

	}

	return reply;
}

void ConsoleCmdTextCtrl::setFoundCmdToText()
{
	auto& str = mFoundCmds[mIndexFoundCmdUsed];

	if (mNamespace.empty() == false &&
		FCString::CompareN(str.c_str(), mNamespace.c_str(), mNamespace.length()) == 0)
	{
		setValue(str.c_str() + mNamespace.length() + 1);
		appendValue(" ");
		return;
	}

	setValue(str.c_str());

	if (str.back() != '.')
		appendValue(" ");
}

bool ConsoleCmdTextCtrl::preSendEvent(int event)
{
	if (event == EVT_TEXTCTRL_COMMITTED)
	{
		char const* cmdText = getValue();
		if (cmdText && *cmdText != 0)
		{
			bool cmdResult;
			if (mNamespace.empty())
			{
				cmdResult = IConsoleSystem::Get().executeCommand(cmdText);
			}
			else
			{
				std::string cmd = mNamespace + '.' + getValue();
				cmdResult = IConsoleSystem::Get().executeCommand(cmd.c_str());
			}
			
			if (mIndexHistoryUsed != INDEX_NONE && mHistoryCmds[mIndexHistoryUsed] == cmdText)
			{
				--mIndexHistoryUsed;
			}
			else
			{
				mHistoryCmds.push_back(cmdText);
				mIndexHistoryUsed = INDEX_NONE;
			}

			clearValue();
			makeFocus();
		}
	}
	else if (event == EVT_TEXTCTRL_VALUE_CHANGED)
	{


	}

	return true;
}
