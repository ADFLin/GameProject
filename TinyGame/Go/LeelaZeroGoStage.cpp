#include "LeelaZeroGoStage.h"

#include "StringParse.h"
#include "RandomUtility.h"
#include "Widget/WidgetUtility.h"
#include "GameGlobal.h"
#include "PropertyKey.h"
#include "FileSystem.h"
#include "InputManager.h"
#include "ConsoleSystem.h"

#include "GLGraphics2D.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/ShaderCompiler.h"

#define MATCH_RESULT_PATH "Go/MatchResult.data"


REGISTER_STAGE("LeelaZero Learning", Go::LeelaZeroGoStage, EStageGroup::Test);

#define DERAULT_LEELA_WEIGHT_NAME "6615567eaa3adc8ea695682fcfbd7eaa3bb557d3720d2b61610b66006104050e"

#include <Dbghelp.h>
#pragma comment (lib,"Dbghelp.lib")


namespace Go
{



	void RunConvertWeight(LeelaWeightTable const& table)
	{

		FileIterator fileIter;
		FixString<256> dir;
		dir.format("%s/%s", LeelaAppRun::InstallDir, LEELA_NET_DIR_NAME);

		std::string bestWeightName = LeelaAppRun::GetBestWeightName();
		if( FileSystem::FindFiles(dir, nullptr, fileIter) )
		{
			for( ; fileIter.haveMore(); fileIter.goNext() )
			{
				if ( FCString::Compare( fileIter.getFileName() , bestWeightName.c_str() ) == 0 )
					continue;

				char const* subName = FileUtility::GetExtension(fileIter.getFileName());
				if( subName )
				{
					--subName;
				}

				if( !LeelaWeightTable::IsOfficialFormat(fileIter.getFileName(), subName) )
					continue;


				FixString< 32 > weightName;
				weightName.assign(fileIter.getFileName(), LeelaWeightInfo::DefaultNameLength);

				auto info = table.find(weightName.c_str());
				if( info )
				{
					FixString<128> newName;
					info->getFormattedName(newName);
					if( subName )
					{
						newName += subName;
					}

					FixString<256> filePath;
					filePath.format("%s/%s", dir.c_str(), fileIter.getFileName());

					FileSystem::RenameFile(filePath, newName);
				}
			}
		}

	}

	class UnderCurveAreaProgram : public Render::GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(UnderCurveAreaProgram)

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
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamAxisValue, SHADER_PARAM(AxisValue));
			parameterMap.bind(mParamProjMatrix, SHADER_PARAM(ProjMatrix));
			parameterMap.bind(mParamUpperColor, SHADER_PARAM(UpperColor));
			parameterMap.bind(mParamLowerColor, SHADER_PARAM(LowerColor));
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

	IMPLEMENT_GLOBAL_SHADER( UnderCurveAreaProgram )

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


	HWND GetProcessWindow(DWORD processId)
	{
		struct FindData
		{
			DWORD  processId;
			HWND   result;


			static BOOL CALLBACK Callback(HWND hWnd, LPARAM param)
			{
				auto myData = (FindData*)param;
				DWORD processId = -1;
				::GetWindowThreadProcessId(hWnd, &processId);
				if( processId == myData->processId )
				{
					myData->result = hWnd;
					return FALSE;
				}
				return TRUE;
			}
		};

		FindData findData;
		findData.processId = processId;
		findData.result = NULL;
		::EnumWindows(FindData::Callback, (LPARAM)&findData);
		return findData.result;
	}


	bool LeelaZeroGoStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		::Global::GetDrawEngine()->changeScreenSize(1080, 720);

		if( !::Global::GetDrawEngine()->startOpenGL(8) )
			return false;


		//ILocalization::Get().changeLanguage(LAN_ENGLISH);

		VERIFY_RETURN_FALSE(mBoardRenderer.initializeRHI());

		using namespace Render;
#if 0
		if( !DumpFunSymbol("Zen7.dump", "aa.txt") )
			return false;
#endif

		VERIFY_RETURN_FALSE( mProgUnderCurveArea = ShaderManager::Get().getGlobalShaderT< UnderCurveAreaProgram >( true ) );

		LeelaAppRun::InstallDir = ::Global::GameConfig().getStringValue("LeelaZeroInstallDir", "Go" , "E:/Desktop/LeelaZero");
		AQAppRun::InstallDir = ::Global::GameConfig().getStringValue("AQInstallDir", "Go", "E:/Desktop/AQ");

		::Global::GUI().cleanupWidget();

		mGame.setup(19);

		auto GameActionFun = [&](GameProxy& game)
		{
			if( &game == &getViewingGame() )
			{
				updateViewGameTerritory();

			}

			if( bReviewingGame && &game == &mGame && mGame.guid == mReviewGame.guid )
			{
				mGame.synchronizeHistory(mReviewGame);
			}
		};

		mGame.onStateChanged = GameActionFun;
		mReviewGame.onStateChanged = GameActionFun;
		mTryPlayGame.onStateChanged = GameActionFun;

		mMatchResultMap.load(MATCH_RESULT_PATH);
		mMatchResultMap.checkMapValid();
#if 0
#if 1
		if( !buildLearningMode() )
			return false;
#else
		if( !buildPlayMode() )
			return false;
#endif
#endif

		auto devFrame = WidgetUtility::CreateDevFrame( Vec2i(150, 200 + 55) );

		devFrame->addButton("Save SGF", [&](int eventId, GWidget* widget) -> bool
		{
			mGame.getInstance().saveSGF("Game.sgf");
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
		devFrame->addButton("Run Analysis", [&](int eventId, GWidget* widget) -> bool
		{
			if ( mGameMode != GameMode::Analysis )
			{
				cleanupModeData();
				buildAnalysisMode();
			}
			return false;
		});

#if 0
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
#endif
		devFrame->addButton("Run Covert Weight", [&](int eventId, GWidget* widget) -> bool
		{
			LeelaWeightTable table;

			FixString<256> path;
			path.format("%s/%s/%s", LeelaAppRun::InstallDir, LEELA_NET_DIR_NAME, "table.txt");
			if( !table.load(path) )
				return false;

			RunConvertWeight( table );
			mMatchResultMap.convertLeelaWeight(table);
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
					if( mLeelaAIRun.process.resume() )
					{
						widget->cast<GCheckBox>()->setTitle("Pause Game");
						bPauseGame = false;
					}
				}
				else
				{
					if( mLeelaAIRun.process.suspend() )
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
				mReviewGame.copy(mGame);
				widget->cast<GCheckBox>()->setTitle("Close Review");
				Vec2i screenSize = ::Global::GetDrawEngine()->getScreenSize();
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
					mReviewGame.reviewNextStep(1);
					return false;
				});

				frame->addButton(">>", [&](int eventId, GWidget* widget) ->bool
				{
					mReviewGame.reviewNextStep(10);
					return false;
				});

				frame->addButton("[>]", [&](int eventId, GWidget* widget) ->bool
				{
					mReviewGame.reviewLastStep();
					return false;
				});
				::Global::GUI().addWidget(frame);
			}
			else
			{
				::Global::GUI().findTopWidget(UI_REPLAY_FRAME)->destroy();
				updateViewGameTerritory();
				widget->cast<GCheckBox>()->setTitle("Review Game");
			}
			return false;
		});

		devFrame->addCheckBox("Try Play", [&](int eventId, GWidget* widget) ->bool
		{
#if 1
			if( mTryPlayWidget )
			{
				mTryPlayWidgetPos = mTryPlayWidget->getWorldPos();
				mTryPlayWidget->destroy();
				mTryPlayWidget = nullptr;
				widget->cast<GCheckBox>()->setTitle("Try Play");
			}
			else
			{
				GameProxy& Gamelooking = bReviewingGame ? mReviewGame : mGame;
				mTryPlayWidget = new TryPlayBoardFrame( UI_ANY , mTryPlayWidgetPos, Vec2i(600,600) , nullptr );
				mTryPlayWidget->game.copy(Gamelooking , true );
				mTryPlayWidget->renderer = &mBoardRenderer;
				::Global::GUI().addWidget(mTryPlayWidget);
				widget->cast<GCheckBox>()->setTitle("End Play");
			}
#else
			bTryPlayingGame = !bTryPlayingGame;

			if( bTryPlayingGame )
			{
				GameProxy& Gamelooking = bReviewingGame ? mReviewGame : mGame;
				mTryPlayGame.copy(Gamelooking);
				mTryPlayGame.removeUnplayedHistory();
				widget->cast<GCheckBox>()->setTitle("End Play");
			}
			else
			{
				updateViewGameTerritory();
				widget->cast<GCheckBox>()->setTitle("Try Play");
			}
#endif
			return false;
		});
		devFrame->addButton("Show Territory", [&](int eventId, GWidget* widget) ->bool
		{
			if( bUpdateTerritory )
			{
				bShowTerritory = !bShowTerritory;
				if( !bShowTerritory )
					bUpdateTerritory = false;
			}
			else
			{
				bUpdateTerritory = true;
			}
			updateViewGameTerritory();
			return false;
		});
		return true;
	}

	void LeelaZeroGoStage::onEnd()
	{
		cleanupModeData( true );
		mBoardRenderer.releaseRHI();
		::Global::GetDrawEngine()->stopOpenGL(true);
		BaseClass::onEnd();
	}

	void LeelaZeroGoStage::onUpdate(long time)
	{
#if 0
		DWORD pId = FPlatformProcess::FindPIDByName(TEXT("foxwq.exe"));
		if( pId != -1 )
		{
			HWND hWnd = GetProcessWindow(pId);

			char const* BoardPanelClass = "#32770";
			char const* BoardPaenlTitle = "CChessboardPanel";
			HWND hBoardWnd = FindWindowExA(NULL, NULL, BoardPanelClass, NULL);

			if ( hBoardWnd )
			{
				HDC hDC = GetDC(hBoardWnd);

				if( hDC )
				{
					::Rectangle(hDC, 0, 0, 100, 100);
				}

				::ReleaseDC(hBoardWnd, hDC);
			}
		}
#endif

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
			mLeelaAIRun.update();
			processLearningCommand();

			if( mbRestartLearning )
			{
				cleanupModeData(false);
				buildLearningMode();
				mbRestartLearning = false;
				return;
			}
			else
			{
				keepLeelaProcessRunning(time);
			}
			
		}

		if( bAnalysisEnabled )
		{
			mLeelaAIRun.update();
			auto ProcFun = [&](GameCommand& com)
			{
				int color = mGame.getInstance().getNextPlayColor();
				switch( com.id )
				{
				case GameCommand::eParam:
					{
						switch( com.paramId )
						{
						case LeelaGameParam::eThinkResult:
							if ( !static_cast< LeelaThinkInfoVec* >(com.ptrParam )->empty() )
								analysisResult = *static_cast< LeelaThinkInfoVec* >(com.ptrParam);
							break;
						}
					}
					break;
				case GameCommand::ePlayStone:
					if ( mGameMode == GameMode::Analysis )
					{
						if( mGame.playStone(com.pos[0], com.pos[1]) )
						{

						}
						else
						{
							LogMsg("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
						}
					}
					break;
				}
			};
			mLeelaAIRun.outputThread->procOutputCommand(ProcFun);
		}

		bPrevGameCom = false;

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}


	TConsoleVariable< int > TestValue(0, "TestValue");


	void LeelaZeroGoStage::onRender(float dFrame)
	{
		using namespace Render;

		using namespace Go;

		glClear(GL_COLOR_BUFFER_BIT);

		GLGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();


		auto& viewingGame = getViewingGame();

		RenderContext context( viewingGame.getBoard() , BoardPos , RenderBoardScale );

		mBoardRenderer.drawBorad( g , context );

		if( bShowTerritory )
		{
			DrawTerritoryStatus(mBoardRenderer, context, mShowTerritoryInfo);
		}

		int lastPlayPos[2] = { -1,-1 };
		viewingGame.getInstance().getLastStepPos(lastPlayPos);
		if( lastPlayPos[0] != -1 && lastPlayPos[1] != -1 )
		{
			Vector2 pos = mBoardRenderer.getStonePos(context, lastPlayPos[0], lastPlayPos[1]);
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawCircle(pos, context.stoneRadius / 2);
		}

		if( bAnalysisEnabled && bAnalysisPondering && analysisPonderColor == mGame.getInstance().getNextPlayColor() )
		{
			drawAnalysis( g , context );
		}

		if( bestMoveVertex.isOnBoard() )
		{
			if( showBranchVertex == bestMoveVertex && !bestThinkInfo.vSeq.empty() )
			{
				mBoardRenderer.drawStoneSequence(context, bestThinkInfo.vSeq, mGame.getInstance().getNextPlayColor(), 0.7);
			}

			Vector2 pos = context.getIntersectionPos(bestMoveVertex.x, bestMoveVertex.y);
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawCircle(pos, 8);
		}

		FixString< 512 > str;

		g.setTextColor(Color3ub(255, 0, 0));

		SimpleTextLayout textLayout;
		{
			textLayout.posX = 100;
			textLayout.posY = 10;
			textLayout.offset = 15;
			RenderUtility::SetFont(g, FONT_S10);
			if( mGameMode == GameMode::Learning )
			{
				textLayout.show(g, "Num Game Completed = %d", numGameCompleted);
				if( bMatchJob )
				{
					if( matchChallenger != StoneColor::eEmpty )
					{
						textLayout.show(g, "Job Type = Match , Challenger = %s",
										matchChallenger == StoneColor::eBlack ? "B" : "W");
					}
					else
					{
						textLayout.show(g, "Job Type = Match");
					}
				}
				else
				{
					textLayout.show(g, "Job Type = Self Play , Weight = %s", mUsedWeight.c_str());
				}

				if( !mLastGameResult.empty() )
				{
					textLayout.show(g, "Last Game Result : %s", mLastGameResult.c_str());
				}
			}
			else if( mGameMode == GameMode::Match )
			{
				if( mMatchData.bAutoRun )
				{
					int totalMatchNum = mMatchData.players[0].winCount + mMatchData.players[1].winCount;
					int totalMatchNumHistory = totalMatchNum + mMatchData.historyWinCounts[0] + mMatchData.historyWinCounts[1];
					for( int i = 0; i < 2; ++i )
					{
						auto const& player = mMatchData.players[i];
						float winRate = totalMatchNum ? (100.f * float(player.winCount) / totalMatchNum) : 0;
						float winRateHistory = totalMatchNumHistory ? (100.f * float(player.winCount + mMatchData.historyWinCounts[i]) / totalMatchNumHistory) : 0;
						textLayout.show(g, "%s (%s) = %d ( %.1f %% , History = %d , %.1f %% )", 
										player.getName().c_str(), mMatchData.getPlayerColor(i) == StoneColor::eBlack ? "B" : "W", 
										player.winCount, winRate, mMatchData.historyWinCounts[i], winRateHistory);
					}
					if( unknownWinerCount )
					{
						textLayout.show(g, "Unknown = %d", unknownWinerCount);
					}
				}
			}
		}

		{
			g.setTextColor(Color3ub(255, 0, 0));
			RenderUtility::SetFont(g, FONT_S10);

			textLayout.offset = 15;
			textLayout.posX = 750;
			textLayout.posY = 25;

			if( bUpdateTerritory )
			{
				for( int i = 0; i < 2; ++i )
				{
					textLayout.show(g, "%c = %d , %.2f",
									i == 0 ? 'B' : 'W' ,
									mShowTerritoryInfo.fieldCounts[i], 
									float(mShowTerritoryInfo.totalRemainFields[i])/ Zen::TerritoryInfo::FieldValue );
				}
				float score = mShowTerritoryInfo.fieldCounts[0] - mShowTerritoryInfo.fieldCounts[1] - getViewingGame().getSetting().komi;
				float score2 = score + float(mShowTerritoryInfo.totalRemainFields[0] - mShowTerritoryInfo.totalRemainFields[1]) / Zen::TerritoryInfo::FieldValue;
#if 1
				textLayout.show(g, "%c +%.1f" ,score > 0 ? 'B' : 'W' , Math::Abs(score) );
#else
				textLayout.show(g, "%c +%.1f , %c +%.1f",
								score > 0 ? 'B' : 'W', Math::Abs(score),
								score2 > 0 ? 'B' : 'W', Math::Abs(score2));
#endif
			}
			if( bDrawDebugMsg )
			{
				textLayout.show(g, "bUseBatchedRender = %s", mBoardRenderer.bUseBatchedRender ? "true" : "false");
			}
		}

		if ( bDrawFontCacheTexture )
			DrawUtility::DrawTexture(FontCharCache::Get().mTextAtlas.getTexture(), Vector2(0, 0), Vector2(600, 600));

		g.endRender();
	}

	void LeelaZeroGoStage::drawAnalysis(GLGraphics2D& g, RenderContext &context)
	{
		GPU_PROFILE("Draw Analysis");

		auto iter = analysisResult.end();
		if( showBranchVertex != PlayVertex::Undefiend() )
		{
			iter = std::find_if(analysisResult.begin(), analysisResult.end(),
								[this](auto const& info) { return info.v == showBranchVertex; });
		}
		if( iter != analysisResult.end() )
		{
			mBoardRenderer.drawStoneSequence(context, iter->vSeq, mGame.getInstance().getNextPlayColor(), 0.7);
		}
		else
		{
			int maxVisited = 0;
			for( auto const& info : analysisResult )
			{
				if( info.nodeVisited > maxVisited )
					maxVisited = info.nodeVisited;
			}

			float const MIN_ALPHA = 32 / 255.0;
			float const MIN_ALPHA_TO_DISPLAY_TEXT = 64 / 255.0;
			float const MAX_ALPHA = 240 / 255.0;
			float const HUE_SCALING_FACTOR = 3.0;
			float const ALPHA_SCALING_FACTOR = 5.0;


			RenderUtility::SetFont(g, FONT_S8);
			for( auto const& info : analysisResult )
			{
				if( info.nodeVisited == 0 )
					continue;

				int x = info.v.x;
				int y = info.v.y;

				float percentVsisted = float(info.nodeVisited) / maxVisited;
				float hue = (float)(120 * std::max(0.0f, Math::Log(percentVsisted) / HUE_SCALING_FACTOR + 1));
				float saturation = 0.75f; //saturation
				float brightness = 0.85f; //brightness
				float alpha = MIN_ALPHA + (MAX_ALPHA - MIN_ALPHA) * std::max(0.0f, Math::Log(percentVsisted) /
																			 ALPHA_SCALING_FACTOR + 1);

				Vector3 color = FColorConv::HSVToRGB(Vector3(hue, saturation, brightness));

				Vector2 pos = context.getIntersectionPos(x, y);
				g.beginBlend(Vector2(0, 0), Vector2(0, 0), alpha);

				g.setPen(Color3f(color));
				g.setBrush(Color3f(color));
				g.drawCircle(pos, context.stoneRadius);
				g.endBlend();

				if( alpha > MIN_ALPHA_TO_DISPLAY_TEXT )
				{
					FixString<128> str;
					str.format("%g", info.winRate);
					float len = context.cellLength;
					g.drawText(pos - 0.5 * Vector2(len, len), Vector2(len, 0.5 * len), str);

					if( info.nodeVisited >= 1000000 )
					{
						float fVisited = float(info.nodeVisited) / 100000; // 1234567 -> 12.34567
						str.format("%gm", Math::Round(fVisited) / 10.0);
					}
					else if( info.nodeVisited >= 10000 )
					{
						float fVisited = float(info.nodeVisited) / 1000; // 13265 -> 13.265
						str.format("%gk", Math::Round(fVisited));
					}
					else if( info.nodeVisited >= 1000 )
					{
						float fVisited = float(info.nodeVisited) / 100; // 1265 -> 12.65
						str.format("%gk", Math::Round(fVisited) / 10.0);
					}
					else
					{
						str.format("%d", info.nodeVisited);
					}

					g.drawText(pos - 0.5 * Vector2(len, 0), Vector2(len, 0.5 * len), str);
				}
			}
		}
	}

	void LeelaZeroGoStage::drawWinRateDiagram(Vec2i const& renderPos, Vec2i const& renderSize)
	{
		if( mWinRateHistory[0].size() > 1 || mWinRateHistory[1].size() > 1 )
		{
			GPU_PROFILE("Draw WinRate Diagram");

			ViewportSaveScope viewportSave;

			Vec2i screenSize = ::Global::GetDrawEngine()->getScreenSize();

			RHISetViewport(renderPos.x, screenSize.y - ( renderPos.y + renderSize.y ) , renderSize.x, renderSize.y);
			MatrixSaveScope matrixSaveScope;
			glMatrixMode(GL_PROJECTION);

			float const xMin = 0;
			float const xMax = (mGame.getInstance().getCurrentStep() + 1) / 2 + 1;
			float const yMin = 0;
			float const yMax = 100;
			Matrix4 matProj = OrthoMatrix(xMin - 1, xMax, yMin - 5, yMax + 5, -1, 1);

			glLoadMatrixf(matProj);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			Vector3 colors[2] = { Vector3(1,0,0) , Vector3(0,1,0) };
			float alpha[2] = { 0.4 , 0.4 };

			for( int i = 0; i < 2; ++i )
			{
				auto& winRateHistory = mWinRateHistory[i];
				if( winRateHistory.empty() )
					continue;
				if( mProgUnderCurveArea )
				{
					if( i == 0 )
					{
						RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha >::GetRHI());
					}
					else
					{
						RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOne >::GetRHI());
					}

					GL_BIND_LOCK_OBJECT(mProgUnderCurveArea);

					mProgUnderCurveArea->setParameters(
						float(50), matProj,
						Vector4(colors[i], alpha[i]),
						Vector4(0.3 * colors[i], alpha[i]));

					TRenderRT< RTVF_XY >::DrawShader(PrimitiveType::LineStrip, &winRateHistory[0], winRateHistory.size());
				}
				RHISetBlendState(TStaticBlendState<>::GetRHI());
				TRenderRT< RTVF_XY >::Draw(PrimitiveType::LineStrip, &winRateHistory[0], winRateHistory.size(), colors[i]);
			}

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
				RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eOne , Blend::eOne >::GetRHI());
				TRenderRT< RTVF_XY >::Draw(PrimitiveType::LineList, &buffer[0], buffer.size(), LinearColor(0.3, 0.3, 0.3));
			}

			{
				Vector2 const lines[] =
				{
					Vector2(0,50), Vector2(xMax,50),
					Vector2(0,yMin) , Vector2(0,yMax),
					Vector2(0,yMin) , Vector2(xMax,yMin),
					Vector2(0,yMax) , Vector2(xMax,yMax),
				};
				RHISetBlendState(TStaticBlendState<>::GetRHI());
				TRenderRT< RTVF_XY >::Draw(PrimitiveType::LineList, &lines[0], ARRAY_SIZE(lines), LinearColor(0, 0, 1));
			}

			if( bReviewingGame )
			{
				int posX = ( mReviewGame.getInstance().getCurrentStep() + 1 ) / 2;
				Vector2 const lines[] = { Vector2(posX , yMin) , Vector2(posX , yMax) };

				RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
				TRenderRT< RTVF_XY >::Draw(PrimitiveType::LineList, &lines[0], ARRAY_SIZE(lines), LinearColor(1, 1, 0));
			}

			RHISetBlendState(TStaticBlendState<>::GetRHI());
		}
	}

	void LeelaZeroGoStage::updateViewGameTerritory()
	{
		if( !bUpdateTerritory )
			return;

		if( !mStatusQuery.queryTerritory(getViewingGame(), mShowTerritoryInfo) )
		{
			::LogWarning(0, "Can't get QueryTerritory");
		}
	}

	void LeelaZeroGoStage::DrawTerritoryStatus(BoardRenderer& renderer, RenderContext const& context, Zen::TerritoryInfo const& info)
	{
		GPU_PROFILE("Draw Territory");

		GLGraphics2D& g = ::Global::GetRHIGraphics2D();

		Vector2 cellSize = Vector2(context.cellLength, context.cellLength);
		RenderUtility::SetFont(g, FONT_S8);

		for( int i = 0; i < 19; ++i )
		{
			for( int j = 0; j < 19; ++j )
			{
				int value = info.map[j][i];
				if( value == 0 )
					continue;

				int color = context.board.getData(i, j);
				if( value > 0 )
				{
					if( color == StoneColor::eBlack )
						continue;
					g.setTextColor(Color3ub(255, 0, 0));
					g.setBrush(Color3ub(0, 0, 0));
					g.setPen(Color3ub(255, 255, 255));

				}
				else
				{
					if( color == StoneColor::eWhite )
						continue;
					g.setTextColor(Color3ub(0, 0, 255));
					g.setBrush(Color3ub(255, 255, 255));
					g.setPen(Color3ub(0, 0, 0));
					value = -value;
				}

				float sizeFactor = Math::Min( 1.0f , value / float(Zen::TerritoryInfo::FieldValue) );

				Vector2 pos = renderer.getIntersectionPos(context, i, j);
				Vector2 size = 0.5 * sizeFactor * cellSize;
				if( value >= Zen::TerritoryInfo::FieldValue )
				{
					g.drawRect(pos - 0.5 * size, size);
				}
				else
				{
					g.drawCircle(pos, 0.5 * size.x);
				}
#if 0
				FixString<128> str;
				str.format("%d", value);
				g.drawText( pos - 0.5 * cellSize, cellSize, str);
#endif
			}
		}
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


	template< class Fun >
	void LeelaZeroGoStage::executeAnalysisAICommand(Fun&& fun, bool bKeepPonder /*= true*/)
	{
		assert(bAnalysisEnabled);

		if( bAnalysisPondering )
			mLeelaAIRun.stopPonder();

		fun();

		analysisResult.clear();
		if( bAnalysisPondering && bKeepPonder )
		{
			analysisPonderColor = mGame.getInstance().getNextPlayColor();
			mLeelaAIRun.startPonder(analysisPonderColor);
		}
	}

	bool LeelaZeroGoStage::onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;

		if( msg.onLeftDown() )
		{
			Vec2i pos = RenderContext::CalcCoord(msg.getPos() , BoardPos , RenderBoardScale);

			if( !mGame.getBoard().checkRange(pos.x, pos.y) )
				return false;

			if( bTryPlayingGame )
			{
				mTryPlayGame.playStone(pos.x, pos.y);
			}
			else 
			{
				if( canPlay() )
				{
					if ( mGame.canPlay(pos.x , pos.y) )
						execPlayStoneCommand(pos);
				}
				else if ( mGameMode == GameMode::Match )
				{
					bool bForcePlay = InputManager::Get().isKeyDown(Keyboard::eCONTROL);
					if( bForcePlay )
					{
						auto bot = mMatchData.getCurTurnBot();
					}
				}
				else if( mGameMode == GameMode::Analysis )
				{
					int color = mGame.getInstance().getNextPlayColor();
					if( InputManager::Get().isKeyDown(Keyboard::eCONTROL) )
					{
						if( mGame.addStone(pos.x, pos.y, color) )
						{
							executeAnalysisAICommand([this, pos, color] {  mLeelaAIRun.addStone(pos.x, pos.y, color); });
						}
					}
					else
					{
						if( mGame.playStone(pos.x, pos.y) )
						{
							executeAnalysisAICommand([this, pos, color] {  mLeelaAIRun.playStone(pos.x, pos.y, color); });
						}
					}
				}
			}
		}
		else if( msg.onRightDown() )
		{
			if( bTryPlayingGame )
			{
				mTryPlayGame.undo();
			}
			else if( mGameMode == GameMode::Analysis )
			{
				if( mGame.undo() )
				{
					executeAnalysisAICommand([this] { mLeelaAIRun.undo(); });
				}
			}

		}
		else if( msg.onMoving() )
		{

			Vec2i pos = RenderContext::CalcCoord(msg.getPos(), BoardPos, RenderBoardScale);

			if( mGame.getBoard().checkRange(pos.x, pos.y) )
			{
				showBranchVertex.x = pos.x;
				showBranchVertex.y = pos.y; 
			}
			else
			{
				showBranchVertex = PlayVertex::Undefiend();
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
		case Keyboard::eL: mBoardRenderer.bDrawLinkInfo = !mBoardRenderer.bDrawLinkInfo; break;
		case Keyboard::eZ: mBoardRenderer.bUseBatchedRender = !mBoardRenderer.bUseBatchedRender; break;
		case Keyboard::eF2: bDrawDebugMsg = !bDrawDebugMsg; break;
		case Keyboard::eF5: saveMatchGameSGF(); break;
		case Keyboard::eF6: restartAutoMatch(); break;
		case Keyboard::eF9: ShaderManager::Get().reloadShader( *mProgUnderCurveArea ); break;
		case Keyboard::eX:
			if( !bAnalysisEnabled && mGameMode == GameMode::Match )
			{
				tryEnableAnalysis(true);
			}
			if( bAnalysisEnabled )
			{
				toggleAnalysisPonder();
			}
			break;
		case Keyboard::eC:
			if ( mGameMode == GameMode::Match )
			{
				if( mMatchData.players[0].type == ControllerType::eLeelaZero ||
				    mMatchData.players[1].type == ControllerType::eLeelaZero )
				{
					LeelaBot* bot = static_cast<LeelaBot*>((mMatchData.players[0].type == ControllerType::eLeelaZero) ? mMatchData.players[0].bot.get() : mMatchData.players[1].bot.get());
					bot->mAI.inputCommand("showboard\n", { GTPCommand::eNone , 0 });
				}
			}
			break;
		case Keyboard::eV: glEnable(GL_MULTISAMPLE); break;
		case Keyboard::eB: glDisable(GL_MULTISAMPLE); break;
		case Keyboard::eQ: bDrawFontCacheTexture = !bDrawFontCacheTexture; break;
		}
		return false;
	}

	bool LeelaZeroGoStage::buildLearningMode()
	{
		if( !mLeelaAIRun.buildLearningGame() )
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

	bool LeelaZeroGoStage::buildAnalysisMode()
	{
		if( !tryEnableAnalysis(false) )
			return false;

		mGameMode = GameMode::Analysis;
		GameSetting setting;
		setting.komi = 7.5;
		mGame.setSetting(setting);
		resetGameParam();
		return true;
	}

	bool LeelaZeroGoStage::buildPlayMode()
	{
		std::string bestWeigetName = LeelaAppRun::GetBestWeightName();
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
		path.format("%s/%s/%s" , LeelaAppRun::InstallDir , LEELA_NET_DIR_NAME ,  weightNameA );
		path.replace('/', '\\');
		if( SystemPlatform::OpenFileName(path, path.max_size(), nullptr) )
		{
			weightNameA = FileUtility::GetFileName(path);
			::Global::GameConfig().setKeyValue("LeelaLastOpenWeight", "Go", weightNameA);
		}

		std::string bestWeigetName = LeelaAppRun::GetBestWeightName();

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
			std::string bestWeigetName = LeelaAppRun::GetBestWeightName();
			FixString<256> path;
			path.format("%s/%s/%s", LeelaAppRun::InstallDir, LEELA_NET_DIR_NAME , bestWeigetName.c_str());
			path.replace('/', '\\');


			if( SystemPlatform::OpenFileName(path, path.max_size(), nullptr) )
			{
				setting.weightName = FileUtility::GetFileName(path);
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
		bool bSwap;
		auto matchResultData = mMatchResultMap.getMatchResult(mMatchData.players, setting , bSwap );
		if( matchResultData )
		{
			mMatchData.historyWinCounts[0] = matchResultData->winCounts[bSwap ? 1 : 0];
			mMatchData.historyWinCounts[1] = matchResultData->winCounts[bSwap ? 0 : 1];
		}
		else
		{
			mMatchData.historyWinCounts[0] = 0;
			mMatchData.historyWinCounts[1] = 0;
		}

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
			bot->thinkNextMove(mGame.getInstance().getNextPlayColor());
		}

		return true;
	}


	void LeelaZeroGoStage::cleanupModeData(bool bEndStage)
	{
		if( mGamePlayWidget )
		{
			if( bEndStage )
				mGamePlayWidget->destroy();
			else
				mGamePlayWidget->show(false);
		}
		bPauseGame = false;

		switch( mGameMode )
		{
		case GameMode::Learning:
			mLeelaAIRun.stop();
			break;
		case GameMode::Match:
			{
				mMatchResultMap.addMatchResult(mMatchData.players, mGame.getSetting());
				mMatchResultMap.save(MATCH_RESULT_PATH);
			}
			mMatchData.cleanup();
			break;
		}

		if( bAnalysisEnabled )
		{
			bAnalysisEnabled = false;
			mLeelaAIRun.stop();
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
			mPIDLeela = FPlatformProcess::FindPIDByNameAndParentPID(TEXT("leelaz.exe"), GetProcessId(mLeelaAIRun.process.getHandle()));
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

	void LeelaZeroGoStage::notifyPlayerCommand(int indexPlayer , GameCommand const& com)
	{
		int color = mGame.getInstance().getNextPlayColor();
		switch( com.id )
		{
		case GameCommand::ePass:
			{
				resetTurnParam();

				LogMsg("Pass");
				mGame.playPass();

				if( mGame.getInstance().getLastPassCount() >= 2 )
				{
					LogMsg("GameEnd");
					
					if( mMatchData.bSaveSGF )
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
					mMatchData.advanceStep();
					IBotInterface* bot = mMatchData.getCurTurnBot();
					if( bot )
					{
						bot->playPass(color);
						bot->thinkNextMove(mGame.getInstance().getNextPlayColor());
					}
					if( bAnalysisEnabled )
					{
						executeAnalysisAICommand([this] { mLeelaAIRun.playPass(); }, canAnalysisPonder( mMatchData.getCurTurnPlayer() ));
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

				if( mMatchData.bSaveSGF )
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

				if( !mWinRateHistory[indexPlayer].empty() )
				{
					mWinRateHistory[indexPlayer].pop_back();
				}
				mMatchData.advanceStep();
				IBotInterface* bot = mMatchData.getCurTurnBot();
				if( bot )
				{
					bot->undo();
				}
				if( bAnalysisEnabled )
				{
					executeAnalysisAICommand([this] { mLeelaAIRun.undo(); }, canAnalysisPonder(mMatchData.getCurTurnPlayer()));
				}
			}
			break;

		case GameCommand::eParam:
			switch( com.paramId )
			{
			case LeelaGameParam::eBestMoveVertex:
				{
					LeelaThinkInfo* info = static_cast<LeelaThinkInfo*>(com.ptrParam);
					bestMoveVertex = info->v;
					bestThinkInfo = *info;
				}
				break;
			case ZenGameParam::eBestMoveVertex:
				{
					Zen::ThinkInfo* info = static_cast<Zen::ThinkInfo*>(com.ptrParam);
					bestMoveVertex = info->v;
					bestThinkInfo.v = info->v;
					bestThinkInfo.nodeVisited = 0;
					bestThinkInfo.vSeq = info->vSeq;
					bestThinkInfo.winRate = info->winRate;
					bestThinkInfo.evalValue = 0;
				}
				break;
			case LeelaGameParam::eWinRate:
			case ZenGameParam::eWinRate:
				if ( !bPrevGameCom )
				{
					Vector2 v;
					v.x = ( mGame.getInstance().getCurrentStep() + 1 ) / 2;
					v.y = com.floatParam;
					mWinRateHistory[indexPlayer].push_back(v);

					if( mWinRateWidget == nullptr )
					{
						Vec2i screenSize = ::Global::GetDrawEngine()->getScreenSize();
						Vec2i widgetSize = { 260 , 310 };
						Vec2i widgetPos = { screenSize.x - (widgetSize.x + 20), screenSize.y - ( widgetSize.y + 20 ) };
						mWinRateWidget = new GFrame( UI_ANY , widgetPos , widgetSize , nullptr );
						mWinRateWidget->setColor(Color3ub(0, 0, 0));
						mWinRateWidget->setRenderCallback(
							RenderCallBack::Create([this](GWidget* widget)
							{
								Vec2i screenSize = ::Global::GetDrawEngine()->getScreenSize();
								Vec2i diagramPos  = widget->getWorldPos() + Vec2i(5, 5);
								Vec2i diagramSize = widget->getSize() - 2 * Vec2i(5, 5);
								drawWinRateDiagram(diagramPos, diagramSize);
							})
						);
						::Global::GUI().addWidget(mWinRateWidget);
					}
					//LogMsg("Win rate = %.2f", com.floatParam);
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

					mMatchData.advanceStep();
					IBotInterface* bot = mMatchData.getCurTurnBot();
					if( bot )
					{
						bot->playStone(com.pos[0], com.pos[1], color);
						bot->thinkNextMove(mGame.getInstance().getNextPlayColor());
					}
					if( bAnalysisEnabled )
					{
						int x = com.pos[0];
						int y = com.pos[1];
						executeAnalysisAICommand( [this,x,y,color] { mLeelaAIRun.playStone(x,y,color); }, canAnalysisPonder(mMatchData.getCurTurnPlayer()));
					}
				}
				else
				{
					LogMsg("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
				}

			}
			break;
		}
	}

	void LeelaZeroGoStage::processLearningCommand()
	{
		assert(mGameMode == GameMode::Learning);
		auto ProcFun = [&](GameCommand& com)
		{
			if( mbRestartLearning )
				return;

			int color = mGame.getInstance().getNextPlayColor();
			switch( com.id )
			{
			case GameCommand::eStart:
				{
					LogMsg("GameStart");
					mGame.restart();
					mBoardRenderer.generateNoiseOffset(mGame.getBoard().getSize());
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
					LogMsg("%s Resigned", name);
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
					case LeelaGameParam::eNetWeight:
						{	
							mUsedWeight = StringView(com.strParam, 8);
							for( int i = 0; i < ELFWeights.size(); ++i )
							{
								if( strcmp(com.strParam, ELFWeights[i]) != 0 )
									::Global::GameConfig().setKeyValue("LeelaLastNetWeight", "Go", com.strParam);
							}

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
						LogMsg("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
					}

					if( mGame.getInstance().getCurrentStep() - 1 >= ::Global::GameConfig().getIntValue( "LeelaLearnMaxSetp" , "Go" , 400 ) )
					{
						mbRestartLearning = true;
						return;
					}
				}
				break;
			}
		};

		mLeelaAIRun.outputThread->procOutputCommand(ProcFun);
	}


	void LeelaZeroGoStage::resetGameParam()
	{
		bPrevGameCom = true;
		mGame.restart();
		mBoardRenderer.generateNoiseOffset(mGame.getBoard().getSize());

		resetTurnParam();
		for( int i = 0; i < 2; ++i )
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

	bool LeelaZeroGoStage::tryEnableAnalysis(bool bCopyGame)
	{
		if( mGameMode == GameMode::Learning )
			return false;
		if( bAnalysisEnabled )
			return true;

		if( !mLeelaAIRun.buildAnalysisGame( false ) )
			return false;

		bAnalysisPondering = false;
		analysisResult.clear();
		bAnalysisEnabled = true;

		if ( bCopyGame )
		{
			class MyCopier : public IGameCopier
			{
			public:
				MyCopier(LeelaAppRun& AI) :AI(AI) {}

				virtual void emitSetup(GameSetting const& setting) override
				{
					AI.setupGame(setting);
				}
				virtual void emitPlayStone(int x, int y, int color) override
				{
					AI.playStone(x, y, color);
				}
				virtual void emitAddStone(int x, int y, int color) override
				{
					AI.addStone(x, y, color);
				}
				virtual void emitPlayPass(int color) override
				{
					AI.playPass(color);
				}
				virtual void emitUndo()
				{
					AI.undo();
				}

				LeelaAppRun& AI;
			};
			MyCopier copier(mLeelaAIRun);
			mGame.getInstance().synchronizeState(copier , true);
		}

		return true;
	}


	bool LeelaZeroGoStage::toggleAnalysisPonder()
	{
		assert(bAnalysisEnabled);
		bAnalysisPondering = !bAnalysisPondering;

		if( bAnalysisPondering )
		{
			analysisPonderColor = mGame.getInstance().getNextPlayColor();
			mLeelaAIRun.startPonder(analysisPonderColor);
		}
		else
		{
			mLeelaAIRun.stopPonder();
		}

		return bAnalysisPondering;
	}

	bool LeelaZeroGoStage::saveMatchGameSGF()
	{
		DateTime date = SystemPlatform::GetLocalTime();

		FixString< 512 > path;
		path.format("Go/Save/%04d-%02d-%02d-%02d-%02d-%02d.sgf", date.getYear(), date.getMonth(), date.getDay(), date.getHour(), date.getMinute(), date.getSecond());

		FixString< 512 > dateString;
		dateString.format("%d-%d-%d", date.getYear(), date.getMonth(), date.getDay());
		GameDescription description;
		description.blackName = mMatchData.getPlayer(StoneColor::eBlack).getName();
		description.whiteName = mMatchData.getPlayer(StoneColor::eWhite).getName();
		description.date = dateString.c_str();

		return mGame.getInstance().saveSGF(path, &description);
	}

	void LeelaZeroGoStage::restartAutoMatch()
	{
		LogMsg("==Restart Auto Match==");
		if( bSwapEachMatch )
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
			bot->thinkNextMove(mGame.getInstance().getNextPlayColor());
		}
	}

	void LeelaZeroGoStage::createPlayWidget()
	{
		if( mGamePlayWidget == nullptr )
		{
			Vec2i screenSize = Global::GetDrawEngine()->getScreenSize();

			Vec2i size = Vec2i(150,200);
			auto devFrame = new DevFrame(UI_ANY, Vec2i(screenSize.x - size.x - 5, 300), size , NULL);

			devFrame->addButton("Play Pass", [&](int eventId, GWidget* widget) -> bool
			{
				int color = mGame.getInstance().getNextPlayColor();
				if( canPlay() )
				{
					execPassCommand();
				}
				return false;
			});
			devFrame->addButton("Undo", [&](int eventId, GWidget* widget) -> bool
			{
				int color = mGame.getInstance().getNextPlayColor();
				if( canPlay() )
				{
					execUndoCommand();
				}
				else if( isBotTurn()  )
				{
					//FIXME
					mMatchData.getCurTurnBot()->requestUndo();
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
		int color = mGame.getInstance().getNextPlayColor();
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
		int color = mGame.getInstance().getNextPlayColor();
		assert(isPlayerControl(color));
		GameCommand com;
		com.id = GameCommand::ePass;
		notifyPlayerCommand(mMatchData.idxPlayerTurn, com);
	}

	void LeelaZeroGoStage::execUndoCommand()
	{
		int color = mGame.getInstance().getNextPlayColor();
		//assert(isPlayerControl(color));
		GameCommand com;
		com.id = GameCommand::eUndo;
		notifyPlayerCommand(mMatchData.idxPlayerTurn, com);
	}

	void LeelaZeroGoStage::resetTurnParam()
	{
		bestMoveVertex = PlayVertex::Undefiend();
	}

	void MatchSettingPanel::addBaseWidget()
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

		addCheckBox(UI_AUTO_RUN, "Auto Run", 0, sortOrder);
		addCheckBox(UI_SAVE_SGF, "Save SGF", 0, sortOrder)->bChecked = true;

		adjustChildLayout();

		Vec2i buttonSize = Vec2i(100, 20);
		GButton* button;
		button = new GButton(UI_PLAY, Vec2i(getSize().x / 2 - buttonSize.x, getSize().y - buttonSize.y - 5), buttonSize, this);
		button->setTitle("Play");
		button = new GButton(UI_CANCEL, Vec2i((getSize().x) / 2, getSize().y - buttonSize.y - 5), buttonSize, this);
		button->setTitle("Cancel");
	}

	void MatchSettingPanel::addLeelaParamWidget(int id, int idxPlayer)
	{
		LeelaAISetting setting = LeelaAISetting::GetDefalut();
		GTextCtrl* textCtrl;
		textCtrl = addTextCtrl(id + UPARAM_VISITS, "Visit Num", BIT(idxPlayer), idxPlayer);
		textCtrl->setValue(std::to_string(setting.visits).c_str());
		GFilePicker*  filePicker = addWidget< GFilePicker >(id + UPARAM_WEIGHT_NAME, "Weight Name", BIT(idxPlayer), idxPlayer);
		filePicker->filePath.format("%s/%s/%s", LeelaAppRun::InstallDir, LEELA_NET_DIR_NAME ,LeelaAppRun::GetBestWeightName().c_str());
		filePicker->filePath.replace('/', '\\');
	}

	void MatchSettingPanel::addZenParamWidget(int id, int idxPlayer)
	{
		Zen::CoreSetting setting = ZenBot::GetCoreConfigSetting();
		GTextCtrl* textCtrl;
		textCtrl = addTextCtrl(id + UPARAM_SIMULATIONS_NUM, "Num Simulations", BIT(idxPlayer), idxPlayer);
		textCtrl->setValue(std::to_string(setting.numSimulations).c_str());
		textCtrl = addTextCtrl(id + UPARAM_MAX_TIME, "Max Time", BIT(idxPlayer), idxPlayer);
		FixString<512> valueStr;
		textCtrl->setValue(valueStr.format("%g", setting.maxTime));
	}

	GChoice* MatchSettingPanel::addPlayerChoice(int idxPlayer, char const* title)
	{
		int id = idxPlayer ? UI_CONTROLLER_TYPE_B : UI_CONTROLLER_TYPE_A;
		GChoice* choice = addChoice(id, title, 0, idxPlayer);
		for( int i = 0; i < (int)ControllerType::Count; ++i )
		{
			uint slot = choice->addItem(GetControllerName(ControllerType(i)));
			choice->setItemData(slot, (void*)i);
		}

		choice->onEvent = [this, idxPlayer](int event, GWidget* ui)
		{
			removeChildWithMask(BIT(idxPlayer));
			switch( (ControllerType)(intptr_t)ui->cast< GChoice >()->getSelectedItemData() )
			{
			case ControllerType::eLeelaZero:
				addLeelaParamWidget(ui->getID(), idxPlayer);
				break;
			case ControllerType::eZenV7:
			case ControllerType::eZenV6:
			case ControllerType::eZenV4:
				addZenParamWidget(ui->getID(), idxPlayer);
			default:
				break;
			}
			adjustChildLayout();
			return false;
		};
		return choice;
	}

	bool MatchSettingPanel::setupMatchSetting(MatchGameData& matchData, GameSetting& setting)
	{
		ControllerType types[2] =
		{
			(ControllerType)(intptr_t)findChildT<GChoice>(UI_CONTROLLER_TYPE_A)->getSelectedItemData(),
			(ControllerType)(intptr_t)findChildT<GChoice>(UI_CONTROLLER_TYPE_B)->getSelectedItemData()
		};

		bool bHavePlayerController = types[0] == ControllerType::ePlayer || types[1] == ControllerType::ePlayer;

		matchData.bAutoRun = findChildT<GCheckBox>(UI_AUTO_RUN)->bChecked;
		matchData.bSaveSGF = findChildT<GCheckBox>(UI_SAVE_SGF)->bChecked;
		for( int i = 0; i < 2; ++i )
		{
			int id = (i) ? UI_CONTROLLER_TYPE_B : UI_CONTROLLER_TYPE_A;
			MatchPlayer* otherPlayer = (i) ? &matchData.players[0] : nullptr;
			switch( types[i] )
			{
			case ControllerType::eLeelaZero:
				{
					LeelaAISetting setting = LeelaAISetting::GetDefalut();
					std::string weightName = findChildT<GFilePicker>(id + UPARAM_WEIGHT_NAME)->filePath.c_str();
					setting.weightName = FileUtility::GetFileName(weightName.c_str());
					setting.visits = atoi(findChildT<GTextCtrl>(id + UPARAM_VISITS)->getValue());
					//setting.bUseModifyVersion = true;
					setting.seed = generateRandSeed();
					if( bHavePlayerController )
					{
						setting.resignpct = 0;
					}
					else
					{
						setting.randomcnt = 5;
					}
					if( !matchData.players[i].initialize(types[i], &setting, otherPlayer) )
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
					if( !matchData.players[i].initialize(types[i], &setting, otherPlayer) )
						return false;
				}
				break;
			default:
				if( !matchData.players[i].initialize(types[i],nullptr, otherPlayer) )
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