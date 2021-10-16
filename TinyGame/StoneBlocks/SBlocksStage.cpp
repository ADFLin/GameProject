#include "SBlocksStage.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Serialize/FileStream.h"
#include "FileSystem.h"

namespace SBlocks
{
	REGISTER_STAGE_ENTRY("Stone Blocks", TestStage, EExecGroup::Dev4, "Game");

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
				M(0,0,1,0,0),
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
				{1.5 , 0.5},
			},
			{
				3,
				{
					M(0,1,1),
					M(1,1,0),
				},
				{1.5 , 1.0},
			},
			{
				3,
				{
					M(1,0,0),
					M(1,1,1),
				},
				{1.5 , 1},
			},
			{
				2,
				{
					M(1,1),
				},
				{1 , 0.5},
			},
			{
				3,
				{
					M(1,1,1),
				},
				{1.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,0),
				},
				{0.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,1),
				},
				{1 , 1},
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

#undef M


	void TestStage::restart()
	{
		mLevel.importDesc(TestLv);
		GameData::init();
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
		
		g.transformXForm(mLocalToWorld, true);

		Vec2i mapSize = mLevel.mMap.getBoundSize();

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

				RenderUtility::SetPen(g, EColor::Black);
				switch (value)
				{
				case MarkMap::MAP_BLOCK:
					continue;

				case MarkMap::PIECE_BLOCK:
					RenderUtility::SetBrush(g, EColor::Gray);
					break;
				case 0:
					g.setBrush(mTheme.mapBlockColor);
					break;
				}

				g.drawRect(Vector2(i, j), Vector2(1, 1));
			}
		}
		
		vaildatePiecesOrder();
		for (Piece* piece : mSortedPieces )
		{
			drawPiece(g, *piece, selectedPiece == piece);
		}

#if 1
		RenderUtility::SetPen(g, EColor::Red);
		RenderUtility::SetBrush(g, EColor::Null);
		g.drawRect(DebugPos, Vector2(0.1, 0.1));

#endif
		g.endRender();
	}


	void TestStage::drawPiece(RHIGraphics2D& g, Piece const& piece, bool bSelected)
	{
		auto const& shapeData = piece.shape->mDataMap[0];

		RenderUtility::SetPen(g, EColor::Null);
		//RenderUtility::SetBrush(g, EColor::Green);
		g.setBrush(piece.bLocked ? mTheme.pieceBlockLockedColor : mTheme.pieceBlockColor);
		g.pushXForm();
		g.transformXForm(piece.xform, true);
		for (auto const& block : shapeData.blocks)
		{
			g.drawRect(block, Vector2(1,1));
		}
		
		RenderUtility::SetPen(g, piece.bLocked ? EColor::Red : bSelected ? EColor::Yellow : EColor::Gray );
		g.setPenWidth(5);
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

	void Editor::saveLevel(char const* name)
	{
		LevelDesc desc;
		mGame->mLevel.exportDesc(desc);

		OutputFileSerializer serializer;
		serializer.registerVersion(EName::None, ELevelSaveVersion::LastVersion);

		if (!FFileSystem::IsExist("SBlock/levels"))
		{
			FFileSystem::CreateDirectorySequence("SBlock/levels");
		}

		if (serializer.open(InlineString<>::Make("SBlock/levels/%s.lv", name)))
		{
			serializer.write(desc);
			serializer.writeVersionData();
		}

	}

	void Editor::loadLevel(char const* name)
	{
		InputFileSerializer serializer;
		if (serializer.open(InlineString<>::Make("SBlock/levels/%s.lv", name)))
		{
			LevelDesc desc;
			serializer.read(desc);

			mGame->mLevel.importDesc(desc);
			mGame->init();
		}
	}

}