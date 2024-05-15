#include "SBlocksLevel.h"
#include "GameWidget.h"

namespace SBlocks
{
	extern class Editor* GEditor;

	struct EditPieceShape
	{
		PieceShape* ptr;
		int rotation;

		PieceShapeDesc desc;

		bool bMarkSave;
		int  usageCount;
	};

	class EditorEventHandler
	{
	public:
		virtual ~EditorEventHandler() = default;
		virtual void onNewLevel(){}
	};

	class Editor
	{
	public:

		void initEdit()
		{
			GEditor = this;
			loadShapeLibrary();
		}

		void cleanup()
		{
			GEditor = nullptr;
		}

		void startEdit();
		void endEdit();

		MsgReply onMouse(MouseMsg const& msg, Vector2 const& worldPos, Piece* piece)
		{
			if (msg.onLeftDown())
			{
				if (piece == nullptr)
				{
					for (int i = 0; i < mLevel->mMaps.size(); ++i)
					{
						MarkMap& map = mLevel->mMaps[i];
						Vector2 lPos = worldPos - map.mPos;
						Vec2i mapPos;
						mapPos.x = lPos.x;
						mapPos.y = lPos.y;
						if (map.isInBound(mapPos))
						{
							map.toggleBlock(mapPos);
							return MsgReply::Handled();
						}
					}
				}
			}

			return MsgReply::Unhandled();
		}

		MsgReply onKey(KeyMsg const& msg)
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{

				}

			}
			return MsgReply::Unhandled();
		}

		void registerCommand();
		void unregisterCommand();

		void saveLevel(char const* name);
		void newLevel();

		void setAllowMirrorOp(bool bAllow);

		void setMapSize(int id, int x, int y)
		{
			if (mLevel->mMaps.isValidIndex(id))
			{
				mLevel->mMaps[id].resize(x, y);
				mLevel->resetRenderParams();
			}
		}

		void setMapPos(int id, float x, float y)
		{
			if (mLevel->mMaps.isValidIndex(id))
			{
				mLevel->mMaps[id].mPos.setValue(x, y);
				mLevel->resetRenderParams();
			}
		}

		void addMap(int x, int y)
		{
			MarkMap newMap;
			newMap.resize(x, y);
			newMap.mData.fillValue(0);
			mLevel->mMaps.push_back(std::move(newMap));
			mLevel->resetRenderParams();
		}

		void removeMap(int id)
		{
			if (mLevel->mMaps.isValidIndex(id))
			{
				mLevel->mMaps.erase(mLevel->mMaps.begin() + id);
				mLevel->resetRenderParams();
			}
		}

		void addPieceCmd(int id);
		void addPiece(EditPieceShape &editShape);
		void removePiece(int id);


		void addEditPieceShape(int sizeX, int sizeY);
		void removeEditPieceShape(int id);
		void copyEditPieceShape(int id);
		void editEditPieceShapeCmd(int id);
		void editEditPieceShape(EditPieceShape& editShape);

		void notifyLevelChanged();

		class ShapeListPanel* mShapeLibraryPanel = nullptr;
		class ShapeEditPanel* mShapeEditPanel = nullptr;
		struct EditPiece
		{
			Vector2 pos;
			DirType dir;
			bool    bLock;
		};

		void registerGamePieces();

		EditPieceShape* registerEditShape(PieceShape* shape);


		static void Draw(RHIGraphics2D& g, SceneTheme& theme, EditPieceShape const& editShape);

		template< class OP >
		void serializeShapeLibrary(OP& op)
		{
			if (OP::IsSaving)
			{
				int num = std::count_if(
					mPieceShapeLibrary.begin(), mPieceShapeLibrary.end(), 
					[](auto const& value) {  return value.bMarkSave; }
				);
				op & num;
				for (auto& editShape : mPieceShapeLibrary)
				{
					if (editShape.bMarkSave)
					{
						op & editShape.desc;
					}
				}
			}
			else
			{
				op.redirectVersion(EName::None, "LevelVersion");

				int num;
				op & num;
				mPieceShapeLibrary.resize(num);
				for (auto& editShape : mPieceShapeLibrary)
				{
					op & editShape.desc;
					editShape.bMarkSave = true;
					editShape.usageCount = 0;
					editShape.ptr = nullptr;
					editShape.rotation = 0;
				}

				op.redirectVersion(EName::None, EName::None);
			}
		}

		void saveShapeLibrary();
		void loadShapeLibrary();

		TArray< EditPieceShape > mPieceShapeLibrary;

		bool       mbEnabled = false;
		GameLevel* mLevel;
		EditorEventHandler* mEventHandler;
	};

	class ShapeListPanel : public GFrame
	{
		using BaseClass = GFrame;
	public:
		ShapeListPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent = nullptr)
			:GFrame(id, pos, size, parent)
		{

		}

		struct ShapeInfo
		{
			EditPieceShape*   editShape;
			RenderTransform2D localToFrame;
			RenderTransform2D FrameToLocal;
		};
		TArray< ShapeInfo > mShapeList;
		void refreshShapeList();

		void onRender();

		ShapeInfo* clickTest(Vec2i const& framePos)
		{
			for (auto& shapeInfo : mShapeList)
			{
				Vector2 pos = shapeInfo.FrameToLocal.transformPosition(framePos);
				if (IsInBound(Vec2i(pos), shapeInfo.editShape->desc.getBoundSize()))
				{
					return &shapeInfo;
				}
			}

			return nullptr;
		}

		MsgReply onMouseMsg(MouseMsg const& msg)
		{
			Vec2i framePos = msg.getPos() - getWorldPos();
			if (msg.onLeftDown() && msg.isControlDown())
			{
				ShapeInfo* clickShape = clickTest(framePos);

				if (clickShape)
				{
					clickShape->editShape->bMarkSave = !clickShape->editShape->bMarkSave;
				}

			}
			else if (msg.onRightDClick())
			{
				ShapeInfo* clickShape = clickTest(framePos);

				if (clickShape)
				{
					GEditor->editEditPieceShape(*clickShape->editShape);
					return MsgReply::Handled();
				}
			}
			else if (msg.onLeftDClick())
			{
				ShapeInfo* clickShape = clickTest(framePos);

				if (clickShape)
				{
					GEditor->addPiece(*clickShape->editShape);
					return MsgReply::Handled();
				}
			}

			return BaseClass::onMouseMsg(msg);
		}
	};


	class ShapeEditPanel : public GFrame
	{
		using BaseClass = GFrame;
	public:
		ShapeEditPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent = nullptr)
			:GFrame(id, pos, size, parent)
		{

		}

		void setup(EditPieceShape& shape)
		{
			mShape = &shape;
			mLocalToFrame.setIdentity();
			float scale = 20.0;
			Vector2 offset = Vector2(getSize()) - scale * Vector2(mShape->desc.getBoundSize());
			mLocalToFrame.translateLocal(0.5 * offset);
			mLocalToFrame.scaleLocal(Vector2(scale, scale));
		}

		void refreshSize()
		{
			setSize(Vec2i(400, 400));
		}

		void onRender();

		MsgReply onMouseMsg(MouseMsg const& msg);

		RenderTransform2D mLocalToFrame;
		EditPieceShape* mShape;
	};
}