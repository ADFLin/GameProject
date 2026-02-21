#include "ProfilerPanel.h"
#include "EditorUtils.h"
#include "RHI/RHICommand.h"
#include "ProfileSampleVisitor.h"

#include "ImGui/imgui_internal.h"
#include "InlineString.h"
#include "PlatformThread.h"

#include "RHI/GpuProfiler.h"
#include <algorithm>
#include <atomic>


struct ProfilerSharedState
{
	float scale = 1.0f;
	bool  bPause = false;
	bool  bShowAvg = false;
	bool  bGrouped = false;
	bool  bFlatList = false;

	bool  bSelecting = false;
	float selectStartTime = 0.0f;
	float selectEndTime = 0.0f;

	bool  bDragging = false;
	float dragStartMouseX = 0.0f;
	float dragStartScrollX = 0.0f;
};

ProfilerSharedState GProfilerState;


// Data structure for UI rendering, processed from raw profile nodes
struct SampleView
{
	const char* name;
	double      time;
	double      timeAvg;
	double      timeSelf;
	double      timeSelfAvg;
	int         callCount;
	int         level;
	bool        bIsLeaf;

	struct Timestamp { double start, duration; };
	TArray<Timestamp> timestamps;

	// For tree-like rendering in lists
	int parentIndex = -1;
	int childCount = 0;

	// For grouped (flame graph) rendering
	double offsetGrouped = 0.0;
	double durationGrouped = 0.0;

	// For hover detection in timeline
	Vector2 timelinePos;
	Vector2 timelineSize;

	int     persistentId = -1;
};

struct ThreadView
{
	uint32 threadId;
	std::string name;
	double time;
	TArray<SampleView> samples;
	int maxLevel = 0;
};

class FrameSnapshot
{
public:
	TArray<ThreadView> threads;
	uint64 frameCount = 0;

	void clear() { threads.clear(); }

	SampleView const* getHoveredSample(Vector2 const& mousePos, Vector2 const& clientPos, float scaleFactor, int& outTsIndex) const
	{
		float scale = scaleFactor * 1000;
		double timelineScale = scale / (1000.0 / 60.0);
		float offsetY = 20.0f;

		outTsIndex = -1;
		float threadBaseY = clientPos.y;
		for (auto const& thread : threads)
		{
			float threadHeight = (float)((thread.maxLevel + 1) * offsetY + 10);
			float totalThreadHeight = offsetY + threadHeight;

			// Check if mouse is within this thread's vertical range (Title + Samples)
			if (mousePos.y >= threadBaseY && mousePos.y < threadBaseY + totalThreadHeight)
			{
				// Only check samples if mouse is below the thread title
				float sampleAreaStart = threadBaseY + offsetY;
				if (mousePos.y >= sampleAreaStart)
				{
					for (auto const& sample : thread.samples)
					{
						if (!GProfilerState.bGrouped)
						{
							for (int i = 0; i < (int)sample.timestamps.size(); ++i)
							{
								auto const& ts = sample.timestamps[i];
								Vector2 pos = clientPos + Vector2((float)(ts.start * timelineScale), (threadBaseY - clientPos.y) + offsetY + sample.level * offsetY);
								Vector2 size = Vector2(Math::Max<float>((float)(ts.duration * timelineScale), 1.5f), offsetY);
								if (mousePos.x >= pos.x && mousePos.x <= pos.x + size.x &&
									mousePos.y >= pos.y && mousePos.y <= pos.y + size.y)
								{
									outTsIndex = i;
									return &sample;
								}
							}
						}
						else
						{
							Vector2 pos = clientPos + Vector2((float)(sample.offsetGrouped * timelineScale), (threadBaseY - clientPos.y) + offsetY + sample.level * offsetY);
							Vector2 size = Vector2(Math::Max<float>((float)(sample.durationGrouped * timelineScale), 1.5f), offsetY);
							if (mousePos.x >= pos.x && mousePos.x <= pos.x + size.x &&
								mousePos.y >= pos.y && mousePos.y <= pos.y + size.y)
							{
								outTsIndex = -1;
								return &sample;
							}
						}
					}
				}
				return nullptr; // Within thread area but no sample hit
			}
			threadBaseY += totalThreadHeight;
		}
		return nullptr;
	}
};

static FrameSnapshot GFrameSnapshot[2];
static std::atomic<int> GReadSnapshotIndex{0};
static SpinLock GSnapshotLock;

FrameSnapshot& GetReadSnapshot() { return GFrameSnapshot[GReadSnapshotIndex.load(std::memory_order_acquire)]; }
FrameSnapshot& GetWriteSnapshot() { return GFrameSnapshot[1 - GReadSnapshotIndex.load(std::memory_order_relaxed)]; }

struct NodePersistentStat
{
	double timeAvg = 0;
	double timeSelfAvg = 0;
	int    id = -1;
	double firstRelativeStart = -1.0;
};
static std::unordered_map<void*, NodePersistentStat> GNodeStats;
static int GNextNodeId = 0;

class ProfileDataCollector : public ProfileNodeVisitorT< ProfileDataCollector >
{
public:
	using VisitContext = typename ProfileNodeVisitorT< ProfileDataCollector >::VisitContext;

	ThreadView* mCurrentThread = nullptr;
	double      mTickRate;
	uint64      mTickStart;
	uint64      mCurrentFrame;

	void collect()
	{
		mCurrentFrame = ProfileSystem::Get().getFrameCountSinceReset();
		
		FrameSnapshot& writeSnapshot = GetWriteSnapshot();
		if (writeSnapshot.frameCount == mCurrentFrame)
			return;

		if (GProfilerState.bPause)
		{
			writeSnapshot.frameCount = mCurrentFrame;
			return;
		}

		writeSnapshot.clear();
		writeSnapshot.frameCount = mCurrentFrame;
		
		collectCpu(writeSnapshot);
		collectGpu(writeSnapshot);

		// Swap buffers
		{
			SpinLock::Locker locker(GSnapshotLock);
			GReadSnapshotIndex.store(1 - GReadSnapshotIndex.load(std::memory_order_relaxed), std::memory_order_release);
		}
	}

	void collectCpu(FrameSnapshot& snapshot)
	{
		mTickRate = Profile_GetTickRate();

		TArray<uint32> threadIds;
		ProfileSystem::Get().getAllThreadIds(threadIds);

		uint64 systemFrame = ProfileSystem::Get().getFrameCountSinceReset();
		for (uint32 threadId : threadIds)
		{
			ProfileReadScope scope;
			ProfileSampleNode* root = ProfileSystem::Get().getRootSample(threadId);
			if (!root) continue;

			// Skip threads that haven't been active for more than 2 frames
			if (root->getLastFrame() + 2 < systemFrame)
				continue;

			snapshot.threads.emplace_back();
			mCurrentThread = &snapshot.threads.back();
			mCurrentThread->threadId = threadId;

			mCurrentThread->name = root->getName();
			mCurrentThread->time = root->getFrameExecTime();

			NodePersistentStat& stat = GNodeStats[root];
			stat.timeAvg = 0.9 * stat.timeAvg + 0.1 * root->getFrameExecTime();
			
			mCurrentFrame = root->getLastFrame(); 
			mTickRate = Profile_GetTickRate();
			visitNodes(threadId);
		}
	}

	void collectGpu(FrameSnapshot& snapshot)
	{
		using namespace Render;
		GpuProfileReadScope scope;
		if (scope.isLocked())
		{
			int numSamples = GpuProfiler::Get().getSampleNum();
			if (numSamples > 0)
			{
				snapshot.threads.emplace_back();
				mCurrentThread = &snapshot.threads.back();
				mCurrentThread->threadId = 0xFFFFFFFF; // Virtual ID for GPU
				mCurrentThread->name = "GPU";
				mCurrentThread->time = GpuProfiler::Get().getSample(0)->time;

				mTickRate = 1.0; // GPU times are already in ms relative to start
				mTickStart = 0;

				for (int i = 0; i < numSamples; ++i)
				{
					GpuProfileSample* sample = GpuProfiler::Get().getSample(i);
					if (sample->time < 0) continue;

					SampleView view;
					view.name = sample->name.c_str();
					view.level = sample->level;
					view.callCount = 1;
					view.time = sample->time;
					view.timeAvg = sample->time; // TODO: persistent stats for GPU
					view.timeSelf = sample->time; // Simple for now
					view.timeSelfAvg = sample->time;
					view.offsetGrouped = sample->startTime;
					view.durationGrouped = sample->time;
					view.persistentId = GNextNodeId++; // Temporary ID

					view.bIsLeaf = true; // Simplified
					view.parentIndex = -1; // Flattened for now or logic needed

					view.timestamps.push_back({ sample->startTime, sample->time });

					if (view.level > mCurrentThread->maxLevel) mCurrentThread->maxLevel = view.level;
					mCurrentThread->samples.push_back(std::move(view));
				}
			}
		}
		std::sort(snapshot.threads.begin(), snapshot.threads.end(), 
			[](const ThreadView& a, const ThreadView& b) 
			{
				auto getPriority = [](const std::string& name) 
				{
					if (name == "GameThread") return 0;
					if (name == "RenderThread") return 1;
					if (name == "GPU") return 2;
					return 3;
				};
				int pA = getPriority(a.name);
				int pB = getPriority(b.name);
				if (pA != pB) return pA < pB;
				return a.name < b.name;
			}
		);
	}

	void visitNodes(uint32 threadId)
	{
		ProfileSampleNode* root = ProfileSystem::Get().getRootSample(threadId);
		if (root)
		{
			auto const& timestamps = root->getFrameTimestamps();
			mTickStart = timestamps.empty() ? 0 : timestamps[0].start;

			VisitContext context;
			context.node = root;
			visitRecursive(context, 0, -1, 0.0);
		}
	}

	void visitRecursive(VisitContext& context, int level, int parentIdx, double groupedStartTime)
	{
		if (context.node == nullptr) return;

		bool bIsRoot = (level == 0);
		
		int currentIdx = parentIdx;
		if (!bIsRoot)
		{
			currentIdx = addSample(context, parentIdx, level - 1, groupedStartTime);
			if ((level - 1) > mCurrentThread->maxLevel) mCurrentThread->maxLevel = (level - 1);
		}

		double childAccTime = 0.0;
		double childTotalTime = 0.0;
		ProfileSampleNode* child = context.node->getChild();
		TArray<ProfileSampleNode*> siblings;
		while (child)
		{
			if (filterNode(child))
			{
				siblings.push_back(child);
				NodePersistentStat& stat = GNodeStats[child];
				// Update relative start time to current frame to handle sorting jitter
				auto const& ts = child->getFrameTimestamps();
				if (!ts.empty())
				{
					stat.firstRelativeStart = (double)(ts[0].start - mTickStart) / mTickRate;
				}
			}
			child = child->getSibling();
		}

		std::stable_sort(siblings.begin(), siblings.end(), [](ProfileSampleNode* a, ProfileSampleNode* b)
		{
			double tA = GNodeStats[a].firstRelativeStart;
			double tB = GNodeStats[b].firstRelativeStart;
			if (tA != tB) return tA < tB;
			return strcmp(a->getName(), b->getName()) < 0;
		});

		for (ProfileSampleNode* sibling : siblings)
		{
			VisitContext childContext;
			childContext.node = sibling;

			double childTime = sibling->getFrameExecTime();
			childTotalTime += childTime;

			visitRecursive(childContext, level + 1, currentIdx, groupedStartTime + childAccTime);

			NodePersistentStat& childStat = GNodeStats[sibling];
			childAccTime += (GProfilerState.bShowAvg) ? childStat.timeAvg : childTime;
		}

		if (!bIsRoot)
		{
			// RE-FETCH SampleView by index because the array might have reallocated during recursion!
			SampleView& currentView = mCurrentThread->samples[currentIdx]; 
			currentView.timeSelf = currentView.time - childTotalTime;
			
			NodePersistentStat& stat = GNodeStats[context.node];
			stat.timeSelfAvg = 0.9 * stat.timeSelfAvg + 0.1 * currentView.timeSelf;
			currentView.timeSelfAvg = stat.timeSelfAvg;
		}
	}

	int addSample(VisitContext& context, int parentIdx, int level, double groupedStartTime)
	{
		ProfileSampleNode* node = context.node;
		NodePersistentStat& stat = GNodeStats[node];
		if (stat.id == -1) stat.id = GNextNodeId++;

		stat.timeAvg = 0.9 * stat.timeAvg + 0.1 * node->getFrameExecTime();

		SampleView view;
		view.name = node->getName();
		view.level = level;
		view.callCount = (int)node->getFrameCalls();
		view.time = node->getFrameExecTime();
		view.timeAvg = stat.timeAvg;
		view.timeSelf = view.time;
		view.timeSelfAvg = stat.timeSelfAvg;
		view.offsetGrouped = groupedStartTime;
		view.durationGrouped = (GProfilerState.bShowAvg) ? stat.timeAvg : node->getFrameExecTime();
		view.persistentId = stat.id;

		view.bIsLeaf = (node->getChild() == nullptr);
		view.parentIndex = parentIdx;
		if (parentIdx != -1) mCurrentThread->samples[parentIdx].childCount++;

		for (auto const& ts : node->getFrameTimestamps())
		{
			view.timestamps.push_back({ double(ts.start - mTickStart) / mTickRate, double(ts.end - ts.start) / mTickRate });
		}

		mCurrentThread->samples.push_back(std::move(view));
		return (int)mCurrentThread->samples.size() - 1;
	}

	bool filterNode(ProfileSampleNode* node)
	{
		// Critical check: Only accept nodes that match the current thread-root's frame index.
		// If a node wasn't called in the same frame as its root, it's definitely residual data.
		if (node->getLastFrame() != mCurrentFrame)
			return false;

#if 0
		if (node->getFrameCalls() == 0)
			return false;
#endif

		return !node->getFrameTimestamps().empty();
	}
};

static ProfileDataCollector GProfileCollector;

struct TimelineContext
{
	Vector2 clientPos;
	Vector2 windowPos;
	Vector2 windowSize;
	float   scale;

	bool    bSelecting;
	float   selectStartTime;
	float   selectEndTime;
};

void DrawProfileTimeline(RHIGraphics2D& g, FrameSnapshot& snapshot, TimelineContext const& timelineCtx);
void DrawProfileTable(FrameSnapshot const& snapshot);

REGISTER_EDITOR_PANEL(ProfilerPanel, "Profiler", true, true);
REGISTER_EDITOR_PANEL(ProfilerListPanel, "ProfilerList", true, true);

Color3ub GetNodeColor(const char* name)
{
	uint32 hash = FNV1a::MakeStringHash<uint32>(name);
	float h = (float)fmod(hash * 0.618033988749895, 1.0);
	float s = 0.6f;
	float l = 0.6f;

	auto hslToRgb = [](float h, float s, float l) -> Color3ub
	{
		auto hueToRgb = [](float p, float q, float t)
		{
			if (t < 0.0f) t += 1.0f;
			if (t > 1.0f) t -= 1.0f;
			if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
			if (t < 1.0f / 2.0f) return q;
			if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
			return p;
		};

		float q = (l < 0.5f) ? (l * (1.0f + s)) : (l + s - l * s);
		float p = 2.0f * l - q;

		return Color3ub(
			(uint8)(hueToRgb(p, q, h + 1.0f / 3.0f) * 255.0f),
			(uint8)(hueToRgb(p, q, h) * 255.0f),
			(uint8)(hueToRgb(p, q, h - 1.0f / 3.0f) * 255.0f)
		);
	};

	return hslToRgb(h, s, l);
}


void ProfilerPanel::render()
{
	GProfileCollector.collect();

	SpinLock::Locker snapshotLocker(GSnapshotLock);
	FrameSnapshot& snapshot = GetReadSnapshot();
	int snapshotIndex = GReadSnapshotIndex.load(std::memory_order_acquire);

	ImGui::Checkbox("Pause", &GProfilerState.bPause);
	ImGui::SameLine();
	ImGui::Checkbox("Grouped", &GProfilerState.bGrouped);
	if (GProfilerState.bGrouped)
	{
		ImGui::SameLine();
		ImGui::Checkbox("ShowAvg", &GProfilerState.bShowAvg);
	}
	ImGui::SameLine();

	ImGui::SliderFloat("Scale", &GProfilerState.scale, 0.1f, 1000.0f, "%.1f");
	if (ImGui::IsItemActive())
	{
		GProfilerState.scale = Math::Min(GProfilerState.scale, 1000.0f);
	}

	ImGui::BeginChild("Visualize", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	auto viewport = ImGui::GetWindowViewport();
	ImGuiWindow* window = ImGui::GetCurrentWindowRead();

	struct RenderData
	{
		Math::Vector2 clientPos;
		Math::Vector2 clientSize;
		Math::Vector2 windowPos;
		Math::Vector2 windowSize;
		ImGuiViewport* viewport;
	};
	RenderData data;
	data.clientPos = FImGuiConv::To(ImGui::GetCursorScreenPos() - ImGui::GetWindowViewport()->Pos);
	data.clientSize = FImGuiConv::To(window->ContentSize);
	data.windowPos = FImGuiConv::To(ImGui::GetWindowPos() - ImGui::GetWindowViewport()->Pos);
	data.windowSize = FImGuiConv::To(ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin());
	data.viewport = ImGui::GetWindowViewport();

	float displayTimeLimit = 1000.0f / 60.0f;
	float currentScale = (GProfilerState.scale * 1000) / displayTimeLimit;

	float wheel = ImGui::GetIO().MouseWheel;
	if (wheel != 0 && ImGui::IsWindowHovered())
	{
		float oldScale = GProfilerState.scale;
		GProfilerState.scale *= powf(1.2f, wheel);
		GProfilerState.scale = Math::Clamp(GProfilerState.scale, 0.01f, 10000000.0f);

		float mousePosX = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
		float scrollX = ImGui::GetScrollX();
		float newScrollX = (scrollX + mousePosX) * (GProfilerState.scale / oldScale) - mousePosX;
		ImGui::SetScrollX(newScrollX);

		if (GProfilerState.scale * 1000 > ImGui::GetIO().DisplaySize.x)
		{
			float scrollDelta = newScrollX - scrollX;
			data.clientPos.x -= scrollDelta;
		}

		currentScale = (GProfilerState.scale * 1000) / displayTimeLimit;
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered())
	{
		GProfilerState.bSelecting = true;
		GProfilerState.selectStartTime = (ImGui::GetMousePos().x - (data.clientPos.x + data.viewport->Pos.x)) / currentScale;
		GProfilerState.selectEndTime = GProfilerState.selectStartTime;
	}

	if (GProfilerState.bSelecting)
	{
		GProfilerState.selectEndTime = (ImGui::GetMousePos().x - (data.clientPos.x + data.viewport->Pos.x)) / currentScale;
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			GProfilerState.bSelecting = false;
		}
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !GProfilerState.bSelecting)
	{
		GProfilerState.bDragging = true;
		GProfilerState.dragStartMouseX = ImGui::GetMousePos().x;
		GProfilerState.dragStartScrollX = ImGui::GetScrollX();
	}

	if (GProfilerState.bDragging)
	{
		float currentMouseX = ImGui::GetMousePos().x;
		float mouseDelta = currentMouseX - GProfilerState.dragStartMouseX;
		ImGui::SetScrollX(GProfilerState.dragStartScrollX - mouseDelta);

		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			GProfilerState.bDragging = false;
		}
	}

	if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
	{
		GProfilerState.bDragging = false; 

		float t1 = Math::Min(GProfilerState.selectStartTime, GProfilerState.selectEndTime);
		float t2 = Math::Max(GProfilerState.selectStartTime, GProfilerState.selectEndTime);
		float dt = t2 - t1;
		if (dt > 1e-4f)
		{
			float mouseT = (ImGui::GetMousePos().x - (data.clientPos.x + data.viewport->Pos.x)) / currentScale;
			if (mouseT >= t1 && mouseT <= t2)
			{
				float viewWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
				GProfilerState.scale = (viewWidth * displayTimeLimit) / (1000.0f * dt);
				float newCurrentScale = (GProfilerState.scale * 1000) / displayTimeLimit;
				ImGui::SetScrollX(t1 * newCurrentScale);
			}
		}
	}


	double time = (double)displayTimeLimit;
	for (auto& threadView : snapshot.threads)
	{
		if (threadView.name == "GameThread")
		{
			time = Math::Max<double>(threadView.time, time);
			break;
		}
	}

	ImGui::SetCursorPosX((float)(GProfilerState.scale * 1000.0 * time / (double)displayTimeLimit));

	TimelineContext timelineCtx;
	timelineCtx.clientPos = data.clientPos;
	timelineCtx.windowPos = data.windowPos;
	timelineCtx.windowSize = data.windowSize;
	timelineCtx.scale = GProfilerState.scale;
	timelineCtx.bSelecting = GProfilerState.bSelecting;
	timelineCtx.selectStartTime = GProfilerState.selectStartTime;
	timelineCtx.selectEndTime = GProfilerState.selectEndTime;

	Vector2 displaySize = FImGuiConv::To(ImGui::GetIO().DisplaySize);
	EditorRenderGloabal::Get().addCustomFunc([timelineCtx, displaySize, snapshotIndex](const ImDrawList* parentlist, const ImDrawCmd* cmd)
	{
		using namespace Render;
		PROFILE_ENTRY("Profiler.RenderNodes");

		RHIBeginRender();

		RHIGraphics2D& g = EditorRenderGloabal::Get().getGraphics();
		g.setViewportSize((int)displaySize.x, (int)displaySize.y);
		g.beginRender();

		DrawProfileTimeline(g, GFrameSnapshot[snapshotIndex], timelineCtx);

		g.endRender();
		RHIEndRender(false);
	});

	if (ImGui::IsWindowHovered())
	{
		Vector2 mousePosInViewport = FImGuiConv::To(ImGui::GetMousePos() - ImGui::GetWindowViewport()->Pos);
		int tsIndex = -1;
		auto hoveredSample = snapshot.getHoveredSample(mousePosInViewport, timelineCtx.clientPos, timelineCtx.scale, tsIndex);
		if (hoveredSample)
		{
			double startTime, duration;
			if (GProfilerState.bGrouped)
			{
				startTime = hoveredSample->offsetGrouped;
				duration = hoveredSample->durationGrouped;
			}
			else
			{
				startTime = hoveredSample->timestamps[tsIndex].start;
				duration = hoveredSample->timestamps[tsIndex].duration;
			}

			double endTime = startTime + duration;
			float displayTimeLimit = 1000.0f / 60.0f;
			double percentage = (hoveredSample->time / displayTimeLimit) * 100.0;

			ImGui::BeginTooltip();
			
			// Header
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "%s", hoveredSample->name);
			ImGui::Separator();

			// Layout helpers
			auto drawMetric = [](const char* label, const char* value, const ImVec4& valueColor = ImVec4(1,1,1,1)) {
				ImGui::Text("%-12s", label);
				ImGui::SameLine();
				ImGui::TextColored(valueColor, ": %s", value);
			};

			InlineString<64> valStr;

			// Timestamps
			valStr.format("%.3f ms", (float)startTime);
			drawMetric("Start", valStr.c_str());

			valStr.format("%.3f ms", (float)endTime);
			drawMetric("End", valStr.c_str());

			// Duration
			ImVec4 metricColor(0.4f, 0.8f, 1.0f, 1.0f);
			if (duration < 0.001)
				valStr.format("%.3f us", (float)(duration * 1000.0));
			else
				valStr.format("%.3f ms", (float)duration);
			drawMetric("Duration", valStr.c_str(), metricColor);

			// Total
			if (hoveredSample->time < 0.001)
				valStr.format("%.3f us", (float)(hoveredSample->time * 1000.0));
			else
				valStr.format("%.3f ms", (float)hoveredSample->time);
			drawMetric("Total Time", valStr.c_str(), metricColor);

			// Percentage
			valStr.format("%.2f %%", (float)percentage);
			ImVec4 pctColor = (percentage > 80.0) ? ImVec4(1, 0.2f, 0.2f, 1) : (percentage > 50.0 ? ImVec4(1, 0.6f, 0.2f, 1) : ImVec4(0.4f, 1, 0.4f, 1));
			drawMetric("Percentage", valStr.c_str(), pctColor);

			// Call count
			if (hoveredSample->callCount > 0)
			{
				valStr.format("%d", hoveredSample->callCount);
				drawMetric("Calls", valStr.c_str());
			}

			ImGui::EndTooltip();
		}
	}

	ImGui::EndChild();
}

// Profiler timeline layer policy: groups shapes and text globally.
// Without this, each drawRect/drawText gets a unique sequential layer,
// preventing same-state elements from batching.
// With ~100+ profile nodes, each with 3 rects + text, this reduces
// draw calls from O(N * state_changes) to O(distinct_states) ≈ 3-5.
class ProfilerLayerPolicy : public RHIGraphics2D::LayerPolicy
{
public:
	static constexpr int32 PHASE_SHAPE = 0;
	static constexpr int32 PHASE_TEXT  = 1;
	static constexpr int32 NUM_PHASES  = 2;
	static constexpr int32 PHASE_RANGE = 100000;

	int32 mPhaseCounters[NUM_PHASES] = {};

	void reset()
	{
		for (int i = 0; i < NUM_PHASES; ++i)
			mPhaseCounters[i] = 0;
	}

	int32 getLayer(Render::RenderBatchedElement const& element, RHIGraphics2D::RenderState const& state) override
	{
		int32 phase;
		switch (element.type)
		{
		case Render::RenderBatchedElement::Text:
		case Render::RenderBatchedElement::ColoredText:
			phase = PHASE_TEXT;
			break;
		default:
			phase = PHASE_SHAPE;
			break;
		}
		return phase * PHASE_RANGE + mPhaseCounters[phase]++;
	}
};
static ProfilerLayerPolicy g_ProfilerLayerPolicy;

void DrawProfileTimeline(RHIGraphics2D& g, FrameSnapshot& snapshot, TimelineContext const& timelineCtx)
{
	Vector2 const& clientPos = timelineCtx.clientPos;
	Vector2 const& windowPos = timelineCtx.windowPos;
	Vector2 const& windowSize = timelineCtx.windowSize;
	float scaleFactor = timelineCtx.scale;

	// Set aggressive layer policy to batch profile node rects and text
	g_ProfilerLayerPolicy.reset();
	g.setLayerPolicy(&g_ProfilerLayerPolicy);

	g.beginClip(windowPos - Vector2(1, 0), windowSize);

	float scale = scaleFactor * 1000;
	double timelineScale = scale / (1000.0 / 60.0);
	float offsetY = 20;

	// Draw Grids
	g.enablePen(true);
	g.setPen(Color3ub(80, 80, 80)); 
	for (float t = 0; t <= 120; t += 10)
	{
		float x = clientPos.x + (float)(t * timelineScale);
		g.drawLine(Vector2(x, clientPos.y), Vector2(x, clientPos.y + windowSize.y));
		
		InlineString<32> str;
		str.format("%.0f ms", t);
		g.setTextColor(Color3ub(80, 80, 80));
		g.drawText(Vector2(x + 2, windowPos.y + windowSize.y - offsetY), Vector2(50, offsetY), str.c_str(), EHorizontalAlign::Left, EVerticalAlign::Center, false);
	}

	// Warning Lines
	float thresholds[] = { 1000.0f / 60.0f, 1000.0f / 30.0f, 1000.0f / 15.0f };
	Color3ub thresholdColors[] = { Color3ub(0, 255, 0), Color3ub(255, 255, 0), Color3ub(255, 0, 0) };
	for (int i = 0; i < 3; ++i)
	{
		float x = clientPos.x + (float)(thresholds[i] * timelineScale);
		g.setPen(thresholdColors[i]);
		g.drawLine(Vector2(x, clientPos.y), Vector2(x, clientPos.y + windowSize.y));

		InlineString<32> str;
		str.format("%.2f ms", thresholds[i]);
		g.setTextColor(thresholdColors[i]);
		g.drawText(Vector2(x + 2, windowPos.y + windowSize.y - offsetY), Vector2(60, offsetY), str.c_str(), EHorizontalAlign::Left, EVerticalAlign::Center, false);
	}

	g.setFont(FImGui::mFont);

	struct TextData
	{
		Vector2 pos;
		Vector2 size;

		InlineString<256> str;
	};
	static TArray<TextData> GPendingTexts;
	GPendingTexts.clear();

	float nextBaseY = 0;
	for (auto& thread : snapshot.threads)
	{
		g.setTextColor(Color3ub(255, 255, 255));
		float titleX = Math::Max(clientPos.x + 5, windowPos.x + 5);
		InlineString<256> str;
		str.format("%s (%.2lf ms) (ID = %u)", thread.name.c_str(), thread.time, thread.threadId);
		g.drawText(Vector2(titleX, clientPos.y + nextBaseY), Vector2(windowSize.x - (titleX - windowPos.x), offsetY), str.c_str(), EHorizontalAlign::Left, EVerticalAlign::Center, false);
		nextBaseY += offsetY;

		g.enablePen(true);
		g.setPen(Color3ub(0, 0, 0));

		for (auto& sample : thread.samples)
		{
			Color3ub nodeColor = GetNodeColor(sample.name);
			Color3ub shadowColor = Color3ub((uint8)(nodeColor.r * 0.80f), (uint8)(nodeColor.g * 0.80f), (uint8)(nodeColor.b * 0.80f));
			Color3ub highlightColor = Color3ub((uint8)Math::Min(255, (int)(nodeColor.r * 1.25f)), (uint8)Math::Min(255, (int)(nodeColor.g * 1.25f)), (uint8)Math::Min(255, (int)(nodeColor.b * 1.25f)));

			auto addText = [&](Vector2 const& pos, Vector2 const& size, double duration)
			{
				if (size.x > 8)
				{
					TextData td;
					td.pos = pos;
					td.size = size;
					td.str.format("%s (%.2f ms)", sample.name, (float)duration);
					GPendingTexts.push_back(td);
				}
			};

			auto drawSampleNode = [&](Vector2 const& pos, Vector2 const& size, double duration, double trueWidth)
			{
				sample.timelinePos = pos;
				sample.timelineSize = size;

				// Horizontal culling
				if (pos.x + size.x < windowPos.x || pos.x > windowPos.x + windowSize.x)
					return;

				// Vertical culling
				if (pos.y + size.y < windowPos.y || pos.y > windowPos.y + windowSize.y)
					return;

				if (trueWidth < 0.05)
					return;

				// 1. Calculate pixel boundaries
				float x1 = Math::Floor(pos.x);
				float x2 = Math::Floor(pos.x + size.x);
				float y1 = Math::Floor(pos.y);
				float y2 = Math::Floor(pos.y + size.y);

				// 2. Uniform Gap: Subtract 1px from right and bottom for ALL nodes.
				// This ensures that the last child and its parent end at the exact same pixel.
				float drawX2 = (x2 - x1 >= 1.0f) ? (x2 - 1.0f) : x2;
				float drawY2 = (y2 - y1 >= 1.0f) ? (y2 - 1.0f) : y2;

				float w = drawX2 - x1;
				float h = drawY2 - y1;

				if (w <= 0) w = 1.0f;

				g.enablePen(false);
				
				// Base color
				g.setBrush(nodeColor);
				g.drawRect(Vector2(x1, y1), Vector2(w, h));

				if (h > 4.0f)
				{
					// Shaded bottom (40% height)
					float shadowH = Math::Floor(h * 0.4f);
					g.setBrush(shadowColor);
					g.drawRect(Vector2(x1, drawY2 - shadowH), Vector2(w, shadowH));

					// Highlight top edge (1px)
					g.setBrush(highlightColor);
					g.drawRect(Vector2(x1, y1), Vector2(w, 1.0f));
				}

				addText(Vector2(x1, y1), Vector2(w, h), duration);
			};

			if (!GProfilerState.bGrouped)
			{
				for (auto const& ts : sample.timestamps)
				{
					double trueWidth = ts.duration * timelineScale;
					Vector2 pos = clientPos + Vector2((float)(ts.start * timelineScale), (float)(nextBaseY + sample.level * offsetY));
					Vector2 size = Vector2(Math::Max<float>((float)trueWidth, 1.5f), offsetY);
					drawSampleNode(pos, size, ts.duration, trueWidth);
				}
			}
			else
			{
				double trueWidth = sample.durationGrouped * timelineScale;
				Vector2 pos = clientPos + Vector2((float)(sample.offsetGrouped * timelineScale), (float)(nextBaseY + sample.level * offsetY));
				Vector2 size = Vector2(Math::Max<float>((float)trueWidth, 1.5f), offsetY);
				drawSampleNode(pos, size, sample.durationGrouped, trueWidth);
			}

			// Restore pen state for next items
			g.enablePen(true);
		}

		nextBaseY += (float)((thread.maxLevel + 1) * offsetY + 10);
	}

	// Draw Texts with Clamping
	g.setTextColor(Color3ub(0, 0, 0));

	Vector2 clipPos = windowPos;
	for (auto const& textData : GPendingTexts)
	{
		Vector2 pos = textData.pos;
		Vector2 size = textData.size;
		float originalX = pos.x;
		pos.x = Math::Clamp(pos.x, clipPos.x, pos.x + size.x);
		size.x -= pos.x - originalX;

		// Prune text that is too small or clipped away
		if (size.x > 8.0f)
		{
			// bClip must be true to prevent text bleeding out of nodes
			g.drawText(pos + Vector2(3,0), size - Vector2(3,0), textData.str.c_str(), EHorizontalAlign::Left, EVerticalAlign::Center, true);
		}
	}

	// Draw Selection ON TOP
	if (timelineCtx.bSelecting || (timelineCtx.selectStartTime != timelineCtx.selectEndTime))
	{
		float x1 = clientPos.x + (float)(timelineCtx.selectStartTime * timelineScale);
		float x2 = clientPos.x + (float)(timelineCtx.selectEndTime * timelineScale);

		if (x1 > x2) std::swap(x1, x2);

		// Semi-transparent background
		g.enablePen(false);
		g.setBrush(Color3ub(255, 255, 255));
		g.beginBlend(0.2f);
		g.drawRect(Vector2(x1, windowPos.y), Vector2(x2 - x1, windowSize.y));
		g.endBlend();

		// Bright white borders
		g.enablePen(true);
		g.setPen(Color3ub(255, 255, 255), 1);
		g.drawLine(Vector2(x1, windowPos.y), Vector2(x1, windowPos.y + windowSize.y));
		g.drawLine(Vector2(x2, windowPos.y), Vector2(x2, windowPos.y + windowSize.y));

		// Draw selection duration and timestamps
		float t1 = timelineCtx.selectStartTime;
		float t2 = timelineCtx.selectEndTime;
		if (t1 > t2) std::swap(t1, t2);

		float duration = t2 - t1;
		InlineString<64> str;
		g.setTextColor(Color3ub(255, 255, 255));

		float textWidth = 80;
		float textGap = 5;

		// Start Time (Left)
		{
			str.format("%.3f ms", t1);
			float drawX = Math::Max(x1 - textWidth - textGap, windowPos.x + textGap);
			// If we are overlapping with the selection area, clamp to the left side
			if (drawX + textWidth > x1) drawX = x1 - textWidth - textGap;
			g.drawText(Vector2(drawX, windowPos.y), Vector2(textWidth, offsetY), str.c_str(), EHorizontalAlign::Right, EVerticalAlign::Center, false);
		}

		// End Time (Right)
		{
			str.format("%.3f ms", t2);
			float drawX = Math::Min(x2 + textGap, windowPos.x + windowSize.x - textWidth - textGap);
			// If we are overlapping with the selection area, clamp to the right side
			if (drawX < x2) drawX = x2 + textGap;
			g.drawText(Vector2(drawX, windowPos.y), Vector2(textWidth, offsetY), str.c_str(), EHorizontalAlign::Left, EVerticalAlign::Center, false);
		}

		// Duration (Center)
		str.format("%.3f ms", duration);
		float centerX = (x1 + x2) * 0.5f;
		float durationX = Math::Clamp(centerX - textWidth * 0.5f, x1, x2 - textWidth);
		// Only draw duration if it fits or area is wide enough
		if (x2 - x1 > 10)
		{
			g.drawText(Vector2(x1, windowPos.y), Vector2(x2 - x1, offsetY), str.c_str(), EHorizontalAlign::Center, EVerticalAlign::Center, true);
		}
	}

	g.endClip();

	// Reset layer policy
	g.setLayerPolicy(nullptr);
}

void DrawProfileTable(FrameSnapshot const& snapshot)
{
	float displayTimeLimit = 1000.0f / 60.0f;

	for (auto const& thread : snapshot.threads)
	{
		InlineString<256> headerId;
		headerId.format("%s###Thread_%u", thread.name.c_str(), thread.threadId);
		if (ImGui::CollapsingHeader(headerId.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
			if (GProfilerState.bFlatList)
				tableFlags |= ImGuiTableFlags_Sortable;

			InlineString<64> tableId;
			tableId.format("##Nodes_%u", thread.threadId);
			if (ImGui::BeginTable(tableId.c_str(), 5, tableFlags))
			{
				ImGuiTableColumnFlags colFlags = GProfilerState.bFlatList ? ImGuiTableColumnFlags_None : ImGuiTableColumnFlags_NoSort;
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch | colFlags);
				float timeColWidth = 80;
				ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed | colFlags | (GProfilerState.bFlatList ? ImGuiTableColumnFlags_DefaultSort : 0), timeColWidth);
				ImGui::TableSetupColumn("Inner", ImGuiTableColumnFlags_WidthFixed | colFlags, timeColWidth);
				ImGui::TableSetupColumn("%", ImGuiTableColumnFlags_WidthFixed | colFlags, 60);
				ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed | colFlags, 60);
				ImGui::TableHeadersRow();

				TArray<SampleView const*> displayNodes;
				for (auto const& sample : thread.samples) displayNodes.push_back(&sample);

				if (GProfilerState.bFlatList)
				{
					if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs())
					{
						std::sort(displayNodes.begin(), displayNodes.end(), [&](SampleView const* a, SampleView const* b) {
							for (int n = 0; n < sortSpecs->SpecsCount; n++)
							{
								const ImGuiTableColumnSortSpecs* spec = &sortSpecs->Specs[n];
								int res = 0;
								switch (spec->ColumnIndex)
								{
								case 0: res = strcmp(a->name, b->name); break;
								case 1: 
								case 3:
								{
									double valA = (GProfilerState.bShowAvg) ? a->timeAvg : a->time;
									double valB = (GProfilerState.bShowAvg) ? b->timeAvg : b->time;
									res = (valA < valB) ? -1 : (valA > valB ? 1 : 0);
								}
								break;
								case 2:
								{
									double valA = (GProfilerState.bShowAvg) ? a->timeSelfAvg : a->timeSelf;
									double valB = (GProfilerState.bShowAvg) ? b->timeSelfAvg : b->timeSelf;
									res = (valA < valB) ? -1 : (valA > valB ? 1 : 0);
								}
								break;
								case 4: res = (a->callCount - b->callCount); break;
								}

								if (res != 0)
								{
									if (spec->SortDirection == ImGuiSortDirection_Ascending)
										return res < 0;
									return res > 0;
								}
							}
							return a->persistentId < b->persistentId;
						});
					}
				}

				int skipLevel = 1000;
				for (int i = 0; i < (int)displayNodes.size(); ++i)
				{
					auto const& sample = *displayNodes[i];
					
					if (!GProfilerState.bFlatList)
					{
						if (sample.level > skipLevel) continue;
						skipLevel = 1000;
					}

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					if (!GProfilerState.bFlatList)
					{
						for (int j = 0; j < sample.level; ++j) ImGui::Indent();
						ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_NoTreePushOnOpen;
						if (sample.bIsLeaf) flags |= ImGuiTreeNodeFlags_Leaf;
						else flags |= ImGuiTreeNodeFlags_DefaultOpen;

						bool open = ImGui::TreeNodeEx((void*)(intptr_t)sample.persistentId, flags, "%s", sample.name);
						for (int j = 0; j < sample.level; ++j) ImGui::Unindent();

						if (!open && !sample.bIsLeaf) skipLevel = sample.level;
					}
					else
					{
						ImGui::Text("%s", sample.name);
					}

					double frameTime = (GProfilerState.bShowAvg) ? sample.timeAvg : sample.time;
					double selfTime = (GProfilerState.bShowAvg) ? sample.timeSelfAvg : sample.timeSelf;
					if (selfTime < 0) selfTime = 0; // Avoid precision jitter showing negative

					double percentage = (frameTime / displayTimeLimit) * 100.0;
					ImGui::TableNextColumn(); ImGui::Text("%.3f ms", (float)frameTime);
					ImGui::TableNextColumn(); ImGui::Text("%.3f ms", (float)selfTime);
					ImGui::TableNextColumn(); ImGui::Text("%.2f%%", (float)percentage);
					ImGui::TableNextColumn(); ImGui::Text("%d", sample.callCount);
				}
				ImGui::EndTable();
			}
		}
	}
}

void ProfilerListPanel::render()
{
	ImGui::Checkbox("Pause", &GProfilerState.bPause);
	ImGui::SameLine();
	ImGui::Checkbox("Flat List", &GProfilerState.bFlatList);

	SpinLock::Locker snapshotLocker(GSnapshotLock);
	DrawProfileTable(GetReadSnapshot());
}

void ProfilerPanel::getRenderParams(WindowRenderParams& params) const {}