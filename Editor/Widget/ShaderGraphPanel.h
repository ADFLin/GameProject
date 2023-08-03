#pragma once
#ifndef ShaderGraphPanel_H_48C9804A_A350_41B7_9B82_B55520778564
#define ShaderGraphPanel_H_48C9804A_A350_41B7_9B82_B55520778564

#include "EditorPanel.h"

#include "ShaderGraph/ShaderGraph.h"


#include "RHI/ShaderProgram.h"
#include "RHI/RHICommon.h"

#include "ImNodeEditor/imgui_node_editor.h"
#include "Renderer/SceneView.h"
#include "Core/Tickable.h"
#include "DataStructure/Array.h"
#include "Math/TVector2.h"

namespace ImNode = ax::NodeEditor;


class ShaderGraphPanel : public IEditorPanel
	                   , public Tickable
{
public:

	void runTest();

	bool compileShader();

	void renderShaderPreview(TVector2<int> const& size);

	template< typename TNode, typename ...TArgs>
	void registerNode(TArgs&& ...args)
	{
		std::shared_ptr<TNode> result = std::make_shared<TNode>(std::forward<TArgs>(args)...);
		mNodesList.push_back(result);
	}
	TArray<SGNodePtr> mNodesList;
	Render::ViewInfo mView;

	Render::RHIFrameBufferRef mFrameBuffer;
	Render::ShaderProgram mShaderProgram;
	Render::RHITexture2DRef mTexture;
	ShaderGraph mGraph;

	void onOpen() override;


	void render() override;


	template< typename TNode, typename ...TArgs>
	std::shared_ptr<TNode> createNode(TArgs&& ...args)
	{
		std::shared_ptr<TNode> result = mGraph.createNode<TNode>(std::forward<TArgs>(args)...);
		return result;
	}


	bool bRealTimePreview = true;
	ImNode::EditorContext* mEditorContext;

	virtual void tick(float deltaTime) override;

};


#endif // ShaderGraphPanel_H_48C9804A_A350_41B7_9B82_B55520778564
