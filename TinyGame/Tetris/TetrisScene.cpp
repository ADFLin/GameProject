#include "TetrisPCH.h"
#include "TetrisScene.h"

#include "TetrisLevelManager.h"
#include "GameWidget.h"

#include "RenderUtility.h"
#include <cmath>
#include <algorithm>

#include "ConsoleSystem.h"

TConsoleVariable< bool > CVarTetrisUseRotateAnim( true , "Tetris.UseRotateAnim" , CVF_TOGGLEABLE );

namespace Tetris
{

	Scene::Scene( Level* level )
		:mLevel( level )
	{
		mShowFallPosition = false;
	}

	void Scene::restart()
	{
		mAngle = 0;
		mPieceDownOffset = 0;
		mNextPieceOffset = 0;

		if ( mShowFallPosition )
		{
			mYPosShowFall = getLevel()->calcFallPosY();
		}
	}

	void Scene::updateFrame( int frame )
	{
		int time = frame * gDefaultTickTime;
		float delta = 1.2f * time;

		if ( mAngle != 0 )
		{
			if ( mAngle > 0 )
			{
				mAngle = std::max( mAngle - delta , 0.0f );
			}
			else
			{
				mAngle = std::min( mAngle + delta , 0.0f );
			}
		}

		long countCurG = ( 1000 * Level::GravityUnit ) / getLevel()->getGravityValue();

		mPieceDownOffset = BlockSize * ( countCurG - getLevel()->mCountMove ) / float( countCurG );

		if ( mNextPieceOffset != 0 )
		{
			mNextPieceOffset -= 0.6f * time;
			if ( mNextPieceOffset < 0 )
				mNextPieceOffset = 0;
		}

	}

	void Scene::renderConMapMask(IGraphics2D& g , Vec2i const& pos )
	{
		g.setBrush( Color3ub(0,0,0) );

		BlockStorage& strage = getLevel()->getBlockStorage();

		int extendMapSizeY = strage.getExtendSizeY();

		for (int i = 0;  i < strage.getSizeX(); ++i)
		{
			for (int j = 0 ; j < extendMapSizeY ; ++j)
			{
				if ( strage.mConMap[j][i] == 0 )
					continue;
				Vec2i bPos = calcBlockPos( pos , i , j );
				g.drawRect( bPos , Vec2i( BlockSize + 1, BlockSize + 1 )  );	
			}
		}
	}

	void Scene::renderConnectBlock(IGraphics2D& g , Vec2i const& pos , bool useRemoveLayer , float offset )
	{
		int nR = 0;

		int extendMapSizeY = mLevel->getBlockStorage().getExtendSizeY();
		int  mapSizeX = getLevel()->getBlockStorage().getSizeX();

		int* removeLayer = getLevel()->getLastRemoveLayer();
		int  numRemoveLayer = getLevel()->getLastRemoveLayerNum();

		for( int layer = extendMapSizeY  - 1; layer >= 0 ; --layer )
		{
			bool checkTop = true;

			if ( useRemoveLayer )
			{
				if ( removeLayer[ nR ] == layer )
					continue;

				//layer of top is removed
				if ( removeLayer[ nR ] == layer + 1 )
				{
					++nR;
					if ( removeLayer[ nR ] == layer )
						continue;
					else checkTop = false;
				}
			}

			Vec2i uPos = pos;

			if ( useRemoveLayer )
			{
				if ( removeLayer[ nR ] == layer )
					continue;

				//layer of top is removed
				if ( removeLayer[ nR ] == layer + 1 )
				{
					++nR;
					if ( removeLayer[ nR ] == layer )
						continue;
					else checkTop = false;
				}

				uPos += Vec2i( 0 , int( ( numRemoveLayer - nR ) * offset ) );
			}

			for( int i = 0; i < mapSizeX ; ++i )
			{
				int data = getLevel()->getBlock( i , layer );
				if ( data == 0 )
					continue;

				Vec2i bPos = calcBlockPos( uPos , i , layer );
				int color = Piece::Color( data );

				int dataR = getLevel()->getBlock( i + 1 , layer );
				int dataT = ( layer == extendMapSizeY  - 1 ) ? 0 : getLevel()->getBlock(  i , layer + 1 );

				bool beCon = false;

				RenderUtility::SetPen( g , EColor::Null );

				if ( g.isUseRHI() )
				{
					if (color == Piece::Color(dataR) && i != mapSizeX - 1)
					{
						RenderUtility::SetBrush(g, color, COLOR_DEEP);
						g.drawRect(
							bPos + Vec2i(BlockSize - 5, 0),
							Vec2i(10, BlockSize));

						RenderUtility::SetBrush(g, color);
						g.drawRect(
							bPos + Vec2i(BlockSize - 5, 3),
							Vec2i(10, BlockSize - 6));
						beCon = true;
					}


					if (color == Piece::Color(dataT) && layer != extendMapSizeY - 1 && checkTop)
					{
						RenderUtility::SetBrush(g, color, COLOR_DEEP);
						g.drawRect(
							bPos + Vec2i(0, -5),
							Vec2i(BlockSize, 10));

						RenderUtility::SetBrush(g, color);
						g.drawRect(
							bPos + Vec2i(3, -5),
							Vec2i(BlockSize - 6, 10));
						beCon = true;

					}

				}
				else
				{
					if (color == Piece::Color(dataR) && i != mapSizeX - 1)
					{
						RenderUtility::SetBrush(g, color, COLOR_DEEP);
						g.drawRect(
							bPos + Vec2i(BlockSize - 5, 1),
							Vec2i(10, BlockSize - 1));

						RenderUtility::SetBrush(g, color);
						g.drawRect(
							bPos + Vec2i(BlockSize - 5, 3),
							Vec2i(10, BlockSize - 5));
						beCon = true;
					}


					if (color == Piece::Color(dataT) && layer != extendMapSizeY - 1 && checkTop)
					{
						RenderUtility::SetBrush(g, color, COLOR_DEEP);
						g.drawRect(
							bPos + Vec2i(1, -5),
							Vec2i(BlockSize - 1, 10));

						RenderUtility::SetBrush(g, color);
						g.drawRect(
							bPos + Vec2i(3, -5),
							Vec2i(BlockSize - 5, 10));
						beCon = true;

					}
				}

				if ( !beCon )
				{
					RenderUtility::SetBrush( g , color , COLOR_LIGHT );
					g.drawRoundRect( bPos + Vec2i( 9 + 1, 9 - 4 )  , Vec2i( 4 , 4 ) , Vec2i(3 , 3 ) );
				}
			}
		}
	}

	void Scene::renderBlockMap(IGraphics2D& g , Vec2i const& pos )
	{
		int extendMapSizeY = mLevel->getBlockStorage().getExtendSizeY();
		for(int j=0;j < extendMapSizeY ;++j)
		{
			if ( getLevel()->isEmptyLayer( j ) )
				continue;
			renderLayer( g , pos  , j );
		}
	}

	void Scene::renderLayer(IGraphics2D& g , Vec2i const& pos  , int j )
	{
		BlockType const* layerData = getLevel()->getLayer(j);
		for(int i = 0 ; i < mLevel->getBlockStorage().getSizeX() ; ++i )
		{
			short color = Piece::Color( layerData[i] );
			if (color)
			{
				Vec2i const& bPos  = calcBlockPos( pos , i , j );
				RenderUtility::DrawBlock( g , bPos , color );
			}
		}
	}

	Vec2i Scene::calcBlockPos( Vec2i const& org , int i , int j )
	{
		return org + BlockSize * Vec2i( i , ( mLevel->getBlockStorage().getSizeY() - j ) - 2 );
	}

	void Scene::renderPiece(IGraphics2D& g , Piece const& piece , Vec2i const& pos )
	{
		for( int i = 0 ; i < piece.getBlockNum(); ++i )
		{
			PieceBlock const& block = piece.getBlock(i);
			Vec2i bPos = pos + BlockSize * Vec2i( block.getX() , - block.getY() );
			RenderUtility::DrawBlock( g , bPos , Piece::Color( block.getType() ) );
		}
	}

	void Scene::renderPiece(IGraphics2D& g , Piece const& piece , Vec2i const& pos , int nx , int ny )
	{
		for( int i = 0 ; i < piece.getBlockNum(); ++i )
		{
			PieceBlock const& block = piece.getBlock(i);
			Vec2i bPos = calcBlockPos( pos , block.getX() + nx , block.getY() + ny );
			RenderUtility::DrawBlock( g , bPos , Piece::Color( block.getType() ) );
		}
	}

	void Scene::renderHoldPiece( GWidget* ui )
	{
		Vec2i pos = ui->getWorldPos();
		IGraphics2D& g = Global::GetIGraphics2D();

		RenderUtility::SetFont( g , FONT_S12 );
		g.setTextColor( Color3ub(255 , 255 , 0) );
		g.drawText( Vector2( pos.x + 16 , pos.y + 10 ) , "Next:" );

		renderPiece( g , getLevel()->getHoldPiece() , pos + Vec2i( 40 , 80 ) );
	}


	void Scene::renderPieceStorage( GWidget* ui )
	{

		Vec2i pos = ui->getWorldPos();
		IGraphics2D& g = Global::GetIGraphics2D();

		RenderUtility::SetFont( g , FONT_S10 );
		g.setTextColor( Color3ub(255 , 255 , 0) );

		g.drawText( Vector2( pos.x + 16 , pos.y + 10) , "Next:" );
		renderNextPieceInternal( g , pos + Vec2i( 40 , 70 ) );


		g.drawText( Vector2(pos.x + 16 , pos.y + 210) , "Hold:" );
		renderPiece( g , getLevel()->getHoldPiece() , pos + Vec2i( 40 , 265 ) );
	}


	void Scene::render(IGraphics2D& g , LevelMode* levelMode )
	{
		Vec2i mapPos = Vec2i( BlockSize , BlockSize );

		g.beginXForm();
		g.identityXForm();
		g.translateXForm( mScenePos.x , mScenePos.y );

		if ( mScale != 1.0f )
			g.scaleXForm( mScale , mScale );

		renderBackground( g , Vec2i::Zero() );

		int extendMapSizeY = mLevel->getBlockStorage().getExtendSizeY();

		Vec2i const sceneSize( mLevel->getBlockStorage().getSizeX() * BlockSize , extendMapSizeY * BlockSize );

		switch( getLevel()->getState() )
		{
		case LVS_REMOVE_MAPLINE:
			{
				long stateTime = getLevel()->getStateDuration();
				float offset;
				if ( stateTime > getLevel()->getClearLayerTime() / 2 )
				{
					float s = (  2 * stateTime / float( getLevel()->getClearLayerTime() ) - 1  ) ;
					offset = BlockSize * s * s;
				}
				else
				{
					offset = 0;
				}

				int* remove = getLevel()->mRemoveLayer;
				int  numRemove = getLevel()->mRemoveLayerNum;
				int nR = 0;
				for( int j = extendMapSizeY - 1 ; j >= 0 ; --j )
				{
					if ( nR >= numRemove || j != getLevel()->mRemoveLayer[nR] )
					{
						renderLayer(  g , mapPos + Vec2i( 0 , int( ( getLevel()->getLastRemoveLayerNum()- nR ) * offset ) ) , j );
					}
					else
					{
						++nR;
					}
				}
				renderConnectBlock( g , mapPos , true , offset );

				int sceneWidth  = BlockSize * ( mLevel->getBlockStorage().getSizeX() + 2 );
				int sceneHeight = BlockSize * ( mLevel->getBlockStorage().getSizeY() );

				if ( stateTime < getLevel()->getClearLayerTime() / 2 )
				{
					g.beginBlend( mapPos , sceneSize , ( 1.0f - 2 * float( stateTime ) / getLevel()->getClearLayerTime() ) );
					{
						for( int i = 0 ; i < numRemove ; ++i )
						{
							renderLayer( g , mapPos , remove[i] );
						}
					}
					g.endBlend();
				}
			}
			break;
		case  LVS_REMOVE_CONNECT:
			{
				renderBlockMap( g , mapPos );
				renderConnectBlock( g , mapPos , false );

				g.beginBlend( mapPos ,sceneSize , float( getLevel()->getStateDuration() )/ getLevel()->getClearLayerTime() );
				{
					renderConMapMask( g , mapPos );
				}
				g.endBlend();
			}
			break;
		default:
			{
				if ( getLevel()->getState() != LVS_OVER )
				{
					if ( mShowFallPosition )
					{
						Vec2i size( 4 * BlockSize , 4 * BlockSize );
						Vec2i pos = calcBlockPos( mapPos , getLevel()->mXPosMP , mYPosShowFall );
						g.beginBlend( pos - Vec2i( 0 , size.y ) , 2 * size , 0.3f );
						renderPiece( g , getLevel()->getMovePiece() , pos );
						g.endBlend();
					}

					renderCurPiece( g , mapPos );
				}
				renderBlockMap( g , mapPos );
				renderConnectBlock( g , mapPos , false );
			}
		}

		levelMode->render( this );

		g.finishXForm();
	}

	void Scene::renderBackground(IGraphics2D& g , Vec2i const& pos )
	{
		RenderUtility::SetPen( g , EColor::Black );

		int sizeX = mLevel->getBlockStorage().getSizeX();
		int sizeY = mLevel->getBlockStorage().getSizeY();

		RenderUtility::DrawBlock( g , pos , 1 , sizeY + 1 , EColor::Gray );
		RenderUtility::DrawBlock( g , pos + Vec2i( BlockSize , BlockSize * sizeY ) , sizeX , 1  , EColor::Gray );
		RenderUtility::DrawBlock( g , pos + Vec2i( BlockSize * ( sizeX + 1 ) ,0 ) , 1 , sizeY + 1 , EColor::Gray );
	}

	void Scene::renderCurPiece(IGraphics2D& g , Vec2i const& mapPos )
	{
		Vec2i pos = calcBlockPos( mapPos , getLevel()->mXPosMP , getLevel()->mYPosMP );

		//pos -= Vec2i( 0 , 0.5 * mPieceDownOffset );

		if ( mAngle != 0 )
		{
			g.pushXForm();
			{
				g.translateXForm( pos.x + 1.5 * BlockSize  , pos.y - 0.5 * BlockSize  );
				g.rotateXForm( mAngle );
				g.translateXForm( -1.5 * BlockSize , 0.5 * BlockSize  );
				renderPiece( g , getLevel()->getMovePiece() , Vec2i( 0 , 0 ) );
			}
			g.popXForm();
		}
		else 
		{		
			renderPiece( g , getLevel()->getMovePiece() , pos );
		}
	}



	void Scene::renderNextPieceInternal(IGraphics2D& g , Vec2i const& pos )
	{
		Vec2i renderPos = Vec2i( 0 , (int)mNextPieceOffset );

		g.beginXForm();

		g.identityXForm();
		g.translateXForm( pos.x , pos.y );

		float scale = 0.7f;
		int offset = BlockSize * 4;

		int num = 2;
		if ( getLevel()->getState() == LVS_ENTRY_DELAY && fabs( mNextPieceOffset ) > offset / 2 )
			num = 3;

		g.pushXForm();
		if ( mNextPieceOffset == 0 )
			++num;
		else
			g.scaleXForm( scale , scale );

		renderPiece( g , getLevel()->getNextPiece() , renderPos );
		g.popXForm();

		renderPos += Vec2i( 0 , mNextPieceOffset2[0] + 5 );
		g.scaleXForm( scale , scale );
		for( int i = 1 ; i < num ; ++i )
		{
			renderPiece( g , getLevel()->getNextPiece( i ) , renderPos );
			renderPos += Vec2i( 0 , mNextPieceOffset2[i] );
		}
		g.finishXForm();
	}

	void Scene::notifyChangePiece()
	{
		mNextPieceOffset = BlockSize * 4;
		calcNextPieceOffset();

		if ( mShowFallPosition )
		{
			mYPosShowFall = getLevel()->calcFallPosY();
		}

	}

	void Scene::notifyRemoveLayer( int layer[] , int numLayer )
	{

	}

	void Scene::notifyMarkPiece( int layer[] , int numLayer )
	{

	}

	void Scene::calcPieceRnageY( Piece const& piece , int& max , int& min )
	{
		max = -10;
		min =  10;
		for( int i = 0 ; i < piece.getBlockNum() ; ++i )
		{
			PieceBlock const& block = piece.getBlock(i);
			if ( block.getY() > max )
				max = block.getY();
			if ( block.getY() < min )
				min = block.getY();
		}
	}

	void Scene::calcNextPieceOffset()
	{
		int max , min;
		calcPieceRnageY( getLevel()->getNextPiece( 0 ) , max , min );
		int prevMin = min;
		for( int i = 0 ; i < RenderNextPieceNum ; ++i )
		{
			calcPieceRnageY( getLevel()->getNextPiece( i + 1 ) , max , min );
			mNextPieceOffset2[i] = ( max - prevMin + 2 ) * BlockSize;

			if ( ( max - min ) <= 1 )
				mNextPieceOffset2[i] += BlockSize;

			prevMin = min;
		}
	}

	void Scene::notifyRotatePiece(int orgDir , bool beCW)
	{
		if ( CVarTetrisUseRotateAnim )
		{
			int dDir = getLevel()->getMovePiece().getDirection() - orgDir;

			if (dDir >= 3)
				dDir -= 4;
			else if (dDir <= -3)
				dDir += 4;

			mAngle = 90.0f * dDir;
			//FIXME
			if (getLevel()->getMovePiece().getRotationSize() == 4)
				mAngle = -mAngle;
		}

		if ( mShowFallPosition )
		{
			mYPosShowFall = getLevel()->calcFallPosY();
		}
	}

	void Scene::notifyMovePiece(int x)
	{
		if ( mShowFallPosition )
		{
			mYPosShowFall = getLevel()->calcFallPosY();
		}
	}

	void Scene::notifyHoldPiece()
	{
		calcNextPieceOffset();
	}

	void Scene::notifyFallPiece()
	{

	}

	void Scene::notifyMoveDown()
	{

	}

}//namespace Tetris
