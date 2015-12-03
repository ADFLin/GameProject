#include "TinyGamePCH.h"
#include "ReplayStage.h"

#include "GamePackage.h"
#include "GamePackageManager.h"

#include "GameReplay.h"
#include "GameAction.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"

#include "GameStage.h"
#include "GameSingleStage.h"

#include "FileSystem.h"

#include <algorithm>

int const g_ReplayNormalSpeed = 1 << 5;
int const g_ReplaySpeed[] =
{
	g_ReplayNormalSpeed >> 4 , 
	g_ReplayNormalSpeed >> 3 , 
	g_ReplayNormalSpeed >> 2 , 
	g_ReplayNormalSpeed >> 1 , 
	g_ReplayNormalSpeed ,
	g_ReplayNormalSpeed << 1 , 
	g_ReplayNormalSpeed << 2 , 
	g_ReplayNormalSpeed << 3 ,
	g_ReplayNormalSpeed << 4 , 
};
int const g_NormalSpeedIndex = ARRAY_SIZE( g_ReplaySpeed ) / 2;


template < class T , class P , class Q >
T clamp( T val , P low , Q hi )
{
	return std::min( std::max( (T) low , val ) , (T)hi );
}

ReplayListPanel::ReplayListPanel( int id , Vec2i const& pos , Vec2i const& size , GWidget* parent ) :BaseClass( id , pos , size , parent )
{
	mCurDir    = REPLAY_DIR;

	int  broader = 10;
	mFileListCtrl = new GListCtrl( 
		ReplayEditStage::UI_REPLAY_LIST , 
		Vec2i( broader , broader ) , 
		size - Vec2i( 2 * broader , 2 * broader ) , this );
}


String ReplayListPanel::getFilePath()
{
	char const* name = mFileListCtrl->getSelectValue();
	if ( name )
		return mCurDir + "/" + name;

	return String();
}

bool ReplayListPanel::onChildEvent( int event , int id , GWidget* ui )
{

	return true;
}

void ReplayListPanel::loadDir( char const* dir )
{
	mCurDir = dir;
	mFileListCtrl->removeAllItem();

	FileIterator fileIter;
	if ( !FileSystem::findFile( dir , REPLAY_SUB_FILE_NAME , fileIter ) )
		return;

	for( ; fileIter.haveMore() ; fileIter.goNext() )
	{
		mFileListCtrl->appendItem( fileIter.getFileName() );
	}
}

void ReplayListPanel::deleteCurFile()
{
	if ( mFileListCtrl->getSelection() == -1 )
		return;
	String path = getFilePath();
	::DeleteFile( path.c_str() );

	mFileListCtrl->removeItem( mFileListCtrl->getSelection() );
}


GameReplayStage::GameReplayStage() 
	:BaseClass( GT_REPLAY )
	,mPlayerManager( new LocalPlayerManager )
{
}

bool GameReplayStage::onInit()
{
	ReplayInfo   info;
	ReplayHeader header;
	if ( !ReplayBase::loadReplayInfo( mReplayFilePath.c_str() , header , info ) )
	{
		return false;
	}

	AttribValue attrInfo( ATTR_REPLAY_INFO , &info );
	if ( !getSubStage()->setupAttrib( attrInfo ) )
	{
		return false;
	}

	if ( !BaseClass::onInit() )
		return false;
	//// Replay Input////////////

	IFrameActionTemplate* actionTemplate = NULL;
	if ( header.version >= VERSION( 0,1,0 ) )
	{
		actionTemplate = getSubStage()->createActionTemplate( info.templateVersion );
		if ( !actionTemplate )
			return false;

		mReplayInput.reset( new ReplayInput( actionTemplate , mReplayFrame ) );
	}
	else
	{
		ReplayTemplate* replayTemp = getGame()->createReplayTemplate( info.templateVersion );

		if ( !replayTemp )
		{
			return false;
		}
		mReplayInput.reset( new OldVersion::ReplayInput( replayTemp , mReplayFrame ) );
	}

	if ( !loadReplay( mReplayFilePath.c_str() ) )
		return false;

	getActionProcessor().addInput( *mReplayInput.get() );
	///////////////////////////////////////////////////////////////
	::Global::getGUI().cleanupWidget();

	mSubStage->setupScene( *mPlayerManager.get() );

	if ( actionTemplate )
		actionTemplate->setupPlayer( *mPlayerManager );

	AttribValue attrPos( ATTR_REPLAY_UI_POS );
	mSubStage->getAttribValue( attrPos );

	//create replay control UI
	Vec2i baseSize( 120 , 20 );

	int xUi = attrPos.v2.x;
	int yUi = attrPos.v2.y;

	int broader = 8;
	int offset = 28;

	GFrame*  frame = new GFrame( UI_ANY , Vec2i(xUi ,yUi) , Vec2i(  baseSize.x + 2 * broader , 2 * baseSize.y + 2 * broader )  , NULL );
	frame->setRenderType( GPanel::eRectType );
	GButton* button;
	::Global::getGUI().addWidget( frame );

	int px = broader;
	int py = 12;

	mProgressSlider = new GSlider( 0 , Vec2i( px , py ), baseSize.x , true , frame );

	Vec2i replayBtnSize( baseSize.x / 4 , baseSize.y );

	py += 18;

	button = new GButton( UI_REPLAY_RESTART , Vec2i( px , py  ) , replayBtnSize , frame );
	button->setTitle( "R" );

	button = new GButton( UI_REPLAY_TOGGLE_PAUSE , Vec2i(  px + replayBtnSize.x , py ) , replayBtnSize , frame );
	button->setTitle( "=" );

	button = new GButton( UI_REPLAY_SLOW , Vec2i( px + 2 * replayBtnSize.x , py  ) , replayBtnSize , frame );
	button->setTitle( "<<" );

	button = new GButton( UI_REPLAY_FAST , Vec2i( px + 3 * replayBtnSize.x , py  ) , replayBtnSize , frame );
	button->setTitle( ">>" );
	//



	restart( true );
	return true;
}

void GameReplayStage::onEnd()
{
	BaseClass::onEnd();
}


void GameReplayStage::onUpdate( long time )
{
	if ( !mReplayInput->isVaild() )
		return;

	if ( mReplayInput->isPlayEnd() )
		changeState( GS_END );

	int numFrame = time / gDefaultTickTime;

	int numGameFrame = 0;

	for( int i = 0 ; i < numFrame ; ++i )
	{
		switch ( getState() )
		{
		case GS_RUN:
			mReplayUpdateCount += mReplaySpeed;
			while( mReplayUpdateCount >= g_ReplayNormalSpeed )
			{
				++numGameFrame;
				++mReplayFrame;
				getActionProcessor().beginAction();
				if ( !mReplayInput->isPlayEnd() )
					getSubStage()->tick();
				getActionProcessor().endAction();

				mReplayUpdateCount -= g_ReplayNormalSpeed;
				if ( getState() == GS_END )
					break;
			}
			break;

		default:
			++numGameFrame;
			getActionProcessor().beginAction( CTF_FREEZE_FRAME );
			getSubStage()->tick();
			getActionProcessor().endAction();
		}
	}

	getSubStage()->updateFrame( numGameFrame );


	::Global::getGUI().scanHotkey( getGame()->getController() );

	int totalFrame = mReplayInput->getRecordFrame();
	if ( totalFrame )
		mProgressSlider->setValue( int ( 1000 * mReplayFrame / totalFrame ) );
	else
		mProgressSlider->setValue( 0 );
}

bool GameReplayStage::loadReplay( char const* path )
{
	if ( !path )
		return false;

	if ( !mReplayInput->load( path ) )
		return false;

	return true;
}

void  GameReplayStage::onRestart( uint64& seed )
{
	seed = mReplayInput->getSeed();

	BaseClass::onRestart( seed );

	mReplaySpeed = g_ReplayNormalSpeed;
	mReplayUpdateCount = 0;
	mIndexSpeed = g_NormalSpeedIndex;
	mReplayInput->restart();
}

bool GameReplayStage::onEvent( int event , int id , GWidget* ui )
{
	switch( id )
	{
	case UI_REPLAY_RESTART:
		restart( true );
		return false;
	case UI_REPLAY_TOGGLE_PAUSE:
		assert( event == EVT_BUTTON_CLICK );
		if ( togglePause() )
		{
			GUI::castFast< GButton* >(ui)->setTitle( ( getState() == GS_PAUSE ) ? "->" : "=" );
		}
		return false;
	case UI_REPLAY_FAST:
		mIndexSpeed = clamp( mIndexSpeed + 1 , 0 , ARRAY_SIZE( g_ReplaySpeed ) - 1 );
		mReplaySpeed = g_ReplaySpeed[ mIndexSpeed ];
		return false;
	case UI_REPLAY_SLOW:
		mIndexSpeed = clamp( mIndexSpeed - 1 , 0 , ARRAY_SIZE( g_ReplaySpeed ) - 1 );
		mReplaySpeed = g_ReplaySpeed[ mIndexSpeed ];
		return false;
	case UI_GAME_MENU:
		togglePause();
		::Global::getGUI().showMessageBox( UI_MAIN_MENU , LAN("back Main Menu?") );
		return false;

	case UI_MAIN_MENU:
		if ( event == EVT_BOX_YES )
		{
			getManager()->changeStage( STAGE_MAIN_MENU );
			return true;
		}
		else if ( event == EVT_BOX_NO )
		{
			togglePause();
			return true;
		}
		else
		{
			togglePause();
			::Global::getGUI().showMessageBox( UI_MAIN_MENU , LAN("back Main Menu?") );
			return false;
		}
		break;
	}

	return BaseClass::onEvent( event , id , ui );
}


LocalPlayerManager* GameReplayStage::getPlayerManager()
{
	return mPlayerManager.get();
}

bool ReplayEditStage::onInit()
{
	if ( !BaseClass::onInit() )
		return false;


	::Global::getGUI().cleanupWidget();

	Vec2i panelPos( 170 , 30 );
	Vec2i panelSize( 250 , 500 );
	mRLPanel = new ReplayListPanel( UI_REPLAYLIST_PANEL , panelPos , panelSize , NULL );
	::Global::getGUI().addWidget( mRLPanel );
	mRLPanel->loadDir( REPLAY_DIR );

	GPanel* infoPanel = new GPanel( UI_ANY , panelPos + Vec2i( panelSize.x + 10 , 0 ) , Vec2i( 200 , 400 ) , NULL );
	::Global::getGUI().addWidget( infoPanel );
	infoPanel->setRenderCallback( RenderCallBack::create( this , &ReplayEditStage::renderReplayInfo ));

	GButton* button;
	Vec2i buttonPos = panelPos + Vec2i( panelSize.x + 10 , 400 + 10 );
	Vec2i buttonSize( 200 , 25 );
	int offset = buttonSize.y + 8;
	button = new GButton( UI_VIEW_REPLAY , buttonPos , buttonSize , NULL );
	button->setTitle( LAN( "View Replay" ) );
	::Global::getGUI().addWidget( button );
	mViewButton = button;
	mViewButton->enable( false );
	buttonPos.y += offset;

	button = new GButton( UI_DELETE_REPLAY , buttonPos , buttonSize , NULL );
	button->setTitle( LAN( "Delete Replay" ) );
	::Global::getGUI().addWidget( button );
	mDelButton = button;
	mDelButton->enable( false );
	buttonPos.y += offset;

	button = new GButton( UI_MAIN_MENU , buttonPos , buttonSize , NULL );
	button->setTitle( LAN( "Back Main Menu" ) );
	::Global::getGUI().addWidget( button );
	buttonPos.y += offset;

	return true;
}

bool ReplayEditStage::onEvent( int event , int id , GWidget* ui )
{
	switch( id )
	{
	case UI_REPLAY_LIST:
		switch( event )
		{
		case EVT_LISTCTRL_DCLICK:
			viewReplay();
			break;
		case EVT_LISTCTRL_SELECT:
			mReplayFilePath = mRLPanel->getFilePath();
			mIsVaildReplay = Replay::loadReplayInfo( mReplayFilePath.c_str() , mReplayHeader , mGameInfo );
			mViewButton->enable( mIsVaildReplay );
			mDelButton->enable( true );
			break;
		}
		return false;
	case UI_DELETE_REPLAY:
		{
			mRLPanel->deleteCurFile();
			mViewButton->enable( false );
			mDelButton->enable( false );
		}
		return false;
	case UI_VIEW_REPLAY:
		viewReplay();
		return false;
	case UI_MAIN_MENU:
		getManager()->changeStage( STAGE_MAIN_MENU );
		return false;
	}
	return true;
}

void ReplayEditStage::renderReplayInfo( GWidget* ui )
{
	if ( !mIsVaildReplay )
		return;

	Vec2i pos = ui->getWorldPos();
	Graphics2D& g = Global::getGraphics2D();
	

	int px = pos.x + 10;
	int py = pos.y + 10;
	int d = 20;

	g.setTextColor(255 , 255 , 255 );

	FixString< 256 > str;
	str.format( "Game Name : %s" , mGameInfo.name );
	g.drawText( Vec2i( px , py ) , str );
	str.format( "Frame Number : %u" , mReplayHeader.totalFrame );
	py += d;
	g.drawText( Vec2i( px , py ) , str );
	str.format( "File Size : %u" , mReplayHeader.totalSize );
	py += d;
	g.drawText( Vec2i( px , py ) , str );
}

void ReplayEditStage::viewReplay()
{
	if ( !mIsVaildReplay )
		return;

	IGamePackage* game = Global::getGameManager().changeGame( mGameInfo.name );
	if ( game )
	{
		GameReplayStage* stage = static_cast< GameReplayStage* >( 
			getManager()->changeStage( STAGE_REPLAY_GAME ) );
		stage->setReplayPath( mReplayFilePath );
	}
}
