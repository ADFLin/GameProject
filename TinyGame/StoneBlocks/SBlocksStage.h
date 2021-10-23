#include "Stage/TestStageHeader.h"

#include "SBlocksCore.h"
#include "SBlocksSolver.h"

#include "GameRenderSetup.h"
#include "ConsoleSystem.h"

#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include <algorithm>
#include "GameWidget.h"

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
		std::vector< Piece* > mSortedPieces;
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

		void vaildatePiecesOrder()
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
			vaildatePiecesOrder();
			for (int index = mSortedPieces.size() - 1; index >= 0; --index)
			{
				Piece* piece = mSortedPieces[index];
				if (piece->hitTest(pos, outHitLocalPos))
					return piece;
			}
			return nullptr;
		}
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

	class Editor
	{
	public:

		void initEdit()
		{
			loadShapeLibrary();
			registerGamePieces();
		}

		void cleanup()
		{

		}

		void startEdit();
		void endEdit();

		bool onMouse(MouseMsg const& msg, Vector2 const& worldPos , Piece* piece)
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
							map.toggleDataType(mapPos);
							return false;
						}
					}

				}
			}

			return true;
		}
		bool onKey(KeyMsg const& msg)
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{

				}

			}
			return true;
		}

		void registerCommand();
		void unregisterCommand();

		void runScript();

		void saveLevel(char const* name);
		void newLevel();


		void setMapSize(int id, int x, int y)
		{
			if (IsValidIndex(mGame->mLevel.mMaps, id))
			{
				mGame->mLevel.mMaps[id].resize(x, y);
				mGame->resetRenderParams();
			}
		}

		void setMapPos(int id, float x, float y)
		{
			if (IsValidIndex(mGame->mLevel.mMaps, id))
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
			if (IsValidIndex(mGame->mLevel.mMaps, id))
			{
				mGame->mLevel.mMaps.erase(mGame->mLevel.mMaps.begin() + id);
				mGame->resetRenderParams();
			}
		}

		void addPiece(int id);
		void removePiece(int id);

		void addEditPieceShape(int sizeX, int sizeY);

		void removeEditPieceShape(int id);

		void copyEditPieceShape(int id);

		class ShapeListPanel* mShapeLibraryPanel = nullptr;
		class ShapeEditPanel* mShapeEditPanel = nullptr;

		void openEditPieceShapeEditor(int id);

		void notifyLevelChanged();


		struct EditPiece
		{
			Vector2 pos;
			DirType dir;
			bool    bLock;
		};



		void registerGamePieces();

		EditPieceShape* registerEditShape(PieceShape* shape);


		static void Draw(RHIGraphics2D& g, SceneTheme& theme , PieceShapeDesc const& desc)
		{
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
			}
		}

		void saveShapeLibrary();

		void loadShapeLibrary();

		std::vector< EditPieceShape > mPieceShapeLibrary;

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
		std::vector< ShapeInfo > mShapeList;
		void refreshShapeList()
		{
			mShapeList.clear();

			Vector2 pos = { 10 , 10 };
			float scale = 20.0;

			for (auto& editShape : mEditor->mPieceShapeLibrary)
			{
				ShapeInfo shapeInfo;
				shapeInfo.editShape = &editShape;
				shapeInfo.localToFrame.setIdentity();
				shapeInfo.localToFrame.translateLocal(pos);
				shapeInfo.localToFrame.scaleLocal(Vector2(scale, scale));
				shapeInfo.FrameToLocal = shapeInfo.localToFrame.inverse();
				Vec2i boundSize = editShape.desc.getBoundSize();

				mShapeList.push_back(std::move(shapeInfo));
				pos.x += boundSize.x * scale + 5;
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

			for (auto const& shapeInfo : mShapeList)
			{
				g.pushXForm();
				g.transformXForm(shapeInfo.localToFrame, true);
				Editor::Draw(g, mEditor->mGame->mTheme, shapeInfo.editShape->desc);
				g.popXForm();
			}

			g.popXForm();
		}

		Editor* mEditor;
	};


	class ShapeEditPanel : public GFrame
	{
		using BaseClass = GFrame;
	public:
		ShapeEditPanel(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent = nullptr)
			:GFrame(id, pos, size, parent)
		{

		}

		void init()
		{
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
			Editor::Draw(g, editor->mGame->mTheme, mShape->desc);
			g.popXForm();
		}

		bool onMouseMsg(MouseMsg const& msg)
		{
			if ( msg.onLeftDown() )
			{
				Vec2i framePos = msg.getPos() - getWorldPos();

				Vec2i localPos = mLocalToFrame.inverse().transformPosition(framePos);

				Vec2i boundSize = mShape->desc.getBoundSize();
				if (IsInBound(localPos, boundSize))
				{
					mShape->desc.toggleValue(localPos);
					if (mShape->ptr)
					{
						mShape->ptr->importDesc(mShape->desc);
					}
					return false;
				}
			}

			return BaseClass::onMouseMsg(msg);
		}
		RenderTransform2D mLocalToFrame;
		Editor* editor;
		EditPieceShape* mShape;
	};

	class TestStage : public StageBase
					, public GameData
					, public IGameRenderSetup
	{
		using BaseClass = StageBase;

		Editor*  mEditor = nullptr;
		bool     bEditEnabled;

		std::unique_ptr< Solver > mSolver;

		virtual void initializeGame()
		{
			GameData::initializeGame();
			mSolver.release();

			if (mEditor)
			{
				mEditor->notifyLevelChanged();
				if (bEditEnabled)
				{


				}
			}
		}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();
			restart();

			auto& console = ConsoleSystem::Get();
#define REGISTER_COM( NAME , FUNC , ... )\
			console.registerCommand("SBlocks."NAME, &TestStage::FUNC, this , ##__VA_ARGS__ )
			REGISTER_COM("Solve", solveLevel, CVF_CAN_OMIT_ARGS);
			REGISTER_COM("Load", loadLevel);
#undef REGISTER_COM
			return true;
		}

		void loadLevel(char const* name);


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

		void solveLevel(int option);

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

		bool onMouse(MouseMsg const& msg) override
		{
			Vector2 worldPos = mScreenToWorld.transformPosition(msg.getPos());
			Vector2 hitLocalPos;
			Piece* piece = getPiece(worldPos, hitLocalPos);

			if (bEditEnabled)
			{
				if (!mEditor->onMouse(msg, worldPos, piece))
					return false;
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
					mLevel.tryLockPiece(*selectedPiece);
					bPiecesOrderDirty = true;
					selectedPiece = nullptr;
				}
			}
			if (msg.onRightDown())
			{
				if (piece)
				{
					if (piece->isLocked())
					{
						mLevel.unlockPiece(*piece);
						bPiecesOrderDirty = true;
					}

					updatePieceClickFrame(*piece);
					struct PieceAngle
					{
						using DataType = Piece&;
						using ValueType = float;
						void operator()(DataType& data, ValueType const& value)			
						{ 
							data.angle = value;
							data.updateTransform();
						}
					};
					mTweener.tween< Easing::IOBounce, PieceAngle >(*piece, int(piece->dir) * Math::PI / 2, (int(piece->dir) + 1) * Math::PI / 2, 0.1, 0);
					piece->dir += 1;
					//piece->angle = piece->dir * Math::PI / 2;
					piece->updateTransform();

					//mLevel.tryLockPiece(*piece);
				}
			}
			if (msg.onMoving())
			{
				if (bStartDragging)
				{
					Vector2 pinPos = selectedPiece->xFormRender.transformPosition(lastHitLocalPos);
					Vector2 offset = worldPos - pinPos;

					selectedPiece->pos += offset;
					selectedPiece->updateTransform();
				}
			}

			return BaseClass::onMouse(msg);
		}

		bool onKey(KeyMsg const& msg) override
		{
			if (!msg.isDown())
				return false;

			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::Num1: 
				bEditEnabled = !bEditEnabled;
				if ( bEditEnabled )
				{
					if (mEditor == nullptr)
					{
						mEditor = new Editor;
						mEditor->mGame = this;
						mEditor->initEdit();
					}
					mEditor->startEdit();
				}
				else
				{
					mEditor->endEdit();
				}
				break;
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
	public:
		ERenderSystem getDefaultRenderSystem() override;
		bool setupRenderSystem(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;

		Tween::GroupTweener< float > mTweener;

	};

}