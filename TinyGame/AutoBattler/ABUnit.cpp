#include "ABPCH.h"
#include "ABUnit.h"
#include "RenderUtility.h"
#include "GameGraphics2D.h"
#include "GameGlobal.h"

#include "ABWorld.h"

namespace AutoBattler
{

	Unit::Unit()
	{
		mPos = Vector2(-1000, -1000);
		mState = SectionState::Idle;
		mTeam = UnitTeam::Player;
		mTarget = nullptr;
		mAttackTimer = 0.0f;
		
		mUnitId = 1;
		mCastTimer = 0.0f;

		// Default test stats
		mStats.maxHp = 100;
		mStats.hp = 100;
		mStats.maxMana = 100;
		mStats.mana = 0;
		mStats.attackDamage = 10;
		mStats.attackSpeed = 1.0f;
		mStats.range = 50.0f; // Melee range
		mMoveSpeed = 100.0f;
	}

	void Unit::setStats(UnitStats const& stats)
	{
		mStats = stats;
	}

	void Unit::setPos(Vector2 const& pos)
	{
		mPos = pos;
	}

	void Unit::update(float dt, World& world)
	{
		if (isDead())
		{
			// Countdown death timer for fade-out
			if (mDeathTimer > 0)
			{
				mDeathTimer -= dt;
			}
			return;
		}

		if (world.getPhase() == BattlePhase::Combat)
		{
			// Bench units should not fight
			if (!mCurrentBoard)
				return;

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
					executeSkill();
					mStats.mana = 0;
					mState = SectionState::Idle;
				}
				return;
			}

			// PRIORITY 1: Finish Current Move Step
			if (mMovingToNextCell)
			{
				moveStep(dt);
				return; // Do not attack or retarget while moving between cells
			}

			// PRIORITY 2: Check Target & Attack
			// Determine Target
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
				if (dist <= mStats.range + 20.0f)
				{
					// Attack
					mAttackTimer -= dt;
					if (mAttackTimer <= 0)
					{
						attack(mTarget, world);
						mAttackTimer = 1.0f / mStats.attackSpeed;
					}
				}
				else
				{
					// PRIORITY 3: Start New Move Step
					startMoveStep(mTarget->getPos(), world);
				}
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

	void Unit::startMoveStep(Vector2 const& targetPos, World& world)
	{
		PlayerBoard* boardPtr = mCurrentBoard;
		if (!boardPtr)
			return;
			
		PlayerBoard& board = *boardPtr;

		// Decide next move
		Vec2i gridPos = board.getCoord(mPos);

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
				if (!board.isOccupied(nx, ny)) // Check occupation
				{
					Vector2 cellPos = board.getPos(nx, ny);
					float distSq = (cellPos - targetPos).length2();
					if (distSq < bestDistSq - 1.0f) // Must be strictly better
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
			board.setUnit(bestNeighbor.x, bestNeighbor.y, this);
			board.setUnit(gridPos.x, gridPos.y, nullptr); // Leave current

			mNextCellPos = board.getPos(bestNeighbor.x, bestNeighbor.y);
			mMovingToNextCell = true;
		}
		else
		{
			// No better neighbor found (stuck or reached best local spot)
		}
	}

	void Unit::attack(Unit* target, World& world)
	{
		if (target)
		{
			// Gain Mana on Attack
			mStats.mana = Math::Min(mStats.mana + 10.0f, mStats.maxMana);

			if (mStats.range > 100.0f)
			{
				// Ranged Attack: Spawn Projectile
				world.addProjectile(this, target, 500.0f, mStats.attackDamage);
			}
			else
			{
				// Melee Attack: Instant Damage
				target->takeDamage(mStats.attackDamage);
			}
		}
	}

	void Unit::executeSkill()
	{
		// Simple Skill: Deal damage to current target (or nearest if lost)
		if (mTarget && !mTarget->isDead())
		{
			mTarget->takeDamage(50.0f); // Big damage
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

	void Unit::saveStartState()
	{
		mBattleStartPos = mPos;
	}

	void Unit::restoreStartState()
	{
		mPos = mBattleStartPos;
		mStats.hp = mStats.maxHp;
		mStats.mana = 0;
		mState = SectionState::Idle;
		mTarget = nullptr;
		mAttackTimer = 0;
		mCastTimer = 0;
		mMovingToNextCell = false;
	}

	void Unit::render(IGraphics2D& g, bool bShowState)
	{
		// Don't render if death fade-out is complete
		if (isDead() && mDeathTimer <= 0)
			return;
		
		// Calculate alpha for fade-out effect
		float alpha = 1.0f;
		if (isDead() && mDeathTimer > 0)
		{
			alpha = mDeathTimer; // Fade from 1.0 to 0.0
		}
		
		if (isDead())
		{
			RenderUtility::SetBrush(g, EColor::Gray, (uint8)(alpha * 255));
		}
		else if (mTeam == UnitTeam::Player)
		{
			RenderUtility::SetBrush(g, EColor::Blue);
		}
		else
		{
			RenderUtility::SetBrush(g, EColor::Red);
		}
		
		RenderUtility::SetPen(g, EColor::White);
		g.drawCircle(mPos, 20); 

		if (mState == SectionState::Cast)
		{
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawCircle(mPos, 22);
		}

		// UI Bars
		if (bShowState && !isDead())
		{
			Vector2 barPos = mPos - Vector2(20, 30);
			Vector2 barSize(40, 5);
			
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawRect(barPos, barSize);
			
			float hpRatio = mStats.hp / mStats.maxHp;
			Vector2 curBarSize(40 * hpRatio, 5);
			RenderUtility::SetBrush(g, EColor::Green);
			g.drawRect(barPos, curBarSize);

			// Mana Bar
			barPos.y += 6;
			RenderUtility::SetBrush(g, EColor::Black);
			g.drawRect(barPos, barSize);

			float manaRatio = mStats.mana / mStats.maxMana;
			curBarSize.x = 40 * manaRatio;
			RenderUtility::SetBrush(g, EColor::Blue);
			g.drawRect(barPos, curBarSize);
		}
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

	UnitDefinition const* UnitDataManager::getRandomUnit(int level) const
	{
		// Simple random for now, ignore level odds
		if (mUnits.empty()) return nullptr;
		int idx = ::Global::RandomNet() % mUnits.size();
		return &mUnits[idx];
	}

} // namespace AutoBattler
