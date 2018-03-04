#include "LeelaZeroGoStage.h"

#include "StringParse.h"
#include "RandomUtility.h"
#include "WidgetUtility.h"
#include "GameGlobal.h"
#include "PropertyKey.h"
#include "FileSystem.h"

#include "GLGraphics2D.h"
#include "RenderGL/GLCommon.h"
#include "RenderGL/DrawUtility.h"
#include "RenderGL/GpuProfiler.h"

REGISTER_STAGE("LeelaZero Learning", Go::LeelaZeroGoStage, EStageGroup::Test);

#include <Dbghelp.h>
#pragma comment (lib,"Dbghelp.lib")

namespace Go
{

	bool DumpFunSymbol( char const* path , char const* outPath )
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

		ILocalization::Get().changeLanguage(LAN_ENGLISH);

#if 0
		if( !DumpFunSymbol("Zen7.dump", "aa.txt") )
			return false;
#endif
		
		LeelaAppRun::InstallDir = ::Global::GameConfig().getStringValue("LeelaZeroInstallDir", "Go" , "E:/Desktop/LeelaZero");
		AQAppRun::InstallDir = ::Global::GameConfig().getStringValue("AQInstallDir", "Go", "E:/Desktop/AQ");

		FixString<256> path;
		path.format("%s/%s", LeelaAppRun::InstallDir, LeelaAppRun::GetBestWeightName().c_str());
		path.replace('/', '\\');
		//if( SystemPlatform::OpenFileName(path, path.max_size(), nullptr) )
		{
			
		}
		::Global::GUI().cleanupWidget();

		mGame.setup(19);
#if 1
		if( !buildLearningMode() )
			return false;
#else
		if( !buildPlayMode() )
			return false;
#endif

		auto devFrame = WidgetUtility::CreateDevFrame( Vec2i(150, 200 + 30) );
		devFrame->addButton("Pause Process", [&](int eventId, GWidget* widget) -> bool
		{
			if ( mGameMode == GameMode::Learning )
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
		devFrame->addButton("Run Leela/Leela Match", [&](int eventId, GWidget* widget) -> bool
		{
			cleanupModeData();
			buildLeelaMatchMode();
			return false;
		});
		devFrame->addButton("Run Leela/Zen Match", [&](int eventId, GWidget* widget) -> bool
		{
			cleanupModeData();
			buildLeelaZenMatchMode();
			return false;
		});
		devFrame->addButton("Run Custom Match", [&](int eventId, GWidget* widget) ->bool 
		{
			MatchSettingPanel* panel = new MatchSettingPanel( UI_ANY , Vec2i( 100 , 100 ) , Vec2i(300 , 400 ) , nullptr );
			panel->onEvent = [&](int eventId, GWidget* widget)
			{
				cleanupModeData();

				auto* panel = widget->cast<MatchSettingPanel>();
				GameSetting setting;
				if( panel->setupMatchSetting( mMatchData , setting) )
				{
					startMatchGame(setting);
				}
				return false;
				
			};
			::Global::GUI().addWidget(panel);
			return false;
		});
		devFrame->addButton("Review Game", [&](int eventId, GWidget* widget) ->bool
		{
			bReviewingGame = !bReviewingGame;

			if( bReviewingGame )
			{
				mReviewGame.guid = mGame.guid;
				mReviewGame.copy(mGame);
				widget->cast<GButton>()->setTitle("Close Review");
				Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();
				Vec2i widgetSize = Vec2i(100, 300);
				auto frame = new DevFrame(UI_REPLAY_FRAME, Vec2i(screenSize.x - widgetSize.x - 5, 300) , widgetSize , nullptr);
				frame->addButton("[<]", [&](int eventId, GWidget* widget) ->bool
				{
					mReviewGame.reviewBeginStep();
					return false;
				});

				frame->addButton("<<", [&](int eventId, GWidget* widget) ->bool
				{
					mReviewGame.reviewPrevSetp(10);
					return false;
				});

				frame->addButton("<", [&](int eventId, GWidget* widget) ->bool
				{
					mReviewGame.reviewPrevSetp(1);
					return false;
				});

				frame->addButton(">", [&](int eventId, GWidget* widget) ->bool
				{
					if( mGame.guid == mReviewGame.guid )
					{
						mReviewGame.updateHistory(mGame);
					}
					mReviewGame.reviewNextStep(1);
					return false;
				});

				frame->addButton(">>", [&](int eventId, GWidget* widget) ->bool
				{
					if( mGame.guid == mReviewGame.guid )
					{
						mReviewGame.updateHistory(mGame);
					}
					mReviewGame.reviewNextStep(10);
					return false;
				});

				frame->addButton("[>]", [&](int eventId, GWidget* widget) ->bool
				{
					if( mGame.guid == mReviewGame.guid )
					{
						mReviewGame.updateHistory(mGame);
					}
					
					mReviewGame.reviewLastStep();
					return false;
				});
				::Global::GUI().addWidget(frame);
			}
			else
			{
				::Global::GUI().findTopWidget(UI_REPLAY_FRAME)->destroy();
				widget->cast<GButton>()->setTitle("Review Game");
			}
			return false;
		});

		devFrame->addButton("Try Play", [&](int eventId, GWidget* widget) ->bool
		{
			bTryPlayingGame = !bTryPlayingGame;

			if( bTryPlayingGame )
			{
				MyGame& Gamelooking = bReviewingGame ? mReviewGame : mGame;
				mTryPlayGame.guid = Gamelooking.guid;
				mTryPlayGame.copy(Gamelooking);
				mTryPlayGame.removeUnplayedHistory();
				widget->cast<GButton>()->setTitle("End Play");
			}
			else
			{
				widget->cast<GButton>()->setTitle("Try Play");
			}

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
			IBotInterface* bot = mMatchData.getCurTurnBot();
			if( bot )
			{
				bot->update(*this);
			}
		}
		else if( mGameMode == GameMode::Learning )
		{
			mLearningAIRun.update();
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

		GpuProfiler::Get().beginFrame();

		glClear(GL_COLOR_BUFFER_BIT);

		mGameRenderer.draw(BoardPos, getViewingGame() );

		GpuProfiler::Get().endFrame();

		g.endRender();

		g.beginRender();

		FixString< 512 > str;

		g.setTextColor(255, 0, 0);
		RenderUtility::SetFont(g, FONT_S10);
		if( mGameMode == GameMode::Learning )
		{
			g.drawText(200, 20, str.format("Num Game Completed = %d", numGameCompleted));
			g.drawText(200, 35, str.format("Job Type = %s", bMatchJob ? "Match" : "Self Play") );

			if( !mLastGameResult.empty() )
			{
				g.drawText(200, 50, str.format( "Last Game Result : %s" , mLastGameResult.c_str() ) );
			}
		}

		g.setTextColor(255, 0, 0);
		RenderUtility::SetFont(g, FONT_S10);
		int const offset = 15;
		int textX = 300;
		int y = 10;

		if ( bDrawDebugMsg )
		{
			str.format("bUseBatchedRender = %s", mGameRenderer.bUseBatchedRender ? "true" : "false");
			g.drawText(textX, y += offset, str);
			for( int i = 0; i < GpuProfiler::Get().getSampleNum(); ++i )
			{
				GpuProfileSample* sample = GpuProfiler::Get().getSample(i);
				str.format("%.8lf => %s", sample->time, sample->name.c_str());
				g.drawText(textX + 10 * sample->level, y += offset, str);

			}
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
			Vec2i pos = (msg.getPos() - BoardPos + Vec2i(mGameRenderer.CellLength, mGameRenderer.CellLength) / 2) / mGameRenderer.CellLength;
			
			if( bTryPlayingGame )
			{
				mTryPlayGame.playStone(pos.x, pos.y);
			}
			else 
			{
				if( canPlay() )
				{
					execPlayStoneCommand(pos);
				}
			}
		}
		else if( msg.onRightDown() )
		{
			if( bTryPlayingGame )
			{
				mTryPlayGame.undo();
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
		case Keyboard::eL: mGameRenderer.bDrawLinkInfo = !mGameRenderer.bDrawLinkInfo; break;
		case Keyboard::eZ: mGameRenderer.bUseBatchedRender = !mGameRenderer.bUseBatchedRender; break;
		case Keyboard::eF2: bDrawDebugMsg = !bDrawDebugMsg; break;
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
		mGameRenderer.generateNoiseOffset(mGame.getBoard().getSize());

		bMatchJob = false;
#if DETECT_LEELA_PROCESS
		mLeelaRebootTimer = LeelaRebootStartTime;
		mPIDLeela = -1;
#endif
		return true;
	}

	bool LeelaZeroGoStage::buildPlayMode()
	{
		std::string bestWeigetName = LeelaAppRun::GetBestWeightName();
		char const* weightName = nullptr;
		if( bestWeigetName != "" )
			weightName = bestWeigetName.c_str();
		else
			weightName = Global::GameConfig().getStringValue("WeightData", "LeelaZero", "e671af6f29f0acce07405b51d946e1e69a3249f24194a052cd2da2325fa8a332");

		{
			if( !mMatchData.players[0].initialize(ControllerType::ePlayer) )
				return false;
		}

		{
			LeelaAISetting setting;
			setting.weightName = weightName;
			if( !mMatchData.players[1].initialize(ControllerType::eLeelaZero , &setting ) )
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
		char const* weightNameA = ::Global::GameConfig().getStringValue( "LeelaLastOpenWeight" , "Go" , "aaa33e89d06aeeb2e2dca276f2e19e5cdd0d5a3d761d7638eedde11c62125003" );

		FixString<256> path;
		path.format("%s/%s" , LeelaAppRun::InstallDir , weightNameA );
		path.replace('/', '\\');
		if( SystemPlatform::OpenFileName(path, path.max_size(), nullptr) )
		{
			weightNameA = FileUtility::GetDirPathPos(path) + 1;
			::Global::GameConfig().setKeyValue("LeelaLastOpenWeight", "Go", weightNameA);
		}

		std::string bestWeigetName = LeelaAppRun::GetBestWeightName();

		{
			LeelaAISetting setting;
			setting.weightName = weightNameA;
			setting.seed = generateRandSeed();
			setting.playouts = 4000;
			if( !mMatchData.players[0].initialize(ControllerType::eLeelaZero, &setting) )
				return false;
		}

		{
			LeelaAISetting setting;
			setting.weightName = bestWeigetName.c_str();
			setting.seed = generateRandSeed();
			setting.playouts = 4000;
			if( !mMatchData.players[1].initialize(ControllerType::eLeelaZero, &setting) )
				return false;
		}

		GameSetting setting;
		setting.fixedHandicap = 0;
		if ( setting.fixedHandicap )
			setting.bBlackFrist = false;
		
		if( !startMatchGame(setting) )
			return false;

		return true;
	}

	bool LeelaZeroGoStage::buildLeelaZenMatchMode()
	{
		{
			LeelaAISetting setting;
			std::string bestWeigetName = LeelaAppRun::GetBestWeightName();
			setting.weightName = bestWeigetName.c_str();
			setting.seed = generateRandSeed();
			if( !mMatchData.players[0].initialize(ControllerType::eLeelaZero, &setting) )
				return false;
		}

		{
			if( !mMatchData.players[1].initialize(ControllerType::eZenV7 , nullptr) )
				return false;
		}

		GameSetting setting;
		setting.fixedHandicap = 0;
		if( setting.fixedHandicap )
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
			IBotInterface* bot = mMatchData.players[i].bot.get();
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

		mMatchData.idxPlayerTurn = (setting.bBlackFrist) ? 0 : 1;
		IBotInterface* bot = mMatchData.getCurTurnBot();
		if( bot )
		{
			bot->thinkNextMove(mGame.getNextPlayColor());
		}

		return true;
	}

	void LeelaZeroGoStage::cleanupModeData()
	{
		if( mGamePlayWidget )
		{
			mGamePlayWidget->show(false);
		}
		bProcessPaused = false;

		switch( mGameMode )
		{
		case GameMode::Learning:
			mLearningAIRun.stop();
			break;
		case GameMode::Match:
			mMatchData.cleanup();
			break;
		}

		mGameMode = GameMode::None;
	}

	void LeelaZeroGoStage::keepLeelaProcessRunning(long time)
	{
		if( bMatchJob )
		{
			return;
		}
#if DETECT_LEELA_PROCESS
		if( mPIDLeela == -1 )
		{
			mPIDLeela = FPlatformProcess::FindPIDByNameAndParentPID(TEXT("leelaz.exe"), GetProcessId(mLearningAIRun.process.getHandle()));
			if( mPIDLeela == -1 )
			{
				mLeelaRebootTimer -= time;
			}
		}
		else
		{
			if( !FPlatformProcess::IsProcessRunning(mPIDLeela) )
			{
				mLeelaRebootTimer -= time;
			}
			else
			{
				mLeelaRebootTimer = LeelaRebootTime;
			}
		}

		if( mLeelaRebootTimer < 0 )
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
		assert(mGameMode == GameMode::Learning);
		auto ProcFun = [&](GameCommand& com)
		{
			int color = mGame.getNextPlayColor();
			switch( com.id )
			{
			case GameCommand::eStart:
				{
					::LogMsg("GameStart");
					mGame.restart();
					mGameRenderer.generateNoiseOffset(mGame.getBoard().getSize());
#if DETECT_LEELA_PROCESS
					mPIDLeela = -1;
					mLeelaRebootTimer = LeelaRebootStartTime;
#endif	
				}
				break;
			case GameCommand::ePass:
				{
					::LogMsg("Pass");
					mGame.playPass();
				}
				break;
			case GameCommand::eResign:
				{
					char const* name = (color == StoneColor::eBlack) ? "Black" : "White";
					::LogMsgF("%s Resigned", name);
				}
				break;
			case GameCommand::eEnd:
				{
					::LogMsg("Game End");
					++numGameCompleted;
					bMatchJob = false;

					if ( com.winNum == 0 )
						mLastGameResult.format("%s+Resign" , com.winner == StoneColor::eBlack ? "B" : "W" );
					else
						mLastGameResult.format("%s+%.1f", com.winner == StoneColor::eBlack ? "B" : "W" , com.winNum);
#if DETECT_LEELA_PROCESS
					mPIDLeela = -1;
					mLeelaRebootTimer = LeelaRebootStartTime;
#endif	
				}
				break;
			case GameCommand::eParam:
				{
					switch( com.paramId )
					{
					case LeelaGameParam::eJobMode:
						bMatchJob = com.intParam;
						break;
					}
				}
				break;
			case GameCommand::ePlay:
			default:
				{
					if( mGame.playStone(com.pos[0], com.pos[1]) )
					{

					}
					else
					{
						::LogMsgF("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
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
				::LogMsg("Pass");
				mGame.playPass();

				if( mGame.getLastPassCount() >= 2 )
				{
					::LogMsg("GameEnd");
					if( mMatchData.players[0].type == ControllerType::eLeelaZero ||
					    mMatchData.players[1].type == ControllerType::eLeelaZero )
					{
						LeelaBot* bot = static_cast< LeelaBot* >( (mMatchData.players[0].type == ControllerType::eLeelaZero ) ? mMatchData.players[0].bot.get() : mMatchData.players[1].bot.get());
						bot->mAI.inputProcessStream("final_score\n");
					}
				}
				else
				{
					mMatchData.advanceTurn();
					IBotInterface* bot = mMatchData.getCurTurnBot();
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
				mMatchData.advanceTurn();
				IBotInterface* bot = mMatchData.getCurTurnBot();
				if( bot )
				{
					bot->undo();
				}
			}
			break;
		case GameCommand::ePlay:
		default:
			{
				if( mGame.playStone(com.pos[0], com.pos[1]) )
				{
					mMatchData.advanceTurn();
					IBotInterface* bot = mMatchData.getCurTurnBot();
					if( bot )
					{
						bot->playStone(com.pos[0], com.pos[1], color);
						bot->thinkNextMove(mGame.getNextPlayColor());
					}
				}
				else
				{
					::LogMsgF("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
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
				if( canPlay() || ( isBotTurn() && !mMatchData.getCurTurnBot()->isThinking() ) )
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
		com.id = GameCommand::ePlay;
		com.playColor = color;
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