#include "ProjectPCH.h"
#include "RenderSystem.h"

#include "CFWorld.h"
#include "CFViewport.h"

#include "CFPluginManager.h"
#include "Cw3FileLoader.h"

RenderSystem::RenderSystem()
{
	mScreenViewport = NULL;
	mCFWorld = NULL;
}

RenderSystem::~RenderSystem()
{
	mScreenViewport->release();
	mCFWorld->release();
}

bool RenderSystem::init( SSystemInitParams& params )
{
	if ( !CFly::initSystem() )
		return false;

	CFly::PluginManager::Get().registerLinker( "CW3" , new CFly::Cw3FileLinker );

	mCFWorld = CFly::createWorld( 
		params.hWnd , params.bufferWidth , params.bufferHeight , 32 , false );

	if ( mCFWorld == nullptr )
		return false;

	mScreenViewport = mCFWorld->createViewport( 0 , 0 , params.bufferWidth , params.bufferHeight );
	mScreenViewport->setBackgroundColor( 0 ,0, 0 );

	return true;
}

Material* RenderSystem::createEmptyMaterial()
{
	return mCFWorld->createMaterial();
}

RenderScene* RenderSystem::createRenderScene()
{
	return mCFWorld->createScene( 5 );
}
