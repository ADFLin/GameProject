#ifndef MaterialGraphPanel_h__
#define MaterialGraphPanel_h__

#pragma once
#include "EditorPanel.h"

#include "ImGuiNode/imgui_node_editor.h"

namespace ImNode = ax::NodeEditor;

class MaterialGraphPanel : public IEditorPanel
{
public:

	ImNode::EditorContext* mNodeContext = nullptr;

	struct LinkInfo
	{
		ImNode::LinkId Id;
		ImNode::PinId  InputId;
		ImNode::PinId  OutputId;
	};


	void ImGuiEx_BeginColumn()
	{
		ImGui::BeginGroup();
	}

	void ImGuiEx_NextColumn()
	{
		ImGui::EndGroup();
		ImGui::SameLine();
		ImGui::BeginGroup();
	}

	void ImGuiEx_EndColumn()
	{
		ImGui::EndGroup();
	}

	void render() override
	{
		auto& io = ImGui::GetIO();

		ImNode::SetCurrentEditor(mNodeContext);

		ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

		ImGui::Separator();


		// Start interaction with editor.
		ImNode::Begin("My Editor", ImVec2(0.0, 0.0f));

		int uniqueId = 1;

		//
		// 1) Commit known data to editor
		//

		// Submit Node A
		ImNode::NodeId nodeA_Id = uniqueId++;
		ImNode::PinId  nodeA_InputPinId = uniqueId++;
		ImNode::PinId  nodeA_OutputPinId = uniqueId++;

		if (m_FirstFrame)
			ImNode::SetNodePosition(nodeA_Id, ImVec2(10, 10));
		ImNode::BeginNode(nodeA_Id);
		ImGui::Text("Node A");
		ImNode::BeginPin(nodeA_InputPinId, ImNode::PinKind::Input);
		ImGui::Text("-> In");
		ImNode::EndPin();
		ImGui::SameLine();
		ImNode::BeginPin(nodeA_OutputPinId, ImNode::PinKind::Output);
		ImGui::Text("Out ->");
		ImNode::EndPin();
		ImNode::EndNode();

		// Submit Node B
		ImNode::NodeId nodeB_Id = uniqueId++;
		ImNode::PinId  nodeB_InputPinId1 = uniqueId++;
		ImNode::PinId  nodeB_InputPinId2 = uniqueId++;
		ImNode::PinId  nodeB_OutputPinId = uniqueId++;

		if (m_FirstFrame)
			ImNode::SetNodePosition(nodeB_Id, ImVec2(210, 60));

		ImNode::BeginNode(nodeB_Id);
		ImGui::Text("Node B");
		ImGuiEx_BeginColumn();
		ImNode::BeginPin(nodeB_InputPinId1, ImNode::PinKind::Input);
		ImGui::Text("-> In1");
		ImNode::EndPin();
		ImNode::BeginPin(nodeB_InputPinId2, ImNode::PinKind::Input);
		ImGui::Text("-> In2");
		ImNode::EndPin();
		ImGuiEx_NextColumn();
		ImNode::BeginPin(nodeB_OutputPinId, ImNode::PinKind::Output);
		ImGui::Text("Out ->");
		ImNode::EndPin();
		ImGuiEx_EndColumn();
		ImNode::EndNode();

		// Submit Links
		for (auto& linkInfo : m_Links)
			ImNode::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

		//
		// 2) Handle interactions
		//

		// Handle creation action, returns true if editor want to create new object (node or link)
		if (ImNode::BeginCreate())
		{
			ImNode::PinId inputPinId, outputPinId;
			if (ImNode::QueryNewLink(&inputPinId, &outputPinId))
			{
				// QueryNewLink returns true if editor want to create new link between pins.
				//
				// Link can be created only for two valid pins, it is up to you to
				// validate if connection make sense. Editor is happy to make any.
				//
				// Link always goes from input to output. User may choose to drag
				// link from output pin or input pin. This determine which pin ids
				// are valid and which are not:
				//   * input valid, output invalid - user started to drag new ling from input pin
				//   * input invalid, output valid - user started to drag new ling from output pin
				//   * input valid, output valid   - user dragged link over other pin, can be validated

				if (inputPinId && outputPinId) // both are valid, let's accept link
				{
					// ed::AcceptNewItem() return true when user release mouse button.
					if (ImNode::AcceptNewItem())
					{
						// Since we accepted new link, lets add one to our list of links.
						m_Links.push_back({ ImNode::LinkId(m_NextLinkId++), inputPinId, outputPinId });

						// Draw new link.
						ImNode::Link(m_Links.back().Id, m_Links.back().InputId, m_Links.back().OutputId);
					}

					// You may choose to reject connection between these nodes
					// by calling ed::RejectNewItem(). This will allow editor to give
					// visual feedback by changing link thickness and color.
				}
			}
		}
		ImNode::EndCreate(); // Wraps up object creation action handling.


		// Handle deletion action
		if (ImNode::BeginDelete())
		{
			// There may be many links marked for deletion, let's loop over them.
			ImNode::LinkId deletedLinkId;
			while (ImNode::QueryDeletedLink(&deletedLinkId))
			{
				// If you agree that link can be deleted, accept deletion.
				if (ImNode::AcceptDeletedItem())
				{
					// Then remove link from your data.
					for (auto& link : m_Links)
					{
						if (link.Id == deletedLinkId)
						{
							m_Links.erase(&link);
							break;
						}
					}
				}

				// You may reject link deletion by calling:
				// ed::RejectDeletedItem();
			}
		}
		ImNode::EndDelete(); // Wrap up deletion action



		// End of interaction with editor.
		ImNode::End();

		if (m_FirstFrame)
			ImNode::NavigateToContent(0.0f);

		ImNode::SetCurrentEditor(nullptr);

		m_FirstFrame = false;

		// ImGui::ShowMetricsWindow();
	}

	bool                 m_FirstFrame = true;    // Flag set for first frame only, some action need to be executed once.
	ImVector<LinkInfo>   m_Links;                // List of live links. It is dynamic unless you want to create read-only view over nodes.
	int                  m_NextLinkId = 100;     // Counter to help generate link ids. In real application this will probably based on pointer to user data structure.


	virtual void onOpen() override;


	virtual void onClose() override;

};

#endif // MaterialGraphPanel_h__
