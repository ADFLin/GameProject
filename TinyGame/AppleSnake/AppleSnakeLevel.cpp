#include "AppleSnakeLevel.h"

namespace AppleSnake
{
	uint8 WorldData::getTile(Vec2i const& pos, bool bCheckEntity /*= true*/)
	{
		if (bCheckEntity)
		{
			for (auto const& e : mEntities)
			{
				if (e.pos == pos)
					return e.id;
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

	uint8 WorldData::getThing(Vec2i const& pos)
	{
		uint8 tileId = getTile(pos);
		if (tileId == ETile::None || tileId == ETile::Portal)
		{
			if (getSnakeBody(pos) != INDEX_NONE)
				return ETile::SnakeBody;
		}
		return tileId;
	}

	int WorldData::getSnakeBody(Vec2i const& pos)
	{
		for (auto bodyIt = mSnake.createIterator(); bodyIt.haveMore(); bodyIt.goNext())
		{
			auto const& body = bodyIt.getElement();
			if (body.pos == pos)
				return bodyIt.getIndex();
		}

		return INDEX_NONE;
	}

	bool WorldData::checkSnakeFall()
	{
		auto bodyIter = mSnake.createIterator();
		for (; bodyIter.haveMore(); bodyIter.goNext())
		{
			auto const& body = bodyIter.getElement();
			Vec2i pos = body.pos + GetDirOffset(GravityDir);
			auto tileId = getThing(pos);
			if (!ETile::CanFallIn(tileId))
				return  false;
		}

		return true;
	}

	int WorldData::checkPortalMove(Vec2i pos, DirType dir, Vec2i& outPos, DirType& outDir)
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