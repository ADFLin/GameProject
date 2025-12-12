#include "ABPCH.h"

#include "ABStage.h"
#include "RenderUtility.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"
#include "GameGUISystem.h"
#include "ABDefine.h"
#include "ABAction.h"
#include "NetGameMode.h"
#include "CSyncFrameManager.h"
#include "GameWorker.h"
#include "ABBot.h"

namespace AutoBattler
{
	class ABActionPacket : public IComPacket
	{
	public:
		enum { PID = 50 };
		ABActionPacket() : IComPacket(PID) {}

		ActionPort port;
		ABActionItem item;

		void doRead(SocketBuffer& buffer) override
		{
			buffer.take(port);
			buffer.take(item);
			LogMsg("Packet Read: Port=%d Type=%d", port, item.type);
		}
		void doWrite(SocketBuffer& buffer) override
		{
			buffer.fill(port);
			buffer.fill(item);
			LogMsg("Packet Write: Port=%d Type=%d", port, item.type);
		}
	};

	struct ABActionData
	{
		ActionPort   port;
		ABActionItem item;
	};

	class ABFramePacket : public IComPacket
	{
	public:
		enum { PID = 51 };
		ABFramePacket() : IComPacket(PID) {}

		uint32 frameID;
		std::vector<ABActionData> actions;

		void doRead(SocketBuffer& buffer) override
		{
			buffer.take(frameID);
			uint8 count;
			buffer.take(count);
			actions.resize(count);
			for (int i = 0; i < count; ++i)
			{
				buffer.take(actions[i].port);
				buffer.take(actions[i].item);
			}
			LogMsg("FramePacket Read: ID=%u Count=%d", frameID, count);
		}
		void doWrite(SocketBuffer& buffer) override
		{
			buffer.fill(frameID);
			uint8 count = (uint8)actions.size();
			buffer.fill(count);
			for (int i = 0; i < count; ++i)
			{
				buffer.fill(actions[i].port);
				buffer.fill(actions[i].item);
			}
			LogMsg("FramePacket Write: ID=%u Count=%d", frameID, count);
		}
	};

	class ABNetEngine : public INetEngine
	{
	public:

		ABNetEngine(LevelStage* stage)
			: mStage(stage)
		{
		}

		bool build(BuildParam& param) override
		{
			LogMsg("NetEngine Build: Worker=%p", param.worker);
			mWorker = param.worker;
			mNetWorker = param.netWorker;

			//mWorker->getEvaluator().setUserFunc<ABActionPacket>(this, &ABNetEngine::onPacket);
			mWorker->getEvaluator().setUserFunc<ABFramePacket>(this, &ABNetEngine::onFramePacket);

			if (mNetWorker->isServer())
			{
				mNetWorker->getEvaluator().setUserFunc<ABActionPacket>(this, &ABNetEngine::onPacketSV);
				mNetWorker->getEvaluator().setUserFunc<ABFramePacket>(this, &ABNetEngine::onFramePacket);
			}
			return true;
		}

		std::vector<ABActionData> mPendingActions;
		long mAccTime = 0;
		static const long TickRate = 66; // 15 FPS
		uint32 mNextFrameID = 0;

		void update(IFrameUpdater& updater, long time) override 
		{ 
			if (mNetWorker->isServer())
			{
				mAccTime += time;
				while (mAccTime >= TickRate)
				{
					mAccTime -= TickRate;
					broadcastFrame();
				}
			}
		}

		void broadcastFrame()
		{
			CHECK(mNetWorker->isServer());

			ABFramePacket packet;
			packet.frameID = mNextFrameID++;
			packet.actions = mPendingActions;
			mPendingActions.clear();

			mNetWorker->sendTcpCommand(&packet, WSF_IGNORE_LOCAL);
		}

		void setupInputAI(IPlayerManager& manager) override {}
		void release() override { delete this; }

		void onFramePacket(IComPacket* cp)
		{
			ABFramePacket* packet = static_cast<ABFramePacket*>(cp);
			for (auto const& action : packet->actions)
			{
				mStage->executeAction(action.port, action.item);
			}
		}

		// Server receives Client Action
		void onPacketSV(IComPacket* cp)
		{
			ABActionPacket* packet = static_cast<ABActionPacket*>(cp);
			// Exec Immediate (Host Advantage / Low Latency)
			mStage->executeAction(packet->port, packet->item);
			// Buffer for Frame
			mPendingActions.push_back({ packet->port, packet->item });
		}

		// Unused but required signature if mapped? No mapped
		void onPacket(IComPacket* cp) {}

		void sendAction(ActionPort port, ABActionItem const& item)
		{
			if (mNetWorker->isServer())
			{
				// Exec Immediate
				mStage->executeAction(port, item);
				// Buffer for Frame
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

		LevelStage* mStage;
		ComWorker* mWorker;
		NetWorker* mNetWorker;
	};

	LevelStage::LevelStage()
	{

	}

	bool LevelStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		mWorld.init();

		for (auto bot : mBots) delete bot;
		mBots.clear();

		int localPlayer = mWorld.getLocalPlayerIndex();
		// Create Bots
		// Assuming Single Player for now, or Host
		// If Networking, we need to know if we are Server?
		// But in this simple framework, maybe just create for all non-local?
		// GameMode::NetLevelStageMode handles logic for syncing?
		// If Net mode is on, we might not want bots on Client?
		// But let's replicate original ABWorld logic: "if (i != mLocalPlayerIndex) create bot".
		// This implies Local Play.
		// If Net Play, mLocalPlayerIndex is set.
		
		// Wait, if Net Play, other players are REAL humans. Bots shouldn't be created for them.
		// But the original code didn't check for "Real Human".
		// It just assumed !Local -> Bot.
		// This implies the original code was PURE SINGLE PLAYER locally.
		// So I will keep that logic.
		
		int playerNum = AB_MAX_PLAYER_NUM; // World knows size? mPlayers.size()
		// We can iterate mPlayers via World helper? World doesn't expose mPlayers iteration easily.
		// But World::getPlayer(i) works.
		
		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			if (i != localPlayer)
			{
				Player& player = mWorld.getPlayer(i);
				mBots.push_back(new BotController(player, mWorld));
			}
		}

		::Global::GUI().cleanupWidget();
		
		// ... UI setup

		return true;
	}

	void LevelStage::onEnd()
	{
		for (auto bot : mBots) delete bot;
		mBots.clear();

		mWorld.cleanup();
		BaseClass::onEnd();
	}

	void LevelStage::onRestart(bool beInit)
	{
		mWorld.restart(beInit);
	}

	void LevelStage::tick()
	{
		if (getModeType() == EGameStageMode::Net)
		{
			if (mActionTemplate)
			{
				mActionTemplate->executeActions(*this);
			}
		}

		float deltaTime = float(getTickTime()) / 1000.0f;
		
		for (auto bot : mBots)
			bot->update(deltaTime);

		mWorld.tick(deltaTime);
	}

	IFrameActionTemplate* LevelStage::createActionTemplate(unsigned version)
	{
		if (getModeType() == EGameStageMode::Net)
		{
			ABFrameActionTemplate* ptr = nullptr;
			auto netMode = getGameStage()->getStageMode()->getNetLevelMode();
			if (netMode && netMode->haveServer())
			{
				ptr = new ABServerNetFrameCollector(AB_MAX_PLAYER_NUM);
			}
			else
			{
				ptr = new ABClientNetFrameCollector(AB_MAX_PLAYER_NUM);
			}
			mActionTemplate = ptr;
			return ptr;
		}
		return nullptr;
	}

	void LevelStage::buildServerLevel(GameLevelInfo& info)
	{
		info.seed = ::Global::Random();
	}

	void LevelStage::buildLocalLevel(GameLevelInfo& info)
	{
		info.seed = ::Global::Random();
	}

	void LevelStage::setupLocalGame(LocalPlayerManager& playerManager)
	{
		for (int i = 0; i < AB_MAX_PLAYER_NUM; ++i)
		{
			playerManager.createPlayer(i);
		}
		playerManager.setUserID(0);
	}

	void LevelStage::setupLevel(GameLevelInfo const& info)
	{
		::Global::RandSeedNet(info.seed);
		mWorld.restart(true);
	}

	void LevelStage::setupScene(IPlayerManager& playerManager)
	{
		int localID = playerManager.getUserID();
		if (localID != ERROR_PLAYER_ID)
		{
			mWorld.setLocalPlayerIndex(localID);
		}
	}

	bool LevelStage::setupNetwork(NetWorker* netWorker, INetEngine** engine)
	{
		ABNetEngine* netEngine = new ABNetEngine(this);
		*engine = netEngine;
		mNetEngine = netEngine;
		return true;
	}

	void LevelStage::executeAction(ActionPort port, ABActionItem const& item)
	{
		// Map Port to Player Index
		// Ideally we use IPlayerManager, but for now assuming Port == PlayerIndex (if strictly assigned)
		// Or assume local linear mapping 0..N
		int playerIndex = (int)port;

		if (playerIndex >= 0 && playerIndex < AB_MAX_PLAYER_NUM)
		{
			Player& player = mWorld.getPlayer(playerIndex);

			switch (item.type)
			{
			case ACT_BUY_UNIT:
				player.buyUnit(item.buy.slotIndex);
				break;
			case ACT_SELL_UNIT:
				player.sellUnit(item.sell.slotIndex);
				break;
			case ACT_REFRESH_SHOP:
				player.rerollShop();
				break;
			case ACT_LEVEL_UP:
				player.buyExperience();
				break;
			case ACT_DEPLOY_UNIT:
				{
					// Resolve Unit from Source
					Unit* unit = nullptr;
					Vec2i srcPos(-1, -1);
					
					if (item.deploy.srcType == 1) // Bench
					{
						if (player.mBench.isValidIndex(item.deploy.srcX))
							unit = player.mBench[item.deploy.srcX];
					}
					else // Board
					{
						int x = item.deploy.srcX;
						int y = item.deploy.srcY;
						if (player.getBoard().isValid(Vec2i(x, y)))
						{
							unit = player.getBoard().getUnit(x, y);
							srcPos = Vec2i(x, y);
						}
					}

					if (unit)
					{
						player.deployUnit(unit, Vec2i(item.deploy.destX, item.deploy.destY), srcPos);
					}
				}
				break;
			case ACT_RETRACT_UNIT:
				{
					Unit* unit = nullptr;
					if (item.retract.srcType == 0) // Board
					{
						int x = item.retract.srcX;
						int y = item.retract.srcY;
						if (player.getBoard().isValid(Vec2i(x, y)))
							unit = player.getBoard().getUnit(x, y);
					}
					else // Bench
					{
						if (player.mBench.isValidIndex(item.retract.srcX))
							unit = player.mBench[item.retract.srcX];
					}

					if (unit)
					{
						Vec2i srcPos(-1, -1);
						if (item.retract.srcType == 0) srcPos = Vec2i(item.retract.srcX, item.retract.srcY);
						player.retractUnit(unit, item.retract.benchIndex, srcPos);
					}
				}
				break;
			}
		}
	}
	
	void LevelStage::sendAction(ABActionItem const& item)
	{
		if (getModeType() == EGameStageMode::Net)
		{
			auto* manager = getGameStage()->getStageMode()->getPlayerManager();
			unsigned id = manager->getUserID();
			if (id == ERROR_PLAYER_ID)
				id = mWorld.getLocalPlayerIndex();

			if (mNetEngine)
			{
				mNetEngine->sendAction((ActionPort)id, item);
			}
		}
		else
		{
			executeAction((ActionPort)mWorld.getLocalPlayerIndex(), item);
		}
	}

	void LevelStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);
		if (getModeType() != EGameStageMode::Net)
		{
			mWorld.tick(deltaTime);
		}
	}

	void LevelStage::onRender(float dFrame)
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		
		g.beginRender();

		// 1. Render World (Background/Scene)
		g.pushXForm();
		g.beginXForm();
		g.translateXForm(mViewOffset.x, mViewOffset.y);
		g.scaleXForm(mViewScale, mViewScale);

		mWorld.render(g);

		if (mDraggedUnit)
		{
			// Highlight Valid Cells
			PlayerBoard& board = mWorld.getLocalPlayerBoard();
			RenderUtility::SetBrush(g, EColor::Null);

			for (int y = PlayerBoard::MapSize.y / 2; y < PlayerBoard::MapSize.y; ++y)
			{
				for (int x = 0; x < PlayerBoard::MapSize.x; ++x)
				{
					Vector2 pos = board.getPos(x, y);
					
					bool bOccupied = board.isOccupied(x, y);
					bool bValid = !bOccupied;
					if (bOccupied)
					{
						Unit* other = board.getUnit(x, y);
						// Allow swap if friendly
						if (other && other->getTeam() == UnitTeam::Player)
							bValid = true;
					}

					if (bValid)
					{
						RenderUtility::SetPen(g, EColor::Green);
						g.drawCircle(pos, PlayerBoard::CellSize.x / 2 - 5);
					}
					else
					{
						RenderUtility::SetPen(g, EColor::Red);
						g.drawCircle(pos, PlayerBoard::CellSize.x / 2 - 5);
					}
				}
			}

			mDraggedUnit->render(g); 
		}

		// Debug info for Board picking usually in world space
		Vec2i mousePos = Global::GUI().getManager().getLastMouseMsg().getPos();
		Vector2 mouseWorldPos = screenToWorld(mousePos); // Calculate once
		Vec2i coord = mWorld.getLocalPlayerBoard().getCoord( mouseWorldPos );
		
		if (mWorld.getLocalPlayerBoard().isValid(coord))
		{
			Vector2 pos = mWorld.getLocalPlayerBoard().getPos(coord.x, coord.y);
			RenderUtility::SetPen(g, EColor::Yellow);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawCircle(pos, PlayerBoard::CellSize.x / 2);
			
			RenderUtility::SetFont(g, FONT_S12);
			g.drawText(pos, InlineString<32>::Make("%d,%d", coord.x, coord.y));
		}

		g.popXForm();


		// 2. Render UI (Foreground)
		
		// Render UI info
		{
			InlineString<64> info;
			char const* phaseName = "Unknown";
			switch (mWorld.getPhase())
			{
			case BattlePhase::Preparation: phaseName = "Preparation"; break;
			case BattlePhase::Combat:      phaseName = "Combat"; break;
			}
			info.format("%s (%.1f)", phaseName, mWorld.getPhaseTimer());

			RenderUtility::SetFont(g, FONT_S24);
			g.setTextColor(Color3ub(255, 255, 255));
			g.drawText(Vec2i(10, 10), info);
			
			Player& player = mWorld.getLocalPlayer();
			info.format("Gold: %d  HP: %d  Lvl: %d", player.getGold(), player.getHp(), player.getLevel());
			g.drawText(Vec2i(10, 40), info);

			// View Info
			InlineString<32> viewInfo;
			viewInfo.format("Viewing Player %d", mViewPlayerIndex);
			g.drawText(Vec2i(10, 70), viewInfo);
		}
		
		// Render Shop UI
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			Vec2i shopSize(screenSize.x, 150);
			Vec2i shopPos(0, screenSize.y - shopSize.y);

			RenderUtility::SetBrush(g, EColor::Gray);
			RenderUtility::SetPen(g, EColor::Black);
			g.drawRect(shopPos, shopSize);
			
			int cardWidth = 100;
			int cardHeight = 120;
			int gap = 15;
			int startX = (shopSize.x - (5 * cardWidth + 4 * gap)) / 2;
			int startY = shopPos.y + (shopSize.y - cardHeight) / 2;

			Player& player = mWorld.getLocalPlayer();
			for (int i = 0; i < player.mShopList.size(); ++i)
			{
				int unitId = player.mShopList[i];
				if (unitId == 0) continue;

				Vec2i cardPos(startX + i * (cardWidth + gap), startY);
				RenderUtility::SetBrush(g, EColor::White);
				g.drawRect(cardPos, Vec2i(cardWidth, cardHeight));
				
				g.setTextColor(Color3ub(0, 0, 0));
				
				UnitDefinition const* def = mWorld.getUnitDataManager().getUnit(unitId);
				if (def)
				{
					g.drawText(cardPos + Vec2i(10, 10), def->name);
					
					InlineString<16> costStr;
					costStr.format("$%d", def->cost);
					g.drawText(cardPos + Vec2i(10, cardHeight - 25), costStr);
				}
				else
				{
					InlineString<32> str;
					str.format("Unit %d", unitId);
					g.drawText(cardPos + Vec2i(10, 10), str);
					g.drawText(cardPos + Vec2i(10, cardHeight - 25), "$?");
				}
			}
		}

		g.finishXForm();
		g.endRender();
	}

	ERenderSystem LevelStage::getDefaultRenderSystem()
	{
		return ERenderSystem::D3D11;
	}

	void LevelStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.bWasUsedPlatformGraphics = true;
	}

	bool LevelStage::setupRenderResource(ERenderSystem systemName)
	{
		return true;
	}

	void LevelStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{

	}

	MsgReply LevelStage::onMouse(MouseMsg const& msg)
	{
		Player& player = mWorld.getLocalPlayer();
		PlayerBoard& board = mWorld.getLocalPlayerBoard();

		// Shop Interaction
		if (msg.onLeftDown())
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			Vec2i shopSize(screenSize.x, 150);
			Vec2i shopPos(0, screenSize.y - shopSize.y);
			Vec2i mousePos = msg.getPos();

			if (mousePos.x >= shopPos.x && mousePos.x < shopPos.x + shopSize.x &&
				mousePos.y >= shopPos.y && mousePos.y < shopPos.y + shopSize.y)
			{
				int cardWidth = 100;
				int cardHeight = 120;
				int gap = 15;
				int startX = (shopSize.x - (5 * cardWidth + 4 * gap)) / 2;
				int startY = shopPos.y + (shopSize.y - cardHeight) / 2;

				for (int i = 0; i < 5; ++i)
				{
					Vec2i cardPos(startX + i * (cardWidth + gap), startY);
					if (mousePos.x >= cardPos.x && mousePos.x < cardPos.x + cardWidth &&
						mousePos.y >= cardPos.y && mousePos.y < cardPos.y + cardHeight)
					{
						ABActionItem item;
						item.type = ACT_BUY_UNIT;
						item.buy.slotIndex = i;
						sendAction(item);
						return MsgReply::Handled();
					}
				}
				return MsgReply::Handled(); // Consumed by shop background
			}
		}

		static Vec2i lastPos = msg.getPos();

		if (msg.onLeftDown())
		{
			Vec2i mousePos = msg.getPos();

			// Try pick unit
			Vector2 worldPos = screenToWorld(Vector2(msg.getPos().x, msg.getPos().y));
			
			// Simple hit test
			// Iterating backwards to pick top-most if any overlap (though grid avoids this)
			// Check Player Units (Board)
			// Iterate backwards safely
			if (mWorld.getPhase() == BattlePhase::Preparation)
			{
				int unitCount = (int)player.mUnits.size();
				for (int i = unitCount - 1; i >= 0; --i)
				{
					if (!player.mUnits.isValidIndex(i)) continue;
					
					Unit* unit = player.mUnits[i];
					float distSq = (unit->getPos() - worldPos).length2();
					float radius = PlayerBoard::CellSize.x / 2.0f; 
					if (distSq < radius * radius)
					{
						mDraggedUnit = unit;
						mDragStartPos = unit->getPos();
						mDragStartCoord = board.getCoord(mDragStartPos);
						mDragOffset = unit->getPos() - worldPos;
						mDragSource = DragSource::Board;
						mDragSourceIndex = i;
						return BaseClass::onMouse(msg); 
					}
				}
			}
			
			// Check Bench (World Space)
			Vector2 boardOffset = board.getOffset();
			Vector2 benchStartPos = boardOffset + Vector2(0, PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f + 20); // Move match Player::render
			int benchSlotSize = 40; 
			int benchGap = 5;
			int benchTotalWidth = AB_BENCH_SIZE * (benchSlotSize + benchGap) - benchGap;

			if (worldPos.y >= benchStartPos.y && worldPos.y < benchStartPos.y + benchSlotSize)
			{
				float relX = worldPos.x - benchStartPos.x;
				if (relX >= 0 && relX < benchTotalWidth)
				{
					int slotIndex = (int)(relX / (benchSlotSize + benchGap));
					// Check gap
					float slotRelX = relX - slotIndex * (benchSlotSize + benchGap);
					
					if (slotRelX < benchSlotSize && player.mBench.isValidIndex(slotIndex))
					{
						Unit* unit = player.mBench[slotIndex];
						if (unit)
						{
							mDraggedUnit = unit;
							// Calculate World Pos of slot logic
							Vector2 slotPos = benchStartPos + Vector2(slotIndex * (benchSlotSize + benchGap), 0);
							mDragStartPos = slotPos + Vector2(benchSlotSize / 2, benchSlotSize / 2);
							
							mDragOffset = mDragStartPos - worldPos; 
							
							// For dragging visual, set pos to actual world pos
							unit->setPos(worldPos); 
							// Actually drag offset should be relative to unit center?
							// If we picked unit, unit->getPos() might be undefined if it was on bench.
							// So valid logic is: mDragStartPos is the slot center.
							// mDragOffset is (SlotCenter - MousePos).
							// When rendering dragged unit: Pos = MousePos + Offset = MousePos + SlotCenter - MousePosStart...
							// Actually standar drag logic:
							// OnPick: Offset = UnitPos - MousePos.
							// OnDrag: UnitPos = MousePos + Offset.
							// Here UnitPos (virtual) is SlotCenter.
							mDragOffset = mDragStartPos - worldPos;

							mDragSource = DragSource::Bench;
							mDragSourceIndex = slotIndex;
							return BaseClass::onMouse(msg);
						}
					}
				}
			}
		}
		else if (msg.onLeftUp())
		{
			if (mDraggedUnit)
			{
				Vector2 worldPos = screenToWorld(Vector2(msg.getPos().x, msg.getPos().y));
				Vec2i coord = board.getCoord(worldPos);

				bool bDropped = false;

				// 1. Try Drop on Grid (Board)
				if (board.isValid(coord))
				{
					bool bPhaseOK = (mWorld.getPhase() == BattlePhase::Preparation);
					bool bAreaOK = (coord.y >= PlayerBoard::MapSize.y / 2);

					if (bPhaseOK && bAreaOK)
					{
						// Pass source coordinate if dragging from board
						Vec2i sourceCoord = (mDragSource == DragSource::Board) ? mDragStartCoord : Vec2i(-1, -1);
						
						ABActionItem item;
						item.type = ACT_DEPLOY_UNIT;
						item.deploy.destX = coord.x;
						item.deploy.destY = coord.y;

						if (mDragSource == DragSource::Board)
						{
							item.deploy.srcType = 0;
							item.deploy.srcX = mDragStartCoord.x;
							item.deploy.srcY = mDragStartCoord.y;
						}
						else
						{
							item.deploy.srcType = 1;
							item.deploy.srcX = mDragSourceIndex;
							item.deploy.srcY = 0; // Unused
						}
						sendAction(item);
						bDropped = true;
					}
				}
				// 2. Try Drop on Bench
				else 
				{
					// Move match Player::render
					Vector2 boardOffset = board.getOffset();
					Vector2 benchStartPos = boardOffset + Vector2(0, PlayerBoard::MapSize.y * PlayerBoard::CellSize.y * 0.75f + 20); 
					int benchSlotSize = 40; 
					int benchGap = 5;
					int benchTotalWidth = AB_BENCH_SIZE * (benchSlotSize + benchGap) - benchGap;

					// Check World Pos in Bench Rect
					if (worldPos.y >= benchStartPos.y && worldPos.y < benchStartPos.y + benchSlotSize &&
						worldPos.x >= benchStartPos.x && worldPos.x < benchStartPos.x + benchTotalWidth)
					{
						float relX = worldPos.x - benchStartPos.x;
						int slotIndex = (int)(relX / (benchSlotSize + benchGap));
						float slotRelX = relX - slotIndex * (benchSlotSize + benchGap);
						
						if (slotRelX < benchSlotSize && player.mBench.isValidIndex(slotIndex))
						{
							ABActionItem item;
							item.type = ACT_RETRACT_UNIT;
							item.retract.benchIndex = slotIndex;

							if (mDragSource == DragSource::Board)
							{
								item.retract.srcType = 0;
								item.retract.srcX = mDragStartCoord.x;
								item.retract.srcY = mDragStartCoord.y;
							}
							else
							{
								item.retract.srcType = 1;
								item.retract.srcX = mDragSourceIndex;
								item.retract.srcY = 0;
							}
							sendAction(item);
							bDropped = true;
						}
					}
				}

				if (!bDropped)
				{
					// Revert
					if (mDragSource == DragSource::Board)
					{
						mDraggedUnit->setPos(mDragStartPos);
					}
					else if (mDragSource == DragSource::Bench)
					{
						// Return to bench slot (Visual only, simple Reset)
						// Actually ABPlayer::retractUnit logic might need mDraggedUnit to be in a consistent state if it failed?
						// But if failure, we just restore Position?
						// ABPlayer::deploy/retract assumed logic starts from valid state.
						// If we dragged visually, mDraggedUnit pointer is valid, but its internal state (mInternalBoard, mPos, Index) 
						// is NOT changed until Drop Logic succeeds.
						// Wait, did we remove it from Bench/Board on PICK?
						// NO. ABStage logic only moved it visually?
						// Let's check "Pick" logic.
						// "mDraggedUnit = unit;" "unit->setPos(worldPos);"
						// It does NOT remove unit from vectors.
						// So unit is arguably still "at mDragStartPos" logically.
						// So reverting just means fixing the Visual Position.
						mDraggedUnit->setPos(mDragStartPos);
					}
				}

				mDraggedUnit = nullptr;
				mDragSource = DragSource::None;
				mDragSourceIndex = -1;
			}
		}
		
		if (mDraggedUnit && msg.isLeftDown())
		{
			Vector2 worldPos = screenToWorld(Vector2(msg.getPos().x, msg.getPos().y));
			mDraggedUnit->setPos(worldPos + mDragOffset);
		}

		if (msg.onRightDown())
		{
			lastPos = msg.getPos();
		}
		else if (msg.isRightDown() && msg.getPos() != lastPos)
		{
			Vec2i offset = msg.getPos() - lastPos;
			mViewOffset += Vector2(offset.x, offset.y);
			lastPos = msg.getPos();
		}
		
		if (msg.onWheelFront())
		{
			mViewScale *= 1.1f;
		}
		else if (msg.onWheelBack())
		{
			mViewScale /= 1.1f;
		}

		return BaseClass::onMouse(msg);
	}

	Vector2 LevelStage::screenToWorld(Vector2 const& screenPos) const
	{
		return (screenPos - mViewOffset) / mViewScale;
	}

	Vector2 LevelStage::worldToScreen(Vector2 const& worldPos) const
	{
		return worldPos * mViewScale + mViewOffset;
	}

	MsgReply LevelStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::Q: // Previous Player
			{
				mViewPlayerIndex--;
				if (mViewPlayerIndex < 0) mViewPlayerIndex = AB_MAX_PLAYER_NUM - 1;
				
				Player& player = mWorld.getPlayer(mViewPlayerIndex);
				Vector2 boardOffset = player.getBoard().getOffset();
				// Center on this board
				Vec2i screenSize = ::Global::GetScreenSize();
				mViewOffset = Vector2(screenSize.x / 2, screenSize.y / 2) - (boardOffset + Vector2(300, 300)) * mViewScale;
			}
			break;
			case EKeyCode::E: // Next Player
			{
				mViewPlayerIndex++;
				if (mViewPlayerIndex > 7) mViewPlayerIndex = 0;
				
				Player& player = mWorld.getPlayer(mViewPlayerIndex);
				Vector2 boardOffset = player.getBoard().getOffset();
				// Center on this board
				Vec2i screenSize = ::Global::GetScreenSize();
				mViewOffset = Vector2(screenSize.x / 2, screenSize.y / 2) - (boardOffset + Vector2(300, 300)) * mViewScale;
			}
			break;
			case EKeyCode::D: 
				// Reroll Shop (Test)
				{
					ABActionItem item;
					item.type = ACT_REFRESH_SHOP;
					sendAction(item);
				}
				break;
			case EKeyCode::F:
				{
					ABActionItem item;
					item.type = ACT_LEVEL_UP;
					sendAction(item);
				}
				break;
			case EKeyCode::Space:
				if (mWorld.getPhase() == BattlePhase::Preparation)
					mWorld.changePhase(BattlePhase::Combat);
				else if (mWorld.getPhase() == BattlePhase::Combat)
					mWorld.changePhase(BattlePhase::Preparation);
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

}
