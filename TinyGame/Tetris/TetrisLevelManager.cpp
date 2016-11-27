#include "TetrisPCH.h"
#include "TetrisLevelManager.h"

#include "TetrisLevel.h"
#include "TetrisActionId.h"
#include "TetrisGame.h"
#include "TetrisGameInfo.h"

#include "GameGlobal.h"
#include "GameControl.h"

#include "GamePlayer.h"
#include "DrawEngine.h"
#include "GameGUISystem.h"
#include "GameWidgetID.h"

namespace Tetris
{
	LevelData::LevelData()
	{
		mPlayerID = ERROR_PLAYER_ID;
		mScene = NULL;
		mLevel = NULL;
		mModeData  = NULL;
	}

	void LevelData::build( Mode* mode , bool bNeedScene )
	{
		if ( !mModeData )
			mModeData = mode->createModeData();

		if ( !mLevel )
		{
			mLevel = new Level( mode->getLevelManager() );
			mLevel->setUserData( this );
		}

		if ( bNeedScene )
		{
			if ( !mScene )
			{
				mScene = new Scene( mLevel );
			}
		}
	}

	void LevelData::cleanup()
	{
		if ( mScene )
		{
			delete mScene;
			mScene = NULL;
		}

		if ( mLevel )
		{
			delete mLevel;
			mLevel = NULL;
		}

		delete mModeData;
		mModeData  = NULL;
	}

	void LevelData::fireAction(ActionTrigger& trigger)
	{
		Level* level = getLevel();
		Scene* scene = getScene();
		if ( level->getState() == LVS_NORMAL )
		{
			if( trigger.detect( ACT_MOVE_LEFT ) ) 
			{
				level->movePiece( -1 , 0 );
				if ( scene )
					scene->notifyMovePiece( -1 );
			}
			else if( trigger.detect( ACT_MOVE_RIGHT) )   
			{
				level->movePiece( 1 , 0 );
				if ( scene )
					scene->notifyMovePiece( 1 );
			}

			if( trigger.detect( ACT_MOVE_DOWN ) ) 
			{
				level->moveDown();
				if ( scene )
					scene->notifyMoveDown();
			}

			if( trigger.detect( ACT_ROTATE_CCW ) )
			{
				int orgDir = level->getMovePiece().getDir();
				level->rotatePiece( true );
				if ( scene )
					scene->notifyRotatePiece( orgDir , true );
			}
			else if( trigger.detect( ACT_ROTATE_CW ) )
			{
				int orgDir = level->getMovePiece().getDir();
				level->rotatePiece( false );
				if ( scene )
					scene->notifyRotatePiece( orgDir , false );
			}
			else if ( trigger.detect( ACT_FALL_PIECE ) )
			{
				level->fallPiece();
				if ( scene )
					scene->notifyFallPiece();
			}
		}

		if ( getLevel()->getState() != LVS_OVER )
		{
			if ( trigger.detect( ACT_HOLD_PIECE ) )
			{
				level->holdPiece();
				if ( scene )
					scene->notifyHoldPiece();
			}
		}
	}

	void GameWorld::init( Mode* mode )
	{
		mNeedScene = true;

		mLevelMode = mode;
		mLevelMode->mLevelManager = this;
		mLevelMode->init();

		mNumLevel = 0;
		for ( int i = 0 ; i < gTetrisMaxPlayerNum ; ++i )
		{
			mUsingData[i] = NULL;
			mDataStorage[i].id = i;
			mDataStorage[i].idxUse = -1;
		}
	}

	bool GameWorld::removePlayer( unsigned id )
	{
		LevelData* data = findPlayerData( id );
		if ( !data )
			return false;

		--mNumLevel;

		if ( mNumLevel && data->idxUse != 1 )
		{
			int chIdx = mNumLevel - 1;
			mUsingData[ data->idxUse ] =  mUsingData[ chIdx ];
			mDataStorage[ chIdx ].idxUse = data->idxUse;
			data->idxUse = -1;
		}
		return true;
	}

	LevelData* GameWorld::createPlayerLevel( GamePlayer* player )
	{
		if ( mNumLevel >= gTetrisMaxPlayerNum )
			return NULL;

		LevelData* data = NULL;
		PlayerInfo& info = player->getInfo();
		for ( int i = 0; i < gTetrisMaxPlayerNum ; ++ i )
		{
			if ( mDataStorage[i].idxUse == -1 )
			{
				data = mDataStorage + i;

				info.actionPort = data->getID();
				data->mPlayerID  = player->getId();

				data->idxUse = mNumLevel;
				mUsingData[ mNumLevel ] = data;
				++mNumLevel;

				data->build( mLevelMode , mNeedScene );
				return data;
			}
		}

		return NULL;
	}

	LevelData* GameWorld::findPlayerData( unsigned id )
	{
		for( int i = 0 ; i < mNumLevel ; ++i )
		{
			LevelData& data = getUsingData( i );
			if ( data.getPlayerID() == id )
				return &data;
		}
		return NULL;
	}

	void GameWorld::render( Graphics2D& g )
	{
		if ( !mNeedScene )
			return;

		for ( int i = 0; i < mNumLevel ; ++ i )
		{
			LevelData& data = getUsingData( i );
			data.getScene()->render( g , getLevelMode() );
		}
	}


	Vec2i const SinglePlayerScenePos( 320 , 140 );
	Vec2i const MultiPlayerMainScenePos( 155 , 140 );

	Vec2i const MultiPlayerSubScenePos( 500 , 100 );
	Vec2i const MultiPlayerSubSceneOffset( 150 , 280 );

	GameWorld::~GameWorld()
	{
		delete mLevelMode;
		for( int i = 0; i < gTetrisMaxPlayerNum ; ++i )
			mDataStorage[i].cleanup();
	}


	void GameWorld::tick()
	{
		if ( mbGameEnd )
			return;

		if ( mNumLevelOver == mNumLevel || mLevelMode->checkOver() )
		{
			mbGameEnd = true;
			mLevelMode->onGameOver();
			return;
		}

		mLevelMode->tick();

		for( int i = 0 ; i < mNumLevel ; ++i )
		{
			LevelData& data = getUsingData( i );

			data.getLevel()->update( gDefaultTickTime );

			LevelState curState = data.getLevel()->getState();

			if ( data.lastState != curState )
			{
				switch( curState )
				{
				case LVS_OVER:
					{
						Mode::LevelEvent event;
						event.id = Mode::eLEVEL_OVER;
						mLevelMode->onLevelEvent( data , event );
						++mNumLevelOver;
					}
					break;
				}
			}
			data.lastState = curState;
		}

	}

	void GameWorld::updateFrame( int frame )
	{
		for( int i = 0 ; i < mNumLevel ; ++i )
		{
			LevelData& data = getUsingData( i );
			data.getScene()->updateFrame( frame );
		}
	}

	void GameWorld::restart(  bool beInit )
	{
		mLevelMode->restart( beInit );

		for ( int i = 0 ; i < mNumLevel ; ++i )
		{
			LevelData& data = getUsingData(i);
			data.getModeData()->reset( mLevelMode , data , beInit );
			data.getLevel()->restart();
			if ( mNeedScene )
				data.getScene()->restart();
			data.lastState = LVS_NORMAL;
		}

		mNumLevelOver = 0;
		mbGameEnd = false;
	}


	void GameWorld::fireAction( ActionTrigger& trigger )
	{
		assert( mLevelMode );

		for( int i = 0 ; i < mNumLevel ; ++ i )
		{
			LevelData& data = getUsingData( i );
			trigger.setPort( data.getID() );
			mLevelMode->fireAction( data , trigger  );
		}
	}


	void GameWorld::fireLevelAction( ActionTrigger& trigger )
	{
		LevelData& data = *getLevelData( trigger.getPort() );
		mLevelMode->fireAction( data , trigger );
	}


	void GameWorld::setLevelMode( Mode* mode )
	{



	}

	int GameWorld::getSortedLevelData( unsigned mainID , LevelData* sortedData[] )
	{
		int other = 1;
		for( int i = 0 ; i < mNumLevel ; ++i )
		{
			LevelData& data = getUsingData( i );

			if ( data.getPlayerID() == mainID )
			{
				sortedData[0] = &data;
			}
			else
			{
				sortedData[ other ] = &data;
				++other;
			}
		}

		return mNumLevel;
	}

	void GameWorld::storePlayerLevel( IPlayerManager& playerMgr )
	{
		for( IPlayerManager::Iterator iter = playerMgr.getIterator();
			iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			if ( player->getType() != PT_SPECTATORS )
			{
				LevelData* data = createPlayerLevel( player );
			}
		}
	}

	void GameWorld::onRemoveLayer( Level* level , int layer[] , int numLayer )
	{
		LevelData* data = static_cast< LevelData* >( level->getUserData() );
		if ( mNeedScene )
			data->getScene()->notifyRemoveLayer( layer , numLayer );
		Mode::LevelEvent event;
		event.id       = Mode::eREMOVE_LAYER;
		event.layer    = layer;
		event.numLayer = numLayer;
		mLevelMode->onLevelEvent( *data , event );
	}

	void GameWorld::onMarkPiece( Level* level , int layer[] , int numLayer )
	{
		LevelData* data = static_cast< LevelData* >( level->getUserData() );
		if ( mNeedScene )
			data->getScene()->notifyMarkPiece( layer , numLayer );
		Mode::LevelEvent event;
		event.id       = Mode::eMARK_PIECE;
		event.layer    = layer;
		event.numLayer = numLayer;
		mLevelMode->onLevelEvent( *data , event );
	}


	void GameWorld::onChangePiece( Level* level )
	{
		LevelData* data = static_cast< LevelData* >( level->getUserData() );
		if ( mNeedScene )
			data->getScene()->notifyChangePiece();
		Mode::LevelEvent event;
		event.id    = Mode::eCHANGE_PIECE;
		mLevelMode->onLevelEvent( *data , event );
	}

	void GameWorld::setupScene( unsigned mainID , unsigned flag )
	{
		calcScenePos( mainID );

		LevelData* levelData[ gTetrisMaxPlayerNum ];
		getSortedLevelData( mainID , levelData );
		Vec2i pos = levelData[0]->getScene()->getSurfacePos() - Vec2i( 140 , 0 );

		addBaseUI( *levelData[0] , pos , 
			RenderCallBack::create( mLevelMode , &Mode::renderStats ) );

		switch( getLevelNum() )
		{
		case 2:
			{
				LevelData* lvData = levelData[1];
				pos = lvData->getScene()->getSurfacePos() + Vec2i( 250 , 0 );
				addBaseUI( *lvData , pos , 
					RenderCallBack::create( mLevelMode , &Mode::renderStats ) );
			}
			break;
		}

		mLevelMode->setupScene( flag );
	}

	void GameWorld::calcScenePos( unsigned mainID )
	{
		if ( getLevelNum() == 1 )
		{
			LevelData& data = getUsingData( 0 );
			data.getScene()->setSurfacePos( SinglePlayerScenePos );
			data.getScene()->setSurfaceScale( 1.0f );
		}
		else if ( getLevelNum() == 2 )
		{
			LevelData* userData;
			LevelData* otherData;
			userData = &getUsingData( 0 );
			if ( userData->getPlayerID() == mainID )
			{
				otherData = &getUsingData( 1 );
			}
			else
			{
				otherData = userData;
				userData = &getUsingData( 1 );
			}

			userData->getScene()->setSurfacePos( MultiPlayerMainScenePos );
			userData->getScene()->setSurfaceScale( 1.0f );

			otherData->getScene()->setSurfacePos( MultiPlayerMainScenePos + Vec2i(250,0));
			otherData->getScene()->setSurfaceScale( 1.0f );

		}
		else
		{
			int subCount = 0;
			for( int i = 0 ; i < mNumLevel ; ++i )
			{
				LevelData& data = getUsingData( i );
				Scene* scene = data.getScene();
				if ( data.getPlayerID() == mainID )
				{
					scene->setSurfacePos( MultiPlayerMainScenePos );
					scene->setSurfaceScale( 1.0f );
				}
				else
				{
					Vec2i pos = MultiPlayerSubScenePos;
					if ( subCount / 2 )
						pos.x += MultiPlayerSubSceneOffset.x;
					if ( subCount % 2 )
						pos.y += MultiPlayerSubSceneOffset.y;

					scene->setSurfacePos( pos );
					scene->setSurfaceScale( 0.5f );
					++subCount;
				}
			}
		}
	}

	void GameWorld::addBaseUI( LevelData& lvData , Vec2i& pos , RenderCallBack* statsCallback )
	{
		GPanel* panel;
		int ppHeight = 290; 

		pos -= Vec2i( 0 , 80 );
		panel = new GPanel( UI_PANEL , pos , Vec2i( 130 , ppHeight )  ,  NULL );

		panel->setRenderCallback( 
			RenderCallBack::create( lvData.getScene() ,  &Scene::renderPieceStorage ) );
		::Global::GUI().addWidget( panel );
		pos += Vec2i( 0 , ppHeight + 10 );

		panel = new GPanel( UI_PANEL , pos , Vec2i( 130 , 200 ) ,  NULL );
		panel->setRenderCallback( statsCallback );
		panel->setUserData( intptr_t(&lvData) );
		::Global::GUI().addWidget( panel );
		pos += Vec2i( 0 , 200 );
	}


	int Mode::getMaxPlayerNumber( ModeID mode )
	{
		switch( mode )
		{
		case MODE_TS_BATTLE: return 2;
		}
		return gTetrisMaxPlayerNum;
	}


	void Mode::restart( bool beInit )
	{
		mGameTime = 0;
		doRestart( beInit );
	}

}//namespace Tetris
