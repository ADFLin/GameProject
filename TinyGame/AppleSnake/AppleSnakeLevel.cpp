#include "AppleSnakeLevel.h"

namespace AppleSnake
{
	int GetSnakeBody(Snake& snake, Vec2i const& pos)
	{
		for (auto bodyIt = snake.createIterator(); bodyIt.haveMore(); bodyIt.goNext())
		{
			auto const& body = bodyIt.getElement();
			if (body.pos == pos)
				return bodyIt.getIndex();
		}

		return INDEX_NONE;
	}

	int World::getEntityIndex(Vec2i const& pos)
	{
		int index = mEntities.findIndexPred([&](auto const& e) { return e.pos == pos; });
		if (index == INDEX_NONE)
		{
			for (auto& snake : mDeadSnakes)
			{
				auto& e = mEntities[snake.entityIndex];
				Vec2i testPos = pos - e.pos;
				if (GetSnakeBody(snake, testPos) != INDEX_NONE)
					return snake.entityIndex;
			}
		}
		return index;
	}

	bool World::checkMoveTo(Entity& entity, DirType dir, bool bSnakeMove)
	{
		if (entity.id == ETile::SnakeDead)
		{
			auto CheckPos = [&](Vec2i const& testPos, bool bSnakeMove)
			{
				if (!mMap.checkRange(testPos))
					return true;

				int entityIndex = getEntityIndex(testPos);

				uint8 landId;
				if (entityIndex != INDEX_NONE)
				{
					auto& e = mEntities[entityIndex];
					if (&e == &entity)
						return true;

					landId = e.id;
				}
				else
				{
					landId = getThing(testPos, false);
				}

				if (landId == ETile::SnakeBody)
				{
					return false;
				}
				else if (ETile::CanEnityMoveTo(landId))
					return true;

				return false;
			};

			DeadSnake& snake = mDeadSnakes[entity.meta];
			for (auto bodyIt = snake.createIterator(); bodyIt.haveMore(); bodyIt.goNext())
			{
				auto const& body = bodyIt.getElement();
				Vec2i testPos = entity.pos + body.pos + GetDirOffset(dir);

				if (!CheckPos(testPos, false))
					return false;
			}
			return true;
		}

		Vec2i testPos;
		DirType testDir;
		int portalIndex = checkPortalMove(entity.pos, dir, testPos, testDir);
		if (portalIndex == INDEX_NONE)
		{
			testPos = entity.pos + GetDirOffset(dir);
		}

		auto CheckPos = [&](Vec2i const& testPos, bool bSnakeMove)
		{
			if (!mMap.checkRange(testPos))
				return true;

			uint8 landId = getThing(testPos);

			if (landId == ETile::SnakeBody)
			{
				return bSnakeMove;
			}
			else if (ETile::CanEnityMoveTo(landId))
				return true;

			return false;
		};

		return CheckPos(testPos, bSnakeMove);
	}

	uint8 World::getTile(Vec2i const& pos, bool bCheckEntity /*= true*/)
	{
		if (bCheckEntity)
		{
			int entityId = getEntityIndex(pos);
			if (entityId != INDEX_NONE)
			{
				return mEntities[entityId].id;
			}
		}

		if (mMap.checkRange(pos))
		{
			auto tileId = mMap(pos);
			if (tileId)
				return tileId;
		}
		return ETile::None;
	}



	uint8 World::getThing(Vec2i const& pos, bool bCheckEntity)
	{
		uint8 tileId = getTile(pos, bCheckEntity);
		if (tileId == ETile::None || tileId == ETile::Portal)
		{
			if (getSnakeBody(pos) != INDEX_NONE)
				return ETile::SnakeBody;

		}
		return tileId;
	}

	int World::getSnakeBody(Vec2i const& pos)
	{
		return GetSnakeBody(mSnake, pos);
	}

	bool World::checkSnakeFall()
	{
		for (auto bodyIt = mSnake.createIterator(); bodyIt.haveMore(); bodyIt.goNext())
		{
			auto const& body = bodyIt.getElement();

			auto bodyTileId = getTile(body.pos, false);
			if (bodyTileId == ETile::Cobweb)
				return false;

			Vec2i pos = body.pos + GetDirOffset(GravityDir);
			auto tileId = getThing(pos);
			if (!ETile::CanFallIn(tileId))
				return  false;
		}

		return true;
	}

	int World::checkPortalMove(Vec2i pos, DirType dir, Vec2i& outPos, DirType& outDir)
	{
		uint8 tileId = getTile(pos, false);
		if (tileId != ETile::Portal)
			return INDEX_NONE;

		uint8 portalId = mMap(pos).meta;
		auto const& portal = mPortals[portalId];
		if (portal.dir != dir || portal.link < 0)
			return INDEX_NONE;

		auto const& portalLink = mPortals[portal.link];
		outPos = portalLink.pos;
		outDir = InverseDir(portalLink.dir);
		return portalId;
	}

}//namespace AppleSnake