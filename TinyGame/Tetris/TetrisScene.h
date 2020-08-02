#ifndef TetrisScene_h__
#define TetrisScene_h__

#include "TetrisLevel.h"
#include "GameWidget.h"
#include "GameControl.h"
#include "RenderLayer.h"

class  ActionTrigger;
class  Graphics2D;

namespace Tetris
{
	class  LevelMode;
	class  LevelData;

	class Scene : public RenderLayer
	{
	public:
		Scene( Level* level );

		Level* getLevel(){ return mLevel; }
		void   restart();
		void   updateFrame( int frame );
		void   render( Graphics2D& g , LevelMode* levelMode );

		void   showFallPosition( bool beS ){  mShowFallPosition = beS;  }


		void   notifyMovePiece( int x );
		void   notifyRotatePiece( int orgDir , bool beCW );
		void   notifyHoldPiece();
		void   notifyFallPiece();
		void   notifyMoveDown();



		static void renderPiece( Graphics2D& g ,Piece const& piece , Vec2i const& pos );

		

		void  renderHoldPiece( GWidget* ui );
		void  renderPieceStorage( GWidget* ui );

		void  notifyChangePiece();
		void  notifyMarkPiece( int layer[] , int numLayer );
		void  notifyRemoveLayer( int layer[] , int numLayer );

	protected:
		Vec2i calcBlockPos( Vec2i const& org , int i , int j );
		void renderPiece( Graphics2D& g , Piece const& piece , Vec2i const& pos , int nx , int ny );
		void renderNextPieceInternal( Graphics2D& g , Vec2i const& pos );
		void renderCurPiece( Graphics2D& g , Vec2i const& mapPos );
		void renderLayer( Graphics2D& g , Vec2i const& pos , int j );
		void renderConMapMask( Graphics2D& g , Vec2i const& pos );
		void renderConnectBlock( Graphics2D& g , Vec2i const& pos , bool useRemoveLayer , float offset = 0.0f );
		void renderBlockMap( Graphics2D& g , Vec2i const& pos );
		void renderBackground( Graphics2D& g , Vec2i const& pos );
		static void calcPieceRnageY( Piece const& piece , int& max , int& min );

		void  calcNextPieceOffset();
	private:

		static int const MaxBlurNum = 5;
		bool   mShowFallPosition;
		int    mYPosShowFall;
		float  mNextPieceOffset;
		int    mNextPieceOffset2[ 3 ];

		float  mPieceDownOffset;
		float  mAngle;
		static int const RenderNextPieceNum = 3;
		Level* mLevel;
	};


}//namespace Tetris

#endif // TetrisScene_h__