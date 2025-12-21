#ifndef ABUnit_h__
#define ABUnit_h__

#include "Math/Vector2.h"
#include "ABDefine.h"
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
		UnitDefinition const* getRandomUnit(int level, int randomValue) const;

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
		virtual ~Unit();

		void setStats(UnitStats const& stats);
		void setPos(Vector2 const& pos);
		void setTeam(UnitTeam team) { mTeam = team; }
		UnitTeam getTeam() const { return mTeam; }

		void update(float dt, World& world);
		void render(IGraphics2D& g, bool bShowState = true);


		Vector2 const& getPos() const { return mPos; }
		bool isDead() const { return mState == SectionState::Dead; }
		float getAlpha() const { return (isDead()) ? Math::Clamp(mDeathTimer, 0.0f, 1.0f) : 1.0f; }
		uint32 getMagic() const { return mMagic; }
		
		void takeDamage(float damage);
		void resetRound();
		UnitStats const& getStats() const { return mStats; }
		
		void setUnitId(int id) { mUnitId = id; }
		int getUnitId() const { return mUnitId; }

		void setTypeId(int id) { mTypeId = id; }
		int getTypeId() const { return mTypeId; }



		void place(PlayerBoard& board, Vec2i const& pos);


		void setInternalBoard(PlayerBoard* board) { mCurrentBoard = board; }
		PlayerBoard* getInternalBoard() const { return mCurrentBoard; }

		void setIsGhost(bool val) { mbIsGhost = val; }
		bool isGhost() const { return mbIsGhost; }

		void setHoldLocation(UnitLocation const& loc) { mHoldLocation = loc; }
		UnitLocation const& getHoldLocation() const { return mHoldLocation; }


		void restoreStartState();
		void stopMove();

	protected:
		uint32    mMagic = 0x12345678;
		uint32    mPadding[5] = { 0, 0, 0, 0, 0 };
		Vector2   mPos;
		Vector2   mBattleStartPos;
		UnitStats mStats;
		SectionState mState;
		UnitTeam  mTeam;
		bool      mbIsGhost = false;

		UnitLocation mHoldLocation;

		PlayerBoard* mCurrentBoard = nullptr;

		Unit*     mTarget;
		float     mAttackTimer;
		float     mMoveSpeed; // Maybe from stats?

		float     mCastTimer;
		float     mActionCooldown; // New member variable
		int       mUnitId;
		int       mTypeId;

		Vector2   mNextCellPos;
		bool      mMovingToNextCell = false;
	
		float     mDeathTimer = 0.0f; // Timer for death fade-out effect

	public:
		void findTarget(World& world);
		void startMoveStep(Vector2 const& targetPos, World& world);
		void startVisualMove(Vector2 const& targetPos) { mNextCellPos = targetPos; mMovingToNextCell = true; }
		void moveStep(float dt);
		void attack(Unit* target, World& world);
		void executeSkill(World& world);
	};

} // namespace AutoBattler

#endif // ABUnit_h__
