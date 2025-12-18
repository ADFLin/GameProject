#include "ABPCH.h"
#include "ABNetwork.h"
#include "ABStage.h"
#include "ABBot.h"

namespace AutoBattler
{

	ABNetEngine::ABNetEngine(LevelStage* stage)
		: mStage(stage)
	{
	}

	bool ABNetEngine::build(BuildParam& param)
	{
		LogMsg("NetEngine Build: Worker=%p", param.worker);
		mWorker = param.worker;
		mNetWorker = param.netWorker;

		mNetWorker->getPacketDispatcher().setUserFunc<ABFramePacket>(this, &ABNetEngine::onFramePacket);
		mNetWorker->getPacketDispatcher().setUserFunc<ABSyncPacket>(this, &ABNetEngine::onSyncPacket); // Register for both

		if (mNetWorker->isServer())
		{
			mNetWorker->getPacketDispatcher().setUserFunc<ABActionPacket>(this, &ABNetEngine::onPacketSV);
		}

		mSimAccumulator = 0;
		mAccTime = 0;
		mNextFrameID = 0;
		mLastReceivedFrame = 0;
		mSyncTimer = 0;
		mConnectionTimeout = 0;

		std::fill(std::begin(mPlayerTimeouts), std::end(mPlayerTimeouts), 0);
		std::fill(std::begin(mPlayerTimeoutStatus), std::end(mPlayerTimeoutStatus), false);

		return true;
	}

	void ABNetEngine::update(IFrameUpdater& updater, long time)
	{
		bool bIsPaused = false;

		if (mNetWorker->isServer())
		{
			// Server: Check Clients
			int localPlayerId = mStage->getWorld().getLocalPlayerIndex();
			for (auto it = mNetWorker->getPlayerManager()->createIterator(); it; ++it)
			{
				GamePlayer* player = it.getElement();
				if (player->getType() != PT_PLAYER)
					continue;

				int id = player->getActionPort();
				if (id == localPlayerId)
					continue;

				mPlayerTimeouts[id] += time;
				if (mPlayerTimeouts[id] > TimeoutSpan)
				{
					if (!mPlayerTimeoutStatus[id])
					{
						mPlayerTimeoutStatus[id] = true;
						if (OnTimeout)
							OnTimeout(id, true);
					}
				}
				else
				{
					if (mPlayerTimeoutStatus[id])
					{
						mPlayerTimeoutStatus[id] = false;
						if (OnTimeout)
							OnTimeout(id, false);
					}
				}
			}

			bool bAnyClientTimeout = false;
			for (auto it = mNetWorker->getPlayerManager()->createIterator(); it; ++it)
			{
				GamePlayer* player = it.getElement();
				if (player->getType() != PT_PLAYER)
					continue;

				int id = player->getActionPort();
				if (id == localPlayerId)
					continue;

				if (mPlayerTimeoutStatus[id])
				{
					bAnyClientTimeout = true;
					break;
				}
			}

			if (bAnyClientTimeout)
			{
				bIsPaused = true;

				// Send Timeout Packet
				if (!bServerPaused)
				{
					bServerPaused = true;
					ABSyncPacket packet;
					packet.frameID = mNextFrameID;
					packet.checksum = 0;
					packet.playerId = 0; // TODO: Find who timed out?
					packet.status = 1;
					mNetWorker->sendTcpCommand(&packet);
					mSyncTimer = 0;
				}
				else
				{
					mSyncTimer += time;
					if (mSyncTimer >= SyncInterval)
					{
						mSyncTimer = 0;
						ABSyncPacket packet;
						packet.frameID = mNextFrameID;
						packet.checksum = 0;
						packet.playerId = 0;
						packet.status = 1;
						mNetWorker->sendTcpCommand(&packet);
					}
				}
			}
			else
			{
				if (bServerPaused)
				{
					bServerPaused = false;
					// Send Restore Packet
					ABSyncPacket packet;
					packet.frameID = mNextFrameID;
					packet.checksum = 0;
					packet.playerId = 0;
					packet.status = 2;
					mNetWorker->sendTcpCommand(&packet);
				}

				mAccTime += time;
				if (mAccTime >= TickRate)
				{
					int numFrame = mAccTime / TickRate;
					mAccTime -= numFrame * TickRate;
					broadcastFrame(numFrame);
				}
			}
		}
		else
		{

			// Client: Check Server
			mConnectionTimeout += time;
			if (mConnectionTimeout > TimeoutSpan)
			{
				if (!bIsTimeout)
				{
					bIsTimeout = true;
					if (OnTimeout)
						OnTimeout(mStage->getWorld().getLocalPlayerIndex(), true);
				}
				// Client returns? Existing code returned. But maybe we shouldn't return if we want to try sending heartbeat?
				// But if connection lost, sending is futile.
				// Existing code: return;
			}

			if (bIsTimeout && mConnectionTimeout <= TimeoutSpan)
			{
				bIsTimeout = false;
				if (OnTimeout)
					OnTimeout(mStage->getWorld().getLocalPlayerIndex(), false);
			}

			if (bIsTimeout)
				return;


			// Client: Send Checksum/Heartbeat
			mSyncTimer += time;
			if (mSyncTimer >= SyncInterval)
			{
				mSyncTimer = 0;

				ABSyncPacket packet;
				packet.frameID = mLastReceivedFrame;
				packet.checksum = 0; // TODO: Implement World Hash
				packet.playerId = mStage->getWorld().getLocalPlayerIndex();
				mWorker->sendTcpCommand(&packet);
			}
		}

		long deltaTime = mStage->getTickTime();
		if (!bIsPaused)
			mLocalTimeAccumulator += time;

		int maxSteps = mLocalTimeAccumulator / deltaTime;
		int steps = 0;
		while (mSimAccumulator >= deltaTime && steps < maxSteps)
		{
			mSimAccumulator -= deltaTime;
			mLocalTimeAccumulator -= deltaTime;

			mStage->runLogic(float(deltaTime) / 1000.0f);
			steps++;
		}
	}

	void ABNetEngine::broadcastFrame(int numFrame)
	{
		CHECK(mNetWorker->isServer());

		// Determinism: Execute in the exact same order as sent to clients
		for (auto const& data : mPendingActions)
		{
			mStage->executeAction(data.port, data.item);
		}

		addSimuateTime(TickRate * numFrame);

		ABFramePacket packet;
		packet.frameID = mNextFrameID++;
		packet.actions = std::move(mPendingActions);
		packet.numFrame = numFrame;
		mNetWorker->sendTcpCommand(&packet, WSF_IGNORE_LOCAL);
	}

	void ABNetEngine::onFramePacket(IComPacket* cp)
	{
		mConnectionTimeout = 0;

		ABFramePacket* packet = cp->cast<ABFramePacket>();
		mLastReceivedFrame = packet->frameID;

		long deltaTime = mStage->getTickTime();
		while (mSimAccumulator >= deltaTime)
		{
			mSimAccumulator -= deltaTime;
			mLocalTimeAccumulator -= deltaTime;
			mStage->runLogic(float(deltaTime) / 1000.0f);
		}

		LogMsg("Client Run Frame %u : %d", packet->frameID, packet->actions.size());
		for (auto const& action : packet->actions)
		{
			mStage->executeAction(action.port, action.item);
		}

		// Step Simulation (Client)
		addSimuateTime(TickRate * packet->numFrame);
	}

	void ABNetEngine::onPacketSV(IComPacket* cp)
	{
		ABActionPacket* packet = cp->cast<ABActionPacket>();

		if (packet->port < AB_MAX_PLAYER_NUM)
			mPlayerTimeouts[packet->port] = 0;

		// Buffer for Frame
		mPendingActions.push_back({ packet->port, packet->item });
	}

	void ABNetEngine::onSyncPacket(IComPacket* cp)
	{
		ABSyncPacket* packet = cp->cast<ABSyncPacket>();

		if (mNetWorker->isServer())
		{
			// Server Logic: Heartbeat from Client
			if (packet->playerId < AB_MAX_PLAYER_NUM)
				mPlayerTimeouts[packet->playerId] = 0;
		}
		else
		{
			// Client Logic: Status from Server
			mConnectionTimeout = 0; // Server is alive

			if (packet->status == 1) // Timeout
			{
				if (OnTimeout)
					OnTimeout(packet->playerId, true);
			}
			else if (packet->status == 2) // Restore
			{
				if (OnTimeout)
					OnTimeout(packet->playerId, false);
			}
		}

		//LogMsg("Client Sync: Frame %u Hash %u", packet->frameID, packet->checksum);

		// TODO: Check for divergence
	}

	void ABNetEngine::sendAction(ActionPort port, ABActionItem const& item)
	{
		if (mNetWorker->isServer())
		{
			// Host Action: Do NOT Exec Immediate. Buffer for Frame.
			// mStage->executeAction(port, item);
			mPendingActions.push_back({ port, item });
		}
		else
		{
			// Client: Send Only
			ABActionPacket packet;
			packet.port = port;
			packet.item = item;
			mWorker->sendTcpCommand(&packet);
		}
	}


} // namespace AutoBattler
