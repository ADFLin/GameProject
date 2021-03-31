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

int const gReplayNormalSpeed = 1 << 5;
int const gReplaySpeed[] =
{
	gReplayNormalSpeed >> 4 , 
	gReplayNormalSpeed >> 3 , 
	gReplayNormalSpeed >> 2 , 
	gReplayNormalSpeed >> 1 , 
	gReplayNormalSpeed ,
	gReplayNormalSpeed << 1 , 
	gReplayNormalSpeed << 2 , 
	gReplayNormalSpeed << 3 ,
	gReplayNormalSpeed << 4 , 
};
int const gNormalSpeedIndex = ARRAY_SIZE( gReplaySpeed ) / 2;


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
			mbReplayValid = Replay::LoadReplayInfo( mReplayFilePath.c_str() , mReplayHeader , mGameInfo );
			mViewButton->enable( mbReplayValid );
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
	if ( !mbReplayValid )
		return;

	Vec2i pos = ui->getWorldPos();
	Graphics2D& g = Global::GetGraphics2D();
	

	int px = pos.x + 10;
	int py = pos.y + 10;
	int d = 20;

	g.setTextColor(Color3ub(255 , 255 , 255) );

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
	if ( !mbReplayValid )
		return;

	IGameModule* game = Global::ModuleManager().changeGame( mGameInfo.name );
	if ( game )
	{
		game->beginPlay( *getManager() , EGameStageMode::Replay );
		//#TODO
		StageBase* nextStage = getManager()->getNextStage();
		if( auto gameStage = nextStage->getGameStage() )
		{
			if ( auto replayMode = gameStage->getStageMode()->getReplayMode() )
			{
				replayMode->setReplayPath(mReplayFilePath);

			}
		}
	}
}

ReplayStageMode::ReplayStageMode() 
	:BaseClass(EGameStageMode::Replay)
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
	if( !ReplayBase::LoadReplayInfo(mReplayFilePath.c_str(), header, info) )
	{
		return false;
	}

	GameAttribute attrInfo(ATTR_REPLAY_INFO, &info);
	if( !stage->setupAttribute(attrInfo) )
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

	if( actionTemplate )
		actionTemplate->setupPlayer(*mPlayerManager);


	//GameLevelInfo info;
	//mReplayInput->getLevelInfo(info);
	
	stage->setupScene(*mPlayerManager.get());


	GameAttribute attrPos(ATTR_REPLAY_UI_POS);
	if( !stage->queryAttribute(attrPos) )
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

	restart(true);
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
	if( !mReplayInput->isValid() )
		return;

	if( mReplayInput->isPlayEnd() )
		changeState(EGameState::End);

	int numFrame = time / mCurStage->getTickTime();

	int numGameFrame = 0;

	ActionProcessor& processor = getStage()->getActionProcessor();
	for( int i = 0; i < numFrame; ++i )
	{
		switch( getGameState() )
		{
		case EGameState::Run:
			mReplayUpdateCount += mReplaySpeed;
			while( mReplayUpdateCount >= gReplayNormalSpeed )
			{
				++numGameFrame;
				++mReplayFrame;
				processor.beginAction();
				if (!mReplayInput->isPlayEnd())
				{
					getStage()->tick();
				}
				processor.endAction();

				mReplayUpdateCount -= gReplayNormalSpeed;
				if( getGameState() == EGameState::End )
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

	if (getGame())
	{
		::Global::GUI().scanHotkey(getGame()->getController());
	}

	int totalFrame = mReplayInput->getRecordFrame();
	mProgressSlider->setValue(totalFrame ? int(1000 * mReplayFrame / totalFrame) : 0);
}

bool ReplayStageMode::onWidgetEvent(int event, int id, GWidget* ui)
{
	switch( id )
	{
	case UI_REPLAY_RESTART:
		restart(false);
		return false;
	case UI_REPLAY_TOGGLE_PAUSE:
		assert(event == EVT_BUTTON_CLICK);
		if( togglePause() )
		{
			GUI::CastFast< GButton >(ui)->setTitle((getGameState() == EGameState::Pause) ? "->" : "=");
		}
		return false;
	case UI_REPLAY_FAST:
		mIndexSpeed = clamp(mIndexSpeed + 1, 0, ARRAY_SIZE(gReplaySpeed) - 1);
		mReplaySpeed = gReplaySpeed[mIndexSpeed];
		return false;
	case UI_REPLAY_SLOW:
		mIndexSpeed = clamp(mIndexSpeed - 1, 0, ARRAY_SIZE(gReplaySpeed) - 1);
		mReplaySpeed = gReplaySpeed[mIndexSpeed];
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

	mReplaySpeed = gReplayNormalSpeed;
	mReplayUpdateCount = 0;
	mIndexSpeed = gNormalSpeedIndex;
	mReplayInput->restart();
}
