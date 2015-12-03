#include "TetrisPCH.h"

#include "TetrisStage.h"
#include "TetrisGame.h"
#include "TetrisLevelManager.h"
#include "TetrisLevelMode.h"
#include "TetrisAction.h"

#include "GameGUISystem.h"
#include "GameClient.h"
#include "GameReplay.h"
#include "GamePackageManager.h"
#include "GameWidgetID.h"

#include "GameServer.h"
#include "GameClient.h"

#include "RenderUtility.h"
#include "OptionNoteBook.h"

#include "GameSingleStage.h"
#include "NetGameStage.h"

#include "GameWorker.h"
#include "CSyncFrameManager.h"

#include "GameRecord.h"

#include <ctime>
#include <cmath>
#include <algorithm>

namespace Tetris
{
	Vec2i const LevelStage::UI_BasePos = Vec2i( 283 , 130 );
	Vec2i const LevelStage::UI_ButtonSize( 120 , 20 );

	LevelStage::LevelStage()
	{
		mGameMode = NULL;
	}

	LevelStage::~LevelStage()
	{


	}

	bool LevelStage::onInit()
	{
		if ( !BaseClass::onInit() )
			return false;

		if ( !mGameMode )
		{
			return false;
		}

		mWorld.reset( new GameWorld( mGameMode ) );
		getStage()->getActionProcessor().setEnumer( mWorld.get() );
		return true;
	}



	void LevelStage::onEnd()
	{
		getStage()->getActionProcessor().setEnumer( NULL );
	}

	void LevelStage::setupScene( IPlayerManager& playerMgr )
	{
		unsigned flag = 0;

		::Global::getGUI().cleanupWidget();

		MyController& controller = static_cast< MyController& >(	
			getGame()->getController() );

		mWorld->storePlayerLevel( playerMgr );

		switch( getGameType() )
		{
		case GT_SINGLE_GAME:
			mGameMode->setupSingleGame( controller );
			break;
		case GT_REPLAY:
			if ( getGameType() == GT_REPLAY )
			{
				flag |= Mode::eReplay;
			}
			break;
		case GT_NET_GAME: 
			{
				GamePlayer* player = playerMgr.getPlayer( playerMgr.getUserID() );
				controller.setPortControl( player->getActionPort() , 0 );
			}
			flag |= Mode::eNetGame; 
			if ( getManager()->getNetWorker()->isServer() )
				flag |= Mode::eOwnServer;
			break;
		}

		mWorld->setupScene( playerMgr.getUserID() , flag );

		Vec2i const UI_ButtonSize( 90 , 20 );
		Vec2i offset( -( UI_ButtonSize.x + 4 ) , 0 );

		GPanel* panel = new GPanel( UI_ANY , Vec2i(0,0) , 
			Vec2i( Global::getDrawEngine()->getScreenWidth() , UI_ButtonSize.y + 10 ) , NULL );

		GButton* button;
		panel->setRenderType( GPanel::eRectType );
		::Global::getGUI().addWidget( panel );

		Vec2i pos( Global::getDrawEngine()->getScreenWidth() - ( UI_ButtonSize.x + 10 ) , 5 );

		button = new GButton( ( flag & Mode::eNetGame ) ? UI_MAIN_MENU : UI_GAME_MENU , pos , UI_ButtonSize  , panel );
		button->setTitle( LAN("Exit Game") );

		if ( !( flag & Mode::eReplay ) )
		{
			pos += offset;
			button = new GButton( UI_PAUSE_GAME , pos , UI_ButtonSize  , panel );
			button->setTitle( LAN("Pause Game") );

			if ( !( flag & Mode::eNetGame ) || ( flag & Mode::eOwnServer ) )
			{
				pos += offset;
				button = new GButton( UI_RESTART_GAME , pos , UI_ButtonSize , panel );
				button->setTitle( LAN("Restart Game") );
			}
		}

	}


	CFrameActionTemplate* LevelStage::createActionTemplate( unsigned version )
	{
		switch ( version )
		{
		case CFrameActionTemplate::LastVersion:
		case LAST_VERSION:
			{
				CFrameActionTemplate* actionTemplate = new CFrameActionTemplate( *mWorld );
				return actionTemplate;
			}
		}
		return NULL;
	}

	bool LevelStage::setupNetwork( NetWorker* netWorker , INetEngine** engine )
	{
		IFrameActionTemplate* actionTemplate = createActionTemplate( LAST_VERSION );
		INetFrameManager* netFrameMgr;
		if ( netWorker->isServer() )
		{
			ServerWorker* server = static_cast< ServerWorker*>( netWorker );
			CServerFrameGenerator* frameGenerator = new CServerFrameGenerator( gTetrisMaxPlayerNum );
			netFrameMgr = new SVSyncFrameManager( server , actionTemplate , frameGenerator );
		}
		else
		{
			ClientWorker* client = static_cast< ClientWorker* >( netWorker );
			CClientFrameGenerator* frameGenerator = new CClientFrameGenerator;
			netFrameMgr = new CLSyncFrameManager( client , actionTemplate , frameGenerator );

		}
		*engine = new CFrameActionEngine( netFrameMgr );
		return true;
	}


	void LevelStage::onRestart( uint64 seed , bool beInit )
	{
		if ( beInit )
			Global::RandSeedNet( seed );

		mGameTime = 0;
		mWorld->restart( beInit );
	}

	void LevelStage::tick()
	{
		if ( getState() == GS_RUN )
		{
			mWorld->tick();
			if ( mWorld->isGameEnd() )
				changeState( GS_END );
		}

		if ( getState() != GS_PAUSE )
			mGameTime += gDefaultTickTime;

		switch( getState() )
		{
		case GS_START:
			if ( mGameTime > 2500 || getGameType() == GT_REPLAY )
				changeState( GS_RUN );
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mWorld->updateFrame( frame );
	}


	void LevelStage::onChangeState( GameState state )
	{
		switch( state )
		{
		case GS_END:
			switch( getGameType() )
			{
			case  GT_SINGLE_GAME:
				{
					IPlayerManager* playerManager = getPlayerManager();
					mLastGameOrder = mGameMode->markRecord( 
						playerManager->getPlayer( playerManager->getUserID() ),
						getRecordManager() );

					if ( mLastGameOrder < 10 && mGameMode->getModeID() == MODE_TS_CHALLENGE )
					{
						FixString< 256 > str;

						time_t rawtime;
						struct tm timeinfo;

						::time ( &rawtime );
						::localtime_s( &timeinfo , &rawtime );

						str.format( "%4d-%02d-%02d-%02d-%02d(rank %d).rpf" , 
							1900 + timeinfo.tm_year , 
							timeinfo.tm_mon + 1 , timeinfo.tm_mday , 
							timeinfo.tm_hour , timeinfo.tm_min ,
							mLastGameOrder + 1 );
						getStage()->saveReplay( str );
					}
					::Global::getGUI().showMessageBox( 
						UI_RESTART_GAME , "Do You Want To Play Game Again ?" );
				}
				break;
			case GT_NET_GAME:
				if ( getManager()->getNetWorker()->isServer() )
				{
					::Global::getGUI().showMessageBox( 
						UI_RESTART_GAME , "Do You Want To Play Game Again ?" );
				}
				break;
			}
		}
	}

	void LevelStage::onRender( float dFrame )
	{
		DrawEngine* de = Global::getDrawEngine();
		Graphics2D& g = Global::getGraphics2D();

		mWorld->render( g );

		if ( getState() == GS_START )
		{
			RenderUtility::setFont( g , FONT_S24 );
			Vec2i pos( de->getScreenWidth() / 2 , de->getScreenHeight() / 2 );
			Vec2i size( 100 , 50 );
			pos -= size / 2;
			if ( mGameTime > 1000 )
			{
				g.setTextColor( 255 , 0 , 0 );
				g.drawText( pos , size , "Go!" );
			}
			else
			{
				g.setTextColor(255 , 255 , 255 );
				g.drawText( pos , size , "Ready!" );
			}
		}

	}

	bool LevelStage::onWidgetEvent( int event , int id , GWidget* ui )
	{
		if ( !mGameMode->onEvent( event , id , ui ) )
			return false;

		switch ( id )
		{
		case UI_RESTART_GAME:
			if ( event == EVT_BOX_NO )
			{
				if ( getGameType() == GT_SINGLE_GAME )
				{
					RecordStage* stage = (RecordStage*)getManager()->changeStage( STAGE_RECORD_GAME );
					stage->setPlayerOrder( mLastGameOrder );
				}
			}
			break;
		}

		return BaseClass::onWidgetEvent( event , id , ui );
	}


	Mode* LevelStage::createMode( GameInfo const& info )
	{
		Mode* levelMode = NULL;
		switch( info.mode )
		{
		case MODE_TS_BATTLE:
			levelMode = new BattleMode;
			break;
		case MODE_TS_CHALLENGE:   
			levelMode = new ChallengeMode( info.modeNormal );
			break;
		case MODE_TS_PRACTICE: 
			levelMode = new PracticeMode;
			break;
		}
		return levelMode;
	}


	bool LevelStage::setupAttrib( AttribValue const& value )
	{
		switch( value.id )
		{
		case ATTR_REPLAY_INFO:
			{
				ReplayInfo* info = reinterpret_cast< ReplayInfo* >( value.ptr );
				if ( strcmp( info->name , TETRIS_NAME ) != 0 )
					return false;

				switch( info->gameVersion )
				{
				case GameInfo::LastVersion:
					{
						GameInfoData* data = reinterpret_cast< GameInfoData*> ( info->gameInfoData );
						if ( !setupGame( data->info ) )
							return false;
					}
					return true;
				}
			}
			break;
		case ATTR_GAME_INFO:
			{
				GameInfoData* data = reinterpret_cast< GameInfoData*> ( value.ptr );
				if ( !setupGame( data->info ) )
					return false;
			}
			return true;
		}
		return false;
	}


	bool LevelStage::setupGame( GameInfo &gameInfo )
	{
		if ( getStage() && getGameType() != GT_NET_GAME )
		{
			LocalPlayerManager* playerManager = static_cast< LocalPlayerManager* >( getPlayerManager() );
			for( int i = 0 ; i < gameInfo.numLevel ; ++i )
			{
				playerManager->createPlayer( i );
			}
			playerManager->setUserID( gameInfo.userID );
		}

		mGameMode = createMode( gameInfo );
		if ( !mGameMode )
			return false;

		return true;
	}


	bool LevelStage::getAttribValue( AttribValue& value )
	{
		switch( value.id )
		{
		case ATTR_REPLAY_UI_POS:
			value.v2.x = UI_BasePos.x + ( 150 - UI_ButtonSize.x ) / 2;
			value.v2.y = UI_BasePos.y + 200 - 18;
			return true;

		case ATTR_REPLAY_INFO:
			{
				ReplayInfo* info = reinterpret_cast< ReplayInfo* >( value.ptr );
				GameInfo& gameInfo = reinterpret_cast< GameInfoData*> ( info->gameInfoData )->info;

				IPlayerManager* playerManager = getPlayerManager();
				gameInfo.mode      = mGameMode->getModeID();
				gameInfo.numLevel  = (unsigned)mWorld->getLevelNum();

				LevelData* lvData = mWorld->findPlayerData( playerManager->getUserID() );
				if ( lvData )
					gameInfo.userID = lvData->getID();
				else
					gameInfo.userID = 0;

				mGameMode->getGameInfo( gameInfo );
			}
			return true;
		}
		return BaseClass::getAttribValue( value );
	}

	void LevelStage::setupLocalGame( LocalPlayerManager& playerManager )
	{

	}

	static unsigned titleMap[] = 
	{
		BINARY32( 11111111,11110100,11000000 , 00000000 ) , 
		BINARY32( 01010001,01010101,00000000 , 00000000 ) , 
		BINARY32( 01011101,01100101,11000000 , 00000000 ) , 
		BINARY32( 01010001,01010100,01000000 , 00000000 ) , 
		BINARY32( 01011101,01010101,10000000 , 00000000 ) , 
	};

	static int const gBGColor[] = 
	{ 
		Color::eRed , Color::ePurple , Color::eBlue , Color::eOrange , Color::eCyan,
		Color::eGreen , Color::eYellow , Color::eGray , Color::eWhite 
	};

	bool MenuStage::onInit()
	{
		if ( !BaseClass::onInit() )
			return false;

		::Global::getGUI().cleanupWidget();

		changeGroup( eMainGroup );

		offsetBG.setValue( 0 , 0 );

		ix = 0;
		iy = 0;
		t = 0.0;

		return true;
	}

	void MenuStage::onRender( float dFrame )
	{
		DrawEngine* de = Global::getDrawEngine();
		Graphics2D& g = Global::getGraphics2D();

		for ( int j = 0 ; j < 10 ; ++j )
		{
			for ( int i = 0 ; i < 10 ; ++i )
			{
				Vec2i pos( ( i -1 )* 6 * BlockSize + offsetBG.x / 2 , 
					( j- 1 )* 6 * BlockSize + offsetBG.y / 2 );
				int color = gBGColor[ ( i + ix + j + iy ) % ARRAYSIZE( gBGColor ) ];
				RenderUtility::drawBlock( g , pos , 6 , 6 , color );
			}
		}

		int color[] = { Color::eRed , Color::ePurple , Color::eBlue , Color::eOrange , Color::eCyan, Color::eGreen };


		Vec2i const titlePos( 230 , 150 );
		g.beginBlend( Vec2i(0,0) , de->getScreenSize() , 0.7f + 0.3f * fabs(sin(t)) );

		Vec2i blockPos = titlePos;

		for ( int i = 0 ; i < ARRAYSIZE( titleMap ) ; ++ i )
		{
			int ci = 0;
			for ( unsigned bit = 0x80000000 ; bit > ( 1 << 8 ) ; bit >>= 1 )
			{
				if ( titleMap[i] & bit )
					RenderUtility::drawBlock( g , blockPos , color[ ci / 3 ] );

				blockPos.x += BlockSize;
				++ci;
			}

			blockPos.y += BlockSize;
			blockPos.x = titlePos.x;
		}

		g.endBlend();
	}


	void MenuStage::onUpdate( long time )
	{
		t += 0.001f * time;

		offsetBG.x += 2;
		if ( offsetBG.x > 12 * BlockSize )
		{
			offsetBG.x -= 12 * BlockSize; 
			ix = ( ix - 1 + ARRAYSIZE( gBGColor ) ) % ( ARRAYSIZE( gBGColor ));
		}

		offsetBG.y += 3;
		if ( offsetBG.y > 12 * BlockSize )
		{
			offsetBG.y -= 12 * BlockSize;
			iy = ( iy - 1 + ARRAYSIZE( gBGColor ) )% ( ARRAYSIZE( gBGColor ) );
		}
	}


	bool MenuStage::onEvent( int event , int id , GWidget* ui )
	{
		switch ( id )
		{
		case UI_SHOW_SCORE:
			getManager()->changeStage( STAGE_RECORD_GAME );
			break;
		case UI_ABOUT_GAME:
			getManager()->changeStage( STAGE_ABOUT_GAME );
			break;
		case UI_BACK_GROUP:
			popGroup();
			return false;

		case UI_GAME_OPTION:
			{
				Vec2i pos = ::Global::getGUI().calcScreenCenterPos( OptionNoteBook::UI_Size );
				OptionNoteBook* book = new OptionNoteBook( UI_GAME_OPTION , pos  , NULL );
				book->init( Global::getGameManager().getCurGame()->getController() );
				::Global::getGUI().addWidget( book );
				book->setTop();
				book->doModal();
			}
			return false;
		case UI_BATTLE_MODE:
		case UI_CHALLENGE_MODE:
		case UI_PRACTICE_MODE:
		case UI_TIME_MODE:
			{
				GameInfo info;
				info.userID    = 0;

				switch( id )
				{
				case UI_CHALLENGE_MODE:
					info.mode      = MODE_TS_CHALLENGE;
					info.numLevel = 1;
					info.modeNormal.startGravityLevel = 0;
					break;
				case UI_BATTLE_MODE: 
					info.mode      = MODE_TS_BATTLE; 
					info.numLevel = 2;
					break;
				case UI_PRACTICE_MODE:
					info.mode      = MODE_TS_PRACTICE; 
					info.numLevel = 1;
					break;
				default:
					info.mode = -1;
				}
				bool isOK = false;

				if ( info.mode != -1 )
				{
					GameSingleStage* stage = static_cast< GameSingleStage* >( 
						getManager()->changeStage( STAGE_SINGLE_GAME ) );

					if ( stage->getSubStage()->setupAttrib( AttribValue( ATTR_GAME_INFO , &info ) )  )
						isOK = true;
				}

				if ( !isOK )
				{
					getManager()->changeStage( STAGE_GAME_MENU );
				}
			}
			return false;
		case UI_YES :
			switch ( ::Global::getGUI().getModalID() )
			{
			case UI_GAME_OPTION:
				{
					::Global::getGUI().getManager().getModalUI()->destroy();
				}
				return false;
			}
			break;
		}

		return true;
	}

	void MenuStage::doChangeGroup( StageGroupID group )
	{
		Vec2i btnSize( 120 , 25 );
		int xUi = ( Global::getDrawEngine()->getScreenWidth() - btnSize.x ) / 2 ;
		int yUi = 250;
		int offset = 30;

		int delay = 0;
		int dDelay = 100;

		fadeoutGroup( dDelay );
		mGroupUI.clear();

#define CREATE_BUTTON( ID , NAME )\
	createButton( delay += dDelay , ID  , NAME  , Vec2i( xUi , yUi += offset  ) , btnSize )

		switch( group )
		{
		case eMainGroup:
			CREATE_BUTTON( UI_CHALLENGE_MODE, LAN("Challenge Mode"));
			CREATE_BUTTON( UI_BATTLE_MODE   , LAN("Battle Mode")   );
			CREATE_BUTTON( UI_PRACTICE_MODE , LAN("Practice Mode") );
			//CREATE_BUTTON( UI_TIME_MODE     , LAN("Time Mode")     );
			CREATE_BUTTON( UI_SHOW_SCORE    , LAN("Show Score")    );
			CREATE_BUTTON( UI_GAME_OPTION   , LAN("Option")        );
			CREATE_BUTTON( UI_MAIN_MENU     , LAN("Back Main Menu"));
			break;
		}
#undef CREATE_BUTTON
	}

	class ComputerAI : public AIBase
	{
	public:

		void upate( unsigned time )
		{

		}

		int calcPiece( Piece& piece , int x )
		{



		}
		Scene* mScene;
		Level* mLevel;
	};

	static void  renderVortex( Graphics2D& g , Vec2f const& center )
	{
		static float angle = 0;
		angle += 0.01f;

		g.beginXForm();
		g.setPen( ColorKey3(0,0,0));

		for( int i = 0; i < 20 ; ++ i )
		{
			g.identityXForm();
			g.translateXForm( center.x + 10 , center.y + 10 );
			g.rotateXForm( 0.13f * angle * i );
			float s = 0.2f * ( 1 - i * 1.0f / 200 );
			g.scaleXForm( s , s );
			g.drawRect( Vec2i(0,0) , Vec2i(1000 + i ,1000 + i ) );
		}
		g.finishXForm();
	}

	void AboutGameStage::onRender( float dFrame )
	{
		//RenderUtility::setGraphicsBursh( de->getGraphics2D() , Color::eBlue );
		//renderVortex();

		for( int i = 0 ; i < TotalSpriteNum ; ++i)
			sprite[i].render();

	}

	bool AboutGameStage::onInit()
	{
		curIndex = 0;
		return true;
	}

	void AboutGameStage::onUpdate( long time )
	{
		static long count = 0;

		static long const tiggerTime = 100;

		count += time;

		if ( count > tiggerTime )
		{
			productSprite( sprite[ curIndex ] );
			count -= tiggerTime;
			++curIndex;
			if ( curIndex >= TotalSpriteNum )
				curIndex = 0;
		}
		float dt = time * 0.001f;

		for( int i = 0 ; i < TotalSpriteNum ; ++i)
			sprite[i].update( dt );

		//if ( ( pos.x < 0  && vel.x < 0 ) || 
		//	 ( pos.x > Global::getDrawEngine()->getScreenWidth() && vel.x > 0 ) )
		//	 vel.x = -vel.x;
		//if ( ( pos.y < 0  && vel.y < 0 ) || 
		//	 ( pos.y > Global::getDrawEngine()->getScreenHeight() && vel.y > 0 ) )
		//	 vel.y = -vel.y;
	}

	float RandomFloat()
	{
		return float( Global::Random() ) / ( float )RAND_MAX;
	}

	float RandomFloat( float s , float e )
	{
		return s + ( e - s ) * float( Global::Random() ) / ( float )RAND_MAX;
	}

	void AboutGameStage::productSprite( PieceSprite& spr )
	{
		PieceTemplateSet& tempSet = PieceTemplateSet::getClassicSet();
		tempSet.setTemplate( tempSet.getTemplateNum() % Global::Random() , spr.piece );
		spr.piece.rotate( spr.piece.getDirNum() % Global::Random() );
		spr.angle    = RandomFloat() * 2 * PI;
		spr.angleVel = RandomFloat( -1 , 1 ) * 5;
		spr.vel   =  500 * Vec2f( RandomFloat( -1 , 1 )  , RandomFloat( -1 , 1 ) );
		spr.pos   =  Vec2f( 400 , 300 ) + 50 * Vec2f( RandomFloat( -1 , 1 )  , RandomFloat( -1 , 1 ) );
	}

	void AboutGameStage::PieceSprite::render()
	{
		Graphics2D& g = Global::getGraphics2D();
		g.beginXForm();
		g.translateXForm( pos.x , pos.y );
		g.rotateXForm( angle );

		for( int i = 0 ; i < piece.getBlockNum(); ++i )
		{
			Tetris::Block const& block = piece.getBlock(i);
			Vec2i bPos = BlockSize * Vec2i( block.getX() , - block.getY() );
			RenderUtility::drawBlock( g , bPos , Tetris::Piece::getColor( block.getType() ) );
		}
		g.finishXForm();
	}

	void AboutGameStage::PieceSprite::update( float dt )
	{
		angle += angleVel * dt;
		pos   += vel * dt;
	}

	RecordStage::RecordStage()
	{
		chName = false;
		lightOrder = -1;
		idxChar = 0;
	}


	bool RecordStage::onInit()
	{
		Vec2i panelSize = Vec2i( 400 , 440 );
		::Global::getGUI().cleanupWidget();

		GPanel* panel;
		Vec2i pos = GUISystem::calcScreenCenterPos( panelSize );
		panel = new GPanel( UI_PANEL , pos, panelSize , NULL );
		panel->setAlpha( 0.8f );
		panel->setRenderCallback( RenderCallBack::create( this , &RecordStage::renderRecord ) );
		::Global::getGUI().addWidget( panel );

		Vec2i btnSize( 100 , 20 );

		GButton* button;
		button = new GButton( UI_GAME_MENU , Vec2i( (panelSize.x - btnSize.x) / 2 ,panelSize.y - 30 ) , btnSize , panel );
		button->setTitle( LAN("Menu") );

		lightBlink = 0;
		return true;
	}

	void RecordStage::onRender( float dFrame )
	{

	}


	void RecordStage::setPlayerOrder( int order )
	{
		if ( order >= RecordManager::NumMaxRecord )
			return;

		lightOrder = order;
		lightRecord = Tetris::getRecordManager().getRecord();
		for ( int i = 0 ; i < order ; ++i )
		{
			lightRecord = lightRecord->next;
		}
		chName = true;
	}

	static void limitChar( char& ch )
	{
		if ( ch < 33 )
			ch = 33;
		if ( ch > 126 )
			ch = 126;
	}

	bool RecordStage::onKey( unsigned key , bool isDown )
	{
		if ( !chName || !isDown )
			return true;

		switch ( key )
		{
		case VK_UP   : limitChar( ++lightRecord->name[ idxChar ] ); break;
		case VK_DOWN : limitChar( --lightRecord->name[ idxChar ] ); break;
		case VK_RIGHT: idxChar = std::min( idxChar + 1 , 2 ); break;
		case VK_LEFT : idxChar = std::max( idxChar - 1 , 0 ); break;
		}
		return false;
	}


	void RecordStage::onUpdate( long time )
	{
		int const freq = 1000;
		lightBlink += time;
		if ( lightBlink > freq )
			lightBlink = -freq / 3;
	}

	void RecordStage::renderRecord( GWidget* ui )
	{
		Graphics2D& g = Global::getGraphics2D();

		Vec2i pos  = ui->getWorldPos();
		Vec2i size = ui->getSize();

		int d = 22;
		int x = pos.x + 10;
		int y = pos.y + 10;

		struct Prop
		{
			char const* name;
			int  titlePos;
			int  valPos;
			char const* fmt;
			char const* empty;

			void drawText( Graphics2D& g , int x , int y , ... ) const
			{
				FixString< 128 > str;
				va_list vl;
				va_start( vl ,y );
				g.drawText( x + valPos , y , str.formatV( fmt , vl ) );
				va_end(vl);
			}

			void drawEmpty( Graphics2D& g , int x , int y  ) const
			{
				g.drawText( x + valPos , y , empty );
			}
		}; 

		static const Prop propList[] =
		{
			"No."   , 0   , 7   , "%02d", "--",
			"Name"  , 50  , 50  , "%s",   "---",
			"Score" , 140 , 115 , "%07d", "-------",
			"Level" , 210 , 210 , "%03d", "---",
			"Time"  , 320 , 280 , "%02d:%02d:%03d", "--:--:---",
		};

		//de->setTextMode();
		RenderUtility::setFont( g , FONT_S16 );
		g.setTextColor(255 , 255 , 0 );

		for( int i = 0 ; i < ARRAY_SIZE( propList ); ++i )
		{
			g.drawText( x +  propList[i].titlePos , y , propList[i].name );
		}

		g.setTextColor(255 , 255 , 255 );

		Record* curRecord = Tetris::getRecordManager().getRecord();
		for( int i = 0 ; i < RecordManager::NumMaxRecord ; ++ i )
		{
			y += d;
			if ( curRecord )
			{
				long sec = ( curRecord->durtion / 1000 );
				long min = ( sec / 60 ); 

				propList[0].drawText( g , x , y , i + 1 );
				propList[1].drawText( g , x , y , curRecord->name );
				propList[2].drawText( g , x , y , curRecord->score );
				propList[3].drawText( g , x , y , curRecord->level );
				propList[4].drawText( g , x , y , min , sec % 60 , curRecord->durtion % 1000 );

				curRecord = curRecord->next;
			}
			else
			{
				propList[0].drawEmpty( g , x , y );
				propList[1].drawEmpty( g , x , y );
				propList[2].drawEmpty( g , x , y );
				propList[3].drawEmpty( g , x , y );
				propList[4].drawEmpty( g , x , y );
				//sprintf( str , "%02d  ---  -------  ---  --:--:---"  , i + 1 );
				//g.drawText( x , y , str );
			}

			switch ( i )
			{
			case 0 : g.setBrush( ColorKey3( 255 , 215, 0 ) ); break;
			case 1 : g.setBrush( ColorKey3( 192,192,192 ) ); break;
			case 2 : g.setBrush( ColorKey3( 184, 115, 51 ) ); break;
			default: g.setBrush( ColorKey3( 0 , 255 , 255 ) ); break;
			}

			if ( i == lightOrder && lightBlink > 0 )
			{
				RenderUtility::setBrush( g, Color::eRed );
			}

			g.beginBlend( Vec2i(0,y) , Vec2i( Global::getDrawEngine()->getScreenWidth() , 22 ), 0.5f );
			g.drawRoundRect( Vec2i( x - 8 , y ) , Vec2i( size.x - 6 , 22) , Vec2i( 10 , 10 ) );
			g.endBlend();
		}
	}


	bool RecordStage::onEvent( int event , int id , GWidget* ui )
	{
		switch ( id )
		{
		case UI_GAME_MENU:
			getManager()->changeStage( STAGE_GAME_MENU );
			return false;
		}
		return BaseClass::onEvent( event , id , ui );
	}





}//namespace Tetris
