#include "ProjectPCH.h"
#include "LevelEditor.h"

#include "CSceneLevel.h"

#include "CFWorld.h"
#include "CFCamera.h"
#include "CFViewport.h"
#include "CFScene.h"


#include "CSceneCGF.h"

LevelEditor::LevelEditor( LevelManager* levelManager ) 
	:mLevelManager( levelManager )
{
	mCurCamera = NULL;

}

bool LevelEditor::init( SSystemInitParams& params )
{
	return true;
}

ICGFBase* LevelEditor::createCGF( CGFType type , CGFCallBack const& setupCB )
{
	ICGFBase* result = NULL;
	switch( type )
	{
	case CGF_SCENE: result = new CSceneCGF; break;
	}

	mHelper.isLoading = false;

	if ( result )
	{
		setupCB( result );

		
		if ( !result->construct( mHelper ) )
		{
			delete result;
			return NULL;
		}

		mCGFContent->add( result );
	}

	return result;
}

bool LevelEditor::createEmptyLevel( char const* name )
{
	mLevelManager->destroyLevel( name );
	CSceneLevel* level = mLevelManager->createEmptyLevel( name );
	if ( level == NULL )
		return false;

	mHelper.level = level;

	mLevelManager->changeRunningLevel( name );
	mCGFContent = &level->getCGFContent();

	if ( mCurCamera == NULL )
	{
		mCurCamera = level->getRenderScene()->createCamera();
	}
	else
	{
		mCurCamera->changeScene( level->getRenderScene() );
	}

	return true;
}

void LevelEditor::render3D( SRenderParams& params )
{
	CSceneLevel* level = mLevelManager->getRunningLevel();

	if ( level == NULL )
		return;

	CFScene* scene = level->getRenderScene();

	int w, h;
	params.viewport->getSize( &w , &h );

	mCurCamera->setAspect( float(w)/ h );
	scene->render( mCurCamera , params.viewport );

}

bool LevelEditor::save( char const* path )
{


	return true;
}
