#include "SBlocksStage.h"

#include "SBlocksSerialize.h"
#include "SBlocksLevelData.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Serialize/FileStream.h"
#include "FileSystem.h"
#include "Widget/ConsoleFrame.h"

#define SBLOCKS_DIR "SBlocks"

namespace SBlocks
{
	REGISTER_STAGE_ENTRY("Stone Blocks", LevelStage, EExecGroup::Dev4, "Game");

	TConsoleVariable< bool > CVarDevMode{ false, "SBlocks.DevMode", CVF_TOGGLEABLE };
	TConsoleVariable< bool > CVarShowShape{ false, "SBlocks.ShowShape", CVF_TOGGLEABLE };
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

		auto const& theme = mLevel.mTheme;

		g.beginRender();

		RenderUtility::SetPen(g, EColor::Null);
		g.setBrush(theme.backgroundColor);
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		g.transformXForm(mLevel.mWorldToScreen, true);

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
			g.setBrush(theme.mapOuterColor);
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

			g.setBrush(theme.shadowColor);
			g.beginBlend(theme.shadowOpacity);
			g.pushXForm();
			g.translateXForm(0.5 * theme.shadowOffset.x, 0.5 * theme.shadowOffset.y);
			DrawMapFrame();
			g.popXForm();
			g.endBlend();

			g.setBrush(theme.mapFrameColor);
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
						g.setBrush(theme.mapBlockColor);
					}
					else
					{
						g.setBrush(theme.mapEmptyColor);
					}

					float border = 0.025;
					g.drawRect(Vector2(i + border, j + border), Vector2(1 - 2 * border, 1 - 2 * border));

					if (CVarDevMode)
					{
						g.pushXForm();
						g.identityXForm();
						Vector2 pos = mLevel.mWorldToScreen.transformPosition(Vector2(i + border, j + border));
						g.drawText(pos, InlineString<>::Make("%d-%d-%d",
							value,
							(int)MarkMap::IsLocked(value),
							(int)MarkMap::GetType(value)));
						g.popXForm();
					}
				}
			}

			g.popXForm();
		}

#if 1
		mLevel.validatePiecesOrder();
		for (Piece* piece : mLevel.mSortedPieces)
		{
			drawPiece(g, *piece, selectedPiece == piece);
			if (CVarDevMode)
			{
				g.pushXForm();
				g.identityXForm();

				Vector2 pos = mLevel.mWorldToScreen.transformPosition(piece->renderXForm.transformPosition(piece->shape->pivot));
				g.drawText(pos, InlineString<>::Make("%d %d %d", piece->index, (int)piece->dir, (int)piece->mirror));
				g.popXForm();
			}
		}
#endif

		if (CVarDevMode)
		{
			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(DebugPos, Vector2(0.1, 0.1));
		}

		if (CVarShowShape)
		{
			drawShapeDebug(g);
		}

		g.endRender();
		RHIFlushCommand(commandList);
	}


	void LevelStage::drawPiece(RHIGraphics2D& g, Piece const& piece, bool bSelected)
	{
		auto const& shapeData = piece.shape->getData(DirType::ValueChecked(0), piece.mirror);
		auto const& theme = mLevel.mTheme;

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

		if (piece.isLocked() == false)
		{
			g.setBrush(theme.shadowColor);
			g.beginBlend(theme.shadowOpacity);

			g.pushXForm();
			Vector2 offset = piece.renderXForm.transformInvVectorAssumeNoScale(theme.shadowOffset);
			g.translateXForm(offset.x, offset.y);
			DrawPiece();
			g.popXForm();
			g.endBlend();
		}

		g.setBrush(piece.isLocked() ? theme.pieceBlockLockedColor : theme.pieceBlockColor);
		DrawPiece();


		RenderUtility::SetPen(g, piece.isLocked() ? EColor::Red : bSelected ? EColor::Yellow : piece.bCanOperate ? EColor::Gray : EColor::Black);
		g.setPenWidth(3);
		for (auto const& line : piece.shape->outlines[piece.mirror])
		{
			g.drawLine(line.start, line.end);
		}
		g.setPenWidth(1);
		g.popXForm();
	}

	void LevelStage::drawShapeDebug(RHIGraphics2D& g)
	{
		auto const& theme = mLevel.mTheme;
		RenderUtility::SetPen(g, EColor::Black);
		g.setBrush(theme.pieceBlockColor);

		g.pushXForm();
		g.identityXForm();
		g.translateXForm(10, 10);
		for (auto const& shape : mLevel.mShapes)
		{
			int maxBoundY = 0;
			float len = 8;

			g.pushXForm();
			for (int i = 0; i < shape->getDifferentShapeNum(); ++i)
			{
				auto const& shapeData = shape->getDataByIndex(i);
				if (maxBoundY < shapeData.boundSize.y)
					maxBoundY = shapeData.boundSize.y;

				for (auto const& block : shapeData.blocks)
				{
					g.drawRect(len * block.pos, Vector2(len, len));
				}
				g.translateXForm((float(mMaxBound.x) + 1 ) * len, 0);
			}
			g.popXForm();
			g.translateXForm(0, (float(maxBoundY) + 1) * len);
		}
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

	void LevelStage::onNewLevel()
	{
		mSolver.reset();
		mClickFrame = 0;
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
	TArrayView< ScriptData const > ScriptList = ARRAY_VIEW_REAONLY_DATA(ScriptData, MakeView(BatchScript2), MakeView(BatchScript));

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

		Vector2 worldPos = mLevel.mScreenToWorld.transformPosition(msg.getPos());
		Vector2 hitLocalPos;
		Piece* piece = mLevel.getPiece(worldPos, hitLocalPos);

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
				bDraggingPiece = true;
				if (selectedPiece->isLocked())
				{
					mLevel.unlockPiece(*piece);
					mLevel.bPiecesOrderDirty = true;
				}
			}
		}
		if (msg.onLeftUp())
		{
			if (selectedPiece)
			{
				bDraggingPiece = false;
				if (isEditEnabled() == false)
				{
					if (mLevel.tryLockPiece(*selectedPiece))
					{
						mLevel.notifyPieceLocked();
						mLevel.bPiecesOrderDirty = true;
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
					mLevel.bPiecesOrderDirty = true;
				}

				updatePieceClickFrame(*piece);

				mTweener.tween< Easing::OCubic, PieceAngle >(*piece, int(piece->dir) * Math::PI / 2, (int(piece->dir) + 1) * Math::PI / 2, 0.15, 0);
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
					mLevel.bPiecesOrderDirty = true;
				}

				updatePieceClickFrame(*piece);

				piece->mirror = (EMirrorOp::Type)((piece->mirror + 1) % EMirrorOp::COUNT);
				//piece->angle = piece->dir * Math::PI / 2;
				piece->updateTransform();
				//mLevel.tryLockPiece(*piece);
			}
		}
		if (msg.onMoving())
		{
			if (bDraggingPiece)
			{
				Vector2 pinPos = selectedPiece->renderXForm.transformPosition(lastHitLocalPos);
#if 1
				Vector2 posClamped = Math::Clamp(msg.getPos(), Vector2::Zero(), ::Global::GetScreenSize());
				Vector2 worldPosClamped = mLevel.mScreenToWorld.transformPosition(posClamped);
				selectedPiece->move(worldPosClamped - pinPos);
#else
				selectedPiece->move(worldPos - pinPos);
#endif
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
				mEditor->mLevel = &mLevel;
				mEditor->mEventHandler = this;
				mEditor->initEdit();
			}
			mEditor->startEdit();
		}
	}
}