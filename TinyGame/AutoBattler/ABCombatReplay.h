#ifndef ABCombatReplay_h__
#define ABCombatReplay_h__

#include "ABCombat.h"
#include <queue>
#include <unordered_map>

namespace AutoBattler
{
	class World;
	class Unit;

	// Combat replay structure for client-side event playback
	struct CombatReplay
	{
		uint32 combatID;
		int homePlayerIndex;
		int awayPlayerIndex;

		// Event queue for playback
		std::queue<CombatEvent> eventQueue;

		// Playback state
		float playbackTime = 0;
		bool ended = false;
		int winnerIndex = -1;
		float playbackSpeed = 1.0f;
	};

	// Combat Replay Manager - handles client-side combat visualization
	class CombatReplayManager
	{
	public:
		CombatReplayManager() : mWorld(nullptr) {}
		~CombatReplayManager();

		// Set the World reference for applying effects
		void setWorld(World* world) { mWorld = world; }

		// Setup a new combat replay
		void setupReplay(
			uint32 combatID,
			int homeIndex, int awayIndex,
			TArray<UnitSnapshot> const& homeUnits,
			TArray<UnitSnapshot> const& awayUnits);

		// Enqueue events for a specific combat
		void enqueueEvents(uint32 combatID, TArray<CombatEvent> const& events);

		// Mark combat as ended (but may still have events to play)
		void markEnded(uint32 combatID, int winnerIndex);

		// Update all active replays
		void update(float dt);

		// Get active replay for a combat (returns nullptr if not found)
		CombatReplay* getReplay(uint32 combatID);

		// Check if any replays are active
		bool hasActiveReplays() const { return !mActiveReplays.empty(); }

		// Clear all replays
		void clearAll();

	private:
		std::unordered_map<uint32, CombatReplay> mActiveReplays;
		World* mWorld;

		// Dispatch event to appropriate World apply function
		void applyCombatEvent(CombatEvent const& event);

		// Helper: Find unit by ID in the world
		Unit* findUnitByID(int unitID);
	};

} // namespace AutoBattler

#endif // ABCombatReplay_h__
