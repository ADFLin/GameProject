#include "SBlocksStage.h"

#include "SBlocksSerialize.h"
#include "SBlocksLevelData.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Serialize/FileStream.h"
#include "FileSystem.h"
#include "Widget/ConsoleFrame.h"

namespace SBlocks
{
	REGISTER_STAGE_ENTRY("Stone Blocks", LevelStage, EExecGroup::Dev4, "Game");

#define SBLOCKS_DIR "SBlocks"


	TConsoleVariable< bool > CVarDevMode{false, "SBlocks.DevMode", CVF_TOGGLEABLE};
	TConsoleVariable< bool > CVarSolverEnableRejection{ true, "SBlocks.SolverEnableRejection", CVF_TOGGLEABLE };
	TConsoleVariable< bool > CVarSolverEnableSortPiece{ true, "SBlocks.SolverEnableSortPiece", CVF_TOGGLEABLE };
	TConsoleVariable< bool > CVarSolverEnableIdenticalShapeCombination{ true, "SBlocks.SolverEnableIdenticalShapeCombination", CVF_TOGGLEABLE };
	TConsoleVariable< int  > CVarSolverParallelThreadCount{ 8, "SBlocks.SolverParallelThreadCount" };

	void LevelStage::restart()
	{
		mLevel.importDesc(TestLv3);
		initializeGame();
	}

	void LevelStage::onRender(float dFrame)
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

		g.transformXForm(mWorldToScreen, true);

		float const MapBorder = 0.3;
		float const MapFrameWidth = 0.3;

		float const RenderBorder = MapBorder + 0.5 * MapFrameWidth;

		for (int indexMap = 0; indexMap < mLevel.mMaps.size(); ++indexMap)
		{
			MarkMap const& map = mLevel.mMaps[indexMap];

			g.pushXForm();
			g.translateXForm(map.mPos.x, map.mPos.y);

			Vec2i mapSize = map.getBoundSize();

			RenderUtility::SetPen(g, EColor::Null);
			g.setBrush(mTheme.mapOuterColor);
			g.drawRect(-Vector2(RenderBorder, RenderBorder), Vector2(mapSize) + 2.0 * Vector2(RenderBorder, RenderBorder));

			float MapOffset = MapFrameWidth + MapBorder;
			RenderUtility::SetPen(g, EColor::Null);

			auto DrawMapFrame = [&]()
			{
				g.drawRect(-Vector2(MapOffset, MapOffset),
					Vector2(float(mapSize.x) + 2 * MapOffset, MapFrameWidth));
				g.drawRect(Vector2(0, float(mapSize.y) + 2 * MapOffset - MapFrameWidth) - Vector2(MapOffset, MapOffset),
					Vector2(float(mapSize.x) + 2 * MapOffset, MapFrameWidth));

				g.drawRect(Vector2(0, MapFrameWidth) - Vector2(MapOffset, MapOffset),
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
			RenderUtility::SetFont(g, FONT_S8);
			for (int j = 0; j < mapSize.y; ++j)
			{
				for (int i = 0; i < mapSize.x; ++i)
				{
					uint8 value = map.getValue(i, j);

					RenderUtility::SetPen(g, EColor::Null);
					if (MarkMap::HaveBlock(value))
					{
						g.setBrush(mTheme.mapBlockColor);
					}
					else
					{
						g.setBrush(mTheme.mapEmptyColor);
					}

					float border = 0.025;
					g.drawRect(Vector2(i + border, j + border), Vector2(1 - 2 * border, 1 - 2 * border));

					if (CVarDevMode)
					{
						g.pushXForm();
						g.identityXForm();
						Vector2 pos = mWorldToScreen.transformPosition(Vector2(i + border, j + border));
						g.drawText(pos, InlineString<>::Make("%d-%d-%d", 
							value ,
							(int)MarkMap::IsLocked(value),
							(int)MarkMap::GetType(value)) );
						g.popXForm();
					}
				}
			}

			g.popXForm();
		}
	
		vaildatePiecesOrder();
		for (Piece* piece : mSortedPieces )
		{
			drawPiece(g, *piece, selectedPiece == piece);
			if (CVarDevMode)
			{
				g.pushXForm();
				g.identityXForm();

				Vector2 pos = mWorldToScreen.transformPosition( piece->xFormRender.transformPosition(piece->shape->pivot) );
				g.drawText(pos, InlineString<>::Make( "%d", piece->index ) );
				g.popXForm();
			}
		}

		if ( CVarDevMode )
		{
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(DebugPos, Vector2(0.1, 0.1));
		}

		g.endRender();
	}


	void LevelStage::drawPiece(RHIGraphics2D& g, Piece const& piece, bool bSelected)
	{
		auto const& shapeData = piece.shape->mDataMap[0];


		RenderUtility::SetPen(g, EColor::Null);

		
		g.pushXForm();
		g.transformXForm(piece.xFormRender, true);
		auto DrawPiece = [&]()
		{
			for (auto const& block : shapeData.blocks)
			{
				g.drawRect(block.pos, Vector2(1, 1));
			}
		};

		if ( piece.isLocked() == false )
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
		
		g.setBrush(piece.isLocked() ? mTheme.pieceBlockLockedColor : mTheme.pieceBlockColor);
		DrawPiece();

		
		RenderUtility::SetPen(g, piece.isLocked() ? EColor::Red : bSelected ? EColor::Yellow : piece.bCanRoate ? EColor::Gray : EColor::Black );
		g.setPenWidth(3);
		for (auto const& line : piece.shape->outlines)
		{
			g.drawLine(line.start, line.end);
		}
		g.setPenWidth(1);
		g.popXForm();
	}

	ERenderSystem LevelStage::getDefaultRenderSystem()
	{
		return ERenderSystem::D3D12;
	}

	bool LevelStage::setupRenderSystem(ERenderSystem systemName)
	{
		return true;
	}

	void LevelStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{

	}

	void LevelStage::solveLevel(int option)
	{
		bool bForceReset = option == 1;
		bool bFullSolve = option == 2;
		bool bParallelSolve = option == 3;
		TIME_SCOPE("Solve Level");

		bool bFristSolve = false;
		if (bFullSolve || bParallelSolve || bForceReset || mSolver == nullptr)
		{
			SolveOption option;
			option.bEnableSortPiece = CVarSolverEnableSortPiece;
			option.bEnableRejection = CVarSolverEnableRejection;
			option.bEnableIdenticalShapeCombination = CVarSolverEnableIdenticalShapeCombination;
			
			TIME_SCOPE("Solver Setup");
			mSolver = std::make_unique< Solver >();
			mSolver->setup(mLevel, option);
			bFristSolve = true;
		}

		if (bFullSolve)
		{
			int numSolutions = mSolver->solveAll();
			LogMsg("Solve level %d Solution !", numSolutions);
			mSolver.release();
			return;
		}
		else if (bParallelSolve)
		{
			{
				TIME_SCOPE("SolveParallel");
				mSolver->solveParallel(CVarSolverParallelThreadCount);
			}
#if 0
			int numSolutions = 0;
			while (!mSolver->mSolutionList.empty())
			{
				++numSolutions;
				mSolver->mSolutionList.pop();
			}
			LogMsg("Solve level %d Solution !", numSolutions);
#endif
			return;
		}


		bool bSuccess;
		if (bFristSolve)
		{
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
				auto const& state = sovledStates[piece->indexSolve];
				LogMsg("%d = [%d](%d, %d) dir = %d", i, state.mapIndex, state.pos.x, state.pos.y, state.dir);
				piece->dir = DirType::ValueChecked(state.dir);
				piece->angle = piece->dir * Math::PI / 2;
				piece->pos = mLevel.calcPiecePos(*piece, state.mapIndex , state.pos , DirType::ValueChecked(state.dir));
				piece->updateTransform();

				mLevel.tryLockPiece(*piece);
			}
		}
		else
		{
			mSolver.release();
		}
	}

	void LevelStage::loadLevel(char const* name)
	{
		InputFileSerializer serializer;
		if (serializer.open(InlineString<>::Make(SBLOCKS_DIR"/levels/%s.lv", name)))
		{
			LevelDesc desc;
			serializer.read(desc);

			mLevel.importDesc(desc);
			initializeGame();
			LogMsg("Load Level %s Success", name);
		}
		else
		{
			LogMsg("Load Level %s Fail", name);
		}
	}

	bool LevelStage::onKey(KeyMsg const& msg)
	{
		if (!msg.isDown())
			return false;

		switch (msg.getCode())
		{
		case EKeyCode::R: restart(); break;
		case EKeyCode::Num1:
			if (IsEditEnabled())
			{
				mEditor->endEdit();
			}
			else
			{
				if (mEditor == nullptr)
				{
					mEditor = new Editor;
					mEditor->mGame = this;
					mEditor->initEdit();
				}
				mEditor->startEdit();
			}
			break;
		case EKeyCode::Return:
			if (true)
			{
				if (mCmdTextCtrl == nullptr)
				{
					int len = 400;

					Vec2i screenSize = ::Global::GetScreenSize();

					mCmdTextCtrl = new ConsoleCmdTextCtrl(UI_CmdCtrl, Vec2i( ( screenSize.x - len ) / 2, screenSize.y - 50), len , nullptr);

					mCmdTextCtrl->mNamespace = "SBlocks";
					::Global::GUI().addWidget(mCmdTextCtrl);
				}
				mCmdTextCtrl->show();
				mCmdTextCtrl->makeFocus();
			}
		}
		return BaseClass::onKey(msg);
	}

	bool LevelStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch (id)
		{
		case UI_CmdCtrl:
			if (event == EVT_TEXTCTRL_COMMITTED)
			{
				if (mCmdTextCtrl)
				{
					mCmdTextCtrl->show(false);
					mCmdTextCtrl->clearFocus();
				}
			}
			return false;
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	void Editor::registerCommand()
	{
		auto& console = ConsoleSystem::Get();
#define REGISTER_COM( NAME , FUNC )\
		console.registerCommand("SBlocks."NAME, &Editor::FUNC, this)

		REGISTER_COM("Save", saveLevel);
		REGISTER_COM("New", newLevel);

		REGISTER_COM("SetMapSize", setMapSize);
		REGISTER_COM("SetMapPos", setMapPos);
		REGISTER_COM("AddMap", addMap);
		REGISTER_COM("RemoveMap", removeMap);


		REGISTER_COM("AddPiece", addPieceCmd);
		REGISTER_COM("RemovePiece", removePiece);

		REGISTER_COM("AddShape", addEditPieceShape);
		REGISTER_COM("RemoveShape", addEditPieceShape);
		REGISTER_COM("CopyShape", copyEditPieceShape);
		REGISTER_COM("EditShape", editEditPieceShapeCmd);

		REGISTER_COM("RunScript", runScript);
#undef REGISTER_COM
	}

	void Editor::unregisterCommand()
	{
		auto& console = ConsoleSystem::Get();
		console.unregisterAllCommandsByObject(this);
	}

	char const* BatchScript[] =
	{
		"Load lv05",
	};

	void Editor::runScript()
	{
		for (int i = 0; i < ARRAY_SIZE(BatchScript); ++i)
		{
			std::string cmd = "SBlocks.";
			cmd += BatchScript[i];
			ConsoleSystem::Get().executeCommand(cmd.c_str());
		}
	}


	void Editor::saveLevel(char const* name)
	{

		if (!FFileSystem::IsExist(SBLOCKS_DIR"/levels"))
		{
			FFileSystem::CreateDirectorySequence(SBLOCKS_DIR"/levels");
		}

		OutputFileSerializer serializer;
		serializer.registerVersion(EName::None, ELevelSaveVersion::LastVersion);

		if (serializer.open(InlineString<>::Make(SBLOCKS_DIR"/levels/%s.lv", name)))
		{
			LevelDesc desc;
			mGame->mLevel.exportDesc(desc);
			serializer.write(desc);
			LogMsg("Save Level %s Success", name);
		}
		else
		{
			LogMsg("Save Level %s Fail", name);
		}
	}


	void Editor::newLevel()
	{
		mGame->mLevel.importDesc(DefautlNewLevel);
		mGame->initializeGame();
	}

	void Editor::editEditPieceShapeCmd(int id)
	{
		if (!IsValidIndex(mPieceShapeLibrary, id))
			return;

		EditPieceShape& editShape = mPieceShapeLibrary[id];
		editEditPieceShape(editShape);

	}

	void Editor::editEditPieceShape(EditPieceShape& editShape)
	{
		if (mShapeEditPanel == nullptr)
		{
			mShapeEditPanel = new ShapeEditPanel(UI_ANY, Vec2i(0, 0), Vec2i(200, 200), nullptr);
			::Global::GUI().addWidget(mShapeEditPanel);
		}

	
		mShapeEditPanel->editor = this;
		mShapeEditPanel->mShape = &editShape;
		mShapeEditPanel->init();

		mShapeEditPanel->show();
	}

	void Editor::registerGamePieces()
	{
		for (int indexPiece = 0; indexPiece < mGame->mLevel.mPieces.size(); ++indexPiece)
		{
			Piece* piece = mGame->mLevel.mPieces[indexPiece].get();
			EditPieceShape* editShape = registerEditShape(piece->shape);
			editShape->usageCount += 1;
		}
	}

	EditPieceShape* Editor::registerEditShape(PieceShape* shape)
	{
		for (EditPieceShape& editShape : mPieceShapeLibrary)
		{
			if (editShape.ptr == shape)
				return &editShape;
		}

		for (EditPieceShape& editShape : mPieceShapeLibrary)
		{
			PieceShapeData shapeData;
			shapeData.initialize(editShape.desc);
			int dir = shape->findSameShape(shapeData);

			if (dir != INDEX_NONE)
			{
				editShape.ptr = shape;
				editShape.rotation = dir;

				return &editShape;
			}
		}

		EditPieceShape editShape;
		editShape.bMarkSave = false;
		editShape.usageCount = 0;
		editShape.ptr = shape;
		editShape.rotation = 0;
		shape->exportDesc(editShape.desc);

		mPieceShapeLibrary.push_back(std::move(editShape));
		return &mPieceShapeLibrary.back();
	}

	void Editor::saveShapeLibrary()
	{
		OutputFileSerializer serializer;
		if (!FFileSystem::IsExist(SBLOCKS_DIR))
		{
			FFileSystem::CreateDirectorySequence(SBLOCKS_DIR);
		}

		if (serializer.open(SBLOCKS_DIR"/ShapeLib.bin"))
		{
			serializer.registerVersion("LevelVersion", ELevelSaveVersion::LastVersion);
			serializeShapeLibrary(IStreamSerializer::WriteOp(serializer));
		}
	}

	void Editor::loadShapeLibrary()
	{
		InputFileSerializer serializer;
		if (serializer.open(SBLOCKS_DIR"/ShapeLib.bin"))
		{
			serializeShapeLibrary(IStreamSerializer::ReadOp(serializer));
		}
	}

	void Editor::startEdit()
	{
		mbEnabled = true;

		LogMsg("Edit Start");
		registerCommand();
		mGame->mLevel.unlockAllPiece();

		registerGamePieces();

		mShapeLibraryPanel = new ShapeListPanel(UI_ANY, Vec2i(10, 10), Vec2i(::Global::GetScreenSize().x - 20, 120), nullptr);
		mShapeLibraryPanel->mEditor = this;
		::Global::GUI().addWidget(mShapeLibraryPanel);

		mShapeLibraryPanel->refreshShapeList();
	}

	void Editor::endEdit()
	{
		if (mShapeEditPanel)
		{
			mShapeEditPanel->destroy();
			mShapeEditPanel = nullptr;
		}

		if (mShapeLibraryPanel)
		{
			mShapeLibraryPanel->destroy();
			mShapeLibraryPanel = nullptr;
		}
		saveShapeLibrary();

		LogMsg("Edit End");
		unregisterCommand();

		mbEnabled = false;
	}

	void Editor::notifyLevelChanged()
	{
		RemoveAllPred(mPieceShapeLibrary, [](auto& value)
		{
			return value.bMarkSave == false;
		});

		for (EditPieceShape& editShape : mPieceShapeLibrary)
		{
			editShape.ptr = nullptr;
			editShape.usageCount = 0;
		}

		if (mShapeEditPanel)
		{
			mShapeEditPanel->destroy();
			mShapeEditPanel = nullptr;
		}

		if (mbEnabled)
		{
			registerGamePieces();
		}

		if (mShapeLibraryPanel)
		{
			mShapeLibraryPanel->refreshShapeList();
		}
	}

	void Editor::addEditPieceShape(int sizeX, int sizeY)
	{
		EditPieceShape editShape;
		editShape.bMarkSave = false;
		editShape.usageCount = 0;
		editShape.ptr = nullptr;
		editShape.desc.bUseCustomPivot = false;
		editShape.desc.customPivot = 0.5 * Vector2(sizeX, sizeY);
		editShape.desc.sizeX = sizeX;
		editShape.desc.data.resize(sizeX * sizeY, 1);
		mPieceShapeLibrary.push_back(std::move(editShape));

		mShapeLibraryPanel->refreshShapeList();
	}

	void Editor::removeEditPieceShape(int id)
	{
		if (!IsValidIndex(mPieceShapeLibrary, id))
			return;

		EditPieceShape& shape = mPieceShapeLibrary[id];
		if (shape.ptr)
		{
			if (shape.usageCount != 0)
				return;

			mGame->mLevel.removePieceShape(shape.ptr);
		}

		mPieceShapeLibrary.erase(mPieceShapeLibrary.begin() + id);

		mShapeLibraryPanel->refreshShapeList();
	}

	void Editor::copyEditPieceShape(int id)
	{
		if (!IsValidIndex(mPieceShapeLibrary, id))
			return;

		EditPieceShape editShape = mPieceShapeLibrary[id];
		editShape.bMarkSave = false;
		editShape.ptr = nullptr;
		editShape.rotation = 0;
		editShape.usageCount = 0;
		mPieceShapeLibrary.push_back(std::move(editShape));

		mShapeLibraryPanel->refreshShapeList();
	}

	void Editor::addPieceCmd(int id)
	{
		if (!IsValidIndex(mPieceShapeLibrary, id))
			return;

		EditPieceShape& editShape = mPieceShapeLibrary[id];

		addPiece(editShape);

	}

	void Editor::addPiece(EditPieceShape &editShape)
	{
		if (editShape.ptr == nullptr)
		{
			editShape.ptr = mGame->mLevel.findPieceShape(editShape.desc, editShape.rotation);
			if (editShape.ptr == nullptr)
			{
				editShape.ptr = mGame->mLevel.createPieceShape(editShape.desc);
				editShape.rotation = 0;
			}
		}

		Piece* piece = mGame->mLevel.createPiece(*editShape.ptr, DirType::ValueChecked(editShape.rotation));
		editShape.usageCount += 1;
		mGame->refreshPieceList();
	}

	void Editor::removePiece(int id)
	{
		auto& piecesList = mGame->mLevel.mPieces;
		if (!IsValidIndex(piecesList, id))
			return;

		Piece* piece = piecesList[id].get();
		auto iter = std::find_if(mPieceShapeLibrary.begin(), mPieceShapeLibrary.end(),
			[piece](auto& value)
		{
			return value.ptr = piece->shape;
		}
		);
		CHECK(iter != mPieceShapeLibrary.end());

		if (iter->ptr)
		{
			iter->usageCount -= 1;
			if (iter->usageCount <= 0)
			{
				mGame->mLevel.removePieceShape(iter->ptr);
				iter->ptr = nullptr;
				iter->rotation = 0;
			}
		}

		piecesList.erase(piecesList.begin() + id);
		mGame->refreshPieceList();
	}

}