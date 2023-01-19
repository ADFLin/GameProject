#pragma once

#ifndef ContentPanel_H_E2316AC7_9BAC_4606_A3FE_055801C20154
#define ContentPanel_H_E2316AC7_9BAC_4606_A3FE_055801C20154

#include "EditorPanel.h"
#include "Template/StringView.h"

class ContentPanel : public IEditorPanel
{
public:

	void onOpen() override;
	void render() override;

	void renderFolderTree(char const* name, char const* path, int level, bool bInCurFolderSeq);
	struct FileData
	{
		std::string name;
		bool bFolder;
		bool bSelected;
	};

	std::vector< FileData > mCurrentFiles;
	float mThumbnailSize = 64.0f;
	float mPadding = 8;

	void addCurrentFolder(char const* path, bool bReset = true)
	{
		mCurrentFolderPath = path;
		mFolderPathHistory.push_back(path);
		indexHistory = mFolderPathHistory.size() - 1;
	}
	void updateContent();


	std::string mCurrentFolderPath = ".";
	int indexHistory = INDEX_NONE;
	std::vector< std::string > mFolderPathHistory;
	std::vector<StringView> mFolderSeq;

	void getRenderParams(WindowRenderParams& params) const
	{
#if 1
		params.flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
#endif
	}

};


#endif // ContentPanel_H_E2316AC7_9BAC_4606_A3FE_055801C20154
