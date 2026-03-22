#include "SpriteSheetPanel.h"
#include "imgui.h"
#include "EditorUtils.h"
#include "Math/Base.h"
#include "SystemPlatform.h"
#include "RHI/RHIUtility.h"
#include "RHI/RHICommand.h"
#include <cstdio>
#include <cmath>

REGISTER_EDITOR_PANEL(SpriteSheetPanel, SpriteSheetPanel::ClassName, true, false);

SpriteSheetPanel::SpriteSheetPanel()
{
	mEditedSheet = new SpriteSheet();
}

void SpriteSheetPanel::getUpdateParams(WindowUpdateParams& params) const
{
	params.flags |= ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar;
	params.InitSize = ImVec2(1024, 768);
}

void SpriteSheetPanel::update()
{
	drawToolBar();

	ImGuiIO& io = ImGui::GetIO();
	if (mIsPlaying && mEditedSheet && !mSelectedAnimName.empty())
	{
		auto it = mEditedSheet->animations.find(mSelectedAnimName);
		if (it != mEditedSheet->animations.end())
		{
			SpriteAnimSequence& anim = it->second;
			if (!anim.frames.empty())
			{
				if (mCurrentPlaybackFrame >= (int)anim.frames.size())
					mCurrentPlaybackFrame = 0;

				float currentFrameDuration = anim.frames[mCurrentPlaybackFrame].duration;
				if (currentFrameDuration <= 0.0f) currentFrameDuration = 0.1f;

				mAnimTimer += io.DeltaTime;
				if (mAnimTimer >= currentFrameDuration)
				{
					mCurrentPlaybackFrame = (mCurrentPlaybackFrame + 1) % (int)anim.frames.size();
					mAnimTimer = 0.0f;
				}
			}
		}
	}

	ImGui::SetNextWindowSize(ImVec2(350, 700), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Region Properties", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar))
	{
		drawPropertiesPanel();
	}
	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(600, 700), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Animation Editor"))
	{
		drawAnimEditor();
	}
	ImGui::End();

	drawTexturePreview();
}

void SpriteSheetPanel::drawAnimEditor()
{
	if (!mEditedSheet)
	{
		ImGui::Text("No Sprite Sheet loaded.");
		return;
	}

	if (ImGui::Button("Add New Animation"))
	{
		char newName[64];
		sprintf(newName, "Anim_%d", (int)mEditedSheet->animations.size());
		HashString nameKey(newName);
		if (mEditedSheet->animations.find(nameKey) == mEditedSheet->animations.end())
		{
			SpriteAnimSequence anim;
			mEditedSheet->animations[nameKey] = anim;
			mSelectedAnimName = nameKey;
		}
	}
	ImGui::Separator();

	// Top Section: Settings & Preview
	ImGui::BeginChild("AnimTopPanel", ImVec2(0, 260), true);
	if (!mSelectedAnimName.empty() && mEditedSheet->animations.find(mSelectedAnimName) != mEditedSheet->animations.end())
	{
		auto it = mEditedSheet->animations.find(mSelectedAnimName);
		SpriteAnimSequence& anim = it->second;

		ImGui::Text("Animation:");
		ImGui::SameLine();
		char nameBuf[64];
		strncpy(nameBuf, mSelectedAnimName.c_str(), sizeof(nameBuf));
		ImGui::SetNextItemWidth(150);
		if (ImGui::InputText("##AnimName", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			HashString newName(nameBuf);
			if (!newName.empty() && newName != mSelectedAnimName && mEditedSheet->animations.find(newName) == mEditedSheet->animations.end())
			{
				mEditedSheet->animations[newName] = std::move(it->second);
				mEditedSheet->animations.erase(it);
				mSelectedAnimName = newName;
				ImGui::EndChild();
				return;
			}
		}
		ImGui::SameLine();
		ImGui::Checkbox("Loop", &anim.bLoop);
		
		if (ImGui::Button(mIsPlaying ? "STOP PLAYBACK" : "START PLAYBACK", ImVec2(200, 30)))
		{
			mIsPlaying = !mIsPlaying;
			mAnimTimer = 0;
			mCurrentPlaybackFrame = 0;
		}

		ImGui::Separator();
		ImGui::Text("Current Preview:");
		ImGui::BeginChild("PreviewFrame", ImVec2(0, 150), true);
		if (!anim.frames.empty())
		{
			int frameIdx = mIsPlaying ? mCurrentPlaybackFrame : (mSelectedFrameIndex >= 0 ? mSelectedFrameIndex : 0);
			if (frameIdx >= 0 && frameIdx < (int)anim.frames.size())
			{
				int regionIdx = anim.frames[frameIdx].regionIndex;
				if (regionIdx >= 0 && regionIdx < (int)mEditedSheet->regions.size())
				{
					SpriteRegion& r = mEditedSheet->regions[regionIdx];
					if (mEditedSheet->texture.isValid())
					{
						float aspect = r.size.x / ::Math::Max(0.01f, r.size.y);
						float h = 130.0f;
						ImVec2 u0(r.uvPos.x, r.uvPos.y);
						ImVec2 u1(r.uvPos.x + r.uvSize.x, r.uvPos.y + r.uvSize.y);
						ImGui::Image(FImGui::GetTextureID(*mEditedSheet->texture), ImVec2(h * aspect, h), u0, u1);
					}
				}
			}
		}
		ImGui::EndChild();

		if (mSelectedFrameIndex >= 0 && mSelectedFrameIndex < (int)anim.frames.size())
		{
			ImGui::Text("Selected Frame [%d] Settings", mSelectedFrameIndex);
			ImGui::SameLine(200);
			ImGui::SetNextItemWidth(150);
			ImGui::DragFloat("Duration (s)", &anim.frames[mSelectedFrameIndex].duration, 0.01f, 0.01f, 10.0f);
		}
	}
	else
	{
		ImGui::TextDisabled("Select or create an animation sequence to start editing.");
	}
	ImGui::EndChild();

	// Bottom Section: Lists
	// Column 1: Animation Sequences
	ImGui::BeginChild("AnimList", ImVec2(mAnimListWidth, 0), true);
	ImGui::TextDisabled("ANIMATIONS");
	ImGui::Separator();
	for (auto& pair : mEditedSheet->animations)
	{
		ImGui::PushID(pair.first.c_str());
		bool bSelected = (mSelectedAnimName == pair.first);
		if (ImGui::Selectable(pair.first.c_str(), bSelected))
		{
			mSelectedAnimName = pair.first;
			mSelectedFrameIndex = -1;
			mCurrentPlaybackFrame = 0;
			mAnimTimer = 0;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// Vertical Splitter for Horizontal Resize
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.2f));
	ImGui::Button("##AnimListSplitter", ImVec2(4, -1));
	ImGui::PopStyleColor(3);

	if (ImGui::IsItemActive())
	{
		mAnimListWidth += ImGui::GetIO().MouseDelta.x;
		mAnimListWidth = Math::Max(80.0f, mAnimListWidth);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}

	ImGui::SameLine();

	// Column 2: Frame List (Vertical)
	ImGui::BeginChild("FrameListColumn", ImVec2(0, 0), true);
	ImGui::TextDisabled("FRAMES");
	if (!mSelectedAnimName.empty() && mEditedSheet->animations.find(mSelectedAnimName) != mEditedSheet->animations.end())
	{
		auto it = mEditedSheet->animations.find(mSelectedAnimName);
		SpriteAnimSequence& anim = it->second;

		ImGui::SameLine(100);
		if (ImGui::Button("Add Frame") && !mSelectedRegions.empty())
		{
			for (int rIdx : mSelectedRegions)
			{
				SpriteAnimFrame frame;
				frame.regionIndex = rIdx;
				frame.duration = 0.1f;
				anim.frames.push_back(frame);
			}
			mSelectedFrameIndex = (int)anim.frames.size() - 1;
		}
		ImGui::Separator();

		ImGui::BeginChild("FrameScrollList");
		for (int f = 0; f < (int)anim.frames.size(); ++f)
		{
			ImGui::PushID(f);
			bool bSelected = (mSelectedFrameIndex == f);
			
			// Frame item layout similar to Region List
			ImGui::BeginGroup();
			char fLabel[16];
			sprintf(fLabel, "%d", f);
			
			if (ImGui::Selectable(fLabel, bSelected, 0, ImVec2(0, 50)))
			{
				mSelectedFrameIndex = f;
			}

			ImGui::SameLine(30);
			int regionIdx = anim.frames[f].regionIndex;
			if (regionIdx >= 0 && regionIdx < (int)mEditedSheet->regions.size())
			{
				SpriteRegion& r = mEditedSheet->regions[regionIdx];
				if (mEditedSheet->texture.isValid())
				{
					ImVec2 u0(r.uvPos.x, r.uvPos.y);
					ImVec2 u1(r.uvPos.x + r.uvSize.x, r.uvPos.y + r.uvSize.y);
					ImGui::Image(FImGui::GetTextureID(*mEditedSheet->texture), ImVec2(40, 40), u0, u1);
					ImGui::SameLine();
					ImGui::Text("R:%d", regionIdx);
				}
			}
			else
			{
				ImGui::Text("Invalid");
			}
			ImGui::EndGroup();

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			{
				anim.frames.erase(anim.frames.begin() + f);
				if (mSelectedFrameIndex >= (int)anim.frames.size()) mSelectedFrameIndex = (int)anim.frames.size() - 1;
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
	}
	else
	{
		ImGui::Separator();
	}
	ImGui::EndChild();
}

void SpriteSheetPanel::drawToolBar()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Load Texture"))
			{
				char filePath[512] = "";
				OpenFileFilterInfo filters[] = { {"Image File", "*.png;*.tga;*.jpg;*.bmp"} };
				if (SystemPlatform::OpenFileName(filePath, ARRAY_SIZE(filePath), filters, ""))
				{
					Render::RHITexture2D* texture = Render::RHIUtility::LoadTexture2DFromFile(filePath);
					if (texture)
					{
						if (!mEditedSheet)
						{
							mEditedSheet = new SpriteSheet();
						}
						mEditedSheet->texture = texture;
						mTexturePath = filePath;
					}
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save SpriteSheet JSON"))
			{
				if (mEditedSheet)
				{
					char filePath[512] = "";
					OpenFileFilterInfo filters[] = { {"SpriteSheet JSON", "*.json"} };
					if (SystemPlatform::SaveFileName(filePath, ARRAY_SIZE(filePath), filters, nullptr, "Save SpriteSheet"))
					{
						std::string pathStr = filePath;
						if (pathStr.find('.') == std::string::npos)
						{
							pathStr += ".json";
						}
						FSpriteUtility::save(pathStr.c_str(), *mEditedSheet, mTexturePath.c_str());
					}
				}
			}

			if (ImGui::MenuItem("Load SpriteSheet JSON"))
			{
				char filePath[512] = "";
				OpenFileFilterInfo filters[] = { {"SpriteSheet JSON", "*.json"} };
				if (SystemPlatform::OpenFileName(filePath, ARRAY_SIZE(filePath), filters, nullptr, "Load SpriteSheet"))
				{
					if (!mEditedSheet)
					{
						mEditedSheet = new SpriteSheet();
					}

					if (FSpriteUtility::load(filePath, *mEditedSheet))
					{
						mSelectedRegions.clear();
					}
				}
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Reset Zoom")) 
			{ 
				mZoomScale = 1.0f; 
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void SpriteSheetPanel::drawPropertiesPanel()
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Sprite Sheet Editor");
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Asset Operations", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button("Add Region"))
		{
			if (mEditedSheet)
			{
				SpriteRegion region;
				region.uvPos = Math::Vector2(0, 0);
				region.uvSize = Math::Vector2(0.1f, 0.1f);
				region.size = Math::Vector2(64, 64);
				region.pivot = Math::Vector2(0.5f, 0.5f);
				mEditedSheet->regions.push_back(region);
				mSelectedRegions = { (int)mEditedSheet->regions.size() - 1 };
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete") && !mSelectedRegions.empty())
		{
			if (mEditedSheet)
			{
				std::vector<int> sortedIndices = mSelectedRegions;
				std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());
				for (int idx : sortedIndices)
				{
					if (idx >= 0 && idx < (int)mEditedSheet->regions.size())
						mEditedSheet->regions.erase(mEditedSheet->regions.begin() + idx);
				}
				mSelectedRegions.clear();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear All"))
		{
			if (mEditedSheet) 
			{ 
				mEditedSheet->regions.clear(); 
			}
			mSelectedRegions.clear();
		}
	}

	if (ImGui::CollapsingHeader("Auto Grid Slice", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SetNextItemWidth(45);
		ImGui::DragInt("Rows", &mGridRows, 0.1f, 1, 1000);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(45);
		ImGui::DragInt("Cols", &mGridCols, 0.1f, 1, 1000);

		ImGui::SetNextItemWidth(45);
		ImGui::DragInt("Space X", &mCellSpacingX, 0.1f, 0, 1000);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(45);
		ImGui::DragInt("Space Y", &mCellSpacingY, 0.1f, 0, 1000);

		mGridRows = Math::Max(1, mGridRows);
		mGridCols = Math::Max(1, mGridCols);
		mCellSpacingX = Math::Max(0, mCellSpacingX);
		mCellSpacingY = Math::Max(0, mCellSpacingY);

		float resCw = 0, resCh = 0;
		if (mEditedSheet && mEditedSheet->texture.isValid())
		{
			float texW = (float)mEditedSheet->texture->getSizeX();
			float texH = (float)mEditedSheet->texture->getSizeY();
			float totalW = texW, totalH = texH;
			if (!mSelectedRegions.empty())
			{
				const SpriteRegion& scope = mEditedSheet->regions[mSelectedRegions[0]];
				totalW = scope.uvSize.x * texW;
				totalH = scope.uvSize.y * texH;
			}
			resCw = (totalW - (mGridCols - 1) * mCellSpacingX) / (float)mGridCols;
			resCh = (totalH - (mGridRows - 1) * mCellSpacingY) / (float)mGridRows;
		}
		ImGui::Text("Result Cell Size: %.1f x %.1f px", resCw, resCh);

		ImGui::Checkbox("Preview Slice", &mShowSlicePreview);

		char const* sliceBtnText = (!mSelectedRegions.empty()) ? "Slice Highlighted Region" : "Slice Full Texture";
		if (ImGui::Button(sliceBtnText, ImVec2(-FLT_MIN, 0)))
		{
			if (mEditedSheet && mEditedSheet->texture.isValid())
			{
				float texW = (float)mEditedSheet->texture->getSizeX();
				float texH = (float)mEditedSheet->texture->getSizeY();

				float startX = 0, startY = 0, totalW = texW, totalH = texH;

				if (!mSelectedRegions.empty())
				{
					const SpriteRegion& scope = mEditedSheet->regions[mSelectedRegions[0]];
					startX = scope.uvPos.x * texW;
					startY = scope.uvPos.y * texH;
					totalW = scope.uvSize.x * texW;
					totalH = scope.uvSize.y * texH;
				}
				else
				{
					mEditedSheet->regions.clear();
					mSelectedRegions.clear();
				}

				float cw = (totalW - (mGridCols - 1) * mCellSpacingX) / (float)mGridCols;
				float ch = (totalH - (mGridRows - 1) * mCellSpacingY) / (float)mGridRows;

				for (int r = 0; r < mGridRows; ++r)
				{
					for (int c = 0; c < mGridCols; ++c)
					{
						SpriteRegion region;
						float px = startX + (float)c * (cw + (float)mCellSpacingX);
						float py = startY + (float)r * (ch + (float)mCellSpacingY);

						region.uvPos = Math::Vector2(px / texW, py / texH);
						region.uvSize = Math::Vector2(cw / texW, ch / texH);
						region.size = Math::Vector2(cw, ch);
						region.pivot = Math::Vector2(0.5f, 0.5f);
						mEditedSheet->regions.push_back(region);
					}
				}
			}
		}
	}

	ImGui::Separator();
	ImGui::Text("Region List");
	ImGui::BeginChild("RegionListChild", ImVec2(0, mRegionListHeight), true);
	if (mEditedSheet)
	{
		float thumbnailSize = 40.0f;
		for (int i = 0; i < (int)mEditedSheet->regions.size(); ++i)
		{
			SpriteRegion& region = mEditedSheet->regions[i];
			ImGui::PushID(i);
			auto it = std::find(mSelectedRegions.begin(), mSelectedRegions.end(), i);
			bool bSelected = (it != mSelectedRegions.end());
			float itemHeight = thumbnailSize + 10;
			ImVec2 startPos = ImGui::GetCursorPos();

			if (ImGui::Selectable("##Item", bSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0, itemHeight)))
			{
				if (ImGui::GetIO().KeyCtrl)
				{
					if (bSelected) mSelectedRegions.erase(it);
					else mSelectedRegions.push_back(i);
				}
				else if (ImGui::GetIO().KeyShift && !mSelectedRegions.empty())
				{
					int startIdx = mSelectedRegions.back();
					int endIdx = i;
					if (startIdx > endIdx) std::swap(startIdx, endIdx);
					mSelectedRegions.clear();
					for (int k = startIdx; k <= endIdx; ++k) mSelectedRegions.push_back(k);
				}
				else
				{
					mSelectedRegions.clear();
					mSelectedRegions.push_back(i);
				}
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			{
				mPendingFocusRegionIndex = i;
			}

			ImGui::SetCursorPos(ImVec2(startPos.x + 5, startPos.y + 5));
			ImGui::Text("%d:", i);
			ImGui::SameLine();

			if (mEditedSheet->texture.isValid())
			{
				ImVec2 uv0(region.uvPos.x, region.uvPos.y), uv1(region.uvPos.x + region.uvSize.x, region.uvPos.y + region.uvSize.y);
				ImGui::Image(FImGui::GetTextureID(*mEditedSheet->texture), ImVec2(thumbnailSize, thumbnailSize), uv0, uv1);
				ImGui::SameLine();
			}

			ImGui::Text("(%dx%d)", (int)region.size.x, (int)region.size.y);
			ImGui::SetCursorPos(ImVec2(startPos.x, startPos.y + itemHeight));

			ImGui::PopID();
		}
	}
	ImGui::EndChild();

	// Vertical Splitter
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.2f));
	ImGui::Button("##ListSplitter", ImVec2(-1, 5));
	ImGui::PopStyleColor(3);

	if (ImGui::IsItemActive())
	{
		mRegionListHeight += ImGui::GetIO().MouseDelta.y;
		mRegionListHeight = Math::Max(50.0f, mRegionListHeight);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	}

	if (mEditedSheet && mSelectedRegions.size() == 1)
	{
		int selectedIdx = mSelectedRegions[0];
		if (selectedIdx >= 0 && selectedIdx < (int)mEditedSheet->regions.size())
		{
			SpriteRegion& region = mEditedSheet->regions[selectedIdx];
			ImGui::Separator();
			ImGui::Text("Selected ID: %d", selectedIdx);
		if (mEditedSheet->texture.isValid()) 
		{
			float availWidth = ImGui::GetContentRegionAvail().x - 10;
			float aspect = region.size.y / Math::Max(0.01f, region.size.x);
			float displayH = availWidth * aspect;
			if (displayH > 200.0f) 
			{ 
				displayH = 200.0f; 
				availWidth = displayH / aspect; 
			}
			ImVec2 uv0(region.uvPos.x, region.uvPos.y), uv1(region.uvPos.x + region.uvSize.x, region.uvPos.y + region.uvSize.y);
			ImGui::Text("Focus Preview:");
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - availWidth) * 0.5f);
			ImGui::Image(FImGui::GetTextureID(*mEditedSheet->texture), ImVec2(availWidth, displayH), uv0, uv1, ImVec4(1,1,1,1), ImVec4(1,1,1,0.5f));
			ImGui::Separator();
		}
		ImGui::PushItemWidth(-FLT_MIN);
		ImGui::Text("UV Pos"); ImGui::DragFloat2("##uvPos", &region.uvPos.x, 0.001f);
		ImGui::Text("UV Size"); ImGui::DragFloat2("##uvSize", &region.uvSize.x, 0.001f);
		ImGui::Text("Size (Pixel)"); ImGui::DragFloat2("##size", &region.size.x, 1.0f);
		ImGui::Text("Pivot"); ImGui::SliderFloat2("##pivot", &region.pivot.x, 0.0f, 1.0f);
		ImGui::PopItemWidth();
		if (ImGui::Button("Duplicate Selected", ImVec2(-FLT_MIN, 0))) 
		{
			mEditedSheet->regions.push_back(region);
			mSelectedRegions = { (int)mEditedSheet->regions.size() - 1 };
		}
		}
	}
}

enum EDragMode
{
	Drag_None = 0, Drag_Create = 1, Drag_Move = 2,
	Drag_L = 3, Drag_R = 4, Drag_T = 5, Drag_B = 6,
	Drag_TL = 7, Drag_TR = 8, Drag_BL = 9, Drag_BR = 10
};

void SpriteSheetPanel::drawTexturePreview()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("Main Preview");
	ImGui::SameLine();
	ImGui::Checkbox("Grid", &mShowGrid);
	ImGui::SameLine();
	ImGui::Checkbox("Snap", &mSnapToGrid);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(90);
	if (ImGui::InputInt("##GridSpacing", &mGridSpacing, 1, 8))
	{
		mGridSpacing = Math::Max(1, mGridSpacing);
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(-120);
	float oldZoom = mZoomScale;
	bool bZoomChanged = false, bZoomFromMouse = false;
	if (ImGui::SliderFloat("Zoom", &mZoomScale, 0.1f, 100.0f)) 
	{
		bZoomChanged = true;
	}
	
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) 
	{
		if (io.KeyCtrl && io.MouseWheel != 0) 
		{
			mZoomScale *= std::pow(1.2f, io.MouseWheel);
			mZoomScale = Math::Clamp(mZoomScale, 0.1f, 100.0f);
			bZoomChanged = true; 
			bZoomFromMouse = true;
		}
	}

	ImGui::BeginChild("PreviewBoard", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoMove);
	ImVec2 canvasBase = ImGui::GetCursorScreenPos();
	float texWidth = 512.0f, texHeight = 512.0f;
	if (mEditedSheet && mEditedSheet->texture.isValid()) 
	{
		texWidth = (float)mEditedSheet->texture->getSizeX(); 
		texHeight = (float)mEditedSheet->texture->getSizeY();
	}

	static Math::Vector2 pivotInPixels(0,0); 
	static ImVec2 pivotInViewport(0,0); 
	static bool bPendingZoomAdjust = false;
	
	int activeRegionIdx = mSelectedRegions.size() >= 1 ? mSelectedRegions[0] : -1;
	
	if (mPendingFocusRegionIndex != -1 && mEditedSheet && mEditedSheet->texture.isValid())
	{
		int idx = mPendingFocusRegionIndex;
		if (idx >= 0 && idx < (int)mEditedSheet->regions.size())
		{
			const SpriteRegion& r = mEditedSheet->regions[idx];
			float texW = (float)mEditedSheet->texture->getSizeX();
			float texH = (float)mEditedSheet->texture->getSizeY();

			pivotInPixels = Math::Vector2((r.uvPos.x + r.uvSize.x * 0.5f) * texW, (r.uvPos.y + r.uvSize.y * 0.5f) * texH);
			
			ImVec2 viewportSize = ImGui::GetWindowSize();
			pivotInViewport = ImVec2(viewportSize.x * 0.5f, viewportSize.y * 0.5f);

			float targetZoomX = (viewportSize.x * 0.6f) / Math::Max(1.0f, r.size.x);
			float targetZoomY = (viewportSize.y * 0.6f) / Math::Max(1.0f, r.size.y);
			mZoomScale = Math::Min(targetZoomX, targetZoomY);
			mZoomScale = Math::Clamp(mZoomScale, 0.1f, 10.0f);
			
			bPendingZoomAdjust = true;
		}
		mPendingFocusRegionIndex = -1;
	}

	if (bZoomChanged) 
	{
		bPendingZoomAdjust = true;
		float currentScrollX = ImGui::GetScrollX();
		float currentScrollY = ImGui::GetScrollY();
		ImVec2 viewportSize = ImGui::GetWindowSize();

		if (bZoomFromMouse) 
		{ 
			// Maintain point under mouse
			pivotInPixels = Math::Vector2((io.MousePos.x - canvasBase.x) / oldZoom, (io.MousePos.y - canvasBase.y) / oldZoom); 
			pivotInViewport = ImVec2(io.MousePos.x - (canvasBase.x + currentScrollX), io.MousePos.y - (canvasBase.y + currentScrollY));
		}
		else 
		{
			// Maintain viewport center
			pivotInViewport = ImVec2(viewportSize.x * 0.5f, viewportSize.y * 0.5f);
			pivotInPixels = Math::Vector2((currentScrollX + pivotInViewport.x) / oldZoom, (currentScrollY + pivotInViewport.y) / oldZoom);
		}
	}

	if (bPendingZoomAdjust) 
	{
		float newScrollX = Math::Max(0.0f, pivotInPixels.x * mZoomScale - pivotInViewport.x);
		float newScrollY = Math::Max(0.0f, pivotInPixels.y * mZoomScale - pivotInViewport.y);
		
		float oldScrollX = ImGui::GetScrollX();
		float oldScrollY = ImGui::GetScrollY();
		
		ImGui::SetScrollX(newScrollX); 
		ImGui::SetScrollY(newScrollY); 
		
		// Compensate canvasBase immediately to prevent "jumping" ghosting in the current frame
		canvasBase.x += (oldScrollX - newScrollX);
		canvasBase.y += (oldScrollY - newScrollY);

		bPendingZoomAdjust = false;
	}

	ImVec2 previewAreaSize = ImVec2(texWidth * mZoomScale, texHeight * mZoomScale);
	ImGui::InvisibleButton("TextureCanvas", previewAreaSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	bool isMouseDown = ImGui::IsItemActive(), isMouseClicked = ImGui::IsItemClicked(0);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// Right-click Panning
	if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1))
	{
		ImVec2 delta = io.MouseDelta;
		ImGui::SetScrollX(ImGui::GetScrollX() - delta.x);
		ImGui::SetScrollY(ImGui::GetScrollY() - delta.y);
	}

	if (mEditedSheet && mEditedSheet->texture.isValid())
	{
		drawList->AddImage(FImGui::GetTextureID(*mEditedSheet->texture), canvasBase, ImVec2(canvasBase.x + previewAreaSize.x, canvasBase.y + previewAreaSize.y));
	}
	else
	{
		drawList->AddRect(canvasBase, ImVec2(canvasBase.x + previewAreaSize.x, canvasBase.y + previewAreaSize.y), IM_COL32(255, 255, 255, 255));
	}

	if (mShowSlicePreview && mEditedSheet && mEditedSheet->texture.isValid())
	{
		float texW = (float)mEditedSheet->texture->getSizeX();
		float texH = (float)mEditedSheet->texture->getSizeY();
		float startX = 0, startY = 0, totalW = texW, totalH = texH;

		if (activeRegionIdx >= 0)
		{
			const SpriteRegion& scope = mEditedSheet->regions[activeRegionIdx];
			startX = scope.uvPos.x * texW;
			startY = scope.uvPos.y * texH;
			totalW = scope.uvSize.x * texW;
			totalH = scope.uvSize.y * texH;
		}

		float cw = (totalW - (mGridCols - 1) * mCellSpacingX) / (float)mGridCols;
		float ch = (totalH - (mGridRows - 1) * mCellSpacingY) / (float)mGridRows;

		ImU32 previewColor = IM_COL32(0, 255, 255, 180); // Cyan
		for (int r = 0; r < mGridRows; ++r)
		{
			for (int c = 0; c < mGridCols; ++c)
			{
				float px = startX + (float)c * (cw + (float)mCellSpacingX);
				float py = startY + (float)r * (ch + (float)mCellSpacingY);
				ImVec2 p1(canvasBase.x + px * mZoomScale, canvasBase.y + py * mZoomScale);
				ImVec2 p2(p1.x + cw * mZoomScale, p1.y + ch * mZoomScale);
				drawList->AddRect(p1, p2, previewColor, 0.0f, 0, 1.0f);
			}
		}
	}

	if (mShowGrid && mGridSpacing > 0)
	{
		float spacing = (float)mGridSpacing * mZoomScale;
		ImU32 gridColor = IM_COL32(200, 200, 200, 60);
		for (float x = 0; x <= previewAreaSize.x; x += spacing) 
		{
			drawList->AddLine(ImVec2(canvasBase.x + x, canvasBase.y), ImVec2(canvasBase.x + x, canvasBase.y + previewAreaSize.y), gridColor);
		}
		for (float y = 0; y <= previewAreaSize.y; y += spacing) 
		{
			drawList->AddLine(ImVec2(canvasBase.x, canvasBase.y + y), ImVec2(canvasBase.x + previewAreaSize.x, canvasBase.y + y), gridColor);
		}
	}

	Math::Vector2 rawLocalMousePos((io.MousePos.x - canvasBase.x) / mZoomScale, (io.MousePos.y - canvasBase.y) / mZoomScale);
	auto snapVal = [&](float val) 
	{ 
		if (mSnapToGrid && mGridSpacing > 0) return (float)std::round(val / mGridSpacing) * mGridSpacing; 
		return val; 
	};
	Math::Vector2 localMousePos(snapVal(rawLocalMousePos.x), snapVal(rawLocalMousePos.y));

	float handleDisplaySize = 6.0f, handleHitThreshold = 10.0f / mZoomScale;
	if (isMouseClicked) 
	{
		mDragStartPos = rawLocalMousePos; 
		mDragMode = Drag_None;
		if (mEditedSheet && activeRegionIdx >= 0) 
		{
			const SpriteRegion& r = mEditedSheet->regions[activeRegionIdx];
			float minX = Math::Min(r.uvPos.x, r.uvPos.x+r.uvSize.x)*texWidth, maxX = Math::Max(r.uvPos.x, r.uvPos.x+r.uvSize.x)*texWidth;
			float minY = Math::Min(r.uvPos.y, r.uvPos.y+r.uvSize.y)*texHeight, maxY = Math::Max(r.uvPos.y, r.uvPos.y+r.uvSize.y)*texHeight;
			float midX = (minX + maxX) * 0.5f, midY = (minY + maxY) * 0.5f;
			auto checkHitGizmo = [&](float px, float py, int ox, int oy) 
			{
				float cx = px + ox * (handleDisplaySize + 2) / mZoomScale, cy = py + oy * (handleDisplaySize + 2) / mZoomScale;
				return Math::Abs(rawLocalMousePos.x - cx) < handleHitThreshold && Math::Abs(rawLocalMousePos.y - cy) < handleHitThreshold;
			};
			if (checkHitGizmo(minX, minY, -1, -1)) mDragMode = Drag_TL; 
			else if (checkHitGizmo(maxX, minY, 1, -1)) mDragMode = Drag_TR;
			else if (checkHitGizmo(minX, maxY, -1, 1)) mDragMode = Drag_BL; 
			else if (checkHitGizmo(maxX, maxY, 1, 1)) mDragMode = Drag_BR;
			else if (checkHitGizmo(minX, midY, -1, 0)) mDragMode = Drag_L; 
			else if (checkHitGizmo(maxX, midY, 1, 0)) mDragMode = Drag_R;
			else if (checkHitGizmo(midX, minY, 0, -1)) mDragMode = Drag_T; 
			else if (checkHitGizmo(midX, maxY, 0, 1)) mDragMode = Drag_B;
		}
		if (mDragMode == Drag_None) 
		{
			bool bHitNormal = false;
			if (mEditedSheet) 
			{
				for (int i = (int)mEditedSheet->regions.size() - 1; i >= 0; --i) 
				{
					const SpriteRegion& r = mEditedSheet->regions[i];
					float minX = Math::Min(r.uvPos.x,r.uvPos.x+r.uvSize.x)*texWidth, maxX = Math::Max(r.uvPos.x,r.uvPos.x+r.uvSize.x)*texWidth;
					float minY = Math::Min(r.uvPos.y,r.uvPos.y+r.uvSize.y)*texHeight, maxY = Math::Max(r.uvPos.y,r.uvPos.y+r.uvSize.y)*texHeight;
					if (rawLocalMousePos.x >= minX && rawLocalMousePos.x <= maxX && rawLocalMousePos.y >= minY && rawLocalMousePos.y <= maxY) 
					{
						mSelectedRegions = { i };
						activeRegionIdx = i;
						mDragMode = Drag_Move; 
						bHitNormal = true; 
						break;
					}
				}
			}
			if (!bHitNormal) 
			{ 
				mSelectedRegions.clear();
				activeRegionIdx = -1;
				mDragMode = Drag_Create; 
				mDragStartPos = localMousePos; 
			}
		}
	}
	if (isMouseDown && mDragMode != Drag_None && mEditedSheet) 
	{
		if (mDragMode == Drag_Create) 
		{
			if (!mIsDraggingNew) 
			{
				SpriteRegion newRegion; 
				newRegion.uvPos = Math::Vector2(mDragStartPos.x / texWidth, mDragStartPos.y / texHeight);
				newRegion.uvSize = Math::Vector2(0, 0); 
				mEditedSheet->regions.push_back(newRegion);
				mSelectedRegions = { (int)mEditedSheet->regions.size() - 1 };
				activeRegionIdx = mSelectedRegions[0];
				mIsDraggingNew = true;
			}
			SpriteRegion& r = mEditedSheet->regions[activeRegionIdx];
			float x1 = mDragStartPos.x, y1 = mDragStartPos.y, x2 = localMousePos.x, y2 = localMousePos.y;
			r.uvPos = Math::Vector2(x1 / texWidth, y1 / texHeight); 
			r.uvSize = Math::Vector2((x2 - x1) / texWidth, (y2 - y1) / texHeight);
			r.size = Math::Vector2(Math::Abs(x2 - x1), Math::Abs(y2 - y1));
		} 
		else if (activeRegionIdx >= 0) 
		{
			SpriteRegion& r = mEditedSheet->regions[activeRegionIdx];
			float x1 = r.uvPos.x * texWidth, y1 = r.uvPos.y * texHeight, x2 = (r.uvPos.x + r.uvSize.x) * texWidth, y2 = (r.uvPos.y + r.uvSize.y) * texHeight;
			if (mDragMode == Drag_Move) 
			{
				float dx = localMousePos.x - snapVal(mDragStartPos.x), dy = localMousePos.y - snapVal(mDragStartPos.y);
				if (dx != 0 || dy != 0) 
				{ 
					r.uvPos.x += dx / texWidth; 
					r.uvPos.y += dy / texHeight; 
					mDragStartPos.x += dx; 
					mDragStartPos.y += dy; 
				}
			} 
			else 
			{
				switch (mDragMode) 
				{
				case Drag_L: x1 = localMousePos.x; break; 
				case Drag_R: x2 = localMousePos.x; break; 
				case Drag_T: y1 = localMousePos.y; break; 
				case Drag_B: y2 = localMousePos.y; break;
				case Drag_TL: x1 = localMousePos.x; y1 = localMousePos.y; break; 
				case Drag_TR: x2 = localMousePos.x; y1 = localMousePos.y; break;
				case Drag_BL: x1 = localMousePos.x; y2 = localMousePos.y; break; 
				case Drag_BR: x2 = localMousePos.x; y2 = localMousePos.y; break;
				}
				r.uvPos = Math::Vector2(x1 / texWidth, y1 / texHeight); 
				r.uvSize = Math::Vector2((x2 - x1) / texWidth, (y2 - y1) / texHeight);
				r.size = Math::Vector2(Math::Abs(x2 - x1), Math::Abs(y2 - y1));
			}
		}
	} 
	else 
	{ 
		mDragMode = Drag_None; 
		mIsDraggingNew = false; 
	}
	if (mEditedSheet) 
	{
		for (int i = 0; i < (int)mEditedSheet->regions.size(); ++i) 
		{
			bool bIsSelected = std::find(mSelectedRegions.begin(), mSelectedRegions.end(), i) != mSelectedRegions.end();
			const SpriteRegion& region = mEditedSheet->regions[i];
			float minX = Math::Min(region.uvPos.x, region.uvPos.x + region.uvSize.x) * texWidth * mZoomScale, maxX = Math::Max(region.uvPos.x, region.uvPos.x + region.uvSize.x) * texWidth * mZoomScale;
			float minY = Math::Min(region.uvPos.y, region.uvPos.y + region.uvSize.y) * texHeight * mZoomScale, maxY = Math::Max(region.uvPos.y, region.uvPos.y + region.uvSize.y) * texHeight * mZoomScale;
			ImVec2 r1(canvasBase.x + minX, canvasBase.y + minY), r2(canvasBase.x + maxX, canvasBase.y + maxY);
			drawList->AddRect(r1, r2, bIsSelected ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 160), 0.0f, 0, bIsSelected ? 2.0f : 1.0f);
			if (bIsSelected) 
			{
				float hs = 6, gap = 2, mx = (r1.x + r2.x) * 0.5f, my = (r1.y + r2.y) * 0.5f;
				auto drawH = [&](float x, float y, int ox, int oy) 
				{
					float cx = x + ox*(hs+gap), cy = y + oy*(hs+gap);
					drawList->AddRectFilled(ImVec2(cx-hs,cy-hs), ImVec2(cx+hs,cy+hs), IM_COL32(0,255,0,255));
				};
				drawH(r1.x,r1.y,-1,-1); drawH(mx,r1.y,0,-1); drawH(r2.x,r1.y,1,-1);
				drawH(r1.x,my,-1,0); drawH(r2.x,my,1,0);
				drawH(r1.x,r2.y,-1,1); drawH(mx,r2.y,0,1); drawH(r2.x,r2.y,1,1);
			}
		}
	}
	ImGui::EndChild();
}
