#ifndef GoStage_h__
#define GoStage_h__

#include "StageBase.h"
#include "GoCore.h"

#include "GameNetPacket.h"
#include "GameWorker.h"
#include "GameGUISystem.h"

#include "Bots/ZenBot.h"

namespace Go
{
	enum PacketID
	{
		CSP_PLAY = GDP_NEXT_ID ,
	};

	class CSPPlay : public GamePacketT< CSPPlay  , CSP_PLAY >
	{
	public:
		short idxPos;
		template < class BufferOP >
		void  operateBuffer( BufferOP& op )
		{
			op & idxPos;
		}
	};

	class Server
	{
	public:
		void setup( NetWorker* worker )
		{
			ComEvaluator& evaluator = worker->getEvaluator();
			evaluator.setUserFunc< CSPPlay >( this , &Server::procPlay );
		}
		void procPlay( IComPacket* cp)
		{
			CSPPlay* com = cp->cast< CSPPlay >();

			if ( com->idxPos == -1 )
				getGame().playPass();
			else
			{
				Board::Pos pos = getGame().getBoard().getPos( com->idxPos );
				getGame().playStone( pos );
			}
		}



		Game& getGame();
		Game mGame;
	};

	class Client
	{





		ComWorker* mWorker;
	};

	class GameMode
	{


	};

	class BotTestMode : public GameMode
	{
	public:
		BotTestMode()
		{
			bGameEnd = true;
			bDrawPriorKnowledge = true;
			bDrawTerritoryStatictics = true;
		}
		typedef Zen::TBotCore< 4 > ZenCoreV4;
		typedef Zen::TBotCore< 6 > ZenCoreV6;
		bool init();

		void setupGame(Game& game);

		void nextStep();

		void update();
		void exit()
		{
			if( !bGameEnd && players[idxCur]->mCore->isThinking() )
			{
				players[idxCur]->mCore->stopThink();
			}
			ZenCoreV4::Get().release();
			ZenCoreV6::Get().release();
		}


		void  draw(Graphics2D& g, Vec2i const& pos, int x, int y);
		bool  bDrawTerritoryStatictics;
		bool  bDrawPriorKnowledge;

		int   territoryStatictics[19][19];
		int   priorKnowledge[19][19];
		Game* mGame;
		bool  bGameEnd;
		int   passCount;
		int   idxCur;
		Zen::Bot* players[2];
		Zen::Bot botA;
		Zen::Bot botB;
	};



	class Stage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		virtual bool onInit()
		{
			if( !mMode.init() )
				return false;

			mGame.setup(13);
			mCaptureCount = 0;
			mMode.setupGame(mGame);

			::Global::GUI().cleanupWidget();
			return true; 
		}
		virtual void onEnd()
		{
			mMode.exit();
		}
		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			BaseClass::onUpdate(deltaTime);
			mMode.update();
		}
		virtual void onRender( float dFrame );

		void drawStone( Graphics2D& g , Vec2i const& pos , int color );

		virtual MsgReply onMouse( MouseMsg const& msg );
		virtual MsgReply onKey( KeyMsg const& msg )
		{
			if ( msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: mGame.restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}
		BotTestMode mMode;
		Game        mGame;
		int         mCaptureCount;
	};


}

#endif // GoStage_h__
