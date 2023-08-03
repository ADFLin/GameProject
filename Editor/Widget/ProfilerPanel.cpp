#include "ProfilerPanel.h"
#include "EditorUtils.h"
#include "RHI/RHICommand.h"
#include "ProfileSampleVisitor.h"

#include "ImGui/imgui_internal.h"
#include "InlineString.h"

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
	int   mLevel;
	TArray<float> mLevelTimePos;
	void onRoot(VisitContext& context)
	{
		updateDisplayTime(context);

		mTotalTime = context.displayTime;
		mTickStart = context.node->getFrameTimestamps()[0].start;
		mScale     = mMaxLength / mTotalTime;
		mCurTime   = 0;
		mLevel     = 0;
		mLevelTimePos.push_back(mCurTime);
	}

	void onNode(VisitContext& context)
	{
		NodeStat& stat = updateDisplayTime(context);

		SampleNode* node = context.node;
		Vector2 pos = mBasePos + Vector2(mScale * (mCurTime + context.displayTimeAcc), mLevel * offsetY);
		Vector2 size = Vector2(Math::Max<float>(mScale * context.displayTime, 1.5f), offsetY);

		int color = FNV1a::MakeStringHash<uint32>(node->getName()) % ARRAY_SIZE(GColors);
		mGraphics2D.setBrush(GColors[color]);

		auto DrawNode = [&](double timePos, double duration)
		{
			Vector2 pos = mBasePos + Vector2(mScale * timePos, mLevel * offsetY);
			Vector2 size = Vector2(Math::Max<float>(mScale * duration, 3.0f), offsetY);
			mGraphics2D.drawRect(pos, size);
			InlineString<256> text;
			text.format("%s (%.2lf)", node->getName(), duration);
			mGraphics2D.drawText(pos + Vector2(2, 0), size, text, EHorizontalAlign::Left, EVerticalAlign::Center, true);
		};
		if ( bGrouped )
		{
			DrawNode(mCurTime + context.displayTimeAcc, context.displayTime);
		}
		else
		{
			for (auto const& timestamp : stat.timestamps)
			{
				DrawNode(timestamp.start, timestamp.duration);
			}
		}
	}


	struct Timestamp
	{
		double start;
		double duration;
	};

	struct NodeStat
	{
		double displayTime = 0;
		double execAvg = 0;
		TArray< Timestamp > timestamps;
	};

	NodeStat& updateDisplayTime(VisitContext &context)
	{
		SampleNode* node = context.node;
		NodeStat& stat = nodeStatMap[node];
		stat.execAvg = 0.9 * stat.execAvg + 0.1 * node->getFrameExecTime();
		if (!bPause)
		{
			stat.displayTime = bShowAvg ? stat.execAvg : node->getFrameExecTime();
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


	double mMaxLength = 600;
	double offsetY = 20;
	Vector2      mBasePos = Vector2::Zero();
	RHIGraphics2D& mGraphics2D;
};


void ProfilerPanel::render()
{
	static RenderData data;
	ImGui::Checkbox("Pause", &bPause);
	ImGui::SameLine();
	ImGui::Checkbox("ShowAvg", &bShowAvg);
	ImGui::SameLine();
	ImGui::Checkbox("Grouped", &bGrouped);
	ImGui::SameLine();
	ImGui::SliderFloat("Scale", &scale, 0.1, 100, "%g");

	ImGui::BeginChild("Visualize", ImVec2(0,0), false , ImGuiWindowFlags_HorizontalScrollbar);
	auto drawData = ImGui::GetDrawData();
	auto viewport = ImGui::GetWindowViewport();
	ImGuiWindow* window = ImGui::GetCurrentWindowRead();

	data.panel = this;
	data.clientPos = FImGuiConv::To(ImGui::GetCursorScreenPos()) - FImGuiConv::To(ImGui::GetWindowViewport()->Pos);
	data.clientSize = FImGuiConv::To(window->ContentSize);
	data.windowPos = FImGuiConv::To(ImGui::GetWindowPos()) - FImGuiConv::To(ImGui::GetWindowViewport()->Pos);
	data.windowSize = FImGuiConv::To(ImGui::GetWindowSize());
	data.viewportSize = FImGuiConv::To(ImGui::GetWindowViewport()->Size);
	
	ImGui::SetCursorPosX(scale * 1000);
	ImGui::GetWindowDrawList()->AddCallback(RenderCallback, &data);
	ImGui::GetWindowDrawList()->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
	ImGui::EndChild();

}

void ProfilerPanel::renderInternal(RenderData& data)
{
	using namespace Render;

	RHIBeginRender();

	RHIGraphics2D& g = EditorRenderGloabal::Get().getGraphics();

	g.setViewportSize(data.viewportSize.x, data.viewportSize.y);
	g.beginRender();

	g.beginClip(data.windowPos, data.windowSize);

	g.setPen(Color3ub(0, 0, 0));
	g.setFont(FImGui::mFont);
	static ProfileFrameVisualizeDraw drawer(g);
	drawer.mBasePos = data.clientPos;
	drawer.bShowAvg = bShowAvg;
	drawer.bPause = bPause;
	drawer.bGrouped = bGrouped;
	drawer.mMaxLength = data.clientSize.x;
	drawer.visitNodes();
	
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