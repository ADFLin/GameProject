#pragma once
#ifndef LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045
#define LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045

#include "Stage/TestStageHeader.h"

#include "Go/GoCore.h"
#include "Go/GoRenderer.h"
#include "Go/GoBot.h"
#include "ZenBot.h"
#include "LeelaBot.h"

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
		
		eZenV4 ,
		eZenV6 ,
		eZenV7 ,
	};

	struct MatchGameData
	{

		struct PlayerData
		{
			ControllerType type;
			std::unique_ptr< IBotInterface > bot;

			bool isBot() const { return type != ControllerType::ePlayer; }
		};
		PlayerData mPlayers[2];
		int mIdxPlayerTurn = 0;


		static bool buildPlayerBot(PlayerData& playerData, void* botSetting = nullptr)
		{
			switch( playerData.type )
			{
			case ControllerType::eLeelaZero:
				playerData.bot.reset(new LeelaBot());
				break;
			case ControllerType::eZenV4:
				playerData.bot.reset(new ZenBot(4));
				break;
			case ControllerType::eZenV6:
				playerData.bot.reset(new ZenBot(6));
				break;
			case ControllerType::eZenV7:
				playerData.bot.reset(new ZenBot(7));
				break;
			case ControllerType::ePlayer:
				return true;
			}

			if( !playerData.bot )
				return false;

			if( !playerData.bot->initilize(botSetting) )
			{
				playerData.bot.release();
				return false;
			}
			return true;
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

			Vec2i buttonSize = Vec2i(200, 20);
			GButton* button = new GButton(UI_PLAY , Vec2i( ( getSize().x - buttonSize.x ) / 2 , getSize().y - buttonSize.y - 5 ) , buttonSize , this );
			button->setTitle("Play");
		}

		GChoice* addPlayerChoice(int id, char const* title)
		{
			GChoice* choice = addChoice(id , title);
			choice->addItem("Player");
			choice->addItem("LeelaZero");
			choice->addItem("Zen 4");
			choice->addItem("Zen 6");
			choice->addItem("Zen 7");
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
			case UI_CONTROLLER_TYPE_A:
			case UI_CONTROLLER_TYPE_B:
				return false;
			}

			return BaseClass::onChildEvent(event, id, ui);
		}

		bool buildMatchSetting(MatchGameData& matchData , GameSetting& setting)
		{
			setting.fixedHandicap = findChildT<GChoice>(UI_FIXED_HANDICAP)->getSelection();
			setting.bBlackFrist = setting.fixedHandicap == 0;
			setting.boardSize = 19;
			setting.komi = (setting.fixedHandicap) ? 0.5 : 6.5;

			matchData.mPlayers[0].type = (ControllerType)findChildT<GChoice>(UI_CONTROLLER_TYPE_A)->getSelection();
			matchData.mPlayers[1].type = (ControllerType)findChildT<GChoice>(UI_CONTROLLER_TYPE_B)->getSelection();
			for( int i = 0; i < 2; ++i )
			{
				if( !matchData.buildPlayerBot(matchData.mPlayers[i]) )
					return false;
			}

			return true;
		}
	};


	class LeelaZeroGoStage : public StageBase
		                   , public MatchGameData
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

		LeelaAIRun  mLearningAIRun;

		bool bProcessPaused = false;
		int  NumLearnedGame = 0;
#if DETECT_LEELA_PROCESS
		DWORD  mPIDLeela = -1;
		static long const RestartTime = 20000;
		long   mRestartTimer = RestartTime;
#endif

		Go::Game mGame;
		GameRenderer mGameRenderer;


		virtual bool onInit();

		virtual void onEnd();
		virtual void onUpdate(long time);

		void onRender(float dFrame);


		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override;

		bool onMouse(MouseMsg const& msg);


		virtual bool onKey(unsigned key, bool isDown) override;


		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}

		void cleanupModeData();

		void keepLeelaProcessRunning(long time);

		void processLearningCommand();
		virtual void notifyCommand(GameCommand const& com) override;

		GameMode mGameMode;

		bool bViewReplay = false;
		Game mReplayGame;

		GWidget* mGamePlayWidget = nullptr;


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
			return !mPlayers[color - 1].isBot();
		}
	};

}//namespace Go


#endif // LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045
