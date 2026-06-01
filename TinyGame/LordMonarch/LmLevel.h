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

		TID_CASTLE,
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
			eCastle,
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
			int countCastle = 0;
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
			mCastleId = INDEX_NONE;
			mStatus.momey = 80;
		}

		Status& getStatus() { return mStatus; }
		Status const& getStatus() const { return mStatus; }

		void setTaxRate(int rate) { mTaxRate = Math::Clamp(rate, 0, 30); }
		int  getTaxRate() const { return mTaxRate; }

		int getBaseDecRate(int population, int numTerritory, int effectiveTaxRate) const
		{
			int crowding = std::max(0, population - 24 * (numTerritory + 1));
			return effectiveTaxRate / 6 + crowding / 16;
		}

		int getId() const { return mId; }
		int getLeaderId() const { return mLeaderId; }
		void setLeaderId(int id) { mLeaderId = id; }
		int getCastleId() const { return mCastleId; }
		void setCastleId(int id) { mCastleId = id; }

		void updateEconomy(int effectiveTaxRate)
		{
			if (effectiveTaxRate <= 0)
				return;

			mStatus.momey += std::max(1, effectiveTaxRate * (mStatus.countTerritory + 2 * mStatus.countVillege + 4 * mStatus.countCastle) / 20);
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

		void addCastle(Tile& tile)
		{
			tile.id = TID_CASTLE;
			tile.meta = getId();
			++mStatus.countCastle;
		}

		void removeCastle(Tile& tile)
		{
			if (tile.id == TID_CASTLE && tile.meta == getId())
			{
				tile.id = TID_DIRT;
				tile.meta = INDEX_NONE;
				tile.unitId = INDEX_NONE;
				--mStatus.countCastle;
			}
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
		int    mCastleId = INDEX_NONE;
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

			createCastle(Vec2i(3, 3), 0);
			createCastle(Vec2i(size.x - 4, 3), 1);
			createCastle(Vec2i(3, size.y - 4), 2);
			createCastle(Vec2i(size.x - 4, size.y - 4), 3);

			createVillage(Vec2i(5, 3), 0, 72);
			createVillage(Vec2i(size.x - 6, 3), 1, 72);
			createVillage(Vec2i(5, size.y - 4), 2, 72);
			createVillage(Vec2i(size.x - 6, size.y - 4), 3, 72);

			createLeader(Vec2i(4, 3), 0);
			createLeader(Vec2i(size.x - 5, 3), 1);
			createLeader(Vec2i(4, size.y - 4), 2);
			createLeader(Vec2i(size.x - 5, size.y - 4), 3);
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

		bool isLeaderInCastle(int owner) const
		{
			if (owner < 0 || owner >= NumKingdoms)
				return false;

			Kingdom const& kingdom = mPlayers[owner];
			int leaderId = kingdom.getLeaderId();
			int castleId = kingdom.getCastleId();
			if (leaderId == INDEX_NONE || castleId == INDEX_NONE)
				return false;

			ActorData const& leader = mUnitData[leaderId];
			ActorData const& castle = mUnitData[castleId];
			return leader.bAlive && castle.bAlive && leader.pos == castle.pos;
		}

		int getEffectiveTaxRate(int owner) const
		{
			if (!isLeaderInCastle(owner))
				return 0;

			return mPlayers[owner].getTaxRate();
		}

		bool canEnter(Vec2i const& pos) const
		{
			if (!isValidPos(pos))
				return false;

			TerrainID id = TerrainID(getTile(pos).id);
			return !isBlocked(id) || id == TID_FOREST;
		}

		bool canBuildVillage(Vec2i const& pos, int owner = INDEX_NONE) const
		{
			if (!isValidPos(pos))
				return false;

			Tile const& tile = getTile(pos);
			if (tile.id != TID_DIRT && !(tile.id == TID_TERRITORY && (owner == INDEX_NONE || tile.meta == owner)))
				return false;

			for (int i = 0; i < 8; ++i)
			{
				Vec2i linkPos = pos + linkOffset(i);
				if (isValidPos(linkPos) && (getTile(linkPos).id == TID_VILLAGE || getTile(linkPos).id == TID_CASTLE))
					return false;
			}
			return true;
		}

		bool commandActor(int actorId, Action::Id action, Vec2i const& dest)
		{
			if (actorId < 0 || actorId >= int(mUnitData.size()))
				return false;

			ActorData& actor = mUnitData[actorId];
			if (!actor.bAlive || actor.type == ActorData::eVillage || actor.type == ActorData::eCastle || !isValidPos(dest))
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
					mPlayers[i].updateEconomy(getEffectiveTaxRate(i));
			}

			for (int id = 0; id < int(mUnitData.size()); ++id)
			{
				if (mUnitData[id].bAlive && mUnitData[id].type == ActorData::eVillage)
					updateVillage(id);
				else if (mUnitData[id].bAlive && mUnitData[id].type == ActorData::eCastle)
					updateCastle(id);
			}

			for (int id = 0; id < int(mUnitData.size()); ++id)
			{
				if (mUnitData[id].bAlive && mUnitData[id].type != ActorData::eVillage && mUnitData[id].type != ActorData::eCastle)
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

		int createCastle(Vec2i const& pos, int owner)
		{
			int id = createActor(pos, owner, 180, ActorData::eCastle);
			Tile& tile = getTile(pos);
			mPlayers[owner].addCastle(tile);
			mPlayers[owner].setCastleId(id);
			tile.unitId = id;
			claimAround(pos, owner);
			return id;
		}

		int createVillage(Vec2i const& pos, int owner, int score)
		{
			int id = createActor(pos, owner, score, ActorData::eVillage);
			Tile& tile = getTile(pos);
			if (tile.id == TID_TERRITORY && tile.meta == owner)
				mPlayers[owner].removeTerritory(tile);
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
			if (isBlocked(TerrainID(tile.id)) || tile.id == TID_VILLAGE || tile.id == TID_CASTLE)
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
			int effectiveTaxRate = getEffectiveTaxRate(village.owner);

			int numTerritory = 0;
			int tileId[8];
			for (int i = 0; i < 8; ++i)
			{
				Vec2i pos = village.pos + linkOffset(i);
				tileId[i] = isValidPos(pos) ? getTile(pos).id : TID_HILL;
				if (isValidPos(pos) && getTile(pos).id == TID_TERRITORY && getTile(pos).meta == village.owner)
					++numTerritory;
			}

			int growRate = (numTerritory + 1) - player.getBaseDecRate(village.score, numTerritory, effectiveTaxRate);
			village.score += std::max(1, growRate);
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

		void updateCastle(int castleId)
		{
			ActorData& castle = mUnitData[castleId];
			int effectiveTaxRate = getEffectiveTaxRate(castle.owner);

			castle.score = Math::Clamp(castle.score + 1 + effectiveTaxRate / 10, 0, 255);
			if (castle.score >= 120 && produceUnit(castle.pos, castle.owner, castle.score / 2))
			{
				castle.score /= 2;
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

			if (tryCompleteAutoAction(actorId))
				return;

			if (actor.curAction == Action::eMove && actor.pos == actor.posDest)
			{
				actor.curAction = Action::eStandBy;
				return;
			}

			if (actor.curAction == Action::eAuto || actor.pos == actor.posDest)
				chooseAutoTarget(actorId);

			Vec2i nextPos = chooseStep(actor.pos, actor.posDest, actor.owner);
			if (nextPos == actor.pos)
				return;

			moveActor(actorId, nextPos);
			actor.moveCooldown = (actor.type == ActorData::eLeader) ? 2 : 3;
		}

		void chooseAutoTarget(int actorId)
		{
			ActorData& actor = mUnitData[actorId];
			Vec2i target;

			if (actor.type == ActorData::eLeader)
			{
				int castleId = getPlayer(actor.owner).getCastleId();
				if (castleId != INDEX_NONE && mUnitData[castleId].bAlive)
				{
					actor.curAction = Action::eDefend;
					actor.posDest = mUnitData[castleId].pos;
				}
				return;
			}

			if (findNearestAutoWorkerTarget(actor.owner, actor.pos, target))
			{
				actor.curAction = Action::eAuto;
				actor.posDest = target;
				return;
			}

			if (findNearestOwnedVillage(actor.owner, actor.pos, target))
			{
				actor.curAction = Action::eStandBy;
				actor.posDest = target;
				return;
			}

			if (actor.score >= 80 && findNearestEnemyAsset(actor.owner, actor.pos, target))
			{
				actor.curAction = Action::eAttack;
				actor.posDest = target;
			}
		}

		bool tryCompleteAutoAction(int actorId)
		{
			ActorData& actor = mUnitData[actorId];
			if (actor.type != ActorData::eArmy || actor.curAction != Action::eAuto)
				return false;

			Kingdom& kingdom = getPlayer(actor.owner);

			Tile& tile = getTile(actor.pos);
			if (tile.id == TID_FOREST)
			{
				tile.id = TID_DIRT;
				tile.meta = INDEX_NONE;
				actor.score = std::max(8, actor.score - 8);
				return false;
			}

			if (actor.score >= 110 && kingdom.getStatus().momey >= 50 && canBuildVillage(actor.pos, actor.owner))
			{
				kingdom.getStatus().momey -= 50;
				int score = actor.score / 2;
				if (tile.unitId == actorId)
					tile.unitId = INDEX_NONE;
				actor.bAlive = false;
				createVillage(actor.pos, actor.owner, std::max(48, score));
				return true;
			}

			if (actor.pos == actor.posDest)
			{
				Vec2i home;
				if (findNearestOwnedVillageWaitTile(actor.owner, actor.pos, home))
				{
					actor.curAction = Action::eMove;
					actor.posDest = home;
				}
			}

			return false;
		}

		bool findNearestEnemyAsset(int owner, Vec2i const& from, Vec2i& outPos) const
		{
			int bestDist = std::numeric_limits<int>::max();
			bool bFound = false;

			for (ActorData const& actor : mUnitData)
			{
				if (!actor.bAlive || actor.owner == owner)
					continue;

				if (actor.type != ActorData::eVillage && actor.type != ActorData::eLeader && actor.type != ActorData::eCastle)
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

		bool isAutoWorkerTargetTile(int owner, Vec2i const& pos) const
		{
			if (!isValidPos(pos))
				return false;

			Tile const& tile = getTile(pos);
			if (tile.id == TID_BRIDGE || tile.id == TID_CASTLE || tile.id == TID_VILLAGE)
				return false;

			if (tile.id == TID_TERRITORY && tile.meta == owner)
				return false;

			if (tile.id == TID_DIRT || tile.id == TID_FOREST)
				return canBuildVillageAfterClear(pos, owner);

			return tile.id == TID_TERRITORY && tile.meta != owner;
		}

		bool canBuildVillageAfterClear(Vec2i const& pos, int owner) const
		{
			if (!isValidPos(pos))
				return false;

			Tile const& tile = getTile(pos);
			if (tile.id != TID_DIRT && tile.id != TID_FOREST && !(tile.id == TID_TERRITORY && tile.meta == owner))
				return false;

			for (int i = 0; i < 8; ++i)
			{
				Vec2i linkPos = pos + linkOffset(i);
				if (isValidPos(linkPos) && (getTile(linkPos).id == TID_VILLAGE || getTile(linkPos).id == TID_CASTLE))
					return false;
			}
			return true;
		}

		bool findNearestAutoWorkerTarget(int owner, Vec2i const& from, Vec2i& outPos) const
		{
			TGrid2D<int> distMap(mMap.getSize());
			distMap.fillValue(INDEX_NONE);

			std::vector<Vec2i> queue;
			queue.push_back(from);
			distMap(from) = 0;

			bool bFound = false;
			int bestDist = std::numeric_limits<int>::max();
			for (int read = 0; read < int(queue.size()); ++read)
			{
				Vec2i pos = queue[read];
				int dist = distMap(pos);

				if (dist > 0 && isAutoWorkerTargetTile(owner, pos) && dist < bestDist)
				{
					bestDist = dist;
					outPos = pos;
					bFound = true;
				}

				for (int i = 0; i < 8; ++i)
				{
					Vec2i next = pos + linkOffset(i);
					if (!canEnter(next) || distMap(next) != INDEX_NONE)
						continue;

					Tile const& tile = getTile(next);
					if (tile.unitId != INDEX_NONE && mUnitData[tile.unitId].owner != owner)
						continue;

					distMap(next) = dist + 1;
					queue.push_back(next);
				}
			}

			return bFound;
		}

		bool findNearestOwnedVillage(int owner, Vec2i const& from, Vec2i& outPos) const
		{
			int bestDist = std::numeric_limits<int>::max();
			bool bFound = false;
			for (ActorData const& actor : mUnitData)
			{
				if (!actor.bAlive || actor.owner != owner || actor.type != ActorData::eVillage)
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

		bool findNearestOwnedVillageWaitTile(int owner, Vec2i const& from, Vec2i& outPos) const
		{
			int bestDist = std::numeric_limits<int>::max();
			bool bFound = false;
			for (ActorData const& actor : mUnitData)
			{
				if (!actor.bAlive || actor.owner != owner || actor.type != ActorData::eVillage)
					continue;

				for (int i = 0; i < 8; ++i)
				{
					Vec2i pos = actor.pos + linkOffset(i);
					if (!canEnter(pos))
						continue;

					Tile const& tile = getTile(pos);
					if (tile.unitId != INDEX_NONE)
						continue;

					int dist = TaxicabDistance(from, pos);
					if (dist < bestDist)
					{
						bestDist = dist;
						outPos = pos;
						bFound = true;
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
				score += getMoveCostScore(owner, tile);

				if (score < bestScore)
				{
					bestScore = score;
					best = pos;
				}
			}

			return best;
		}

		int getMoveCostScore(int owner, Tile const& tile) const
		{
			if (tile.id == TID_VILLAGE && tile.meta == owner)
				return -10;

			if (tile.id == TID_CASTLE && tile.meta == owner)
				return -8;

			if (tile.id == TID_TERRITORY && tile.meta == owner)
				return 2;

			return 4;
		}

		bool tryCompleteBuildAction(int actorId)
		{
			ActorData& actor = mUnitData[actorId];
			Kingdom& kingdom = getPlayer(actor.owner);

			switch (actor.curAction)
			{
			case Action::eBuildVillage:
				if (actor.pos == actor.posDest && canBuildVillage(actor.pos, actor.owner) && kingdom.getStatus().momey >= 50)
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
			Tile& toTile = getTile(pos);
			if (toTile.unitId != INDEX_NONE && mUnitData[toTile.unitId].owner == actor.owner)
			{
				if (tryMergeActor(actorId, toTile.unitId))
					return;

				return;
			}

			Tile& fromTile = getTile(actor.pos);
			if (fromTile.unitId == actorId)
				fromTile.unitId = INDEX_NONE;

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
			applyMoveDamage(actorId, toTile);
		}

		void applyMoveDamage(int actorId, Tile const& tile)
		{
			ActorData& actor = mUnitData[actorId];
			if (!actor.bAlive || actor.type != ActorData::eArmy)
				return;

			if ((tile.id == TID_VILLAGE || tile.id == TID_CASTLE) && tile.meta == actor.owner)
				return;

			int damage = 1;
			if (tile.id == TID_TERRITORY && tile.meta == actor.owner)
				damage = 1;
			else if (tile.id == TID_FOREST)
				damage = 2;

			actor.score -= damage;
			if (actor.score <= 0)
				killActor(actorId);
		}

		bool tryMergeActor(int actorId, int otherId)
		{
			ActorData& actor = mUnitData[actorId];
			ActorData& other = mUnitData[otherId];
			if (!actor.bAlive || !other.bAlive)
				return false;

			if (actor.type != ActorData::eArmy || other.type != ActorData::eArmy)
				return false;

			int totalScore = actor.score + other.score;
			ActorData* keep = &actor;
			ActorData* remove = &other;
			int keepId = actorId;
			int removeId = otherId;
			if (other.score >= actor.score)
			{
				keep = &other;
				remove = &actor;
				keepId = otherId;
				removeId = actorId;
			}

			Vec2i mergePos = other.pos;
			keep->score = Math::Min(255, totalScore);
			remove->bAlive = false;

			Tile& removeTile = getTile(remove->pos);
			if (removeTile.unitId == removeId)
				removeTile.unitId = INDEX_NONE;

			if (keepId == actorId)
			{
				Tile& oldTile = getTile(actor.pos);
				if (oldTile.unitId == actorId)
					oldTile.unitId = INDEX_NONE;

				actor.pos = mergePos;
				getTile(mergePos).unitId = actorId;
			}

			return true;
		}

		void resolveCombat(int attackerId, int defenderId)
		{
			ActorData& attacker = mUnitData[attackerId];
			ActorData& defender = mUnitData[defenderId];
			if (attacker.owner == defender.owner)
				return;

			int attackPower = attacker.score;
			int defensePower = defender.score;
			if (defender.type == ActorData::eVillage)
				defensePower += 24;
			else if (defender.type == ActorData::eCastle)
				defensePower += 64;

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
			else if (actor.type == ActorData::eCastle)
			{
				getPlayer(actor.owner).removeCastle(tile);
				getPlayer(actor.owner).setCastleId(INDEX_NONE);
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
