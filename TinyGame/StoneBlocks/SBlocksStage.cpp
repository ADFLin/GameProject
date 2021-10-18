#include "SBlocksStage.h"

#include "SBlocksSerialize.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Serialize/FileStream.h"
#include "FileSystem.h"

namespace SBlocks
{
	REGISTER_STAGE_ENTRY("Stone Blocks", TestStage, EExecGroup::Dev4, "Game");

	TConsoleVariable< bool > CVarShowDebug(false, "SBlocks.ShowDebug", CVF_TOGGLEABLE);

	constexpr uint8 MakeByte(uint8 b0) { return b0; }
	template< typename ...Args >
	constexpr uint8 MakeByte(uint8 b0, Args ...args) { return b0 | ( MakeByte(args...) << 1 ); }

#define M(...) MakeByte( __VA_ARGS__ )

	LevelDesc TestLv =
	{
		//map
		{ 
			5 , 
			{ 
				M(0,0,0,0,0),
				M(0,0,0,0,0),
				M(0,0,0,1,0),
				M(0,0,0,0,0),
				M(0,0,0,0,0),
			} 
		} ,

		//shape
		{
			{
				3,
				{
					M(1,1,1),
					M(0,1,0),
				},
				true, {1.5 , 0.5},
			},
			{
				3,
				{
					M(0,1,1),
					M(1,1,0),
				},
				true, {1.5 , 1.0},
			},
			{
				3,
				{
					M(1,0,0),
					M(1,1,1),
				},
				true, {1.5 , 1},
			},
			{
				2,
				{
					M(1,1),
				},
				true, {1 , 0.5},
			},
			{
				3,
				{
					M(1,1,1),
				},
				true, {1.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,0),
				},
				true, {0.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,1),
				},
				true, {1 , 1},
			},
		},

		//piece
		{
			{0},
			{1},
			{2},
			{3},
			{4},
			{5},
			{6},
		}
	};

	LevelDesc TestLv2 =
	{
		//map
		{
			6 ,
			{
				M(0,0,0,0,0,0),
				M(0,0,0,0,0,0),
				M(0,0,1,0,0,0),
				M(0,0,0,0,0,0),
				M(0,0,0,0,0,1),
			}
		} ,

		//shape
		{
			{
				3,
				{
					M(1,1,1),
					M(0,1,0),
				},
				true, {1.5 , 0.5},
			},
			{
				3,
				{
					M(0,1,1),
					M(1,1,0),
				},
				true, {1.5 , 1.0},
			},
			{
				3,
				{
					M(1,0,0),
					M(1,1,1),
				},
				true, {1.5 , 1},
			},
			{
				2,
				{
					M(1,1),
				},
				true, {1 , 0.5},
			},
			{
				3,
				{
					M(1,1,1),
				},
				true, {1.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,0),
				},
				true, {0.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,1),
				},
				true, {1 , 1},
			},

			{
				5,
				{
					M(1,1,1,1),
				},
				true, {2.5 , 0.5},
			},
		},

		//piece
		{
			{7},
			{0},
			{1},
			{2},
			{3},
			{4},
			{5},
			{6},

		}
	};

	LevelDesc TestLv3 =
	{
		//map
		{
			8 ,
			{
				M(0,0,0,0,0,0,0,1),
				M(0,0,0,0,0,0,0,1),
				M(1,0,0,0,0,0,1,1),
				M(1,0,1,0,0,0,1,1),
				M(1,0,0,0,0,0,0,0),
				M(0,0,0,0,0,0,0,0),
				M(0,0,0,0,0,0,0,0),
				M(0,0,0,0,0,0,0,0),
			}
		} ,

		//shape
		{
			{
				5,
				{
					M(1,1,0,0,0),
					M(0,1,1,1,1),
				},
				true, {2.5 , 1.0},
			},
			{
				4,
				{
					M(0,0,1,1),
					M(1,1,1,1),
				},
				true, {2.0 , 1.0},
			},
			{
				3,
				{
					M(0,1,1),
					M(0,1,1),
					M(1,1,1),
				},
				true, {1.5 , 1.5},
			},
			{
				3,
				{
					M(1,1,0),
					M(1,1,1),
					M(1,1,1),
				},
				true, {1.5 , 1.5},
			},
			{
				2,
				{
					M(0,1),
					M(0,1),
					M(1,1),
				},
				true, {1 , 1.5},
			},
			{
				4,
				{
					M(1,1,0,0),
					M(1,1,0,0),
					M(1,1,1,0),
					M(1,1,1,1),
					M(1,1,1,1),
				},
				true, {2 , 2.5},
			},
			{
				4,
				{
					M(0,0,1,1),
					M(0,0,1,1),
					M(1,1,1,1),
				},
				true, {2.5 , 1.5},
			},
		},

		//piece
		{
			{0},
			{1},
			{2},
			{3},
			{4},
			{5},
			{6},
			{7},
		}
	};

#undef M


	void TestStage::restart()
	{
		mLevel.importDesc(TestLv3);
		initializeGame();
	}

	void TestStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		using namespace Render;

		Vec2i screenSize = ::Global::GetScreenSize();
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);

		g.beginRender();

		RenderUtility::SetPen(g, EColor::Null);
		g.setBrush(mTheme.backgroundColor);	
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		g.transformXForm(mLocalToWorld, true);

		float const MapBorder = 0.3;
		float const MapFrameWidth = 0.3;

		float renderBorder = MapBorder + 0.5 * MapFrameWidth;

		Vec2i mapSize = mLevel.mMap.getBoundSize();
		RenderUtility::SetPen(g, EColor::Null);
		g.setBrush(mTheme.mapOuterColor);
		g.drawRect(-Vector2(renderBorder, renderBorder), Vector2(mapSize) + 2.0 * Vector2(renderBorder, renderBorder));

		float MapOffset = MapFrameWidth + MapBorder;
		RenderUtility::SetPen(g, EColor::Null);

		auto DrawMapFrame = [&]()
		{	
			g.drawRect(-Vector2(MapOffset, MapOffset),
				Vector2(float(mapSize.x) + 2 * MapOffset, MapFrameWidth));
			g.drawRect(Vector2(0, float(mapSize.y) + 2 * MapOffset - MapFrameWidth) - Vector2(MapOffset, MapOffset),
				Vector2(float(mapSize.x) + 2 * MapOffset, MapFrameWidth));

			g.drawRect(Vector2(0, MapFrameWidth) -Vector2(MapOffset, MapOffset),
				Vector2(MapFrameWidth, float(mapSize.y) + 2 * MapBorder));
			g.drawRect(Vector2(float(mapSize.x) + 2 * MapOffset - MapFrameWidth, MapFrameWidth) - Vector2(MapOffset, MapOffset),
				Vector2(MapFrameWidth, float(mapSize.y) + 2 * MapBorder));
		};

		g.setBrush(mTheme.shadowColor);
		g.beginBlend(mTheme.shadowOpacity);
		g.pushXForm();
		g.translateXForm(0.5 * mTheme.shadowOffset.x, 0.5 * mTheme.shadowOffset.y);
		DrawMapFrame();
		g.popXForm();
		g.endBlend();

		g.setBrush(mTheme.mapFrameColor);
		DrawMapFrame();


		RenderUtility::SetPen(g, EColor::Black);
		for (int j = 0; j < mapSize.y; ++j)
		{
			for (int i = 0; i < mapSize.x; ++i)
			{

				uint8 value = mLevel.mMap.getValue(i, j);

				if (bEditEnabled)
				{
					RenderUtility::SetPen(g, EColor::Gray);
					RenderUtility::SetBrush(g, EColor::Gray);
					float const border = 0.1;
					g.drawRect(Vector2(i + border, j + border) , Vector2(1 - 2 * border, 1 - 2 * border) );
				}

				RenderUtility::SetPen(g, EColor::Null);
				switch (value)
				{
				case MarkMap::MAP_BLOCK:
					g.setBrush(mTheme.mapBlockColor);
					break;
				case MarkMap::PIECE_BLOCK:
				case 0:
					g.setBrush(mTheme.mapEmptyColor);
					break;
				}

				float border = 0.025;
				g.drawRect(Vector2(i + border, j + border ), Vector2(1 - 2 * border, 1 - 2 * border) );
			}
		}
		
		vaildatePiecesOrder();
		for (Piece* piece : mSortedPieces )
		{
			drawPiece(g, *piece, selectedPiece == piece);
			if (CVarShowDebug)
			{
				g.pushXForm();
				g.identityXForm();

				Vector2 pos = mLocalToWorld.transformPosition(piece->getLTCornerPos());
				g.drawText(pos, InlineString<>::Make( "%d", piece->index ) );
				g.popXForm();
			}
		}

		if ( CVarShowDebug )
		{
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(DebugPos, Vector2(0.1, 0.1));
		}

		g.endRender();
	}


	void TestStage::drawPiece(RHIGraphics2D& g, Piece const& piece, bool bSelected)
	{
		auto const& shapeData = piece.shape->mDataMap[0];


		RenderUtility::SetPen(g, EColor::Null);

		
		g.pushXForm();
		g.transformXForm(piece.xFormRender, true);
		auto DrawPiece = [&]()
		{
			for (auto const& block : shapeData.blocks)
			{
				g.drawRect(block, Vector2(1, 1));
			}
		};

		if ( piece.bLocked == false )
		{
			g.setBrush(mTheme.shadowColor);
			g.beginBlend(mTheme.shadowOpacity);

			g.pushXForm();
			Vector2 offset = piece.xFormRender.transformInvVectorAssumeNoScale(mTheme.shadowOffset);
			g.translateXForm(offset.x, offset.y);
			DrawPiece();
			g.popXForm();
			g.endBlend();
		}
		
		g.setBrush(piece.bLocked ? mTheme.pieceBlockLockedColor : mTheme.pieceBlockColor);
		DrawPiece();

		
		RenderUtility::SetPen(g, piece.bLocked ? EColor::Red : bSelected ? EColor::Yellow : EColor::Gray );
		g.setPenWidth(3);
		for (auto const& line : piece.shape->outlines)
		{
			g.drawLine(line.start, line.end);
		}
		g.setPenWidth(1);
		g.popXForm();
	}

	ERenderSystem TestStage::getDefaultRenderSystem()
	{
		return ERenderSystem::OpenGL;
	}

	bool TestStage::setupRenderSystem(ERenderSystem systemName)
	{
		return true;
	}

	void TestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{

	}

	void TestStage::solveLevel(bool bForceReset)
	{
		TIME_SCOPE("Solve Level");
		bool bSuccess;
		if (mSolver == nullptr || bForceReset)
		{
			mSolver = std::make_unique< Solver >();
			mSolver->setup(mLevel);
			bSuccess = mSolver->solve();
		}
		else
		{
			bSuccess = mSolver->solveNext();
		}

		LogMsg("Solve level %s !", bSuccess ? "success" : "fail");
		if (bSuccess)
		{
			std::vector<PieceSolveState> sovledStates;
			mSolver->getSolvedStates(sovledStates);

			mLevel.unlockAllPiece();
			for (int i = 0; i < mLevel.mPieces.size(); ++i)
			{
				Piece* piece = mLevel.mPieces[i].get();
				auto const& state = sovledStates[i];
				LogMsg("%d = (%d, %d) dir = %d", i , state.pos.x , state.pos.y , state.dir );
				piece->dir = DirType::ValueChecked(state.dir );
				piece->angle = piece->dir * Math::PI / 2;
				piece->updateTransform();
				Vector2 pos = piece->getLTCornerPos();
				piece->pos += Vector2(state.pos) - pos;
				piece->updateTransform();

				mLevel.tryLockPiece(*piece);
			}
		}
		else
		{
			mSolver.release();
		}
	}

	void TestStage::loadLevel(char const* name)
	{
		InputFileSerializer serializer;
		if (serializer.open(InlineString<>::Make("SBlock/levels/%s.lv", name)))
		{
			LevelDesc desc;
			serializer.read(desc);

			mLevel.importDesc(desc);
			initializeGame();
		}
	}

	void Editor::saveLevel(char const* name)
	{
		OutputFileSerializer serializer;
		serializer.registerVersion(EName::None, ELevelSaveVersion::LastVersion);

		if (!FFileSystem::IsExist("SBlock/levels"))
		{
			FFileSystem::CreateDirectorySequence("SBlock/levels");
		}

		if (serializer.open(InlineString<>::Make("SBlock/levels/%s.lv", name)))
		{
			LevelDesc desc;
			mGame->mLevel.exportDesc(desc);
			serializer.write(desc);
			serializer.writeVersionData();
		}
	}

}