#include "ShaderFormat.h"

#include "CPreprocessor.h"
#include "FileSystem.h"

#include <sstream>
#include "ProfileSystem.h"
#include "InlineString.h"


namespace Render
{
	bool ShaderFormat::preprocessCode(char const* path, ShaderCompileInfo* compileInfo, char const* def, CPP::CodeSourceLibrary* sourceLibrary, std::vector<char>& inoutCodes)
	{
		TimeScope scope("PreprocessCode");

		CPP::CodeSource source;

		if (def)
		{
			int len = strlen(def);
			source.appendString(StringView(def, len));
			source.lineOffset = -FStringParse::CountChar(def, def + len + 1, '\n');
		}
		source.appendString( StringView(&inoutCodes[0] , inoutCodes.size() ));
		source.filePath = path;

		auto settings = getPreprocessSettings();

		CPP::Preprocessor preprocessor;
		if (sourceLibrary)
		{
			preprocessor.setSourceLibrary(*sourceLibrary);
		}
		preprocessor.bReplaceMarcoText = true;
		preprocessor.lineFormat = (settings.bSupportLineFilePath) ? CPP::Preprocessor::LF_LineNumberAndFilePath : CPP::Preprocessor::LF_LineNumber;
		std::stringstream oss;
		CPP::CodeOutput codeOutput(oss);

		char const* DefaultDir = "Shader";
		preprocessor.setOutput(codeOutput);
		preprocessor.addSreachDir(DefaultDir);
		char const* dirPathEnd = FFileUtility::GetFileName(path);
		if (dirPathEnd != path)
		{
			--dirPathEnd;
		}
		if (strncmp(DefaultDir, path, dirPathEnd - path) != 0)
		{
			std::string dir(path, dirPathEnd);
			preprocessor.addSreachDir(dir.c_str());
		}

		try
		{
			preprocessor.translate(source);
		}
		catch (std::exception& e)
		{
			LogWarning( 0 , "Preprocess Shader code fail : %s" , e.what() );
			return false;
		}

		if (compileInfo)
		{
			preprocessor.getUsedIncludeFiles(compileInfo->includeFiles);
		}
#if 1
		inoutCodes.assign(std::istreambuf_iterator< char >(oss), std::istreambuf_iterator< char >());
#else
		std::string code = oss.str();
		inoutCodes.assign(code.begin(), code.end());
#endif
		inoutCodes.push_back('\0');

		if (bOuputPreprocessedCode)
		{
			FFileSystem::CreateDirectory("ShaderOutput");
			std::string outputPath = "ShaderOutput/";
			outputPath += FFileUtility::GetBaseFileName(path).toCString();
			outputPath += "_";
			outputPath += compileInfo->entryName.c_str();
			outputPath += "_";
			outputPath += getName();
			outputPath += SHADER_FILE_SUBNAME;
			FFileUtility::SaveFromBuffer(outputPath.c_str(), &inoutCodes[0], inoutCodes.size() - 1);
		}
		return true;
	}


	void ShaderFormat::emitCompileError(ShaderCompileInput const& input, char const* errorCode)
	{
		std::string title;
		title += FFileUtility::GetBaseFileName(input.path).toCString();
		title += "_";
		title += input.entry;
		title += "_";
		title += getName();
		title += SHADER_FILE_SUBNAME;

		OutputError(title.c_str(), errorCode);
	}

	void ShaderFormat::OutputError(char const* title, char const* text)
	{
#if SYS_PLATFORM_WIN
		::MessageBoxA(NULL, text, (title) ? InlineString<256>::Make("Shader Compile Error : %s", title ).c_str() :"Shader Compile Error" , 0);
#endif
	}

}//namespace Render


