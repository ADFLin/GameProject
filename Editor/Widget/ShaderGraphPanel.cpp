#include "ShaderGraphPanel.h"

#include "EditorUtils.h"

#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/DrawUtility.h"
#include "ProfileSystem.h"
#include "imgui_internal.h"

REGISTER_EDITOR_PANEL(ShaderGraphPanel, "ShaderGraph", true, true);




void ShaderGraphPanel::runTest()
{
	mGraph.setDoamin(ESGDomainType::Test);

	auto nodeA = createNode<SGNodeConst>(2.0);
	auto nodeB = createNode<SGNodeConst>(3.0);
	auto nodeOp = createNode<SGNodeOperator>(ESGValueOperator::Mul);
	auto nodeUV = createNode<SGNodeTexcooord>();
	auto nodeFunc = createNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Cos);
	auto nodeTime = createNode<SGNodeTime>(true);

	nodeA->linkTo(*nodeOp, 0);
	nodeUV->linkTo(*nodeOp, 1);
	nodeUV->linkTo(*nodeFunc, 0);

	mGraph.linkDomainInput(*nodeFunc, 0);

	if (compileShader())
	{
		renderShaderPreview(TVector2<int>(200,200));
	}

}

bool ShaderGraphPanel::compileShader()
{
	SGCompilerCodeGen compiler;
	std::string code;

	if (!compiler.generate(mGraph, code))
		return false;

	using namespace Render;
	ShaderCompileOption option;
	return ShaderManager::Get().loadFile(mShaderProgram, nullptr, "ScreenVS", "MainPS", option, code.c_str());
}

void ShaderGraphPanel::renderShaderPreview(TVector2<int> const& size)
{
	using namespace Render;

	if (mShaderProgram.getRHI() == nullptr)
		return;

	if (mFrameBuffer.isValid() == false)
	{
		mFrameBuffer = RHICreateFrameBuffer();
	}
	if ( mTexture.isValid() == false || ( mTexture->getSizeX() != size.x || mTexture->getSizeY() != size.y ) )
	{
		mTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::BGRA8, size.x, size.y).Flags(TCF_RenderTarget | TCF_CreateSRV | TCF_DefalutValue));
		mFrameBuffer->setTexture(0 ,*mTexture);
	}
	RHICommandList& commandList = RHICommandList::GetImmediateList();
	RHIBeginRender();

	RHISetFrameBuffer(commandList, mFrameBuffer);
	RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);

	RHISetViewport(commandList, 0, 0, size.x, size.y);

	RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

	RHISetShaderProgram(commandList, mShaderProgram.getRHI());
	mView.setupShader(commandList, mShaderProgram);
	DrawUtility::ScreenRect(commandList);

	RHIFlushCommand(commandList);
	RHIEndRender(false);

}

void ShaderGraphPanel::onOpen()
{
	ImNode::Config config;

	//config.SettingsFile = "Blueprints.json";
	config.UserPointer = this;

#if 0
	config.LoadNodeSettings = [](ImNode::NodeId nodeId, char* data, void* userPointer) -> size_t
	{
		return node->State.size();
	};

	config.SaveNodeSettings = [](ImNode::NodeId nodeId, const char* data, size_t size, ImNode::SaveReasonFlags reason, void* userPointer) -> bool
	{
		return true;
	};
#endif

	mEditorContext = ImNode::CreateEditor(&config);
	ImNode::SetCurrentEditor(mEditorContext);

	mView.gameTime = 0;
	mView.realTime = 0;

	if (mNodesList.empty())
	{
		registerNode<SGNodeOperator>(ESGValueOperator::Add);
		registerNode<SGNodeOperator>(ESGValueOperator::Sub);
		registerNode<SGNodeOperator>(ESGValueOperator::Mul);
		registerNode<SGNodeOperator>(ESGValueOperator::Div);
		registerNode<SGNodeOperator>(ESGValueOperator::Dot);
		registerNode<SGNodeConst>(0);
		registerNode<SGNodeConst>(0,0);
		registerNode<SGNodeConst>(0,0,0);
		registerNode<SGNodeConst>(0,0,0,0);
		registerNode<SGNodeTexcooord>(0);
		registerNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Abs);
		registerNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Cos);
		registerNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Sin);
		registerNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Tan);
		registerNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Exp);
		registerNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Log);
		registerNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Frac);
		registerNode<SGNodeCustom>(ESGValueType::Float2);
	}

	runTest();
}

static ImRect ImGui_GetItemRect()
{
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

class NodeGUIVisitor : public SGNodeVisitor
{
public:
	void visit(class SGNode& node) override
	{
		ImGui::BeginHorizontal("content");
		ImGui::Spring(1, 0);
		ImGui::TextUnformatted(node.getTitle().c_str());
		ImGui::Spring(1, 0);
		ImGui::EndHorizontal();
		drawLinkPin(node);
	}

	void visit(class SGNodeCustom& node) override
	{
		ImGui::BeginHorizontal("content");
		ImGui::Spring(1, 0);
		ImGui::TextUnformatted(node.getTitle().c_str());
		ImGui::Spring(1, 0);
		ImGui::EndHorizontal();
		drawLinkCustomPin(node);
	}

	void visit(class SGNodeConst& node) override
	{


		//ImGui::BeginHorizontal("content");
		//ImGui::Spring(1, 0);


		ImGui::PushID((intptr_t)&node);
		switch (node.type)
		{
		case ESGValueType::Float1:
			ImGui::SetNextItemWidth(100);
			ImGui::InputFloat("##Object", node.value);
			break;
		case ESGValueType::Float2:
			ImGui::SetNextItemWidth(150);
			ImGui::InputFloat2("##Object", node.value);
			break;
		case ESGValueType::Float3:
			ImGui::SetNextItemWidth(200);
			ImGui::InputFloat3("##Object", node.value);
			break;
		case ESGValueType::Float4:
			ImGui::SetNextItemWidth(250);
			ImGui::InputFloat4("##Object", node.value);
			break;
		}
		ImGui::PopID();
		//ImGui::Spring(1, 0);
		//ImGui::EndHorizontal();
		drawLinkPin(node);


	}
	void drawLinkCustomPin(SGNodeCustom& node)
	{
		struct Funcs
		{
			static int MyResizeCallback(ImGuiInputTextCallbackData* data)
			{
				if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
				{
					std::string& my_str = *reinterpret_cast<std::string*>(data->UserData);
					IM_ASSERT(my_str.data() == data->Buf);
					my_str.resize(data->BufSize); // NB: On resizing calls, generally data->BufSize == data->BufTextLen + 1
					data->Buf = my_str.data();
				}
				return 0;
			}

			// Note: Because ImGui:: is a namespace you would typically add your own function into the namespace.
			// For example, you code may declare a function 'ImGui::InputText(const char* label, MyString* my_str)'
			static bool InputTextMultiline(const char* label, std::string& my_str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0)
			{
				IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
				return ImGui::InputTextMultiline(label, my_str.data(), (size_t)my_str.size(), size, flags | ImGuiInputTextFlags_CallbackResize, Funcs::MyResizeCallback, (void*)&my_str);
			}


			static bool InputText(const char* label, std::string& my_str, ImGuiInputTextFlags flags = 0)
			{
				IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
				return ImGui::InputText(label, my_str.data(), (size_t)my_str.size(), flags | ImGuiInputTextFlags_CallbackResize, Funcs::MyResizeCallback, (void*)&my_str);
			}
		};

		int maxIndex = Math::Max<int>(node.inputs.size() + 1, 2u);
		bool bAdd = false;
		for (int index = 0; index < maxIndex; ++index)
		{
			InlineString<> inputStr;
			inputStr.format("input%d", index);
			ImGui::BeginHorizontal(inputStr);

			if (node.inputs.isValidIndex(index))
			{
				ImNode::BeginPin((intptr_t)&node.inputs[index], ImNode::PinKind::Input);
				ImNode::PinPivotAlignment(ImVec2(0.0f, 0.5f));
				ImGui::PushID(index);
				ImGui::SetNextItemWidth(50);
				Funcs::InputText("##Input", node.inputNames[index]);
				ImGui::PopID();
				ImNode::EndPin();
			}
			else if ( bAdd == false )
			{
				bAdd = true;
				ImGui::BeginHorizontal("Add");
				if (ImGui::Button("+"))
				{
					node.addInput();
				}
				if (ImGui::Button("-"))
				{
					node.removeLastInput();
				}
				ImGui::EndHorizontal();
			}
			ImGui::Spring(1, 10);

			if (index == 0)
			{
				ImNode::BeginPin((intptr_t)&node.outputLinks, ImNode::PinKind::Output);
				ImNode::PinPivotAlignment(ImVec2(1.0f, 0.5f));
				ImGui::TextUnformatted("Out");
				ImNode::EndPin();
			}

			ImGui::EndHorizontal();
		}

		ImGui::BeginHorizontal("Code");
		ImGui::PushID((intptr_t)&node);
		ImGui::SetNextItemWidth(200);
		ImGuiInputTextFlags textFlags = 0;
			//ImGuiInputTextFlags_EnterReturnsTrue |
			//ImGuiInputTextFlags_AllowTabInput |
			//ImGuiInputTextFlags_CtrlEnterForNewLine;
		if (Funcs::InputText("##Object", node.code, textFlags))
		{

		}
		ImGui::PopID();
		ImGui::EndHorizontal();
	}

	void drawLinkPin(SGNode& node)
	{
		int maxIndex = Math::Max<int>(node.inputs.size(), 1u);
		for(int index = 0 ; index < maxIndex; ++index)
		{
			InlineString<> inputStr;
			inputStr.format("input%d", index);
			ImGui::BeginHorizontal(inputStr);

			if (node.inputs.isValidIndex(index))
			{
				ImNode::BeginPin((intptr_t)&node.inputs[index], ImNode::PinKind::Input);
				ImNode::PinPivotAlignment(ImVec2(0.0f, 0.5f));
				ImGui::TextUnformatted("Input");
				ImNode::EndPin();
			}

			ImGui::Spring(1, 10);

			if (index == 0)
			{
				ImNode::BeginPin((intptr_t)&node.outputLinks, ImNode::PinKind::Output);
				ImNode::PinPivotAlignment(ImVec2(1.0f, 0.5f));
				ImGui::TextUnformatted("Out");
				ImNode::EndPin();
			}

			ImGui::EndHorizontal();
		}
	}

};

void ShaderGraphPanel::render()
{
	PROFILE_ENTRY("ShaderGraphPanel");

	bool bRenderPreviewRequest = false;
	ImNode::SetCurrentEditor(mEditorContext);


	if (ImGui::Button("Compile"))
	{
		if (compileShader())
		{
			bRenderPreviewRequest = true;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Fit to content"))
	{
		ImNode::NavigateToContent();
	}

	static float leftPaneWidth = 400.0f;
	static float rightPaneWidth = 200.0f;
	float temp = ImGui::GetContentRegionAvail().x - leftPaneWidth;

	FImGui::Splitter("##Left",true, 2.0f, &leftPaneWidth, &temp, 50.0f, rightPaneWidth);


	if (bRealTimePreview || bRenderPreviewRequest)
	{
		mView.updateRHIResource();
		renderShaderPreview(TVector2<int>(leftPaneWidth, leftPaneWidth));
	}

	ImGui::BeginChild("Selection", ImVec2(leftPaneWidth - 4, 0));

	if (mTexture.isValid())
	{
		ImGui::Image(FImGui::GetTextureID(*mTexture), ImVec2(leftPaneWidth, leftPaneWidth));
	}


#if 1
	ImGui::BeginChild("Selection3");
	for (auto& node : mNodesList)
	{
		if (ImGui::Button(node->getTitle().c_str()))
		{
			mGraph.nodes.push_back(SGNodePtr(node->copy()));
			//ImNode::SetNodePosition( (intptr_t)&*mGraph.nodes.back() , ImGui::GetWindowPos() );
		}
	}
	ImGui::EndChild();
#endif
	ImGui::EndChild();

	ImGui::SameLine(0.0f, 8.0f);

	temp -= rightPaneWidth;
	//FImGui::Splitter("##Right", true, 2.0f, &temp, &rightPaneWidth, 50.0f, 50.0f);

	//ImGui::BeginChild("Graph", ImVec2(temp - 4, 0));

	//ImGui::SetNextItemWidth(temp - 4);

	ImNode::Begin("Node editor");

	NodeGUIVisitor visitor;
	for (auto& nodePtr : mGraph.nodes)
	{
		SGNode& node = *nodePtr;
		ImNode::BeginNode((intptr_t)&node);
		ImGui::BeginVertical((intptr_t)&node);
		node.acceptVisit(visitor);
		ImGui::EndVertical();
		ImNode::EndNode();
	}


	ImNode::PushStyleColor(ImNode::StyleColor_NodeBg, ImColor(128, 128, 128, 200));
	ImNode::PushStyleColor(ImNode::StyleColor_NodeBorder, ImColor(32, 32, 32, 200));
	ImNode::PushStyleColor(ImNode::StyleColor_PinRect, ImColor(60, 180, 255, 150));
	ImNode::PushStyleColor(ImNode::StyleColor_PinRectBorder, ImColor(60, 180, 255, 150));
	ImNode::BeginNode((intptr_t)&mGraph);
	switch (mGraph.mDomain)
	{
	case ESGDomainType::Test:
		{
			char const* inputNames[] = { "Color" };
			int index = 0;
			for (auto& input : mGraph.domainInputs)
			{
				ImNode::BeginPin((intptr_t)&input, ImNode::PinKind::Input);
				ImGui::Text(inputNames[index]);
				ImNode::PinPivotAlignment(ImVec2(0.0f, 0.5f));
				ImNode::EndPin();
				++index;
			}
		}
		break;
	}
	ImNode::EndNode();
	ImNode::PopStyleColor(4);


	for (auto& node : mGraph.nodes)
	{
		for (auto& link : node->outputLinks)
		{
			ImNode::Link((intptr_t)&link,(intptr_t)node->getOutputId(), (intptr_t)&link.getIntput());
		}	
	}

	for (auto& input : mGraph.domainInputs)
	{
		if (!input.link.expired())
		{
			ImNode::Link((intptr_t)&input, (intptr_t)input.link.lock()->getOutputId(), (intptr_t)&input);
		}
	}

	// Handle creation action, returns true if editor want to create new object (node or link)
	if (ImNode::BeginCreate())
	{
		ImNode::PinId startPinId, endPinId;
		if (ImNode::QueryNewLink(&startPinId, &endPinId))
		{
			if (startPinId && endPinId) // both are valid, let's accept link
			{

				if (startPinId == endPinId)
				{

				}
				else
				{
					bool bValidLink = true;
					if (!mGraph.isNodeOutputId(startPinId.AsPointer<void>()))
					{
						using namespace std;
						swap(startPinId, endPinId);
						if (!mGraph.isNodeOutputId(startPinId.AsPointer<void>()))
						{
							bValidLink = false;
						}
					}
					// ed::AcceptNewItem() return true when user release mouse button.
					if (bValidLink)
					{
						if (ImNode::AcceptNewItem())
						{
							SGNode* node = SGNode::GetFromOuputId(startPinId.AsPointer<void>());
							SGNodeInput* input = endPinId.AsPointer<SGNodeInput>();
							// Since we accepted new link, lets add one to our list of links.
							int inputIndex;
							if (mGraph.isDomainInput(input, inputIndex))
							{
								mGraph.linkDomainInput(*node, inputIndex);
							}
							else
							{
								SGNode* inputNode = mGraph.findNode(input, inputIndex);
								if (inputNode)
								{
									node->linkTo(*inputNode, inputIndex);
								}
							}
						}

					}
					else
					{
						ImNode::RejectNewItem();
					}
				}

			}
		}
	}
	ImNode::EndCreate(); // Wraps up object creation action handling.

	ImNode::End();

	//ImGui::EndChild();

	//ImGui::SameLine(0.0f, 8.0f);

}

void ShaderGraphPanel::tick(float deltaTime)
{
	mView.gameTime += deltaTime;
	mView.realTime += deltaTime;
	mView.mbDataDirty = true;
}
