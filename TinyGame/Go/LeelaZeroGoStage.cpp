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
#include "RenderGL/ShaderCompiler.h"

REGISTER_STAGE("LeelaZero Learning", Go::LeelaZeroGoStage, EStageGroup::Test);

#define DERAULT_LEELA_WEIGHT_NAME "6615567eaa3adc8ea695682fcfbd7eaa3bb557d3720d2b61610b66006104050e"

#include <Dbghelp.h>
#pragma comment (lib,"Dbghelp.lib")

namespace Go
{
	class UnderCurveAreaShaderProgram : public RenderGL::GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(UnderCurveAreaShaderProgram)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/UnderCurveArea";
		}
		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const enties[] =
			{
				{ Shader::eVertex ,  SHADER_ENTRY(MainVS) },
				{ Shader::ePixel ,  SHADER_ENTRY(MainPS) },
				{ Shader::eGeometry ,  SHADER_ENTRY(MainGS) },
				{ Shader::eEmpty ,  nullptr },
			};
			return enties;
		}
		void bindParameters()
		{
			mParamAxisValue.bind(*this, SHADER_PARAM(AxisValue));
			mParamProjMatrix.bind(*this, SHADER_PARAM(ProjMatrix));
			mParamUpperColor.bind(*this, SHADER_PARAM(UpperColor));
			mParamLowerColor.bind(*this, SHADER_PARAM(LowerColor));
		}
		void setParameters(float axisValue, Matrix4 const& projMatrix, Vector4 const& upperColor, Vector4 const& lowerColor)
		{
			setParam(mParamAxisValue, axisValue);
			setParam(mParamProjMatrix, projMatrix);
			setParam(mParamUpperColor, upperColor);
			setParam(mParamLowerColor, lowerColor);
		}
		ShaderParameter mParamAxisValue;
		ShaderParameter mParamProjMatrix;
		ShaderParameter mParamUpperColor;
		ShaderParameter mParamLowerColor;
	};

	IMPLEMENT_GLOBAL_SHADER( UnderCurveAreaShaderProgram )

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

		using namespace RenderGL;
#if 0
		if( !DumpFunSymbol("Zen7.dump", "aa.txt") )
			return false;
#endif
		
		mShaderUnderCurveArea = ShaderManager::Get().getGlobalShaderT< UnderCurveAreaShaderProgram >( true );
		if( mShaderUnderCurveArea == nullptr )
			return false;

		LeelaAppRun::InstallDir = ::Global::GameConfig().getStringValue("LeelaZeroInstallDir", "Go" , "E:/Desktop/LeelaZero");
		AQAppRun::InstallDir = ::Global::GameConfig().getStringValue("AQInstallDir", "Go", "E:/Desktop/AQ");

		::Global::GUI().cleanupWidget();

		mGame.setup(19);
#if 1
		if( !buildLearningMode() )
			return false;
#else
		if( !buildPlayMode() )
			return false;
#endif

		auto devFrame = WidgetUtility::CreateDevFrame( Vec2i(150, 200 + 55) );

		devFrame->addButton("Save SGF", [&](int eventId, GWidget* widget) -> bool
		{
			mGame.saveSGF("Game.sgf");
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

		devFrame->addCheckBox("Pause Game", [&](int eventId, GWidget* widget) -> bool
		{
			if( mGameMode == GameMode::Learning )
			{
				if( bPauseGame )
				{
					if( mLearningAIRun.process.resume() )
					{
						widget->cast<GCheckBox>()->setTitle("Pause Game");
						bPauseGame = false;
					}
				}
				else
				{
					if( mLearningAIRun.process.suspend() )
					{
						widget->cast<GCheckBox>()->setTitle("Resume Game");
						bPauseGame = true;
					}
				}
			}
			else
			{
				bPauseGame = !bPauseGame;
			}
			return false;
		});
		devFrame->addCheckBox("Review Game", [&](int eventId, GWidget* widget) ->bool
		{
			bReviewingGame = !bReviewingGame;

			if( bReviewingGame )
			{
				mReviewGame.guid = mGame.guid;
				mReviewGame.copy(mGame);
				widget->cast<GCheckBox>()->setTitle("Close Review");
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
				widget->cast<GCheckBox>()->setTitle("Review Game");
			}
			return false;
		});

		devFrame->addCheckBox("Try Play", [&](int eventId, GWidget* widget) ->bool
		{
			bTryPlayingGame = !bTryPlayingGame;

			if( bTryPlayingGame )
			{
				MyGame& Gamelooking = bReviewingGame ? mReviewGame : mGame;
				mTryPlayGame.guid = Gamelooking.guid;
				mTryPlayGame.copy(Gamelooking);
				mTryPlayGame.removeUnplayedHistory();
				widget->cast<GCheckBox>()->setTitle("End Play");
			}
			else
			{
				widget->cast<GCheckBox>()->setTitle("Try Play");
			}

			return false;
		});

		return true;
	}

	void LeelaZeroGoStage::onEnd()
	{
		cleanupModeData();
		mGameRenderer.releaseRHI();
		ShaderManager::Get().clearnupRHIResouse();
		::Global::getDrawEngine()->stopOpenGL(true);
		BaseClass::onEnd();
	}

	void LeelaZeroGoStage::onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		if( mGameMode == GameMode::Match )
		{
			if ( !bPauseGame )
			{
				IBotInterface* bot = mMatchData.getCurTurnBot();
				for( int i = 0; i < 2; ++i )
				{
					IBotInterface* bot = mMatchData.players[i].bot.get();

					if( bot )
					{
						struct MyListener : IGameCommandListener
						{
							LeelaZeroGoStage* me;
							int  indexPlayer;
							virtual void notifyCommand(GameCommand const& com) override
							{
								me->notifyPlayerCommand(indexPlayer, com);
							}
						};
						MyListener listener;
						listener.me = this;
						listener.indexPlayer = mMatchData.idxPlayerTurn;
						bot->update(listener);
					}
				}
			}
		}
		else if( mGameMode == GameMode::Learning )
		{
			mLearningAIRun.update();
			processLearningCommand();
			keepLeelaProcessRunning(time);
		}

		bPrevGameCom = false;

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

		auto& viewingGame = getViewingGame();

		mGameRenderer.draw(BoardPos, viewingGame );

		if( bestMoveVertex >= 0 )
		{
			int x = bestMoveVertex % LeelaGoSize;
			int y = bestMoveVertex / LeelaGoSize;
			Vector2 pos = mGameRenderer.getIntersectionPos(BoardPos, viewingGame.getBoard(), x, y);

			RenderUtility::SetPen(g, Color::eRed);
			RenderUtility::SetBrush(g, Color::eRed);
			g.drawCircle(pos, 8);
		}


		if( mWinRateHistory[0].size() > 1 || mWinRateHistory[1].size() > 1 )
		{
			GPU_PROFILE("Draw WinRate Graph");
			ViewportSaveScope viewportSave;
			glViewport(610, 10, 175, 250);
			MatrixSaveScope matrixSaveScope;
			glMatrixMode(GL_PROJECTION);

			float const xMin = 0;
			float const xMax = (mGame.getCurrentStep() + 1) / 2 + 1;
			float const yMin = 0;
			float const yMax = 100;
			Matrix4 matProj = OrthoMatrix(xMin-1, xMax, yMin-5, yMax + 5 , -1, 1);

			glLoadMatrixf(matProj);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			static std::vector<Vector2> buffer;

			buffer.clear();
			float offset = 25;
			for( float y = 10; y < yMax; y += 10 )
			{
				buffer.push_back(Vector2(xMin, y));
				buffer.push_back(Vector2(xMax, y));
			}
			for( float x = offset; x < xMax; x += offset )
			{
				buffer.push_back(Vector2(x, yMin));
				buffer.push_back(Vector2(x, yMax));
			}

			if( !buffer.empty() )
			{
				glColor3f(0.3, 0.3, 0.3);
				TRenderRT< RTVF_XY >::Draw(PrimitiveType::eLineList, &buffer[0], buffer.size());
			}

			Vector3 colors[2] = { Vector3(1,0,0) , Vector3(0,1,0) };
			float alpha[2] = { 0.6 , 0.6 };
			for( int i = 0; i < 2; ++i )
			{
				auto& winRateHistory = mWinRateHistory[i];
				if( winRateHistory.empty() )
					continue;
				{
					glEnable(GL_BLEND);
					if( i == 0 )
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					else
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);

					GL_BIND_LOCK_OBJECT(mShaderUnderCurveArea);

					mShaderUnderCurveArea->setParameters(
						float(50), matProj, 
						Vector4(colors[i], alpha[i]),
						Vector4(0.3 * colors[i], alpha[i]));

					TRenderRT< RTVF_XY >::DrawShader(PrimitiveType::eLineStrip, &winRateHistory[0], winRateHistory.size());

					glDisable(GL_BLEND);
				}

				glColor3fv(colors[i]);
				TRenderRT< RTVF_XY >::Draw(PrimitiveType::eLineStrip, &winRateHistory[0], winRateHistory.size());
			}


			Vector2 const lines[] =
			{
				Vector2(0,50), Vector2(xMax,50),
				Vector2(0,yMin) , Vector2(0,yMax),
				Vector2(0,yMin) , Vector2(xMax,yMin),
				Vector2(0,yMax) , Vector2(xMax,yMax),
			};

			glColor3f(0, 0, 1);
			TRenderRT< RTVF_XY >::Draw(PrimitiveType::eLineList, &lines[0], ARRAY_SIZE(lines));
		}

		GpuProfiler::Get().endFrame();
		g.endRender();

		g.beginRender();

		FixString< 512 > str;

		g.setTextColor(255, 0, 0);
		RenderUtility::SetFont(g, FONT_S10);
		if( mGameMode == GameMode::Learning )
		{
			g.drawText(200, 20, str.format("Num Game Completed = %d", numGameCompleted));
			if ( bMatchJob )
			{
				if ( matchChallenger != StoneColor::eEmpty )
				{
					str.format("Job Type = Match , Challenger = %s",
							   matchChallenger == StoneColor::eBlack ? "B" : "W");
				}
				else
				{
					str.format("Job Type = Match");
				}
			}
			else
			{
				str.format("Job Type = Self Play");
			}
			g.drawText(200, 35, str);

			if( !mLastGameResult.empty() )
			{
				g.drawText(200, 50, str.format( "Last Game Result : %s" , mLastGameResult.c_str() ) );
			}
		}
		else
		{
			if( mMatchData.bAutoRun )
			{
				int y = 20;
				for( int i = 0; i < 2; ++i )
				{
					auto const& player = mMatchData.players[i];
					str.format("%s (%s) = %d" , GetControllerName(player.type) , mMatchData.getPlayerColor(i) == StoneColor::eBlack ? "B" : "W" ,  player.winCount );
					g.drawText(200, y , str);
					y += 15;
				}
				str.format( "Unknown = %d" ,unknownWinerCount );
				g.drawText( 200, y , str );
			}
		}

		g.setTextColor(255, 0, 0);
		RenderUtility::SetFont(g, FONT_S10);
		int const offset = 15;
		int textX = 350;
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
		case Keyboard::eF5: saveMatchGameSGF(); break;
		case Keyboard::eF6: restartAutoMatch(); break;
		case Keyboard::eF9: ShaderManager::Get().reloadShader( *mShaderUnderCurveArea ); break;
		}
		return false;
	}

	bool LeelaZeroGoStage::buildLearningMode()
	{

		if( !mLearningAIRun.buildLearningGame() )
			return false;

		mGameMode = GameMode::Learning;
		GameSetting setting;
		setting.komi = 7.5;
		mGame.setSetting(setting);
		
		resetGameParam();

		bMatchJob = false;
#if DETECT_LEELA_PROCESS
		mLeelaRebootTimer = LeelaRebootStartTime;
		mPIDLeela = -1;
#endif
		return true;
	}

	bool LeelaZeroGoStage::buildPlayMode()
	{
		std::string bestWeigetName = LeelaAppRun::GetDesiredWeightName();
		char const* weightName = nullptr;
		if( bestWeigetName != "" )
		{
			weightName = bestWeigetName.c_str();
			::Global::GameConfig().setKeyValue("LeelaLastMatchWeight", "Go", weightName);
		}
		else
		{
			weightName = Global::GameConfig().getStringValue("LeelaLastMatchWeight", "Go", DERAULT_LEELA_WEIGHT_NAME);
		}
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
		setting.komi = 7.5;
		if( !startMatchGame(setting) )
			return false;

		return true;
	}

	bool LeelaZeroGoStage::buildLeelaMatchMode()
	{
		char const* weightNameA = ::Global::GameConfig().getStringValue( "LeelaLastOpenWeight" , "Go" , DERAULT_LEELA_WEIGHT_NAME );

		FixString<256> path;
		path.format("%s/" LEELA_NET_DIR "%s" , LeelaAppRun::InstallDir , weightNameA );
		path.replace('/', '\\');
		if( SystemPlatform::OpenFileName(path, path.max_size(), nullptr) )
		{
			weightNameA = FileUtility::GetDirPathPos(path) + 1;
			::Global::GameConfig().setKeyValue("LeelaLastOpenWeight", "Go", weightNameA);
		}

		std::string bestWeigetName = LeelaAppRun::GetDesiredWeightName();

		{
			LeelaAISetting setting;
			setting.weightName = weightNameA;
			setting.seed = generateRandSeed();
			setting.visits = 6000;
			setting.playouts = 0;
			setting.bNoise = true;
			if( !mMatchData.players[0].initialize(ControllerType::eLeelaZero, &setting) )
				return false;
		}

		{
			LeelaAISetting setting;
			setting.weightName = bestWeigetName.c_str();
			setting.seed = generateRandSeed();
			setting.visits = 6000;
			setting.playouts = 0;
			setting.bNoise = true;
			if( !mMatchData.players[1].initialize(ControllerType::eLeelaZero, &setting) )
				return false;
		}


		mMatchData.bSwapColor = true;

		GameSetting setting;
		setting.numHandicap = 0;
		if( setting.numHandicap )
		{
			setting.bBlackFrist = false;
			setting.komi = 0.5;
		}
		else
		{
			setting.bBlackFrist = true;
			setting.komi = 7.5;
		}
		
		if( !startMatchGame(setting) )
			return false;

		return true;
	}

	bool LeelaZeroGoStage::buildLeelaZenMatchMode()
	{
		{
			LeelaAISetting setting = LeelaAISetting::GetDefalut();
			std::string bestWeigetName = LeelaAppRun::GetDesiredWeightName();
			FixString<256> path;
			path.format("%s/" LEELA_NET_DIR "%s", LeelaAppRun::InstallDir, bestWeigetName.c_str());
			path.replace('/', '\\');


			if( SystemPlatform::OpenFileName(path, path.max_size(), nullptr) )
			{
				setting.weightName = FileUtility::GetDirPathPos(path) + 1;
			}
			else
			{
				setting.weightName = bestWeigetName.c_str();
			}

			if( !mMatchData.players[0].initialize(ControllerType::eLeelaZero, &setting) )
				return false;
		}

		{
			Zen::CoreSetting setting;
			int numCPU = SystemPlatform::GetProcessorNumber();
			setting.numThreads = numCPU - 2;
			setting.numSimulations = 20000000;
			setting.maxTime = 25;
			if( !mMatchData.players[1].initialize(ControllerType::eZenV7 , &setting) )
				return false;
		}

		mMatchData.bSwapColor = true;

		GameSetting setting;
		setting.numHandicap = 0;
		if( setting.numHandicap )
			setting.bBlackFrist = false;

		if( !startMatchGame(setting) )
			return false;

		return true;
	}



	bool LeelaZeroGoStage::startMatchGame(GameSetting const& setting)
	{
		mGameMode = GameMode::Match;

		unknownWinerCount = 0;

		bool haveBot = mMatchData.players[0].type != ControllerType::ePlayer ||
			           mMatchData.players[1].type != ControllerType::ePlayer;
		if( haveBot && setting.numHandicap )
		{
			GameSetting tempSetting = setting;
			//tempSetting.numHandicap = 0;

			mGame.setSetting(tempSetting);
		}
		else
		{
			mGame.setSetting(setting);
		}

		
		int indexBlack = (mMatchData.bSwapColor) ? 1 : 0;
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

		resetGameParam();

		mMatchData.idxPlayerTurn = (setting.bBlackFrist) ? 0 : 1;
		if( mMatchData.bSwapColor )
			mMatchData.idxPlayerTurn = 1 - mMatchData.idxPlayerTurn;

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
		bPauseGame = false;

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
					LogMsg("GameStart");
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
					LogMsg("Pass");
					mGame.playPass();
				}
				break;
			case GameCommand::eResign:
				{
					char const* name = (color == StoneColor::eBlack) ? "Black" : "White";
					LogMsgF("%s Resigned", name);
				}
				break;
			case GameCommand::eEnd:
				{
					LogMsg("Game End");
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
					case LeelaGameParam::eMatchChallengerColor:
						matchChallenger = com.intParam;
						break;
					case LeelaGameParam::eLastNetWeight:
						{
							::Global::GameConfig().setKeyValue("LeelaLastNetWeight", "Go", com.strParam);
						}
						break;
					}
				}
				break;
			case GameCommand::ePlayStone:
			default:
				{
					if( mGame.playStone(com.pos[0], com.pos[1]) )
					{

					}
					else
					{
						LogMsgF("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
					}

				}
				break;
			}
		};

		mLearningAIRun.outputThread->procOutputCommand(ProcFun);
	}


	void LeelaZeroGoStage::notifyPlayerCommand(int indexPlayer , GameCommand const& com)
	{
		int color = mGame.getNextPlayColor();
		switch( com.id )
		{
		case GameCommand::ePass:
			{
				resetTurnParam();

				LogMsg("Pass");
				mGame.playPass();

				if( mGame.getLastPassCount() >= 2 )
				{
					LogMsg("GameEnd");
					
					if( bAutoSaveMatchSGF )
						saveMatchGameSGF();

					if( mMatchData.players[0].type == ControllerType::eLeelaZero ||
					    mMatchData.players[1].type == ControllerType::eLeelaZero )
					{
						LeelaBot* bot = static_cast< LeelaBot* >( (mMatchData.players[0].type == ControllerType::eLeelaZero ) ? mMatchData.players[0].bot.get() : mMatchData.players[1].bot.get());
						bot->mAI.showResult();
					}
					else if( mMatchData.bAutoRun )
					{
						unknownWinerCount += 1;
						restartAutoMatch();
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
		case GameCommand::eEnd:
			{
				if( mMatchData.bAutoRun )
				{
					mMatchData.getPlayer(com.winner).winCount += 1;
					restartAutoMatch();
				}
				else
				{
					char const* name = (com.winner == StoneColor::eBlack) ? "Black" : "White";
					FixString< 128 > str;

					str.format("%s Win", name);
					::Global::GUI().showMessageBox(UI_ANY, str, GMB_OK);
				}
			}
			break;
		case GameCommand::eResign:
			{
				LogMsg("GameEnd");

				if( bAutoSaveMatchSGF )
					saveMatchGameSGF();

				if( mMatchData.bAutoRun )
				{
					if( color == StoneColor::eBlack )
					{
						mMatchData.getPlayer(StoneColor::eWhite).winCount += 1;
					}
					else
					{
						mMatchData.getPlayer(StoneColor::eBlack).winCount += 1;
					}

					restartAutoMatch();
				}
				else
				{
					char const* name = (color == StoneColor::eBlack) ? "Black" : "White";
					FixString< 128 > str;

					str.format("%s Resigned", name);
					::Global::GUI().showMessageBox(UI_ANY, str, GMB_OK);
				}
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

		case GameCommand::eParam:
			switch( com.paramId )
			{
			case LeelaGameParam::eBestMoveVertex:
			case ZenGameParam::eBestMoveVertex:
				bestMoveVertex = com.intParam;
				break;
			case LeelaGameParam::eWinRate:
			case ZenGameParam::eWinRate:
				if ( !bPrevGameCom )
				{
					Vector2 v;
					v.x = ( mGame.getCurrentStep() + 1 ) / 2;
					v.y = com.floatParam;
					mWinRateHistory[indexPlayer].push_back(v);
					//LogMsgF("Win rate = %.2f", com.floatParam);
				}
				break;
			}
			break;
		case GameCommand::eAddStone:

			break;
		case GameCommand::ePlayStone:
		default:
			{
				if( mGame.playStone(com.pos[0], com.pos[1]) )
				{
					resetTurnParam();

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
					LogMsgF("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
				}

			}
			break;
		}
	}

	bool LeelaZeroGoStage::saveMatchGameSGF()
	{
		DateTime date = SystemPlatform::GetLocalTime();

		FixString< 512 > path;
		path.format("Go/Save/%04d-%02d-%02d-%02d-%02d-%02d.sgf", date.getYear(), date.getMonth(), date.getDay(), date.getHour(), date.getMinute(), date.getSecond());

		FixString< 512 > dateString;
		dateString.format("%d-%d-%d", date.getYear(), date.getMonth(), date.getDay());
		GameDescription description;
		description.blackName = GetControllerName(mMatchData.getPlayer(StoneColor::eBlack).type);
		description.whiteName = GetControllerName(mMatchData.getPlayer(StoneColor::eWhite).type);
		description.date = dateString.c_str();

		return mGame.saveSGF(path, &description);
	}

	void LeelaZeroGoStage::restartAutoMatch()
	{
		LogMsg("==Restart Auto Match==");
		if( bSwapEveryMatch )
		{
			mMatchData.bSwapColor = !mMatchData.bSwapColor;
		}


		mMatchData.idxPlayerTurn = (mGame.getSetting().bBlackFrist) ? 0 : 1;
		if( mMatchData.bSwapColor )
			mMatchData.idxPlayerTurn = 1 - mMatchData.idxPlayerTurn;

		resetGameParam();


		for( int i = 0; i < 2; ++i )
		{
			if( mMatchData.players[i].bot )
			{
				mMatchData.players[i].bot->restart();
			}
		}

		IBotInterface* bot = mMatchData.getCurTurnBot();
		if( bot )
		{
			bot->thinkNextMove(mGame.getNextPlayColor());
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
		com.id = GameCommand::ePlayStone;
		com.playColor = color;
		com.pos[0] = pos.x;
		com.pos[1] = pos.y;
		notifyPlayerCommand( mMatchData.idxPlayerTurn , com);
	}

	void LeelaZeroGoStage::execPassCommand()
	{
		int color = mGame.getNextPlayColor();
		assert(isPlayerControl(color));
		GameCommand com;
		com.id = GameCommand::ePass;
		notifyPlayerCommand(mMatchData.idxPlayerTurn, com);
	}

	void LeelaZeroGoStage::execUndoCommand()
	{
		int color = mGame.getNextPlayColor();
		//assert(isPlayerControl(color));
		GameCommand com;
		com.id = GameCommand::eUndo;
		notifyPlayerCommand(mMatchData.idxPlayerTurn, com);
	}


	bool MatchPlayer::initialize(ControllerType inType, void* botSetting /*= nullptr*/)
	{
		type = inType;
		winCount = 0;

		bot.release();
		switch( type )
		{
		case ControllerType::ePlayer:
			return true;
		case ControllerType::eLeelaZero:
			bot.reset(new LeelaBot());
			break;
		case ControllerType::eAQ:
			bot.reset(new AQBot());
			break;
		case ControllerType::eZenV7:
			bot.reset(new ZenBot(7));
			break;
		case ControllerType::eZenV6:
			bot.reset(new ZenBot(6));
			break;
		case ControllerType::eZenV4:
			bot.reset(new ZenBot(4));
			break;
		}

		if( !bot )
			return false;

		if( !bot->initilize(botSetting) )
		{
			bot.release();
			return false;
		}
		return true;
	}

	void MatchSettingPanel::addLeelaParamWidget(int id, int idxPlayer)
	{
		LeelaAISetting setting = LeelaAISetting::GetDefalut();
		GTextCtrl* textCtrl = addTextCtrl(id + UPARAM_VISITS, "Visit Num", BIT(idxPlayer), idxPlayer);
		textCtrl->setValue(std::to_string(setting.visits).c_str());
		GFilePicker*  filePicker = addWidget< GFilePicker >(id + UPARAM_WEIGHT_NAME, "Weight Name", BIT(idxPlayer), idxPlayer);
		filePicker->filePath.format("%s/" LEELA_NET_DIR "%s", LeelaAppRun::InstallDir, LeelaAppRun::GetDesiredWeightName().c_str());
		filePicker->filePath.replace('/', '\\');
	}

	bool MatchSettingPanel::setupMatchSetting(MatchGameData& matchData, GameSetting& setting)
	{
		ControllerType types[2] =
		{
			(ControllerType)(intptr_t)findChildT<GChoice>(UI_CONTROLLER_TYPE_A)->getSelectedItemData(),
			(ControllerType)(intptr_t)findChildT<GChoice>(UI_CONTROLLER_TYPE_B)->getSelectedItemData()
		};

		matchData.bAutoRun = findChildT<GCheckBox>(UI_AUTO_RUN)->bChecked;
		for( int i = 0; i < 2; ++i )
		{
			int id = (i) ? UI_CONTROLLER_TYPE_B : UI_CONTROLLER_TYPE_A;
			switch( types[i] )
			{
			case ControllerType::eLeelaZero:
				{
					LeelaAISetting setting = LeelaAISetting::GetDefalut();
					std::string weightName = findChildT<GFilePicker>(id + UPARAM_WEIGHT_NAME)->filePath.c_str();
					setting.weightName = FileUtility::GetDirPathPos(weightName.c_str()) + 1;
					setting.visits = atoi(findChildT<GTextCtrl>(id + UPARAM_VISITS)->getValue());
					if( !matchData.players[i].initialize(types[i], &setting) )
						return false;
				}
				break;
			case ControllerType::eZenV4:
			case ControllerType::eZenV6:
			case ControllerType::eZenV7:
				{
					Zen::CoreSetting setting = ZenBot::GetCoreConfigSetting();
					setting.numSimulations = atoi(findChildT<GTextCtrl>(id + UPARAM_SIMULATIONS_NUM)->getValue());
					setting.maxTime = atof(findChildT<GTextCtrl>(id + UPARAM_MAX_TIME)->getValue());
					if( !matchData.players[i].initialize(types[i], &setting) )
						return false;
				}
				break;
			default:
				if( !matchData.players[i].initialize(types[i]) )
					return false;
				break;
			}
		}

		setting.numHandicap = findChildT<GChoice>(UI_FIXED_HANDICAP)->getSelection();
		if( setting.numHandicap )
		{
			setting.bBlackFrist = false;
			setting.komi = 0.5;
		}
		else
		{
			setting.bBlackFrist = true;
			setting.komi = 7.5;
		}
		return true;
	}

}//namespace Go