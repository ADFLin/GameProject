#pragma once

#ifndef EditorDrawUtil_H_3F0AAB5D_6D2C_4B73_8A9B_44986611DA1D
#define EditorDrawUtil_H_3F0AAB5D_6D2C_4B73_8A9B_44986611DA1D

#include "RHI/TextureAtlas.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "Core/FNV1a.h"
#include "RHI/Font.h"

using ::Math::Vector2;

struct FImGuiConv
{
	FORCEINLINE static ImVec4 To(Color4f const& value) { return *reinterpret_cast<ImVec4 const*>(&value); }
	FORCEINLINE static ImVec2 To(Vector2 const& value) { return *reinterpret_cast<ImVec2 const*>(&value); }

	FORCEINLINE static Vector2 To(ImVec2 const& value ){ return *reinterpret_cast<Vector2 const*>(&value); }
};

struct FEditor
{
	static uint32 Hash(StringView const& str, uint32 mask = 0xffff)
	{
		return FNV1a::MakeStringHashIgnoreCase<uint32>(str.data(), str.length()) & mask;
	}
	static constexpr uint32 Hash(char const* str, uint32 mask = 0xffff)
	{
		return FNV1a::MakeStringHashIgnoreCase<uint32>(str) & mask;
	}
};


namespace EIconId
{
	enum Type
	{
		Folder,
		Document,
		FolderOpen,
		FolderClosed,

		CircleArrowLeft,
		CircleArrowRight,
		CircleArrowUp,
		CircleArrowDown,
		Count,
	};
};

struct FImGui
{
	static void InitializeRHI();
	static void ReleaseRHI();

	static void Icon(int id, ImVec2 const& size, ImVec4 const& tintColor = ImVec4(1,1,1,1));
	static bool IconButton(int id, ImVec2 const& size, ImVec4 const& tintColor = ImVec4(1,1,1,1), ImVec4 const& bgColor = ImVec4(0,0,0,0), int framePadding = -1);
	static int ReigisterIcon(char const* path);

	static ImTextureID IconTextureID();

	static ImTextureID GetTextureID(Render::RHITexture2D& texture);

	static bool Splitter(char const* pid, bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);

	static void DisableBlend();
	static void RestoreBlend();

	static Render::TextureAtlas mIconAtlas;
	static Render::FontDrawer   mFont;
	struct Rect
	{
		ImVec2 min;
		ImVec2 max;
	};
	static TArray<Rect> mCachedIconUVs;

};
#endif // EditorDrawUtil_H_3F0AAB5D_6D2C_4B73_8A9B_44986611DA1D
