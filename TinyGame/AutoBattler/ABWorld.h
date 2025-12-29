#ifndef ABWorld_h__
#define ABWorld_h__

#include "ABPlayerBoard.h"
#include "ABPlayer.h"
#include "ABUnit.h"
#include "ABRound.h"
#include "ABCombatDefs.h" // Includes CombatEvent definition

#include "DataStructure/Array.h"
#include <functional>


namespace AutoBattler
{
	enum class BattlePhase
	{
		None,
		Preparation,
		Combat,
		End,
	};

	struct ActionError
	{
		enum Code
		{
			NONE = 0,
			INSUFFICIENT_GOLD,
			INVALID_POSITION,
			SHOP_SLOT_EMPTY,
			BENCH_FULL,
			BOARD_FULL,
			PHASE_MISMATCH,
			INVALID_PLAYER,
			NO_UNIT,
			INVALID_SOURCE,
			INVALID_DESTINATION,
		};

		Code code = NONE;
		const char* message = "";
	};

	struct ABActionItem;  // Forward declaration

	class World
	{
	public:
		World();
		~World();

		void init();
		void cleanup();
		void tick(float dt);
		void render(IGraphics2D& g);
		
		void restart();
		
		// Game Logic
		void changePhase(BattlePhase phase);
		BattlePhase getPhase() const { return mPhase; }
		float getPhaseTimer() const { return mPhaseTimer; }
		void  setPhaseTimer(float time) { mPhaseTimer = time; }

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
		
		// PVP Match Generation & Unit Teleportation (Refactored)
		void generateMatches();           // Generate match pairings only
		void teleportMatchedUnits();      // Teleport units based on existing matches
		void teleportUnits(int fromIdx, int toIdx, bool isGhost); // Low-level helper
		void teleportNetworkGhosts();
		
		void resolveCombat();
		void restoreUnits();
		
		void setRound(int round) { mRound = round; }
		int getRound() const { return mRound; }
		RoundManager const& getRoundManager() const { return mRoundManager; }
		UnitDataManager const& getUnitDataManager() const { return mUnitDataManager; }

		// Match pairing structure
		struct MatchPair
		{
			int home;
			int away;
			bool isGhost;
		};

		// Get current round match pairings
		TArray<MatchPair> const& getMatches() const { return mMatches; }

		// ========================================================================
		// Combat Replay Support
		// ========================================================================

		// Check if currently playing a combat replay
		bool isPlayingReplay() const { return mPlayingReplay; }

		// Enable replay mode (disables AI, only applies events)
		void setReplayMode(bool enabled);
		void setAutoResolveCombat(bool enable) { mAutoResolveCombat = enable; }
		bool isAutoResolveCombat() const { return mAutoResolveCombat; }

		// Network Mode Control
		void setNetworkMode(bool enabled) { mbNetworkMode = enabled; }
		bool isNetworkMode() const { return mbNetworkMode; }
		
		// Authority Control: Explicitly set if this World is authoritative (Server/Host)
		void setAuthority(bool auth) { mbIsAuthority = auth; }
		bool isAuthority() const { return mbIsAuthority; }

		// ========================================================================
		// Combat Action Application (shared by Units and Replay)
		// These functions encapsulate combat logic so both normal gameplay
		// and replay can use the same implementation
		// ========================================================================

		void applyUnitMove(Unit& unit, Vec2i const& moveTo, bool immediate = false);
		void applyUnitAttack(Unit& attacker, Unit& target);
		void applyUnitDamage(Unit& target, float damage);
		void applyUnitHeal(Unit& target, float healAmount);
		void applyUnitBuff(Unit& target, int buffType, float buffValue, float duration);
		void applyUnitCastSkill(Unit& caster, int skillID, Unit* target = nullptr);
		void applyUnitManaGain(Unit& unit, float manaAmount);

		// Apply combat result (damage, rewards) to players
		// homeAlive/awayAlive: number of surviving units (-1 if not applicable)
		void applyCombatResult(int homePlayerIndex, int awayPlayerIndex, 
		                       int homeAlive, int awayAlive, bool isPVE = false);

		// Combat Event Recording
		void setEventRecording(bool enable) { mRecordingEvents = enable; }
		bool isRecordingEvents() const { return mRecordingEvents; }
		float getSimulationTime() const { return mSimulationTime; }
		void setSimulationTime(float time) { mSimulationTime = time; }
		TArray<struct CombatEvent> const& getRecordedEvents() const { return mRecordedEvents; }
		void clearRecordedEvents() { mRecordedEvents.clear(); }
		void recordEvent(struct CombatEvent const& event);


		bool validateAction(int playerIndex, ABActionItem const& action, ActionError* outError = nullptr);


		std::function< void (BattlePhase) > onPhaseChanged;

	private:
		bool validateBuyUnit(Player const& player, struct ActionBuyUnit const& action, ActionError* outError);
		bool validateSellUnit(Player const& player, struct ActionSellUnit const& action, ActionError* outError);
		bool validateRefreshShop(Player const& player, ActionError* outError);
		bool validateLevelUp(Player const& player, ActionError* outError);
		bool validateDeployUnit(Player const& player, struct ActionDeployUnit const& action, ActionError* outError);
		bool validateRetractUnit(Player const& player, struct ActionRetractUnit const& action, ActionError* outError);

	public:
		int  getPlayerCount() const { return (int)mPlayers.size(); }
		bool isValidPlayer(int index) const { return index >= 0 && index < mPlayers.size(); }

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

		TArray<MatchPair> mMatches;

		bool        mCombatEnded;

		// Combat event recording
		bool                  mRecordingEvents = false;
		float                 mSimulationTime = 0.0f;
		TArray<CombatEvent>   mRecordedEvents;

		// Combat replay mode flag (when true, Units skip AI logic)
		bool        mPlayingReplay = false;
		bool        mAutoResolveCombat = true;
		bool        mbNetworkMode = false;  // If true, skip local unit teleportation in PVP
		bool        mbIsAuthority = true;   // Default true (Standalone), set to false for Client in Net Mode
		
		bool        mUseLocalRandom = false;
		unsigned int mRandomSeed = 1;
		int         mNextUnitID = 1;

	public:
		int allocUnitID() { return mNextUnitID++; }
		// Random Number Generation
		void setUseLocalRandom(bool useLocal) { mUseLocalRandom = useLocal; }
		void setRandomSeed(unsigned int seed) { mRandomSeed = seed; }
		unsigned int getRandomSeed() const { return mRandomSeed; }
		int getRand();

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
