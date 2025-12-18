#pragma once
#ifndef ABNetSession_H_INCLUDED
#define ABNetSession_H_INCLUDED

#include "Net/IGameNetSession.h"
#include "ABDefine.h"
#include "ABAction.h"

namespace AutoBattler
{
	class LevelStage;
	class World;
	class Board;
	struct ABActionItem;

	/**
	 * @brief AutoBattler 遊戲網路會話
	 * 
	 * 實作回合制自走棋遊戲的網路邏輯：
	 * - 回合同步
	 * - 棋盤狀態同步
	 * - Late Join 支援（在準備階段加入）
	 */
	class ABNetSession : public IGameNetSession
	{
	public:
		ABNetSession(LevelStage* stage);
		~ABNetSession();

		//========================================
		// IGameNetSession
		//========================================
		bool initialize(INetSession* session) override;
		void shutdown() override;
		void update(long time) override;

		char const* getGameName() const override { return "AutoBattler"; }
		EGameSyncMode getSyncMode() const override { return EGameSyncMode::StateSync; }

		void serializeGameState(DataStreamBuffer& buffer) override;
		void deserializeGameState(DataStreamBuffer& buffer) override;

		uint32 getCurrentFrame() const override;
		uint64 getRandomSeed() const override;

		//========================================
		// Late Join
		//========================================
		bool supportsLateJoin() const override { return true; }
		GameLateJoinInfo generateLateJoinInfo() override;
		bool applyLateJoinInfo(GameLateJoinInfo const& info) override;

		bool onPlayerJoinRequest(PlayerId playerId) override;
		void onPlayerJoined(PlayerId playerId, bool isLateJoin) override;
		void onPlayerLeft(PlayerId playerId) override;

		//========================================
		// 封包處理
		//========================================
		void registerGamePacketHandlers() override;
		void unregisterGamePacketHandlers() override;

		//========================================
		// 遊戲特定功能
		//========================================
		
		// 發送玩家行動
		void sendAction(ActionPort port, ABActionItem const& action);
		
		// 廣播回合結束
		void broadcastRoundEnd(int roundNumber);
		
		// 同步棋盤狀態
		void syncBoardState(int playerId);

		// 取得 World（遊戲狀態）
		World* getWorld() const;

	private:
		// 封包處理
		void procActionPacket(PlayerId senderId, IComPacket* cp);
		void procFramePacket(PlayerId senderId, IComPacket* cp);
		void procSyncPacket(PlayerId senderId, IComPacket* cp);
		void procGameStatePacket(PlayerId senderId, IComPacket* cp);

		// 序列化輔助
		void serializeBoard(DataStreamBuffer& buffer, Board* board);
		void deserializeBoard(DataStreamBuffer& buffer, Board* board);
		void serializeUnit(DataStreamBuffer& buffer, class Unit* unit);
		void deserializeUnit(DataStreamBuffer& buffer, class Unit*& unit);

		// 驗證行動
		bool validateAction(PlayerId playerId, ABActionItem const& action);

		LevelStage* mStage;
		
		// 同步狀態
		uint32 mCurrentFrame = 0;
		uint64 mRandomSeed = 0;
		
		// Late Join 相關
		bool mCanAcceptLateJoin = true;  // 只在準備階段允許加入
		TArray<PlayerId> mPendingJoinPlayers;
	};

	/**
	 * @brief 完整遊戲狀態封包（用於 Late Join）
	 */
	class ABGameStatePacket : public IComPacket
	{
		COM_PACKET_DECL(ABGameStatePacket)
	public:
		uint32 frameId;
		uint64 randomSeed;
		uint8  roundNumber;
		uint8  phase;  // 0: Prepare, 1: Battle
		DataStreamBuffer stateData;

		template<class BufferOP>
		void operateBuffer(BufferOP& op)
		{
			op & frameId & randomSeed & roundNumber & phase;
			
			if (BufferOP::IsTake)
			{
				uint32 dataSize;
				op & dataSize;
				stateData.resize(dataSize);
				op.take(stateData.getData(), dataSize);
			}
			else
			{
				uint32 dataSize = (uint32)stateData.getAvailableSize();
				op & dataSize;
				op.fill(stateData.getData(), dataSize);
			}
		}
	};

	// 封包 ID
	enum ABNetPacketId
	{
		ABNP_GAME_STATE = NET_PACKET_CUSTOM_ID + 100,  // 避免與現有封包衝突
		ABNP_LATE_JOIN_REQUEST,
		ABNP_LATE_JOIN_RESPONSE,
	};

}  // namespace AutoBattler

#endif // ABNetSession_H_INCLUDED
