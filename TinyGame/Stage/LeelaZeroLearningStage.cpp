#include "LeelaZeroLearningStage.h"


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


REGISTER_STAGE("LeelaZero Learning", LeelaZeroLearningStage, EStageGroup::Test);

class GameLearningThread : public IGameProcessThread 
{
public:

	int curStep = 0;

	unsigned run()
	{
		for( ;;)
		{
			char buffer[1024 + 1];
			int numRead;
			bool bSuccess = process->readOutputStream(buffer, ARRAY_SIZE(buffer) - 1, numRead);
			if( !bSuccess || numRead == 0 )
				break;
			buffer[numRead] = 0;
			parseOutput(buffer, numRead);
		}
		return 0;
	}

	void parseOutput(char* buffer, int num)
	{
		char const* cur = buffer;
		while( *cur != 0  )
		{
			int step;
			char coord[32];
			int numRead;
			cur = ParseUtility::SkipSpace(cur);
			if( sscanf(cur, "%d%s%n", &step, coord, &numRead) == 2 && coord[0] == '(')
			{
				GameProcessCommad com;
				if( step == 1 )
				{
					com.id = GameComId::Restart;
					addCommand(com);
					curStep = 1;
				}

				if( curStep != step )
				{
					::Msg("Warning:Error Step");
				}
				if( strcmp("(pass)", coord) == 0 )
				{
					com.id = GameComId::Pass;
					addCommand(com);
				}
				else if( strcmp("(resign)", coord) == 0 )
				{
					com.id = GameComId::Resign;
					addCommand(com);
				}
				else if ( ('A' <= coord[1] && coord[1] <= 'Z' ) && 
						  ('0' <= coord[2] && coord[2] <= '9' ) )
				{
					int pos[2];
					Go::ReadCoord(coord + 1, pos);
					com.pos[0] = pos[0];
					com.pos[1] = pos[1];
					addCommand(com);
				}
				else
				{
					::Msg("Unknown Com = %s", coord);
				}
				++curStep;
				cur += numRead;
			}
			else
			{
				char const* next = ParseUtility::SkipToNextLine(cur);
				int num = next - cur;
				if( num )
				{
					FixString< 512 > str{ cur , num };
					::Msg("%s", str.c_str());
					cur = next;
				}
			}
		}
	}

};


class GamePlayingThread : public IGameProcessThread
{
public:

	char mBuffer[1024 + 1];
	int  mNumUsed = 0;
	unsigned run()
	{
		for( ;;)
		{
			int numRead;
			bool bSuccess = process->readOutputStream(mBuffer + mNumUsed, ARRAY_SIZE(mBuffer) - ( 1 + mNumUsed ), numRead);
			if( !bSuccess || numRead == 0 )
				break;

			char* pData = mBuffer;
			char* pLineEnd = pData + mNumUsed;
			mNumUsed += numRead;
			char* pDataEnd = mBuffer + mNumUsed;
			for(;; )
			{
				for( ; pLineEnd != pDataEnd; ++pLineEnd )
				{
					if ( *pLineEnd == '\r' || *pLineEnd == '\n' )
						break;
				}
				if( pLineEnd == pDataEnd )
					break;

				*pLineEnd = 0;
				if ( pData != pLineEnd  )
				{
					::Msg(pData);
					parseLine(pData, pLineEnd - pData);
				}

				++pLineEnd;
				pData = pLineEnd;
			}

			if( pData != pDataEnd )
			{
				mNumUsed = pDataEnd - pData;
				for( int i = 0; i < mNumUsed; ++i )
					mBuffer[i] = *(pData++);
			}
			else
			{
				mNumUsed = 0;
			}

		}
		return 0;
	}
	void parseLine(char* buffer, int num)
	{
		char const* cur = buffer;
		while( *cur != 0 )
		{
			char coord[32];
			int numRead;
			cur = ParseUtility::SkipSpace(cur);
			if( sscanf(cur, "= %s%n", &coord, &numRead) == 1 )
			{
				GameProcessCommad com;
				if( strcmp(coord, "resign") == 0 )
				{
					com.id = GameComId::Resign;
					addCommand(com);
				}
				else if( strcmp(coord, "pass") == 0 )
				{
					com.id = GameComId::Pass;
					addCommand(com);
				}
				else
				{
					com.pos[0] = coord[0] - 'A';
					if( coord[0] > 'I' )
						com.pos[0] -= 1;
					com.pos[1] = atoi(coord + 1) - 1;

					if( 0 <= com.pos[0] && com.pos[0] < 19 &&
					    0 <= com.pos[1] && com.pos[1] < 19 &&
					   '0' <= coord[1] && coord[1] <= '9' )
					{
						addCommand(com);
					}
					else
					{
						::Msg(cur);
					}
				}

				cur += numRead;
			}
			else
			{
				FixString< 512 > str{ cur , num };
				//::Msg("%s", str.c_str());
				break;
			}
		}
	}
};

bool LeelaZeroLearningStage::onInit()
{
	if( !BaseClass::onInit() )
		return false;

	if( !::Global::getDrawEngine()->startOpenGL(true) )
		return false;

	if( !mGameRenderer.initializeRHI() )
		return false;

	mLeelaZeroDir = Global::GameConfig().getStringValue("InstallDir", "LeelaZero", "E:/Desktop/LeelaZero");

	::Global::GUI().cleanupWidget();

	mGame.setup(19);
	mGameRenderer.generateNoiseOffset(mGame.getBoard().getSize());

#if 1
	if( !buildLearningProcess() )
		return false;
#else
	if( !buildPlayProcess() )
		return false;
#endif

	auto devFrame = WidgetUtility::CreateDevFrame();
	devFrame->addButton(UI_TOGGLE_PAUSE_GAME, "Pause Process");
	devFrame->addButton(UI_PLAY_GAME, "Play Game");
	devFrame->addButton(UI_RUN_LEARNING, "Run Learning");
	return true;
}

void LeelaZeroLearningStage::onEnd()
{
	::Global::getDrawEngine()->stopOpenGL(true);
	killGameProcess();
	BaseClass::onEnd();
}

void LeelaZeroLearningStage::onUpdate(long time)
{
	BaseClass::onUpdate(time);

	processGameCommand();

	if( !bPlayMode )
	{
		keepLeelaProcessRunning(time);
	}

	int frame = time / gDefaultTickTime;
	for( int i = 0; i < frame; ++i )
		tick();

	updateFrame(frame);
}

void LeelaZeroLearningStage::onRender(float dFrame)
{
	using namespace RenderGL;

	using namespace Go;

	GLGraphics2D& g = ::Global::getGLGraphics2D();
	g.beginRender();

	GpuProfiler::getInstance().beginFrame();

	glClear(GL_COLOR_BUFFER_BIT);

	mGameRenderer.draw(BoardPos, mGame);

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

bool LeelaZeroLearningStage::onWidgetEvent(int event, int id, GWidget* ui)
{
	switch( id )
	{
	case UI_TOGGLE_PAUSE_GAME:
		if( bProcessPaused )
		{
			if( mGameRunProcess.resume() )
			{
				GUI::CastFast<GButton>(ui)->setTitle("Pause Process");
				bProcessPaused = false;
			}
		}
		else
		{
			if( mGameRunProcess.suspend() )
			{
				GUI::CastFast<GButton>(ui)->setTitle("Resume Process");
				bProcessPaused = true;
			}
		}
		return false;
	case UI_PLAY_GAME:

		if( !bPlayMode || isLeelaTurn() )
		{
			killGameProcess();
			buildPlayProcess();
			bPlayMode = true;
		}
		else
		{
			inputProcessStream("clear_board\n");
		}
		startPlayGame();
		return false;
	case UI_RUN_LEARNING:
		if( bPlayMode )
		{
			killGameProcess();
			buildLearningProcess();
			mGame.restart();
			bPlayMode = false;
		}
		return false;
	default:
		break;
	}

	return BaseClass::onWidgetEvent(event, id, ui);
}

bool LeelaZeroLearningStage::onMouse(MouseMsg const& msg)
{
	if( !BaseClass::onMouse(msg) )
		return false;

	if( msg.onLeftDown() )
	{
		if( bPlayMode && !isLeelaTurn() )
		{
			Vec2i pos = (msg.getPos() - BoardPos + Vec2i(mGameRenderer.CellSize, mGameRenderer.CellSize) / 2) / mGameRenderer.CellSize;

			int color = mGame.getNextPlayColor();
			if( mGame.play(pos.x, pos.y) )
			{
				playStone(color, pos);
			}
		}
	}
	return true;
}

bool LeelaZeroLearningStage::onKey(unsigned key, bool isDown)
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



bool LeelaZeroLearningStage::buildLearningProcess()
{
	FixString<256> path;
	path.format("%s/%s", mLeelaZeroDir, "/autogtp.exe");

	if( !mGameRunProcess.createWithIO(path) )
		return false;

	auto myThread = new GameLearningThread;
	myThread->process = &mGameRunProcess;
	myThread->start();
	myThread->setDisplayName("Output Thread");
	mProcessThread = myThread;

	return true;
}

bool LeelaZeroLearningStage::buildPlayProcess()
{
	FixString<256> path;
	path.format("%s/%s", mLeelaZeroDir, "/leelaz.exe");
	FixString<512> command;

	std::string bestWeigetName = getBestWeightName();
	char const* weightName = nullptr;
	if( bestWeigetName != "" )
		weightName = bestWeigetName.c_str();
	else
		weightName = Global::GameConfig().getStringValue("WeightData", "LeelaZero", "223737476718d58a4a5b0f317a1eeeb4b38f0c06af5ab65cb9d76d68d9abadb6");

	::Msg("Play weight = %s", weightName);
	command.format(" -r 1 -g -t 1 -q -d -n -m 30 -w %s -p 1600 --noponder", weightName);
	if( !mGameRunProcess.createWithIO(path, command) )
		return false;

	auto myThread = new GamePlayingThread;
	myThread->process = &mGameRunProcess;
	myThread->start();
	myThread->setDisplayName("Output Thread");
	mProcessThread = myThread;

	bPlayMode = true;
	return true;
}

void LeelaZeroLearningStage::killGameProcess()
{
	bProcessPaused = false;

	if( mProcessThread )
	{
		mProcessThread->kill();
		delete mProcessThread;
		mProcessThread = nullptr;
	}

	mGameRunProcess.terminate();
}

void LeelaZeroLearningStage::keepLeelaProcessRunning(long time)
{
#if DETECT_LEELA_PROCESS
	if( mPIDLeela == -1 )
	{
		mPIDLeela = FPlatformProcess::FindPIDByNameAndParentPID(TEXT("leelaz.exe"), GetProcessId(mGameRunProcess.getHandle()));
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
		killGameProcess();
		buildLearningProcess();
	}
#endif
}

void LeelaZeroLearningStage::processGameCommand()
{
	if( !mProcessThread )
		return;

	mProcessThread->procCommand([&](GameProcessCommad& com)
	{
		switch( com.id )
		{
		case GameComId::Restart:
			{
				mGame.restart();
				mGameRenderer.generateNoiseOffset(mGame.getBoard().getSize());
				if( !bPlayMode )
				{
					++NumLearnedGame;
#if DETECT_LEELA_PROCESS
					mPIDLeela = -1;
					mRestartTimer = RestartTime;
#endif
				}
			}
			break;
		case GameComId::Pass:
			{
				mGame.pass();
				::Msg("Pass");
			}
			break;
		case GameComId::Resign:
			{
				char const* name = (mGame.getNextPlayColor() == Board::eBlack) ? "Black" : "White";
				::Msg("%s Resigned", name);
			}
			break;
		default:
			if( mGame.play(com.pos[0], com.pos[1]) )
			{
				if( bPlayMode )
				{
					checkLeelaPlay();
				}
			}
			else
			{
				::Msg("Warning:Can't Play step : [%d,%d]", com.pos[0], com.pos[1]);
			}
			break;
		}
	});
}

std::string LeelaZeroLearningStage::getBestWeightName()
{
	FileIterator fileIter;
	if( !FileSystem::FindFile(mLeelaZeroDir, nullptr, fileIter) )
	{
		return "";
	}

	bool haveBest = false;
	DateTime bestDate;
	std::string result;
	for( ; fileIter.haveMore(); fileIter.goNext() )
	{
		if( FileUtility::GetSubName(fileIter.getFileName()) != nullptr )
			continue;

		DateTime date = fileIter.getLastModifyDate();

		if( !haveBest || date > bestDate )
		{
			result = fileIter.getFileName();
			bestDate = date;
			haveBest = true;
		}
	}
	return result;
}

