#include "LifeStage.h"

#include "GollyFile.h"

#include "Serialize/FileStream.h"
#include "FileSystem.h"
#include "Serialize/SerializeFwd.h"
#include "InputManager.h"


namespace Life
{
	REGISTER_STAGE_ENTRY("Game of Life", TestStage, EExecGroup::Dev4, "Game");

	TConsoleVariable< bool > CVarUseRenderer{ true, "Life.UseRenderer", CVF_TOGGLEABLE };

#define LIFE_DIR "LifeGame"

	bool TestStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto& console = ConsoleSystem::Get();

#define REGISTER_COM( NAME , FUNC , ... )\
				console.registerCommand("Life."NAME, &TestStage::FUNC, this , ##__VA_ARGS__ )
		REGISTER_COM("Save", savePattern);
		REGISTER_COM("Load", loadPattern);
		REGISTER_COM("LoadGolly", loadGolly);
#undef REGISTER_COM

		mEditMode = EditMode::SelectCell;

		restart();

		auto devFrame = WidgetUtility::CreateDevFrame();
		FWidgetProperty::Bind(devFrame->addCheckBox(UI_ANY, "Run"), bRunEvolate);
		devFrame->addButton("Step", [this](int event, GWidget*)
		{
			if (bRunEvolate == false)
			{
				mAlgorithm->step();
			}
			return false;
		});
		devFrame->addText("Evolate Time Rate");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), evolateTimeRate, 0.5, 100, 2, [this](float v)
		{

		});

		devFrame->addText("Zoom Out");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mViewport.zoomOut, 0.25, 25.0, 2, [this](float v)
		{
			mViewport.updateTransform();
		});
		devFrame->addButton("Clear", [this](int event, GWidget*)
		{
			if (bRunEvolate == false)
			{
				mAlgorithm->clearCell();
			}
			return false;
		});
		devFrame->addButton("ChangeMode", [this](int event, GWidget* widget)
		{
			if (mEditMode == EditMode::AddCell )
			{
				mEditMode = EditMode::SelectCell;
				widget->cast<GButton>()->setTitle("Select Mode");
			}
			else
			{
				if (mSelectionRect)
				{
					mSelectionRect->destroy();
					mSelectionRect = nullptr;
				}

				mEditMode = EditMode::AddCell;
				widget->cast<GButton>()->setTitle("Add Mode");
			}
			return false;
		});
		return true;
	}


	struct PatternData
	{
		std::vector< Vec2i > cellList;

		template< class OP >
		void serialize(OP&& op)
		{
			op & cellList;
		}
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(PatternData);

	bool TestStage::savePattern(char const* name)
	{
		if (!FFileSystem::IsExist(LIFE_DIR))
		{
			FFileSystem::CreateDirectorySequence(LIFE_DIR);
		}

		OutputFileSerializer serializer;
		if (!serializer.open(InlineString<>::Make(LIFE_DIR"/%s.bin", name)))
		{
			LogWarning(0, "Can't Open Pattern File : %s", name);
			return false;
		}

		//serializer.registerVersion("LevelVersion", ELevelSaveVersion::LastVersion);

		PatternData data;
		mAlgorithm->getPattern(data.cellList);
		serializer.write(data);
		return true;
	}

	bool TestStage::loadPattern(char const* name)
	{
		InputFileSerializer serializer;
		if (!serializer.open(InlineString<>::Make(LIFE_DIR"/%s.bin", name)))
		{
			LogWarning(0, "Can't Open Pattern File : %s", name);
			return false;
		}

		PatternData data;
		serializer.read(data);

		mAlgorithm->clearCell();
		for (auto const& pos : data.cellList)
		{
			mAlgorithm->setCell(pos.x, pos.y, 1);
		}

		return true;
	}

	bool TestStage::loadGolly(char const* name)
	{
		GollyFileReader reader;
		InlineString<> path;
		path.format(LIFE_DIR"/%s", name);
		char const* err = reader.readpattern(path, *mAlgorithm);
		if (err)
		{
			LogWarning(0, "Load Fail : %s", err);
			return false;
		}
		return true;
	}

	void TestStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vec2i screenSize = ::Global::GetScreenSize();
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);

		g.beginRender();

		RenderUtility::SetPen(g, EColor::Null);
		RenderUtility::SetBrush(g, EColor::Gray, COLOR_DEEP);
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		float cellScale = mViewport.cellRenderScale;

		bool bDrawGridLine = true;
		float gridLineAlpha = 1.0f;
		float GridLineFadeStart = 10;
		if (cellScale < 3.0)
		{
			bDrawGridLine = false;
		}
		else if (cellScale < GridLineFadeStart)
		{
			gridLineAlpha = Remap(cellScale, 3.0, GridLineFadeStart, 0, 1);

		}


		g.pushXForm();
		g.transformXForm(mViewport.xform, true);


		BoundBox boundRender = mViewport.getViewBound();

		BoundBox limitBound = mAlgorithm->getLimitBound();
		if (limitBound.isValid())
		{
			boundRender = boundRender.intersection(limitBound);
		}

		if (bDrawGridLine)
		{
			if (gridLineAlpha != 1.0)
			{
				g.beginBlend(gridLineAlpha);
			}
			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::White);
			Vector2 offsetH = Vector2(boundRender.getSize().x + 1, 0);
			for (int j = boundRender.min.y; j <= boundRender.max.y + 1; ++j)
			{
				Vector2 pos = Vector2(boundRender.min.x, j);
				g.drawLine(pos, pos + offsetH);
			}

			Vector2 offsetV = Vector2(0, boundRender.getSize().y + 1);
			for (int i = boundRender.min.x; i <= boundRender.max.x + 1; ++i)
			{
				Vector2 pos = Vector2(i, boundRender.min.y);
				g.drawLine(pos, pos + offsetV);
			}

			if (gridLineAlpha != 1.0)
			{
				g.endBlend();
			}


		}

		{
			RenderUtility::SetPen(g, EColor::Yellow);
			Vector2 offsetH = Vector2(boundRender.getSize().x + 1, 0);
			for (int j = ChunkAlgo::ChunkLength * ( (boundRender.min.y + ChunkAlgo::ChunkLength - 1 ) / ChunkAlgo::ChunkLength ); j <= boundRender.max.y + 1; j += ChunkAlgo::ChunkLength)
			{
				Vector2 pos = Vector2(boundRender.min.x, j);
				g.drawLine(pos, pos + offsetH);
			}

			Vector2 offsetV = Vector2(0, boundRender.getSize().y + 1);
			for (int i = ChunkAlgo::ChunkLength * ( (boundRender.min.x + ChunkAlgo::ChunkLength - 1 ) / ChunkAlgo::ChunkLength ); i <= boundRender.max.x + 1; i += ChunkAlgo::ChunkLength)
			{
				Vector2 pos = Vector2(i, boundRender.min.y);
				g.drawLine(pos, pos + offsetV);
			}
		}

		RenderUtility::SetPen(g, (cellScale > 3.0) ? EColor::Black : EColor::Null);
		RenderUtility::SetBrush(g, EColor::White);
		IRenderer* renderer = mAlgorithm->getRenderer();
		if (CVarUseRenderer && renderer)
		{
			renderer->draw(g, mViewport, boundRender);
		}
		else
		{

			BoundBox boundCellRender = boundRender.intersection(mAlgorithm->getBound());

			if (boundCellRender.isValid())
			{
				for (int j = boundCellRender.min.y; j <= boundCellRender.max.y; ++j)
				{
					for (int i = boundCellRender.min.x; i <= boundCellRender.max.x; ++i)
					{
						uint8 value = mAlgorithm->getCell(i, j);
						if (value)
						{
							g.drawRect(Vector2(i, j), Vector2(1, 1));
						}
					}
				}
			}
		}

		if (mEditMode == EditMode::SelectCell)
		{
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Red);
			for (auto const& pos : mCopyPattern.cellList)
			{
				g.drawRect(pos, Vector2(1, 1));
			}
		}
		if (mEditMode == EditMode::PastePattern)
		{
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Red);

			Vec2i originPos = mViewport.xform.transformInvPosition(mLastMousePos);
			for (auto const& pos : mCopyPattern.cellList)
			{
				g.drawRect(originPos + pos, Vector2(1, 1));
			}
		}


		g.popXForm();

		g.endRender();
	}

	bool TestStage::onMouse(MouseMsg const& msg)
	{
		mLastMousePos = msg.getPos();

		if (msg.onLeftDown())
		{
			switch (mEditMode)
			{
			case EditMode::AddCell:
				if (bRunEvolate == false || InputManager::Get().isKeyDown(EKeyCode::Control) )
				{
					Vector2 localPos = mViewport.xform.transformInvPosition(msg.getPos());
					Vec2i cellPos = { Math::FloorToInt(localPos.x), Math::FloorToInt(localPos.y) };

					uint8 value = mAlgorithm->getCell(cellPos.x, cellPos.y);
					mAlgorithm->setCell(cellPos.x, cellPos.y, !value);
				}
				break;
			case EditMode::SelectCell:
				{
					if (mSelectionRect == nullptr)
					{
						mSelectionRect = new SelectionRect(mLastMousePos, Vec2i(0, 0), nullptr);
						::Global::GUI().addWidget(mSelectionRect);
						mSelectionRect->onLock = [this]()
						{
							selectPattern(InputManager::Get().isKeyDown(EKeyCode::Shift));
						};
						mSelectionRect->startResize(mLastMousePos);
					}
				}
				break;
			case EditMode::PastePattern:
				{
					Vec2i originPos = mViewport.xform.transformInvPosition(mLastMousePos);
					for (auto const& pos : mCopyPattern.cellList)
					{
						Vec2i cellPos = originPos + pos;
						mAlgorithm->setCell(cellPos.x, cellPos.y, 1);
					}
				}
				break;
			}
		}
		else if (msg.onRightDown())
		{
			bStartDragging = true;
			draggingPos = mViewport.xform.transformInvPosition(msg.getPos());

			if (mSelectionRect)
			{
				mSelectionRect->destroy();
				mSelectionRect = nullptr;
			}
		}
		else if (msg.onRightUp())
		{
			bStartDragging = false;
		}
		else if (msg.onMoving())
		{
			if (bStartDragging)
			{
				Vector2 localPos = mViewport.xform.transformInvPosition(msg.getPos());
				mViewport.pos -= (localPos - draggingPos);
				mViewport.updateTransform();
			}
		}

		return BaseClass::onMouse(msg);
	}

	bool TestStage::onKey(KeyMsg const& msg)
	{
		if (!msg.isDown())
			return false;

		auto RemoveCells = [this]()
		{
			for (auto const& pos : mCopyPattern.cellList)
			{
				mAlgorithm->setCell(pos.x, pos.y, 0);
			}
		};

		switch (msg.getCode())
		{
		case EKeyCode::Escape:
			{
				if (mEditMode == EditMode::PastePattern)
				{
					mEditMode = EditMode::SelectCell;
				}
			}
			break;
		case EKeyCode::R:
			{
				if (mEditMode == EditMode::PastePattern)
				{
					mCopyPattern.rotate();
				}
				else
				{
					restart();
				}
			}
			break;
		case EKeyCode::W:
		case EKeyCode::V:
			{
				if (mEditMode == EditMode::PastePattern)
				{
					mCopyPattern.mirrorV();
				}
			}
			break;
		case EKeyCode::E:
		case EKeyCode::H:
			{
				if (mEditMode == EditMode::PastePattern)
				{
					mCopyPattern.mirrorH();
				}
			}
			break;
		case EKeyCode::Return:
			{
				if (mEditMode == EditMode::SelectCell)
				{
					if (mSelectionRect)
					{
						selectPattern(InputManager::Get().isKeyDown(EKeyCode::Shift));
					}
				}
			}
			break;
		case EKeyCode::X:
			{
				if (mEditMode == EditMode::SelectCell)
				{
					if (InputManager::Get().isKeyDown(EKeyCode::Control))
					{
						RemoveCells();

						mCopyPattern.validate();
						mEditMode = EditMode::PastePattern;
					}
				}
			}
			break;
		case EKeyCode::C:
			{
				if (mEditMode == EditMode::SelectCell)
				{
					if (InputManager::Get().isKeyDown(EKeyCode::Control))
					{
						mCopyPattern.validate();
						mEditMode = EditMode::PastePattern;
					}
				}
			}
			break;
		case EKeyCode::Delete:
			{
				if (mEditMode == EditMode::SelectCell)
				{
					RemoveCells();
					mCopyPattern.cellList.clear();
				}
			}
			break;
		}
		return BaseClass::onKey(msg);
	}

	void TestStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 1024;
		systemConfigs.screenHeight = 768;
		systemConfigs.numSamples = 4;
	}

}//namespace Life