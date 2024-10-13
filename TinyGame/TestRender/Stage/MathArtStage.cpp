#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Math/PrimitiveTest.h"
#include "RHI/DrawUtility.h"
#include "CurveBuilder/ColorMap.h"

using namespace Render;

class MathArtStage : public StageBase
                   , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:

	enum
	{
		UI_BlendMode = BaseClass::NEXT_UI_ID,


		NEXT_UI_ID,
	};

	MathArtStage():colorMap(1024)
	{

	}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

#if 1
		mNodes =
		{
			{ 8 , 1.1 },
			{ 23 , 0.88 },
			{ -12 , 1.3 },
			{ -27 , 0.72 },
			//{  5 , 0.25 },
		};
#else
		mNodes =
		{
			{ 1 , 1 },
		};
#endif
		
		colorMap.addPoint(0, Color3ub(0, 7, 100));
		colorMap.addPoint(640 / 4, Color3ub(32, 107, 203));
		colorMap.addPoint(1680 / 4, Color3ub(237, 255, 255));
		colorMap.addPoint(2570 / 4, Color3ub(255, 170, 0));
		colorMap.addPoint(3430 / 4, Color3ub(0, 2, 0));
		colorMap.setSmoothLine(true);
		colorMap.calcColorMap(true);

		auto frame = WidgetUtility::CreateDevFrame();
		auto choice = frame->addChoice("DebugDisplay Mode", UI_BlendMode);
		char const* ModeTextList[] =
		{
			"Opaque",
			"Add",
			"Translucent",
		};
		static_assert(ARRAY_SIZE(ModeTextList) == (int)EBlendMode::COUNT);
		for (int i = 0; i < (int)EBlendMode::COUNT; ++i)
		{
			choice->addItem(ModeTextList[i]);
		}
		choice->setSelection((int)mBlendMode);
		frame->addCheckBox("Pause", bPaused);
		frame->addButton("Restart", [this](int eventId, GWidget* widget)->bool
		{
			restart();
			return false;
		});

		restart();


		return true;
	}
	ColorMap colorMap;

	void onEnd() override
	{
		BaseClass::onEnd();
	}
	int GCD(int num1, int num2)
	{
		if (num2 == 0)
		{
			return num1;
		}

		return GCD(num2, num1%num2);
	}
	double fgcd(double a, double b)
	{
		if (a < b)
			return fgcd(b, a);

		// base case
		if (fabs(b) < 0.001)
			return a;

		else
			return (fgcd(b, a - floor(a / b) * b));
	}
	void restart() 
	{
		mIndexBufferWrite = 0;
		mRenderTime = 0;
		mPoints[0].resize(mNodes.size() + 1);
		updatePoints(0 , mPoints[0]);
		mPoints[1] = mPoints[0];
		bClearImageRequested = true;

		float totalLength = 0;

		int factor = 1;

		if (!mNodes.empty())
		{
			float gcd = Math::Abs(mNodes[0].speed);
			for (auto const& node : mNodes)
			{
				totalLength += node.length;
				float speed = Math::Abs(node.speed);
				gcd = fgcd(gcd, speed);
			}
			for (auto& node : mNodes)
			{
				node.length /= totalLength;
				node.length *= 10;
			}

			mRenderCycleTime = 1.0f / gcd;
		}
		else
		{
			mRenderCycleTime = 1;
		}

		mZoom = ::Global::GetScreenSize().y / ( 2.2 * 10 );
	}


	struct Node
	{
		float speed;
		float length;
	};

	TArray< Node > mNodes;
	int mIndexBufferWrite = 1;
	TArray<Vector2> mPoints[2];
	bool bPaused = false;
	bool bAdvanceFrameRequested = false;

	float mRenderTime = 0;
	float mRenderCycleTime;
	bool  bClearImageRequested = true;
	float mZoom = 1;
	long  mTimeAcc = 0;
	float mOpacity = 0.3;

	enum class EBlendMode
	{
		Opaque,
		Add,
		Translucent,

		COUNT,
	};
	EBlendMode mBlendMode = EBlendMode::Add;

	bool checkNeedRender()
	{
		if (bAdvanceFrameRequested)
		{
			bAdvanceFrameRequested = false;
			return true;
		}

		if (bPaused)
			return false;

		if (mRenderTime >= mRenderCycleTime)
			return true;

		return true;
	}

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);

		if (!checkNeedRender())
			return;

		mTimeAcc += long(deltaTime);
		float speed = 2.0f;
		while (mTimeAcc > gDefaultTickTime)
		{
			mTimeAcc -= gDefaultTickTime;
			float dt = Math::Min(speed / 1000.0f, mRenderCycleTime - mRenderTime);
			renderToImage(mRenderTime, dt);
			mRenderTime += dt;
		}	
	}

	void updatePoints(float time, TArray<Vector2>& outPoints)
	{
		outPoints[0] = Vector2(0, 0);
		for (int indexNode = 0; indexNode < mNodes.size(); ++indexNode)
		{
			Node& node = mNodes[indexNode];
			float angle = Math::PI * (2 * node.speed * time);
			float c, s;
			Math::SinCos(angle, s, c);
			outPoints[indexNode + 1] = outPoints[indexNode] + node.length * Vector2(s, c);
		}
	}

	void renderToImage(float time, float dt)
	{
		auto& commandList = RHICommandList::GetImmediateList();

		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		auto worldToView = RenderTransform2D::LookAt(::Global::GetScreenSize(), Vector2(0, 0), Vector2(0, 1), mZoom);

		RHISetFrameBuffer(commandList, mFrameBuffer);
		RHISetViewport(commandList, 0, 0, mImage->getSizeX(), mImage->getSizeY());

		if (bClearImageRequested)
		{
			bClearImageRequested = false;
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);
		}

		switch (mBlendMode)
		{
		case MathArtStage::EBlendMode::Opaque:
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			break;
		case MathArtStage::EBlendMode::Add:
			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
			break;
		case MathArtStage::EBlendMode::Translucent:
			RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
			break;
		}
		RHISetRasterizerState(commandList, GetStaticRasterizerState(ECullMode::None, EFillMode::Solid));
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

		RHISetFixedShaderPipelineState(commandList, worldToView.toMatrix4() * g.getBaseTransform(), LinearColor(1, 1, 1, 1));


		struct Vertex
		{
			Vector2 pos;
			LinearColor color;
		};

		auto GetBaseColor = [this](float time, float alpha)
		{
			float pos = time / mRenderCycleTime;

			Color3f color;
			colorMap.getColor(pos, color);
			return color;

			return 2 * Color3f(0.2, 0.2, 0.2);
		};

		auto GetColor = [this](Color3f const& color, float d)
		{
			float scale = Math::Pow((1.0 / (4 * d + 1.0)), 3.5);
			switch (mBlendMode)
			{
			case EBlendMode::Add:
				return LinearColor( mOpacity * scale * color, 1);
			case EBlendMode::Translucent:
				return LinearColor( scale * color, mOpacity);
			}
			return LinearColor(scale * color, 1);
		};

		int stepCount = Math::CeilToInt( dt * 4000 );

		TArray< Vertex > vertices;
		vertices.reserve(6 * stepCount);
		for (int step = 0; step < stepCount; ++step)
		{
			float prevTime = time + (step) * dt / stepCount;
			float curTime = time + (step + 1) * dt / stepCount;
			updatePoints(curTime, mPoints[mIndexBufferWrite]);
			mIndexBufferWrite = 1 - mIndexBufferWrite;

			auto const& curPoints = mPoints[1 - mIndexBufferWrite];
			auto const& prevPoints = mPoints[mIndexBufferWrite];

			Vector2 pA1 = prevPoints[mNodes.size() - 1];
			Vector2 pA2 = prevPoints[mNodes.size()];
			Vector2 pB1 = curPoints[mNodes.size() - 1];
			Vector2 pB2 = curPoints[mNodes.size()];
			float d1 = Math::Distance(pA1, pB1);
			float d2 = Math::Distance(pA2, pB2);

			LinearColor cA1 = GetColor(GetBaseColor(prevTime,0), d1);
			LinearColor cA2 = GetColor(GetBaseColor(prevTime,1), d2);
			LinearColor cB1 = GetColor(GetBaseColor(curTime,0), d1);
			LinearColor cB2 = GetColor(GetBaseColor(curTime,1), d2);

			auto CheckAddTriangle = [&]( Vertex const* v)
			{
#if 0
				Vector2 d1 = v[1].pos - v[0].pos;
				Vector2 d2 = v[2].pos - v[0].pos;
				Vector2 d12 = v[1].pos - v[2].pos;
				float error = 1e-4;
				if (d1.length2() < error && d2.length2() < error && d12.length2() < error)
					return;
#endif
#if 0
				float area = Math::Abs(d1.cross(d2));
				if (area < 1e-2)
					return;
#endif
				vertices.append(v, v + 3);
			};

			Vector2 pos;
			if (Math::SegmentSegmentTest(pA1, pA2, pB1, pB2, pos))
			{
				float alphaA = Math::Distance(pA1, pos) / mNodes.back().length;
				float aplhaB = Math::Distance(pB1, pos) / mNodes.back().length;
				LinearColor c = 0.5 * (GetBaseColor(prevTime, alphaA) + GetBaseColor(curTime, aplhaB));
				Vertex v[] =
				{
					{pA1, cA1}, {pos, c}, {pB1, cB1},
					{pA2, cA2}, {pos, c}, {pB2, cB2},
				};
#if 1
				vertices.append(v, v + ARRAY_SIZE(v));
#else
				CheckAddTriangle(v);
				CheckAddTriangle(v + 3);
#endif
			}
			else
			{

				Vertex v[] =
				{
					{pA1, cA1}, {pA2, cA2}, {pB2, cB2},
					{pA1, cA1}, {pB2, cB2}, {pB1, cB1},
				};
#if 1
				vertices.append(v, v + ARRAY_SIZE(v));
#else
				CheckAddTriangle(v);
				CheckAddTriangle(v + 3);
#endif
			}
		}

		TRenderRT< RTVF_XY_CA >::Draw(commandList, EPrimitive::TriangleList, vertices.data(), vertices.size());
	}

	void onRender(float dFrame) override
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vector2 screenSize = ::Global::GetScreenSize();
		auto& commandList = g.getCommandList();

		auto const& curPoints = mPoints[1 - mIndexBufferWrite];
		auto const& prevPoints = mPoints[mIndexBufferWrite];
		auto worldToView = RenderTransform2D::LookAt(screenSize, Vector2(0, 0), Vector2(0, 1), mZoom);

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0,0,0,1), 1 );
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
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
			g.drawCircle(curPoints[indexNode] , 0.15);
		}
		g.popXForm();
		g.drawTextF(Vector2(10, 10), "Time = %f", mRenderTime);
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
			case EKeyCode::X: bAdvanceFrameRequested = true; break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch (id)
		{
		case UI_BlendMode:
			{
				mBlendMode = (EBlendMode)ui->cast<GChoice>()->getSelection();
			}
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
		mImage = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA32F, 2 * screenSize.x, 2 * screenSize.y).AddFlags(TCF_RenderTarget));

		mFrameBuffer = RHICreateFrameBuffer();
		mFrameBuffer->addTexture(*mImage);

		return true;
	}


	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.screenWidth = 1024;
		systemConfigs.screenHeight = 768;
	}

protected:
};

REGISTER_STAGE_ENTRY("Math Art", MathArtStage, EExecGroup::MiscTest, 1, "Render|Test");