#include "Stage/TestStageHeader.h"

#include "SBlocksCore.h"
#include "SBlocksSolver.h"

#include "GameRenderSetup.h"
#include "ConsoleSystem.h"

#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"


namespace SBlocks
{
	Vector2 DebugPos;


	struct GameData
	{

		Level mLevel;
		RenderTransform2D mLocalToWorld;
		RenderTransform2D mWorldToLocal;

		void resetRenderParams()
		{
			constexpr float BlockLen = 32;
			constexpr Vector2 BlockSize = Vector2(BlockLen, BlockLen);

			Vector2 worldPos = 0.5 * (Vector2(::Global::GetScreenSize()) - BlockSize.mul(Vector2(mLevel.mMap.getBoundSize())));
			mLocalToWorld = RenderTransform2D(BlockSize, worldPos);
			mWorldToLocal = mLocalToWorld.inverse();
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
						if (lhs->bLocked == rhs->bLocked)
							return lhs->clickFrame < rhs->clickFrame;
						return lhs->bLocked;
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

	struct LevelTheme
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

		LevelTheme()
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



	class Editor
	{
	public:

		void init()
		{
			registerCommand();
		}

		void cleanup()
		{
			unregisterCommand();
		}

		void startEdit()
		{
			LogMsg("Edit Start");

			mGame->mLevel.unlockAllPiece();
		}

		void endEdit()
		{
			LogMsg("Edit End");

		}


		bool onMouse(MouseMsg const& msg, Vector2 const& lPos , Piece* piece)
		{
			if ( msg.onLeftDown() )
			{
				if (piece == nullptr)
				{
					Vec2i mapPos;
					mapPos.x = lPos.x;
					mapPos.y = lPos.y;
					if (mGame->mLevel.mMap.isInBound(mapPos))
					{
						mGame->mLevel.mMap.toggleDataType(mapPos);
						return false;
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

		void registerCommand()
		{
			auto& console = ConsoleSystem::Get();
#define REGISTER_COM( NAME , FUNC )\
			console.registerCommand("SBlocks."NAME, &Editor::FUNC, this)

			REGISTER_COM("SetMapSize", setMapSize);
			REGISTER_COM("Save", saveLevel);

			REGISTER_COM("AddPiece", addPiece);
			//REGISTER_COM("NewShape, ")

#undef REGISTER_COM
		}

		void unregisterCommand()
		{
			auto& console = ConsoleSystem::Get();
			console.unregisterAllCommandsByObject(this);
		}

		void setMapSize(int x, int y)
		{
			mGame->mLevel.mMap.resize(x, y);
			mGame->resetRenderParams();
		}
		void addPiece(int id)
		{
			if (0 <= id && id < mPieceShapeLibrary.size())
			{
				EditPieceShape& editShape = mPieceShapeLibrary[id];
				if (editShape.ptr == nullptr)
				{
					editShape.ptr = mGame->mLevel.createPieceShape(editShape.desc);
				}

				mGame->mLevel.createPiece(*editShape.ptr);
				mGame->refreshPieceList();
			}
		}
		void newEditPieceShape(int sizeX, int sizeY)
		{
			EditPieceShape shape;
			shape.ptr = nullptr;
			shape.desc.bUseCustomPivot = false;
			shape.desc.customPivot = 0.5 * Vector2(sizeX, sizeY);
			shape.desc.sizeX = sizeX;
			shape.desc.data.resize(FBitGird::GetDataSizeX(sizeX) * sizeY, 1);
			mPieceShapeLibrary.push_back(shape);
		}

		void saveLevel(char const* name);

		struct EditPiece
		{
			Vector2 pos;
			DirType dir;
			bool    bLock;
		};

		struct EditPieceShape
		{
			PieceShape* ptr;
			PieceShapeDesc desc;
		};

		static void Draw(RHIGraphics2D& g, PieceShapeDesc const& desc)
		{
			RenderUtility::SetPen(g, EColor::Black);

			uint32 dataSizeX = FBitGird::GetDataSizeX(desc.sizeX);
			uint32 sizeY = (desc.data.size() + dataSizeX - 1) / dataSizeX;
			for (uint32 y = 0; y < sizeY; ++y)
			{
				for (uint32 x = 0; x < desc.sizeX; ++x)
				{
					if (FBitGird::Read(desc.data, dataSizeX, x, y))
					{
						RenderUtility::SetBrush(g, EColor::Gray);
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
		}


		std::vector< EditPieceShape > mPieceShapeLibrary;
		GameData* mGame;
	};



	class ShapeEditPanel : public GPanel
	{
	public:


		Editor::EditPieceShape* mShape;
	};

	class TestStage : public StageBase
					, public GameData
					, public IGameRenderSetup
	{
		using BaseClass = StageBase;

		Editor*  mEditor = nullptr;
		bool     bEditEnabled;

		std::unique_ptr< Solver > mSolver;
		LevelTheme mTheme;

		virtual void initializeGame()
		{
			GameData::initializeGame();
			mSolver.release();
		}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();
			restart();

			auto& console = ConsoleSystem::Get();
#define REGISTER_COM( NAME , FUNC )\
			console.registerCommand("SBlocks."NAME, &TestStage::FUNC, this)
			REGISTER_COM("Solve", solveLevel);
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

		void solveLevel();


		Piece* selectedPiece = nullptr;
		bool bStartDragging = false;
		Vector2 lastHitLocalPos;
		void drawPiece(RHIGraphics2D& g, Piece const& piece, bool bSelected);

		bool onMouse(MouseMsg const& msg) override
		{
			Vector2 lPos = mWorldToLocal.transformPosition(msg.getPos());
			Vector2 hitLocalPos;
			Piece* piece = getPiece(lPos, hitLocalPos);

			if (bEditEnabled)
			{
				if (!mEditor->onMouse(msg, lPos, piece))
					return false;
			}


			if (msg.onLeftDown())
			{
				if (selectedPiece != piece)
				{
					mClickFrame += 1;
					piece->clickFrame = mClickFrame;
					bPiecesOrderDirty = true;
				}
				selectedPiece = piece;
				if (selectedPiece)
				{
					lastHitLocalPos = hitLocalPos;
					bStartDragging = true;
					if (selectedPiece->bLocked)
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
					if (piece->bLocked)
					{
						mLevel.unlockPiece(*piece);
						bPiecesOrderDirty = true;
					}
					class PieceAngle
					{
					public:
						using DataType = Piece&;
						using ValueType = float;
						void operator()(DataType& data, ValueType const& value)			
						{ 
							data.angle = value;
							data.updateTransform();
						}
					};
					piece->dir += 1;
					mTweener.tween< Easing::IOSine, PieceAngle >(*piece, (int(piece->dir) - 1) * Math::PI / 2, (int(piece->dir)) * Math::PI / 2, 0.1, 0);
					//piece->angle = piece->dir * Math::PI / 2;
					piece->updateTransform();

					//mLevel.tryLockPiece(*piece);
				}
			}
			if (msg.onMoving())
			{
				if (bStartDragging)
				{
					Vector2 pinPos = selectedPiece->xform.transformPosition(lastHitLocalPos);
					Vector2 offset = lPos - pinPos;

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
						mEditor->init();
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