#pragma once
#ifndef ShaderGraphPanel_H_48C9804A_A350_41B7_9B82_B55520778564
#define ShaderGraphPanel_H_48C9804A_A350_41B7_9B82_B55520778564

#include "EditorPanel.h"

#include "ShaderGraph/ShaderGraph.h"


#include "RHI/ShaderProgram.h"
#include "RHI/RHICommon.h"

#include "ImNodeEditor/imgui_node_editor.h"

namespace ImNode = ax::NodeEditor;


class ShaderGraphPanel : public IEditorPanel
{
public:

	void runTest();

	Render::RHIFrameBufferRef mFrameBuffer;
	Render::ShaderProgram mShaderProgram;
	Render::RHITexture2DRef mTexture;
	ShaderGraph mGraph;

	void onOpen() override;


	void render() override;

	int FetchId()
	{
		return mNdexId++;
	}
	int mNdexId = 1;

	ImNode::EditorContext* mEditorContext;
};


#endif // ShaderGraphPanel_H_48C9804A_A350_41B7_9B82_B55520778564
