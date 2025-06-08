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


	std::unordered_map< std::string, std::string > mReplaceMap;
	std::unordered_map< std::string, std::string > mChangeNameMap;


	std::unordered_map< std::string, int > mFormatDigitMap;


	struct PatternDesc
	{
		std::string regexText;
		std::regex  regex;
		int indexCode;
		int indexNumber;
		int indexPartNumber;
		int indexFormat;
		int indexFileFormat;
	};


	void loadConfigMap(char const* keyName, std::unordered_map< std::string, std::string >& outMap)
	{
		TArray<char const*> names;
		::Global::GameConfig().getStringValues(keyName, CONFIG_SECTION, names);
		for (auto str : names)
		{
			StringView key;
			if (FStringParse::StringToken(str, ",", key))
			{
				++str;
				outMap[key.toStdString()] = str;
			}
		}
	}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();

		loadConfigMap("Replace", mReplaceMap);
		loadConfigMap("ChangeName", mChangeNameMap);

		{
			TArray<char const*> formatDigits;
			::Global::GameConfig().getStringValues("FormatDigit", CONFIG_SECTION, formatDigits);
			for (auto str : formatDigits)
			{
				StringView key;
				if (FStringParse::StringToken(str, ",", key))
				{
					++str;
					mFormatDigitMap[key.toStdString()] = atoi(str);
				}
			}
		}

		{
			TArray<char const*> patterns;
			::Global::GameConfig().getStringValues("Pattern", CONFIG_SECTION, patterns);
			for (auto str : patterns)
			{
				PatternDesc desc;
				StringView key;
				if (!FStringParse::StringToken(str, ",", key))
					continue;
				
				try
				{
					desc.regex = std::regex(key.toStdString());
				}
				catch (std::regex_error&)
				{
					continue;
				}

				++str;
				if (!FStringParse::StringToken(str, ",", key))
					continue;
				desc.indexCode = key.toValue<int>();

				++str;
				if (!FStringParse::StringToken(str, ",", key))
					continue;
				desc.indexNumber = key.toValue<int>();

				++str;
				if (!FStringParse::StringToken(str, ",", key))
					continue;
				desc.indexPartNumber = key.toValue<int>();

				++str;
				if (!FStringParse::StringToken(str, ",", key))
					continue;
				desc.indexFormat = key.toValue<int>();

				++str;
				if (!FStringParse::StringToken(str, ",", key))
					continue;
				desc.indexFileFormat = key.toValue<int>();

				mPatternList.push_back(std::move(desc));
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


		frame->addButton("Check Change Name", [this](int event, GWidget*)
		{
			TArray<char const*> checkFolders;
			::Global::GameConfig().getStringValues("CheckDir", CONFIG_SECTION, checkFolders);

			mFolderNameSet.clear();
			for (auto path : checkFolders)
			{
				LogMsg("=Check Folder : %s", path);
				checkChangeName(path);
			}
			return true;
		});


		restart();
		return true;
	}

	bool replaceName(std::string& inoutStr)
	{
		auto iter = mReplaceMap.find(inoutStr);

		if (iter != mReplaceMap.end())
		{
			inoutStr = iter->second;
			return true;
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

	bool changeName(std::string const& str, std::string& outNewName)
	{
		auto iter = mChangeNameMap.find(str);

		if (iter != mChangeNameMap.end())
		{
			outNewName = iter->second;
			return true;
		}

		return false;
	}
	void checkChangeName(char const* dirPath)
	{
		std::regex regex(CODE_STRING(([\w]+)-([\d]+|[\d]+[A-Z])$));

		int changeCount = 0;
		FileIterator fileIter;
		if (FFileSystem::FindFiles(dirPath, fileIter))
		{
			for (; fileIter.haveMore(); fileIter.goNext())
			{
				if (!fileIter.isDirectory())
					continue;

				if (IsSystemFolder(fileIter.getFileName()))
					continue;

				std::smatch matches;
				std::string fileName(fileIter.getFileName());

				if (fileName.back() == 'U')
					fileName.pop_back();
				if (fileName.back() == 'B')
					fileName.pop_back();
				if (fileName.back() == 'K')
					fileName.pop_back();

				if (!std::regex_search(fileName, matches, regex, std::regex_constants::match_continuous))
					continue;

				std::string newCodeName;
				if (!changeName(matches[1].str(), newCodeName))
					continue;


				std::string newFolderName = fileIter.getFileName();
				newFolderName.replace(0, matches[1].length(), newCodeName);

				std::string newSubFileName = fileIter.getFileName();
				newSubFileName.replace(0, matches[1].length(), newCodeName);

				InlineString<256> folderPath;
				folderPath.format("%s\\%s", dirPath, fileIter.getFileName());
				LogMsg("Change Folder Name: %s => %s", folderPath.c_str(), newFolderName.c_str());

				FileIterator subFileIter;
				if (FFileSystem::FindFiles(folderPath.c_str(), subFileIter))
				{
					for (; subFileIter.haveMore(); subFileIter.goNext())
					{
						if (subFileIter.isDirectory())
							continue;

						if (FCString::CompareN(subFileIter.getFileName(), fileName.c_str(), fileName.length()) != 0)
							continue;


						std::string newName = subFileIter.getFileName();
						newName.replace(0, fileName.length(), newSubFileName);

						std::string oldSubFilePath = std::string(folderPath) + "\\" + subFileIter.getFileName();
						LogMsg("Change File Name: %s => %s", subFileIter.getFileName(), newName.c_str());
						FFileSystem::RenameFile(oldSubFilePath.c_str(), newName.c_str());
					}
				}

				FFileSystem::RenameFile(folderPath.c_str(), newFolderName.c_str());
				++changeCount;
			}
		}


		LogMsg("==Check Complete, %u Folders", changeCount);
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

	int getDigitCount(std::string const& name)
	{
		auto iter = mFormatDigitMap.find(name);
		if ( iter != mFormatDigitMap.end())
			return iter->second;

		return 3;
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

					int digit = getDigitCount(desc.code);

					InlineString<256> newName;
					if (desc.partNumber)
						newName.format("%s-%0*d-%c.%s", desc.code.c_str(), digit, desc.number, 'A' + (desc.partNumber-1), subName);
					else
						newName.format("%s-%0*d.%s", desc.code.c_str(), digit, desc.number, subName);

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
					newDir.format("%s/%s-%0*d%s", dirPath, desc.code.c_str(), digit, desc.number, desc.b8K ? "K" : "");
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

					int digit = getDigitCount(desc.code);
					auto iter = mFormatDigitMap.find(desc.code);

					replaceName(desc.code);
					InlineString<256> newName;
					newName.format("%s-%0*d.%s", desc.code.c_str(), digit, desc.number, subName);

					InlineString<256> filePath;
					filePath.format("%s/%s", dirPath, fileIter.getFileName());

					if (newName != fileIter.getFileName())
					{
						LogMsg("Rename: %s => %s", fileIter.getFileName(), newName.c_str());
						FFileSystem::RenameFile(filePath, newName);
						filePath.format("%s/%s", dirPath, newName.c_str());
					}

					InlineString<256> newDir;
					newDir.format("%s/%s-%0*d", dirPath, desc.code.c_str(), digit, desc.number);
					if (!FFileSystem::IsExist(newDir))
					{
						newDir.format("%s/%s-%0*dK", dirPath, desc.code.c_str(), digit, desc.number);
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

	TArray<PatternDesc> mPatternList;
#if 0
	static constexpr PatternDesc PatternList[] =
	{
		{ CODE_STRING(([\w]+)-([\d]+).([\w]+)$), 0 , 1 , INDEX_NONE, INDEX_NONE, 3 },
		{ CODE_STRING(([\w]+)-([\d]+)-([\d+|\a-zA-Z]).([\w]+)$), 0 , 1 , 2, INDEX_NONE, 3 },
		{ CODE_STRING(([\w]+)-([\d]+)([\a-zA-Z]).([\w]+)$), 0 , 1 , 2, INDEX_NONE, 3 },
		{ CODE_STRING(([\w]+)-([\d]+)([\a-zA-Z])-([\w.]+).([\w]+)$), 0 , 1 , 2, 3, 4 },
		{ CODE_STRING(([\w]+)-([\d]+)-([\d+|\a-zA-Z])[_|-]([\w.]+).([\w]+)$), 0 , 1 , 2, 3, 4 },
	};
#endif

	bool parseFileName(std::string const& dirPath, std::string const& fileName, FileDesc& outDesc)
	{
		for (auto const& pattern : mPatternList)
		{
			std::smatch matches;
			if (!std::regex_search(fileName, matches, pattern.regex, std::regex_constants::match_continuous))
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