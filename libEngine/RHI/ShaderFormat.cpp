#include "ShaderFormat.h"

#include "CPreprocessor.h"
#include "FileSystem.h"

#include <sstream>
#include "ProfileSystem.h"


namespace Render
{
	bool ShaderFormat::PreprocessCode(char const* path, ShaderCompileInfo* compileInfo, char const* def, std::vector<char>& inoutCodes)
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

		CPP::Preprocessor preprocessor;

		std::stringstream oss;
		CPP::CodeOutput codeOutput(oss);

		char const* DefaultDir = "Shader";
		preprocessor.setOutput(codeOutput);
		preprocessor.addSreachDir(DefaultDir);
		char const* dirPathEnd = FileUtility::GetFileName(path);
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
			preprocessor.getIncludeFiles(compileInfo->includeFiles);
		}
#if 1
		inoutCodes.assign(std::istreambuf_iterator< char >(oss), std::istreambuf_iterator< char >());
#else
		std::string code = oss.str();
		inoutCodes.assign(code.begin(), code.end());
#endif
		inoutCodes.push_back('\0');
		return true;
	}


}//namespace Render

