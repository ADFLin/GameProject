#pragma once
#ifndef LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045
#define LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045

#include "Stage/TestStageHeader.h"

#include "Go/GoCore.h"
#include "Go/GoRenderer.h"
#include "Go/GoBot.h"
#include "ZenBot.h"
#include "LeelaBot.h"
#include "Widget/WidgetUtility.h"

#include "Misc/Guid.h"
#include "FileSystem.h"

#include "GameSettingPanel.h"


using namespace RenderGL;

#define DETECT_LEELA_PROCESS 0

namespace Go
{

	class GoReaplyFrame : public GFrame
	{
		typedef GFrame BaseClass;



	};

	enum class ControllerType
	{
		ePlayer   = 0,
		eLeelaZero ,
		eAQ ,
		eZenV7,
		eZenV6,
		eZenV4 ,


		Count ,
	};

	char const* GetControllerName(ControllerType type)
	{
		switch( type )
		{

		case Go::ControllerType::ePlayer:
			return "Player";
		case Go::ControllerType::eLeelaZero:
			return "LeelaZero";
		case Go::ControllerType::eAQ:
			return "AQ";
		case Go::ControllerType::eZenV7:
			return "Zen7";
		case Go::ControllerType::eZenV6:
			return "Zen6";
		case Go::ControllerType::eZenV4:
			return "Zen4";
		}

		return "Unknown";
	}

	class MyGame : public Go::Game
	{

	public:
		void restart()
		{
			Go::Game::restart();
			guid = Guid::New();
		}
		Guid guid;
	};

	struct MatchPlayer
	{
		ControllerType type;
		std::unique_ptr< IBotInterface > bot;
		int winCount;

		bool isBot() const { return type != ControllerType::ePlayer; }
		bool initialize(ControllerType inType , void* botSetting = nullptr);
	};

	class GFilePicker : public GWidget
	{
		typedef GWidget BaseClass;
	public:
		GFilePicker(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(pos, size, parent)
		{
			mID = id;
			GButton* button = new GButton(UI_ANY, Vec2i(size.x - 20 ,0), Vec2i(20, size.y), this);
			button->setTitle("..");
			button->onEvent = [this](int event, GWidget*) -> bool
			{
				SystemPlatform::OpenFileName(filePath, filePath.max_size(), nullptr);
				return false;
			};
		}

		FixString<512> filePath;
		bool bShowFileNameOnly = true;

		virtual void onRender() override
		{
			IGraphics2D& g = ::Global::getIGraphics2D();

			Vec2i pos = getWorldPos();
			Vec2i size;
			g.beginClip(pos, getSize() - Vec2i(20, 0));
			if( bShowFileNameOnly )
			{
				char const* fileName = FileUtility::GetDirPathPos(filePath) + 1;
				g.drawText(pos + Vec2i(2,3), fileName);
			}
			else
			{
				g.drawText(pos + Vec2i(2,3), filePath);
			}
			g.endClip();
		}

	};



	struct MatchGameData
	{
		MatchPlayer players[2];
		int         idxPlayerTurn = 0;
		bool        bSwapColor = false;
		bool        bAutoRun = false;

		MatchPlayer& getPlayer(int color)
		{
			int idx = (color == StoneColor::eBlack) ? 0 : 1;
			if( bSwapColor )
				idx = 1 - idx;
			return players[idx];
		}

		int  getPlayerColor(int idx)
		{
			if( bSwapColor )
			{
				return (idx == 1) ? StoneColor::eBlack : StoneColor::eWhite;
			}
			return (idx == 0) ? StoneColor::eBlack : StoneColor::eWhite;
		}

		void advanceStep()
		{
			idxPlayerTurn = 1 - idxPlayerTurn;
		}

		void cleanup()
		{
			for( int i = 0; i < 2; ++i )
			{
				auto& bot = players[i].bot;
				if( bot )
				{
					bot->destroy();
					bot.release();
				}
			}
		}

		MatchPlayer&    getCurTurnPlayer()
		{
			return players[idxPlayerTurn];
		}
		IBotInterface* getCurTurnBot()
		{
			return players[idxPlayerTurn].bot.get();
		}
	};

	class MatchSettingPanel : public BaseSettingPanel
	{
		typedef BaseSettingPanel BaseClass;
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
			init();
		}

		void init()
		{
			GChoice* choice;
			choice = addPlayerChoice(0, "Player A");
			choice->modifySelection(0);
			choice = addPlayerChoice(1, "Player B");
			choice->modifySelection(1);

			int sortOrder = 5;
			choice = addChoice(UI_FIXED_HANDICAP, "Fixed Handicap", 0, sortOrder);
			for( int i = 0; i <= 9; ++i )
			{
				FixString<128> str;
				choice->addItem(str.format("%d", i));
			}
			choice->setSelection(0);


			addCheckBox(UI_AUTO_RUN, "Auto Run", 0 , sortOrder);

			adjustChildLayout();

			Vec2i buttonSize = Vec2i(100, 20);
			GButton* button;
			button = new GButton(UI_PLAY, Vec2i(getSize().x /2 - buttonSize.x , getSize().y - buttonSize.y - 5), buttonSize, this);
			button->setTitle("Play");
			button = new GButton(UI_CANCEL, Vec2i((getSize().x)/ 2, getSize().y - buttonSize.y - 5), buttonSize, this);
			button->setTitle("Cancel");
		}

		void addLeelaParamWidget(int id , int idxPlayer );

		void addZenParamWidget(int id, int idxPlayer);

		GChoice* addPlayerChoice(int idxPlayer, char const* title);

		virtual bool onChildEvent(int event, int id, GWidget* ui) override
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

		bool setupMatchSetting(MatchGameData& matchData , GameSetting& setting);
	};


	class BoardFrame : public GFrame
	{
	public:
		typedef GFrame BaseClass;

		BoardFrame(int id, Vec2i const& pos, Vec2i const size, GWidget* parent)
			:GFrame(id, pos, size, parent)
			,renderContext( game.getBoard() , getWorldPos() + GetBorder(), CalcRenderScale(size) )
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
		virtual void onRender()
		{
			BaseClass::onRender();
			GLGraphics2D& g = ::Global::getGLGraphics2D();
			{
				TGuardValue<bool> gurdValue(renderer->bDrawCoord , false);
				renderer->drawBorad(g, renderContext);
			}
		}
		virtual void onChangePos(Vec2i const& pos, bool bParentMove) override
		{
			BaseClass::onChangePos(pos, bParentMove);
			renderContext.renderPos = getWorldPos() + GetBorder();
		}


		virtual bool onMouseMsg(MouseMsg const& msg)
		{
			if( msg.onLeftDown() )
			{
				Vec2i pos = renderContext.getCoord(msg.getPos());
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
		MyGame game;
		RenderContext  renderContext;
		BoardRenderer* renderer;
	};

	class LeelaZeroGoStage : public StageBase
	{
		typedef StageBase BaseClass;
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

		MyGame mGame;

		bool bDrawDebugMsg = false;
		bool bDrawFontCacheTexture = false;
		BoardRenderer mBoardRenderer;
		float const RenderBoardScale = 1.2;
		Vector2 const BoardPos = Vector2(50, 50);

		GameMode mGameMode = GameMode::None;
		class UnderCurveAreaProgram* mProgUnderCurveArea = nullptr;

		void toggleAnalysisPonder();
		bool tryEnableAnalysis();
		bool canAnalysisPonder(MatchPlayer& player)
		{
			if( player.type == ControllerType::ePlayer )
				return true;
			if( player.bot && !player.bot->isGPUBased() )
				return true;
			return false;
		}

		template< class Fun >
		void executeAnalysisAICommand(Fun&& fun, bool bKeepPonder = true)
		{
			assert(bAnalysisEnabled);

			if( bAnalysisPondering )
				mLeelaAIRun.stopPonder();

			fun();

			analysisResult.clear();
			if( bAnalysisPondering && bKeepPonder )
			{
				analysisPonderColor = mGame.getNextPlayColor();
				mLeelaAIRun.startPonder(analysisPonderColor);
			}
			
		}

		bool bAnalysisEnabled = false;
		bool bAnalysisPondering;
		LeelaThinkInfoVec analysisResult;
		LeelaThinkInfo bestThinkInfo;

		int  analysisPonderColor = 0;
		int  showBranchVertex = -1;


		bool bSwapEachMatch = true;
		bool bAutoSaveMatchSGF = true;
		int  unknownWinerCount = 0;
		MatchGameData mMatchData;
		FixString<32> mLastGameResult;


		void drawWinRateDiagram( Vec2i const& renderPos ,  Vec2i const& renderSize );



		std::vector< Vector2 > mWinRateHistory[2];
		
		int     bestMoveVertex;
		
		bool    bReviewingGame = false;
		MyGame  mReviewGame;
		bool    bTryPlayingGame = false;
		MyGame  mTryPlayGame;
		BoardFrame* mTryPlayWidget = nullptr;
		Vec2i   mTryPlayWidgetPos = Vec2i(0,0);

		GWidget* mGamePlayWidget = nullptr;
		GWidget* mWinRateWidget = nullptr;
		bool    mbRestartLearning = false;

		MyGame& getViewingGame()
		{
			if( bTryPlayingGame )
				return mTryPlayGame;
			else if( bReviewingGame )
				return mReviewGame;
			return mGame;
		}

		bool saveMatchGameSGF();
	

		virtual bool onInit();
		virtual void onEnd();
		virtual void onUpdate(long time);

		void onRender(float dFrame);


		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override;

		bool onMouse(MouseMsg const& msg);


		virtual bool onKey(unsigned key, bool isDown) override;


		void tick() {}
		void updateFrame(int frame) {}

		void cleanupModeData(bool bEndStage = false);
		void keepLeelaProcessRunning(long time);

		void processLearningCommand();
		void notifyPlayerCommand(int indexPlayer, GameCommand const& com);

		bool bPrevGameCom = false;
		void resetGameParam()
		{
			bPrevGameCom = true;
			mGame.restart();
			mBoardRenderer.generateNoiseOffset(mGame.getBoard().getSize());

			resetTurnParam();
			for( int i = 0 ; i < 2 ; ++i )
			{
				mWinRateHistory[i].clear();
				mWinRateHistory[i].push_back(Vector2(0, 50));
			}
			if( mWinRateWidget )
			{
				mWinRateWidget->destroy();
				mWinRateWidget = nullptr;
			}
		}
		void resetTurnParam()
		{
			bestMoveVertex = -3;
		}
		void restartAutoMatch();

		bool buildLearningMode();
		bool buildAnalysisMode();
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
			return !isPlayerControl(mGame.getNextPlayColor());
		}

		bool isPlayerControl(int color)
		{
			return !mMatchData.players[color - 1].isBot();
		}
	};

}//namespace Go


#endif // LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045
