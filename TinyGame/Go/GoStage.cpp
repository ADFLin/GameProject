#include "TinyGamePCH.h"
#include "GoStage.h"
#include "Bots/ZenBot.h"

#include "RenderUtility.h"
#include "PlatformThread.h"
#include "SystemPlatform.h"
#include "FixString.h"

namespace Go
{

	int const CellSize = 30;
	Vec2i const BoardPos = Vec2i( 50 , 50 );
	char* CoordStr = "ABCDEFGHJKLMNOPQRSTQV";

	class BotThinkRunnable : public RunnableThreadT< BotThinkRunnable >
	{
	public:
		unsigned run() 
		{ 
			bThinkComplete = true;
			while( 1 )
			{
				if ( bThinkComplete == false )
				{
					while( bot->mCore->isThinking() )
					{
						SystemPlatform::Sleep(0);
					}
					bThinkComplete = true;
				}
			}
			return 0; 
		}

		bool bThinkComplete;
		Zen::Bot* bot;
	};

	int const StarMarkPos[3] = { 3 , 9 , 15 };

	void Stage::onRender( float dFrame )
	{
		Graphics2D& g = ::Global::GetGraphics2D();

		Board const& board = mGame.getBoard();

		int size = board.getSize();
		int length = ( size - 1 ) * CellSize;

		int border = 40;
		int boardSize = length + 2 * border;
		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , EColor::Orange );
		g.drawRect( BoardPos - Vec2i( border , border ) , Vec2i( boardSize , boardSize ) );

		Vec2i posV = BoardPos;
		Vec2i posH = BoardPos;
		
		RenderUtility::SetFont( g , FONT_S12 );
		g.setTextColor(Color3ub(0 , 0 , 0) );
		for( int i = 0 ; i < size ; ++i )
		{
			g.drawLine( posV , posV + Vec2i( 0 , length ) );
			g.drawLine( posH , posH + Vec2i( length , 0 ) );

			FixString< 64 > str;
			str.format( "%2d" , i + 1 );
			g.drawText( posH - Vec2i( 30 , 8 ) , str );
			g.drawText( posH + Vec2i( 12 + length , -8 ) , str );

			str.format( "%c" , CoordStr[i] );
			g.drawText( posV - Vec2i( 5 , 30 ) , str );
			g.drawText( posV + Vec2i( -5 , 15 + length ) , str );

			posV.x += CellSize;
			posH.y += CellSize;
		}

		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , EColor::Black );

		int const starRadius = 5;
		switch( size )
		{
		case 19:
			{
				Vec2i pos;
				for( int i = 0 ; i < 3 ; ++i )
				{
					pos.x = BoardPos.x + StarMarkPos[ i ] * CellSize;
					for( int j = 0 ; j < 3 ; ++j )
					{
						pos.y = BoardPos.y + StarMarkPos[ j ] * CellSize;
						g.drawCircle( pos , starRadius );
					}
				}
			}
			break;
		case 13:
			{
				Vec2i pos;
				for( int i = 0 ; i < 2 ; ++i )
				{
					pos.x = BoardPos.x + StarMarkPos[ i ] * CellSize;
					for( int j = 0 ; j < 2 ; ++j )
					{
						pos.y = BoardPos.y + StarMarkPos[ j ] * CellSize;
						g.drawCircle( pos , starRadius );
					}
				}
				g.drawCircle( BoardPos + CellSize * Vec2i( 6 , 6 ) , starRadius );
			}
			break;
		}

		for( int i = 0 ; i < size ; ++i )
		{
			for( int j = 0 ; j < size ; ++j )
			{
				int data = board.getData( i , j );
				Vec2i pos = BoardPos + CellSize * Vec2i(i, j);
				if ( data )
				{
					drawStone( g , pos , data );
				}
				mMode.draw(g, pos, i, j);
			}
		}

		RenderUtility::SetFont( g , FONT_S8 );
		g.setTextColor(Color3ub(255 , 255 , 0) );
		FixString< 64 > str;
		str.format( "life = %d" , mCaptureCount );
		g.drawText( Vec2i( 5 , 5 ) , str );
		str.format( "B = %d | W = %d" , mGame.getWhiteCapturedNum() , mGame.getBlackCapturedNum() );
		g.drawText( Vec2i( 5 , 5 + 15 ) , str );
	}

	bool Stage::onMouse( MouseMsg const& msg )
	{
		Vec2i lPos = ( msg.getPos() - BoardPos + Vec2i( CellSize , CellSize ) / 2 ) / CellSize;

		int size = mGame.getBoard().getSize();

		if ( 0 <= lPos.x && lPos.x < size && 
			 0 <= lPos.y && lPos.y < size )
		{
			Board::Pos bPos = mGame.getBoard().getPos( lPos.x , lPos.y );
			if ( msg.onLeftDown() )
			{
				mGame.playStone( bPos );
			}
			else if ( msg.onMiddleDown() )
			{
				const_cast< Board& >( mGame.getBoard() ).fillLinkedStone(  bPos , StoneColor::eBlack );
			}
			else if ( msg.onRightDown() || msg.onRightDClick() )
			{
				mGame.undo();
			}
			else if ( msg.onMoving() )
			{
				if ( mGame.getBoard().getData( bPos ) )
				{
					mCaptureCount = mGame.getBoard().getCaptureCount( bPos );
				}
			}
		}

		return false;
	}

	void Stage::drawStone( Graphics2D& g , Vec2i const& pos , int color )
	{
		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g ,( color == StoneColor::eBlack ) ? EColor::Black : EColor::White );
		g.drawCircle( pos ,  CellSize / 2  );
		if ( color == StoneColor::eBlack )
		{
			RenderUtility::SetBrush( g ,EColor::White );
			g.drawCircle( pos + Vec2i( 5 , -5 ) , 3 );
		}
	}

	bool BotTestMode::init()
	{
		int numCPU = SystemPlatform::GetProcessorNumber();

		Zen::CoreSetting setting;
		setting.numThreads = numCPU - 1;
		setting.numSimulations = 10000000;
		setting.maxTime = 20;


		if( !ZenCoreV4::Get().initialize() )
			return false;
		ZenCoreV4::Get().setCoreSetting(setting);

		if( !ZenCoreV6::Get().initialize() )
			return false;
		setting.maxTime /= 2;
		ZenCoreV6::Get().setCoreSetting(setting);

		botA.setup(ZenCoreV4::Get());
		botB.setup(ZenCoreV6::Get());
		return true;
	}

	void BotTestMode::setupGame(Game& game)
	{
		mGame = &game;

		Zen::GameSetting gameSetting;
		gameSetting.boardSize = game.getBoard().getSize();
		gameSetting.bBlackFirst = game.getFirstPlayColor() == StoneColor::eBlack;

		botA.startGame(gameSetting);
		botB.startGame(gameSetting);

		players[0] = &botA;
		players[1] = &botB;

		passCount = 0;
		idxCur = 0;
		game.restart();
		bGameEnd = false;

		nextStep();
	}

	void BotTestMode::nextStep()
	{
		ZenCoreV6::Get().getTerritoryStatictics(territoryStatictics);
		ZenCoreV6::Get().getPriorKnowledge(priorKnowledge);

		players[idxCur]->mCore->startThink();
	}

	void BotTestMode::update()
	{
		if( bGameEnd )
			return;

		Zen::Bot* curPlayer = players[idxCur];
		Zen::Color color = ZenCoreV6::Get().getNextColor();

		if( curPlayer->mCore->isThinking() )
			return;

		Zen::ThinkResult thinkStep;
		curPlayer->mCore->getThinkResult(thinkStep);

		if( thinkStep.bPass )
		{
			botA.playPass(color);
			botB.playPass(color);
			mGame->playPass();
			++passCount;
			if( passCount == 2 )
			{
				bGameEnd = true;
				return;
			}
		}
		else
		{
			if( !botA.playStone(thinkStep.x, thinkStep.y, color) ||
			   !botB.playStone(thinkStep.x, thinkStep.y, color) ||
			   !mGame->playStone(thinkStep.x, thinkStep.y) )
			{
				bGameEnd = true;
				return;
			}
			passCount = 0;
		}

		idxCur = 1 - idxCur;

		nextStep();
	}

	void BotTestMode::draw(Graphics2D& g, Vec2i const& pos, int x, int y)
	{
		if ( bDrawTerritoryStatictics )
		{
			int value = territoryStatictics[y][x];
			if( value )
			{
				int color = value > 0 ? EColor::Red : EColor::Blue;

				if( !( value > 0 && mGame->getBoard().getData(x, y) == StoneColor::eBlack ||
				      value < 0 && mGame->getBoard().getData(x, y) == StoneColor::eWhite ) )
				{
					int size = 18 * value / 1000;
					RenderUtility::SetBrush(g, color);
					g.drawRect(pos - Vec2i(size, size) / 2, Vec2i(size, size));
				}
			}
		}

		if( bDrawPriorKnowledge )
		{
			int value = priorKnowledge[y][x] - territoryStatictics[y][x];

			int threshold = 500;
			if( mGame->getNextPlayColor() == StoneColor::eBlack )
			{
				if( value > 0 )
				{
					//	return;
					//int color = value > 0 ? EColor::eRed : EColor::Blue;

					int size = 18 * value / 1000;
					RenderUtility::SetBrush(g, EColor::Green);
					g.drawRect(pos - Vec2i(size, size) / 2, Vec2i(size, size));
				}

			}
			else
			{
				if( value > 0 )
				{
					int size = 18 * value / 1000;
					RenderUtility::SetBrush(g, EColor::Green);
					g.drawRect(pos - Vec2i(size, size) / 2, Vec2i(size, size));
				}
			}
#if 1
			RenderUtility::SetFontColor(g, EColor::Purple);
			RenderUtility::SetFont(g, FONT_S8);
			FixString< 128 > str;
			str.format("%d", value);
			g.drawText(pos, str);
#endif

		}
	}

}//namespace Go