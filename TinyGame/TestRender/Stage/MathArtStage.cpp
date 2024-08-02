#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Math/PrimitiveTest.h"
#include "RHI/DrawUtility.h"

using namespace Render;

class MathArtStage : public StageBase
                   , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	MathArtStage() {}


	struct Node
	{
		float speed;
		float length;
	};

	TArray< Node > mNodes;
	void update(float time, TArray<Vector2>& outPoints)
	{
		outPoints[0] = Vector2(0,0);
		for (int indexNode = 0; indexNode < mNodes.size(); ++indexNode)
		{
			Node& node = mNodes[indexNode];
			float angle = Math::PI * (2 * node.speed * time - 0.5);
			float c , s;
			Math::SinCos(angle, s, c);
			outPoints[indexNode + 1] = outPoints[indexNode] + node.length * Vector2(c, s);
		}
	}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

#if 1
		mNodes =
		{
			{ -3 , 2 },
			{  5 , 1 },
		};
#else
		mNodes =
		{
			{ 1 , 1 },
		};
#endif

		restart();

		Vector2 outPos;
		bool bOk = Math::SegmentSegmentTest(Vector2(0,0), Vector2(100,100), Vector2(100,0), Vector2(0,100), outPos);
		return true;
	}

	int mIndexBufferWrite = 1;
	TArray<Vector2> mPoints[2];
	bool bPaused = true;
	bool bRequestAdvanceFrame = false;

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		mIndexBufferWrite = 0;
		mTime = 0;
		mPoints[0].resize(mNodes.size() + 1);
		update(0 , mPoints[0]);
		mPoints[1] = mPoints[0];
		bClearImageRequested = true;
	}

	float mTime = 0;
	bool  bClearImageRequested = true;
	bool  bRenderToImage = false;
	void onUpdate(long time) override
	{
		if (bPaused)
		{
			if (!bRequestAdvanceFrame)
			{
				return;
			}
		}

		bRequestAdvanceFrame = false;

		mTime += float(time) / 1000.0f;
		update(mTime / 10, mPoints[mIndexBufferWrite]);
		mIndexBufferWrite = 1 - mIndexBufferWrite;
		bRenderToImage = true;
	}

	void onRender(float dFrame) override
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		auto& commandList = g.getCommandList();

		auto const& curPoints = mPoints[1 - mIndexBufferWrite];
		auto const& prevPoints = mPoints[mIndexBufferWrite];
		auto worldToView = RenderTransform2D::LookAt(::Global::GetScreenSize(), Vector2(0, 0), Vector2(0, 1), ::Global::GetScreenSize().x / 5.0f);


		bool bDrawDebug = false;
		Vector2 debugPos;
		if ( bRenderToImage )
		{
			bRenderToImage = false;

			RHISetFrameBuffer(commandList, mFrameBuffer);
			if (bClearImageRequested)
			{
				bClearImageRequested = false;
				RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);
			}

			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
			RHISetRasterizerState(commandList, GetStaticRasterizerState(ECullMode::None, EFillMode::Solid));
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());


			RHISetFixedShaderPipelineState(commandList, worldToView.toMatrix4() * g.getBaseTransform(), LinearColor(1, 1, 1, 1));

			Vector2 pA1 = prevPoints[mNodes.size() - 1];
			Vector2 pA2 = prevPoints[mNodes.size()];
			Vector2 pB1 = curPoints[mNodes.size() - 1];
			Vector2 pB2 = curPoints[mNodes.size()];

			Vector2 pos;

			struct Vertex
			{
				Vector2 pos;
				LinearColor color;
			};

			LinearColor color = LinearColor(0.2, 0.2, 0.2, 1);
			if (Math::SegmentSegmentTest(pA1, pA2, pB1, pB2, pos))
			{
#if 0
				bDrawDebug = true;
				debugPos = pos;
				color = LinearColor(0.5,0,0, 1);
#endif
				Vertex v[] =
				{
					{pA1, color}, {pos, color}, {pB1, color},
					{pA2, color}, {pos, color}, {pB2, color},
				};
				TRenderRT< RTVF_XY_CA >::Draw(commandList, EPrimitive::TriangleList, v, ARRAY_SIZE(v));
			}
			else
			{
				Vertex v[] =
				{
					{pA1, color},
					{pA2, color},
					{pB2, color},
					{pB1, color},
				};
				TRenderRT< RTVF_XY_CA >::Draw(commandList, EPrimitive::Quad, v, ARRAY_SIZE(v));
			}
		}

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0,0,0,1), 1 );

		ShaderHelper::Get().copyTextureToBuffer(commandList, *mImage);

		g.beginRender();
		g.pushXForm();
		g.transformXForm(worldToView, false);

		g.enablePen(true);
		g.setPenWidth(4);

		RenderUtility::SetPen(g, EColor::White);
		g.drawLine(curPoints[0], Vector2(0,0));
		for (int indexNode = 1; indexNode < curPoints.size(); ++indexNode)
		{
			g.drawLine(curPoints[indexNode], curPoints[indexNode- 1]);
		}
		g.setPenWidth(0);
		g.enablePen(false);
		RenderUtility::SetBrush(g, EColor::Red);
		for (int indexNode = 0; indexNode < curPoints.size(); ++indexNode)
		{
			g.drawCircle(curPoints[indexNode] , 0.1);
		}

		if (bDrawDebug)
		{
			RenderUtility::SetBrush(g, EColor::Yellow);
			g.drawCircle(debugPos, 0.1);
		}

		g.popXForm();
		g.endRender();
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::X: bRequestAdvanceFrame = true; break;
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

	RHIFrameBufferRef mFrameBuffer;
	RHITexture2DRef   mImage;
	bool setupRenderResource(ERenderSystem systemName) override
	{

		ShaderHelper::Get().init();

		auto screenSize = ::Global::GetScreenSize();
		mImage = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, screenSize.x, screenSize.y).AddFlags(TCF_RenderTarget));

		mFrameBuffer = RHICreateFrameBuffer();
		mFrameBuffer->addTexture(*mImage);

		return true;
	}

protected:
};

REGISTER_STAGE_ENTRY("Math Art", MathArtStage, EExecGroup::MiscTest, 1, "Render|Test");