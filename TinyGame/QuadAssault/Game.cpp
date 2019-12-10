#include "Game.h"

#include "GUISystem.h"
#include "TextureManager.h"
#include "SoundManager.h"
#include "RenderEngine.h"
#include "RenderSystem.h"

#include "DataPath.h"
#include "Platform.h"

#include "FixString.h"

#include <cassert>
#include <ctime>
#include <iostream>


static Game* gGame = NULL;
IGame* getGame(){ return gGame; }

IGame::~IGame()
{

}

Game::Game()
{
	assert( gGame == NULL );
	gGame = this;

	mFPS = 0;
	mStageAdd = NULL;

	mMouseState = 0;
}

bool Game::init( char const* pathConfig , Vec2i const& screenSize , bool bCreateWindow )
{
	 // This falls under the config ***
	int width=800;
	int height=600;
	//width=1024;
	//height=800;

	if( screenSize.x != 0 && screenSize.y != 0 )
	{
		width = screenSize.x;
		height = screenSize.y;
	}

	mScreenSize.x = width;
	mScreenSize.y = height;

	QA_LOG("----QuadAssault----");
	QA_LOG("*******************");

	if( bCreateWindow )
	{
		QA_LOG("Setting Window...");
	
		char const* tile = "QuadAssault";

		mWindow.reset(Platform::CreateWindow(tile, mScreenSize, 32, false));
		if( !mWindow )
		{
			QA_ERROR("ERROR: Can't create window !");
			return false;
		}

		mWindow->setSystemListener(*this);
		mWindow->showCursor(false);
	}

	QA_LOG("Build Render System...");
	mRenderSystem.reset( new RenderSystem );
	if ( !mRenderSystem->init( mWindow ) )
	{
		return false;
	}

	IFont* font = IFont::loadFont( DATA_DIR"DialogueFont.TTF" );
	//IFont* font = NULL;
	if ( font )
		mFonts.push_back( font );

	mSoundMgr.reset( new SoundManager );
	mRenderEngine.reset( new RenderEngine );

	QA_LOG("Initialize Render Engine...");
	VERIFY_RETURN_FALSE(mRenderEngine->init(width, height));
		
	mNeedEnd=false;
	srand(time(NULL));
		
	QA_LOG("Setting OpenGL...");
	glViewport(0,0,(GLsizei)width,(GLsizei)height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0,width,height,0,0,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();	
	glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT );
	glDisable( GL_DEPTH_TEST );
	//glDisable( GL_LIGHTING );	
	

	QA_LOG("Game Init!");
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

	mWindow.clear();

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

	if ( text )
	{
		FixString< 256 > str;
		str.format("FPS = %f", mFPS);
		text->setString(str.c_str());

		mRenderSystem->drawText(text,
								Vec2i(getGame()->getScreenSize().x - 100, 10),
								TEXT_SIDE_LEFT | TEXT_SIDE_TOP);
	}

	mRenderSystem->postRender();

}

void Game::run()
{
	int64 prevTime = Platform::GetTickCount();
	
	timeFrame = Platform::GetTickCount();
	frameCount = 0;
	text = IText::create( mFonts[0] , 18 , Color4ub(255,255,25) );

	std::fill_n( fpsSamples , NUM_FPS_SAMPLES , 60.0f );

	while( !mNeedEnd )
	{
		int64 curTime = Platform::GetTickCount();
		float deltaT = ( curTime - prevTime ) / 1000.0f;

		tick(deltaT);

		prevTime = curTime;
	}

	text->release();
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

void Game::procSystemEvent()
{
	if ( mWindow )
		mWindow->procSystemMessage();
}

bool Game::onMouse( MouseMsg const& msg )
{
	mMousePos = msg.getPos();

	if ( !GUISystem::Get().mManager.procMouseMsg( msg ) )
	{
		if ( !msg.onMoving() )
			return true;
	}
	mStageStack.back()->onMouse( msg );
	return true;
}

bool Game::onKey(KeyMsg const& msg)
{
	if ( !GUISystem::Get().mManager.procKeyMsg( msg ) )
		return true;

	mStageStack.back()->onKey( msg );
	return true;
}

bool Game::onChar( unsigned code )
{
	if ( !GUISystem::Get().mManager.procCharMsg( code ) )
		return true;

	return true;
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

