#ifndef ABNetwork_h__
#define ABNetwork_h__

#include "NetGameMode.h"
#include "GameNetPacket.h"
#include "ABAction.h"
#include "ABDefine.h"

namespace AutoBattler
{
	class LevelStage;

	enum
	{
		NP_ACTION = NET_PACKET_CUSTOM_ID,
		NP_FRAME,
		NP_SYNC,
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
				// LogMsg("FramePacket Read: ID=%u Count=%d", frameID, count);
			}
			else
			{
				// LogMsg("FramePacket Write: ID=%u Count=%d", frameID, count);
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

		template < class BufferOP >
		void  operateBuffer(BufferOP& op)
		{
			op & frameID & checksum & playerId & status;
		}
	};

	class ABNetEngine : public INetEngine
	{
	public:

		bool bServerPaused = false;

		ABNetEngine(LevelStage* stage);

		bool build(BuildParam& param) override;

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

		void update(IFrameUpdater& updater, long time) override;

		void addSimuateTime(long dt) { mSimAccumulator += dt; }

		void broadcastFrame(int numFrame);

		void setupInputAI(IPlayerManager& manager) override {}
		void release() override { delete this; }

		void onFramePacket(IComPacket* cp);
		void onPacketSV(IComPacket* cp);
		void onSyncPacket(IComPacket* cp);
		
		// Unused but required signature if mapped? No mapped
		void onPacket(IComPacket* cp) {}

		void sendAction(ActionPort port, ABActionItem const& item);

		LevelStage* mStage;
		ComWorker* mWorker;
		NetWorker* mNetWorker;
	};
}

#endif // ABNetwork_h__
