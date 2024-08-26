#ifndef ConsoleFrame_h__
#define ConsoleFrame_h__

#include "GameWidget.h"

#include "TypeMemoryOp.h"
#include "DataStructure/CycleQueue.h"

class ConsoleCmdTextCtrl : public GTextCtrl
{
	using BaseClass = GTextCtrl;
public:
	using GTextCtrl::GTextCtrl;

	TINY_API MsgReply onCharMsg(unsigned code);
	TINY_API MsgReply onKeyMsg(KeyMsg const& msg);


	TINY_API void setFoundCmdToText();
	TArray< std::string > mFoundCmds;
	TArray< std::string > mHistoryCmds;
	int mIndexHistoryUsed = INDEX_NONE;
	int mIndexFoundCmdUsed = INDEX_NONE;

	std::string mNamespace;

	TINY_API bool preSendEvent(int event);

};

class ConsoleFrame : public GFrame
	               , public LogOutput
{
	typedef GFrame BaseClass;

public:

	TINY_API ConsoleFrame( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent );
	TINY_API ~ConsoleFrame();
	enum 
	{
		UI_COM_TEXT = BaseClass::NEXT_UI_ID ,
	};

	virtual void onRender();
	virtual MsgReply onKeyMsg(KeyMsg const& msg);
	virtual MsgReply onCharMsg(unsigned code);
	virtual bool onChildEvent(int event, int id, GWidget* ui);
	struct ComLine
	{
		Color3ub    color;
		std::string content;

		template< class S >
		ComLine(Color3ub color , S&& s )
			:color( color ) , content( std::forward< S >( s )){}
	};

	TCycleBuffer< ComLine , 37 > mLines;
	ConsoleCmdTextCtrl* mCmdText;

public:
	//LogOutput
	virtual bool filterLog(LogChannel channel, int level) override
	{
		return true;
	}
	virtual void receiveLog(LogChannel channel, char const* str) override
	{
		Color3ub color = RenderUtility::GetColor( EColor::White );
		switch( channel )
		{
		case LOG_MSG: color = RenderUtility::GetColor(EColor::White); break;
		case LOG_DEV: color = RenderUtility::GetColor(EColor::Yellow , COLOR_LIGHT); break;
		case LOG_WARNING: color = RenderUtility::GetColor(EColor::Orange, COLOR_DEEP); break;
		case LOG_ERROR:   color = RenderUtility::GetColor(EColor::Red, COLOR_DEEP); break;
		}
		
		mLines.emplace(color, str);
	}
	//~LogOutput
};

#endif // ConsoleFrame_h__
