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

#include "Core/FNV1a.h"
#include "Misc/Guid.h"
#include "FileSystem.h"
#include "Delegate.h"

#include "GameSettingPanel.h"


using namespace Render;

#define DETECT_LEELA_PROCESS 0

namespace Go
{

	class GoReplayFrame : public GFrame
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
		case ControllerType::ePlayer:
			return "Player";
		case ControllerType::eLeelaZero:
			return "LeelaZero";
		case ControllerType::eAQ:
			return "AQ";
		case ControllerType::eZenV7:
			return "Zen7";
		case ControllerType::eZenV6:
			return "Zen6";
		case ControllerType::eZenV4:
			return "Zen4";
		}

		return "Unknown";
	}


	class GameProxy;

	typedef std::function< void(GameProxy& game) > GameEventDelegate;
	
	class GameProxy
	{

	public:
		GameProxy()
		{
			guid = Guid::New();
		}

		Guid guid;
		GameEventDelegate onStateChanged;

		void setup(int size) { mGame.setup(size); }
		void setSetting(GameSetting const& setting) { mGame.setSetting(setting); }
		void restart()
		{
			guid = Guid::New();
			mGame.restart();
			if( onStateChanged )
				onStateChanged(*this);
		}
		bool canPlay(int x, int y) const { return mGame.canPlay(x, y); }
		bool playStone(int x, int y)
		{
			if( mGame.playStone(x, y) )
			{
				if( onStateChanged )
					onStateChanged(*this);
				return true;
			}
			return false;
		}
		bool addStone(int x, int y, int color)
		{
			if( mGame.addStone(x, y, color) )
			{
				if( onStateChanged )
					onStateChanged(*this);
				return true;
			}
			return false;
		}
		void    playPass()
		{
			mGame.playPass();
			if( onStateChanged )
				onStateChanged(*this);
		}
		bool undo()
		{
			if( mGame.undo() )
			{
				if( onStateChanged )
					onStateChanged(*this);
				return true;
			}
			return false;
		}

		int reviewBeginStep()
		{ 
			int result = mGame.reviewBeginStep();
			if( result && onStateChanged )
				onStateChanged(*this);
			return result;
		}
		int reviewPrevSetp(int numStep = 1) 
		{ 
			int result = mGame.reviewPrevSetp(numStep);
			if( result && onStateChanged )
				onStateChanged(*this);
			return result;
		}
		int reviewNextStep(int numStep = 1)
		{ 
			int result = mGame.reviewNextStep(numStep);
			if( result && onStateChanged )
				onStateChanged(*this);
			return result;
		}
		int reviewLastStep()
		{
			int result = mGame.reviewLastStep();
			if( result && onStateChanged )
				onStateChanged(*this);
			return result;
		}
		void copy(GameProxy& otherGame, bool bRemoveUnplayedHistory = false)
		{
			guid = otherGame.guid;
			mGame.copy(otherGame.mGame);
			if( bRemoveUnplayedHistory )
				mGame.removeUnplayedHistory();
		}
		void synchronizeHistory(GameProxy& other)
		{
			if( guid != other.guid )
			{
				LogWarning(0 , "Can't synchronize step history");
				return;
			}
			mGame.synchronizeHistory(other.mGame);
		}
		Board const& getBoard() const { return mGame.getBoard(); }
		GameSetting const& getSetting() const { return mGame.getSetting(); }
		Game const& getInstance() { return mGame; }

	private:
		Game mGame;
	};

	struct MatchParamKey
	{
		uint64 mHash;
		bool operator < (MatchParamKey const& other) const
		{
			return mHash < other.mHash;
		}
		bool operator == (MatchParamKey const& other) const
		{
			return mHash == other.mHash;
		}

		void setValue(std::string const&paramString)
		{
			mHash = FNV1a::MakeStringHash< uint64 >(paramString.c_str());
		}
	};

	struct MatchPlayer
	{
		ControllerType type;
		std::unique_ptr< IBotInterface > bot;
		
		std::string    paramString;
		MatchParamKey  paramKey;
		int winCount;

		FixString< 128 > getName() const;
		bool   isBot() const { return type != ControllerType::ePlayer; }
		bool   initialize(ControllerType inType , void* botSetting = nullptr);


		static std::string GetParamString(ControllerType inType, void* botSetting);
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




	class MatchResultMap
	{
	public:
		enum Version
		{

			DATA_LAST_VERSION_PLUS_ONE ,
			DATA_LAST_VERSION = DATA_LAST_VERSION_PLUS_ONE - 1,
		};
		struct ResultData
		{
			std::string playerSetting[2];
			std::string gameSetting;
			uint32 winCounts[2];

			template< class OP >
			void serialize(OP op)
			{
				int version = DATA_LAST_VERSION;
				op & version;
				op & winCounts[0] & winCounts[1];
				op & gameSetting & playerSetting[0] & playerSetting[1];
			}

			friend DataSerializer& operator << (DataSerializer& serializer, ResultData const& data)
			{
				const_cast<ResultData&>(data).serialize(DataSerializer::WriteOp(serializer));
				return serializer;
			}
			friend DataSerializer& operator >> (DataSerializer& serializer, ResultData& data)
			{
				data.serialize(DataSerializer::ReadOp(serializer));
				return serializer;
			}
		};

		struct MatchKey
		{
			MatchParamKey playerParamKeys[2];
			bool setValue(MatchPlayer players[2] , GameSetting const& gameSetting )
			{
				playerParamKeys[0] = players[0].paramKey;
				playerParamKeys[1] = players[1].paramKey;
				if( playerParamKeys[1] < playerParamKeys[0] )
				{
					std::swap(playerParamKeys[0] , playerParamKeys[1]);
					return true;
				}
				return false;
			}

			bool operator < (MatchKey const& other) const
			{
				assert(!(playerParamKeys[1] < playerParamKeys[0]));
				assert(!(other.playerParamKeys[1] < other.playerParamKeys[0]));

				if( playerParamKeys[1] < other.playerParamKeys[0] )
					return true;
				if ( !( playerParamKeys[0] < other.playerParamKeys[0] ) )
					return playerParamKeys[1] < other.playerParamKeys[1];
				return false;
			}
		};

		ResultData* getMatchResult(MatchPlayer players[2], GameSetting const& gameSetting , bool& bSwap )
		{
			MatchKey key;
			bSwap = key.setValue(players, gameSetting);

			auto iter = mDataMap.find(key);
			if( iter == mDataMap.end() )
				return nullptr;

			return &iter->second;
		}

		static std::string ToString(GameSetting const& gameSetting)
		{
			FixString<128> result;
			result.format("size=%d handicap=%d komi=%f start=%c",
						  gameSetting.boardSize, gameSetting.numHandicap,
						  gameSetting.komi, gameSetting.bBlackFrist ? 'b' : 'w');
			return result.c_str();
		}

		void addMatchResult(MatchPlayer players[2] , GameSetting const& gameSetting )
		{
			MatchKey key;
			key.setValue(players, gameSetting);

			auto& resultData = mDataMap[key];

			resultData.gameSetting = ToString( gameSetting );

			if( key.playerParamKeys[0] == players[0].paramKey )
			{
				resultData.winCounts[0] += players[0].winCount;
				resultData.winCounts[1] += players[1].winCount;
				resultData.playerSetting[0] = players[0].paramString;
				resultData.playerSetting[1] = players[1].paramString;
			}
			else
			{
				resultData.winCounts[1] += players[0].winCount;
				resultData.winCounts[0] += players[1].winCount;
				resultData.playerSetting[1] = players[0].paramString;
				resultData.playerSetting[0] = players[1].paramString;
			}
		}

		bool save(char const* path)
		{
			OutputFileStream stream;
			if( !stream.open(path) )
				return false;

			DataSerializer serializer(stream);
			serializer << mDataMap;

			return false;
		}
		bool load(char const* path)
		{
			InputFileStream stream;
			if( !stream.open(path) )
				return false;

			DataSerializer serializer( stream );
			serializer >> mDataMap;

			return false;
		}

		std::map< MatchKey, ResultData > mDataMap;
	};

	struct MatchGameData
	{
		MatchPlayer players[2];
		int         idxPlayerTurn = 0;
		bool        bSwapColor = false;
		bool        bAutoRun = false;
		bool        bSaveSGF = false;
		int         historyWinCounts[2];


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

		BoardFrame( GameProxy& game , int id, Vec2i const& pos, Vec2i const size, GWidget* parent)
			:GFrame(id, pos, size, parent)
			,renderContext(game.getBoard(), getWorldPos() + GetBorder(), CalcRenderScale(size))
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
			GLGraphics2D& g = ::Global::GetRHIGraphics2D();
			{
				TGuardValue<bool> gurdValue(renderer->bDrawCoord, false);
				renderer->drawBorad(g, renderContext);
			}
		}
		virtual void onChangePos(Vec2i const& pos, bool bParentMove) override
		{
			BaseClass::onChangePos(pos, bParentMove);
			renderContext.renderPos = getWorldPos() + GetBorder();
		}
		RenderContext  renderContext;
		BoardRenderer* renderer;
	};

	class TryPlayBoardFrame : public BoardFrame
	{
	public:
		typedef BoardFrame BaseClass;

		TryPlayBoardFrame(int id, Vec2i const& pos, Vec2i const size, GWidget* parent)
			:BoardFrame(game , id, pos, size, parent)
		{

		}

		virtual bool onMouseMsg(MouseMsg const& msg)
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

		GameProxy mGame;

		MatchResultMap mMatchResultMap;

		bool bDrawDebugMsg = false;
		bool bDrawFontCacheTexture = false;
		BoardRenderer mBoardRenderer;
		float const RenderBoardScale = 1.2;
		Vector2 BoardPos = Vector2(100, 55);

		GameMode mGameMode = GameMode::None;
		class UnderCurveAreaProgram* mProgUnderCurveArea = nullptr;

		bool toggleAnalysisPonder();
		bool tryEnableAnalysis(bool bCopyGame);

		bool canAnalysisPonder(MatchPlayer& player)
		{
			if( player.type == ControllerType::ePlayer )
				return true;
			if( player.bot && !player.bot->isGPUBased() )
				return true;
			return false;
		}

		template< class Fun >
		void executeAnalysisAICommand(Fun&& fun, bool bKeepPonder = true);

		bool bAnalysisEnabled = false;
		bool bAnalysisPondering;
		LeelaThinkInfoVec analysisResult;
		LeelaThinkInfo bestThinkInfo;

		int  analysisPonderColor = 0;
		int  showBranchVertex = -1;


		bool bSwapEachMatch = true;
		int  unknownWinerCount = 0;
		MatchGameData mMatchData;
		FixString<32> mLastGameResult;

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
				if( mBot )
				{
					mBot->release();
				}
			}
			virtual void emitSetup(GameSetting const& setting) override
			{
				mBot->startGame(setting);
			}
			virtual void emitPlayStone(int x, int y, int color) override
			{
				mBot->playStone(x, y, color == StoneColor::eBlack ? Zen::Color::Black : Zen::Color::White);
			}
			virtual void emitAddStone(int x, int y, int color) override
			{
				mBot->addStone(x, y, color == StoneColor::eBlack ? Zen::Color::Black : Zen::Color::White);
			}
			virtual void emitPlayPass(int color) override
			{
				mBot->playPass(color == StoneColor::eBlack ? Zen::Color::Black : Zen::Color::White);
			}
			virtual void emitUndo() override
			{
				mBot->undo();
			}

			bool ensureBotInitialized()
			{
				if( mBot == nullptr )
				{
					mBot.reset(Zen::TBotCore<7>::Create<Zen::TBotCore<7>>());
					if( !mBot->initialize() )
					{
						mBot.release();
						return false;
					}
				}
				return true;
			}
			Guid mLastQueryId;
			int  mNextCopyStep;
			std::unique_ptr< Zen::TBotCore<7> > mBot;



		};

		bool bShowTerritory = false;

		Zen::TerritoryInfo mShowTerritoryInfo;
		GameStatusQuery mStatusQuery;

		void updateViewGameTerritory();

		static void DrawTerritoryStatus( BoardRenderer& renderer , RenderContext const& context ,  Zen::TerritoryInfo const& info);


		std::vector< Vector2 > mWinRateHistory[2];
		
		int     bestMoveVertex;
		
		bool    bReviewingGame = false;
		GameProxy  mReviewGame;
		bool    bTryPlayingGame = false;
		GameProxy  mTryPlayGame;
		TryPlayBoardFrame* mTryPlayWidget = nullptr;
		Vec2i   mTryPlayWidgetPos = Vec2i(0,0);

		GWidget* mGamePlayWidget = nullptr;
		GWidget* mWinRateWidget = nullptr;
		bool    mbRestartLearning = false;

		GameProxy& getViewingGame()
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

		void drawAnalysis(GLGraphics2D& g , RenderContext &context);


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
		void resetGameParam();
		void resetTurnParam();
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
			return !isPlayerControl(mGame.getInstance().getNextPlayColor());
		}

		bool isPlayerControl(int color)
		{
			return !mMatchData.players[color - 1].isBot();
		}
	};


}//namespace Go


#endif // LeelaZeroGoStage_H_4F29E774_E4D8_4238_AE9B_09E80E2C7045
