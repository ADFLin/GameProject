#include "SBlocksEditor.h"

#include "SBlocksLevelData.h"
#include "SBlocksSerialize.h"

#include "GameGUISystem.h"

#include "ConsoleSystem.h"

#include "FileSystem.h"
#include "Serialize/FileStream.h"

#include "RHI/RHIGraphics2D.h"


#define SBLOCKS_DIR "SBlocks"

namespace SBlocks
{
	void Editor::registerCommand()
	{
		auto& console = IConsoleSystem::Get();
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
		auto& console = IConsoleSystem::Get();
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
			mLevel->exportDesc(desc);
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
		mLevel->importDesc(DefautlNewLevel);
		mLevel->initialize();
		mEventHandler->onNewLevel();
	}

	void Editor::setAllowMirrorOp(bool bAllow)
	{
		mLevel->bAllowMirrorOp = bAllow;
		if (mLevel->bAllowMirrorOp)
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
		for (int indexPiece = 0; indexPiece < mLevel->mPieces.size(); ++indexPiece)
		{
			Piece* piece = mLevel->mPieces[indexPiece].get();
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
			PieceShape::InitData shapeData;
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

	void Editor::Draw(RHIGraphics2D& g, SceneTheme& theme, EditPieceShape const& editShape)
	{
		PieceShapeDesc const& desc = editShape.desc;

		RenderUtility::SetPen(g, EColor::Black);

		for (int index = 0; index < desc.data.size(); ++index)
		{
			int x = index % desc.sizeX;
			int y = index / desc.sizeX;

			if (desc.data[index])
			{
				g.setBrush(theme.pieceBlockColor);
				RenderUtility::SetPen(g, EColor::Black);
				g.drawRect(Vector2(x, y), Vector2(1, 1));
			}
			else
			{
				RenderUtility::SetPen(g, EColor::Gray);
				RenderUtility::SetBrush(g, EColor::Gray);
				float const border = 0.1;
				g.drawRect(Vector2(x + border, y + border), Vector2(1 - 2 * border, 1 - 2 * border));
			}
		}

		RenderUtility::SetBrush(g, EColor::Red);
		RenderUtility::SetPen(g, EColor::Red);
		float len = 0.2;
		Vector2 pivot = editShape.desc.getPivot();
		g.drawRect(pivot - 0.5 * Vector2(len, len), Vector2(len, len));
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
		mLevel->unlockAllPiece();

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
		mPieceShapeLibrary.removeAllPred([](auto& value)
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

			mLevel->removePieceShape(shape.ptr);
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
			editShape.ptr = mLevel->findPieceShape(editShape.desc, editShape.rotation);
			if (editShape.ptr == nullptr)
			{
				editShape.ptr = mLevel->createPieceShape(editShape.desc);
				editShape.rotation = 0;
			}
		}

		Piece* piece = mLevel->createPiece(*editShape.ptr, DirType::ValueChecked(editShape.rotation));
		editShape.usageCount += 1;
		mLevel->refreshPieceList();
	}

	void Editor::removePiece(int id)
	{
		auto& piecesList = mLevel->mPieces;
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
				mLevel->removePieceShape(editShape.ptr);
				editShape.ptr = nullptr;
				editShape.rotation = 0;
			}
		}

		piecesList.removeIndex(id);
		mLevel->refreshPieceList();
	}

	void ShapeEditPanel::onRender()
	{
		BaseClass::onRender();

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		Vec2i screenPos = getWorldPos();
		g.pushXForm();
		g.translateXForm(screenPos.x, screenPos.y);
		g.transformXForm(mLocalToFrame, true);
		Editor::Draw(g, GEditor->mLevel->mTheme, *mShape);
		g.popXForm();
	}

	MsgReply ShapeEditPanel::onMouseMsg(MouseMsg const& msg)
	{
		if (msg.onLeftDown())
		{
			Vec2i framePos = msg.getPos() - getWorldPos();
			Vec2i localPos = mLocalToFrame.transformInvPosition(framePos);
			Vec2i boundSize = mShape->desc.getBoundSize();
			if (IsInBound(localPos, boundSize))
			{
				mShape->desc.toggleValue(localPos);
				if (mShape->ptr)
				{
					mShape->ptr->importDesc(mShape->desc, GEditor->mLevel->bAllowMirrorOp);
				}
				return MsgReply::Handled();
			}
		}

		return BaseClass::onMouseMsg(msg);
	}

	void ShapeListPanel::refreshShapeList()
	{
		mShapeList.clear();

		Vector2 pos = { 5 , 5 };
		float scale = 20.0;
		float maxYSize = 0;
		Vec2i widgetSize = getSize();
		for (auto& editShape : GEditor->mPieceShapeLibrary)
		{
			Vec2i boundSize = editShape.desc.getBoundSize();
			float xSize = boundSize.x * scale;
			float ySize = boundSize.y * scale;

			if (pos.x + xSize + 5 > widgetSize.x)
			{
				pos.y += maxYSize + 15;
				pos.x = 5;
				maxYSize = 0;
			}

			ShapeInfo shapeInfo;
			shapeInfo.editShape = &editShape;
			shapeInfo.localToFrame.setIdentity();
			shapeInfo.localToFrame.translateLocal(pos);
			shapeInfo.localToFrame.scaleLocal(Vector2(scale, scale));
			shapeInfo.FrameToLocal = shapeInfo.localToFrame.inverse();
			mShapeList.push_back(std::move(shapeInfo));

			pos.x += xSize + 5;
			if (maxYSize < ySize)
			{
				maxYSize = ySize;
			}

		}

		if (pos.y + maxYSize + 15 + 5 > widgetSize.y)
		{
			setSize(Vec2i(widgetSize.x, pos.y + maxYSize + 15 + 5));
		}
	}

	void ShapeListPanel::onRender()
	{
		BaseClass::onRender();

		float scale = 20.0;

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		Vec2i screenPos = getWorldPos();
		g.pushXForm();
		g.translateXForm(screenPos.x, screenPos.y);

		RenderUtility::SetFont(g, FONT_S8);

		for (auto const& shapeInfo : mShapeList)
		{
			g.pushXForm();
			g.transformXForm(shapeInfo.localToFrame, true);


			Vector2 boundSize = Vector2(shapeInfo.editShape->desc.getBoundSize());
			float border = 0.1;
			RenderUtility::SetPen(g, shapeInfo.editShape->bMarkSave ? EColor::Orange : EColor::Gray);
			g.enableBrush(false);
			g.drawRect(-Vector2(border, border), boundSize + 2 * Vector2(border, border));
			Editor::Draw(g, GEditor->mLevel->mTheme, *shapeInfo.editShape);
			g.popXForm();

			Vector2 pos = shapeInfo.localToFrame.transformPosition(Vector2(0, boundSize.y));

			g.drawText(pos, InlineString<>::Make("%d", shapeInfo.editShape->usageCount));

		}

		g.popXForm();
	}

}

