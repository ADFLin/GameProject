#pragma once
#ifndef ProfilerPanel_H_C1D3393C_A1C9_4743_A49C_E082A42DFAE0
#define ProfilerPanel_H_C1D3393C_A1C9_4743_A49C_E082A42DFAE0

#include "EditorPanel.h"
#include "Math/Vector2.h"


class ProfilerPanel : public IEditorPanel
{
public:

	struct RenderData
	{
		ProfilerPanel* panel;
		Math::Vector2 clientPos;
		Math::Vector2 clientSize;
		Math::Vector2 windowPos;
		Math::Vector2 windowSize;
		Math::Vector2 viewportSize;
	};

	float scale = 1.0f;
	bool  bPause = false;
	bool  bShowAvg = false;
	bool  bGrouped = false;

	void render();

	void renderInternal(RenderData& data);

	static void RenderCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);

	void getRenderParams(WindowRenderParams& params) const override;

};


#endif // ProfilerPanel_H_C1D3393C_A1C9_4743_A49C_E082A42DFAE0