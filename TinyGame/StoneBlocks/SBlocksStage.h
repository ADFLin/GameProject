#include "Stage/TestStageHeader.h"

#include "SBlocksCore.h"
#include "SBlocksSolver.h"

#include "GameRenderSetup.h"
#include "ConsoleSystem.h"

#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include <algorithm>
#include "GameWidget.h"

class ConsoleCmdTextCtrl;

namespace SBlocks
{
	Vector2 DebugPos;

	struct SceneTheme
	{
		Color3ub pieceBlockColor;
		Color3ub pieceBlockLockedColor;
		Color3ub pieceOutlineColor;

		Color3ub mapEmptyColor;
		Color3ub mapBlockColor;
		Color3ub mapOuterColor;
		Color3ub mapFrameColor;

		Color3ub backgroundColor;

		Color3ub shadowColor;
		float    shadowOpacity;
		Vector2  shadowOffset;

		SceneTheme()
		{
			pieceBlockColor = Color3ub(203, 105, 5);
			pieceBlockLockedColor = Color3ub(255, 155, 18);

			mapEmptyColor = Color3ub(165, 82, 37);
			mapBlockColor = Color3ub(126, 51, 15);
			mapOuterColor = Color3ub(111, 39, 9);

			mapFrameColor = Color3ub(222, 186, 130);

			backgroundColor = Color3ub(203, 163, 106);

			shadowColor = Color3ub(20, 20, 20);
			shadowOpacity = 0.5;
			shadowOffset = Vector2(0.2, 0.2);
		}

		template< class OP >
		void serialize(OP& op)
		{
			op & pieceBlockColor & pieceBlockLockedColor;
			op & mapEmptyColor & mapEmptyColor & mapBlockColor & mapOuterColor;
			op & mapFrameColor;
			op & backgroundColor;
			op & shadowColor & shadowOpacity & shadowOffset;
		}

	};

	struct GameData
	{

		Level mLevel;
		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		SceneTheme mTheme;


		void resetRenderParams()
		{
			constexpr float BlockLen = 32;
			constexpr Vector2 BlockSize = Vector2(BlockLen, BlockLen);

			if (mLevel.mMaps.empty())
			{
				mWorldToScreen = RenderTransform2D(BlockSize, Vector2::Zero());
			}
			else
			{
				Vector2 screenPos = 0.5 * (Vector2(::Global::GetScreenSize()) - BlockSize.mul(Vector2(mLevel.mMaps[0].getBoundSize())));
				if (mLevel.mMaps.size() == 1)
				{
					screenPos = 0.5 * (Vector2(::Global::GetScreenSize()) - BlockSize.mul(Vector2(mLevel.mMaps[0].getBoundSize())));
				}
				else
				{
					Math::TAABBox< Vector2 > aabb;
					aabb.invalidate();

					for (auto& map : mLevel.mMaps)
					{
						aabb.addPoint(map.mPos);
						aabb.addPoint(map.mPos + Vector2(map.getBoundSize()));
					}

					screenPos = 0.5 * (Vector2(::Global::GetScreenSize()) - BlockSize.mul(aabb.getSize()));
				}

				mWorldToScreen = RenderTransform2D(BlockSize, screenPos);
			}

			mScreenToWorld = mWorldToScreen.inverse();
		}
		bool bPiecesOrderDirty;
		TArray< Piece* > mSortedPieces;
		int mClickFrame;

		virtual void initializeGame()
		{
			resetRenderParams();
			mClickFrame = 0;
			refreshPieceList();
		}

		void refreshPieceList()
		{
			mSortedPieces.clear();
			for (int index = 0; index < mLevel.mPieces.size(); ++index)
			{
				Piece* piece = mLevel.mPieces[index].get();
				piece->index = index;
				mSortedPieces.push_back(piece);
			}
			bPiecesOrderDirty = true;
		}

		void validatePiecesOrder()
		{
			if (bPiecesOrderDirty)
			{
				bPiecesOrderDirty = false;
				std::sort(mSortedPieces.begin(), mSortedPieces.end(),
					[](Piece* lhs, Piece* rhs) -> bool
					{
						if (lhs->isLocked() == rhs->isLocked())
							return lhs->clickFrame < rhs->clickFrame;
						return lhs->isLocked();
					}
				);
			}
		}


		Piece* getPiece(Vector2 const& pos, Vector2& outHitLocalPos)
		{
			validatePiecesOrder();
			for (int index = mSortedPieces.size() - 1; index >= 0; --index)
			{
				Piece* piece = mSortedPieces[index];
				if (piece->hitTest(pos, outHitLocalPos))
					return piece;
			}
			return nullptr;
		}

		virtual void notifyPieceLocked()
		{


		}
	};

	struct ChapterEdit
	{
		std::string name;
		TArray< std::string > levels;
	};

	struct ChapterDesc
	{
		std::string name;
	};

	struct EditPieceShape
	{
		PieceShape* ptr;
		int rotation;

		PieceShapeDesc desc;

		bool bMarkSave;
		int  usageCount;
	};

	extern class Editor* GEditor;
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

		MsgReply onMouse(MouseMsg const& msg, Vector2 const& worldPos , Piece* piece)
		{
			if ( msg.onLeftDown() )
			{
				if (piece == nullptr)
				{
					for (int i = 0; i < mGame->mLevel.mMaps.size(); ++i)
					{
						MarkMap& map = mGame->mLevel.mMaps[i];
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
			if (mGame->mLevel.mMaps.isValidIndex(id))
			{
				mGame->mLevel.mMaps[id].resize(x, y);
				mGame->resetRenderParams();
			}
		}

		void setMapPos(int id, float x, float y)
		{
			if (mGame->mLevel.mMaps.isValidIndex(id))
			{
				mGame->mLevel.mMaps[id].mPos.setValue(x,y);
				mGame->resetRenderParams();
			}
		}

		void addMap(int x, int y)
		{
			MarkMap newMap;
			newMap.resize(x, y);
			newMap.mData.fillValue(0);
			mGame->mLevel.mMaps.push_back(std::move(newMap));
			mGame->resetRenderParams();
		}

		void removeMap(int id)
		{
			if (mGame->mLevel.mMaps.isValidIndex(id))
			{
				mGame->mLevel.mMaps.erase(mGame->mLevel.mMaps.begin() + id);
				mGame->resetRenderParams();
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


		static void Draw(RHIGraphics2D& g, SceneTheme& theme , EditPieceShape const& editShape)
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

		template< class OP >
		void serializeShapeLibrary(OP& op)
		{
			if (OP::IsSaving)
			{
				int num = std::count_if(mPieceShapeLibrary.begin(), mPieceShapeLibrary.end(), [](auto const& value) {  return value.bMarkSave; });
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

		bool      mbEnabled = false;
		GameData* mGame;
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
		void refreshShapeList()
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

		void onRender()
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
				g.drawRect(-Vector2(border, border) , boundSize + 2 * Vector2(border, border));
				Editor::Draw(g, GEditor->mGame->mTheme, *shapeInfo.editShape);
				g.popXForm();

				Vector2 pos = shapeInfo.localToFrame.transformPosition(Vector2(0, boundSize.y));

				g.drawText(pos, InlineString<>::Make("%d", shapeInfo.editShape->usageCount) );

			}

			g.popXForm();
		}

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

		void onRender()
		{
			BaseClass::onRender();

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			Vec2i screenPos = getWorldPos();
			g.pushXForm();
			g.translateXForm(screenPos.x, screenPos.y);
			g.transformXForm(mLocalToFrame, true);
			Editor::Draw(g, GEditor->mGame->mTheme, *mShape);
			g.popXForm();
		}

		MsgReply onMouseMsg(MouseMsg const& msg)
		{
			if ( msg.onLeftDown() )
			{
				Vec2i framePos = msg.getPos() - getWorldPos();
				Vec2i localPos = mLocalToFrame.transformInvPosition(framePos);
				Vec2i boundSize = mShape->desc.getBoundSize();
				if (IsInBound(localPos, boundSize))
				{
					mShape->desc.toggleValue(localPos);
					if (mShape->ptr)
					{
						mShape->ptr->importDesc(mShape->desc, GEditor->mGame->mLevel.bAllowMirrorOp);
					}
					return MsgReply::Handled();
				}
			}

			return BaseClass::onMouseMsg(msg);
		}

		RenderTransform2D mLocalToFrame;
		EditPieceShape* mShape;
	};

	class LevelStage : public StageBase
					 , public GameData
					 , public IGameRenderSetup
	{
		using BaseClass = StageBase;

		enum 
		{
			UI_CmdCtrl = BaseClass::NEXT_UI_ID,
			NEXT_UI_ID,
		};

		Editor*  mEditor = nullptr;

		bool isEditEnabled() const { return mEditor && mEditor->mbEnabled; }

		std::unique_ptr< Solver > mSolver;

		virtual void initializeGame()
		{
			GameData::initializeGame();
			mSolver.release();

			if (mEditor)
			{
				mEditor->notifyLevelChanged();
			}
		}

		bool onInit() override;

		void loadLevel(char const* name);
		void loadTestLevel(int index);
		void runScript(int index);

		void onEnd() override
		{
			if (mEditor)
			{
				mEditor->cleanup();
				delete mEditor;
				mEditor = nullptr;
			}

			auto& console = ConsoleSystem::Get();
			console.unregisterAllCommandsByObject(this);

			BaseClass::onEnd();
		}

		ConsoleCmdTextCtrl* mCmdTextCtrl = nullptr;
		void restart();
		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			float dt = time / 1000.0;

			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			mTweener.update(dt);
			updateFrame(frame);
		}

		void onRender(float dFrame) override;
		void solveLevel(int option, char const* params);

		Piece* selectedPiece = nullptr;
		bool bStartDragging = false;
		Vector2 lastHitLocalPos;
		void drawPiece(RHIGraphics2D& g, Piece const& piece, bool bSelected);

		void updatePieceClickFrame(Piece& piece)
		{
			mClickFrame += 1;
			piece.clickFrame = mClickFrame;
			bPiecesOrderDirty = true;
		}

		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;

		void toggleEditor();



		bool onWidgetEvent(int event, int id, GWidget* ui) override;
	public:
		ERenderSystem getDefaultRenderSystem() override;
		bool setupRenderResource(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;


		Tween::GroupTweener< float > mTweener;

	};

}