#include "stage/TestStageHeader.h"

#include "Math/TVector2.h"
#include "DataStructure/Grid2D.h"
#include "RHI/RHICommand.h"

#include <algorithm>
#include "GLGraphics2D.h"

namespace Chess
{
	using namespace Render;
	namespace EChess
	{
		enum Type
		{
			King ,
			Queen ,
			Bishop,
			Knight,
			Rook ,
			Pawn,
		};
	}

	enum EColor
	{
		Black = -1,
		Empty = 0,
		White = 1,
	};
	int BOARD_SIZE = 8;

	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
	Vec2i Dirs[4] = { Vec2i(1,0) ,Vec2i(0,1),Vec2i(-1,0),Vec2i(0,-1) };
	Vec2i DiagonalDirs[4] = { Vec2i(1,1) ,Vec2i(-1,1),Vec2i(1,-1),Vec2i(-1,-1) };
	Vec2i KnightMoves[8] = { Vec2i(2,1) ,Vec2i(1,2),Vec2i(-1,2),Vec2i(-2,1),Vec2i(2,-1) ,Vec2i(1,-2),Vec2i(-1,-2),Vec2i(-2,-1) };
	Vec2i PawnBlackMoves[3] = { Vec2i(0,-1) , Vec2i(-1,-1) , Vec2i(1,-1) };
	Vec2i PawnWhiteMoves[3] = { Vec2i(0,1) , Vec2i(-1,1) , Vec2i(1,1) };


#define B EColor::Black
#define W EColor::White
#define C( COLOR , TYPE ) ( COLOR * EChess::TYPE )
#define C_EMPTY 0
	int StandInitState[]
	{
		C(W , Rook),C(W , Knight),C(W , Bishop),C(W , King),C(W , Queen),C(W , Bishop),C(W , Knight),C(W , Rook),
		C(W , Pawn),C(W , Pawn)  ,C(W , Pawn)  ,C(W , Pawn),C(W , Pawn) ,C(W , Pawn)  ,C(W , Pawn)  ,C(W , Pawn),
		C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,
		C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,
		C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,
		C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,C_EMPTY,
		C(W , Pawn),C(W , Pawn)  ,C(W , Pawn)  ,C(W , Pawn),C(W , Pawn) ,C(W , Pawn)  ,C(W , Pawn)  ,C(W , Pawn),
		C(W , Rook),C(W , Knight),C(W , Bishop),C(W , Queen),C(W ,King),C(W , Bishop),C(W , Knight),C(W , Rook),
		
	};
#undef C
#undef B
#undef W
#undef C_EMPTY

	class Game
	{
	public:

		void restart()
		{
			mBoard.resize(BOARD_SIZE, BOARD_SIZE);

			for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; ++i)
			{
				TileData& data = mBoard[i];
				if (StandInitState[i])
				{
					data.color = StandInitState[i] > 0 ? EColor::White : EColor::Black;
					data.type = (EChess::Type)std::abs(StandInitState[i]);
				}
				else
				{
					data.color = EColor::Empty;
				}

			}
		}


		void getPossibleMovePos(Vec2i const& pos, EChess::Type type, int color , std::vector<Vec2i>& outPosList)
		{
			auto CheckPos = [&](Vec2i const& posCheck) -> bool
			{
				if (!mBoard.checkRange(posCheck.x, posCheck.y))
					return false;
				TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
				if (data.color && data.color == color)
					return false;

				outPosList.push_back(pos);
				return true;
			};

			auto CheckCapturePos = [&](Vec2i const& posCheck) -> bool
			{
				if (!mBoard.checkRange(posCheck.x, posCheck.y))
					return false;
				TileData const& data = mBoard.getData(posCheck.x, posCheck.y);
				if (data.color == 0 || data.color == color)
					return false;

				outPosList.push_back(pos);
				return true;
			};

			auto checkDirection = [&](Vec2i const& startPos, Vec2i const& dir)
			{
				Vec2i posCheck = startPos;
				for (;;)
				{
					posCheck += dir;
					if ( !CheckPos(posCheck) )
						break;
				}
			};
			switch (type)
			{
			case EChess::King:
				for (auto dir : Dirs)
				{
					CheckPos(pos + dir);
				}
				for (auto dir : DiagonalDirs)
				{
					CheckPos(pos + dir);
				}
				break;
			case EChess::Queen:
				for (auto dir : Dirs)
				{
					checkDirection(pos, dir);
				}
				for (auto dir : DiagonalDirs)
				{
					checkDirection(pos, dir);
				}
				break;
			case EChess::Bishop:
				for (auto dir : DiagonalDirs)
				{
					checkDirection(pos, dir);
				}
				break;
			case EChess::Knight:
				for (auto dir : KnightMoves)
				{
					CheckPos(pos + dir);
				}
				break;
			case EChess::Rook:
				for (auto dir : Dirs)
				{
					checkDirection(pos, dir);
				}
				break;
			case EChess::Pawn:
				{
					auto const& PawnMoves = (color == 1) ? PawnWhiteMoves : PawnBlackMoves;

					if (CheckPos(pos + PawnMoves[0]))
					{
						if (pos.x == (color == 1) ? 1 : BOARD_SIZE - 2)
						{
							CheckPos(pos + 2 * PawnMoves[0]);
						}
					}

					CheckCapturePos(pos + PawnMoves[1]);
					CheckCapturePos(pos + PawnMoves[2]);
				}
				break;
			}
		}
		struct TileData
		{
			int color;
			EChess::Type type;
			int whiteCount;
			int blackCount;
		};

		TGrid2D<TileData> mBoard;

	};


	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		Game mGame;

		RHITexture2DRef mChessTex;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			if (!::Global::GetDrawEngine().initializeRHI(RHITargetName::OpenGL))
				return false;

			VERIFY_RETURN_FALSE( mChessTex = RHIUtility::LoadTexture2DFromFile("texture/chess.png") );

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();
			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			mGame.restart();
		}
		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame) override
		{
			GLGraphics2D& g = Global::GetRHIGraphics2D();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);

			g.beginRender();
			g.drawTexture(*mChessTex, Vector2(100, 100) , Vector2(100,100));
			g.endRender();
		}

		bool onMouse(MouseMsg const& msg) override
		{
			if (!BaseClass::onMouse(msg))
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if (!msg.isDown())
				return false;

			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};
	

	REGISTER_STAGE("Chess", TestStage, EStageGroup::Dev4);


}//namespace Chess
