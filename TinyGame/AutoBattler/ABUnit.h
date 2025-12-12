#ifndef ABUnit_h__
#define ABUnit_h__

#include "Math/Vector2.h"
#include "DataStructure/Array.h"

class IGraphics2D;

namespace AutoBattler
{
	using Math::Vector2;

	struct UnitStats
	{
		float maxHp;
		float hp;
		float maxMana;
		float mana;
		float attackDamage;
		float abilityPower;
		float armor;
		float magicResist;
		float attackSpeed;
		float range;
	};

	struct UnitDefinition
	{
		int id;
		char const* name;
		int cost;
		UnitStats baseStats;
	};

	class UnitDataManager
	{
	public:
		void init();
		UnitDefinition const* getUnit(int id) const;
		UnitDefinition const* getRandomUnit(int level) const;

	private:
		TArray<UnitDefinition> mUnits;
	};

	enum class SectionState
	{
		Idle,
		Move,
		Attack,
		Cast,
		Dead,
	};

	enum class UnitTeam
	{
		Player,
		Enemy,
	};

	class World;
	class PlayerBoard;

	class Unit
	{
	public:
		Unit();

		void setStats(UnitStats const& stats);
		void setPos(Vector2 const& pos);
		void setTeam(UnitTeam team) { mTeam = team; }
		UnitTeam getTeam() const { return mTeam; }

		void update(float dt, World& world);
		void render(IGraphics2D& g, bool bShowState = true);

		Vector2 const& getPos() const { return mPos; }
		bool isDead() const { return mState == SectionState::Dead; }
		
		void takeDamage(float damage);
		void resetRound();
		UnitStats const& getStats() const { return mStats; }
		
		void setUnitId(int id) { mUnitId = id; }
		int getUnitId() const { return mUnitId; }

		void setInternalBoard(PlayerBoard* board) { mCurrentBoard = board; }
		PlayerBoard* getInternalBoard() const { return mCurrentBoard; }

		void saveStartState();
		void restoreStartState();
		void stopMove();

	protected:
		Vector2   mPos;
		Vector2   mBattleStartPos;
		UnitStats mStats;
		SectionState mState;
		UnitTeam  mTeam;

		PlayerBoard* mCurrentBoard = nullptr;

		Unit*     mTarget;
		float     mAttackTimer;
		float     mMoveSpeed; // Maybe from stats?

		float     mCastTimer;
		int       mUnitId;

		Vector2   mNextCellPos;
		bool      mMovingToNextCell = false;
	
	float     mDeathTimer = 0.0f; // Timer for death fade-out effect

	public:
		void findTarget(World& world);
		void startMoveStep(Vector2 const& targetPos, World& world);
		void moveStep(float dt);
		void attack(Unit* target);
		void executeSkill();
	};

} // namespace AutoBattler

#endif // ABUnit_h__
