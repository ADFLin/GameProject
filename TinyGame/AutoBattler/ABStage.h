#ifndef ABStage_h__
#define ABStage_h__

#include "GameStage.h"
#include "ABGame.h"
#include "ABWorld.h"

#include "GameRenderSetup.h"
#include "DataStructure/Array.h"

namespace AutoBattler
{
	struct ABActionItem;

	enum class DragSource
	{
		None,
		Board,
		Bench,
	};

	class ABNetEngine;
	class BotController;

	class LevelStage : public GameStageBase
		             , public IGameRenderSetup
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

		void executeAction(ActionPort port, ABActionItem const& item);
		void sendAction(ABActionItem const& item);
		IFrameActionTemplate* createActionTemplate(unsigned version) override;


		virtual void buildServerLevel(GameLevelInfo& info);
		virtual void buildLocalLevel(GameLevelInfo& info);

		virtual void setupLocalGame(LocalPlayerManager& playerManager);
		virtual void setupLevel(GameLevelInfo const& info);
		virtual void setupScene(IPlayerManager& playerManager);

		bool setupNetwork(NetWorker* worker, INetEngine** engine) override;

		// IGameRenderSetup
		ERenderSystem getDefaultRenderSystem() override;
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;
		bool setupRenderResource(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;

	protected:
		World mWorld;
		TArray<BotController*> mBots;

		Vector2 mViewOffset = Vector2(100, 100);
		float   mViewScale = 1.0f;
		int     mViewPlayerIndex = 0; // Local Player is usually 0

		Unit*   mDraggedUnit = nullptr;
		Vector2 mDragOffset;
		Vec2i   mDragStartCoord;
		Vector2 mDragStartPos;
		DragSource mDragSource = DragSource::None;
		int     mDragSourceIndex = -1;

		class ABFrameActionTemplate* mActionTemplate = nullptr;

		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;
		
		Vector2 screenToWorld(Vector2 const& screenPos) const;
		Vector2 worldToScreen(Vector2 const& worldPos) const;

	public:
		BattlePhase getPhase() const { return mWorld.getPhase(); }
		Player& getPlayer() { return mWorld.getLocalPlayer(); }
		PlayerBoard& getPlayerBoard() { return mWorld.getLocalPlayerBoard(); }
	
	};

}//namespace AutoBattler


#endif // ABStage_h__
