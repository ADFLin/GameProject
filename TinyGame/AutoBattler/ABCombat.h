#ifndef ABCombat_h__
#define ABCombat_h__

#include "ABWorld.h"
#include "ABAction.h"
#include "ABCombatDefs.h"

#include "Async/AsyncWork.h"

#include <functional>
#include <queue>
#include <atomic>

namespace AutoBattler
{


	// Unit snapshot (for deep copy to combat thread)
	struct UnitSnapshot
	{
		int   unitID;
		int   unitTypeID;
		UnitLocation location;
		UnitStats stats;
		UnitTeam team;

		static UnitSnapshot fromUnit(Unit const& unit);
	
		// Create a new Unit from this snapshot (for combat simulation)
		Unit* toUnit() const;


		template<class OP>
		void serialize(OP& op)
		{
			op & unitID & unitTypeID & location & stats & team;
		}
	};



	// Combat result
	struct CombatResult
	{
		uint32 combatID;
		int homePlayerIndex;
		int awayPlayerIndex;
		int winnerIndex;     // -1 for draw
		int homeDamage;
		int awayDamage;
		float duration;
		TArray<CombatEvent> allEvents;  // All event records
	};

	// Thread-safe event queue
	class CombatEventQueue
	{
	public:
		void push(CombatEventBatch batch);
		bool tryPop(CombatEventBatch& outBatch);
		bool empty() const;
		void clear();

	private:
		mutable Mutex mMutex;
		std::queue<CombatEventBatch> mQueue;
	};

	// Combat work (implements IQueuedWork interface for QueueThreadPool)
	class CombatWork : public IQueuedWork
	{
	public:
		CombatWork(
			uint32 combatID,
			int homeIndex, 
			int awayIndex,
			TArray<UnitSnapshot> const& homeUnits,
			TArray<UnitSnapshot> const& awayUnits,
			CombatEventQueue* eventQueue,
			std::function<void(CombatResult)> onComplete);

		virtual ~CombatWork() = default;

		// IQueuedWork interface
		virtual void executeWork() override;
		virtual void release() override { delete this; }

	private:
		void simulateAndStream();

		uint32 mCombatID;
		int mHomePlayerIndex;
		int mAwayPlayerIndex;
		TArray<UnitSnapshot> mHomeUnits;
		TArray<UnitSnapshot> mAwayUnits;

		CombatEventQueue* mEventQueue;
		std::function<void(CombatResult)> mOnComplete;
	};

	// Combat manager (uses QueueThreadPool)
	class CombatManager
	{
	public:
		CombatManager();
		~CombatManager();

		void initialize(int numThreads = 4);
		void shutdown();

		// Submit combat task
		uint32 submitCombat(
			int homeIndex, 
			int awayIndex,
			TArray<UnitSnapshot> const& homeUnits,
			TArray<UnitSnapshot> const& awayUnits,
			std::function<void(CombatResult)> onComplete);

		// Poll event batches (called from main thread)
		TArray<CombatEventBatch> pollEventBatches();

		// Check if combats are in progress
		bool hasPendingCombats() const;

	private:
		QueueThreadPool mThreadPool;
		CombatEventQueue mEventQueue;
		std::atomic<uint32> mNextCombatID{1};
		std::atomic<int> mActiveCombats{0};
	};

} // namespace AutoBattler

#endif // ABCombat_h__
