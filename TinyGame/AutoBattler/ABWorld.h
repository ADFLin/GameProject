#ifndef ABWorld_h__
#define ABWorld_h__

#include "ABPlayerBoard.h"
#include "ABPlayer.h"
#include "ABUnit.h"
#include "DataStructure/Array.h"
#include "ABRound.h"


namespace AutoBattler
{
	enum class BattlePhase
	{
		Preparation,
		Combat,
		End,
	};

	class World
	{
	public:
		World();
		~World();

		void init();
		void cleanup();
		void tick(float dt);
		void render(IGraphics2D& g);
		
		void restart(bool bInit);
		
		// Game Logic
		void changePhase(BattlePhase phase);
		BattlePhase getPhase() const { return mPhase; }
		float getPhaseTimer() const { return mPhaseTimer; }

		Unit* getNearestEnemy(Vector2 const& pos, UnitTeam team, PlayerBoard* board);

		// PlayerBoard& getPlayerBoard() { return mPlayerBoard; } // Deprecated/Context sensitive
		// Player& getPlayer() { return mPlayer; } // Deprecated
		
		Player& getPlayer(int index) { return mPlayers[index]; }
		// Local Player helper
		Player& getLocalPlayer() { return mPlayers[mLocalPlayerIndex]; }
		int     getLocalPlayerIndex() const { return mLocalPlayerIndex; }
		void    setLocalPlayerIndex(int index) { mLocalPlayerIndex = index; }
		PlayerBoard& getLocalPlayerBoard() { return getLocalPlayer().getBoard(); }

		TArray<Unit*>& getEnemyUnits() { return mEnemyUnits; }

		// Helper to spawn enemy (for now)
		void setupRound();
		void cleanupEnemies();
		
		void spawnPVEModule(int playerIndex, int round);
		void spawnPVPModule();
		void teleportUnits(int fromIdx, int toIdx, bool isGhost);
		
		void resolveCombat();
		void restoreUnits();

		int getRound() const { return mRound; }
		RoundManager const& getRoundManager() const { return mRoundManager; }
		UnitDataManager const& getUnitDataManager() const { return mUnitDataManager; }

	private:
		RoundManager    mRoundManager;
		UnitDataManager mUnitDataManager;

		// PlayerBoard   mPlayerBoard; // Now inside Player
		// Player  mPlayer; // Now array
		
		TArray<Player> mPlayers;
		int            mLocalPlayerIndex = 0;
		
		TArray<Unit*> mEnemyUnits;

		BattlePhase mPhase;
		float       mPhaseTimer;
		int         mRound;

		struct MatchPair
		{
			int home;
			int away;
			bool isGhost;
		};
		TArray<MatchPair> mMatches;

		bool        mCombatEnded;

	public:
		struct Projectile
		{
			Vector2 pos;
			Vector2 targetPos; // Or Unit* target? better Position for now as target might die. 
			// But tracking target is better for homing. Uses Unit* target with weak validation check?
			// Simpler: Homming to Unit. If Unit dead, moves to last known pos logic? 
			// For simplicity: Homing to Unit.
			Unit* target;
			float speed;
			float damage;
			Unit* owner;
		};
		
		void addProjectile(Unit* owner, Unit* target, float speed, float damage);
		void updateProjectiles(float dt);
		void renderProjectiles(IGraphics2D& g);

	private:
		TArray<Projectile> mProjectiles;
	};

} // namespace AutoBattler

#endif // ABWorld_h__
