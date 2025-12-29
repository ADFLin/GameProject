#ifndef ABNetwork_h__
#define ABNetwork_h__

#include "NetGameMode.h"
#include "GameNetPacket.h"
#include "ABAction.h"
#include "ABDefine.h"
#include "ABCombat.h"
#include "ABCombatReplay.h"

#include "ABCombat.h"
#include "ABCombatReplay.h"

#include <mutex>
#include <unordered_map>
#include <memory>

namespace AutoBattler
{
	class LevelStage;
	class CombatReplayManager;

	enum
	{
		NP_ACTION = NET_PACKET_CUSTOM_ID,
		NP_FRAME,
		NP_SYNC,
		NP_ACTION_REJECT,  // Server-side action rejection notification
		NP_COMBAT_START,   // Combat start notification
		NP_COMBAT_EVENT,   // Combat event streaming
		NP_COMBAT_END,     // Combat end result
		NP_SHOP_UPDATE,    // Shop content sync
	};

	class ABShopUpdatePacket : public GamePacketT<ABShopUpdatePacket, NP_SHOP_UPDATE>
	{
	public:
		uint8 playerId;
		TArray<int> shopList; // Should be 5 integers

		template < class BufferOP >
		void operateBuffer(BufferOP& op)
		{
			op & playerId & shopList;
		}
	};

	class ABActionPacket : public GamePacketT<ABActionPacket, NP_ACTION>
	{
	public:
		ActionPort port;
		ABActionItem item;

		template < class BufferOP >
		void  operateBuffer(BufferOP& op)
		{
			op & port & item;
			if (BufferOP::IsTake)
			{
				LogMsg("Packet Read: Port=%d Type=%d", port, item.type);
			}
			else
			{
				LogMsg("Packet Write: Port=%d Type=%d", port, item.type);
			}
		}
	};

	struct ABActionData
	{
		ActionPort   port;
		ABActionItem item;
	};

	class ABFramePacket : public GamePacketT<ABFramePacket, NP_FRAME>
	{
	public:

		uint32 frameID;
		uint32 numFrame;
		TArray<ABActionData> actions;

		template < class BufferOP >
		void  operateBuffer(BufferOP& op)
		{
			op & frameID & numFrame;
			if (BufferOP::IsTake)
			{
				uint8 count;
				op & count;
				actions.resize(count);
			}
			else
			{
				uint8 count = (uint8)actions.size();
				op & count;
			}

			for (int i = 0; i < actions.size(); ++i)
			{
				op & actions[i].port & actions[i].item;
			}

			if (BufferOP::IsTake)
			{
				// LogMsg("FramePacket Read: ID=%u Count=%d", frameID, actions.size());
			}
			else
			{
				// LogMsg("FramePacket Write: ID=%u Count=%d", frameID, actions.size());
			}
		}
	};

	class ABSyncPacket : public GamePacketT<ABSyncPacket, NP_SYNC>
	{
	public:
		uint32 frameID;
		uint32 checksum;
		uint8  playerId;
		uint8  status; // 0: Heartbeat, 1: Timeout, 2: Restore
		
		// Detailed Validation
		int round;
		int phase;
		int playerGold;
		int playerHP;
		int shopHash;
		float phaseTimer;

		template < class BufferOP >
		void  operateBuffer(BufferOP& op)
		{
			op & frameID & checksum & playerId & status;
			op & round & phase & playerGold & playerHP & shopHash & phaseTimer;
		}
	};

	// Action validation failure notification
	class ABActionRejectPacket : public GamePacketT<ABActionRejectPacket, NP_ACTION_REJECT>
	{
	public:
		ActionPort port;
		uint8 actionType;
		uint8 errorCode;

		template <class BufferOP>
		void operateBuffer(BufferOP& op)
		{
			op & port & actionType & errorCode;
		}
	};

	// Combat start notification
	class ABCombatStartPacket : public GamePacketT<ABCombatStartPacket, NP_COMBAT_START>
	{
	public:
		uint32 combatID;
		int homeIndex;
		int awayIndex;
		TArray<UnitSnapshot> homeUnits;
		TArray<UnitSnapshot> awayUnits;

		template <class BufferOP>
		void operateBuffer(BufferOP& op)
		{
			op & combatID & homeIndex & awayIndex;
			
			if (BufferOP::IsTake)
			{
				uint8 homeCount, awayCount;
				op & homeCount & awayCount;
				homeUnits.resize(homeCount);
				awayUnits.resize(awayCount);
			}
			else
			{
				uint8 homeCount = (uint8)homeUnits.size();
				uint8 awayCount = (uint8)awayUnits.size();
				op & homeCount & awayCount;
			}

			for (auto& unit : homeUnits)
				unit.serialize(op);
			for (auto& unit : awayUnits)
				unit.serialize(op);
		}
	};

	// Combat event streaming
	class ABCombatEventPacket : public GamePacketT<ABCombatEventPacket, NP_COMBAT_EVENT>
	{
	public:
		uint32 combatID;
		uint32 batchIndex;
		TArray<CombatEvent> events;

		template <class BufferOP>
		void operateBuffer(BufferOP& op)
		{
			op & combatID & batchIndex;

			if (BufferOP::IsTake)
			{
				uint8 count;
				op & count;
				events.resize(count);
			}
			else
			{
				uint8 count = (uint8)events.size();
				op & count;
			}

			for (auto& event : events)
				event.serialize(op);
		}
	};

	// Combat end result
	class ABCombatEndPacket : public GamePacketT<ABCombatEndPacket, NP_COMBAT_END>
	{
	public:
		uint32 combatID;
		int homePlayerIndex;
		int awayPlayerIndex;
		int winnerIndex;  // -1 for draw
		int homeDamage;
		int awayDamage;
		float duration;

		template <class BufferOP>
		void operateBuffer(BufferOP& op)
		{
			op & combatID & homePlayerIndex & awayPlayerIndex 
			   & winnerIndex & homeDamage & awayDamage & duration;
		}
	};


	class DedicatedWorld : public World, public IFrameUpdater
	{
	public:
		DedicatedWorld();
		~DedicatedWorld() = default;

		// IFrameUpdater interface
		void tick() override;
		void updateFrame(int frame) override;

		// Configuration
		void setTickTime(long tickTimeMs) { mTickTime = tickTimeMs; }
		long getTickTime() const { return mTickTime; }

	private:
		long mTickTime = 33;  // Default ~30 FPS
		int  mCurrentFrame = 0;
	};

	class ABNetEngine : public INetEngine
	{
	public:
		
		std::mutex mResultMutex;
		TArray<CombatResult> mCombatResults;

		bool bServerPaused = false;

		// Default constructor for dedicated server mode (no Stage)
		ABNetEngine();
		
		// Constructor with Stage for normal mode
		ABNetEngine(LevelStage* stage);
		
		// Set Stage after construction (for normal mode)
		void setStage(LevelStage* stage) { mStage = stage; }

		bool build(BuildParam& param) override;
		
		// INetEngine: Get dedicated updater for headless mode
		IFrameUpdater* getDedicatedUpdater() override;

		TArray<ABActionData> mPendingActions;

		std::function< void(int playerId, bool bLost) > OnTimeout;
		bool bIsTimeout = false;

		long mPlayerTimeouts[AB_MAX_PLAYER_NUM];
		bool mPlayerTimeoutStatus[AB_MAX_PLAYER_NUM];

		long  mSimAccumulator = 0;
		long  mAccTime = 0;
		long  mLocalTimeAccumulator = 0;
		static const long TickRate = 30;
		uint32 mNextFrameID = 0;

		long mSyncTimer = 0;
		uint32 mLastReceivedFrame = 0;
		static const long SyncInterval = 1000;
		static const long TimeoutSpan = 4000;
		long mConnectionTimeout = 0;

		bool bReplayPlaying;

		void update(IFrameUpdater& updater, long time) override;

		void addSimuateTime(long dt) { mSimAccumulator += dt; }

		void broadcastFrame(int numFrame);

		void setupInputAI(IPlayerManager& manager, ActionProcessor& processor) override {}
		void release() override { delete this; }
		
		// Configure level settings for network sync (dedicated server mode)
		void configLevelSetting(GameLevelInfo& info) override;

		void onFramePacket(IComPacket* cp);
		void onPacketSV(IComPacket* cp);
		void onSyncPacket(IComPacket* cp);
		
		// Unused but required signature if mapped? No mapped
		void onPacket(IComPacket* cp) {}

		void sendAction(ActionPort port, ABActionItem const& item);

		void onCombatPhaseStart();
		void broadcastCombatEvents();
		void onCombatComplete(CombatResult result);

		// Client-side combat packet handlers
		void onCombatStartPacket(IComPacket* cp);
		void onCombatEventPacket(IComPacket* cp);
		void onCombatEndPacket(IComPacket* cp);
		void onShopUpdatePacket(IComPacket* cp);

		LevelStage* mStage;
		ComWorker* mWorker;
		NetWorker* mNetWorker;

		bool mIsDedicatedServer = false;
		std::unique_ptr<DedicatedWorld> mDedicatedWorld;
		
		// Helper to get the authoritative World (Dedicated or Stage's)
		World& GetWorld();

		CombatManager mCombatManager;
		CombatReplayManager mReplayManager;
		TArray<uint32> mPendingCombatIDs;
		
		std::unordered_map<uint32, CombatResult> mCompletedCombats;
		
		struct PendingCombatResult
		{
			int homePlayerIndex;
			int awayPlayerIndex;
			int winnerIndex;
			int homeDamage;
			int awayDamage;
			float duration;
		};
		std::unordered_map<uint32, PendingCombatResult> mPendingCombatResults;
		
	private:
		void sendPendingCombatEndPackets();        // Server: Send CombatEnd after events
		void applyCompletedReplayResults();        // Client: Apply results after replay
	};
}

#endif // ABNetwork_h__
