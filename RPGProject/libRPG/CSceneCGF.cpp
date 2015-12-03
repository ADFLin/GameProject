#include "ProjectPCH.h"
#include "CSceneCGF.h"

#include "CSceneLevel.h"

#include "CFScene.h"
#include "CFWorld.h"

void CSceneCGF::onLoadObject( CFly::Object* object )
{
	UnitInfo info;
	info.obj      = object;

	unitList.push_back( info );
}

bool CSceneCGF::construct( CGFContructHelper const& helper )
{
	CFScene* scene = helper.level->getRenderScene();
	scene->getWorld()->setDir( CFly::DIR_SCENE , sceneDir.c_str() );

	if ( !scene->load( fileName.c_str() , NULL , helper.isLoading  ? NULL : this ) )
		return false;

	return true;
}


