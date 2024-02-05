#include "TextureViewerPanel.h"

#include "EditorUtils.h"
#include "Renderer/SceneDebug.h"
#include "imgui_internal.h"
#include "RHI/RHICommand.h"
#include "Math/Vector4.h"
#include "RHI/DrawUtility.h"


REGISTER_EDITOR_PANEL(TextureViewerPanel, TextureViewerPanel::ClassName, true, true);

void TextureViewerPanel::render()
{
	int width = FImGui::mIconAtlas.getTexture().getSizeX();
	int height = FImGui::mIconAtlas.getTexture().getSizeY();

	Render::RHITexture2D* texture = nullptr;
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Type");
	ImVec2 lineMin = ImGui::GetItemRectMin();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200);


	char const* DefaultTextureNames[] =
	{
		"Icon", "Font",
	};
	char const* viewTextureName = "";
	if (selection < ARRAY_SIZE(DefaultTextureNames))
	{
		viewTextureName = DefaultTextureNames[selection];
	}
	else if (mTextureShowManager)
	{
		int index = ARRAY_SIZE(DefaultTextureNames);
		for (auto const& pair : mTextureShowManager->getTextureMap())
		{
			if (index == selection)
			{
				viewTextureName = pair.first.c_str();
				break;
			}
			++index;
		}
	}

	if (ImGui::BeginCombo("##TextureList", viewTextureName))
	{
		int index = 0;
		for (auto name : DefaultTextureNames)
		{
			const bool is_selected = (selection == index);
			if (ImGui::Selectable(name, is_selected))
				selection = index;

			if (is_selected)
				ImGui::SetItemDefaultFocus();
			++index;

		}
		if (mTextureShowManager)
		{
			for (auto const& pair : mTextureShowManager->getTextureMap())
			{
				const bool is_selected = (selection == index);
				if (ImGui::Selectable(pair.first.c_str(), is_selected))
				{
					selection = index;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
				++index;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		bMaskR = bMaskG = bMaskB = bMaskA = true;
		gamma = 1.0f;
		scale = 1.0f;
		sizeScale = 1.0f;
	}

	ImGui::SameLine();
	ImGui::Checkbox("R", &bMaskR);
	ImGui::SameLine();
	ImGui::Checkbox("G", &bMaskG);
	ImGui::SameLine();
	ImGui::Checkbox("B", &bMaskB);
	ImGui::SameLine();
	ImGui::Checkbox("A", &bMaskA);
	ImGui::SameLine();
	ImGui::Text("Gamma");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##Gamma", &gamma, 0.1, -10 , 10 , "%g");
	ImGui::SameLine();
	ImGui::Text("Scale");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragFloat("##Scale", &scale, 0.01, 0.01, 10, "%g");
	ImGui::SameLine();
	ImGui::Text("SizeScale");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200);
	ImGui::DragFloat("##SizeScale", &sizeScale, 0.1, 0.0f, 10.0f);

	ImVec2 lineMax = ImGui::GetItemRectMax();
	switch (selection)
	{
	case 0: texture = &FImGui::mIconAtlas.getTexture(); break;
	case 1: texture = &Render::FontCharCache::Get().getTexture(); break;
	default:
		if (mTextureShowManager)
		{
			int index = ARRAY_SIZE(DefaultTextureNames);
			for (auto& pair : mTextureShowManager->getTextureMap())
			{
				if (index == selection)
				{
					if (pair.second->texture.get())
					{
						texture = const_cast<Render::RHITextureBase*>(pair.second->texture.get())->getTexture2D();
					}
					break;
				}
				++index;
			}
		}
		break;
	}


	if (texture)
	{
		width = texture->getSizeX();
		height = texture->getSizeY();
	}
	Math::Vector2 renderSize = Vector2(sizeScale * width, sizeScale * height);
	ImGui::BeginChild("Split", ImVec2(0,0), false);
	if (texture)
	{
		bool bUseShader = !bMaskR || !bMaskG || !bMaskB || gamma != 1.0 || scale != 1.0;

		if (bUseShader)
		{
			Color4f colorMask;
			colorMask.r = bMaskR ? 1 : 0;
			colorMask.g = bMaskG ? 1 : 0;
			colorMask.b = bMaskB ? 1 : 0;
			colorMask.a = bMaskA ? 1 : 0;

			struct RenderData
			{
				Math::Vector2 clientPos;
				Math::Vector2 clientSize;
				Math::Vector2 windowPos;
				Math::Vector2 windowSize;
				ImGuiViewport* viewport;
			};
			ImGuiWindow* window = ImGui::GetCurrentWindowRead();

			RenderData data;
			data.clientPos = FImGuiConv::To(ImGui::GetCursorScreenPos() - ImGui::GetWindowViewport()->Pos);
			data.clientSize = FImGuiConv::To(window->ContentSize);
			data.windowPos = FImGuiConv::To(ImGui::GetWindowPos() - ImGui::GetWindowViewport()->Pos);
			data.windowSize = FImGuiConv::To(ImGui::GetWindowSize());
			data.viewport = ImGui::GetWindowViewport();

			Math::Vector3 mappingParams;
			mappingParams.x = scale;
			mappingParams.y = 0.0f;
			mappingParams.z = gamma;

			ImGui::Dummy(FImGuiConv::To(renderSize));

			EditorRenderGloabal::Get().addCustomFunc([=](const ImDrawList* parentlist, const ImDrawCmd* cmd)
			{
				using namespace Render;
				ImDrawData* drawData = data.viewport->DrawData;

				RHICommandList& commandList = RHICommandList::GetImmediateList();
				RHIBeginRender();
				RHISetViewport(commandList, 0, 0, drawData->DisplaySize.x, drawData->DisplaySize.y);

				Vector2 clipPos = Vector2(cmd->ClipRect.x, cmd->ClipRect.y);
				Vector2 clipSize = Vector2(cmd->ClipRect.z, cmd->ClipRect.w) - clipPos;
				clipPos -= FImGuiConv::To(drawData->DisplayPos);
				RHISetScissorRect(commandList, clipPos.x, clipPos.y, clipSize.x, clipSize.y);
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None, EFillMode::Solid, EFrontFace::Default, true >::GetRHI());

				RHISetBlendState(commandList,
					bMaskA ? StaticTranslucentBlendState::GetRHI() : TStaticBlendState<>::GetRHI()
				);

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

				Matrix4 xForm = AdjProjectionMatrixForRHI(OrthoMatrix(0, drawData->DisplaySize.x, drawData->DisplaySize.y, 0, 1, -1));
				DrawUtility::DrawTexture(commandList, xForm, *texture, TStaticSamplerState< ESampler::Bilinear >::GetRHI(), data.clientPos, renderSize, &colorMask, &mappingParams);

				RHIEndRender(false);
			});
		}
		else if ( bMaskA )
		{
			ImGui::Image(FImGui::GetTextureID(*texture), ImVec2(sizeScale * width, sizeScale * height));
		}
		else
		{
			FImGui::DisableBlend();
			ImGui::Image(FImGui::GetTextureID(*texture), ImVec2(sizeScale * width, sizeScale * height));
			FImGui::RestoreBlend();
		}
	}
	ImGui::EndChild();
}

void TextureViewerPanel::getRenderParams(WindowRenderParams& params) const
{
	int width = FImGui::mIconAtlas.getTexture().getSizeX();
	int height = FImGui::mIconAtlas.getTexture().getSizeY();
	params.flags |= ImGuiWindowFlags_HorizontalScrollbar;
	params.InitSize = ImVec2(width, height);
}
