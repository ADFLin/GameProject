#include "ContentPanel.h"

#include "FileSystem.h"

#include "ImGui/imgui.h"
#include "InlineString.h"
#include "LogSystem.h"
#include "EditorUtils.h"
#include "StringParse.h"


REGISTER_EDITOR_PANEL(ContentPanel, "Content", true, true);


bool FilterDirectory(char const* name)
{
	switch (FEditor::Hash(name))
	{
	case FEditor::Hash("."):
	case FEditor::Hash(".."):
		return false;
	}
	return true;
}


void ContentPanel::onOpen()
{
	updateContent();
}

void SplitString(char const* str, char const* drop, std::vector<StringView>& outList)
{
	StringView token;
	while (FStringParse::StringToken(str, drop, token))
	{
		outList.push_back(token);
	}
}


void ContentPanel::render()
{
	auto DrawIconButton = [](int id, bool bEnabled)
	{
		ImGui::BeginDisabled(!bEnabled);
		ImGui::PushID(id);
		bool bClicked = FImGui::IconButton(id, ImVec2(16, 16), bEnabled ? ImVec4(1, 1, 1, 1) : ImVec4(0.5, 0.5, 0.5, 0.5));
		ImGui::PopID();
		ImGui::EndDisabled();
		return bClicked;
	};

	auto DrawHint = [](char const* desc)
	{
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	};
	bool bNeedUpdateContent = false;

	bool bEnabled = indexHistory > 0;
	if (DrawIconButton(EIconId::CircleArrowLeft, bEnabled))
	{
		--indexHistory;
		mCurrentFolderPath = mFolderPathHistory[indexHistory];
		bNeedUpdateContent = true;
	}
	else if (bEnabled)
	{
		DrawHint(mFolderPathHistory[indexHistory - 1].c_str());
	}

	ImGui::SameLine();
	bEnabled = indexHistory != INDEX_NONE && indexHistory < (mFolderPathHistory.size() - 1);
	if (DrawIconButton(EIconId::CircleArrowRight, bEnabled))
	{
		++indexHistory;
		mCurrentFolderPath = mFolderPathHistory[indexHistory];
		bNeedUpdateContent = true;
	}
	else if (bEnabled)
	{
		DrawHint(mFolderPathHistory[indexHistory + 1].c_str());
	}

	ImGui::SameLine();
	if (FImGui::IconButton(EIconId::FolderClosed, ImVec2(16, 16)))
	{

	}
	DrawHint("Choose a path");
	ImGui::SameLine();
	{
		int index = 0;
		for (auto& folderName : mFolderSeq)
		{
			if (index != 0)
			{
				ImGui::SameLine();
			}
			if (ImGui::Button((index == 0) ? "All" : folderName.toCString()))
			{
				std::string newFolderPath;
				for (int i = 0; i <= index; ++i)
				{
					if (i != 0)
					{
						newFolderPath += "/";
					}
					newFolderPath += mFolderSeq[i].toCString();
				}
				addCurrentFolder(newFolderPath.c_str());
				bNeedUpdateContent = true;
			}

			if ( index != mFolderSeq.size() - 1 || ( mCurrentFiles.size() > 0 && mCurrentFiles[0].bFolder ))
			{
				ImGui::SameLine();
				ImGui::PushID(index);
				if (ImGui::Button(">"))
				{
					ImGui::OpenPopup("SubFolderSelect");
				}

				if (ImGui::BeginPopup("SubFolderSelect"))
				{
					std::string newFolderPath;
					for (int i = 0; i <= index; ++i)
					{
						if (i != 0)
						{
							newFolderPath += "/";
						}
						newFolderPath += mFolderSeq[i].toCString();
					}
					FileIterator fileIter;
					if (FFileSystem::FindFiles(newFolderPath.c_str(), nullptr, fileIter))
					{
						for (; fileIter.haveMore(); fileIter.goNext())
						{
							if (!fileIter.isDirectory())
								continue;

							if (!FilterDirectory(fileIter.getFileName()))
								continue;

							if (ImGui::MenuItem(fileIter.getFileName()))
							{
								newFolderPath += "/";
								newFolderPath += fileIter.getFileName();

								addCurrentFolder(newFolderPath.c_str());
								bNeedUpdateContent = true;
								break;
							}
						}
					}
					ImGui::EndPopup();
				}
				ImGui::PopID();
			}
			++index;
		}
	}

	ImGui::Separator();

	if (ImGui::BeginTable("Split", 2, ImGuiTableFlags_Resizable))
	{
		ImGui::TableNextColumn();
		ImGui::BeginChild("DirectoryTree");
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12);
		renderFolderTree("All",".", 0, true);
		ImGui::PopStyleVar();
		ImGui::EndChild();

		ImGui::TableNextColumn();

		ImGui::BeginChild("Content");


		float panelWidth = ImGui::GetContentRegionAvail().x - ImGui::GetCursorStartPos().x;
		float cellSize = mThumbnailSize + mPadding;
		int colCount = Math::Max(1, Math::FloorToInt(panelWidth / cellSize));

		const float footer_height_to_reserve = ImGui::GetFrameHeight();
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false);
		if (ImGui::BeginTable("ContentTile", colCount))
		{
			// selectable list
			int id = 0;
			for (auto& fileData : mCurrentFiles)
			{
				ImGui::TableNextColumn();
				ImGui::PushID(id);

				InlineString<32> IdName;
				IdName.format("##Object %d", id);
				auto pos = ImGui::GetCursorPos();

				if (ImGui::Selectable(IdName, &fileData.bSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(cellSize, cellSize)))
				{

				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
				{
					if (fileData.bFolder)
					{
						InlineString<MAX_PATH> childDir;
						childDir.format("%s/%s", mCurrentFolderPath.c_str(), fileData.name.c_str());
						addCurrentFolder(childDir);
						bNeedUpdateContent = true;
					}
				}
				ImGui::SetItemAllowOverlap();

				ImGui::SetCursorPos(ImVec2(pos.x, pos.y));
				if (fileData.bFolder)
				{
					FImGui::Icon(EIconId::Folder, ImVec2(mThumbnailSize, mThumbnailSize), ImVec4(0.8, 0.6, 0.1, 1));
				}
				else
				{
					FImGui::Icon(EIconId::Document, ImVec2(mThumbnailSize, mThumbnailSize), ImVec4(1.0, 1.0, 1.0, 1));
				}

				ImVec2 textSize = ImGui::CalcTextSize(fileData.name.c_str(), nullptr, false , mThumbnailSize);
				ImGui::SetCursorPos(ImVec2(pos.x + (mThumbnailSize - textSize.x) / 2, pos.y + mThumbnailSize));
				ImGui::TextWrapped(fileData.name.c_str());
				ImGui::PopID();
				++id;
			}
			ImGui::EndTable();
		}	
		ImGui::EndChild();
		ImGui::Text("%d Items", mCurrentFiles.size());
		ImGui::EndChild();
		ImGui::EndTable();
	}

	if (bNeedUpdateContent)
	{
		updateContent();
	}
}

void ContentPanel::renderFolderTree(char const* name, char const* path, int level, bool bInCurFolderSeq )
{
	bool bHaveSubFolder = false;
	{
		FileIterator fileIter;
		if (FFileSystem::FindFiles(path, nullptr, fileIter))
		{
			for (; fileIter.haveMore(); fileIter.goNext())
			{
				if (!fileIter.isDirectory())
					continue;

				if (!FilterDirectory(fileIter.getFileName()))
					continue;

				bHaveSubFolder = true;
				break;
			}
		}
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;

	if (!bHaveSubFolder)
		flags |= ImGuiTreeNodeFlags_Leaf;
	
	bool bInCurFolderSeqNew = false;
	if (level == 0)
	{
		bInCurFolderSeqNew = true;
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	else if (bInCurFolderSeq)
	{
		if (level < mFolderSeq.size() && FCString::Compare(name, mFolderSeq[level].toCString()) == 0)
		{
			bInCurFolderSeqNew = true;
			if (level == mFolderSeq.size() - 1)
				flags |= ImGuiTreeNodeFlags_Selected;
		}
	}

	if (bInCurFolderSeqNew && level != mFolderSeq.size() - 1)
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}

	ImGui::PushID(name);
	ImVec2 const iconSize = ImVec2(18, 16);

	InlineString<32> IdName;
	IdName.format("##Object_%s", name);
	ImVec2 framePadding;
	framePadding.x = 0;
	framePadding.y = 0.5f * ( iconSize.y - ImGui::GetFontSize() );
	auto pos = ImGui::GetCursorPos();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePadding);
	bool bOpened = ImGui::TreeNodeEx(IdName, flags);
	ImGui::PopStyleVar();
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		addCurrentFolder(path);
		updateContent();
	}

	int fontSize = ImGui::GetFontSize() + 2;
	ImGui::SetItemAllowOverlap();
	ImGui::SetCursorPos(ImVec2(pos.x + fontSize, pos.y));

	FImGui::Icon((bOpened && bHaveSubFolder) ? EIconId::FolderOpen : EIconId::FolderClosed, iconSize, ImVec4(0.8, 0.6, 0.1, 1));
	float textOffsetY = 0.5 * ( iconSize.y - ImGui::GetFontSize() );
	ImGui::SetCursorPos(ImVec2(pos.x + 18 + 2 + fontSize, pos.y + textOffsetY));
	ImGui::TextUnformatted(name);
	ImGui::PopID();

	if (bOpened)
	{
		FileIterator fileIter;
		if (FFileSystem::FindFiles(path, nullptr, fileIter))
		{
			for (; fileIter.haveMore(); fileIter.goNext())
			{
				if (!fileIter.isDirectory())
					continue;

				if (!FilterDirectory(fileIter.getFileName()))
					continue;

				InlineString<MAX_PATH> childDir;
				childDir.format("%s/%s", path, fileIter.getFileName());
				renderFolderTree(fileIter.getFileName(), childDir.c_str(), level + 1, bInCurFolderSeqNew);
			}
		}

		ImGui::TreePop();
	}
}


void ContentPanel::updateContent()
{
	mFolderSeq.clear();
	SplitString(mCurrentFolderPath.c_str(), "/", mFolderSeq);

	mCurrentFiles.clear();

	FileIterator fileIter;
	if (FFileSystem::FindFiles(mCurrentFolderPath.c_str(), nullptr, fileIter))
	{
		for (; fileIter.haveMore(); fileIter.goNext())
		{
			if (fileIter.isDirectory())
			{
				if (!FilterDirectory(fileIter.getFileName()))
					continue;
			}
			FileData fileData;
			fileData.name = fileIter.getFileName();
			fileData.bFolder = fileIter.isDirectory();
			fileData.bSelected = false;

			mCurrentFiles.push_back(std::move(fileData));
		}
	}

	std::sort( mCurrentFiles.begin(), mCurrentFiles.end() , 
		[](auto const& lhs, auto const& rhs)
		{
			if (lhs.bFolder != rhs.bFolder)
				return lhs.bFolder;

			return FCString::Compare(lhs.name.c_str(), rhs.name.c_str()) < 0;
		}
	);
}

