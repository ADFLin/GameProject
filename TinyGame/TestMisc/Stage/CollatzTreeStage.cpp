#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"
#include "RHI/ShaderProgram.h"
#include "FileSystem.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

using namespace Render;

class CollatzTreeStage : public StageBase
	                   , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	CollatzTreeStage() {}

	struct Node
	{
		int value;
		int parent;
		int level;
	};

	// f(n) = n / 2     ( n % 2 == 0 )
	//      = 3 * n + 1 ( n % 2 == 1 )


	TArray<Node> mNodes;

	void generate(int startValue, int maxLevel)
	{
		mNodes.clear();
		std::unordered_set<int> usedSet;

		auto AddNode = [&](int value, int parent , int level)
		{
			if (usedSet.find(value) != usedSet.end())
				return;

			usedSet.insert(value);
			mNodes.push_back({ value, parent, level });
		};

		auto AddChildren = [&](int indexNode, Node const& node)
		{
			int level = node.level;
			int value = node.value;

			AddNode(2 * value, indexNode, level + 1);
			if (value - 1 > 0 && (value - 1) % 3 == 0)
			{
				AddNode((value - 1) / 3, indexNode, level + 1);
			}
		};

		AddNode(startValue, INDEX_NONE, 0);

		int indexNode = 0;
		while (indexNode < mNodes.size())
		{
			Node& node = mNodes[indexNode];
			if ( node.level <= maxLevel )	
			{
				AddChildren(indexNode, node);
			}
			++indexNode;
		}
	}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		FWidgetProperty::Bind(frame->addSlider("Width"), mWidth, 0.02, 1);
		restart();
		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		generate(1, 30);
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

	float   mWidth = 0.05f;

	void onRender(float dFrame) override
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);

		//RenderTransform2D worldToScreen = RenderTransform2D::LookAt(screenSize, Vector2(0, 0), Vector2(0, 1), float(screenSize.x / len));

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();
		g.setPen(Color3f(1, 0, 0));
		auto GetNodePos = [](Node const& node)
		{
			return Vector2(100 * Math::LogX(10,  node.value), 20 * node.level);
		};
		for (int index = 1; index < mNodes.size(); ++index)
		{
			Node& node = mNodes[index];
			Node& parentNode = mNodes[node.parent];
			g.drawLine(GetNodePos(node), GetNodePos(parentNode));
		}
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

	bool setupRenderResource(ERenderSystem systemName) override
	{
		return true;
	}


	void preShutdownRenderSystem(bool bReInit = false) override
	{

	}


	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.screenWidth = 1280;
		systemConfigs.screenHeight = 1024;
	}

protected:
};

REGISTER_STAGE_ENTRY("Collatz Tree", CollatzTreeStage, EExecGroup::MiscTest);