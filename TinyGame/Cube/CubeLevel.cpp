#include "CubePCH.h"
#include "CubeLevel.h"

#include "CubeBlock.h"
#include "CubeWorld.h"

namespace Cube
{

	void Level::InitializeData()
	{
		Block::InitList();
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

	void Level::tick(float deltaTime)
	{
		mWorld->update(deltaTime);
	}

}//namespace Cube