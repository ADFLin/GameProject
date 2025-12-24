#include "ABPCH.h"
#include "ABUnit.h"
#include "RenderUtility.h"
#include "GameGraphics2D.h"
#include "GameGlobal.h"

#include "ABWorld.h"

#if 0
#include "Algo/AStar.h"
#endif

namespace AutoBattler
{

	Unit::Unit()
	{


		mPos = Vector2(-1000, -1000);
		mBattleStartPos = Vector2::Zero();
		mState = SectionState::Idle;
		mTeam = UnitTeam::Player;
		mTarget = nullptr;

		mAttackTimer = 0.0f;
		
		mUnitId = 1;
		mTypeId = 0;
		mCastTimer = 0.0f;

		mNextCellPos = Vector2::Zero();
		mMovingToNextCell = false;
		mDeathTimer = 0.0f;

		// Default test stats
		mStats.maxHp = 100;
		mStats.hp = 100;
		mStats.maxMana = 100;
		mStats.mana = 0;
		mStats.attackDamage = 10;
		mStats.attackSpeed = 1.0f;
		mStats.range = 50.0f; // Melee range
		mMoveSpeed = 100.0f;
		mActionCooldown = 0.0f;

		mDeployLocation.type = ECoordType::None;
		mDeployLocation.type = ECoordType::None;
	}
	
	Unit::~Unit()
	{
	}

	void Unit::setPos(Vector2 const& pos)
	{
		mPos = pos;
	}
	
	void Unit::setStats(UnitStats const& stats)
	{
		mStats = stats;
	}



	void Unit::update(float dt, World& world)
	{
		if (isDead())
		{
			if (mDeathTimer > 0)
				mDeathTimer -= dt;
			return;
		}

		if (world.getPhase() == BattlePhase::Combat)
		{
			if (!mCurrentBoard)
				return;

			// ======== AI Logic (Only if we are simulating combat) ========
			// In Network Client mode, isAutoResolveCombat is false, so we never run AI.
			// We only walk/attack when driven by Replay events.
			if (world.isAutoResolveCombat())
			{
				// Process Cooldown
				if (mActionCooldown > 0.0f)
				{
					mActionCooldown -= dt;
					if (mActionCooldown > 0.0f)
						return; // Wait for cooldown
				}

				// Simple AI:
				// 1. Move to next cell if moving
				// 2. If not moving, find target
				// 3. If target found, move towards target or attack
				// Skill Cast Check
				if (mState != SectionState::Cast && mStats.mana >= mStats.maxMana)
				{
					mState = SectionState::Cast;
					mCastTimer = 1.0f;
				}

				if (mState == SectionState::Cast)
				{
					mCastTimer -= dt;
					if (mCastTimer <= 0)
					{
						executeSkill(world);
						mStats.mana = 0;
						mState = SectionState::Idle;
						mActionCooldown = 0.5f; // Recovery after cast
					}
					return;
				}

				// All AI logic: find target, attack decision, move decision
				if (mTarget && (mTarget->isDead() || (mTarget->getPos() - mPos).length2() > 500 * 500))
				{
					mTarget = nullptr;
				}
				if (mTarget == nullptr)
				{
					findTarget(world);
				}

				if (mTarget)
				{
					float dist = Math::Distance(mTarget->getPos(), mPos);
					if (dist <= mStats.range + 40.0f)
					{
						// Attack
						mAttackTimer -= dt;
						if (mAttackTimer <= 0)
						{
							attack(mTarget, world);
							mAttackTimer = 1.0f / mStats.attackSpeed;
							mActionCooldown = 0.3f; // Recovery after attack
						}
					}
					else
					{
						// Move towards target
						if (!mMovingToNextCell) // Only start new step if not already moving (though update prevents this usually)
						{
							startMoveStep(mTarget->getPos(), world);
						}
					}
				}
				else
				{
					// No target found, do nothing or wander
				}
			} // end of AI check

			// ======== 共用邏輯：移動插值 ========
			if (mMovingToNextCell)
			{
				moveStep(dt);
			}
		}
	}

	void Unit::moveStep(float dt)
	{
		Vector2 dir = mNextCellPos - mPos;
		float dist = dir.length();
		float moveDist = mMoveSpeed * dt;

		if (dist <= moveDist)
		{
			mPos = mNextCellPos;
			mMovingToNextCell = false; // Step Complete
			mActionCooldown = 0.15f; // Small delay between steps
		}
		else
		{
			dir /= dist;
			mPos += dir * moveDist;
		}
	}

	void Unit::stopMove()
	{
		mMovingToNextCell = false;
		// mState = SectionState::Idle; // Optional: Keep state if needed, but usually deployment implies Idle
	}

	void Unit::findTarget(World& world)
	{
		mTarget = world.getNearestEnemy(mPos, mTeam, mCurrentBoard);
	}


#if 0
	namespace
	{
		using namespace AStar;

		struct BattleNode : public NodeBaseT<BattleNode, Vec2i, float>
		{
		};

		class BattleAStar : public AStarT<BattleAStar, BattleNode>
		{
		public:
			BattleAStar(PlayerBoard& board, Unit& unit, Vec2i goal)
				: mBoard(board), mUnit(unit), mGoal(goal)
			{
			}

			PlayerBoard& mBoard;
			Unit& mUnit;
			Vec2i mGoal;

			ScoreType calcHeuristic(StateType& state)
			{
				// Euclidean distance heuristic
				Vector2 pA = mBoard.getWorldPos(state.x, state.y);
				Vector2 pB = mBoard.getWorldPos(mGoal.x, mGoal.y);
				return Math::Distance(pA, pB);
			}

			ScoreType calcDistance(StateType& a, StateType& b)
			{
				Vector2 pA = mBoard.getWorldPos(a.x, a.y);
				Vector2 pB = mBoard.getWorldPos(b.x, b.y);
				return Math::Distance(pA, pB);
			}

			bool isEqual(StateType& s1, StateType& s2) { return s1 == s2; }
			bool isGoal(StateType& s) { return s == mGoal; }

			void processNeighborNode(NodeType& node)
			{
				// Hex grid neighbor offsets
				static const int offsets[2][6][2] = {
					{ {1,0}, {-1,0}, {0,-1}, {0,1}, {-1,-1}, {-1,1} }, // Even Row
					{ {1,0}, {-1,0}, {0,-1}, {0,1}, {1,-1}, {1,1} }    // Odd Row
				};

				int parity = Math::Abs(node.state.y) % 2;

				for (int i = 0; i < 6; ++i)
				{
					int nx = node.state.x + offsets[parity][i][0];
					int ny = node.state.y + offsets[parity][i][1];
					Vec2i nextPos(nx, ny);

					if (mBoard.isValid(nextPos))
					{
						bool isWalkable = !mBoard.isOccupied(nx, ny);
						// Treat goal as walkable so we can path TO it (even if occupied by target)
						if (nextPos == mGoal) isWalkable = true;
					
						// Also treat self as walkable (though we are moving FROM it)
						// And fix for "ghost" occupation if needed, similar to greedy check
						if (!isWalkable)
						{
							Unit* occUnit = mBoard.getUnit(nx, ny);
							if (occUnit == &mUnit) isWalkable = true;
						}

						if (isWalkable)
						{
							addSreachNode(nextPos, node);
						}
					}
				}
			}
		};
	}
#endif

	void Unit::startMoveStep(Vector2 const& targetPos, World& world)
	{
		PlayerBoard& board = *mCurrentBoard; // Assume calling startMoveStep means we are on a board
		Vec2i gridPos = board.getCoord(mPos);
		Vec2i targetGridPos = board.getCoord(targetPos);

#if 0
		// Use A* Pathfinding
		BattleAStar astar(board, *this, targetGridPos);
		BattleAStar::SreachResult result;
		
		if (astar.sreach(gridPos, result))
		{
			// Path found!
			// We need the immediate next step (the child of startNode in the path)
			// The path is linked: Goal -> Parent -> ... -> Start
			// We want the node whose parent is Start.
			
			BattleNode* step = result.globalNode;
			BattleNode* nextStep = nullptr;

			// Backtrack from goal to find the step after start
			while (step != nullptr && step->parent != nullptr)
			{
				if (step->parent->state == gridPos)
				{
					nextStep = step;
					break;
				}
				step = step->parent;
			}

			if (nextStep)
			{
				world.applyUnitMove(*this, nextStep->state);
				return;
			}
		}

		// Fallback to simple greedy if A* fails (e.g. no path) or start==goal
		LogMsg("Unit %d A* Failed to %d,%d. Fallback to greedy.", mUnitId, targetGridPos.x, targetGridPos.y);
#endif
		
		static const int offsets[2][6][2] = {
			{ {1,0}, {-1,0}, {0,-1}, {0,1}, {-1,-1}, {-1,1} }, // Even Row
			{ {1,0}, {-1,0}, {0,-1}, {0,1}, {1,-1}, {1,1} }    // Odd Row
		};

		int parity = Math::Abs(gridPos.y) % 2;
		
		// Current distance to target
		float currentDistSq = (mPos - targetPos).length2();
		
		Vec2i bestNeighbor = gridPos;
		float bestDistSq = currentDistSq;
		bool foundBetter = false;

		// Look for neighbors that get us closer
		for (int i = 0; i < 6; ++i)
		{
			int nx = gridPos.x + offsets[parity][i][0];
			int ny = gridPos.y + offsets[parity][i][1];
			
			if (board.isValid(Vec2i(nx, ny)))
			{
				bool isOccupied = board.isOccupied(nx, ny);
				if (isOccupied)
				{
					// Allow moving into cell if it's occupied by SELF (fix for getting stuck on own occupancy)
					Unit* occUnit = board.getUnit(nx, ny);
					// FIX: Use pointer comparison instead of ID to avoid issues with duplicate IDs in simulation
					if (occUnit == this)
						isOccupied = false;
				}

				if (!isOccupied) // Check occupation
				{
					Vector2 cellPos = board.getWorldPos(nx, ny);
					float distSq = (cellPos - targetPos).length2();
					if (distSq < bestDistSq - 0.1f) // Must be strictly better (relaxed to prevent hesitation)
					{
						bestDistSq = distSq;
						bestNeighbor = Vec2i(nx, ny);
						foundBetter = true;
					}
				}
			}
		}

		if (foundBetter)
		{
			// Update occupation
			// Release old cell? No, we release when we physically leave?
			// Usually we reserve the new cell immediately so no one else takes it.
			// And we release the old cell when we leave.
			
			// Simple Logic: Occupy NEW cell immediately. Release OLD cell immediately (jump occupation)
			// This prevents others from moving into our target or our current.
			// Ideally: "Unit is occupying cell X" and "Unit is moving to cell Y (reserved)"
			
			// For basic phase 2-4:
			world.applyUnitMove(*this, bestNeighbor);
		}
		else
		{
			// No better neighbor found (stuck or reached best local spot)
			// LogMsg("U%d STUCK. Grid=(%d, %d) Dist=%.2f Target=(%.1f, %.1f)", 
			// 	mUnitId, gridPos.x, gridPos.y, Math::Sqrt(currentDistSq), targetPos.x, targetPos.y);
		}
	}

	void Unit::attack(Unit* target, World& world)
	{
		if (target)
		{
			// 委託給World統一處理
			world.applyUnitAttack(*this, *target);
		}
	}

	void Unit::executeSkill(World& world)
	{
		if (mTarget)
		{
			world.applyUnitCastSkill(*this, 1, mTarget);  // skillID = 1
		}
	}

	void Unit::takeDamage(float damage)
	{
		mStats.hp -= damage;
		if (mStats.hp <= 0)
		{
			mStats.hp = 0;
			mState = SectionState::Dead;
			mDeathTimer = 1.0f; // Start fade-out (1 second)
			
			// Clear cell occupation when unit dies
			if (mCurrentBoard)
			{
				Vec2i coord = mCurrentBoard->getCoord(mPos);
				if (mCurrentBoard->isValid(coord) && mCurrentBoard->getUnit(coord.x, coord.y) == this)
				{
					mCurrentBoard->setUnit(coord.x, coord.y, nullptr);
				}
			}
		}
		else
		{
			mStats.mana = Math::Min(mStats.mana + 5.0f, mStats.maxMana);
		}
	}

	void Unit::resetRound()
	{
		mStats.hp = mStats.maxHp;
		mStats.mana = 0; 
		mState = SectionState::Idle;
		mTarget = nullptr;
		mAttackTimer = 0.0f;
		mMovingToNextCell = false;
	}

	void Unit::remove()
	{
		if (mCurrentBoard && mHoldLocation.type == ECoordType::Board)
		{
			CHECK(mCurrentBoard->getUnit(mHoldLocation.pos.x, mHoldLocation.pos.y) == nullptr ||
			      mCurrentBoard->getUnit(mHoldLocation.pos.x, mHoldLocation.pos.y) == this);
			
			mCurrentBoard->setUnit(mHoldLocation.pos.x, mHoldLocation.pos.y, nullptr);
			mCurrentBoard = nullptr;
			mHoldLocation.type = ECoordType::None;
		}
		mTarget = nullptr;
	}

	void Unit::place(PlayerBoard& board, Vec2i const& pos)
	{
		remove();

		Vector2 worldPos = board.getWorldPos(pos.x, pos.y);
		setPos(worldPos);
		holdInternal(board, pos);
	}

	void Unit::hold(Vec2i const& pos)
	{
		PlayerBoard* board = mCurrentBoard;
		remove();
		holdInternal(*board, pos);
	}

	void Unit::holdInternal(PlayerBoard& board, Vec2i const& pos)
	{
		remove();
		CHECK(board.getUnit(pos.x, pos.y) == nullptr || board.getUnit(pos.x, pos.y) == this);
		setInternalBoard(&board);
		board.setUnit(pos.x, pos.y, this);

		mHoldLocation.type = ECoordType::Board;
		mHoldLocation.pos = pos;
	}

	void Unit::restoreStartState(PlayerBoard& board)
	{
		CHECK(mDeployLocation.type == ECoordType::Board);
		place(board, mDeployLocation.pos);

		//mPos = mBattleStartPos;
		mStats.hp = mStats.maxHp;
		mStats.mana = 0;
		mState = SectionState::Idle;
		mTarget = nullptr;
		mAttackTimer = 0;
		mCastTimer = 0;
		mMovingToNextCell = false;
	}



	void UnitDataManager::init()
	{
		mUnits.clear();

		// Helper to add unit
		auto AddUnit = [&](int id, char const* name, int cost, float hp, float dmg, float range, float as)
		{
			UnitDefinition def;
			def.id = id;
			def.name = name;
			def.cost = cost;
			def.baseStats.maxHp = hp;
			def.baseStats.hp = hp;
			def.baseStats.attackDamage = dmg;
			def.baseStats.range = range;
			def.baseStats.attackSpeed = as;
			def.baseStats.maxMana = 100;
			def.baseStats.mana = 0;
			// Defaults
			def.baseStats.abilityPower = 0;
			def.baseStats.armor = 20;
			def.baseStats.magicResist = 20;

			mUnits.push_back(def);
		};

		// 1. Warrior
		AddUnit(1, "Warrior", 1, 600, 50, 50, 0.6f);
		// 2. Hunter
		AddUnit(2, "Hunter", 1, 450, 40, 400, 0.7f);
		// 3. Knight
		AddUnit(3, "Knight", 2, 800, 30, 50, 0.5f);
		// 4. Mage
		AddUnit(4, "Mage", 2, 400, 60, 400, 0.6f);
		// 5. Assassin
		AddUnit(5, "Assassin", 3, 500, 80, 50, 0.8f);
	}

	UnitDefinition const* UnitDataManager::getUnit(int id) const
	{
		for(auto const& unit : mUnits)
		{
			if (unit.id == id)
				return &unit;
		}
		return nullptr;
	}

	UnitDefinition const* UnitDataManager::getRandomUnit(int level, int randomValue) const
	{
		// Simple random for now, ignore level odds
		if (mUnits.empty()) return nullptr;
		int idx = randomValue % mUnits.size();
		return &mUnits[idx];
	}

} // namespace AutoBattler
