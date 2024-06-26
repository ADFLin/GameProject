#include "Game.h"

#include "GUISystem.h"
#include "TextureManager.h"
#include "SoundManager.h"
#include "RenderEngine.h"
#include "RenderSystem.h"

#include "DataPath.h"
#include "Platform.h"

#include "InlineString.h"
#include "MarcoCommon.h"
#include "ColorName.h"

#include "RHI/RHIGraphics2D.h"

#include <cassert>
#include <ctime>
#include <iostream>



static IGame* GGameInstance = nullptr;
IGame* getGame(){ return GGameInstance; }
IGame& IGame::Get() { return *GGameInstance; }

Game::Game()
{
	assert( GGameInstance == nullptr);
	GGameInstance = this;

	mFPS = 0;
	mStageAdd = nullptr;

	mMouseState = 0;
}

IGame::~IGame()
{

}

bool Game::init( char const* pathConfig , Vec2i const& screenSize)
{
	 // This falls under the config ***
	int width=800;
	int height=600;

	if( screenSize.x != 0 && screenSize.y != 0 )
	{
		width = screenSize.x;
		height = screenSize.y;
	}

	mScreenSize.x = width;
	mScreenSize.y = height;

	QA_LOG("----QuadAssault----");
	QA_LOG("*******************");

	IFont* font = IFont::LoadFile( DATA_DIR"Fragile Bombers.TTF" );
	if (font == nullptr)
	{
		font = IFont::LoadFile("Bitwise");
	}
	//IFont* font = IFont::LoadFile("Bitwise");
	//IFont* font = NULL;
	if (font)
	{
		mFonts.push_back(font);
	}

	mSoundMgr.reset( new SoundManager );
	if (!mSoundMgr->initialize())
		return false;

	
	mNeedEnd=false;
	srand(time(NULL));

	std::fill_n(fpsSamples, NUM_FPS_SAMPLES, 0);

	QA_LOG("Game Init!");
	return true;
}

bool Game::initRenderSystem()
{
	QA_LOG("Build Render System...");
	mRenderSystem.reset(new RenderSystem);
	if (!mRenderSystem->init())
	{
		return false;
	}
	mRenderSystem->mGraphics2D = mGraphics2D;

	QA_LOG("Initialize Render Engine...");
	mRenderEngine.reset(new RenderEngine);
	VERIFY_RETURN_FALSE(mRenderEngine->init(mScreenSize.x, mScreenSize.y));

	return true;
}

void Game::exit()
{
	for(int i= mStageStack.size() - 1 ; i > 0 ; --i )
	{
		mStageStack[i]->onExit();
		delete mStageStack[i];
	}
	mStageStack.clear();

	for( int i = 0 ; i < mFonts.size() ; ++i )
	{
		delete mFonts[i];
	}
	mFonts.clear();	
	
	mSoundMgr->cleanup();
	mRenderEngine->cleanup();
	mRenderSystem->cleanup();

	QA_LOG( "Game End !!" );
	QA_LOG( "*******************" );

}

void Game::tick(float deltaT)
{
	while( mStageAdd )
	{
		if( mbRemovePrevStage )
		{
			mStageStack.back()->onExit();
			delete mStageStack.back();
			mStageStack.pop_back();
			QA_LOG("Old stage deleted.");
		}

		GameStage* stage = mStageAdd;
		mStageAdd = NULL;
		mStageStack.push_back(stage);
		QA_LOG("Setup new state...");
		if( !stage->onInit() )
		{
			QA_LOG("Stage Can't Init !" );
		}
		QA_LOG("Stage Init !");
	}

	mRunningStage = mStageStack.back();

	mSoundMgr->update(deltaT);
	mRunningStage->onUpdate(deltaT);

	if( mRunningStage->needStop() )
	{
		mStageStack.pop_back();
		mRunningStage->onExit();
		delete mRunningStage;
		mRunningStage = nullptr;
		QA_LOG("Stage Exit !");
	}
	if( mStageStack.empty() )
		mNeedEnd = true;
}


void Game::render()
{
	if( mNeedEnd )
		return;

	if ( !mRenderSystem->prevRender() )
		return;

	if ( mRunningStage )
		mRunningStage->onRender();

	++frameCount;
	if( frameCount > NumFramePerSample )
	{
		int64 temp = Platform::GetTickCount();
		fpsSamples[idxSample] = 1000.0f * (frameCount) / (temp - timeFrame);
		timeFrame = temp;
		frameCount = 0;

		++idxSample;
		if( idxSample == NUM_FPS_SAMPLES )
			idxSample = 0;

		mFPS = 0;
		for( int i = 0; i < NUM_FPS_SAMPLES; i++ )
			mFPS += fpsSamples[i];

		mFPS /= NUM_FPS_SAMPLES;
#if 0
		std::cout << "FPS =" << mFPS << std::endl;
#endif
	}


	mGraphics2D->beginRender();

	InlineString< 256 > str;
	str.format("FPS = %.2f", mFPS);

	IFont* font = getFont(0);
	if(font)
	{
		mRenderSystem->drawText(
			font, 18, str,
			Vec2i(getGame()->getScreenSize().x - 100, 30), Color4ub(255, 255, 25),
			TEXT_SIDE_LEFT | TEXT_SIDE_TOP);
	}

	mGraphics2D->endRender();

	mRenderSystem->postRender();

}

void Game::run()
{
	int64 prevTime = Platform::GetTickCount();
	
	timeFrame = Platform::GetTickCount();
	frameCount = 0;
	std::fill_n( fpsSamples , NUM_FPS_SAMPLES , 60.0f );

	while( !mNeedEnd )
	{
		int64 curTime = Platform::GetTickCount();
		float deltaT = ( curTime - prevTime ) / 1000.0f;

		tick(deltaT);

		prevTime = curTime;
	}

}

void Game::addStage( GameStage* stage, bool removePrev )
{	
	if ( mStageAdd )
	{
		delete stage;
		QA_ERROR("Add Stage Error");
		return;
	}
	if ( mStageAdd )
		delete mStageAdd;

	mStageAdd = stage;
	mbRemovePrevStage = removePrev;

}

void Game::procWidgetEvent( int event , int id , QWidget* sender )
{
	mStageStack.back()->onWidgetEvent( event , id , sender );
}

IFont* Game::getFont(int idx)
{
	if (!mFonts.isValidIndex(idx))
		return nullptr;

	return mFonts[idx];
}

MsgReply Game::onMouse( MouseMsg const& msg )
{
	mMousePos = msg.getPos();

	MsgReply reply = GUISystem::Get().mManager.procMouseMsg(msg);
	if ( reply.isHandled() )
	{
		if ( !msg.onMoving() )
			return MsgReply::Unhandled();
	}
	if (!mStageStack.empty())
	{
		mStageStack.back()->onMouse(msg);
	}

	return MsgReply::Unhandled();
}

MsgReply Game::onKey(KeyMsg const& msg)
{
	MsgReply reply = GUISystem::Get().mManager.procKeyMsg(msg);
	if ( reply.isHandled() )
		return reply;
	if (!mStageStack.empty())
	{
		return mStageStack.back()->onKey(msg);
	}
	return MsgReply::Unhandled();
}

MsgReply Game::onChar( unsigned code )
{
	return GUISystem::Get().mManager.procCharMsg(code);
}

bool Game::onSystemEvent( SystemEvent event )
{
	switch( event )
	{
	case SYS_WINDOW_CLOSE:
		getGame()->stopPlay();
		return false;
	}

	return true;

}

