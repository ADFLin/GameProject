#ifndef LmLevel_h__
#define LmLevel_h__

#include "DataStructure/Grid2D.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <limits>
#include <vector>

#ifndef INDEX_NONE
#define INDEX_NONE -1
#endif

namespace LordMonarch
{
	static int const NumKingdoms = 4;

	enum TerrainID
	{
		TID_DIRT = 0,

		TID_VILLAGE,
		TID_TERRITORY,
		TID_GRASS,
		TID_CAVE,
		TID_CAVE_BLOCKED,
		TID_BRIDGE,

		TID_FOREST,
		TID_RIVER,
		TID_HILL,
	};

	inline bool isBlocked(TerrainID id)
	{
		switch (id)
		{
		case TID_CAVE_BLOCKED:
		case TID_RIVER:
		case TID_FOREST:
		case TID_HILL:
			return true;
		default:
			return false;
		}
	}

	struct Tile
	{
		int id = TID_DIRT;
		int meta = INDEX_NONE;   // owner kingdom for land/buildings
		int unitId = INDEX_NONE; // village/leader/army occupying this tile
	};

	struct UnitData
	{
		int   score = 0;
		int   owner = INDEX_NONE;
		Vec2i pos = Vec2i(0, 0);
		int   next = INDEX_NONE;
	};

	struct Action
	{
		enum Id
		{
			eAuto,
			eStandBy,
			eDefend,
			eDestroy,
			eBuildVillage,
			eBuildBarricade,
			eBuildBridge,
			eClearLand,
			eClearPath,
			eDismantle,
			eStealCave,
			eMove,
			eAttack,
		};

		Id    id = eAuto;
		Vec2i posDest = Vec2i(0, 0);
	};

	struct ActorData : public UnitData
	{
		enum Type
		{
			eArmy,
			eLeader,
			eVillage,
		};

		Type type = eArmy;
		Action::Id comAction = Action::eAuto;
		Action::Id curAction = Action::eAuto;
		Vec2i posDest = Vec2i(0, 0);
		int moveCooldown = 0;
		bool bAlive = true;
	};

	class Unit
	{
	public:
		virtual ~Unit() {}
		virtual void tick() = 0;
	};

	class IPath
	{
	public:
		virtual ~IPath() {}
		virtual int getNodeNum() const = 0;
		virtual Vec2i getNode(int index) const = 0;
	};

	class SimplePath : public IPath
	{
	public:
		int getNodeNum() const override { return int(nodes.size()); }
		Vec2i getNode(int index) const override { return nodes[index]; }

		std::vector<Vec2i> nodes;
	};

	class IPathFinder
	{
	public:
		virtual ~IPathFinder() {}
		virtual IPath* findPath(Vec2i const& start, Vec2i const& goal) = 0;
		virtual IPath* findTile(Vec2i const& pos, TerrainID id, int maxDist) = 0;
	};

	class Kingdom
	{
	public:
		struct Status
		{
			int countTerritory = 0;
			int countVillege = 0;
			int totalPoplution = 0;
			int momey = 0;
		};

		void init(int id)
		{
			mId = id;
			mStatus = Status();
			mTaxRate = 10;
			mLeaderId = INDEX_NONE;
			mStatus.momey = 80;
		}

		Status& getStatus() { return mStatus; }
		Status const& getStatus() const { return mStatus; }

		void setTaxRate(int rate) { mTaxRate = Math::Clamp(rate, 0, 30); }
		int  getTaxRate() const { return mTaxRate; }

		int getBaseDecRate(int population, int numTerritory) const
		{
			int crowding = std::max(0, population - 24 * (numTerritory + 1));
			return mTaxRate / 6 + crowding / 16;
		}

		int getId() const { return mId; }
		int getLeaderId() const { return mLeaderId; }
		void setLeaderId(int id) { mLeaderId = id; }

		void updateEconomy()
		{
			mStatus.momey += std::max(1, mTaxRate * (mStatus.countTerritory + 2 * mStatus.countVillege) / 20);
		}

		void addTerritory(Tile& tile)
		{
			tile.id = TID_TERRITORY;
			tile.meta = getId();
			++mStatus.countTerritory;
		}

		void removeTerritory(Tile& tile)
		{
			if (tile.id == TID_TERRITORY && tile.meta == getId())
			{
				tile.id = TID_DIRT;
				tile.meta = INDEX_NONE;
				--mStatus.countTerritory;
			}
		}

		void addVillage(Tile& tile)
		{
			tile.id = TID_VILLAGE;
			tile.meta = getId();
			++mStatus.countVillege;
		}

		void removeVillage(Tile& tile)
		{
			if (tile.id == TID_VILLAGE && tile.meta == getId())
			{
				tile.id = TID_DIRT;
				tile.meta = INDEX_NONE;
				tile.unitId = INDEX_NONE;
				--mStatus.countVillege;
			}
		}

		int    mId = INDEX_NONE;
		Status mStatus;
		int    mTaxRate = 10;
		int    mLeaderId = INDEX_NONE;
	};

	inline Vec2i const& linkOffset(int dir)
	{
		static Vec2i const offset[] =
		{
			Vec2i(1, 0), Vec2i(1, 1), Vec2i(0, 1), Vec2i(-1, 1),
			Vec2i(-1, 0), Vec2i(-1, -1), Vec2i(0, -1), Vec2i(1, -1),
		};
		return offset[dir];
	}

	class Level
	{
	public:
		void restart(Vec2i const& size = Vec2i(32, 22))
		{
			mMap.resize(size.x, size.y);
			mMap.fillValue(Tile());
			mUnitData.clear();
			mDay = 0;
			mTickCount = 0;
			mWinner = INDEX_NONE;

			for (int i = 0; i < NumKingdoms; ++i)
				mPlayers[i].init(i);

			generateDefaultMap();

			createVillage(Vec2i(3, 3), 0, 72);
			createVillage(Vec2i(size.x - 4, 3), 1, 72);
			createVillage(Vec2i(3, size.y - 4), 2, 72);
			createVillage(Vec2i(size.x - 4, size.y - 4), 3, 72);

			createLeader(Vec2i(4, 4), 0);
			createLeader(Vec2i(size.x - 5, 4), 1);
			createLeader(Vec2i(4, size.y - 5), 2);
			createLeader(Vec2i(size.x - 5, size.y - 5), 3);
		}

		bool isValidPos(Vec2i const& pos) const { return mMap.checkRange(pos); }
		Vec2i getMapSize() const { return mMap.getSize(); }

		Tile& getTile(Vec2i const& pos) { return mMap.getData(pos.x, pos.y); }
		Tile const& getTile(Vec2i const& pos) const { return mMap.getData(pos.x, pos.y); }

		Kingdom& getPlayer(int id) { return mPlayers[id]; }
		Kingdom const& getPlayer(int id) const { return mPlayers[id]; }

		UnitData& getUnitData(int id) { return mUnitData[id]; }
		ActorData& getActorData(int id) { return mUnitData[id]; }
		ActorData const& getActorData(int id) const { return mUnitData[id]; }

		std::vector<ActorData> const& getActors() const { return mUnitData; }
		int getDay() const { return mDay; }
		int getWinner() const { return mWinner; }
		bool isGameEnd() const { return mWinner != INDEX_NONE || mDay >= 3000; }

		bool isAllies(int id1, int id2) const { return id1 == id2; }

		bool canEnter(Vec2i const& pos) const
		{
			return isValidPos(pos) && !isBlocked(TerrainID(getTile(pos).id));
		}

		bool canBuildVillage(Vec2i const& pos) const
		{
			if (!isValidPos(pos) || getTile(pos).id != TID_DIRT)
				return false;

			for (int i = 0; i < 8; ++i)
			{
				Vec2i linkPos = pos + linkOffset(i);
				if (isValidPos(linkPos) && getTile(linkPos).id == TID_VILLAGE)
					return false;
			}
			return true;
		}

		bool commandActor(int actorId, Action::Id action, Vec2i const& dest)
		{
			if (actorId < 0 || actorId >= int(mUnitData.size()))
				return false;

			ActorData& actor = mUnitData[actorId];
			if (!actor.bAlive || actor.type == ActorData::eVillage || !isValidPos(dest))
				return false;

			actor.comAction = action;
			actor.curAction = action;
			actor.posDest = dest;
			return true;
		}

		void update()
		{
			if (isGameEnd())
				return;

			++mTickCount;
			if ((mTickCount % 12) == 0)
			{
				++mDay;
				for (int i = 0; i < NumKingdoms; ++i)
					mPlayers[i].updateEconomy();
			}

			for (int id = 0; id < int(mUnitData.size()); ++id)
			{
				if (mUnitData[id].bAlive && mUnitData[id].type == ActorData::eVillage)
					updateVillage(id);
			}

			for (int id = 0; id < int(mUnitData.size()); ++id)
			{
				if (mUnitData[id].bAlive && mUnitData[id].type != ActorData::eVillage)
					updateActor(id);
			}

			checkWinner();
		}

		TGrid2D<Tile> mMap;
		Kingdom mPlayers[NumKingdoms];

	private:
		void generateDefaultMap()
		{
			Vec2i size = mMap.getSize();
			for (int y = 0; y < size.y; ++y)
			{
				for (int x = 0; x < size.x; ++x)
				{
					Tile& tile = mMap.getData(x, y);
					if (x == 0 || y == 0 || x == size.x - 1 || y == size.y - 1)
					{
						tile.id = TID_HILL;
					}
					else if (x == size.x / 2 && y > 2 && y < size.y - 3 && y != size.y / 2)
					{
						tile.id = TID_RIVER;
					}
					else if ((x + y * 3) % 17 == 0 && x > 4 && y > 4 && x < size.x - 5 && y < size.y - 5)
					{
						tile.id = TID_FOREST;
					}
				}
			}

			getTile(Vec2i(size.x / 2, size.y / 2)).id = TID_BRIDGE;
		}

		int createActor(Vec2i const& pos, int owner, int score, ActorData::Type type)
		{
			ActorData actor;
			actor.owner = owner;
			actor.pos = pos;
			actor.posDest = pos;
			actor.score = score;
			actor.type = type;
			actor.bAlive = true;

			int id = int(mUnitData.size());
			mUnitData.push_back(actor);

			if (type != ActorData::eVillage && isValidPos(pos))
				getTile(pos).unitId = id;

			return id;
		}

		int createLeader(Vec2i const& pos, int owner)
		{
			int id = createActor(pos, owner, 160, ActorData::eLeader);
			mPlayers[owner].setLeaderId(id);
			claimAround(pos, owner);
			return id;
		}

		int createVillage(Vec2i const& pos, int owner, int score)
		{
			int id = createActor(pos, owner, score, ActorData::eVillage);
			Tile& tile = getTile(pos);
			mPlayers[owner].addVillage(tile);
			tile.unitId = id;
			claimAround(pos, owner);
			return id;
		}

		void claimAround(Vec2i const& pos, int owner)
		{
			claimTile(pos, owner);
			for (int i = 0; i < 8; ++i)
				claimTile(pos + linkOffset(i), owner);
		}

		bool claimTile(Vec2i const& pos, int owner)
		{
			if (!isValidPos(pos))
				return false;

			Tile& tile = getTile(pos);
			if (isBlocked(TerrainID(tile.id)) || tile.id == TID_VILLAGE)
				return false;

			if (tile.id == TID_TERRITORY && tile.meta == owner)
				return true;

			if (tile.id == TID_TERRITORY && tile.meta != INDEX_NONE)
				mPlayers[tile.meta].removeTerritory(tile);

			mPlayers[owner].addTerritory(tile);
			return true;
		}

		bool produceUnit(Vec2i const& pos, int owner, int power)
		{
			for (int i = 0; i < 8; ++i)
			{
				Vec2i target = pos + linkOffset(i);
				if (canEnter(target) && getTile(target).unitId == INDEX_NONE)
				{
					createActor(target, owner, std::max(24, power), ActorData::eArmy);
					return true;
				}
			}
			return false;
		}

		void updateVillage(int villageId)
		{
			ActorData& village = mUnitData[villageId];
			Kingdom& player = getPlayer(village.owner);

			int numTerritory = 0;
			int tileId[8];
			for (int i = 0; i < 8; ++i)
			{
				Vec2i pos = village.pos + linkOffset(i);
				tileId[i] = isValidPos(pos) ? getTile(pos).id : TID_HILL;
				if (isValidPos(pos) && getTile(pos).id == TID_TERRITORY && getTile(pos).meta == village.owner)
					++numTerritory;
			}

			village.score += (numTerritory + 1) - player.getBaseDecRate(village.score, numTerritory);
			village.score = Math::Clamp(village.score, 0, 255);

			int const TerritoryDirMap[] = { 4, 0, 6, 2, 5, 3, 7, 1 };
			if (village.score > 16 * (numTerritory + 1))
			{
				int idx = 0;
				for (; idx < 8; ++idx)
				{
					int dir = TerritoryDirMap[idx];
					Vec2i pos = village.pos + linkOffset(dir);
					if (isValidPos(pos) && tileId[dir] == TID_DIRT)
						break;
				}

				if (idx != 8)
				{
					Vec2i pos = village.pos + linkOffset(TerritoryDirMap[idx]);
					if (claimTile(pos, village.owner))
						village.score -= 8;
				}
				else if (village.score >= 96 && produceUnit(village.pos, village.owner, village.score / 2))
				{
					village.score /= 2;
				}
			}
		}

		void updateActor(int actorId)
		{
			ActorData& actor = mUnitData[actorId];
			if (actor.moveCooldown > 0)
			{
				--actor.moveCooldown;
				return;
			}

			if (actor.curAction == Action::eStandBy)
				return;

			if (tryCompleteBuildAction(actorId))
				return;

			if (actor.curAction == Action::eAuto || actor.pos == actor.posDest)
				chooseAutoTarget(actor);

			Vec2i nextPos = chooseStep(actor.pos, actor.posDest, actor.owner);
			if (nextPos == actor.pos)
				return;

			moveActor(actorId, nextPos);
			actor.moveCooldown = (actor.type == ActorData::eLeader) ? 2 : 3;
		}

		void chooseAutoTarget(ActorData& actor)
		{
			Vec2i target;
			if (findNearestEnemyAsset(actor.owner, actor.pos, target))
			{
				actor.curAction = Action::eAttack;
				actor.posDest = target;
				return;
			}

			if (findNearestClaimable(actor.owner, actor.pos, target))
			{
				actor.curAction = Action::eAuto;
				actor.posDest = target;
			}
		}

		bool findNearestEnemyAsset(int owner, Vec2i const& from, Vec2i& outPos) const
		{
			int bestDist = std::numeric_limits<int>::max();
			bool bFound = false;

			for (ActorData const& actor : mUnitData)
			{
				if (!actor.bAlive || actor.owner == owner)
					continue;

				if (actor.type != ActorData::eVillage && actor.type != ActorData::eLeader)
					continue;

				int dist = TaxicabDistance(from, actor.pos);
				if (dist < bestDist)
				{
					bestDist = dist;
					outPos = actor.pos;
					bFound = true;
				}
			}

			return bFound;
		}

		bool findNearestClaimable(int owner, Vec2i const& from, Vec2i& outPos) const
		{
			int bestDist = std::numeric_limits<int>::max();
			bool bFound = false;

			for (int y = 0; y < mMap.getSizeY(); ++y)
			{
				for (int x = 0; x < mMap.getSizeX(); ++x)
				{
					Vec2i pos(x, y);
					Tile const& tile = getTile(pos);
					if (isBlocked(TerrainID(tile.id)))
						continue;

					if (tile.id == TID_DIRT || (tile.id == TID_TERRITORY && tile.meta != owner))
					{
						int dist = TaxicabDistance(from, pos);
						if (dist < bestDist)
						{
							bestDist = dist;
							outPos = pos;
							bFound = true;
						}
					}
				}
			}

			return bFound;
		}

		Vec2i chooseStep(Vec2i const& from, Vec2i const& to, int owner) const
		{
			Vec2i best = from;
			int bestScore = TaxicabDistance(from, to);

			for (int i = 0; i < 8; ++i)
			{
				Vec2i pos = from + linkOffset(i);
				if (!canEnter(pos))
					continue;

				Tile const& tile = getTile(pos);
				if (tile.unitId != INDEX_NONE && mUnitData[tile.unitId].owner == owner)
					continue;

				int score = TaxicabDistance(pos, to);
				if (tile.id == TID_TERRITORY && tile.meta == owner)
					score -= 1;

				if (score < bestScore)
				{
					bestScore = score;
					best = pos;
				}
			}

			return best;
		}

		bool tryCompleteBuildAction(int actorId)
		{
			ActorData& actor = mUnitData[actorId];
			Kingdom& kingdom = getPlayer(actor.owner);

			switch (actor.curAction)
			{
			case Action::eBuildVillage:
				if (actor.pos == actor.posDest && canBuildVillage(actor.pos) && kingdom.getStatus().momey >= 50)
				{
					kingdom.getStatus().momey -= 50;
					int score = std::max(48, actor.score);
					Tile& tile = getTile(actor.pos);
					if (tile.unitId == actorId)
						tile.unitId = INDEX_NONE;
					actor.bAlive = false;
					createVillage(actor.pos, actor.owner, score);
					return true;
				}
				break;
			case Action::eBuildBridge:
				if (TaxicabDistance(actor.pos, actor.posDest) <= 1 && isValidPos(actor.posDest))
				{
					Tile& tile = getTile(actor.posDest);
					if (tile.id == TID_RIVER && kingdom.getStatus().momey >= 30)
					{
						kingdom.getStatus().momey -= 30;
						tile.id = TID_BRIDGE;
						tile.meta = actor.owner;
						actor.curAction = Action::eAuto;
						return true;
					}
				}
				break;
			case Action::eClearLand:
			case Action::eClearPath:
				if (TaxicabDistance(actor.pos, actor.posDest) <= 1 && isValidPos(actor.posDest))
				{
					Tile& tile = getTile(actor.posDest);
					if ((tile.id == TID_FOREST || tile.id == TID_HILL || tile.id == TID_CAVE_BLOCKED) && kingdom.getStatus().momey >= 15)
					{
						kingdom.getStatus().momey -= 15;
						tile.id = TID_DIRT;
						tile.meta = INDEX_NONE;
						actor.curAction = Action::eAuto;
						return true;
					}
				}
				break;
			default:
				break;
			}

			return false;
		}

		void moveActor(int actorId, Vec2i const& pos)
		{
			ActorData& actor = mUnitData[actorId];
			Tile& fromTile = getTile(actor.pos);
			if (fromTile.unitId == actorId)
				fromTile.unitId = INDEX_NONE;

			Tile& toTile = getTile(pos);
			if (toTile.unitId != INDEX_NONE)
			{
				resolveCombat(actorId, toTile.unitId);
				if (!actor.bAlive)
					return;

				if (toTile.unitId != INDEX_NONE)
					return;
			}

			actor.pos = pos;
			toTile.unitId = actorId;
			onUnitMoveTile(actor);
		}

		void resolveCombat(int attackerId, int defenderId)
		{
			ActorData& attacker = mUnitData[attackerId];
			ActorData& defender = mUnitData[defenderId];
			if (attacker.owner == defender.owner)
				return;

			int attackPower = attacker.score;
			int defensePower = defender.score + ((defender.type == ActorData::eVillage) ? 24 : 0);

			if (attackPower > defensePower)
			{
				attacker.score = std::max(8, attackPower - defensePower / 2);
				killActor(defenderId);
			}
			else
			{
				defender.score = std::max(8, defensePower - attackPower / 2);
				killActor(attackerId);
			}
		}

		void killActor(int actorId)
		{
			ActorData& actor = mUnitData[actorId];
			if (!actor.bAlive)
				return;

			Tile& tile = getTile(actor.pos);
			if (tile.unitId == actorId)
				tile.unitId = INDEX_NONE;

			if (actor.type == ActorData::eVillage)
			{
				getPlayer(actor.owner).removeVillage(tile);
			}
			else if (actor.type == ActorData::eLeader)
			{
				getPlayer(actor.owner).setLeaderId(INDEX_NONE);
			}

			actor.bAlive = false;
		}

		bool onUnitMoveTile(ActorData& actor)
		{
			Tile& tile = getTile(actor.pos);
			switch (tile.id)
			{
			case TID_DIRT:
				claimTile(actor.pos, actor.owner);
				return true;
			case TID_TERRITORY:
				if (tile.meta != actor.owner && !isAllies(tile.meta, actor.owner))
				{
					if (tile.meta != INDEX_NONE)
						getPlayer(tile.meta).removeTerritory(tile);
					getPlayer(actor.owner).addTerritory(tile);
				}
				return true;
			default:
				return false;
			}
		}

		void checkWinner()
		{
			int winner = INDEX_NONE;
			for (int i = 0; i < NumKingdoms; ++i)
			{
				int leaderId = mPlayers[i].getLeaderId();
				if (leaderId != INDEX_NONE && mUnitData[leaderId].bAlive)
				{
					if (winner != INDEX_NONE)
						return;
					winner = i;
				}
			}
			mWinner = winner;
		}

		std::vector<ActorData> mUnitData;
		int mDay = 0;
		int mTickCount = 0;
		int mWinner = INDEX_NONE;
	};

}//namespace LordMonarch

#endif // LmLevel_h__
