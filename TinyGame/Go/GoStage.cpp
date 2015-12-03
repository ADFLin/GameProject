#include "TinyGamePCH.h"
#include "GoStage.h"

#include "RenderUtility.h"

namespace Go
{

	int const CellSize = 27;
	Vec2i const BoardPos = Vec2i( 50 , 50 );
	char* CoordStr = "ABCDEFGHJKLMNOPQRSTQV";

	int const StarMarkPos[3] = { 3 , 9 , 15 };

	void Stage::onRender( float dFrame )
	{
		Graphics2D& g = ::Global::getGraphics2D();

		Board const& board = mGame.getBoard();

		int size = board.getSize();
		int length = ( size - 1 ) * CellSize;

		int border = 40;
		int boardSize = length + 2 * border;
		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , Color::eOrange );
		g.drawRect( BoardPos - Vec2i( border , border ) , Vec2i( boardSize , boardSize ) );

		Vec2i posV = BoardPos;
		Vec2i posH = BoardPos;
		
		RenderUtility::setFont( g , FONT_S12 );
		g.setTextColor( 0 , 0 , 0 );
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

		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , Color::eBlack );

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
				if ( data )
				{
					drawStone( g , BoardPos + CellSize * Vec2i( i , j ) , data );
				}
			}
		}

		RenderUtility::setFont( g , FONT_S8 );
		g.setTextColor( 255 , 255 , 0 );
		FixString< 64 > str;
		str.format( "life = %d" , mLifeParam );
		g.drawText( Vec2i( 5 , 5 ) , str );
		str.format( "B = %d | W = %d" , mGame.getWhiteRemovedNum() , mGame.getBlackRemovedNum() );
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
				mGame.play( bPos );
			}
			else if ( msg.onMiddleDown() )
			{
				const_cast< Board& >( mGame.getBoard() ).fillStone(  bPos , Board::eBlack );
			}
			else if ( msg.onRightDown() || msg.onRightDClick() )
			{
				mGame.undo();
			}
			else if ( msg.onMoving() )
			{
				if ( mGame.getBoard().getData( bPos ) )
				{
					mLifeParam = mGame.getBoard().getLife( bPos );
				}
			}
		}

		return false;
	}

	void Stage::drawStone( Graphics2D& g , Vec2i const& pos , int color )
	{
		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g ,( color == Board::eBlack ) ? Color::eBlack : Color::eWhite );
		g.drawCircle( pos ,  CellSize / 2  );
		if ( color == Board::eBlack )
		{
			RenderUtility::setBrush( g ,Color::eWhite );
			g.drawCircle( pos + Vec2i( 5 , -5 ) , 3 );
		}
	}

}//namespace Go