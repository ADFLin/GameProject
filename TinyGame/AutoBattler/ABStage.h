#ifndef ABStage_h__
#define ABStage_h__

#include "GameStage.h"
#include "ABGame.h"
#include "ABWorld.h"
#include "ABGameActionControl.h"
#include "ABPlayerController.h"

#include "GameRenderSetup.h"
#include "DataStructure/Array.h"
#include "ABDefine.h"
#include "ABViewCamera.h"
#include "ABGameHUD.h"

#include <memory>

namespace AutoBattler
{
	struct ABActionItem;

	class ABNetEngine;
	class BotController;
	class ABGameRenderer;

	class LevelStage : public GameStageBase
		             , public IGameRenderSetup
					 , public IGameActionControl
					 , public IPlayerControllerContext
	{
		using BaseClass = GameStageBase;
	public:
		LevelStage();

		ABNetEngine* mNetEngine = nullptr;

		bool onInit() override;
		void onEnd() override;
		void onUpdate(GameTimeSpan deltaTime) override;
		void onRender(float dFrame) override;


		void onRestart(bool beInit) override;
		void tick() override;


		void runLogic(float dt);

		void executeAction(ActionPort port, ABActionItem const& item);
		void sendAction(ABActionItem const& item);
		void sendAction(ActionPort port, ABActionItem const& item);
		IFrameActionTemplate* createActionTemplate(unsigned version) override;

		// IGameActionControl
		virtual void buyUnit(Player& player, int slotIndex) override;
		virtual void sellUnit(Player& player, int slotIndex) override;
		virtual void refreshShop(Player& player) override;
		virtual void buyExperience(Player& player) override;
		virtual void deployUnit(Player& player, int srcType, int srcX, int srcY, int destX, int destY) override;
		virtual void retractUnit(Player& player, int srcType, int srcX, int srcY, int benchIndex) override;
		virtual void syncDrag(Player& player, int srcType, int srcIndex, int posX, int posY, bool bDrop) override;


		virtual void configLevelSetting(GameLevelInfo& info);

		virtual void setupLocalGame(LocalPlayerManager& playerManager);
		virtual void setupLevel(GameLevelInfo const& info);
		virtual void setupScene(IPlayerManager& playerManager);

		bool setupNetwork(NetWorker* worker, INetEngine** engine) override;

		// IGameRenderSetup
		ERenderSystem getDefaultRenderSystem() override;
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;
		bool setupRenderResource(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;
		
		bool mIsConnectionLost = false;
		bool mUseBots = true;

	protected:
		World mWorld;
		TArray<BotController*> mBots;

		ABViewCamera mCamera;

		class ABFrameActionTemplate* mActionTemplate = nullptr;
		std::unique_ptr<ABPlayerController> mController;
		std::unique_ptr<ABGameRenderer> mRenderer;
		std::unique_ptr<ABGameHUD> mHUD;

		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;

	public:
		BattlePhase getPhase() const { return mWorld.getPhase(); }
		PlayerBoard& getPlayerBoard() { return mWorld.getLocalPlayerBoard(); }
		World& getWorld() override { return mWorld; }
		bool isNetMode() const override { return getModeType() == EGameStageMode::Net; }
	
		// IPlayerControllerContext
		Vector2 screenToWorld(Vector2 const& screenPos) const override;
		Unit* pickUnit(Vec2i screenPos) override;
		UnitLocation screenToDropTarget(Vec2i screenPos) override;
		Vector2 worldToScreen(Vector2 const& worldPos) const;
	
	private:
		// Helper: find closest unit to worldPos within pickRadius
		Unit* pickUnitAtWorldPos(Vector2 worldPos, float pickRadius);
	};

}//namespace AutoBattler


#endif // ABStage_h__
