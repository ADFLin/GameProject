#include "TinyGamePCH.h"
#include "ReplayStageMode.h"

#include "GameModule.h"
#include "GameModuleManager.h"

#include "GameReplay.h"
#include "GameAction.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"

#include "GameStage.h"

#include "FileSystem.h"

#include <algorithm>

enum ReplayFileVersion
{
	


};

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
	mFileListCtrl = new GFileListCtrl( 
		ReplayEditStage::UI_REPLAY_LIST , 
		Vec2i( broader , broader ) , 
		size - Vec2i( 2 * broader , 2 * broader ) , this );
	mFileListCtrl->mSubFileName = REPLAY_SUB_FILE_NAME;
	mFileListCtrl->setDir(REPLAY_DIR);
}


String ReplayListPanel::getFilePath()
{
	return mFileListCtrl->getSelectedFilePath();
}

bool ReplayListPanel::onChildEvent( int event , int id , GWidget* ui )
{

	return true;
}

void ReplayListPanel::loadDir( char const* dir )
{
	mFileListCtrl->setDir(dir);
}

void ReplayListPanel::deleteSelectedFile()
{
	mFileListCtrl->deleteSelectdFile();
}

bool ReplayEditStage::onInit()
{
	if ( !BaseClass::onInit() )
		return false;


	::Global::GUI().cleanupWidget();

	Vec2i panelPos( 170 , 30 );
	Vec2i panelSize( 250 , 500 );
	mRLPanel = new ReplayListPanel( UI_REPLAYLIST_PANEL , panelPos , panelSize , NULL );
	::Global::GUI().addWidget( mRLPanel );
	mRLPanel->loadDir( REPLAY_DIR );

	GPanel* infoPanel = new GPanel( UI_ANY , panelPos + Vec2i( panelSize.x + 10 , 0 ) , Vec2i( 200 , 400 ) , NULL );
	::Global::GUI().addWidget( infoPanel );
	infoPanel->setRenderCallback( RenderCallBack::Create( this , &ReplayEditStage::renderReplayInfo ));

	GButton* button;
	Vec2i buttonPos = panelPos + Vec2i( panelSize.x + 10 , 400 + 10 );
	Vec2i buttonSize( 200 , 25 );
	int offset = buttonSize.y + 8;
	button = new GButton( UI_VIEW_REPLAY , buttonPos , buttonSize , NULL );
	button->setTitle( LOCTEXT( "View Replay" ) );
	::Global::GUI().addWidget( button );
	mViewButton = button;
	mViewButton->enable( false );
	buttonPos.y += offset;

	button = new GButton( UI_DELETE_REPLAY , buttonPos , buttonSize , NULL );
	button->setTitle( LOCTEXT( "Delete Replay" ) );
	::Global::GUI().addWidget( button );
	mDelButton = button;
	mDelButton->enable( false );
	buttonPos.y += offset;

	button = new GButton( UI_MAIN_MENU , buttonPos , buttonSize , NULL );
	button->setTitle( LOCTEXT( "Back Main Menu" ) );
	::Global::GUI().addWidget( button );
	buttonPos.y += offset;

	return true;
}

bool ReplayEditStage::onWidgetEvent( int event , int id , GWidget* ui )
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
			mIsReplay = Replay::loadReplayInfo( mReplayFilePath.c_str() , mReplayHeader , mGameInfo );
			mViewButton->enable( mIsReplay );
			mDelButton->enable( true );
			break;
		}
		return false;
	case UI_DELETE_REPLAY:
		{
			mRLPanel->deleteSelectedFile();
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
	if ( !mIsReplay )
		return;

	Vec2i pos = ui->getWorldPos();
	Graphics2D& g = Global::getGraphics2D();
	

	int px = pos.x + 10;
	int py = pos.y + 10;
	int d = 20;

	g.setTextColor(255 , 255 , 255 );

	FixString< 256 > str;
	str.format( "Game Name : %s" , mGameInfo.name.c_str() );
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
	if ( !mIsReplay )
		return;

	IGameModule* game = Global::GameManager().changeGame( mGameInfo.name );
	if ( game )
	{
		game->beginPlay( SMT_REPLAY , *getManager() );
		//#TODO
		if( auto gameStage = dynamic_cast<GameStageBase*>(getManager()->getNextStage()) )
		{
			if ( auto replayMode = dynamic_cast<ReplayStageMode*>(gameStage->getStageMode()) )
			{
				replayMode->setReplayPath(mReplayFilePath);

			}
		}
	}
}

ReplayStageMode::ReplayStageMode() 
	:BaseClass(SMT_REPLAY)
	, mPlayerManager(new LocalPlayerManager)
{

}

LocalPlayerManager* ReplayStageMode::getPlayerManager()
{
	return mPlayerManager.get();
}
bool ReplayStageMode::prevStageInit()
{
	if( !BaseClass::prevStageInit() )
		return false;

	GameStageBase* stage = getStage();
	ReplayInfo   info;
	ReplayHeader header;
	if( !ReplayBase::loadReplayInfo(mReplayFilePath.c_str(), header, info) )
	{
		return false;
	}

	AttribValue attrInfo(ATTR_REPLAY_INFO, &info);
	if( !stage->setupAttrib(attrInfo) )
	{
		return false;
	}

	//// Replay Input////////////

	actionTemplate = NULL;
	if( header.version >= MAKE_VERSION(0, 1, 0) )
	{
		actionTemplate = stage->createActionTemplate(info.templateVersion);
		if( !actionTemplate )
			return false;

		mReplayInput.reset(new ReplayInput(actionTemplate, mReplayFrame));
	}
	else if ( getGame() )
	{
		ReplayTemplate* replayTemp = getGame()->createReplayTemplate(info.templateVersion);

		if( !replayTemp )
		{
			return false;
		}
		mReplayInput.reset(new OldVersion::ReplayInput(replayTemp, mReplayFrame));
	}
	else
	{
		return false;
	}

	if( !loadReplay(mReplayFilePath.c_str()) )
		return false;

	stage->getActionProcessor().addInput(*mReplayInput.get());
	///////////////////////////////////////////////////////////////

	return true;
}
bool ReplayStageMode::postStageInit()
{
	if( !BaseClass::postStageInit() )
		return false;

	GameStageBase* stage = getStage();

	::Global::GUI().cleanupWidget();

	stage->setupScene(*mPlayerManager.get());

	if( actionTemplate )
		actionTemplate->setupPlayer(*mPlayerManager);

	AttribValue attrPos(ATTR_REPLAY_UI_POS);
	if( !stage->getAttribValue(attrPos) )
	{
		attrPos.v2.x = 0;
		attrPos.v2.y = 0;
	}

	//create replay control UI
	Vec2i baseSize(120, 20);

	int xUi = attrPos.v2.x;
	int yUi = attrPos.v2.y;

	int broader = 8;
	int offset = 28;

	GFrame*  frame = new GFrame(UI_ANY, Vec2i(xUi, yUi), Vec2i(baseSize.x + 2 * broader, 2 * baseSize.y + 2 * broader), NULL);
	frame->setRenderType(GPanel::eRectType);
	GButton* button;
	::Global::GUI().addWidget(frame);

	int px = broader;
	int py = 12;

	mProgressSlider = new GSlider(0, Vec2i(px, py), baseSize.x, true, frame);

	Vec2i replayBtnSize(baseSize.x / 4, baseSize.y);

	py += 18;

	button = new GButton(UI_REPLAY_RESTART, Vec2i(px, py), replayBtnSize, frame);
	button->setTitle("R");

	button = new GButton(UI_REPLAY_TOGGLE_PAUSE, Vec2i(px + replayBtnSize.x, py), replayBtnSize, frame);
	button->setTitle("=");

	button = new GButton(UI_REPLAY_SLOW, Vec2i(px + 2 * replayBtnSize.x, py), replayBtnSize, frame);
	button->setTitle("<<");

	button = new GButton(UI_REPLAY_FAST, Vec2i(px + 3 * replayBtnSize.x, py), replayBtnSize, frame);
	button->setTitle(">>");
	//

	restartImpl(true);
	return true;
}

bool ReplayStageMode::loadReplay(char const* path)
{
	if( !path )
		return false;

	if( !mReplayInput->load(path) )
		return false;

	return true;
}

void ReplayStageMode::onEnd()
{
	BaseClass::onEnd();
}

void ReplayStageMode::updateTime(long time)
{
	if( !mReplayInput->is() )
		return;

	if( mReplayInput->isPlayEnd() )
		changeState(GS_END);

	int numFrame = time / gDefaultTickTime;

	int numGameFrame = 0;

	ActionProcessor& processor = getStage()->getActionProcessor();
	for( int i = 0; i < numFrame; ++i )
	{
		switch( getGameState() )
		{
		case GS_RUN:
			mReplayUpdateCount += mReplaySpeed;
			while( mReplayUpdateCount >= g_ReplayNormalSpeed )
			{
				++numGameFrame;
				++mReplayFrame;
				processor.beginAction();
				if( !mReplayInput->isPlayEnd() )
					getStage()->tick();
				processor.endAction();

				mReplayUpdateCount -= g_ReplayNormalSpeed;
				if( getGameState() == GS_END )
					break;
			}
			break;

		default:
			++numGameFrame;
			processor.beginAction(CTF_FREEZE_FRAME);
			getStage()->tick();
			processor.endAction();
		}
	}

	getStage()->updateFrame(numGameFrame);

	if ( getGame() )
		::Global::GUI().scanHotkey(getGame()->getController());

	int totalFrame = mReplayInput->getRecordFrame();
	if( totalFrame )
		mProgressSlider->setValue(int(1000 * mReplayFrame / totalFrame));
	else
		mProgressSlider->setValue(0);
}

bool ReplayStageMode::onWidgetEvent(int event, int id, GWidget* ui)
{
	switch( id )
	{
	case UI_REPLAY_RESTART:
		restartImpl(true);
		return false;
	case UI_REPLAY_TOGGLE_PAUSE:
		assert(event == EVT_BUTTON_CLICK);
		if( togglePause() )
		{
			GUI::CastFast< GButton >(ui)->setTitle((getGameState() == GS_PAUSE) ? "->" : "=");
		}
		return false;
	case UI_REPLAY_FAST:
		mIndexSpeed = clamp(mIndexSpeed + 1, 0, ARRAY_SIZE(g_ReplaySpeed) - 1);
		mReplaySpeed = g_ReplaySpeed[mIndexSpeed];
		return false;
	case UI_REPLAY_SLOW:
		mIndexSpeed = clamp(mIndexSpeed - 1, 0, ARRAY_SIZE(g_ReplaySpeed) - 1);
		mReplaySpeed = g_ReplaySpeed[mIndexSpeed];
		return false;
	case UI_GAME_MENU:
		togglePause();
		::Global::GUI().showMessageBox(UI_MAIN_MENU, LOCTEXT("back Main Menu?"));
		return false;

	case UI_MAIN_MENU:
		if( event == EVT_BOX_YES )
		{
			getStage()->getManager()->changeStage(STAGE_MAIN_MENU);
			return true;
		}
		else if( event == EVT_BOX_NO )
		{
			togglePause();
			return true;
		}
		else
		{
			togglePause();
			::Global::GUI().showMessageBox(UI_MAIN_MENU, LOCTEXT("back Main Menu?"));
			return false;
		}
		break;
	}

	return BaseClass::onWidgetEvent(event, id, ui);
}

void ReplayStageMode::onRestart(uint64& seed)
{
	seed = mReplayInput->getSeed();

	BaseClass::onRestart(seed);

	mReplaySpeed = g_ReplayNormalSpeed;
	mReplayUpdateCount = 0;
	mIndexSpeed = g_NormalSpeedIndex;
	mReplayInput->restart();
}
