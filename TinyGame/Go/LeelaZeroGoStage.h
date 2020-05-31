#pragma once
#ifndef LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045
#define LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045

#include "Stage/TestStageHeader.h"

#include "Go/MatchGame.h"
#include "Go/GoCore.h"
#include "Go/GoRenderer.h"
#include "Go/GoBot.h"
#include "Widget/WidgetUtility.h"

#include "Bots/ZenBot.h"
#include "Bots/LeelaBot.h"

#include "FileSystem.h"
#include "Delegate.h"

#include "GameSettingPanel.h"



using namespace Render;

#define DETECT_LEELA_PROCESS 0

class GPUDeviceQuery;

namespace Go
{

	class GoReplayFrame : public GFrame
	{
		using BaseClass = GFrame;



	};


	class GFilePicker : public GWidget
	{
		using BaseClass = GWidget;
	public:
		GFilePicker(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(pos, size, parent)
		{
			mID = id;
			auto* button = new GButton(UI_ANY, Vec2i(size.x - 20 ,0), Vec2i(20, size.y), this);
			button->setTitle("..");
			button->onEvent = [this](int event, GWidget*) -> bool
			{
				if (SystemPlatform::OpenFileName(filePath, filePath.max_size(), {}, nullptr, nullptr, ::Global::GetDrawEngine().getWindowHandle()))
					return false;

				return false;
			};
		}

		FixString<512> filePath;
		bool bShowFileNameOnly = true;

		void onRender() override
		{
			IGraphics2D& g = ::Global::GetIGraphics2D();

			Vec2i pos = getWorldPos();
			Vec2i size;
			g.beginClip(pos, getSize() - Vec2i(20, 0));
			if( bShowFileNameOnly )
			{
				char const* fileName = FileUtility::GetFileName(filePath);
				g.drawText(pos + Vec2i(2,3), fileName);
			}
			else
			{
				g.drawText(pos + Vec2i(2,3), filePath);
			}
			g.endClip();
		}

	};


	class MatchSettingPanel : public BaseSettingPanel
	{
		using BaseClass = BaseSettingPanel;
	public:
		enum
		{
			MaskBotSettingA = BIT(0),
			MaskBotSettingB = BIT(1),
		};

		enum
		{
			UI_FIXED_HANDICAP = UI_WIDGET_ID,
			UI_PLAY,
			UI_CANCEL ,
			UI_AUTO_RUN ,
			UI_SAVE_SGF ,

			UI_CONTROLLER_TYPE_A = UI_WIDGET_ID + 100,
			UI_CONTROLLER_TYPE_B = UI_WIDGET_ID + 200,
		};

		enum 
		{
			UPARAM_VISITS = 1,
			UPARAM_WEIGHT_NAME,
			UPARAM_MAX_TIME , 
			UPARAM_SIMULATIONS_NUM ,
		};


		MatchSettingPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(id , pos , size , parent)
		{
			addBaseWidget();
		}

		void addBaseWidget();

		void addLeelaParamWidget(int id , int idxPlayer );
		void addKataPramWidget(int id, int idxPlayer);
		void addZenParamWidget(int id, int idxPlayer);

		GChoice* addPlayerChoice(int idxPlayer, char const* title);

		bool onChildEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			case UI_PLAY:
				sendEvent(EVT_DIALOG_CLOSE);
				this->destroy();
				return false;
			case UI_CANCEL:
				this->destroy();
				return false;
			case UI_CONTROLLER_TYPE_A:
			case UI_CONTROLLER_TYPE_B:
				return false;
			}

			return BaseClass::onChildEvent(event, id, ui);
		}

		bool setupMatchSetting(MatchGameData& matchData , GameSetting& outSetting);


		template< class T , class Widget >
		T getParamValue(int id)
		{
			return FWidgetPropery::Get<T>(findChildT<Widget>(id));
		}
	};


	class BoardFrame : public GFrame
	{
	public:
		using BaseClass = GFrame;

		BoardFrame( GameProxy& game , int id, Vec2i const& pos, Vec2i const size, GWidget* parent)
			:GFrame(id, pos, size, parent)
			,renderContext(game.getBoard(), getWorldPos() + GetBorder(), CalcRenderScale(size))
			,mBorad(game.getBoard())
		{

		}

		static float CalcRenderScale(Vec2i const size)
		{
			Vec2i clientSize = size - 2 * GetBorder();
			int boardRenderSize = Math::Min(clientSize.x, clientSize.y);
			return float(boardRenderSize) / RenderContext::CalcDefalutSize(19);
		}

		static Vec2i GetBorder()
		{
			return Vec2i(20, 20);
		}
		void onRender() override;
		void onChangePos(Vec2i const& pos, bool bParentMove) override
		{
			BaseClass::onChangePos(pos, bParentMove);
			renderContext.renderPos = getWorldPos() + GetBorder();
		}
		RenderContext  renderContext;
		BoardRenderer* renderer;
		Board const&   mBorad;
	};

	class TryPlayBoardFrame : public BoardFrame
	{
	public:
		using BaseClass = BoardFrame;

		TryPlayBoardFrame(int id, Vec2i const& pos, Vec2i const size, GWidget* parent)
			:BoardFrame(game , id, pos, size, parent)
		{

		}

		bool onMouseMsg(MouseMsg const& msg) override
		{
			if( msg.onLeftDown() )
			{
				Vec2i pos = renderContext.getCoord(msg.getPos());
				if( !game.canPlay(pos.x, pos.y) )
					return false;
				if ( game.playStone(pos.x, pos.y) )
					return false;
			}
			else if( msg.onRightDown() )
			{
				game.undo();
				return false;
			}

			if( !BaseClass::onMouseMsg(msg) )
				return false;

			return true;
		}

		GameProxy game;
	};


	class LeelaZeroGoStage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		LeelaZeroGoStage() {}

		enum
		{
			UI_REPLAY_FRAME = BaseClass::NEXT_UI_ID,
			UI_MATCH_PANEL ,
			NEXT_UI_ID,
		};

		enum class GameMode
		{
			Learning,
			Match ,
			Analysis ,
			None ,
		};

		LeelaAppRun   mLeelaAIRun;

		bool bPauseGame = false;
		int  numGameCompleted = 0;
		bool bMatchJob = false;
		FixString<32> mUsedWeight;
		int  matchChallenger = StoneColor::eEmpty;
#if DETECT_LEELA_PROCESS
		DWORD  mPIDLeela = -1;
		static long const LeelaRebootTime = 40000;
		static long const LeelaRebootStartTime = 40 * LeelaRebootTime;
		long   mLeelaRebootTimer = LeelaRebootTime;
#endif

		GameProxy mGame;

		bool bDrawDebugMsg = false;
		bool bDrawFontCacheTexture = false;
		BoardRenderer mBoardRenderer;
		float const RenderBoardScale = 1.2;
		Vector2 BoardPos = Vector2(100, 55);

		GameMode mGameMode = GameMode::None;
		class UnderCurveAreaProgram* mProgUnderCurveArea = nullptr;

		bool toggleAnalysisPonder();

		bool tryEnableAnalysis(bool bCopyGame);
		void synchronizeAnalysisState();

		bool canAnalysisPonder(MatchPlayer& player)
		{
			if( player.type == ControllerType::ePlayer )
				return true;
			if( player.bot && !player.bot->isGPUBased() )
				return true;
			return false;
		}

		template< class TFunc >
		void executeAnalysisAICommand(TFunc&& func, bool bKeepPonder = true);

		bool bAnalysisEnabled = false;
		bool bAnalysisPondering;
		bool bShowAnalysis = true;
		LeelaThinkInfoVec analysisResult;
		LeelaThinkInfo bestThinkInfo;

		int  analysisPonderColor = 0;
		PlayVertex  showBranchVertex = PlayVertex::Undefiend();

		template< class TFunc >
		bool checkWaitBotCommand(EBotExecResult result , TFunc&& func)
		{
			switch (result)
			{
			case BOT_OK:
				func(result); return true;
			case BOT_FAIL:
				return false;
			}
			mWaitBotCommandDelegate = func;
		}

		bool isWatingBotResult()
		{
			return bool(mWaitBotCommandDelegate);
		}

		std::function< void(EBotExecResult) > mWaitBotCommandDelegate;

		bool bSwapEachMatch = true;
		int  unknownWinerCount = 0;
		MatchGameData mMatchData;
		FixString<32> mLastGameResult;

		GPUDeviceQuery* mDeviceQuery = nullptr;

		void drawWinRateDiagram( Vec2i const& renderPos ,  Vec2i const& renderSize );

		class GameStatusQuery : public IGameCopier
		{
		public:
			GameStatusQuery()
			{
				mLastQueryId = Guid::New();
			}
			bool queryTerritory(GameProxy& game, Zen::TerritoryInfo& info)
			{
				if( !ensureBotInitialized() )
					return false;
				
				copyGame(game);
				mBot->getTerritoryStatictics(info);
				return true;
			}

			void copyGame(GameProxy& game)
			{
				if( mLastQueryId != game.guid || mNextCopyStep > game.getInstance().getCurrentStep() )
				{
					game.getInstance().synchronizeState(*this, true);
					mLastQueryId = game.guid;
				}
				else if( mNextCopyStep == game.getInstance().getCurrentStep() )
				{
					return;
				}
				else
				{
					game.getInstance().synchronizeStateKeep(*this, mNextCopyStep, true);
				}
				mNextCopyStep = game.getInstance().getCurrentStep();
			}

			~GameStatusQuery()
			{
				while( mbWaitInitialize ){}

				if( mBot )
				{
					mBot->release();
				}
			}
			void emitSetup(GameSetting const& setting) override
			{
				mBot->startGame(setting);
			}
			void emitPlayStone(int x, int y, int color) override
			{
				mBot->playStone(x, y, ZenBot::ToZColor(color));
			}
			void emitAddStone(int x, int y, int color) override
			{
				mBot->addStone(x, y, ZenBot::ToZColor(color));
			}
			void emitPlayPass(int color) override
			{
				mBot->playPass(ZenBot::ToZColor(color));
			}
			void emitUndo() override
			{
				mBot->undo();
			}

			bool ensureBotInitialized()
			{
				if( mbWaitInitialize )
					return false;

				if( mBot == nullptr )
				{
					mBot.reset(Zen::TBotCore<7>::Create<Zen::TBotCore<7>>());
					mbWaitInitialize = true;
					auto work = new InitializeWork(*this);
					work->start();
				}

				return !mbWaitInitialize;
			}

			struct InitializeWork : public RunnableThreadT<InitializeWork >
			{
			public:
				InitializeWork(GameStatusQuery& query)
					:mQuery(query)
				{
				}

				unsigned run()
				{
					if( !mQuery.mBot->initialize() )
					{
						mQuery.mBot.release();
					}
					return 0;
				}
				void exit() 
				{ 
					mQuery.mbWaitInitialize = false;
					delete this;
				}

				GameStatusQuery& mQuery;

			};

			volatile bool mbWaitInitialize = false;
			Guid mLastQueryId;
			int  mNextCopyStep;
			std::unique_ptr< Zen::TBotCore<7> > mBot;
		};

		bool bUpdateTerritory = false;
		bool bShowTerritory = false;
		bool bTerritoryInfoValid = false;
		Zen::TerritoryInfo mShowTerritoryInfo;
		GameStatusQuery mStatusQuery;

		void updateViewGameTerritory();

		void drawAnalysis(GLGraphics2D& g, SimpleRenderState& renderState, RenderContext& context);
		static void DrawTerritoryStatus( BoardRenderer& renderer , SimpleRenderState& renderState, RenderContext const& context ,  Zen::TerritoryInfo const& info);


		std::vector< Vector2 > mWinRateHistory[2];
		
		PlayVertex  bestMoveVertex;
		
		bool       bReviewingGame = false;
		GameProxy  mReviewGame;
		bool       bTryPlayingGame = false;
		GameProxy  mTryPlayGame;	
		Vec2i      mTryPlayWidgetPos = Vec2i(0,0);
		TryPlayBoardFrame* mTryPlayWidget = nullptr;

		GWidget* mGamePlayWidget = nullptr;
		GWidget* mWinRateWidget = nullptr;
		GWidget* mModeWidget = nullptr;
		bool     mbRestartLearning = false;

		int mBotBoardState[19 * 19];
		bool mbBotBoardStateValid = false;

		MatchResultMap mMatchResultMap;
		uint32         mLastMatchRecordWinCounts[2];
		void recordMatchResult( bool bSaveToFile );

		GameProxy& getAnalysisGame()
		{
			if (bReviewingGame)
				return mReviewGame;
			return mGame;
		}
		GameProxy& getViewingGame()
		{
			if( bTryPlayingGame )
				return mTryPlayGame;
			else if( bReviewingGame )
				return mReviewGame;
			return mGame;
		}

		bool saveMatchGameSGF(char const* matchResult = nullptr);
	

		bool onInit() override;
		void onEnd() override;
		void onUpdate(long time) override;

		void onRender(float dFrame) override;
		bool onWidgetEvent(int event, int id, GWidget* ui) override;
		bool onMouse(MouseMsg const& msg) override;
		bool onKey(KeyMsg const& msg) override;


		void tick() {}
		void updateFrame(int frame) {}

		void cleanupModeData(bool bEndStage = false);

		void keepLeelaProcessRunning(long time);

		void processLearningCommand();
		void notifyPlayerCommand(int indexPlayer, GameCommand const& com);

		bool bPrevGameCom = false;
		void resetGameParam();
		void resetTurnParam();
		void restartAutoMatch();
		void postMatchGameEnd(char const* matchResult);

		bool buildLearningMode();
		bool buildAnalysisMode(bool bRestartGame = true);
		bool buildPlayMode();
		bool buildLeelaMatchMode();
		bool buildLeelaZenMatchMode();

		bool startMatchGame(GameSetting const& setting);

		void createPlayWidget();

		bool canPlay()
		{
			return mGameMode == GameMode::Match && !isBotTurn();
		}
		void execPlayStoneCommand(Vec2i const& pos);
		void execPassCommand();
		void execUndoCommand();
		bool isBotTurn()
		{
			return !isPlayerControl(mGame.getInstance().getNextPlayColor());
		}

		bool isPlayerControl(int color)
		{
			return !mMatchData.players[color - 1].isBot();
		}
	};


}//namespace Go


#endif // LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045
