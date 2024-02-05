#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"
#include "Math/Math2D.h"
#include "ProfileSystem.h"

using namespace Render;

class FractialRenderStage : public StageBase
	                      , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	FractialRenderStage() {}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();

		restart();

		FWidgetProperty::Bind(frame->addSlider("Scale"), mScale, 0, 1, 1, [this](float) { generateTree(); });
		FWidgetProperty::Bind(frame->addSlider("Angle"), mAngle, 0, 180, 1, [this](float) { generateTree(); });
		FWidgetProperty::Bind(frame->addSlider("Branch"), mNumBranch, 2, 10, [this](float) { generateTree(); });
		FWidgetProperty::Bind(frame->addSlider("Level"), mLevel, 0, 20, [this](float) { generateTree(); });
		FWidgetProperty::Bind(frame->addSlider("Rotation"), mViewport.rotation, 0, 360, [this](float) { updateView(); });
		FWidgetProperty::Bind(frame->addSlider("Zoom"), mViewport.zoom, 0.1, 10, [this](float) { updateView(); });

		return true;
	}

	float mScale = 0.5;
	float mAngle = 15.0f;
	int   mNumBranch = 4;
	int   mLevel = 10;

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		mViewport.screenSize = ::Global::GetScreenSize();
		mViewport.viewPos = Vector2(0, 100 + Math::Lerp(0, 100, mScale));
		mViewport.zoom = (mViewport.screenSize.x / 400.0f) * Math::Pow(mScale * 2, 2);
		updateView();

		generateTree();
	}

	void generateTree()
	{
		TIME_SCOPE("Generate Tree");
		mVertices.clear();
		mIndices.clear();
		TreeGen gen(mVertices, mIndices);
		gen.scale = mScale;
		gen.setRotateAngle(Math::DegToRad(mAngle));
		gen.setMaxLevel(mLevel);
		gen.numBranch = mNumBranch;
		gen.generate(Vector2(0, 0), Vector2(0, 100));
	}

	void tick() {}
	void updateFrame(int frame) {}

	void onUpdate(long time) override
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for (int i = 0; i < frame; ++i)
			tick();

		updateFrame(frame);
	}

	struct Viewport
	{
		Vec2i   screenSize;
		Vector2 viewPos;
		float   zoom;
		float   rotation = 0.0f;
		RenderTransform2D worldToScreen;
		RenderTransform2D screenToWorld;

		void updateTransform()
		{
			float c, s;
			Math::SinCos(Math::DegToRad(rotation), s, c);
			worldToScreen = RenderTransform2D::LookAt(screenSize, viewPos, Math::DegToRad(rotation) - Math::PI , zoom, true);
			screenToWorld = worldToScreen.inverse();
		}
	};

	Viewport mViewport;

	void updateView()
	{
		mViewport.updateTransform();
	}

	void onRender(float dFrame) override
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

		auto const& worldToScreen = mViewport.worldToScreen;

		Matrix4 projectionMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0, 1, -1));

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

		RHISetFixedShaderPipelineState(commandList, Matrix4::Translate(0, 100, 0) *  worldToScreen.toMatrix4() * projectionMatrix);
		DrawUtility::AixsLine(commandList, 1000);

		RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::One>::GetRHI());
		RHISetFixedShaderPipelineState(commandList, worldToScreen.toMatrix4() * projectionMatrix);
		TRenderRT< RTVF_XY_CA >::DrawIndexed(commandList, EPrimitive::LineList, mVertices.data(), mVertices.size(), mIndices.data(), mIndices.size());
	}

	struct Vertex
	{
		Vector2 pos;
		LinearColor color;
	};

	TArray< Vertex > mVertices;
	TArray< uint32 > mIndices;

	struct TreeGen
	{
		TreeGen(TArray<Vertex>& outVertices, TArray<uint32>& outIndices)
			:outVertices(outVertices)
			,outIndices(outIndices){}


		void generateOdd(Vector2 const& pos, Vector2 const& offset, int level, uint32 headIndex)
		{
			Vector2 nextPos = pos + offset;
			uint32 index = emitVertex(nextPos, getColor(level));
			outIndices.push_back(headIndex);
			outIndices.push_back(index);

			if (level < maxLevel)
			{
				++level;
				Vector2 newOffset = scale * offset;
				generateOdd(nextPos, newOffset, level, index);
				Vector2 offsetLeft = newOffset;
				Vector2 offsetRight = newOffset;
				for (int i = 0; i < numBranch / 2; ++i)
				{
					offsetLeft = rotation.mul(offsetLeft);
					offsetRight = rotation.mulInv(offsetRight);
					generateOdd(nextPos, offsetLeft, level, index);
					generateOdd(nextPos, offsetRight, level, index);
				}
			}
		}

		void generateEven(Vector2 const& pos, Vector2 const& offset, int level, uint32 headIndex)
		{
			Vector2 nextPos = pos + offset;
			uint32 index = emitVertex(nextPos, getColor(level));
			outIndices.push_back(headIndex);
			outIndices.push_back(index);

			if (level < maxLevel)
			{
				++level;
				Vector2 newOffset = scale * offset;	
				Vector2 offsetLeft = rotationHalf.mul(newOffset);
				Vector2 offsetRight = rotationHalf.mulInv(newOffset);
				for (int i = 0; i < numBranch / 2; ++i)
				{
					generateEven(nextPos, offsetLeft, level, index);
					generateEven(nextPos, offsetRight, level, index);
					offsetLeft = rotation.mul(offsetLeft);
					offsetRight = rotation.mulInv(offsetRight);
				}
			}
		}
		void generate2(Vector2 const& pos, Vector2 const& offset, int level, uint32 headIndex)
		{
			Vector2 nextPos = pos + offset;
			uint32 index = emitVertex(nextPos, getColor(level));
			outIndices.push_back(headIndex);
			outIndices.push_back(index);

			if (level < maxLevel)
			{
				++level;
				Vector2 newOffset = scale * offset;
				generate2(nextPos, rotationHalf.mul(newOffset), level, index);
				generate2(nextPos, rotationHalf.mulInv(newOffset), level, index);
			}
		}


		void generate(Vector2 const& pos, Vector2 const& offset)
		{
			uint32 index = emitVertex(pos, LinearColor::Black());
			switch (numBranch)
			{
			case 2: generate2(pos, offset, 0, index); break;
			default:
				if (numBranch % 2)
				{
					generateOdd(pos, offset, 0, index);
				}
				else
				{
					generateEven(pos, offset, 0, index);
				}
				break;
			}
		}

		TArray<LinearColor> CachedColorMap;
		void generateColorMap()
		{
			CachedColorMap.resize(maxLevel);
			for (int level = 0; level < maxLevel; ++level)
			{
				Vector3 hsv{ 360.0f * float(level) / 20 , 1 , 1 };
				CachedColorMap[level] = FColorConv::HSVToRGB(hsv);
			}
		}

		LinearColor const& getColor(int level)
		{
			return CachedColorMap[level];
		}

		uint32 emitVertex(Vector2 const& pos, LinearColor const& color)
		{
			uint32 result = outVertices.size();
			outVertices.push_back({ pos , color });
			return result;
		}

		void setMaxLevel(int level)
		{
			maxLevel = level;
			generateColorMap();
		}

		void setRotateAngle(float angle)
		{
			rotationHalf = Math::Rotation2D(angle / 2);
			rotation = Math::Rotation2D(angle);
		}

		int numBranch = 2;
		float scale = 0.5f;

	private:
		int maxLevel = 10;

		Math::Rotation2D rotationHalf = Math::Rotation2D(0);
		Math::Rotation2D rotation = Math::Rotation2D(0);
		TArray<uint32>& outIndices;
		TArray<Vertex>& outVertices;
	};


	bool    bStartDragging = false;
	Vector2 draggingPos;
	MsgReply onMouse(MouseMsg const& msg) override
	{
		if (msg.onLeftDown())
		{
			Vector2 screenPos = msg.getPos();
			draggingPos = mViewport.screenToWorld.transformPosition(screenPos);
			bStartDragging = true;
		}
		else if (msg.onLeftUp())
		{
			bStartDragging = false;
		}

		else if (msg.onMoving())
		{
			if (bStartDragging)
			{
				Vector2 screenPos = msg.getPos();
				Vector2 pos = mViewport.screenToWorld.transformPosition(screenPos);
				mViewport.viewPos -= (pos - draggingPos);
				updateView();
			}
		}

		if (bStartDragging)
			return MsgReply::Handled();

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

	ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::None;
	}

protected:
};

REGISTER_STAGE_ENTRY("Fractial Render", FractialRenderStage, EExecGroup::FeatureDev, "Render|Math");