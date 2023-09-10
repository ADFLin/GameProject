#include "ProfilerPanel.h"
#include "EditorUtils.h"
#include "RHI/RHICommand.h"
#include "ProfileSampleVisitor.h"

#include "ImGui/imgui_internal.h"
#include "InlineString.h"
#include "PlatformThread.h"

REGISTER_EDITOR_PANEL(ProfilerPanel, "Profiler", true, true);

Color3ub GColors[] =
{
	Color3ub(0 , 255 , 255),
	Color3ub(0 , 0 , 255),
	Color3ub(255 , 165 , 0),
	Color3ub(255 , 255 , 0),
	Color3ub(0 , 255 , 0),
	Color3ub(160 , 32 , 240),
	Color3ub(255 , 0 , 0),
	Color3ub(100 , 100 , 100),
	Color3ub(255 , 0 , 255),
	Color3ub(255 , 255 , 255),
};

class ProfileFrameVisualizeDraw : public ProfileNodeVisitorT< ProfileFrameVisualizeDraw >
{
public:
	ProfileFrameVisualizeDraw(RHIGraphics2D& g)
		:mGraphics2D(g)
	{

	}
	bool bShowAvg = false;
	bool bPause   = false;
	bool bGrouped = true;
	double mTotalTime;
	uint64 mTickStart;
	double mCurTime;
	double mScale;
	int    mLevel;
	TArray<float> mLevelTimePos;

	void drawGird()
	{
		mGraphics2D.setPen(Color3ub(180, 180, 180));
		for (float t = 0; t <= 120; t += 10)
		{
			Vector2 p1 = toRenderPos(t, 0);
			Vector2 p2 = p1 + Vector2(0, 200);
			mGraphics2D.drawLine(p1, p2);
		}

		float FPSList[] = { 60 , 30 , 15 };
		Color3ub colors [] = { Color3ub(0,255,0) , Color3ub(255,255,0) , Color3ub(255,0,0) };
		int index = 0;
		for (auto fps : FPSList)
		{
			float time = 1000.0f / fps;
			Vector2 p1 = toRenderPos(time, 0);
			Vector2 p2 = p1 + Vector2(0, 200);
			mGraphics2D.setPen(colors[index]);
			mGraphics2D.drawLine(p1, p2);
			++index;
		}
	}

	void onRoot(VisitContext& context)
	{
		mTotalTime = 1000.0 / 60;
		mTickStart = context.node->getFrameTimestamps()[0].start;
		mScale = mMaxLength / mTotalTime;
		mCurTime = 0;
		mLevel = 0;
		mMaxLevel = 0;
		mLevelTimePos.push_back(mCurTime);
		updateDisplayTime(context);
		mGraphics2D.setBrush(Color3ub(255,255,255));
		drawNode(context.node->getName(), (bGrouped) ? context.displayTimeAcc : mCurTime, context.displayTime, 0);
		++mLevel;
	}

	Vector2 toRenderPos(float time, float level)
	{
		return mBasePos + Vector2(mScale * time, level * offsetY);
	}


	void drawNode(char const* name, double timePos, double duration, int callCount)
	{
		Vector2 pos = mBasePos + Vector2(mScale * timePos, mLevel * offsetY);
		Vector2 size = Vector2(Math::Max<float>(mScale * duration, 1.5f), offsetY);

		mGraphics2D.enablePen(size.x > 1);
		mGraphics2D.drawRect(pos, size);

		if (size.x > 5)
		{
			InlineString<256> text;
			if (callCount)
			{
				text.format("%s (%d - %.2lf)", name, callCount, duration);
			}
			else
			{
				text.format("%s (%.2lf)", name, duration);
			}


			TextData textData;
			textData.pos = pos + Vector2(2, 0);
			textData.size = size;
			textData.str = text;
			mRenderTexts.push_back(textData);
		}
	};

	void onNode(VisitContext& context)
	{
		NodeStat& stat = updateDisplayTime(context);

		int color = stat.id % ARRAY_SIZE(GColors);
		SampleNode* node = context.node;

		mGraphics2D.setBrush(GColors[color]);
		if ( bGrouped )
		{
			drawNode(node->getName(), mCurTime + context.displayTimeAcc, context.displayTime, node->getFrameCalls());
		}
		else
		{
			for (auto const& timestamp : stat.timestamps)
			{
				drawNode(node->getName(), timestamp.start, timestamp.duration, 0);
			}
		}
	}

	void drawTexts(Vector2 const& clipPos)
	{
		for (auto const& textData : mRenderTexts)
		{
			Vector2 pos = textData.pos;
			Vector2 size = textData.size;
			pos.x = Math::Clamp(pos.x, clipPos.x, pos.x + size.x);
			size.x -= pos.x - textData.pos.x;
			mGraphics2D.drawText(pos, size, textData.str.c_str(), EHorizontalAlign::Left, EVerticalAlign::Center, true);
		}
	}

	struct TextData
	{
		Vector2 pos;
		Vector2 size;
		std::string str;
	};
	TArray< TextData > mRenderTexts;

	struct Timestamp
	{
		double start;
		double duration;
	};

	struct NodeStat
	{
		int id = INDEX_NONE;
		uint64 frameCount = 0;
		double displayTime = 0;
		double execAvg = 0;
		TArray< Timestamp > timestamps;
	};

	uint64 mFrameCount = 0;

	bool filterNode(SampleNode* node)
	{
		if (bPause)
		{
			NodeStat& stat = nodeStatMap[node];
			return stat.frameCount == mFrameCount;
		}
		else
		{
			return node->getLastFrame() + 1 == ProfileSystem::Get().getFrameCountSinceReset();
		}
	}

	NodeStat& updateDisplayTime(VisitContext &context)
	{
		SampleNode* node = context.node;
		NodeStat& stat = nodeStatMap[node];
		if (stat.id == INDEX_NONE)
		{
			stat.id = mNextId++;
			//stat.id = FNV1a::MakeStringHash<uint32>(node->getName());
		}

		stat.execAvg = 0.9 * stat.execAvg + 0.1 * node->getFrameExecTime();
		if (!bPause)
		{
			mFrameCount = ProfileSystem::Get().getFrameCountSinceReset();
			stat.frameCount = ProfileSystem::Get().getFrameCountSinceReset();
			stat.displayTime = ( bShowAvg && bGrouped ) ? stat.execAvg : node->getFrameExecTime();
			if (bGrouped == false)
			{
				stat.timestamps.resize(node->getFrameTimestamps().size());
				int index = 0;
				for (auto const& timestamp : node->getFrameTimestamps())
				{
					auto& displayTimestamp = stat.timestamps[index];
					displayTimestamp.start = double(timestamp.start - mTickStart) / Profile_GetTickRate();
					displayTimestamp.duration = double(timestamp.end - timestamp.start) / Profile_GetTickRate();
					++index;
				}
			}
		}
		context.displayTime = stat.displayTime;
		return stat;
	}

	bool onEnterChild(VisitContext const& context)
	{
		mLevelTimePos.push_back(mCurTime);
		mCurTime += context.displayTimeAcc;
		++mLevel;
		if (mLevel > mMaxLevel)
			mMaxLevel = mLevel;
		return true;
	}
	void onReturnParent(VisitContext const& context, VisitContext const& childContext)
	{
		SampleNode* node = context.node;
		
		NodeStat& stat = nodeStatMap[node];
		mCurTime = mLevelTimePos.back();
		mLevelTimePos.pop_back();

		--mLevel;
	}

	std::unordered_map< void*, NodeStat > nodeStatMap;
	int mNextId = 0;
	int mMaxLevel = 0;
	uint32 mThreadId;

	double mMaxLength = 600;
	double offsetY = 20;
	Vector2      mBasePos = Vector2::Zero();
	RHIGraphics2D& mGraphics2D;
};


void ProfilerPanel::render()
{
	PROFILE_ENTRY("Profiler.Render");
	static RenderData data;
	ImGui::Checkbox("Pause", &bPause);
	ImGui::SameLine();
	ImGui::Checkbox("Grouped", &bGrouped);
	if (bGrouped)
	{
		ImGui::SameLine();
		ImGui::Checkbox("ShowAvg", &bShowAvg);
	}
	ImGui::SameLine();
	ImGui::SliderFloat("Scale", &scale, 0.1, 100, "%g");

	ImGui::BeginChild("Visualize", ImVec2(0,0), false , ImGuiWindowFlags_HorizontalScrollbar);
	auto drawData = ImGui::GetDrawData();
	auto viewport = ImGui::GetWindowViewport();
	ImGuiWindow* window = ImGui::GetCurrentWindowRead();

	data.panel = this;
	data.clientPos = FImGuiConv::To(ImGui::GetCursorScreenPos() - ImGui::GetWindowViewport()->Pos);
	data.clientSize = FImGuiConv::To(window->ContentSize);
	data.windowPos = FImGuiConv::To(ImGui::GetWindowPos() - ImGui::GetWindowViewport()->Pos);
	data.windowSize = FImGuiConv::To(ImGui::GetWindowSize());
	data.viewportSize = FImGuiConv::To(ImGui::GetWindowViewport()->Size);
	
	ImGui::SetCursorPosX(scale * 1000);
	ImGui::GetWindowDrawList()->AddCallback(RenderCallback, &data);
	ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
	ImGui::EndChild();

}

void ProfilerPanel::renderInternal(RenderData& data)
{
	PROFILE_ENTRY("Profiler.RenderNodes");
	using namespace Render;

	RHIBeginRender();

	RHIGraphics2D& g = EditorRenderGloabal::Get().getGraphics();
	static ProfileFrameVisualizeDraw drawer(g);

	g.setViewportSize(data.viewportSize.x, data.viewportSize.y);
	g.beginRender();

	g.beginClip(data.windowPos - Vector2(1,0), data.windowSize);


	drawer.drawGird();

	g.setPen(Color3ub(0, 0, 0));
	g.setFont(FImGui::mFont);

	drawer.mBasePos = data.clientPos;
	drawer.bShowAvg = bShowAvg;
	drawer.bPause = bPause;
	drawer.bGrouped = bGrouped;
	drawer.mMaxLength = data.clientSize.x;

	drawer.mRenderTexts.clear();

	TArray< uint32 > threadIds;
	ProfileSystem::Get().getAllThreadIds(threadIds);

	for( uint32 threadId : threadIds )
	{
		PROFILE_ENTRY("DrawNodes");
		drawer.mLevelTimePos.clear();
		drawer.mThreadId = threadId;
		drawer.visitNodes(threadId);
		drawer.mBasePos.y += drawer.mMaxLevel * drawer.offsetY + 10;
	}
	{
		PROFILE_ENTRY("DrawTexts");
		drawer.drawTexts(data.windowPos);
	}

	g.endClip();
	g.endRender();

	RHIEndRender(false);
}

void ProfilerPanel::RenderCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	RenderData* renderData = (RenderData*)cmd->UserCallbackData;
	ProfilerPanel* thisPanel = renderData->panel;
	thisPanel->renderInternal(*renderData);
}

void ProfilerPanel::getRenderParams(WindowRenderParams& params) const
{
	//params.flags |= ImGuiWindowFlags_HorizontalScrollbar;
}