#pragma once
#ifndef LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045
#define LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045

#include "Stage/TestStageHeader.h"

#include "Go/GoCore.h"
#include "Go/GoRenderer.h"
#include "Go/GoBot.h"
#include "ZenBot.h"
#include "LeelaBot.h"
#include "WidgetUtility.h"

#include "Misc/Guid.h"

#include "GameSettingPanel.h"


using namespace RenderGL;

#define DETECT_LEELA_PROCESS 1

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
	};

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
			GButton* button = new GButton(UI_ANY, pos, Vec2i(200, size.y), this);
			button->setTitle("..");
			button->onEvent = [this](int event, GWidget*) -> bool
			{
				SystemPlatform::OpenFileName(filePath, filePath.max_size(), nullptr);
				return false;
			};
		}

		FixString<512> filePath;

		virtual void onRender() override
		{
			IGraphics2D& g = ::Global::getIGraphics2D();

			Vec2i pos = getWorldPos();
			Vec2i size;
			g.drawText(pos, filePath);
		}

	};



	struct MatchGameData
	{
		MatchPlayer players[2];
		int         idxPlayerTurn = 0;
		bool        bSwapColor = false;

		void advanceTurn()
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
			UI_CONTROLLER_TYPE_A ,
			UI_CONTROLLER_TYPE_B ,

		};
		MatchSettingPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(id , pos , size , parent)
		{
			init();
		}

		void init()
		{
			GChoice* choice;
			choice = addChoice(UI_FIXED_HANDICAP, "Fixed Handicap");
			for( int i = 0; i <= 9; ++i )
			{
				FixString<128> str;
				choice->addItem( str.format("%d" , i )  );
			}
			choice->setSelection(0);
			choice = addPlayerChoice(UI_CONTROLLER_TYPE_A, "Player A");
			choice->setSelection(0);
			choice = addPlayerChoice(UI_CONTROLLER_TYPE_B, "Player B");
			choice->setSelection(1);

			adjustChildLayout();

			Vec2i buttonSize = Vec2i(100, 20);
			GButton* button;
			button = new GButton(UI_PLAY, Vec2i(getSize().x /2 - buttonSize.x , getSize().y - buttonSize.y - 5), buttonSize, this);
			button->setTitle("Play");
			button = new GButton(UI_CANCEL, Vec2i((getSize().x)/ 2, getSize().y - buttonSize.y - 5), buttonSize, this);
			button->setTitle("Cancel");
		}

		GChoice* addPlayerChoice(int id, char const* title)
		{
			GChoice* choice = addChoice(id , title);
			choice->addItem("Player");
			choice->addItem("LeelaZero");
			choice->addItem("AQ");
			choice->addItem("Zen 7");
			choice->addItem("Zen 6");
			choice->addItem("Zen 4");
			return choice;
		}

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

		bool setupMatchSetting(MatchGameData& matchData , GameSetting& setting)
		{
			ControllerType types[2] =
			{
				(ControllerType)findChildT<GChoice>(UI_CONTROLLER_TYPE_A)->getSelection(),
				(ControllerType)findChildT<GChoice>(UI_CONTROLLER_TYPE_B)->getSelection()
			};

			for( int i = 0; i < 2; ++i )
			{
				if( !matchData.players[i].initialize(types[i]) )
					return false;
			}

			setting.fixedHandicap = findChildT<GChoice>(UI_FIXED_HANDICAP)->getSelection();
			setting.bBlackFrist = setting.fixedHandicap == 0;
			setting.boardSize = 19;
			setting.komi = (setting.fixedHandicap) ? 0.5 : 7.5;
			return true;
		}
	};

	class LeelaZeroGoStage : public StageBase
		                   , public IGameCommandListener
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
			None ,
		};

		LeelaAppRun   mLearningAIRun;

		bool bProcessPaused = false;
		int  numGameCompleted = 0;
		bool bMatchJob = false;
#if DETECT_LEELA_PROCESS
		DWORD  mPIDLeela = -1;
		static long const LeelaRebootTime = 20000;
		static long const LeelaRebootStartTime = 40 * LeelaRebootTime;
		long   mLeelaRebootTimer = LeelaRebootTime;
#endif

		MyGame mGame;

		bool bDrawDebugMsg = false;
		GameRenderer mGameRenderer;

		GameMode mGameMode;
		MatchGameData mMatchData;
		FixString<32> mLastGameResult;

		
		
		bool    bReviewingGame = false;
		MyGame  mReviewGame;
		bool    bTryPlayingGame = false;
		MyGame  mTryPlayGame;

		GWidget* mGamePlayWidget = nullptr;


		MyGame& getViewingGame()
		{
			if( bTryPlayingGame )
				return mTryPlayGame;
			else if( bReviewingGame )
				return mReviewGame;
			return mGame;
		}


		virtual bool onInit();

		virtual void onEnd();
		virtual void onUpdate(long time);

		void onRender(float dFrame);


		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override;

		bool onMouse(MouseMsg const& msg);


		virtual bool onKey(unsigned key, bool isDown) override;


		void tick() {}
		void updateFrame(int frame) {}

		void cleanupModeData();

		void keepLeelaProcessRunning(long time);

		void processLearningCommand();
		virtual void notifyCommand(GameCommand const& com) override;



		bool buildLearningMode();
		bool buildPlayMode();
		bool buildLeelaMatchMode();
		bool buildLeelaZenMatchMode();

		bool startMatchGame(GameSetting const& setting);

		void createPlayWidget();

		Vec2i const BoardPos = Vec2i(50, 50);

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
