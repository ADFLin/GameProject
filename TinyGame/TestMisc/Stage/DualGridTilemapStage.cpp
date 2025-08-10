#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "Math/Math2D.h"
#include "ProfileSystem.h"
#include "RHI/RHIUtility.h"

#include "DataStructure/Grid2D.h"

using namespace Render;


namespace ETileId
{
	enum Type
	{
		Dirt,
		Grass,
	};
}

class DualGridTilemapStage : public StageBase
	                       , public IGameRenderSetup
{
public:
	using BaseClass = StageBase;


	struct Tile
	{
		int id;
	};


	struct RenderTile
	{
		Vector2 texPos;
	};

	int mBorderTileId = ETileId::Dirt;
	TGrid2D<Tile> mTilemap;
	TGrid2D<RenderTile> mRenderTilemap;

	int getTileId(Vec2i const& pos)
	{
		if (mTilemap.checkRange(pos))
			return mTilemap(pos).id;

		return mBorderTileId;
	}

	void setTile(Vec2i const& pos, int id)
	{
		mTilemap(pos).id = id;

		static Vec2i const UpdateOffset[] = { Vec2i(0,0), Vec2i(1,0), Vec2i(0,1), Vec2i(1,1), };
		for (int i = 0; i < ARRAY_SIZE(UpdateOffset); ++i)
		{
			updateRenderTile(pos + UpdateOffset[i]);
		}
	}

	void updateRenderTile(Vec2i const& pos)
	{
		static Vec2i const DirOffset[] = { Vec2i(0,0), Vec2i(-1, 0), Vec2i(0,-1), Vec2i(-1,-1), };
		static Vec2i const TexturePosMap[] =
		{
			//00         00          00          00
			//00         01          10          11
			Vec2i(0, 3), Vec2i(1, 3),Vec2i(0, 0),Vec2i(3, 0),
			//01         01          01          01
			//00         01          10          11
			Vec2i(0, 2), Vec2i(1, 0),Vec2i(2, 3),Vec2i(1, 1),
			//10         10          10          10
			//00         01          10          11
			Vec2i(3, 3), Vec2i(0, 1),Vec2i(3, 2),Vec2i(2, 0),
			//11         11          11          11
			//00         01          10          11
			Vec2i(1, 2), Vec2i(2, 2),Vec2i(3, 1),Vec2i(2, 1),
		};

		uint indexMap = 0;
		for (uint dir = 0; dir < ARRAY_SIZE(DirOffset); ++dir)
		{
			int tileId = getTileId(pos + DirOffset[dir]);
			if (tileId == ETileId::Grass)
				indexMap |= BIT(dir);
		}
		mRenderTilemap(pos).texPos = Vector2(TexturePosMap[indexMap]) / 4.0f;
	}


	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		frame->addCheckBox("Tile Grid", bDrawTileGridLine);
		frame->addCheckBox("RenderTile Grid", bDrawRenderTileGridLine);

		restart();
		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		Tile tileInit;
		tileInit.id = ETileId::Dirt;
		mTilemap.resize(18, 18);
		mTilemap.fillValue(tileInit);
		mRenderTilemap.resize(mTilemap.getSizeX() + 1, mTilemap.getSizeY() + 1);
		for (int j = 0; j < mRenderTilemap.getSizeY(); ++j)
		{
			for (int i = 0; i < mRenderTilemap.getSizeX(); ++i)
			{
				updateRenderTile(Vec2i(i, j));
			}
		}

		setTile(Vec2i(0, 0), ETileId::Grass);
		setTile(Vec2i(1, 1), ETileId::Grass);
		mView.scale = 30.0f;
		mView.worldToScreen = RenderTransform2D::LookAt(::Global::GetScreenSize(), Vector2(9, 9), Vector2(0, 1), mView.scale);
		mView.screenToWorld = mView.worldToScreen.inverse();
	}

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);
	}

	struct View
	{
		float scale = 80;
		RenderTransform2D worldToScreen;
		RenderTransform2D screenToWorld;
	};

	View mView;
	bool bDrawTileGridLine = false;
	bool bDrawRenderTileGridLine = false;

	void onRender(float dFrame) override
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);


		g.beginRender();

		g.pushXForm();
		g.transformXForm(mView.worldToScreen, false);

		RenderUtility::SetBrush(g, EColor::White);

		g.pushXForm();
		g.translateXForm(-0.5, -0.5);
		g.setTexture(*mTexTile);
		g.setSampler(TStaticSamplerState<ESampler::Bilinear, ESampler::Wrap, ESampler::Wrap>::GetRHI());

		struct Range
		{
			int min;
			int max;
		};

		for (int j = 0; j < mRenderTilemap.getSizeY(); ++j)
		{
			for (int i = 0; i < mRenderTilemap.getSizeX(); ++i)
			{
				auto const& tile = mRenderTilemap(i, j);
				g.drawTexture(Vector2(i,j), Vector2(1,1), tile.texPos, Vector2(0.25, 0.25));
			}
		}
		if (bDrawRenderTileGridLine)
		{
			RenderUtility::SetPen(g, EColor::Black);
			drawGridLines(g, Vector2::Zero(), 1, mRenderTilemap.getSize());
		}
		g.popXForm();

		if (bDrawTileGridLine)
		{
			RenderUtility::SetPen(g, EColor::Red);
			drawGridLines(g, Vector2::Zero(), 1, mTilemap.getSize());
		}

		g.popXForm();
		g.endRender();
	}

	void drawGridLines(RHIGraphics2D& g, Vector2 const& pos, float gridLen, Vec2i const& gridSize)
	{
		g.pushXForm();
		g.translateXForm(pos.x, pos.y);
		{
			float len = gridSize.x * gridLen;
			for (int i = 0; i <= gridSize.y; ++i)
			{
				float offset = i * gridLen;
				g.drawLine(Vector2(0, offset), Vector2(len, offset));
			}
		}

		{
			float len = gridSize.y * gridLen;
			for (int i = 0; i <= gridSize.x; ++i)
			{
				float offset = i * gridLen;
				g.drawLine(Vector2(offset, 0), Vector2(offset, len));
			}
		}
		g.popXForm();
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		Vector2 worldPos = mView.screenToWorld.transformPosition(msg.getPos());
		Vec2i tilePos;
		tilePos.x = Math::FloorToInt(worldPos.x);
		tilePos.y = Math::FloorToInt(worldPos.y);

		if (msg.onLeftDown())
		{
			if (mTilemap.checkRange(tilePos))
			{
				setTile(tilePos, ETileId::Grass);
			}
		}
		else if (msg.onRightDown())
		{
			if (mTilemap.checkRange(tilePos))
			{
				setTile(tilePos, ETileId::Dirt);
			}
		}
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
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

	RHITexture2DRef mTexTile;

	bool setupRenderResource(ERenderSystem systemName) override
	{
		mTexTile = RHIUtility::LoadTexture2DFromFile("Texture/TilesDemo.png");

		return true;
	}


	void preShutdownRenderSystem(bool bReInit = false) override
	{
		mTexTile.release();
	}

protected:


};

REGISTER_STAGE_ENTRY("Dual Grid Tilemap", DualGridTilemapStage, EExecGroup::Dev, "Render");