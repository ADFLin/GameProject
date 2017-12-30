#include "LeelaZeroGoStage.h"

#include "StringParse.h"
#include "RandomUtility.h"
#include "WidgetUtility.h"
#include "GameGlobal.h"
#include "PropertyKey.h"

#include "GLGraphics2D.h"
#include "RenderGL/GLCommon.h"
#include "RenderGL/DrawUtility.h"
#include "RenderGL/GpuProfiler.h"

REGISTER_STAGE("LeelaZero Learning", Go::LeelaZeroGoStage, EStageGroup::Test);

#include <Dbghelp.h>
#pragma comment (lib,"Dbghelp.lib")

namespace Go
{
	bool DumpFun( char const* path , char const* outPath )
	{
		std::ifstream fileInput{ path };
		if( !fileInput.is_open() )
			return false;

		std::ofstream fileOutput{ outPath };
		if( !fileOutput.is_open() )
			return false;

		fileInput.peek();
		while(  fileInput.good() )
		{
			char buffer[1024];
			fileInput.getline(buffer, ARRAY_SIZE(buffer));

			char outNames[1024];
			int num = UnDecorateSymbolName(buffer, outNames, ARRAY_SIZE(outNames), UNDNAME_COMPLETE);
			if( num != 0 )
			{
				fileOutput << outNames << '\n';
			}
			fileInput.peek();

		}
		
		return true;
	}
	

	bool LeelaZeroGoStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		if( !::Global::getDrawEngine()->startOpenGL(true) )
			return false;

		if( !mGameRenderer.initializeRHI() )
			return false;

#if 0
		if( !DumpFun("Zen7.dump", "aa.txt") )
			return false;
#endif

		LeelaAIRun::LeelaZeroDir = Global::GameConfig().getStringValue("InstallDir", "LeelaZero", "E:/Desktop/LeelaZero");

		::Global::GUI().cleanupWidget();

		mGame.setup(19);
		mGameRenderer.generateNoiseOffset(mGame.getBoard().getSize());

#if 1
		if( !buildLearningMode() )
			return false;
#else
		if( !buildPlayMode() )
			return false;
#endif

		auto devFrame = WidgetUtility::CreateDevFrame();
		devFrame->addButton("Pause Process", [&](int eventId, GWidget* widget) -> bool
		{
			if( bProcessPaused )
			{
				if( mLearningAIRun.process.resume() )
				{
					GUI::CastFast<GButton>(widget)->setTitle("Pause Process");
					bProcessPaused = false;
				}
			}
			else
			{
				if( mLearningAIRun.process.suspend() )
				{
					GUI::CastFast<GButton>(widget)->setTitle("Resume Process");
					bProcessPaused = true;
				}
			}
			return false;
		});
		devFrame->addButton("Play Game", [&](int eventId, GWidget* widget) -> bool
		{
			cleanupModeData();
			buildPlayMode();
			return false;
		});
		devFrame->addButton("Run Learning", [&](int eventId, GWidget* widget) -> bool
		{
			if( mGameMode != GameMode::Learning )
			{
				cleanupModeData();
				buildLearningMode();
			}
			return false;
		});
		devFrame->addButton("Run PC Match", [&](int eventId, GWidget* widget) -> bool
		{
			cleanupModeData();
			buildLeelaMatchMode();
			return false;
		});
		devFrame->addButton("Run Bot Match", [&](int eventId, GWidget* widget) -> bool
		{
			cleanupModeData();
			buildLeelaZenMatchMode();
			return false;
		});
		devFrame->addButton("Run Custom Match", [&](int eventId, GWidget* widget) ->bool 
		{
			cleanupModeData();
			MatchSettingPanel* panel = new MatchSettingPanel( UI_ANY , Vec2i( 100 , 100 ) , Vec2i(300 , 400 ) , nullptr );
			panel->onEvent = [&](int eventId, GWidget* widget)
			{
				auto* panel = widget->cast<MatchSettingPanel>();
				GameSetting setting;
				if( panel->buildMatchSetting(*this, setting) )
				{
					startMatchGame(setting);
				}
				return false;
				
			};
			::Global::GUI().addWidget(panel);
			return false;
		});

		return true;
	}

	void LeelaZeroGoStage::onEnd()
	{
		cleanupModeData();
		mGameRenderer.releaseRHI();
		::Global::getDrawEngine()->stopOpenGL(true);
		BaseClass::onEnd();
	}

	void LeelaZeroGoStage::onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		if( mGameMode == GameMode::Match )
		{
			IBotInterface* bot = mPlayers[mIdxPlayerTurn].bot.get();
			if( bot )
			{
				bot->update(*this);
			}
		}
		else if( mGameMode == GameMode::Learning )
		{
			processLearningCommand();
			keepLeelaProcessRunning(time);
		}

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void LeelaZeroGoStage::onRender(float dFrame)
	{
		using namespace RenderGL;

		using namespace Go;

		GLGraphics2D& g = ::Global::getGLGraphics2D();
		g.beginRender();

		GpuProfiler::getInstance().beginFrame();

		glClear(GL_COLOR_BUFFER_BIT);

		if( bViewReplay )
		{
			mGameRenderer.draw(BoardPos, mReplayGame);
		}
		else
		{
			mGameRenderer.draw(BoardPos, mGame);
		}

		GpuProfiler::getInstance().endFrame();

		g.endRender();

		g.beginRender();

		FixString< 512 > str;

		g.setTextColor(255, 255, 0);
		RenderUtility::SetFont(g, FONT_S8);
		g.drawText(10, 20, str.format("Num Learned Game = %d", NumLearnedGame));
		g.setTextColor(255, 0, 0);
		RenderUtility::SetFont(g, FONT_S10);
		int const offset = 15;
		int textX = 300;
		int y = 10;

		str.format("bUseBatchedRender = %s", mGameRenderer.bUseBatchedRender ? "true" : "false");
		g.drawText(textX, y += offset, str);
		for( int i = 0; i < GpuProfiler::getInstance().getSampleNum(); ++i )
		{
			GpuProfileSample* sample = GpuProfiler::getInstance().getSample(i);
			str.format("%.8lf => %s", sample->time, sample->name.c_str());
			g.drawText(textX + 10 * sample->level, y += offset, str);

		}
		g.endRender();
	}

	bool LeelaZeroGoStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch( id )
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	bool LeelaZeroGoStage::onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;

		if( msg.onLeftDown() )
		{
			if( canPlay() )
			{
				Vec2i pos = (msg.getPos() - BoardPos + Vec2i(mGameRenderer.CellSize, mGameRenderer.CellSize) / 2) / mGameRenderer.CellSize;
				execPlayStoneCommand(pos);
			}
		}
		return true;
	}

	bool LeelaZeroGoStage::onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;
		switch( key )
		{
		case Keyboard::eR: restart(); break;
		case Keyboard::eL: mGameRenderer.bDrawLinkInfo = !mGameRenderer.bDrawLinkInfo; break;
		case Keyboard::eZ: mGameRenderer.bUseBatchedRender = !mGameRenderer.bUseBatchedRender; break;
		}
		return false;
	}

	bool LeelaZeroGoStage::buildLearningMode()
	{
		if( !mLearningAIRun.buildLearningGame() )
			return false;

		mGameMode = GameMode::Learning;
		mGame.setSetting(GameSetting());
		mGame.restart();

#if DETECT_LEELA_PROCESS
		mRestartTimer = RestartTime;
		mPIDLeela = -1;
#endif
		return true;
	}

	bool LeelaZeroGoStage::buildPlayMode()
	{
		std::string bestWeigetName = LeelaAIRun::GetBestWeightName();
		char const* weightName = nullptr;
		if( bestWeigetName != "" )
			weightName = bestWeigetName.c_str();
		else
			weightName = Global::GameConfig().getStringValue("WeightData", "LeelaZero", "e671af6f29f0acce07405b51d946e1e69a3249f24194a052cd2da2325fa8a332");

		mPlayers[0].type = ControllerType::ePlayer;

		{
			mPlayers[1].type = ControllerType::eLeelaZero;
			LeelaAISetting setting;
			setting.weightName = weightName;
			if( !buildPlayerBot(mPlayers[1], &setting) )
				return false;
		}

		GameSetting setting;
		if( !startMatchGame(setting) )
			return false;

		return true;
	}

	bool LeelaZeroGoStage::buildLeelaMatchMode()
	{
		//char const* weigthNameA = "eebb910dc02d50437b8fe78c43e4f77b3428b092e9477163aa16403238cea918";
		char const* weightNameA = "aaa33e89d06aeeb2e2dca276f2e19e5cdd0d5a3d761d7638eedde11c62125003";
		std::string bestWeigetName = LeelaAIRun::GetBestWeightName();
		char const* weightNameB = bestWeigetName.c_str();

		{
			mPlayers[0].type = ControllerType::eLeelaZero;
			LeelaAISetting setting;
			setting.weightName = weightNameA;
			if( !buildPlayerBot(mPlayers[0], &setting) )
				return false;
		}

		{
			mPlayers[1].type = ControllerType::eLeelaZero;
			LeelaAISetting setting;
			setting.weightName = weightNameB;
			if( !buildPlayerBot(mPlayers[1], &setting) )
				return false;
		}

		GameSetting setting;
		if( !startMatchGame(setting) )
			return false;

		return true;
	}

	bool LeelaZeroGoStage::buildLeelaZenMatchMode()
	{
		{
			mPlayers[0].type = ControllerType::eLeelaZero;
			if( !buildPlayerBot(mPlayers[0], nullptr) )
				return false;
		}

		{
			mPlayers[1].type = ControllerType::eZenV7;
			if( !buildPlayerBot(mPlayers[1], nullptr) )
				return false;
		}
		GameSetting setting;
		setting.fixedHandicap = 9;
		setting.bBlackFrist = false;
		if( !startMatchGame(setting) )
			return false;

		return true;
	}



	bool LeelaZeroGoStage::startMatchGame(GameSetting const& setting)
	{
		mGameMode = GameMode::Match;

		mGame.setSetting(setting);
		mGame.restart();
		mGameRenderer.generateNoiseOffset(mGame.getBoard().getSize());

		bool bHavePlayerController = false;
		for( int i = 0; i < 2; ++i )
		{
			IBotInterface* bot = mPlayers[i].bot.get();
			if( bot )
			{
				bot->setupGame(setting);
			}
			else
			{
				bHavePlayerController = true;
			}
		}


		if( bHavePlayerController )
		{
			createPlayWidget();
		}

		mIdxPlayerTurn = (setting.bBlackFrist) ? 0 : 1;
		if( mPlayers[mIdxPlayerTurn].bot )
		{
			mPlayers[mIdxPlayerTurn].bot->thinkNextMove(mGame.getNextPlayColor());
		}

		return true;
	}

	void LeelaZeroGoStage::cleanupModeData()
	{
		mGameMode = GameMode::None;
		if( mGamePlayWidget )
		{
			mGamePlayWidget->show(false);
		}
		bProcessPaused = false;
		if( mGameMode == GameMode::Match )
		{
			for( int i = 0; i < 2; ++i )
			{
				auto& bot = mPlayers[i].bot;
				if ( bot )
				{
					bot->destroy();
					bot.release();
				}
			}
		}
		else
		{
			mLearningAIRun.stop();
		}
	}

	void LeelaZeroGoStage::keepLeelaProcessRunning(long time)
	{
#if DETECT_LEELA_PROCESS
		if( mPIDLeela == -1 )
		{
			mPIDLeela = FPlatformProcess::FindPIDByNameAndParentPID(TEXT("leelaz.exe"), GetProcessId(mLearningAIRun.process.getHandle()));
			if( mPIDLeela == -1 )
			{
				mRestartTimer -= time;
			}
		}
		else
		{
			if( !FPlatformProcess::IsProcessRunning(mPIDLeela) )
			{
				mRestartTimer -= time;
			}
			else
			{
				mRestartTimer = RestartTime;
			}
		}

		if( mRestartTimer < 0 )
		{
			static int killCount = 0;
			++killCount;
			cleanupModeData();
			buildLearningMode();
		}
#endif
	}

	void LeelaZeroGoStage::processLearningCommand()
	{
		auto ProcFun = [&](GameCommand& com)
		{
			int color = mGame.getNextPlayColor();
			switch( com.id )
			{
			case GameCommand::eStart:
				{
					::Msg("GameStart");
					mGame.restart();
					mGameRenderer.generateNoiseOffset(mGame.getBoard().getSize());
					if( mGameMode == GameMode::Learning )
					{
						++NumLearnedGame;
#if DETECT_LEELA_PROCESS
						mPIDLeela = -1;
						mRestartTimer = RestartTime;
#endif
					}
				}
				break;
			case GameCommand::ePass:
				{
					::Msg("Pass");
					mGame.playPass();
				}
				break;
			case GameCommand::eResign:
				{
					char const* name = (color == StoneColor::eBlack) ? "Black" : "White";
					if( mGameMode == GameMode::Learning )
					{
						::Msg("%s Resigned", name);
					}
					else
					{
						FixString< 128 > str;
						str.format("%s Resigned", name);
						::Global::GUI().showMessageBox(UI_ANY, str, GMB_OK);
					}
				}
				break;
			default:
				{
					if( mGame.playStone(com.pos[0], com.pos[1]) )
					{

					}
					else
					{
						::Msg("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
					}

				}
				break;
			}
		};

		mLearningAIRun.outputThread->procCommand(ProcFun);
	}


	void LeelaZeroGoStage::notifyCommand(GameCommand const& com)
	{
		int color = mGame.getNextPlayColor();
		switch( com.id )
		{
		case GameCommand::ePass:
			{
				::Msg("Pass");
				mGame.playPass();

				if( mGame.getLastPassCount() >= 2 )
				{
					::Msg("GameEnd");
					if( mPlayers[0].type == ControllerType::eLeelaZero ||
					    mPlayers[1].type == ControllerType::eLeelaZero )
					{
						LeelaBot* bot = static_cast< LeelaBot* >( (mPlayers[0].type == ControllerType::eLeelaZero ) ?mPlayers[0].bot.get() : mPlayers[1].bot.get());
						bot->mAI.inputProcessStream("final_score\n");
					}
				}
				else
				{
					mIdxPlayerTurn = 1 - mIdxPlayerTurn;

					IBotInterface* bot = mPlayers[mIdxPlayerTurn].bot.get();
					if( bot )
					{
						bot->playPass(color);
						bot->thinkNextMove(mGame.getNextPlayColor());
					}
				}
			}
			break;
		case GameCommand::eResign:
			{
				char const* name = (color == StoneColor::eBlack) ? "Black" : "White";
				FixString< 128 > str;

				str.format("%s Resigned", name);
				::Global::GUI().showMessageBox(UI_ANY, str , GMB_OK);
			}
			break;
		case GameCommand::eUndo:
			{
				mGame.undo();
				mIdxPlayerTurn = 1 - mIdxPlayerTurn;
				IBotInterface* bot = mPlayers[mIdxPlayerTurn].bot.get();
				if( bot )
				{
					bot->undo();
				}
			}
			break;
		default:
			{
				if( mGame.playStone(com.pos[0], com.pos[1]) )
				{
					mIdxPlayerTurn = 1 - mIdxPlayerTurn;
					IBotInterface* bot = mPlayers[mIdxPlayerTurn].bot.get();
					if( bot )
					{
						bot->playStone(Vec2i(com.pos[0], com.pos[1]), color);
						bot->thinkNextMove(mGame.getNextPlayColor());
					}
				}
				else
				{
					::Msg("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
				}

			}
			break;
		}
	}

	void LeelaZeroGoStage::createPlayWidget()
	{
		if( mGamePlayWidget == nullptr )
		{
			Vec2i screenSize = Global::getDrawEngine()->getScreenSize();

			Vec2i size = Vec2i(150,200);
			auto devFrame = new DevFrame(UI_ANY, Vec2i(screenSize.x - size.x - 5, 300), size , NULL);

			devFrame->addButton("Play Pass", [&](int eventId, GWidget* widget) -> bool
			{
				int color = mGame.getNextPlayColor();
				if( canPlay() )
				{
					execPassCommand();
				}
				return false;
			});
			devFrame->addButton("Undo", [&](int eventId, GWidget* widget) -> bool
			{
				int color = mGame.getNextPlayColor();
				if( canPlay() || ( isBotTurn() && !mPlayers[mIdxPlayerTurn].bot->isThinking() ) )
				{
					execUndoCommand();
				}
				return false;
			});

			::Global::GUI().addWidget(devFrame);
			mGamePlayWidget = devFrame;
		}

		mGamePlayWidget->show();
	}

	void LeelaZeroGoStage::execPlayStoneCommand(Vec2i const& pos)
	{
		int color = mGame.getNextPlayColor();
		assert(isPlayerControl(color));

		GameCommand com;
		com.pos[0] = pos.x;
		com.pos[1] = pos.y;
		notifyCommand(com);
	}

	void LeelaZeroGoStage::execPassCommand()
	{
		int color = mGame.getNextPlayColor();
		assert(isPlayerControl(color));
		GameCommand com;
		com.id = GameCommand::ePass;
		notifyCommand(com);
	}

	void LeelaZeroGoStage::execUndoCommand()
	{
		int color = mGame.getNextPlayColor();
		//assert(isPlayerControl(color));
		GameCommand com;
		com.id = GameCommand::eUndo;
		notifyCommand(com);
	}

}//namespace Go