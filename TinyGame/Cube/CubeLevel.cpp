#include "CubePCH.h"
#include "CubeLevel.h"

#include "CubeBlock.h"
#include "CubeWorld.h"

namespace Cube
{

	void Level::init()
	{
		Block::initList();
	}

	Level::Level()
	{
		mWorld = NULL;
	}

	Level::~Level()
	{
		delete mWorld;
	}

	void Level::setupWorld()
	{
		if ( mWorld )
			delete mWorld;

		mWorld = new World;
	}

}//namespace Cube