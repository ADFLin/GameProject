#pragma once

#ifndef ContentPanel_H_E2316AC7_9BAC_4606_A3FE_055801C20154
#define ContentPanel_H_E2316AC7_9BAC_4606_A3FE_055801C20154

#include "EditorPanel.h"
#include "Template/StringView.h"

class ContentPanel : public IEditorPanel
{
public:

	void onOpen() override;
	void render(const char* title, bool* p_open) override;

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

	void updateContent();


	std::string mCurrentFolderPath = ".";
	int indexHistory;
	std::vector< std::string > mFolderPathHistory;
	std::vector<StringView> mFolderSeq;
};


#endif // ContentPanel_H_E2316AC7_9BAC_4606_A3FE_055801C20154
