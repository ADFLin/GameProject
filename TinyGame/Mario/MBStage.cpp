#include "MBStage.h"

#include "InputManager.h"

namespace Mario
{

	void TestStage::restart()
	{
		mWorld.init(20, 20);
		TileMap& terrain = mWorld.getTerrain();

		for( int i = 0; i < 20; ++i )
			terrain.getData(i, 0).block = BLOCK_STATIC;

		for( int i = 0; i < 10; ++i )
			terrain.getData(i + 6, 4).block = BLOCK_STATIC;

		terrain.getData(4 + 6, 4).block = BLOCK_NULL;

		for( int i = 0; i < 2; ++i )
			terrain.getData(i + 8, 8).block = BLOCK_STATIC;

		terrain.getData(10, 1).block = BLOCK_SLOPE_11;
		terrain.getData(10, 1).meta = 1;

		terrain.getData(13, 1).block = BLOCK_SLOPE_11;
		terrain.getData(13, 1).meta = 0;

		//terrain.getData( 9 , 12 ).block = BLOCK_STATIC;


		terrain.getData(13, 12).block = BLOCK_SLOPE_11;
		terrain.getData(13, 12).meta = 0;


		terrain.getData(10, 12).block = BLOCK_SLOPE_11;
		terrain.getData(10, 12).meta = 1;

		player.reset();
		player.world = &mWorld;
		player.pos = Vector2(200, 200);
	}

	void TestStage::tick()
	{
		InputManager& input = InputManager::Get();
		if( input.isKeyDown(EKeyCode::D) )
			player.button |= ACB_RIGHT;
		if( input.isKeyDown(EKeyCode::A) )
			player.button |= ACB_LEFT;
		if( input.isKeyDown(EKeyCode::W) )
			player.button |= ACB_UP;
		if( input.isKeyDown(EKeyCode::S) )
			player.button |= ACB_DOWN;
		if( input.isKeyDown(EKeyCode::K) )
			player.button |= ACB_JUMP;

		player.tick();
		mWorld.tick();
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();


		//Vector2 posCam = player.pos;

		TileMap& terrain = mWorld.getTerrain();

		RenderUtility::SetBrush(g, EColor::Yellow);
		RenderUtility::SetPen(g, EColor::Yellow);

		for( int i = 0; i < terrain.getSizeX(); ++i )
			for( int j = 0; j < terrain.getSizeY(); ++j )
			{
				Tile& tile = terrain.getData(i, j);

				if( tile.block == BLOCK_NULL )
					continue;

				switch( tile.block )
				{
				case BLOCK_SLOPE_11:
					if( BlockSlope::getDir(tile.meta) == 0 )
					{
						Vector2 v = tile.pos * TileLength;
						drawTriangle(g, v, v + Vector2(TileLength, 0), v + Vector2(0, TileLength));
					}
					else
					{
						Vector2 v = tile.pos * TileLength;
						drawTriangle(g, v, v + Vector2(TileLength, 0), v + Vector2(TileLength, TileLength));
					}
					break;
				case BLOCK_SLOPE_21:
					if( BlockSlope::getDir(tile.meta) == 0 )
					{
						Vector2 v = tile.pos * TileLength;
						drawTriangle(g, v, v + Vector2(TileLength, 0), v + Vector2(0, TileLength));
					}
					else
					{
						Vector2 v = tile.pos * TileLength;
						drawTriangle(g, v, v + Vector2(TileLength, 0), v + Vector2(TileLength, TileLength));
					}
				default:
					drawRect(g, tile.pos * TileLength, Vec2i(TileLength, TileLength));
				}
			}

		RenderUtility::SetBrush(g, EColor::Red);
		RenderUtility::SetPen(g, EColor::Red);
		drawRect(g, player.pos, player.size);
	}

}//namespace Mario

