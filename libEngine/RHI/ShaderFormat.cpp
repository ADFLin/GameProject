#include "ShaderFormat.h"

#include "ShaderManager.h"

#include "CPreprocessor.h"
#include "FileSystem.h"

#include <sstream>
#include "ProfileSystem.h"
#include "InlineString.h"

namespace Render
{
	bool ShaderFormat::preprocessCode(char const* path, ShaderCompileDesc* compileDesc, StringView const& definition, CPP::CodeSourceLibrary* sourceLibrary, std::vector<uint8>& inoutCodes, std::unordered_set<HashString>* outIncludeFiles)
	{
		TimeScope scope("PreprocessCode");

		CPP::CodeSource source;

		if (definition.size())
		{
			source.appendString(definition);
			source.lineOffset = -FStringParse::CountChar(definition.data(), definition.data() + definition.size() + 1, '\n');
		}
		source.appendString( StringView((char const*)inoutCodes.data() , inoutCodes.size() ));
		source.filePath = path;

		auto settings = getPreprocessSettings();

		CPP::Preprocessor preprocessor;
		if (sourceLibrary)
		{
			preprocessor.setSourceLibrary(*sourceLibrary);
		}
		preprocessor.bReplaceMarcoText = true;
		//preprocessor.bAllowRedefineMarco = true;
		preprocessor.lineFormat = (settings.bSupportLineFilePath) ? CPP::Preprocessor::LF_LineNumberAndFilePath : CPP::Preprocessor::LF_LineNumber;
		std::stringstream oss;
		CPP::CodeOutput codeOutput(oss);

		char const* DefaultDir = "Shader";
		preprocessor.setOutput(codeOutput);
		preprocessor.addSreachDir(DefaultDir);
		StringView dir = FFileUtility::GetDirectory(path);
		if (dir != DefaultDir)
		{
			preprocessor.addSreachDir(dir.toCString());
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

		if (outIncludeFiles)
		{
			preprocessor.getUsedIncludeFiles(*outIncludeFiles);
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
			if ( compileDesc )
			{
				outputPath += "_";
				outputPath += compileDesc->entryName.c_str();
			}
			outputPath += "_";
			outputPath += getName();
			outputPath += SHADER_FILE_SUBNAME;
			FFileUtility::SaveFromBuffer(outputPath.c_str(), inoutCodes.data(), inoutCodes.size() - 1);
		}
		return true;
	}

	bool ShaderFormat::loadCode(ShaderCompileContext const& context, std::vector<uint8>& outCodes)
	{
		if (bUsePreprocess)
		{
			if (!FFileUtility::LoadToBuffer(context.getPath(), outCodes, true))
			{
				LogWarning(0, "Can't load shader file %s", context.getPath());
				return false;
			}
			ShaderManagedData* managedData = context.shaderSetupData ? 
				(ShaderManagedData*)context.shaderSetupData->managedData:
				(ShaderManagedData*)context.programSetupData->managedData;
			return preprocessCode(context.getPath(), context.desc, context.getDefinition().data(), context.sourceLibrary, outCodes, &managedData->includeFiles);
		}
		else
		{
			StringView definition = context.getDefinition();
			if (definition.size())
			{
				outCodes.resize(definition.size());
				outCodes.assign(definition.data(), definition.data() + definition.size());
			}

			if (!FFileUtility::LoadToBuffer(context.getPath(), outCodes, true, true))
			{
				LogWarning(0, "Can't load shader file %s", context.getPath());
				return false;
			}
		}

		return true;
	}

	void ShaderFormat::emitCompileError(ShaderCompileContext const& context, char const* errorCode)
	{
		std::string title;
		title += FFileUtility::GetBaseFileName(context.getPath()).toCString();
		title += "_";
		title += context.getEntry();
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

	int ShaderProgramSetupData::getShaderCount() const
	{
		return managedData->descList.size();
	}

}//namespace Render


