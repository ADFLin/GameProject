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

	std::string mPictureDir;

	struct PatternDesc
	{
		std::regex  regex;

		int indexCode;
		int indexNumber;
		int indexPartNumber;
		int indexFormat;
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

		int a = atoi(" 3");
		auto frame = WidgetUtility::CreateDevFrame();

		loadConfigMap("Replace", mReplaceMap);
		loadConfigMap("ChangeName", mChangeNameMap);

		auto& config = ::Global::GameConfig();
		mPictureDir = config.getStringValue("PictureDir", CONFIG_SECTION, "");

		{
			TArray<char const*> formatDigits;
			config.getStringValues("FormatDigit", CONFIG_SECTION, formatDigits);
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

#if 1
		{
			TArray<char const*> patterns;
			config.getStringValues("Pattern", CONFIG_SECTION, patterns);
			int index = 0;
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

				auto TokenInt = [&str](int& outValue) -> bool 
				{
					StringView key;
					if (!FStringParse::StringToken(str, ",", key))
						return false;
					key.trimStartAndEnd();
					outValue = key.toValue<int>();
					return true;
				};

				if (!TokenInt(desc.indexCode))
					continue;
				if (!TokenInt(desc.indexNumber))
					continue;
				if (!TokenInt(desc.indexPartNumber))
					continue;
				if (!TokenInt(desc.indexFormat))
					continue;

				mPatternList.push_back(std::move(desc));
			}
		}
#endif
		frame->addButton("Conv Path", [this](int event, GWidget*)
		{
			char const* dirPath = ::Global::GameConfig().getStringValue("LastDir", CONFIG_SECTION, "");
			char path[1024];
			if (SystemPlatform::OpenDirectoryName(path, ARRAYSIZE(path), dirPath , "Select Convert Directory" , ::Global::GetDrawEngine().getWindowHandle()))
			{
				TIME_SCOPE("Convert File Name");
				::Global::GameConfig().setKeyValue("LastDir", CONFIG_SECTION, path);
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

		GTextCtrl* textCtrl = new GTextCtrl(UI_ANY, Vec2i(100, 100), 200, nullptr);
		textCtrl->onEvent = [this](int event, GWidget* widget) -> bool
		{
			if (event == EVT_TEXTCTRL_COMMITTED)
			{
				auto myWidget = widget->cast<GTextCtrl>();
				FileDesc desc;
				int indexPattern = parseFileName(myWidget->getValue(), desc);
				if (indexPattern != INDEX_NONE)
				{
					LogMsg("Parse Success: %s =[%d] %s-%d-%d %s", myWidget->getValue(), indexPattern, desc.code.c_str(), desc.number, desc.partNumber, desc.b8K ? "8k" : "");
				}
				else
				{
					LogMsg("Parse Fail: %s", myWidget->getValue());
				}
			}
			return false;
		};
		::Global::GUI().addWidget(textCtrl);
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

		void getFileName(int digit, char const* subName, InlineString<256>& outName)
		{
			if (partNumber)
				outName.format("%s-%0*d-%c.%s", code.c_str(), digit, number, 'A' + (partNumber - 1), subName);
			else
				outName.format("%s-%0*d.%s", code.c_str(), digit, number, subName);
		}
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

				std::string fileName(fileIter.getFileName());

				if (fileName.back() == 'U')
					fileName.pop_back();
				if (fileName.back() == 'B')
					fileName.pop_back();
				if (fileName.back() == 'K')
					fileName.pop_back();

				std::smatch matches;
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
				if (subName == nullptr || FCString::Compare(subName, "jpg") == 0)
					continue;

				FileDesc desc;
				std::string fileName = std::string(fileIter.getFileName(), subName - fileIter.getFileName() - 1);
				if (parseFileName(fileName, desc) != INDEX_NONE)
				{
					replaceName(desc.code);

					int digit = getDigitCount(desc.code);

					InlineString<256> newName;
					desc.getFileName(digit, subName, newName);

					//LogMsg("%s => %s %d %d %s", fileIiter.getFileName(), desc.code.c_str(), desc.number, desc.partNumber, desc.b8K ? "true" : "false");

					InlineString<256> filePath;
					filePath.format("%s/%s", dirPath, fileIter.getFileName());

					InlineString<256> newDir;
					newDir.format("%s/%s-%0*d%s", dirPath, desc.code.c_str(), digit, desc.number, desc.b8K ? "K" : "");
					if (!FFileSystem::IsExist(newDir))
					{
						if (!FFileSystem::CreateDirectory(newDir))
						{
							continue;
						}
					}

					if (newName != fileIter.getFileName())
					{
						InlineString<256> newPath;
						newPath.format("%s/%s", newDir.c_str(), newName.c_str());
						LogMsg("RenameAndMove: %s => %s", filePath.c_str(), newPath.c_str());
						FFileSystem::RenameAndMoveFile(filePath, newPath);
					}
					else
					{
						LogMsg("Move: %s => %s", filePath, newDir);
						FFileSystem::MoveFile(filePath, newDir);
					}

					if (desc.partNumber == 0 || desc.partNumber == 1)
					{
						if (!mPictureDir.empty())
						{
							InlineString<256> picName;
							picName.format("%s-%0*d.%s", desc.code.c_str(), digit, desc.number, "jpg");

							InlineString<256> picPath;
							picPath.format("%s/%s", mPictureDir.c_str(), picName.c_str());
							if (FFileSystem::IsExist(picPath))
							{
								LogMsg("Move Pic: %s => %s", picPath, newDir);
								FFileSystem::MoveFile(picPath, newDir);
							}
						}
					}
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
				if (parseFileName(fileIter.getFileName(), desc) != INDEX_NONE)
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

	int parseFileName(std::string const& fileName, FileDesc& outDesc)
	{
		for (auto const& pattern : mPatternList)
		{
			std::smatch matches;
			if (!std::regex_search(fileName, matches, pattern.regex, std::regex_constants::match_continuous))
				continue;

			outDesc.code = matches[pattern.indexCode + 1].str();
			std::transform(outDesc.code.begin(), outDesc.code.end(), outDesc.code.begin(), ::toupper);

			std::string numberText = matches[pattern.indexNumber + 1].str();
			outDesc.number = atoi(numberText.c_str());

			outDesc.partNumber = 0;
			if (pattern.indexPartNumber != INDEX_NONE)
			{
				std::string partNumberText = matches[pattern.indexPartNumber + 1].str();
				if (!partNumberText.empty())
				{
					if (FCString::IsAlpha(partNumberText[0]))
					{
						outDesc.partNumber = FCString::ToUpper(partNumberText[0]) - 'A' + 1;
					}
					else
					{
						outDesc.partNumber = atoi(partNumberText.c_str());
					}
				}
			}

			outDesc.b8K = false;
			if (pattern.indexFormat != INDEX_NONE)
			{
				std::string formatText = matches[pattern.indexFormat + 1].str();
				std::transform(formatText.begin(), formatText.end(), formatText.begin(), ::tolower);
				if (formatText.find("8k") != std::string::npos)
				{
					outDesc.b8K = true;
				}
			}

			return &pattern - mPatternList.data();
		}

		return INDEX_NONE;
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