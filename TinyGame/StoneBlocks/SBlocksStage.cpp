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

	Editor* GEditor = nullptr;


	bool LevelStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		restart();

		auto frame = WidgetUtility::CreateDevFrame();

		frame->addCheckBox("Editor", [this](int eventId, GWidget* widget) ->bool
		{
			toggleEditor();
			return true;
		});

		auto& console = ConsoleSystem::Get();
#define REGISTER_COM( NAME , FUNC , ... )\
			console.registerCommand("SBlocks."NAME, &LevelStage::FUNC, this , ##__VA_ARGS__ )

		REGISTER_COM("Solve", solveLevel, CVF_ALLOW_IGNORE_ARGS);
		REGISTER_COM("Load", loadLevel);
		REGISTER_COM("LoadTest", loadTestLevel);
		REGISTER_COM("RunScript", runScript);
#undef REGISTER_COM
		return true;
	}

	void LevelStage::restart()
	{
		loadTestLevel(3);
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
	
#if 1
		validatePiecesOrder();
		for (Piece* piece : mSortedPieces )
		{
			drawPiece(g, *piece, selectedPiece == piece);
			if (CVarDevMode)
			{
				g.pushXForm();
				g.identityXForm();

				Vector2 pos = mWorldToScreen.transformPosition( piece->renderXForm.transformPosition(piece->shape->pivot) );
				g.drawText(pos, InlineString<>::Make( "%d %d %d", piece->index, (int)piece->dir, (int)piece->mirror) );
				g.popXForm();
			}
		}
#endif

		if ( CVarDevMode)
		{
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(DebugPos, Vector2(0.1, 0.1));
		}

		g.endRender();

		RHIFlushCommand(commandList);
	}


	void LevelStage::drawPiece(RHIGraphics2D& g, Piece const& piece, bool bSelected)
	{
		auto const& shapeData = piece.shape->getData(DirType::ValueChecked(0), piece.mirror);

		RenderUtility::SetPen(g, EColor::Null);

		g.pushXForm();
		g.transformXForm(piece.renderXForm, true);
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
			Vector2 offset = piece.renderXForm.transformInvVectorAssumeNoScale(mTheme.shadowOffset);
			g.translateXForm(offset.x, offset.y);
			DrawPiece();
			g.popXForm();
			g.endBlend();
		}
		
		g.setBrush(piece.isLocked() ? mTheme.pieceBlockLockedColor : mTheme.pieceBlockColor);
		DrawPiece();

		
		RenderUtility::SetPen(g, piece.isLocked() ? EColor::Red : bSelected ? EColor::Yellow : piece.bCanOperate ? EColor::Gray : EColor::Black );
		g.setPenWidth(3);
		for (auto const& line : piece.shape->outlines[piece.mirror])
		{
			g.drawLine(line.start, line.end);
		}
		g.setPenWidth(1);
		g.popXForm();
	}

	ERenderSystem LevelStage::getDefaultRenderSystem()
	{
		return ERenderSystem::None;
	}

	bool LevelStage::setupRenderResource(ERenderSystem systemName)
	{
		return true;
	}

	void LevelStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{

	}

	void LevelStage::solveLevel(int option, char const* params)
	{
		bool bForceReset = option == 1;
		bool bFullSolve = option == 2;
		bool bParallelSolve = option == 3;
		bool bDLXSolve = option == 4;
		TIME_SCOPE("Solve Level");

		auto FindOption = [params](char const* option)
		{
			return FCString::StrStr(params, option) != nullptr;
		};

		bool bFirstSolve = false;
		if (bFullSolve || bParallelSolve || bDLXSolve || bForceReset || mSolver == nullptr)
		{
			SolveOption option;
			option.bEnableSortPiece = CVarSolverEnableSortPiece;
			option.bEnableRejection = CVarSolverEnableRejection;
			option.bEnableIdenticalShapeCombination = CVarSolverEnableIdenticalShapeCombination;
			option.bCheckMapSymmetry = true;
			option.bUseMapMask = true;
			option.bReserveLockedPiece = FindOption("RLP");

			TIME_SCOPE("Solver Setup");
			mSolver = std::make_unique< Solver >();
			mSolver->setup(mLevel, option);
			if (bFullSolve || bParallelSolve)
			{
				mSolver->bLogProgress = true;
			}
			bFirstSolve = true;
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
		else if (bDLXSolve)
		{ 
			bool bRecursive = FindOption("Rec");
			int numSolutions = mSolver->solveDLX(bRecursive);
			LogMsg("Solve level %d Solution !", numSolutions);
			return;
		}

		bool bSuccess;
		if (bFirstSolve)
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
			TArray<PieceSolveState> sovledStates;
			mSolver->getSolvedStates(sovledStates);

			if (mSolver->mUsedOption.bReserveLockedPiece)
			{
				for (auto const& solveData : mSolver->mPieceList)
				{
					Piece* piece = solveData.piece;
					if (piece->isLocked())
					{
						mLevel.unlockPiece(*piece);
					}
				}
			}
			else
			{
				mLevel.unlockAllPiece();
			}

			for (auto const& solveData : mSolver->mPieceList)
			{
				Piece* piece = solveData.piece;
				auto const& state = sovledStates[solveData.index];
				if (mLevel.bAllowMirrorOp)
				{
					LogMsg("%d = [%d](%d, %d) dir = %d mirror = %d", piece->index, state.mapIndex, state.pos.x, state.pos.y, state.op.dir, (int)state.op.mirror);
				}
				else
				{
					LogMsg("%d = [%d](%d, %d) dir = %d", piece->index, state.mapIndex, state.pos.x, state.pos.y, state.op.dir);
				}

				PieceShape::OpState opState = piece->shape->getOpState(state.indexData);
				piece->dir = DirType::ValueChecked(state.op.dir);
				piece->mirror = state.op.mirror;
				if (!mLevel.tryLockPiece(*piece, state.mapIndex, state.pos))
				{
					LogWarning(0, "Solution have problem!!");

					piece->pos = mLevel.calcPiecePos(*piece, state.mapIndex, state.pos, DirType::ValueChecked(state.op.dir));
					piece->angle = piece->dir * Math::PI / 2;
					piece->updateTransform();
				}
			}
		}
		else
		{
			mSolver.release();

			if (!bFirstSolve)
			{
				LogMsg("Solve level End!");
			}
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

	void LevelStage::loadTestLevel(int index)
	{
		TArrayView< LevelDesc* const > TestLevels =
		{
			&ClassicalLevel,
			&TestLv,
			&TestLv2,
			&TestLv3,
			&TestLv4,
			&DebugLevel,
		};

		if (TestLevels.isValidIndex(index))
		{
			mLevel.importDesc(*TestLevels[index]);
			initializeGame();
		}
	}


	char const* BatchScript[] =
	{
		"Load lv05",
	};

	char const* BatchScript2[] =
	{
		"Load STest",
		"Solve 2",
	};
	using ScriptData = TArrayView< char const* >;
	TArrayView< ScriptData const > ScriptList = ARRAY_VIEW_REAONLY_DATA(ScriptData, MakeView(BatchScript2), MakeView(BatchScript) );

	void LevelStage::runScript(int index)
	{
		if (ScriptList.isValidIndex(index))
		{
			auto const& script = ScriptList[index];

			for (int i = 0; i < script.size(); ++i)
			{
				std::string cmd = "SBlocks.";
				cmd += script[i];
				ConsoleSystem::Get().executeCommand(cmd.c_str());
			}
		}
	}

	MsgReply LevelStage::onMouse(MouseMsg const& msg)
	{
		struct PieceAngle
		{
			using DataType = Piece;
			using ValueType = float;
			void operator()(DataType& data, ValueType const& value)
			{
				data.angle = value;
				data.updateTransform();
			}
		};

		Vector2 worldPos = mScreenToWorld.transformPosition(msg.getPos());
		Vector2 hitLocalPos;
		Piece* piece = getPiece(worldPos, hitLocalPos);

		if (isEditEnabled())
		{
			MsgReply reply = mEditor->onMouse(msg, worldPos, piece);
			if (reply.isHandled())
				return reply;
		}

		if (msg.onLeftDown())
		{
			if (selectedPiece != piece)
			{
				updatePieceClickFrame(*piece);
			}
			selectedPiece = piece;
			if (selectedPiece)
			{
				lastHitLocalPos = hitLocalPos;
				bStartDragging = true;
				if (selectedPiece->isLocked())
				{
					mLevel.unlockPiece(*piece);
					bPiecesOrderDirty = true;
				}
			}
		}
		if (msg.onLeftUp())
		{
			if (selectedPiece)
			{
				bStartDragging = false;
				if (isEditEnabled() == false)
				{
					if (mLevel.tryLockPiece(*selectedPiece))
					{
						notifyPieceLocked();
						bPiecesOrderDirty = true;
					}
				}

				selectedPiece = nullptr;
			}
		}
		else if (msg.onRightDown())
		{
			if (piece && piece->bCanOperate)
			{
				if (piece->isLocked())
				{
					mLevel.unlockPiece(*piece);
					bPiecesOrderDirty = true;
				}

				updatePieceClickFrame(*piece);
	

				mTweener.tween< Easing::IOBounce, PieceAngle >(*piece, int(piece->dir) * Math::PI / 2, (int(piece->dir) + 1) * Math::PI / 2, 0.1, 0);
				piece->dir += 1;
				//piece->angle = piece->dir * Math::PI / 2;
				piece->updateTransform();
				//mLevel.tryLockPiece(*piece);
			}
		}
		else if (msg.onMiddleDown())
		{
			if (mLevel.bAllowMirrorOp && piece && piece->bCanOperate)
			{
				if (piece->isLocked())
				{
					mLevel.unlockPiece(*piece);
					bPiecesOrderDirty = true;
				}

				updatePieceClickFrame(*piece);

				piece->mirror = (EMirrorOp::Type)((piece->mirror + 1 ) % EMirrorOp::COUNT);
				//piece->angle = piece->dir * Math::PI / 2;
				piece->updateTransform();
				//mLevel.tryLockPiece(*piece);
			}
		}
		if (msg.onMoving())
		{
			if (bStartDragging)
			{
				Vector2 pinPos = selectedPiece->renderXForm.transformPosition(lastHitLocalPos);
				Vector2 offset = worldPos - pinPos;
				selectedPiece->move(offset);
			}
		}

		return BaseClass::onMouse(msg);
	}

	MsgReply LevelStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::Num1:
				toggleEditor();

				break;
			case EKeyCode::Return:
				if (true)
				{
					if (mCmdTextCtrl == nullptr)
					{
						int len = 400;

						Vec2i screenSize = ::Global::GetScreenSize();

						mCmdTextCtrl = new ConsoleCmdTextCtrl(UI_CmdCtrl, Vec2i((screenSize.x - len) / 2, screenSize.y - 50), len, nullptr);

						mCmdTextCtrl->mNamespace = "SBlocks";
						::Global::GUI().addWidget(mCmdTextCtrl);
					}
					mCmdTextCtrl->show();
					mCmdTextCtrl->makeFocus();
				}
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

	void LevelStage::toggleEditor()
	{
		if (isEditEnabled())
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
	}

	void Editor::registerCommand()
	{
		auto& console = ConsoleSystem::Get();
#define REGISTER_COM( NAME , FUNC )\
		console.registerCommand("SBlocks."##NAME, &Editor::FUNC, this)

		REGISTER_COM("Save", saveLevel);
		REGISTER_COM("New", newLevel);
		REGISTER_COM("SetAllowMirrorOp", setAllowMirrorOp);

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

#undef REGISTER_COM
	}

	void Editor::unregisterCommand()
	{
		auto& console = ConsoleSystem::Get();
		console.unregisterAllCommandsByObject(this);
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


	void Editor::setAllowMirrorOp(bool bAllow)
	{
		mGame->mLevel.bAllowMirrorOp = bAllow;
		if (mGame->mLevel.bAllowMirrorOp)
		{
			for (auto& editShape : mPieceShapeLibrary)
			{
				if (editShape.ptr)
				{
					editShape.ptr->registerMirrorData();
				}
			}
		}
	}

	void Editor::editEditPieceShapeCmd(int id)
	{
		if (!mPieceShapeLibrary.isValidIndex(id))
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

		mShapeEditPanel->setup(editShape);
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
			int index = shape->findSameShape(shapeData);

			if (index != INDEX_NONE)
			{
				auto opState = shape->getOpState(index);
				if (opState.mirror == EMirrorOp::None)
				{
					editShape.ptr = shape;
					editShape.rotation = opState.dir;
					return &editShape;
				}
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
		LogMsg("Edit Start");

		mbEnabled = true;

		registerCommand();
		mGame->mLevel.unlockAllPiece();

		registerGamePieces();

		mShapeLibraryPanel = new ShapeListPanel(UI_ANY, Vec2i(10, 10), Vec2i(::Global::GetScreenSize().x - 20, 120), nullptr);
		::Global::GUI().addWidget(mShapeLibraryPanel);

		mShapeLibraryPanel->refreshShapeList();
	}

	void Editor::endEdit()
	{
		LogMsg("Edit End");

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
		unregisterCommand();

		mbEnabled = false;
	}

	void Editor::notifyLevelChanged()
	{
		mPieceShapeLibrary.removeAllPred( [](auto& value)
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

		editEditPieceShape(mPieceShapeLibrary.back());
	}

	void Editor::removeEditPieceShape(int id)
	{
		if (!mPieceShapeLibrary.isValidIndex(id))
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
		if (!mPieceShapeLibrary.isValidIndex(id))
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
		if (!mPieceShapeLibrary.isValidIndex(id))
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
		if (!piecesList.isValidIndex(id))
			return;

		Piece* piece = piecesList[id].get();
		int index = mPieceShapeLibrary.findIndexPred(
			[piece](auto const& value)
			{
				return value.ptr == piece->shape;
			}
		);
		CHECK(index != INDEX_NONE);

		auto& editShape = mPieceShapeLibrary[index];
		if (editShape.ptr)
		{
			editShape.usageCount -= 1;
			if (editShape.usageCount <= 0)
			{
				mGame->mLevel.removePieceShape(editShape.ptr);
				editShape.ptr = nullptr;
				editShape.rotation = 0;
			}
		}

		piecesList.removeIndex(id);
		mGame->refreshPieceList();
	}

}