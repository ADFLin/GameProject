#include "EditorUtils.h"

#include "RHI/D3D11Common.h"
#include "RHI/D3D11Command.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui_internal.h"

using namespace Render;

Render::TextureAtlas FImGui::mIconAtlas;
std::vector<FImGui::Rect> FImGui::mCachedIconUVs;


void FImGui::InitializeRHI()
{
	mIconAtlas.initialize(ETexture::RGBA8, 2048, 2048, 2);
	mCachedIconUVs.resize((int)EIconId::Count);

	auto AddIcon = [](EIconId::Type id, char const* path)
	{
		if (!mIconAtlas.addImageFile(id, path))
		{
			LogWarning(0, "Can't Add Icon");
			return;
		}
		Vector2 uvMin, uvMax;
		mIconAtlas.getRectUV(id, uvMin, uvMax);
		auto& rect = mCachedIconUVs[id];
		rect.min = FImGuiConv::To(uvMin);
		rect.max = FImGuiConv::To(uvMax);
	};

	AddIcon(EIconId::Folder, "Editor/folder.png");
	AddIcon(EIconId::Document, "Editor/document.png");
	AddIcon(EIconId::FolderOpen, "Editor/FolderOpen.png");
	AddIcon(EIconId::FolderClosed, "Editor/FolderClosed.png");
	AddIcon(EIconId::CircleArrowLeft, "Editor/circle-arrow-left.png");
	AddIcon(EIconId::CircleArrowRight, "Editor/circle-arrow-right.png");
	AddIcon(EIconId::CircleArrowUp, "Editor/circle-arrow-up.png");
	AddIcon(EIconId::CircleArrowDown, "Editor/circle-arrow-down.png");
}

void FImGui::ReleaseRHI()
{
	mIconAtlas.finalize();
}

void FImGui::Icon(int id, ImVec2 const& size, ImVec4 const& color)
{
	auto& rect = mCachedIconUVs[(int)id];
	ImGui::Image(IconTextureID(), size, rect.min, rect.max, color);
}

bool FImGui::IconButton(int id, ImVec2 const& size, ImVec4 const& tintColor, ImVec4 const& bgColor, int framePadding)
{
	auto& rect = mCachedIconUVs[(int)id];
	return ImGui::ImageButton(IconTextureID(), size, rect.min, rect.max,framePadding, bgColor, tintColor);
}

int FImGui::ReigisterIcon(char const* path)
{
	int id = mIconAtlas.addImageFile(path);
	if ( id == INDEX_NONE )
	{
		LogWarning(0, "Can't Add Icon");
		return INDEX_NONE;
	}
	Vector2 uvMin, uvMax;
	mIconAtlas.getRectUV(id, uvMin, uvMax);
	Rect rect;
	rect.min = FImGuiConv::To(uvMin);
	rect.max = FImGuiConv::To(uvMax);
	mCachedIconUVs.push_back(rect);

	return id;
}

ImTextureID FImGui::IconTextureID()
{
	return GetTextureID(mIconAtlas.getTexture());
}

ImTextureID FImGui::GetTextureID(Render::RHITexture2D& texture)
{
	auto resViewImpl = static_cast<D3D11ShaderResourceView*>(texture.getBaseResourceView());
	return resViewImpl->getResource();
}

bool FImGui::Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size /*= -1.0f*/)
{
	using namespace ImGui;
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImGuiID id = window->GetID("##Splitter");
	ImRect bb;
	bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
	bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
	return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

struct BlendState
{
	TComPtr< ID3D11BlendState > resurce;
	FLOAT factor[4];
	UINT  mask;

	bool bDisabled = false;

} GBlendState;


static void DisableBlendCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	CHECK(!GBlendState.bDisabled);
	static_cast<D3D11System*>(GRHISystem)->mDeviceContext->OMGetBlendState(&GBlendState.resurce, GBlendState.factor, &GBlendState.mask);
	static_cast<D3D11System*>(GRHISystem)->mDeviceContext->OMSetBlendState(nullptr, Vector4(0, 0, 0, 0), 0xffffffff);

	GBlendState.bDisabled = true;
}


static void RestoreBlendCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	CHECK(GBlendState.bDisabled);
	static_cast<D3D11System*>(GRHISystem)->mDeviceContext->OMSetBlendState(GBlendState.resurce, GBlendState.factor, GBlendState.mask);

	GBlendState.resurce.reset();
	GBlendState.bDisabled = false;
}

void FImGui::DisableBlend()
{
	ImGui::GetWindowDrawList()->AddCallback(DisableBlendCallback, nullptr);
}


void FImGui::RestoreBlend()
{
	ImGui::GetWindowDrawList()->AddCallback(RestoreBlendCallback, nullptr);
}

