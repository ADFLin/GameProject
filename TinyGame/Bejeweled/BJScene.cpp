#include "TinyGamePCH.h"
#include "BJScene.h"

#include "RenderUtility.h"
#include "SysMsg.h"
#include "EasingFun.h"

namespace Bejeweled
{
	Scene::Scene() 
		:mLevel( this )
		,mBoardPos( 50 , 50 )
	{
		mGemMoveVec.reserve( Level::BoardStorageSize );
	}

	void Scene::drawGem( Graphics2D& g , Vec2i const& pos , int type )
	{
		int const gemColor[] = 
		{ 
			Color::eNull , Color::eGreen , Color::eRed ,  
			Color::eBlue , Color::eYellow , Color::ePurple ,
			Color::eOrange , Color::eCyan ,
		};

		switch( type % 3 )
		{
		case 1:
			{
				int len = CellLength - 15;
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , gemColor[ type ] , COLOR_DEEP );
				g.drawRoundRect( pos - Vec2i( len / 2 , len / 2 ) , 
					Vec2i( len , len ) , Vec2i( 10 , 10 ) );
				RenderUtility::setPen( g , Color::eWhite );
				RenderUtility::setBrush( g , gemColor[ type ] );
				len -= 8;
				g.drawRoundRect( pos - Vec2i( len / 2 , len / 2 ) , 
					Vec2i( len , len ) , Vec2i( 6 , 6 ) );
			}
			break;
		case 2:
			{
				int radius = ( CellLength - 12 ) / 2 ;
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , gemColor[ type ] , COLOR_DEEP );
				g.drawCircle( pos , radius );
				RenderUtility::setPen( g , Color::eWhite );
				RenderUtility::setBrush( g , gemColor[ type ] );
				g.drawCircle( pos , radius - 4 );
			}
			break;
		case 3:
			{




			}
			break;
		default:
			{
				int len = ( CellLength - 10 ) / 2 ;
				int factor = 6;
				Vec2i const vtx[] = 
				{   
					Vec2i( factor , -1 ) , Vec2i( factor , 1 ) , 
					Vec2i( 1 , factor ) , Vec2i( -1 , factor ) ,
					Vec2i( -factor , 1 ) , Vec2i( -factor , -1 ) ,
					Vec2i( -1 , -factor )  , Vec2i( 1 , -factor ) 
				};
				Vec2i rPos[8];

				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , gemColor[ type ] , COLOR_DEEP );
				for( int i = 0 ; i < 8 ; ++i )
					rPos[i] = pos + ( len * vtx[i] ) / factor;
				g.drawPolygon( rPos , 8 );
				RenderUtility::setPen( g , Color::eWhite );
				RenderUtility::setBrush( g , gemColor[ type ] );
				len -= 6;
				for( int i = 0 ; i < 8 ; ++i )
					rPos[i] = pos + ( len * vtx[i] ) / factor;
				g.drawPolygon( rPos , 8 );
			}
		}
	}

	void Scene::render( Graphics2D& g )
	{
		for( int i = 0 ; i < Level::BoardStorageSize ; ++i )
		{
			Vec2i renderPos = mBoardPos + CellLength * Vec2i( i % BoardSize , i / BoardSize );

			RenderUtility::drawBlock( g , renderPos , Vec2i( CellLength , CellLength ) , Color::eGray );

			if ( mState == eChoiceSwapGem && ( mGemRenderFlag[i] & RF_POSIBLE_SWAP ) )
			{
				g.beginBlend( renderPos , Vec2i( CellLength , CellLength ) , mFadeOutAlpha );
				RenderUtility::drawBlock( g , renderPos , Vec2i( CellLength , CellLength ) , Color::eRed );
				g.endBlend();
			}
		}
		if ( mState == eChoiceSwapGem && mCtrlMode == CM_CHOICE_GEM2 )
		{
			int idx = mIndexSwapCell[0];
			Vec2i renderPos = mBoardPos + CellLength * Vec2i( idx % BoardSize , idx / BoardSize );
			g.beginBlend( renderPos , Vec2i( CellLength , CellLength ) , mFadeOutAlpha );
			RenderUtility::drawBlock( g , renderPos , Vec2i( CellLength , CellLength ) , Color::eWhite );
			g.endBlend();
		}

		for( int i = 0 ; i < Level::BoardStorageSize ; ++i )
		{
			if ( mGemRenderFlag[i] & RF_RENDER_ANIM )
				continue;

			int type = getLevel().getBoardGem( i );
			if ( type == 0 )
				continue;

			Vec2i renderPos;
			renderPos = mBoardPos + CellLength * Vec2i( i % BoardSize , i / BoardSize );
			renderPos += Vec2i( CellLength , CellLength ) / 2 ;

			drawGem( g , renderPos , type );
		}

		for( GemMoveVec::iterator iter = mGemMoveVec.begin();
			iter != mGemMoveVec.end() ; ++iter )
		{
			Vec2i renderPos;
			renderPos = mBoardPos + iter->pos;

			int type = getLevel().getBoardGem( iter->index );
			renderPos += Vec2i( CellLength , CellLength ) / 2 ;

			drawGem( g , renderPos , type );
		}

		for( GemFadeOutVec::iterator iter = mGemFadeOutVec.begin();
			iter != mGemFadeOutVec.end() ; ++iter )
		{
			int type = iter->type;
			int idx  = iter->index;

			Vec2i cellPos = mBoardPos + CellLength * Vec2i(  idx % BoardSize , idx / BoardSize );
			Vec2i renderPos = cellPos + Vec2i( CellLength , CellLength ) / 2 ;

			g.beginBlend( cellPos , Vec2i( CellLength , CellLength ) , mFadeOutAlpha );
			drawGem( g , renderPos , type );
			g.endBlend();
		}

		RenderUtility::setFont( g , FONT_S8 );
		FixString< 128 > str;
		Vec2i texPos( Global::getDrawEngine()->getScreenWidth() - 100 , 10 );
		str.format( "CtrlMode = %d" , (int)mCtrlMode );
		g.drawText( texPos , str );
		texPos.y += 15;
		str.format( "MoveCell = ( %d , %d )" , mMoveCell.x , mMoveCell.y );
		g.drawText( texPos , str );
		texPos.y += 15;
	}

	void Scene::restart()
	{
		std::fill_n( mGemRenderFlag , Level::BoardStorageSize , 0 );
		mGemFadeOutVec.clear();
		mGemMoveVec.clear();
		getLevel().generateRandGems();
		mCtrlMode = CM_CHOICE_GEM1;
		changeState( eChoiceSwapGem );
		checkAllPosibleSwapPair();
	}

	void Scene::tick()
	{
		mStateTime += gDefaultTickTime;

		switch( mState )
		{
		case eChoiceSwapGem:
			break;
		case eSwapGem:
			if ( mStateTime > TimeSwapGem )
			{
				removeAllGemAnim();
				if ( getLevel().stepCheckSwap( mIndexSwapCell[0] , mIndexSwapCell[1] ) )
				{
					getLevel().stepDestroy();
					changeState( eDestroyGem );
				}
				else
				{
					swapGem( mIndexSwapCell[0] , mIndexSwapCell[1] );
					changeState( eRestoreGem );
				}
			}
			break;
		case eRestoreGem: 
			if ( mStateTime > TimeSwapGem )
			{
				removeAllGemAnim();
				changeState( eChoiceSwapGem );
			}
			break;
		case eDestroyGem:
			if ( mStateTime > TimeDestroyGem )
			{
				mGemFadeOutVec.clear();
				for( int i = 0 ; i < BoardSize ; ++i )
					mNumFill[i] = 0;
				getLevel().stepFill();
				changeState( eFillGem );
			}
			break;
		case eFillGem:
			if ( mStateTime > TimeFillGem )
			{
				removeAllGemAnim();
				if ( getLevel().setepCheckFill() )
				{
					getLevel().stepDestroy();
					changeState( eDestroyGem );
				}
				else
				{
					changeState( eChoiceSwapGem );
					checkAllPosibleSwapPair();
				}
			}
			break;
		}
	}

	void Scene::updateFrame( int frame )
	{
		updateAnim( gDefaultTickTime * frame );

		switch( mState )
		{
		case eDestroyGem: 
			mFadeOutAlpha = float( TimeDestroyGem - mStateTime ) / TimeDestroyGem;
			break;
		case eChoiceSwapGem:
			mFadeOutAlpha = 0.5f + 0.25f *sin( 0.005f * (float)mStateTime ); 
			break;
		}
	}

	DestroyMethod Scene::onFindSameGems( int idxStart , int num , bool beV )
	{
		return RM_NORMAL;
	}

	bool Scene::procMouseMsg( MouseMsg const& msg )
	{
		if ( mState == eChoiceSwapGem )
		{
			switch( mCtrlMode )
			{
			case CM_CHOICE_GEM1:
				if ( msg.onLeftDown() )
				{
					Vec2i pos = getCellPos( msg.getPos() );
					if ( 0 <= pos.x && pos.x < BoardSize &&
						 0 <= pos.y && pos.y < BoardSize )
					{
						mIndexSwapCell[0] = Level::getIndex( pos.x , pos.y );
						mCtrlMode = CM_CHOICE_GEM2;
					}
				}
				break;
			case CM_CHOICE_GEM2:
				if ( msg.onLeftDown() )
				{
					Vec2i pos = getCellPos( msg.getPos() );
					if ( 0 <= pos.x && pos.x < BoardSize &&
						0 <= pos.y && pos.y < BoardSize )
					{
						mIndexSwapCell[1] = Level::getIndex( pos.x , pos.y );
						if ( Level::isNeighboring( mIndexSwapCell[0] , mIndexSwapCell[1] ) )
						{
							swapGem( mIndexSwapCell[0] , mIndexSwapCell[1] );
							mCtrlMode = CM_CHOICE_GEM1;
							changeState( eSwapGem );
						}
					}
				}
				else if ( msg.onRightDown() )
				{
					mCtrlMode = CM_CHOICE_GEM1;
				}
				break;
			}
		}
		return false;
	}

	void Scene::updateAnim( long dt )
	{
		mTweener.update( (float) dt );
	}

	void Scene::addGemAnim( int idx , Vec2i const& from , Vec2i const& to , long time )
	{
		assert( ( mGemRenderFlag[ idx ] & RF_RENDER_ANIM ) == 0 );
		mGemMoveVec.push_back( GemMove() );
		GemMove& move = mGemMoveVec.back();
		move.index = idx;
		mTweener.tweenValue< Easing::OCubic >( move.pos , Vec2f( from ) , Vec2f( to ) , (float)time );

		mGemRenderFlag[ idx ] |= RF_RENDER_ANIM;
	}

	void Scene::removeAllGemAnim()
	{
		for( GemMoveVec::iterator iter = mGemMoveVec.begin();
			iter != mGemMoveVec.end() ; ++iter )
		{
			mGemRenderFlag[ iter->index ] &= ~RF_RENDER_ANIM;
		}
		mGemMoveVec.clear();
		mTweener.clear();
	}

	void Scene::swapGem( int idx1 , int idx2 )
	{
		getLevel().stepSwap( idx1 , idx2 );
		Vec2i pos1 = CellLength * getCellPos( idx1 );
		Vec2i pos2 = CellLength * getCellPos( idx2 );
		addGemAnim( idx1 , pos2 , pos1 , TimeSwapGem );
		addGemAnim( idx2 , pos1 , pos2 , TimeSwapGem );
	}

	bool Scene::procKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		if ( mCtrlMode == CM_CHOICE_GEM2 )
		{
			switch( key )
			{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
				getLevel().setBoardGem( mIndexSwapCell[0] , key - '1' + 1 );
				mCtrlMode = CM_CHOICE_GEM1;
				break;
			}
		}
		return true;
	}

	void Scene::checkAllPosibleSwapPair()
	{
		removeAllPosibleSwapPair();

		for( int i = 0 ; i < Level::BoardStorageSize ; ++i )
		{
			int idx2;
			idx2 = Level::getAdjoinedIndex( i , Level::eRIGHT );
			if ( ( i / BoardSize ) == ( idx2 / BoardSize ) )
			{
				if ( getLevel().testSwapPairHaveRemove( i , idx2 ) )
					addPosibleSwapPair( i , idx2 );
			}
			idx2 = Level::getAdjoinedIndex( i , Level::eDOWN );
			if ( idx2 < Level::BoardStorageSize )
			{
				if ( getLevel().testSwapPairHaveRemove( i , idx2 ) )
					addPosibleSwapPair( i , idx2 );
			}
		}
	}


	void Scene::addPosibleSwapPair( int idx1 , int idx2 )
	{
		IndexPair pair;
		pair.idx1 = idx1;
		pair.idx2 = idx2;
		mPosibleSwapPairVec.push_back( pair );

		mGemRenderFlag[ idx1 ] |= RF_POSIBLE_SWAP ;
		mGemRenderFlag[ idx2 ] |= RF_POSIBLE_SWAP ;
	}

	void Scene::removeAllPosibleSwapPair()
	{
		for( IndexPairVec::iterator iter = mPosibleSwapPairVec.begin();
			iter != mPosibleSwapPairVec.end() ; ++iter )
		{
			IndexPair& pair = *iter;
			mGemRenderFlag[ pair.idx1 ] &= ~RF_POSIBLE_SWAP ;
			mGemRenderFlag[ pair.idx2 ] &= ~RF_POSIBLE_SWAP ;
		}
		mPosibleSwapPairVec.clear();
	}

	void Scene::onFillGem( int idx , GemType type )
	{
		int x = idx % BoardSize;
		mNumFill[ x ] += 1;

		Vec2i to = CellLength * getCellPos( idx );
		Vec2i from;
		from.x = to.x;
		from.y = -CellLength * mNumFill[ x ];

		addGemAnim( idx , from , to , TimeFillGem );
	}

	void Scene::onDestroyGem( int idx , GemType type )
	{
		GemFadeOut fadeOut;
		fadeOut.index = idx;
		fadeOut.type  = type;
		mGemFadeOutVec.push_back( fadeOut );
	}

	void Scene::onMoveDownGem( int idxFrom , int idxTo )
	{
		addGemAnim( idxTo , CellLength * getCellPos( idxFrom ) , CellLength * getCellPos( idxTo ) , TimeFillGem );
	}

	void Scene::changeState( State state )
	{
		if ( mState == state )
			return;

		mState = state;
		mStateTime = 0;
	}

}//namespace Bejeweled
