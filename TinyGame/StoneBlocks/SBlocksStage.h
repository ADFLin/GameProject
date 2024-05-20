#include "Stage/TestStageHeader.h"

#include "SBlocksLevel.h"
#include "SBlocksEditor.h"
#include "SBlocksSolver.h"

#include "GameRenderSetup.h"
#include "ConsoleSystem.h"

#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"

#include "GameWidget.h"

#include <algorithm>

class ConsoleCmdTextCtrl;

namespace SBlocks
{
	Vector2 DebugPos;

	struct ChapterEdit
	{
		std::string name;
		TArray< std::string > levels;
	};

	struct ChapterDesc
	{
		std::string name;
	};


	class LevelStage : public StageBase
					 , public IGameRenderSetup
		             , public EditorEventHandler
	{
		using BaseClass = StageBase;

		enum 
		{
			UI_CmdCtrl = BaseClass::NEXT_UI_ID,
			NEXT_UI_ID,
		};

		GameLevel mLevel;
		Editor*   mEditor = nullptr;

		bool isEditEnabled() const { return mEditor && mEditor->mbEnabled; }

		std::unique_ptr< Solver > mSolver;

		void initializeGame()
		{
			mLevel.initialize();

			mSolver.reset();
			mClickFrame = 0;

			if (mEditor)
			{
				mEditor->notifyLevelChanged();
			}

			mMaxBound = Vec2i(0, 0);
			for (auto const& shape : mLevel.mShapes)
			{
				for (int i = 0; i < shape->getDifferentShapeNum(); ++i)
				{
					auto const& shapeData = shape->getDataByIndex(i);
					if (mMaxBound.x < shapeData.boundSize.x)
						mMaxBound.x = shapeData.boundSize.x;
					if (mMaxBound.y < shapeData.boundSize.y)
						mMaxBound.y = shapeData.boundSize.y;
				}
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
		bool bDraggingPiece = false;
		Vector2 lastHitLocalPos;
		Vec2i mMaxBound;

		void drawPiece(RHIGraphics2D& g, Piece const& piece, bool bSelected);

		void drawShapeDebug(RHIGraphics2D& g);
		
		int mClickFrame;

		void updatePieceClickFrame(Piece& piece)
		{
			mClickFrame += 1;
			piece.clickFrame = mClickFrame;
			mLevel.bPiecesOrderDirty = true;
		}

		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;

		void toggleEditor();



		bool onWidgetEvent(int event, int id, GWidget* ui) override;
	public:
		ERenderSystem getDefaultRenderSystem() override;
		bool setupRenderResource(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit) override;


		Tween::GroupTweener< float > mTweener;


		void onNewLevel() override;

	};

}