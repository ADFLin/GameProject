#ifndef GoStage_h__
#define GoStage_h__

#include "StageBase.h"
#include "GoCore.h"

#include "GameNetPacket.h"
#include "GameWorker.h"
#include "GameGUISystem.h"

namespace Go
{
	enum PacketID
	{
		CSP_PLAY = GDP_NEXT_ID ,
	};

	class CSPPlay : public GamePacket< CSPPlay  , CSP_PLAY >
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
			evaluator.setUserFun< CSPPlay >( this , &Server::procPlay );
		}
		void procPlay( IComPacket* cp )
		{
			CSPPlay* com = cp->cast< CSPPlay >();

			if ( com->idxPos == -1 )
				getGame().pass();
			else
			{
				Board::Pos pos = getGame().getBoard().getPos( com->idxPos );
				getGame().play( pos );
			}
		}



		Game& getGame();
		Game mGame;
	};

	class Client
	{





		ComWorker* mWorker;
	};

	class Stage : public StageBase
	{
	public:
		virtual bool onInit()
		{
			mGame.setup( 19 );
			mLifeParam = 0;
			::Global::getGUI().cleanupWidget();
			return true; 
		}
		virtual void onEnd(){}
		virtual void onUpdate( long time ){}

		
		virtual void onRender( float dFrame );

		void drawStone( Graphics2D& g , Vec2i const& pos , int color );

		virtual bool onMouse( MouseMsg const& msg );
		virtual bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return true;
			switch( key )
			{
			case 'R': mGame.restart(); break;
			}
			return false;
		}

		Game mGame;
		int  mLifeParam;
	};


}

#endif // GoStage_h__
