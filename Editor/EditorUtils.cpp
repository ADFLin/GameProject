#include "EditorUtils.h"

#include "RHI/D3D11Common.h"

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
	if ( id = INDEX_NONE )
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
}

ImTextureID FImGui::IconTextureID()
{
	auto resViewImpl = static_cast<D3D11ShaderResourceView*>(mIconAtlas.getTexture().getBaseResourceView());
	return resViewImpl->getResource();
}

