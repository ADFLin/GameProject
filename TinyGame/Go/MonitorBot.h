#ifndef MonitorBot_h__
#define MonitorBot_h__

#include "GoBot.h"

namespace Go
{
	class MonitorBoard
	{
	public:
		bool initialize()
		{

			return false;
		}
		HWND getHookWinodow()
		{
			return NULL;
		}
	};

	class MonitorBot : public IBotInterface
	{
	public:
		virtual bool initialize(void* settingData) override
		{
			if( !mBoard.initialize() )
				return false;
			
			return true;
		}


		virtual void destroy() override
		{


		}


		virtual bool setupGame(GameSetting const& setting) override
		{
			
			return true;
		}


		virtual bool restart() override
		{
			return false;
		}


		virtual bool playStone(int x, int y, int color) override
		{

		}

		virtual bool playPass(int color) override
		{
			//

		}

		virtual bool undo() override
		{
			return false;
		}


		virtual bool requestUndo() override
		{
			return false;
		}


		virtual bool thinkNextMove(int color) override
		{
			return true;
		}


		virtual bool isThinking() override
		{
			return false;
		}


		virtual void update(IGameCommandListener& listener) override
		{


		}
		MonitorBoard mBoard;
	};

}//namespace Go

#endif // MonitorBot_h__
