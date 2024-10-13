#include "LeelaZeroGoStage.h"

#include "Bots/AQBot.h"
#include "Bots/KataBot.h"
#include "Bots/MonitorBot.h"

#include "StringParse.h"
#include "RandomUtility.h"
#include "Widget/WidgetUtility.h"
#include "GameGlobal.h"
#include "PropertySet.h"
#include "FileSystem.h"
#include "InputManager.h"
#include "ConsoleSystem.h"

#include "RHI/RHIGraphics2D.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/ShaderManager.h"
#include "RHI/SimpleRenderState.h"

#include "Image/ImageProcessing.h"
#include "ProfileSystem.h"

#include <Dbghelp.h>
#include "Core/ScopeGuard.h"
#include "RenderUtility.h"


#define MATCH_RESULT_PATH "Go/MatchResult.data"

REGISTER_STAGE_ENTRY("LeelaZero Learning", Go::LeelaZeroGoStage, EExecGroup::Test, "Game|AI");

#define DERAULT_LEELA_WEIGHT_NAME "6615567eaa3adc8ea695682fcfbd7eaa3bb557d3720d2b61610b66006104050e"


#pragma comment (lib,"Dbghelp.lib")


namespace Go
{

	void RunConvertWeight(LeelaWeightTable const& table)
	{

		FileIterator fileIter;
		InlineString<256> dir;
		dir.format("%s/%s", LeelaAppRun::InstallDir, LEELA_NET_DIR_NAME);

		std::string bestWeightName = LeelaAppRun::GetBestWeightName();
		if( FFileSystem::FindFiles(dir, nullptr, fileIter) )
		{
			for( ; fileIter.haveMore(); fileIter.goNext() )
			{
				if ( FCString::Compare( fileIter.getFileName() , bestWeightName.c_str() ) == 0 )
					continue;

				char const* subName = FFileUtility::GetExtension(fileIter.getFileName());
				if( subName )
				{
					--subName;
				}

				if( !LeelaWeightTable::IsOfficialFormat( fileIter.getFileName(), subName ) )
					continue;

				InlineString< 32 > weightName;
				weightName.assign(fileIter.getFileName(), LeelaWeightInfo::DefaultNameLength);

				auto info = table.find(weightName.c_str());
				if( info )
				{
					InlineString<128> newName;
					info->getFormattedName(newName);
					if( subName )
					{
						newName += subName;
					}

					InlineString<256> filePath;
					filePath.format("%s/%s", dir.c_str(), fileIter.getFileName());

					FFileSystem::RenameFile(filePath, newName);
				}
			}
		}

	}

	class UnderCurveAreaProgram : public Render::GlobalShaderProgram
	{
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/UnderCurveArea";
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamAxisValue.bind(parameterMap, SHADER_PARAM(AxisValue));
			mParamProjMatrix.bind(parameterMap, SHADER_PARAM(ProjMatrix));
			mParamUpperColor.bind(parameterMap, SHADER_PARAM(UpperColor));
			mParamLowerColor.bind(parameterMap, SHADER_PARAM(LowerColor));
		}
		void setParameters(RHICommandList& commandList, float axisValue, Matrix4 const& projMatrix, Vector4 const& upperColor, Vector4 const& lowerColor)
		{
			setParam(commandList, mParamAxisValue, axisValue);
			setParam(commandList, mParamProjMatrix, projMatrix);
			setParam(commandList, mParamUpperColor, upperColor);
			setParam(commandList, mParamLowerColor, lowerColor);
		}

		ShaderParameter mParamAxisValue;
		ShaderParameter mParamProjMatrix;
		ShaderParameter mParamUpperColor;
		ShaderParameter mParamLowerColor;
	};


	template< bool bUseTessellation >
	class TUnderCurveAreaProgram : public UnderCurveAreaProgram
	{
		DECLARE_SHADER_PROGRAM(TUnderCurveAreaProgram, Global);

		typedef UnderCurveAreaProgram BaseClass;
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), bUseTessellation);
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			if (bUseTessellation)
			{
				static ShaderEntryInfo const entiesWithTessellation[] =
				{
					{ EShader::Vertex ,  SHADER_ENTRY(MainVS) },
					{ EShader::Hull ,  SHADER_ENTRY(MainHS) },
					{ EShader::Domain ,  SHADER_ENTRY(MainDS) },
					{ EShader::Geometry ,  SHADER_ENTRY(MainGS) },
					{ EShader::Pixel ,  SHADER_ENTRY(MainPS) },
				};	
				return entiesWithTessellation;
			}

			static ShaderEntryInfo const enties[] =
			{
				{ EShader::Vertex ,  SHADER_ENTRY(MainVS) },
				{ EShader::Geometry ,  SHADER_ENTRY(MainGS) },
				{ EShader::Pixel ,  SHADER_ENTRY(MainPS) },
			};
							
			return enties;
		}
	};

	IMPLEMENT_SHADER_PROGRAM_T(template<>, TUnderCurveAreaProgram<false>);
	IMPLEMENT_SHADER_PROGRAM_T(template<>, TUnderCurveAreaProgram<true>);

	class SplineProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(SplineProgram, Global);

		SHADER_PERMUTATION_TYPE_INT(SplineType, SHADER_PARAM(SPLINE_TYPE), 0 , 1);
		using PermutationDomain = TShaderPermutationDomain<SplineType>;

		using BaseClass = GlobalShaderProgram;

		static bool constexpr UseTessellation = true;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{

		}

		void setParameters()
		{

		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Spline";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
			if (UseTessellation)
			{
				static ShaderEntryInfo entriesWithTessellation[] =
				{
					{ EShader::Vertex , SHADER_ENTRY(MainVS) },
					{ EShader::Hull   , SHADER_ENTRY(MainHS) },
					{ EShader::Domain , SHADER_ENTRY(MainDS) },
					{ EShader::Pixel , SHADER_ENTRY(MainPS) },
				};
				return entriesWithTessellation;
			}

			static ShaderEntryInfo entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_TESSELLATION), UseTessellation);
		}
	};

	IMPLEMENT_SHADER_PROGRAM(SplineProgram);

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


	ZenithGo7Hook GHook;


	TConsoleVariable< int > TestValue(0, "TestValue");

	template< class TGraphics2D >
	static void DrawRect(TGraphics2D& g, Vector2 const& pos, Vector2 const& size, Vector2 const& poivt)
	{
#if 0
		Vector2 sizeAbs = size;
		Vector2 renderPos = pos;
		if( size.x < 0 )
		{
			sizeAbs.x = -size.x;
			renderPos.x -= sizeAbs.x;
		}
		if( size.y < 0 )
		{
			sizeAbs.y = -size.y;
			renderPos.y -= sizeAbs.y;
		}
		renderPos -= sizeAbs * poivt;
		g.drawRect(renderPos, sizeAbs);
#else
		Vector2 renderPos = pos - Abs(size) * poivt;
		g.drawRect(renderPos, size);
#endif

	}

	float GWinRateFrameSizeScale = 1.0f;
	LeelaZeroGoStage::LeelaZeroGoStage()
		:CVarWinRateFrameSizeScale(
			[]() -> float 
			{ 
				return GWinRateFrameSizeScale; 
			} ,
			[this](float value) 
			{ 
				if (mWinRateWidget)
				{
					Vector2 size = Vector2(mWinRateWidget->getSize() ) * (value / GWinRateFrameSizeScale);
					mWinRateWidget->setSize(size);
					GWinRateFrameSizeScale = value;
				}
			},
			"go.WinRateFrameSizeScale"
		)
	{

	}

	bool LeelaZeroGoStage::onInit()
	{
		GHook.initialize();

		if( !BaseClass::onInit() )
			return false;

		//ILocalization::Get().changeLanguage(LAN_ENGLISH);
		using namespace Render;
#if 0
		if( !DumpFunSymbol("Zen7.dump", "aa.txt") )
			return false;
#endif


		LeelaAppRun::InstallDir = ::Global::GameConfig().getStringValue("Leela.InstallDir", "Go" , "E:/Desktop/LeelaZero");
		KataAppRun::InstallDir = ::Global::GameConfig().getStringValue("Kata.InstallDir", "Go", "E:/Desktop/KataGo");
		AQAppRun::InstallDir = ::Global::GameConfig().getStringValue("AQ.InstallDir", "Go", "E:/Desktop/AQ");
		

		::Global::GUI().cleanupWidget();

		mGame.setup(19);

		auto GameActionDelegate = [&](GameProxy& game)
		{
			if( &game == &getViewingGame() )
			{
				updateViewGameTerritory();

			}

			if (bAnalysisEnabled)
			{
				if (&game == &getAnalysisGame())
				{
					synchronizeAnalysisState();
				}
			}

			if( bReviewingGame && &game == &mGame && mGame.guid == mReviewGame.guid )
			{
				mGame.synchronizeHistory(mReviewGame);
			}
		};

		mGame.onStateChanged = GameActionDelegate;
		mReviewGame.onStateChanged = GameActionDelegate;
		mTryPlayGame.onStateChanged = GameActionDelegate;

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
		return true;
	}

	void LeelaZeroGoStage::postInit()
	{
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
				buildAnalysisMode(false);

				mReviewGame.copy(mGame);
				Vec2i screenSize = ::Global::GetScreenSize();
				Vec2i widgetSize = Vec2i(150, 10);
				auto frame = new DevFrame(UI_ANY, Vec2i(screenSize.x - widgetSize.x - 5, 300), widgetSize, nullptr);
				auto GetPonderingButtonString = [&]()
				{
					return bAnalysisPondering ? "Stop Pondering" : "Start Pondering";
				};
				frame->addButton(GetPonderingButtonString(),
								 [&, GetPonderingButtonString](int eventId, GWidget* widget) ->bool
				{
					toggleAnalysisPonder();
					widget->cast<GButton>()->setTitle(GetPonderingButtonString());
					return false;
				});
				FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Show Analysis"), bShowAnalysis);
				frame->addButton("Restart Leela", [this](int eventId, GWidget* widget)->bool
				{
					cleanupModeData();
					buildAnalysisMode(false);
					return false;
				});
				mModeWidget = widget;
				::Global::GUI().addWidget(frame);

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

			InlineString<256> path;
			path.format("%s/%s/%s", LeelaAppRun::InstallDir, LEELA_NET_DIR_NAME, "table.txt");
			if( !table.load(path) )
				return false;

			RunConvertWeight( table );
			mMatchResultMap.convertLeelaWeight(table);
			return false;
		});

		devFrame->addButton("Run Custom Match", [&](int eventId, GWidget* widget) ->bool 
		{
			auto* panel = new MatchSettingPanel( UI_ANY , Vec2i( 100 , 100 ) , Vec2i(300 , 400 ) , nullptr );
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

			if (bAnalysisEnabled)
			{
				synchronizeAnalysisState();
			}

			if( bReviewingGame )
			{
				mReviewGame.copy(mGame);
				widget->cast<GCheckBox>()->setTitle("Close Review");
				Vec2i screenSize = ::Global::GetScreenSize();
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
				bTerritoryInfoValid = false;
			}
			updateViewGameTerritory();
			return false;
		});

		devFrame->addButton("Load Game", [&](int eventId, GWidget* widget) ->bool
		{
			InlineString<512> path = "";
			if (SystemPlatform::OpenFileName(path, path.max_size(), { {"SGF files" , "*.sgf"} , { "All files" , "*.*"} }, 
				nullptr , nullptr , ::Global::GetDrawEngine().getWindowHandle()))
			{
				mGame.load(path);
				LogMsg(path);
			}
			return false;
		});

	}

	void LeelaZeroGoStage::onEnd()
	{
		cleanupModeData( true );
		mBoardRenderer.releaseRHI();
		BaseClass::onEnd();
	}

	void LeelaZeroGoStage::onUpdate(GameTimeSpan deltaTime)
	{
		GHook.update(long(deltaTime));

		BaseClass::onUpdate(deltaTime);

		if( mGameMode == GameMode::Match )
		{
			if ( !bPauseGame )
			{
				IBot* bot = mMatchData.getCurTurnBot();
				for(auto& player : mMatchData.players)
				{
					IBot* bot = player.bot.get();

					if( bot )
					{
						struct MyListener : IGameCommandListener
						{
							LeelaZeroGoStage* me;
							int  indexPlayer;
							void notifyCommand(GameCommand const& com) override
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
				keepLeelaProcessRunning(long(deltaTime));
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
							if (!static_cast<LeelaThinkInfoVec*>(com.ptrParam)->empty())
							{
								mAnalysisResult.dataType = ControllerType::eLeelaZero;
								mAnalysisResult.candidatePosList.clear();

								for (auto const& thinkInfo : *static_cast<LeelaThinkInfoVec*>(com.ptrParam))
								{
									AnalysisData::PosInfo info;
									info.v = thinkInfo.v;
									info.vSeq = thinkInfo.vSeq;
									info.nodeVisited = thinkInfo.nodeVisited;
									info.winRate = info.winRate;
									info.evalValue = info.evalValue;
									mAnalysisResult.candidatePosList.push_back(info);
								}

							}
							break;
						}
					}
					break;
				}
			};
			mLeelaAIRun.outputThread->procOutputCommand(ProcFun);
		}

		bPrevGameCom = false;
	}


	void LeelaZeroGoStage::onRender(float dFrame)
	{
		using namespace Render;

		using namespace Go;

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		RHICommandList& commandList = g.getCommandList();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);

		g.beginRender();
		SimpleRenderState renderState;

		auto& viewingGame = getViewingGame();

		RenderContext context( viewingGame.getBoard() , BoardPos , RenderBoardScale );

		mBoardRenderer.draw( g , renderState, context , mbBotBoardStateValid ? mBotBoardState : nullptr);

		if( bShowTerritory && bTerritoryInfoValid )
		{
			DrawTerritoryStatus(mBoardRenderer, renderState, context, mShowTerritoryInfo);
		}

		auto lastPlayPos =  viewingGame.getInstance().getLastStepPos();
		if( lastPlayPos.isOnBoard() )
		{
			Vector2 pos = mBoardRenderer.getStonePos(context, lastPlayPos.x, lastPlayPos.y);
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawCircle(pos, context.stoneRadius / 2);
		}

		if( bAnalysisEnabled && bShowAnalysis && analysisPonderColor == getAnalysisGame().getInstance().getNextPlayColor() )
		{
			drawAnalysis( g , renderState, context , getAnalysisGame(), mAnalysisResult);
		}

		if (mMatchData.getCurTurnPlayer().type == ControllerType::eKata)
		{
			IBot* playingBot = mMatchData.getCurTurnBot();
			if (playingBot->isThinking())
			{
				drawAnalysis(g, renderState, context, mGame, mAnalysisResult);
			}
		}

		if( bestMoveVertex.isOnBoard() )
		{
			if( showBranchVertex == bestMoveVertex && !bestThinkInfo.vSeq.empty() )
			{
				mBoardRenderer.drawStoneSequence(g, renderState, context, bestThinkInfo.vSeq, mGame.getInstance().getNextPlayColor(), 0.7);
			}

			Vector2 pos = context.getIntersectionPos(bestMoveVertex.x, bestMoveVertex.y);
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawCircle(pos, 8);
		}


		int debugId = WindowImageHook::eEdgeDetect;
		if( GHook.mDebugTextures[debugId].isValid() )
		{
			g.drawTexture(*GHook.mDebugTextures[debugId], Vector2(100, 100), Vector2(600, 600));
		}
		
		InlineString< 512 > str;
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
					if( matchChallenger != EStoneColor::Empty )
					{
						textLayout.show(g, "Job Type = Match , Challenger = %s",
										matchChallenger == EStoneColor::Black ? "B" : "W");
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
										player.getName().c_str(), mMatchData.getPlayerColor(i) == EStoneColor::Black ? "B" : "W", 
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

			if( bUpdateTerritory && bTerritoryInfoValid )
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

		if( bDrawFontCacheTexture )
		{
			g.setBrush(Color3f(1, 1, 1));
			g.drawTexture(FontCharCache::Get().mTextAtlas.getTexture(), Vector2(0, 0), Vector2(600, 600));
		}

		g.endRender();
	}

	void LeelaZeroGoStage::drawAnalysis(RHIGraphics2D& g, SimpleRenderState& renderState , RenderContext &context , GameProxy& game , AnalysisData const& data)
	{
		if (data.candidatePosList.empty())
			return;

		GPU_PROFILE("Draw Analysis");

		auto iter = data.candidatePosList.end();
		if( showBranchVertex != PlayVertex::Undefiend() )
		{
			iter = std::find_if(data.candidatePosList.begin(), data.candidatePosList.end(),
								[this](auto const& info) { return info.v == showBranchVertex; });
		}
		if( iter != data.candidatePosList.end() )
		{
			mBoardRenderer.drawStoneSequence(g, renderState, context, iter->vSeq, game.getInstance().getNextPlayColor(), 0.7);
		}
		else
		{
			int maxVisited = 0;
			for( auto const& info : data.candidatePosList )
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
			for( auto const& info : data.candidatePosList )
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
					InlineString<128> str;
					str.format("%.2f", info.winRate);
					float len = context.cellLength;
					g.drawText(pos - 0.5 * Vector2(len, len), Vector2(len, 0.5 * len), str);

					if( info.nodeVisited >= 1000000 )
					{
						float fVisited = float(info.nodeVisited) / 100000; // 1234567 -> 12.34567
						str.format("%gm", Math::Round(fVisited) / 10.0);
					}
					else if( info.nodeVisited >= 10000 )
					{
						float fVisited = float(info.nodeVisited) / 1000; // 12345 -> 12.345
						str.format("%gk", Math::Round(fVisited));
					}
					else if( info.nodeVisited >= 1000 )
					{
						float fVisited = float(info.nodeVisited) / 100; // 1234 -> 12.34
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

	TConsoleVariable< bool > CVarUseSpline(true, "go.UseSpline");


	void UpdateCRSplineIndices(TArray< uint32 >& inoutIndices, int newPointsCount)
	{
		int oldPointsCount = inoutIndices.size() / 4;
		if (oldPointsCount)
		{
			oldPointsCount += 1;
		}
		if (oldPointsCount == newPointsCount)
			return;

		int lineCount = Math::Max(0, newPointsCount - 1);
		inoutIndices.resize(4 * lineCount);
		if (lineCount == 0)
			return;

		uint32* pIndices = inoutIndices.data();
		if (lineCount == 1)
		{
			pIndices[0] = 0;
			pIndices[1] = 0;
			pIndices[2] = 1;
			pIndices[3] = 1;
			return;
		}

		if (newPointsCount < oldPointsCount)
		{
			pIndices += 4 * (lineCount - 1);
			int index = lineCount;

			pIndices[0] = index - 1;
			pIndices[1] = index;
			pIndices[2] = index + 1;
			pIndices[3] = index + 1;
		}
		else
		{
			int index;
			if (oldPointsCount == 2)
			{
				pIndices[0] = 0;
				pIndices[1] = 0;
				pIndices[2] = 1;
				pIndices[3] = 2;
				index = 1;
				pIndices += 4;
			}
			else
			{
				index = Math::Max(1, oldPointsCount - 2);
				pIndices += 4 * index;
			}

			for (; index < lineCount - 1; ++index)
			{
				pIndices[0] = index - 1;
				pIndices[1] = index;
				pIndices[2] = index + 1;
				pIndices[3] = index + 2;
				pIndices += 4;
			}

			pIndices[0] = index - 1;
			pIndices[1] = index;
			pIndices[2] = index + 1;
			pIndices[3] = index + 1;
		}
	}

	void LeelaZeroGoStage::drawWinRateDiagram(Vec2i const& renderPos, Vec2i const& renderSize)
	{
		::Global::GetRHIGraphics2D().flush();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		if(mWinRateDataList[0].history.size() > 1 || mWinRateDataList[1].history.size() > 1 )
		{
			GPU_PROFILE("Draw WinRate Diagram");

			Vec2i screenSize = ::Global::GetScreenSize();
			if (GRHISystem->getName() == RHISystemName::OpenGL)
			{
				RHISetViewport(commandList, renderPos.x, screenSize.y - (renderPos.y + renderSize.y), renderSize.x, renderSize.y);
			}
			else
			{
				RHISetViewport(commandList, renderPos.x, renderPos.y , renderSize.x, renderSize.y);
			}
			MatrixSaveScope matrixSaveScope;

			float const xMin = 0;
			float const xMax = (mGame.getInstance().getCurrentStep() + 1) / 2 + 1;
			float const yMin = 0;
			float const yMax = 100;
			Matrix4 matProj = AdjProjectionMatrixForRHI( OrthoMatrix(xMin - 1, xMax, yMin - 5, yMax + 5, -1, 1) );

			Vector3 colors[2] = { Vector3(1,0,0) , Vector3(0,1,0) };
			float alpha[2] = { 0.4 , 0.4 };

			bool const bUseSpline = CVarUseSpline && GRHISystem->getName() == RHISystemName::OpenGL;

			for( int i = 0; i < 2; ++i )
			{
				auto& winRateData = mWinRateDataList[i];
				auto& winRateHistory = winRateData.history;
				if( winRateHistory.empty() )
					continue;

				auto DrawCommand = [&](bool bUseColor)
				{
					if (bUseSpline)
					{
						if (winRateHistory.size() >= 2)
						{
							TArray<uint32>& indices = winRateData.splineIndeics;
							UpdateCRSplineIndices(indices, winRateHistory.size());

							if (bUseColor)
							{
								TRenderRT< RTVF_XY > ::DrawIndexed(commandList, EPrimitive::PatchPoint4,
									winRateHistory.data(), winRateHistory.size(), indices.data(), indices.size(), colors[i]);
							}
							else
							{

								TRenderRT< RTVF_XY > ::DrawIndexed(commandList, EPrimitive::PatchPoint4,
									winRateHistory.data(), winRateHistory.size(), indices.data(), indices.size());
							}
						}
					}
					else
					{
						if (bUseColor)
						{
							TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineStrip, winRateHistory.data(), winRateHistory.size(), colors[i]);
						}
						else
						{
							TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineStrip, winRateHistory.data(), winRateHistory.size());
						}
					}

				};


				//#TODO : ShaderCode
				if ( GRHISystem->getName() == RHISystemName::OpenGL )
				{
					if( i == 0 )
					{
						RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
					}
					else
					{
						RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::SrcAlpha, EBlend::One >::GetRHI());
					}

					UnderCurveAreaProgram* prog = bUseSpline ?
						(UnderCurveAreaProgram*)ShaderManager::Get().getGlobalShaderT < TUnderCurveAreaProgram<true> >() : 
						(UnderCurveAreaProgram*)ShaderManager::Get().getGlobalShaderT < TUnderCurveAreaProgram<false> >();

					RHISetShaderProgram(commandList, prog->getRHI());
					prog->setParameters(commandList,
						float(50), matProj,
						Vector4(colors[i], alpha[i]),
						Vector4(0.3 * colors[i], alpha[i]));

					if (bUseSpline)
					{
						prog->setParam(commandList, SHADER_PARAM(TessFactor), 32);
					}

					DrawCommand(false);
				}

				{
					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

					//#TODO : ShaderCode
					if (bUseSpline)
					{
						SplineProgram::PermutationDomain permutationVector;
						permutationVector.set< SplineProgram::SplineType >(1);

						SplineProgram* progSpline = ShaderManager::Get().getGlobalShaderT<SplineProgram>(permutationVector);
						RHISetShaderProgram(commandList, progSpline->getRHI());
						progSpline->setParam(commandList, SHADER_PARAM(XForm), matProj);
						progSpline->setParam(commandList, SHADER_PARAM(TessFactor), 32);
					}
					else
					{
						RHISetFixedShaderPipelineState(commandList, matProj);
					}

					DrawCommand(true);
				}

			}

			static TArray<Vector2> buffer;
			buffer.clear();
			float offset = 25;
			for( float y = 10; y < yMax; y += 10 )
			{
				buffer.emplace_back(xMin, y);
				buffer.emplace_back(xMax, y);
			}
			for( float x = offset; x < xMax; x += offset )
			{
				buffer.emplace_back(x, yMin);
				buffer.emplace_back(x, yMax);
			}

			RHISetFixedShaderPipelineState(commandList, matProj);
			if( !buffer.empty() )
			{			
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One , EBlend::One >::GetRHI());		
				TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &buffer[0], buffer.size(), LinearColor(0, 0, 1));
			}

			{
				Vector2 const lines[] =
				{
					Vector2(0,50), Vector2(xMax,50),
					Vector2(0,yMin) , Vector2(0,yMax),
					Vector2(0,yMin) , Vector2(xMax,yMin),
					Vector2(0,yMax) , Vector2(xMax,yMax),
				};
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &lines[0], ARRAY_SIZE(lines), LinearColor(0, 0, 1));
			}

			if( bReviewingGame )
			{
				int posX = ( mReviewGame.getInstance().getCurrentStep() + 1 ) / 2;
				Vector2 const lines[] = { Vector2(posX , yMin) , Vector2(posX , yMax) };

				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
				TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &lines[0], ARRAY_SIZE(lines), LinearColor(1, 1, 0));
			}

			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

		}


		::Global::GetRHIGraphics2D().restoreRenderState();
	}

	void LeelaZeroGoStage::updateViewGameTerritory()
	{
		if( !bUpdateTerritory )
			return;

		bTerritoryInfoValid = mStatusQuery.queryTerritory(getViewingGame(), mShowTerritoryInfo);

		if( !bTerritoryInfoValid )
		{
			LogWarning(0, "Can't get QueryTerritory");
		}
	}

	void LeelaZeroGoStage::DrawTerritoryStatus(BoardRenderer& renderer, SimpleRenderState& renderState, RenderContext const& context, Zen::TerritoryInfo const& info)
	{
		GPU_PROFILE("Draw Territory");

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

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
					if( color == EStoneColor::Black )
						continue;
					g.setTextColor(Color3ub(255, 0, 0));
					g.setBrush(Color3ub(0, 0, 0));
					g.setPen(Color3ub(255, 255, 255));

				}
				else
				{
					if( color == EStoneColor::White )
						continue;
					g.setTextColor(Color3ub(0, 0, 255));
					g.setBrush(Color3ub(255, 255, 255));
					g.setPen(Color3ub(0, 0, 0));
					value = -value;
				}

				float sizeFactor = Math::Min( 1.0f , value / float(Zen::TerritoryInfo::FieldValue) );

				Vector2 pos = context.getIntersectionPos(i, j);
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
				InlineString<128> str;
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


	template< class TFunc >
	void LeelaZeroGoStage::executeAnalysisAICommand(TFunc&& func, bool bKeepPonder /*= true*/)
	{
		assert(bAnalysisEnabled);

		if( bAnalysisPondering )
			mLeelaAIRun.stopPonder();

		func();

		analysisResult.clear();
		if( bAnalysisPondering && bKeepPonder )
		{
			analysisPonderColor = getAnalysisGame().getInstance().getNextPlayColor();
			mLeelaAIRun.startPonder(analysisPonderColor);
		}
	}

	MsgReply LeelaZeroGoStage::onMouse(MouseMsg const& msg)
	{
		if( msg.onLeftDown() )
		{
			Vec2i pos = RenderContext::CalcCoord(msg.getPos() , BoardPos , RenderBoardScale, mGame.getBoard().getSize());

			if( !mGame.getBoard().checkRange(pos.x, pos.y) )
				return MsgReply::Handled();

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
					bool bForcePlay = InputManager::Get().isKeyDown(EKeyCode::Control);
					if( bForcePlay )
					{
						auto bot = mMatchData.getCurTurnBot();
					}
				}
				else if( mGameMode == GameMode::Analysis )
				{
					int color = mGame.getInstance().getNextPlayColor();
					if( InputManager::Get().isKeyDown(EKeyCode::Control) )
					{
						mGame.addStone(pos.x, pos.y, color);
					}
					else
					{
						mGame.playStone(pos.x, pos.y);
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
				mGame.undo();
			}

		}
		else if( msg.onMoving() )
		{

			Vec2i pos = RenderContext::CalcCoord(msg.getPos(), BoardPos, RenderBoardScale, mGame.getBoard().getSize());

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
		return BaseClass::onMouse(msg);
	}

	MsgReply LeelaZeroGoStage::onKey( KeyMsg const& msg )
	{
		if(msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::L: mBoardRenderer.bDrawLinkInfo = !mBoardRenderer.bDrawLinkInfo; break;
			case EKeyCode::Z: mBoardRenderer.bUseBatchedRender = !mBoardRenderer.bUseBatchedRender; break;
			case EKeyCode::N: mBoardRenderer.bUseNoiseOffset = !mBoardRenderer.bUseNoiseOffset; break;
			case EKeyCode::O:
				{
					if (mGameMode == GameMode::Match)
					{
						if (mbBotBoardStateValid)
						{
							mbBotBoardStateValid = !mbBotBoardStateValid;
						}
						else
						{
							IBot* bot = mMatchData.players[0].bot ? mMatchData.players[0].bot.get() : mMatchData.players[1].bot.get();
							if (bot)
							{
								if (bot->readBoard(mBotBoardState) == BOT_OK)
								{
									mbBotBoardStateValid = true;
									LogMsg("Show Bot Board State");
								}
							}
						}
					}
				}
				break;
			case EKeyCode::F2: bDrawDebugMsg = !bDrawDebugMsg; break;
			case EKeyCode::F5: saveMatchGameSGF(); break;
			case EKeyCode::F6: restartAutoMatch(); break;
			case EKeyCode::X:
				if (!bAnalysisEnabled && mGameMode == GameMode::Match)
				{
					tryEnableAnalysis(true);
				}
				if (bAnalysisEnabled)
				{
					toggleAnalysisPonder();
				}
				break;
			case EKeyCode::C:
				if (mGameMode == GameMode::Match)
				{
					if (mMatchData.players[0].type == ControllerType::eLeelaZero ||
						mMatchData.players[1].type == ControllerType::eLeelaZero)
					{
						auto* bot = static_cast<LeelaBot*>((mMatchData.players[0].type == ControllerType::eLeelaZero) ? mMatchData.players[0].bot.get() : mMatchData.players[1].bot.get());
						bot->mAI.inputCommand("showboard\n", { GTPCommand::eNone , 0 });
					}
				}
				break;
			case EKeyCode::V: glEnable(GL_MULTISAMPLE); break;
			case EKeyCode::B: glDisable(GL_MULTISAMPLE); break;
			case EKeyCode::Q: bDrawFontCacheTexture = !bDrawFontCacheTexture; break;
			}
		}

		return MsgReply::Handled();
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

	bool LeelaZeroGoStage::buildAnalysisMode( bool bRestartGame )
	{
		if( !tryEnableAnalysis( !bRestartGame ) )
			return false;

		mGameMode = GameMode::Analysis;
		if( bRestartGame )
		{
			GameSetting setting;
			setting.komi = 7.5;
			mGame.setSetting(setting);
			resetGameParam();
		}
		return true;
	}

	bool LeelaZeroGoStage::buildPlayMode()
	{
		std::string bestWeigetName = LeelaAppRun::GetBestWeightName();
		char const* weightName = nullptr;
		if( !bestWeigetName.empty() )
		{
			weightName = bestWeigetName.c_str();
			::Global::GameConfig().setKeyValue("Leela.LastMatchWeight", "Go", weightName);
		}
		else
		{
			weightName = Global::GameConfig().getStringValue("Leela.LastMatchWeight", "Go", DERAULT_LEELA_WEIGHT_NAME);
		}
		{
			if( !mMatchData.players[0].initialize(ControllerType::ePlayer) )
				return false;
		}

		{
			TBotSettingData< LeelaAISetting > setting;
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
		char const* weightNameA = ::Global::GameConfig().getStringValue( "Leela.LastOpenWeight" , "Go" , DERAULT_LEELA_WEIGHT_NAME );

		InlineString<256> path;
		path.format("%s/%s/%s" , LeelaAppRun::InstallDir , LEELA_NET_DIR_NAME ,  weightNameA );
		path.replace('/', '\\');
		if (SystemPlatform::OpenFileName(path, path.max_size(), {} , nullptr, nullptr , ::Global::GetDrawEngine().getWindowHandle()))
		{
			weightNameA = FFileUtility::GetFileName(path);
			::Global::GameConfig().setKeyValue("Leela.LastOpenWeight", "Go", weightNameA);
		}

		std::string bestWeigetName = LeelaAppRun::GetBestWeightName();

		{
			TBotSettingData<LeelaAISetting> setting;
			setting.weightName = weightNameA;
			setting.seed = GenerateRandSeed();
			setting.visits = 6000;
			setting.playouts = 0;
			setting.bNoise = true;
			if( !mMatchData.players[0].initialize(ControllerType::eLeelaZero, &setting) )
				return false;
		}

		{
			TBotSettingData<LeelaAISetting> setting;
			setting.weightName = bestWeigetName.c_str();
			setting.seed = GenerateRandSeed();
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
			setting.bBlackFirst = false;
			setting.komi = 0.5;
		}
		else
		{
			setting.bBlackFirst = true;
			setting.komi = 7.5;
		}
		
		if( !startMatchGame(setting) )
			return false;

		return true;
	}

	bool LeelaZeroGoStage::buildLeelaZenMatchMode()
	{
		{
			TBotSettingData<LeelaAISetting> setting = LeelaAISetting::GetDefalut();
			std::string bestWeigetName = LeelaAppRun::GetBestWeightName();
			InlineString<256> path;
			path.format("%s/%s/%s", LeelaAppRun::InstallDir, LEELA_NET_DIR_NAME , bestWeigetName.c_str());
			path.replace('/', '\\');


			if (SystemPlatform::OpenFileName(path, path.max_size(), {} , nullptr))
			{
				setting.weightName = FFileUtility::GetFileName(path);
			}
			else
			{
				setting.weightName = bestWeigetName.c_str();
			}

			if( !mMatchData.players[0].initialize(ControllerType::eLeelaZero, &setting) )
				return false;
		}

		{
			TBotSettingData< Zen::CoreSetting > setting;
			int numCPU = SystemPlatform::GetProcessorNumber();
			setting.numThreads = numCPU - 2;
			setting.numSimulations = 20000000;
			setting.maxTime = 25;
			if( !mMatchData.players[1].initialize(ControllerType::eZen , &setting) )
				return false;
		}

		mMatchData.bSwapColor = true;

		GameSetting setting;
		setting.numHandicap = 0;
		if( setting.numHandicap )
			setting.bBlackFirst = false;

		if( !startMatchGame(setting) )
			return false;

		return true;
	}



	bool LeelaZeroGoStage::startMatchGame(GameSetting const& setting)
	{
		mGameMode = GameMode::Match;

		mMatchData.startTime = SystemPlatform::GetLocalTime();

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
		mLastMatchRecordWinCounts[0] = 0;
		mLastMatchRecordWinCounts[1] = 0;

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

		resetGameParam();
		mMatchData.resetGame(setting);

		bool bHavePlayerController = false;
		for (auto& player : mMatchData.players)
		{
			IBot* bot = player.bot.get();
			if (!bot)
			{
				bHavePlayerController = true;
			}
		}
		if (bHavePlayerController)
		{
			createPlayWidget();
		}

		IBot* bot = mMatchData.getCurTurnBot();
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

		if( mModeWidget )
		{
			mModeWidget->destroy();
			mModeWidget = nullptr;
		}
		bPauseGame = false;

		switch( mGameMode )
		{
		case GameMode::Learning:
			mLeelaAIRun.stop();
			break;
		case GameMode::Match:
			{
				recordMatchResult( true );
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
					if( mMatchData.players[0].type == ControllerType::eLeelaZero ||
					    mMatchData.players[1].type == ControllerType::eLeelaZero )
					{
						auto* bot = static_cast< LeelaBot* >( (mMatchData.players[0].type == ControllerType::eLeelaZero ) ? mMatchData.players[0].bot.get() : mMatchData.players[1].bot.get());
						bot->mAI.showResult();
					}
					else if( mMatchData.bAutoRun )
					{
						unknownWinerCount += 1;
						postMatchGameEnd(nullptr);
					}
				}
				else
				{
					mMatchData.notifyTurnAdvance();

					auto CheckRunAnalysis = [this]
					{
						if (bAnalysisEnabled)
						{
							executeAnalysisAICommand([this] { mLeelaAIRun.playPass(); }, canAnalysisPonder(mMatchData.getCurTurnPlayer()));
						}
					};

					IBot* bot = mMatchData.getCurTurnBot();
					if( bot )
					{
						auto execResult = bot->playPass(color);
						checkWaitBotCommand(execResult, [this , bot , CheckRunAnalysis](EBotExecResult inResult)
						{
							if (inResult == BOT_FAIL)
							{
								LogWarning(0, "Game can't sync : Bot Can't pass");
								return;
							}
							if (!bot->thinkNextMove(mGame.getInstance().getNextPlayColor()))
							{


							}
							CheckRunAnalysis();
						});
					}
					else
					{
						CheckRunAnalysis();
					}
				}
			}
			break;
		case GameCommand::eEnd:
			{
				if( mMatchData.bAutoRun )
				{
					mMatchData.getPlayer(com.winner).winCount += 1;
				}
				else
				{
					char const* name = (com.winner == EStoneColor::Black) ? "Black" : "White";
					InlineString< 128 > str;

					str.format("%s Win", name);
					::Global::GUI().showMessageBox(UI_ANY, str, EMessageButton::Ok);
				}

				InlineString<128> matchResult;
				matchResult.format("%s+%g", com.winner == EStoneColor::Black ? "B" : "W", com.winNum);
				postMatchGameEnd(matchResult);

			}
			break;
		case GameCommand::eResign:
			{
				LogMsg("GameEnd");

				if( mMatchData.bAutoRun )
				{
					if( color == EStoneColor::Black )
					{
						mMatchData.getPlayer(EStoneColor::White).winCount += 1;
					}
					else
					{
						mMatchData.getPlayer(EStoneColor::Black).winCount += 1;
					}
				}
				else
				{
					char const* name = (color == EStoneColor::Black) ? "Black" : "White";
					InlineString< 128 > str;

					str.format("%s Resigned", name);
					::Global::GUI().showMessageBox(UI_ANY, str, EMessageButton::Ok);
				}
				InlineString<128> matchResult;
				matchResult.format("%s+R", EStoneColor::Opposite(color) == EStoneColor::Black ? "B" : "W");
				postMatchGameEnd(matchResult);
			}
			break;
		case GameCommand::eUndo:
			{
				mGame.undo();

				if( !mWinRateDataList[indexPlayer].history.empty() )
				{
					mWinRateDataList[indexPlayer].history.pop_back();
				}
				mMatchData.notifyTurnAdvance();

				auto CheckRunAnalysis = [this]
				{
					if (bAnalysisEnabled)
					{
						executeAnalysisAICommand([this] { mLeelaAIRun.undo(); }, canAnalysisPonder(mMatchData.getCurTurnPlayer()));
					}
				};

				IBot* bot = mMatchData.getCurTurnBot();
				if( bot )
				{
					auto execResult = bot->undo();
					checkWaitBotCommand(execResult, [CheckRunAnalysis](EBotExecResult inResult)
					{
						if (inResult == BOT_FAIL)
						{
							LogWarning(0, "Game can't sync : Bot Can't undo");
							return;
						}
						CheckRunAnalysis();
					});
				}
				else
				{
					CheckRunAnalysis();
				}
			}
			break;
		case GameCommand::eBoardState:
			{
				mbBotBoardStateValid = true;
				LogMsg("Show Bot Board State");
			}
			break;
#define LEELA_PARAM_REMAP(NAME) (BotParam::LeelaBase + LeelaGameParam::NAME)
#define ZEN_PARAM_REMAP(NAME) (BotParam::ZenBase + ZenGameParam::NAME)
#define KATA_PARAM_REMAP(NAME) (BotParam::KataBase + KataGameParam::NAME)

		case GameCommand::eParam:
			{
				uint32 paramRemapId = com.paramId + mMatchData.players[indexPlayer].botParamBase;
				switch (paramRemapId)
				{
				case LEELA_PARAM_REMAP(eBestMoveVertex):
					{
						auto* info = static_cast<LeelaThinkInfo*>(com.ptrParam);
						bestMoveVertex = info->v;
						bestThinkInfo = *info;
					}
					break;
				case ZEN_PARAM_REMAP(eBestMoveVertex):
					{
						auto* info = static_cast<Zen::ThinkInfo*>(com.ptrParam);
						bestMoveVertex = info->v;
						bestThinkInfo.v = info->v;
						bestThinkInfo.nodeVisited = 0;
						bestThinkInfo.vSeq = info->vSeq;
						bestThinkInfo.winRate = info->winRate;
						bestThinkInfo.evalValue = 0;
					}
					break;
				case LEELA_PARAM_REMAP(eWinRate):
				case ZEN_PARAM_REMAP(eWinRate):
				case KATA_PARAM_REMAP(eWinRate):
					if (!bPrevGameCom)
					{
						Vector2 v;
						if (paramRemapId == KATA_PARAM_REMAP(eWinRate))
						{
							v.x = (mGame.getInstance().getCurrentStep() + 2) / 2;
						}
						else
						{
							v.x = (mGame.getInstance().getCurrentStep() + 1) / 2;
						}
						v.y = com.floatParam;
						mWinRateDataList[indexPlayer].history.push_back(v);

						if (mWinRateWidget == nullptr)
						{
							Vec2i screenSize = ::Global::GetScreenSize();
							Vec2i widgetSize = { 260 , 310 };
							Vec2i widgetPos = { screenSize.x - (widgetSize.x + 20), screenSize.y - (widgetSize.y + 20) };
							mWinRateWidget = new GFrame(UI_ANY, widgetPos, widgetSize, nullptr);
							mWinRateWidget->setColor(Color3ub(0, 0, 0));
							mWinRateWidget->setRenderCallback(
								RenderCallBack::Create([this](GWidget* widget)
								{
									Vec2i screenSize = ::Global::GetScreenSize();
									Vec2i diagramPos = widget->getWorldPos() + Vec2i(5, 5);
									Vec2i diagramSize = widget->getSize() - 2 * Vec2i(5, 5);
									drawWinRateDiagram(diagramPos, diagramSize);
								})
							);
							::Global::GUI().addWidget(mWinRateWidget);
						}
						//LogMsg("Win rate = %.2f", com.floatParam);
					}
					break;
				case KATA_PARAM_REMAP(eThinkResult):
					{
						TArray< KataThinkInfo >& thinkInfos = *static_cast<TArray< KataThinkInfo >*>(com.ptrParam);

						mAnalysisResult.candidatePosList.clear();
						mAnalysisResult.dataType = ControllerType::eKata;

						if ( !thinkInfos.empty() )
						{
							for (auto& thinkInfo : thinkInfos)
							{
								AnalysisData::PosInfo info;
								info.v = thinkInfo.v;
								info.vSeq = thinkInfo.vSeq;
								info.nodeVisited = thinkInfo.nodeVisited;
								info.winRate = thinkInfo.winRate;

								mAnalysisResult.candidatePosList.push_back(std::move(info));
							}

							auto topThinkInfo = thinkInfos.front();

							bestMoveVertex = topThinkInfo.v;
							bestThinkInfo.v = topThinkInfo.v;
							bestThinkInfo.nodeVisited = topThinkInfo.nodeVisited;
							bestThinkInfo.vSeq = topThinkInfo.vSeq;
							bestThinkInfo.winRate = topThinkInfo.winRate;
							bestThinkInfo.evalValue = 0;
						}
					}
					break;
				}
			}
			break;
		case GameCommand::eAddStone:

			break;
		case GameCommand::ePlayStone:
		default:
			{
				int x = com.pos[0];
				int y = com.pos[1];
				if( mGame.playStone(x, y) )
				{
					resetTurnParam();
					mMatchData.notifyTurnAdvance();

					auto CheckRunAnalysis = [this, x , y, color]
					{
						if (bAnalysisEnabled)
						{
							executeAnalysisAICommand([this, x, y, color] { mLeelaAIRun.playStone(x, y, color); }, canAnalysisPonder(mMatchData.getCurTurnPlayer()));
						}
					};
					IBot* bot = mMatchData.getCurTurnBot();
					if( bot )
					{
						auto execResult = bot->playStone(com.pos[0], com.pos[1], color);
						checkWaitBotCommand(execResult, [=]( EBotExecResult inResult)
						{
							if (inResult == BOT_FAIL)
							{
								LogWarning(0, "Game can't sync : Bot Can't play stone");
								return;
							}
							if (!bot->thinkNextMove(mGame.getInstance().getNextPlayColor()))
							{

							}
							CheckRunAnalysis();
						});		
					}
					else
					{
						CheckRunAnalysis();
					}
				}
				else
				{
					LogMsg("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
				}

			}
			break;
		case GameCommand::eExecResult:
			{
				assert((bool)mWaitBotCommandDelegate);
				mWaitBotCommandDelegate(com.result);
				mWaitBotCommandDelegate = nullptr;
			}
			break;
		}
	}

	void LeelaZeroGoStage::processLearningCommand()
	{
		assert(mGameMode == GameMode::Learning);
		auto ProcFunc = [&](GameCommand& com)
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
					char const* name = (color == EStoneColor::Black) ? "Black" : "White";
					LogMsg("%s Resigned", name);
				}
				break;
			case GameCommand::eEnd:
				{
					LogMsg("Game End");
					++numGameCompleted;
					bMatchJob = false;

					if ( com.winNum == 0 )
						mLastGameResult.format("%s+Resign" , com.winner == EStoneColor::Black ? "B" : "W" );
					else
						mLastGameResult.format("%s+%.1f", com.winner == EStoneColor::Black ? "B" : "W" , com.winNum);
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

							auto IsELFWeight = [&]()
							{
								int index = 0;
								for (; index < ELFWeights.size(); ++index)
								{
									if (FCString::Compare(com.strParam, ELFWeights[index]) == 0)
										return true;
								}
								return false;
							};

							if( !IsELFWeight() )
							{
								::Global::GameConfig().setKeyValue("Leela.LastNetWeight", "Go", com.strParam);
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

					if( mGame.getInstance().getCurrentStep() - 1 >= ::Global::GameConfig().getIntValue( "Leela.LearnMaxSetp" , "Go" , 400 ) )
					{
						mbRestartLearning = true;
						return;
					}
				}
				break;
			}
		};

		mLeelaAIRun.outputThread->procOutputCommand(ProcFunc);
	}


	void LeelaZeroGoStage::resetGameParam()
	{
		bPrevGameCom = true;
		mGame.restart();
		mBoardRenderer.generateNoiseOffset(mGame.getBoard().getSize());
		bTerritoryInfoValid = false;

		resetTurnParam();
		for(auto& data : mWinRateDataList)
		{
			data.history.clear();
			data.history.emplace_back(0.0f, 50.0f);
		}
		if( mWinRateWidget )
		{
			mWinRateWidget->destroy();
			mWinRateWidget = nullptr;
		}
	}

	void LeelaZeroGoStage::resetTurnParam()
	{
		bestMoveVertex = PlayVertex::Undefiend();
	}

	void LeelaZeroGoStage::restartAutoMatch()
	{
		LogMsg("==Restart Auto Match==");
		
		if (mMatchData.bSwapEachMatch)
		{
			mMatchData.bSwapColor = !mMatchData.bSwapColor;
		}

		resetGameParam();
		mMatchData.resetGame(mGame.getSetting());

		IBot* bot = mMatchData.getCurTurnBot();
		if( bot )
		{
			bot->thinkNextMove(mGame.getInstance().getNextPlayColor());
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
			synchronizeAnalysisState();
		}

		return true;
	}

	void LeelaZeroGoStage::synchronizeAnalysisState()
	{

		class AnalysisCopier : public IGameCopier
		{
		public:
			AnalysisCopier(LeelaAppRun& AI) :AI(AI) 
			{
				mLastQueryId = Guid::New();
			}

			Guid mLastQueryId;
			int  mNextCopyStep;

			void copyGame(GameProxy& game)
			{
				if (mLastQueryId != game.guid || mNextCopyStep > game.getInstance().getCurrentStep())
				{
					game.getInstance().synchronizeState(*this, true);
					mLastQueryId = game.guid;
				}
				else if (mNextCopyStep == game.getInstance().getCurrentStep())
				{
					return;
				}
				else
				{
					game.getInstance().synchronizeStateKeep(*this, mNextCopyStep, true);
				}
				mNextCopyStep = game.getInstance().getCurrentStep();
			}

			void emitSetup(GameSetting const& setting) override
			{
				AI.setupGame(setting);
			}
			void emitPlayStone(int x, int y, int color) override
			{
				AI.playStone(x, y, color);
			}
			void emitAddStone(int x, int y, int color) override
			{
				AI.addStone(x, y, color);
			}
			void emitPlayPass(int color) override
			{
				AI.playPass(color);
			}
			void emitUndo() override
			{
				AI.undo();
			}

			LeelaAppRun& AI;
		};

		if (bAnalysisPondering)
		{
			mLeelaAIRun.stopPonder();
		}

		static AnalysisCopier copier(mLeelaAIRun);
		copier.copyGame(getAnalysisGame());
		analysisPonderColor = getAnalysisGame().getInstance().getNextPlayColor();

		if (bAnalysisPondering)
		{
			mLeelaAIRun.startPonder(analysisPonderColor);
		}
	}

	bool LeelaZeroGoStage::toggleAnalysisPonder()
	{
		assert(bAnalysisEnabled);
		bAnalysisPondering = !bAnalysisPondering;

		if( bAnalysisPondering )
		{
			analysisPonderColor = getAnalysisGame().getInstance().getNextPlayColor();
			mLeelaAIRun.startPonder(analysisPonderColor);
		}
		else
		{
			mLeelaAIRun.stopPonder();
		}

		return bAnalysisPondering;
	}

	void LeelaZeroGoStage::recordMatchResult( bool bSaveToFile )
	{
		mMatchResultMap.addMatchResult(mMatchData.players, mGame.getSetting(), mLastMatchRecordWinCounts);
		if ( bSaveToFile )
			mMatchResultMap.save(MATCH_RESULT_PATH);
	}

	bool LeelaZeroGoStage::saveMatchGameSGF(char const* matchResult)
	{
		DateTime& date = mMatchData.startTime;

		InlineString< 512 > path;
		path.format("Go/Save/%04d-%02d-%02d-%02d-%02d-%02d.sgf", date.getYear(), date.getMonth(), date.getDay(), date.getHour(), date.getMinute(), date.getSecond());

		InlineString< 512 > dateString;
		dateString.format("%d-%d-%d", date.getYear(), date.getMonth(), date.getDay());
		GameDescription description;
		description.blackPlayer = mMatchData.getPlayer(EStoneColor::Black).getName();
		description.whitePlayer = mMatchData.getPlayer(EStoneColor::White).getName();
		description.date = dateString.c_str();
		if( matchResult )
			description.mathResult = matchResult;

		return mGame.getInstance().saveSGF(path, &description);
	}

	bool LeelaZeroGoStage::setupRenderResource(ERenderSystem systemName)
	{
		VERIFY_RETURN_FALSE(mBoardRenderer.initializeRHI());
		return true;
	}

	void LeelaZeroGoStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		mBoardRenderer.releaseRHI();
	}

	void LeelaZeroGoStage::postMatchGameEnd(char const* matchResult)
	{
		LogMsg("GameEnd");
		if( mMatchData.bSaveSGF )
		{
			saveMatchGameSGF(matchResult);
		}

		recordMatchResult(true);

		if( mMatchData.bAutoRun )
		{
			restartAutoMatch();
		}
	}

	void LeelaZeroGoStage::createPlayWidget()
	{
		if( mGamePlayWidget == nullptr )
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			Vec2i size = Vec2i(150,200);
			auto devFrame = new DevFrame(UI_ANY, Vec2i(screenSize.x - size.x - 5, 300), size , nullptr);

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
			choice->addItem(FStringConv::From(i));
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
		auto*  filePicker = addWidget< GFilePicker >(id + UPARAM_WEIGHT_NAME, "Weight Name", BIT(idxPlayer), idxPlayer);
		filePicker->filePath.format("%s/%s/%s", LeelaAppRun::InstallDir, LEELA_NET_DIR_NAME ,LeelaAppRun::GetBestWeightName().c_str());
		filePicker->filePath.replace('/', '\\');
	}

	void MatchSettingPanel::addKataPramWidget(int id, int idxPlayer)
	{
		KataAISetting setting;
		GTextCtrl* textCtrl;
		textCtrl = addTextCtrl(id + UPARAM_VISITS, "Max Visits", BIT(idxPlayer), idxPlayer);
		textCtrl->setValue(std::to_string(setting.maxVisits).c_str());

		auto checkBox = addCheckBox(id + UPARAM_USE_CUDA, "UseCUDA", BIT(idxPlayer), idxPlayer);
		checkBox->bChecked = setting.bUseCuda;

		auto*  filePicker = addWidget< GFilePicker >(id + UPARAM_WEIGHT_NAME, "Model Name", BIT(idxPlayer), idxPlayer);
		filePicker->filePath.format("%s/%s/%s", KataAppRun::InstallDir, KATA_MODEL_DIR_NAME, KataAppRun::GetLastModeltName().c_str());
		filePicker->filePath.replace('/', '\\');
	}

	void MatchSettingPanel::addZenParamWidget(int id, int idxPlayer)
	{
		Zen::CoreSetting setting = ZenBot::GetCoreConfigSetting();
		GTextCtrl* textCtrl;
		textCtrl = addTextCtrl(id + UPARAM_SIMULATIONS_NUM, "Num Simulations", BIT(idxPlayer), idxPlayer);
		textCtrl->setValue(std::to_string(setting.numSimulations).c_str());
		textCtrl = addTextCtrl(id + UPARAM_MAX_TIME, "Max Time", BIT(idxPlayer), idxPlayer);
		InlineString<512> valueStr;
		valueStr.format("%g", setting.maxTime);
		textCtrl->setValue(valueStr);
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
			case ControllerType::eKata:
				addKataPramWidget(ui->getID(), idxPlayer);
				break;
			case ControllerType::eZen:
				addZenParamWidget(ui->getID(), idxPlayer);
				break;
			default:
				break;
			}
			adjustChildLayout();
			return false;
		};
		return choice;
	}

	bool MatchSettingPanel::setupMatchSetting(MatchGameData& matchData, GameSetting& outSetting)
	{
		ControllerType types[2] =
		{
			(ControllerType)(intptr_t)findChildT<GChoice>(UI_CONTROLLER_TYPE_A)->getSelectedItemData(),
			(ControllerType)(intptr_t)findChildT<GChoice>(UI_CONTROLLER_TYPE_B)->getSelectedItemData()
		};

		bool bHavePlayerController = types[0] == ControllerType::ePlayer || types[1] == ControllerType::ePlayer;

		matchData.bAutoRun = getParamValue< bool, GCheckBox >(UI_AUTO_RUN);
		matchData.bSaveSGF = getParamValue< bool, GCheckBox >(UI_SAVE_SGF);


		outSetting.numHandicap = findChildT<GChoice>(UI_FIXED_HANDICAP)->getSelection();
		if (outSetting.numHandicap)
		{
			matchData.bSwapEachMatch = false;
			if (outSetting.numHandicap == 1)
			{
				outSetting.bBlackFirst = true;
				outSetting.numHandicap = 0;
			}
			else
			{
				outSetting.bBlackFirst = false;
			}
			outSetting.komi = 0.5;
		}
		else
		{
			matchData.bSwapEachMatch = true;
			outSetting.bBlackFirst = true;
			outSetting.komi = 7.5;
		}

		for( int i = 0; i < 2; ++i )
		{
			int id = (i) ? UI_CONTROLLER_TYPE_B : UI_CONTROLLER_TYPE_A;
			MatchPlayer* otherPlayer = (i) ? &matchData.players[0] : nullptr;
			switch( types[i] )
			{
			case ControllerType::eLeelaZero:
				{
					TBotSettingData<LeelaAISetting> setting = LeelaAISetting::GetDefalut();
					std::string weightName = findChildT<GFilePicker>(id + UPARAM_WEIGHT_NAME)->filePath.c_str();
					setting.weightName = FFileUtility::GetFileName(weightName.c_str());
					setting.visits = getParamValue< int, GTextCtrl >(id + UPARAM_VISITS);
					//setting.bUseModifyVersion = true;
					setting.seed = GenerateRandSeed();
					if( bHavePlayerController || outSetting.numHandicap)
					{
						setting.resignpct = 0;
					}
					if (outSetting.numHandicap)
					{
						if (i == matchData.bSwapColor ? 1 : 0)
						{
							setting.resignpct = 20;
						}
					}
					else
					{
						setting.randomcnt = 5;
					}
					if( !matchData.players[i].initialize(types[i], &setting, otherPlayer) )
						return false;
				}
				break;
			case ControllerType::eKata:
				{
					TBotSettingData<KataAISetting> setting;
					//setting.rootNoiseEnabled = true;
					setting.maxVisits = getParamValue< int , GTextCtrl >(id + UPARAM_VISITS);
					//setting.bUseDefaultConfig = true;
					setting.bUseCuda = getParamValue< bool, GCheckBox >(id + UPARAM_USE_CUDA);
					std::string modelName = findChildT<GFilePicker>(id + UPARAM_WEIGHT_NAME)->filePath.c_str();
					setting.modelName = FFileUtility::GetFileName( modelName.c_str() );

					if( !matchData.players[i].initialize(types[i], &setting, otherPlayer) )
						return false;
				}
				break;
			case ControllerType::eZen:
				{
					TBotSettingData<Zen::CoreSetting> setting = ZenBot::GetCoreConfigSetting();
					//#TODO
					setting.version = 7;
					setting.numSimulations = getParamValue< float , GTextCtrl >(id + UPARAM_SIMULATIONS_NUM);
					setting.maxTime = getParamValue< float, GTextCtrl >(id + UPARAM_MAX_TIME);
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

		return true;
	}


	void BoardFrame::onRender()
	{
		BaseClass::onRender();
		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		{
			TGuardValue<bool> gurdValue(renderer->bDrawCoord, false);
			SimpleRenderState renderState;
			renderer->draw(g, renderState, renderContext);
		}
	}

}//namespace Go