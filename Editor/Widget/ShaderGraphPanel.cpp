#include "ShaderGraphPanel.h"

#include "EditorUtils.h"

#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/DrawUtility.h"

REGISTER_EDITOR_PANEL(ShaderGraphPanel, "ShaderGraph", true, true);


void ShaderGraphPanel::runTest()
{
	mGraph.setDoamin(ESGDomainType::Test);

	auto nodeA = mGraph.createNode<SGNodeConst>(2.0);
	auto nodeB = mGraph.createNode<SGNodeConst>(3.0);
	auto nodeOp = mGraph.createNode<SGNodeOperator>(ESGValueOperator::Add);
	auto nodeUV = mGraph.createNode<SGNodeTexcooord>();
	auto nodeFunc = mGraph.createNode<SGNodeIntrinsicFunc>(ESGIntrinsicFunc::Cos);

	nodeA->linkOutput(*nodeOp, 0);
	nodeUV->linkOutput(*nodeOp, 1);
	nodeUV->linkOutput(*nodeFunc, 0);

	mGraph.linkDomainInput(*nodeFunc, 0);

	SGCompilerCodeGen compiler;
	std::string code;

	if (compiler.generate(mGraph, code))
	{
		using namespace Render;
		ShaderCompileOption option;
		ShaderManager::Get().loadFile(mShaderProgram, nullptr, "ScreenVS", "MainPS", option, code.c_str());

		mTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::BGRA8, 200, 200).Flags(TCF_RenderTarget | TCF_CreateSRV | TCF_DefalutValue));

		mFrameBuffer = RHICreateFrameBuffer();
		mFrameBuffer->addTexture(*mTexture);

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHIBeginRender();

		RHISetFrameBuffer(commandList, mFrameBuffer);
		RHISetViewport(commandList, 0, 0, 200, 200);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(1, 1, 0, 1), 1);
		RHISetShaderProgram(commandList, mShaderProgram.getRHI());

		RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

		DrawUtility::ScreenRect(commandList);

		RHISetFrameBuffer(commandList, nullptr);

		RHIFlushCommand(commandList);
		RHIEndRender(false);
	}
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

	runTest();
}

void ShaderGraphPanel::render()
{

	ImNode::SetCurrentEditor(mEditorContext);

	static float leftPaneWidth = 400.0f;
	static float rightPaneWidth = 800.0f;
	FImGui::Splitter(true, 2.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f);

	ImGui::BeginChild("Selection", ImVec2(leftPaneWidth - 4, 0));

	if (mTexture.isValid())
	{
		ImGui::Image(FImGui::GetTextureID(*mTexture), ImVec2(200, 200));
	}

	ImGui::EndChild();

	ImGui::SameLine(0.0f, 12.0f);

	ImNode::Begin("Node editor");


	for (auto& node : mGraph.nodes)
	{




	}
	ImNode::End();
}
