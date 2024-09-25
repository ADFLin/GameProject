#include "AppleSnakeStage.h"


#include "ApplsSnakeLevelData.h"


namespace AppleSnake
{
	REGISTER_STAGE_ENTRY("Apple Snake", GameStage, EExecGroup::Dev);

	bool GameStage::loadMap(LevelData const& levelData)
	{
		int border = levelData.border;

		mMap.resize(levelData.size.x + 2 * border, levelData.size.y + 2 * border);
		mPortals.clear();
		mEntities.clear();
		Tile tile;
		tile.id = ETile::None;
		tile.meta = 0;
		mMap.fillValue(tile);

		int indexMeta = 0;
		auto ReadMeta = [&]()
		{
			if (indexMeta < levelData.meta.size())
			{
				auto meta = levelData.meta[indexMeta];
				++indexMeta;
				return meta;
			}
			return uint8(0);
		};

		auto ToDataIndex = [&](Vec2i const& pos)
		{
			return (pos.x - border) + levelData.size.x * (pos.y - border);
		};

		auto CheckDataRange = [&](Vec2i const& pos)
		{
			Vec2i dataPos = pos - Vec2i(border, border);
			return 0 <= dataPos.x && dataPos.x < levelData.size.x &&
				0 <= dataPos.y && dataPos.y < levelData.size.y &&
				ToDataIndex(pos) < levelData.data.size();
		};

		for (int i = 0; i < levelData.data.size(); ++i)
		{
			uint8 data = levelData.data[i];
			if (data == 0)
				continue;

			Vec2i pos = Vec2i(i % levelData.size.x, i / levelData.size.x) + Vec2i(border, border);

			if (ETile::IsEntity(data))
			{
				Entity e;
				e.id = data;
				e.pos = pos;
				mEntities.push_back(e);
				continue;
			}

			switch (data)
			{
			case ETile::Portal:
				{
					auto& tile = mMap(pos);
					tile.id = data;
					tile.meta = mPortals.size();

					auto meta = ReadMeta();
					Portal portal;
					portal.color = meta >> 4;
					portal.pos = pos;
					portal.dir = meta & 0xf;
					portal.link = INDEX_NONE;
					for (auto& other : mPortals)
					{
						if (other.color == portal.color)
						{
							portal.link = &other - mPortals.data();
							other.link = mPortals.size();
							break;
						}
					}
					mPortals.push_back(portal);
				}
				break;
			case ETile::SnakeHead:
				{
					Vec2i curPos = pos;
					int prevDir = INDEX_NONE;
					for (;;)
					{
						int dir = 0;
						for (; dir < 4; ++dir)
						{
							if (prevDir == dir)
								continue;

							Vec2i pos = curPos + GetDirOffset(dir);
							if (!CheckDataRange(pos))
								continue;

							if (levelData.data[ToDataIndex(pos)] == ETile::SnakeBody)
							{
								int dirInv = InverseDir(dir);
								if (prevDir == INDEX_NONE)
								{
									mSnake.reset(curPos, dirInv, 1);
								}
								mSnake.addTial(dirInv);
								prevDir = dirInv;
								curPos = pos;
								break;
							}
						}

						if (dir == 4)
							break;
					}

					for (int i = mSnake.getLength() - 2; i > 0; --i)
					{
						auto& body = mSnake.getBody(i);
						auto const& bodyNext = mSnake.getBody(i + 1);

						Vec2i offset = body.pos - bodyNext.pos;
						if (offset.x == 1)
						{
							body.dir = 0;
						}
						else if (offset.y == 1)
						{
							body.dir = 1;
						}
						else if (offset.x == -1)
						{
							body.dir = 2;
						}
						else //if (offset.y == -1)
						{
							body.dir = 3;
						}
					}
				}
				break;
			case ETile::SnakeBody:
				break;
			default:
				mMap(pos) = data;
			}
		}

		return true;
	}

	void GameStage::restart()
	{
		mIndexDraw = INDEX_NONE;
		mMoveOffset = 0;
		mFallOffset = 0;
		mBlockFallOffset = 0;
		mMoveEntityIndex = INDEX_NONE;
		mCPIndex = INDEX_NONE;
		mTweener.clear();
		Coroutines::Stop(mGameHandle);
		loadMap(LV_LOAD);

		Vec2i screenSize = ::Global::GetScreenSize();
		mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(mMap.getSize()), Vector2(0, -1), screenSize.y / float(10 + 2.5), false);
		mScreenToWorld = mWorldToScreen.inverse();
	}

}//namespace AppleSnake