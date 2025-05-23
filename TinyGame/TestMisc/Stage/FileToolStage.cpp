#include "Stage/TestStageHeader.h"
#include "FileSystem.h"

#include "DrawEngine.h"
#include <regex>
#include <algorithm>
#include <string>
#include "PropertySet.h"
#include "StringParse.h"
#include "ProfileSystem.h"

class FileToolStage : public StageBase
{
	using BaseClass = StageBase;
public:
	FileToolStage() {}

#define CONFIG_SECTION "FileTool"

	struct ReplaceInfo
	{
		std::string from;
		std::string to;
	};


	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();

		TArray<char const*> replaceNames;
		::Global::GameConfig().getStringValues("Replace", CONFIG_SECTION, replaceNames);
		for (auto str : replaceNames)
		{
			StringView from;
			if (FStringParse::StringToken(str, ",", from))
			{
				ReplaceInfo info;
				info.from = from.toCString();
				++str;
				info.to = str;
				mReplaces.push_back(std::move(info));
			}
		}

		frame->addButton("Conv Path", [this](int event, GWidget*)
		{
			char const* dirPath = ::Global::GameConfig().getStringValue("Dir", CONFIG_SECTION, "");
			char path[1024];
			if (SystemPlatform::OpenDirectoryName(path, ARRAYSIZE(path), dirPath , "Select Convert Directory" , ::Global::GetDrawEngine().getWindowHandle()))
			{
				TIME_SCOPE("Convert File Name");
				::Global::GameConfig().setKeyValue("Dir", CONFIG_SECTION, path);
				convFileName(path);
			}

			return true;
		});

		frame->addButton("Check Image", [this](int event, GWidget*)
		{
			TIME_SCOPE("Check Image");
			TArray<char const*> checkFolders;
			::Global::GameConfig().getStringValues("CheckDir", CONFIG_SECTION, checkFolders);
			for (auto path : checkFolders)
			{
				LogMsg("=Check Folder : %s", path);
				checkImage(path);
			}
			return true;
		});


		frame->addButton("Check Folder Name", [this](int event, GWidget*)
		{
			TArray<char const*> checkFolders;
			::Global::GameConfig().getStringValues("CheckDir", CONFIG_SECTION, checkFolders);

			mFolderNameSet.clear();
			for (auto path : checkFolders)
			{
				LogMsg("=Check Folder : %s", path);
				checkSameNameFolder(path);
			}

			LogMsg("==Check Complete, %u Folders", mFolderNameSet.size());
			return true;
		});


		restart();
		return true;
	}

	bool replaceName(std::string& inoutStr)
	{
		for (auto const& info : mReplaces)
		{
			if (inoutStr == info.from)
			{
				inoutStr = info.to;
				return true;
			}
		}
		return false;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}

	struct FileDesc
	{
		std::string code;
		int number;
		int partNumber;
		bool b8K;
	};


	TArray<ReplaceInfo> mReplaces;



	std::unordered_set<std::string> mFolderNameSet;

	bool IsSystemFolder(char const* path)
	{
		if (FCString::Compare(path, ".") == 0 ||
			FCString::Compare(path, "..") == 0)
			return true;
		return false;
	}
	void checkSameNameFolder(char const* dirPath)
	{
		FileIterator fileIter;
		if (FFileSystem::FindFiles(dirPath, fileIter))
		{
			for (; fileIter.haveMore(); fileIter.goNext())
			{
				if (!fileIter.isDirectory())
					continue;

				if (IsSystemFolder(fileIter.getFileName()))
					continue;

				auto [iter, bOk] = mFolderNameSet.insert(fileIter.getFileName());
				if (!bOk)
				{
					LogMsg("=> %s", fileIter.getFileName());
				}

			}
		}
	}

	void checkImage(char const* dirPath)
	{
		FileIterator fileIter;
		if (FFileSystem::FindFiles(dirPath, fileIter))
		{
			for (; fileIter.haveMore(); fileIter.goNext())
			{
				if (!fileIter.isDirectory())
					continue;

				if (IsSystemFolder(fileIter.getFileName()))
					continue;

				std::string imageName = fileIter.getFileName();
				if (imageName.back() =='U')
					imageName.pop_back();
				if (imageName.back() == 'B')
					imageName.pop_back();
				if (imageName.back() == 'K')
					imageName.pop_back();

				InlineString<256> imagePath;
				imagePath.format("%s\\%s\\%s.jpg", dirPath, fileIter.getFileName(), imageName.c_str());

				if (!FFileSystem::IsExist(imagePath))
				{
					LogMsg("=> %s", imageName.c_str());
				}
			}
		}
	}

	void convFileName(char const* dirPath)
	{ 

		FileIterator fileIter;
		if (FFileSystem::FindFiles(dirPath, fileIter))
		{
			for (; fileIter.haveMore(); fileIter.goNext())
			{
				if (fileIter.isDirectory() )
					continue;

				char const* subName = FFileUtility::GetExtension(fileIter.getFileName());
				if (FCString::Compare(subName, "jpg") == 0)
					continue;

				FileDesc desc;
				if (parseFileName(dirPath, fileIter.getFileName(), desc))
				{
					replaceName(desc.code);
					InlineString<256> newName;
					if (desc.partNumber)
						newName.format("%s-%03d-%c.%s", desc.code.c_str(), desc.number, 'A' + (desc.partNumber-1), subName);
					else
						newName.format("%s-%03d.%s", desc.code.c_str(), desc.number, subName);

					//LogMsg("%s => %s %d %d %s", fileIiter.getFileName(), desc.code.c_str(), desc.number, desc.partNumber, desc.b8K ? "true" : "false");

					InlineString<256> filePath;
					filePath.format("%s/%s", dirPath, fileIter.getFileName());

					if (newName != fileIter.getFileName())
					{
						LogMsg("Rename: %s => %s", fileIter.getFileName(), newName.c_str());
						FFileSystem::RenameFile(filePath, newName);
						filePath.format("%s/%s", dirPath, newName.c_str());
					}

					InlineString<256> newDir;
					newDir.format("%s/%s-%03d%s", dirPath, desc.code.c_str(), desc.number, desc.b8K ? "K" : "");
					if (!FFileSystem::IsExist(newDir))
					{
						if (!FFileSystem::CreateDirectory(newDir))
						{
							continue;
						}
					}
					LogMsg("Move: %s => %s", filePath, newDir);
					FFileSystem::MoveFile(filePath, newDir);
				}
				else
				{
					LogMsg("%s", fileIter.getFileName());
				}	
			}
		}


		if (FFileSystem::FindFiles(dirPath, ".jpg", fileIter))
		{
			for (; fileIter.haveMore(); fileIter.goNext())
			{
				if (fileIter.isDirectory())
					continue;

				FileDesc desc;
				if (parseFileName(dirPath, fileIter.getFileName(), desc))
				{
					char const* subName = FFileUtility::GetExtension(fileIter.getFileName());

					replaceName(desc.code);
					InlineString<256> newName;
					newName.format("%s-%03d.%s", desc.code.c_str(), desc.number, subName);

					InlineString<256> filePath;
					filePath.format("%s/%s", dirPath, fileIter.getFileName());

					if (newName != fileIter.getFileName())
					{
						LogMsg("Rename: %s => %s", fileIter.getFileName(), newName.c_str());
						FFileSystem::RenameFile(filePath, newName);
						filePath.format("%s/%s", dirPath, newName.c_str());
					}

					InlineString<256> newDir;
					newDir.format("%s/%s-%03d", dirPath, desc.code.c_str(), desc.number);
					if (!FFileSystem::IsExist(newDir))
					{
						newDir.format("%s/%s-%03dK", dirPath, desc.code.c_str(), desc.number);
						if (!FFileSystem::IsExist(newDir))
						{
							continue;
						}
					}

					LogMsg("Move: %s => %s", filePath, newDir);
					FFileSystem::MoveFile(filePath, newDir);
				}
				else
				{
					LogMsg("%s", fileIter.getFileName());
				}
			}
		}
	}

	struct PatternDesc
	{
		char const* regexText;
		int indexCode;
		int indexNumber;
		int indexPartNumber;
		int indexFormat;
		int indexFileFormat;
	};
	static constexpr PatternDesc PatternList[] =
	{
		{ CODE_STRING(([\w]+)-([\d]+).([\w]+)$), 0 , 1 , INDEX_NONE, INDEX_NONE, 3 },
		{ CODE_STRING(([\w]+)-([\d]+)-([\d+|\a-zA-Z]).([\w]+)$), 0 , 1 , 2, INDEX_NONE, 3 },
		{ CODE_STRING(([\w]+)-([\d]+)([\a-zA-Z]).([\w]+)$), 0 , 1 , 2, INDEX_NONE, 3 },
		{ CODE_STRING(([\w]+)-([\d]+)([\a-zA-Z])-([\w.]+).([\w]+)$), 0 , 1 , 2, 3, 4 },
		{ CODE_STRING(([\w]+)-([\d]+)-([\d+|\a-zA-Z])[_|-]([\w.]+).([\w]+)$), 0 , 1 , 2, 3, 4 },
	};

	bool parseFileName(std::string const& dirPath, std::string const fileName, FileDesc& outDesc)
	{
		for (auto pattern : PatternList)
		{
			std::regex regex(pattern.regexText);
			std::smatch matches;
			if (!std::regex_search(fileName, matches, regex, std::regex_constants::match_continuous))
				continue;

			std::string code = matches[pattern.indexCode + 1].str();
			std::transform(code.begin(), code.end(), code.begin(), ::toupper);

			std::string numberText = matches[pattern.indexNumber + 1].str();
			int number = atoi(numberText.c_str());

			int partNumber = 0;
			if (pattern.indexPartNumber != INDEX_NONE)
			{
				std::string partNumberText = matches[pattern.indexPartNumber + 1].str();
				if (FCString::IsAlpha(partNumberText[0]))
				{
					partNumber = FCString::ToUpper(partNumberText[0]) - 'A' + 1;
				}
				else
				{
					partNumber = atoi(partNumberText.c_str());
				}
			}


			bool b8K = false;
			if (pattern.indexFormat != INDEX_NONE)
			{
				std::string formatText = matches[pattern.indexFormat + 1].str();
				std::transform(formatText.begin(), formatText.end(), formatText.begin(), ::tolower);
				if (formatText.find("8k") != std::string::npos)
				{
					b8K = true;
				}
			}

			outDesc.code = std::move(code);
			outDesc.number = number;
			outDesc.partNumber = partNumber;
			outDesc.b8K = b8K;
			return true;
		}

		return false;
	}

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);
	}

	void onRender(float dFrame) override
	{
		Graphics2D& g = Global::GetGraphics2D();
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch (id)
		{
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}
protected:
};


REGISTER_STAGE_ENTRY("File Tool", FileToolStage, EExecGroup::Test);