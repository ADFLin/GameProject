#include "ABNetSession.h"

#include "ABStage.h"
#include "ABWorld.h"
#include "ABBoard.h"
#include "ABUnit.h"
#include "ABNetwork.h"

#include "Net/INetSession.h"
#include "DataStreamBuffer.h"
#include "ComPacket.h"

namespace AutoBattler
{
	ABNetSession::ABNetSession(LevelStage* stage)
		: mStage(stage)
	{
	}

	ABNetSession::~ABNetSession()
	{
		shutdown();
	}

	bool ABNetSession::initialize(INetSession* session)
	{
		mSession = session;
		mIsHost = dynamic_cast<INetSessionHost*>(session) != nullptr;
		
		registerGamePacketHandlers();
		
		// 取得初始種子
		if (mStage && mStage->getWorld())
		{
			mRandomSeed = mStage->getWorld()->getRandomSeed();
		}
		
		LogMsg("ABNetSession initialized (isHost=%d)", mIsHost);
		return true;
	}

	void ABNetSession::shutdown()
	{
		unregisterGamePacketHandlers();
		mSession = nullptr;
	}

	void ABNetSession::update(long time)
	{
		// 更新遊戲網路邏輯
		
		// 處理待加入的玩家
		if (mIsHost && !mPendingJoinPlayers.empty() && mCanAcceptLateJoin)
		{
			for (PlayerId playerId : mPendingJoinPlayers)
			{
				// 發送當前遊戲狀態
				ABGameStatePacket statePacket;
				statePacket.frameId = mCurrentFrame;
				statePacket.randomSeed = mRandomSeed;
				
				if (mStage && mStage->getWorld())
				{
					// statePacket.roundNumber = mStage->getWorld()->getRoundNumber();
					// statePacket.phase = mStage->getWorld()->isInBattle() ? 1 : 0;
				}
				
				serializeGameState(statePacket.stateData);
				
				mSession->sendReliableTo(playerId, &statePacket);
				
				LogMsg("Sent game state to late-join player %d", playerId);
			}
			mPendingJoinPlayers.clear();
		}
	}

	void ABNetSession::serializeGameState(DataStreamBuffer& buffer)
	{
		World* world = getWorld();
		if (!world)
			return;
		
		// 序列化基本狀態
		// buffer << world->getRoundNumber();
		// buffer << world->getPhase();
		buffer << mCurrentFrame;
		buffer << mRandomSeed;
		
		// 序列化每個玩家的棋盤
		uint8 playerCount = AB_MAX_PLAYER_NUM;
		buffer << playerCount;
		
		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			Board* board = world->getBoard(i);
			if (board)
			{
				uint8 hasBoard = 1;
				buffer << hasBoard;
				serializeBoard(buffer, board);
			}
			else
			{
				uint8 hasBoard = 0;
				buffer << hasBoard;
			}
		}
		
		// 序列化商店狀態等其他資料
		// TODO: 根據實際遊戲需求添加
	}

	void ABNetSession::deserializeGameState(DataStreamBuffer& buffer)
	{
		World* world = getWorld();
		if (!world)
			return;
		
		// 反序列化基本狀態
		buffer >> mCurrentFrame;
		buffer >> mRandomSeed;
		
		// 反序列化棋盤
		uint8 playerCount;
		buffer >> playerCount;
		
		for (int i = 0; i < playerCount; ++i)
		{
			uint8 hasBoard;
			buffer >> hasBoard;
			
			if (hasBoard)
			{
				Board* board = world->getBoard(i);
				if (board)
				{
					deserializeBoard(buffer, board);
				}
			}
		}
	}

	uint32 ABNetSession::getCurrentFrame() const
	{
		return mCurrentFrame;
	}

	uint64 ABNetSession::getRandomSeed() const
	{
		return mRandomSeed;
	}

	GameLateJoinInfo ABNetSession::generateLateJoinInfo()
	{
		GameLateJoinInfo info;
		info.randomSeed = mRandomSeed;
		info.currentFrame = mCurrentFrame;
		serializeGameState(info.gameState);
		return info;
	}

	bool ABNetSession::applyLateJoinInfo(GameLateJoinInfo const& info)
	{
		mRandomSeed = info.randomSeed;
		mCurrentFrame = info.currentFrame;
		
		DataStreamBuffer buffer;
		buffer.copy(info.gameState);
		deserializeGameState(buffer);
		
		LogMsg("Applied late join info: frame=%d, seed=%llu", mCurrentFrame, mRandomSeed);
		return true;
	}

	bool ABNetSession::onPlayerJoinRequest(PlayerId playerId)
	{
		// 只在準備階段允許加入
		if (!mCanAcceptLateJoin)
		{
			LogMsg("Rejected late join from player %d: not in prepare phase", playerId);
			return false;
		}
		
		return true;
	}

	void ABNetSession::onPlayerJoined(PlayerId playerId, bool isLateJoin)
	{
		LogMsg("Player %d joined (lateJoin=%d)", playerId, isLateJoin);
		
		if (isLateJoin && mIsHost)
		{
			// 將玩家加入待處理列表，下次 update 時發送遊戲狀態
			mPendingJoinPlayers.push_back(playerId);
		}
		
		// 通知遊戲邏輯
		if (mStage)
		{
			// mStage->onPlayerJoined(playerId, isLateJoin);
		}
	}

	void ABNetSession::onPlayerLeft(PlayerId playerId)
	{
		LogMsg("Player %d left", playerId);
		
		// 通知遊戲邏輯
		if (mStage)
		{
			// mStage->onPlayerLeft(playerId);
		}
	}

	void ABNetSession::registerGamePacketHandlers()
	{
		if (!mSession)
			return;
		
		// 註冊 AutoBattler 特定封包
		mSession->registerPacketHandler(NP_ACTION, 
			[this](PlayerId sender, IComPacket* cp) {
				procActionPacket(sender, cp);
			});
		
		mSession->registerPacketHandler(NP_FRAME,
			[this](PlayerId sender, IComPacket* cp) {
				procFramePacket(sender, cp);
			});
		
		mSession->registerPacketHandler(NP_SYNC,
			[this](PlayerId sender, IComPacket* cp) {
				procSyncPacket(sender, cp);
			});
		
		mSession->registerPacketHandler(ABNP_GAME_STATE,
			[this](PlayerId sender, IComPacket* cp) {
				procGameStatePacket(sender, cp);
			});
	}

	void ABNetSession::unregisterGamePacketHandlers()
	{
		if (!mSession)
			return;
		
		mSession->unregisterPacketHandler(NP_ACTION);
		mSession->unregisterPacketHandler(NP_FRAME);
		mSession->unregisterPacketHandler(NP_SYNC);
		mSession->unregisterPacketHandler(ABNP_GAME_STATE);
	}

	void ABNetSession::sendAction(ActionPort port, ABActionItem const& action)
	{
		ABActionPacket packet;
		packet.port = port;
		packet.item = action;
		
		mSession->sendReliable(&packet);
	}

	void ABNetSession::broadcastRoundEnd(int roundNumber)
	{
		// TODO: 實作回合結束廣播
	}

	void ABNetSession::syncBoardState(int playerId)
	{
		// TODO: 同步特定玩家棋盤
	}

	World* ABNetSession::getWorld() const
	{
		return mStage ? mStage->getWorld() : nullptr;
	}

	void ABNetSession::procActionPacket(PlayerId senderId, IComPacket* cp)
	{
		ABActionPacket* packet = cp->cast<ABActionPacket>();
		
		// 驗證行動
		if (!validateAction(senderId, packet->item))
		{
			LogWarning(0, "Invalid action from player %d", senderId);
			return;
		}
		
		// 轉發給遊戲邏輯
		// if (mStage)
		// {
		//     mStage->handleNetworkAction(senderId, packet->port, packet->item);
		// }
	}

	void ABNetSession::procFramePacket(PlayerId senderId, IComPacket* cp)
	{
		ABFramePacket* packet = cp->cast<ABFramePacket>();
		
		// 更新幀號
		mCurrentFrame = packet->frameID;
		
		// 轉發給遊戲邏輯
		// ...
	}

	void ABNetSession::procSyncPacket(PlayerId senderId, IComPacket* cp)
	{
		ABSyncPacket* packet = cp->cast<ABSyncPacket>();
		
		// 處理同步封包
		// ...
	}

	void ABNetSession::procGameStatePacket(PlayerId senderId, IComPacket* cp)
	{
		ABGameStatePacket* packet = cp->cast<ABGameStatePacket>();
		
		// Client 收到完整遊戲狀態（Late Join）
		if (!mIsHost)
		{
			mCurrentFrame = packet->frameId;
			mRandomSeed = packet->randomSeed;
			
			deserializeGameState(packet->stateData);
			
			LogMsg("Received game state: frame=%d, seed=%llu", mCurrentFrame, mRandomSeed);
		}
	}

	void ABNetSession::serializeBoard(DataStreamBuffer& buffer, Board* board)
	{
		// TODO: 實作棋盤序列化
		// 包括：格子狀態、單位列表等
	}

	void ABNetSession::deserializeBoard(DataStreamBuffer& buffer, Board* board)
	{
		// TODO: 實作棋盤反序列化
	}

	void ABNetSession::serializeUnit(DataStreamBuffer& buffer, Unit* unit)
	{
		// TODO: 實作單位序列化
		// 包括：類型、等級、位置、裝備等
	}

	void ABNetSession::deserializeUnit(DataStreamBuffer& buffer, Unit*& unit)
	{
		// TODO: 實作單位反序列化
	}

	bool ABNetSession::validateAction(PlayerId playerId, ABActionItem const& action)
	{
		// TODO: 驗證行動合法性
		// 例如：檢查是否是該玩家的回合、行動是否有效等
		return true;
	}

}  // namespace AutoBattler
