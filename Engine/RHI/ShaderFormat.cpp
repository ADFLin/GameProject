#include "ShaderFormat.h"

#include "CPreprocessor.h"
#include "FileSystem.h"
#include "ProfileSystem.h"
#include "InlineString.h"


#include <sstream>

namespace Render
{
	bool ShaderFormat::preprocessCode(char const* path, ShaderCompileDesc* compileDesc, StringView const& definition, CPP::CodeSourceLibrary* sourceLibrary, TArray<uint8>& inoutCodes, std::unordered_set<HashString>* outIncludeFiles, bool bOuputPreprocessedCode)
	{
		TimeScope scope("PreprocessCode");

#define USE_MULTI_INPUT 1

		CPP::CodeBufferSource source;
		CPP::Preprocessor preprocessor;

#if USE_MULTI_INPUT
		if (path)
		{
			if (!source.loadFile(path))
			{
				LogWarning(0, "Can't load shader file %s", path);
				return false;
			}
			source.filePath = path;
			preprocessor.pushInput(source);
		}

		CPP::CodeStringSource definitionSource;
		if (definition.size())
		{
			definitionSource.mString = definition;
			preprocessor.pushInput(definitionSource);
		}
#else

		if (path)
		{
			if (definition.size())
			{
				source.appendString(definition);
				source.lineOffset = -FStringParse::CountChar(definition.data(), definition.data() + definition.size() + 1, '\n');
			}
			if (!source.appendFile(path))
			{
				LogWarning(0, "Can't load shader file %s", path);
				return false;
			}

			source.filePath = path;
		}
		else
		{
			if (definition.size())
			{
				source.appendString(definition);
			}
			source.filePath = "RuntimeCode";
		}

		bool bOuputUnPreprocessedCode = false;
		if (bOuputUnPreprocessedCode)
		{
#if 0
			FFileSystem::CreateDirectory("ShaderOutput");
			std::string outputPath = "ShaderOutput/";
			if (path)
			{
				outputPath += FFileUtility::GetBaseFileName(path).toCString();
			}
			else
			{
				outputPath += "RuntimeCode";
			}
			if (compileDesc)
			{
				outputPath += "_";
				outputPath += compileDesc->entryName.c_str();
			}
			outputPath += "_";
			outputPath += getName();
			outputPath += SHADER_FILE_SUBNAME;
#else
			std::string outputPath = "temp_unpreprocessed.sgc";
#endif
			FFileUtility::SaveFromBuffer(outputPath.c_str(), source.getBuffer().data(), source.getBuffer().size());
		}

		preprocessor.pushInput(source);
#endif

		auto settings = getPreprocessSettings();

		if (sourceLibrary)
		{
			preprocessor.setSourceLibrary(*sourceLibrary);
		}
		preprocessor.bReplaceMacroText = true;
		preprocessor.bAllowRedefineMacro = true;
		preprocessor.bAddLineMacro = settings.bAddLineMacro;
		preprocessor.lineFormat = (settings.bSupportLineFilePath) ? CPP::Preprocessor::LF_LineNumberAndFilePath : CPP::Preprocessor::LF_LineNumber;
		std::stringstream oss;
		CPP::CodeOutput codeOutput(oss);

		char const* DefaultDir = "Shader";
		preprocessor.setOutput(codeOutput);
		preprocessor.addSearchDir(DefaultDir);
		if ( path )
		{
			StringView dir = FFileUtility::GetDirectory(path);
			if (dir != DefaultDir)
			{
				preprocessor.addSearchDir(dir.toCString());
			}
		}

		try
		{
			preprocessor.translate();
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

#if 0
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
			if (path)
			{
				outputPath += FFileUtility::GetBaseFileName(path).toCString();
			}
			else
			{
				outputPath += "RuntimeCode";
			}
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

	bool ShaderFormat::loadCode(ShaderCompileContext const& context, TArray<uint8>& outCodes)
	{
		if (context.bUsePreprocess)
		{
			if (context.haveFile())
			{
				if (!FFileUtility::LoadToBuffer(context.getPath(), outCodes, true))
				{
					LogWarning(0, "Can't load shader file %s", context.getPath());
					return false;
				}
			}
			return preprocessCode(context.haveFile() ? context.getPath() : nullptr, context.desc, context.getDefinition(), context.sourceLibrary, outCodes, context.includeFiles, context.bOuputPreprocessedCode);
		}
		else
		{
			StringView definition = context.getDefinition();
			if (definition.size())
			{
				outCodes.resize(definition.size());
				outCodes.assign(definition.data(), definition.data() + definition.size());
			}

			if (context.haveFile())
			{
				if (!FFileUtility::LoadToBuffer(context.getPath(), outCodes, true, true))
				{
					LogWarning(0, "Can't load shader file %s", context.getPath());
					return false;
				}
			}
		}

		return true;
	}

	void ShaderFormat::emitCompileError(ShaderCompileContext const& context, char const* errorCode)
	{
		if (context.bAllowRecompile)
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
		else
		{
			LogWarning(0, errorCode);
		}
	}

	void ShaderFormat::OutputError(char const* title, char const* text)
	{
#if SYS_PLATFORM_WIN
		::MessageBoxA(NULL, text, (title) ? InlineString<256>::Make("Shader Compile Error : %s", title ).c_str() :"Shader Compile Error" , 0);
#endif
	}

	void ShaderCompileContext::checkOuputDebugCode() const
	{
		if (!bUsePreprocess)
			return;

		FFileUtility::SaveFromBuffer("temp" SHADER_FILE_SUBNAME, (uint8 const*)codes[0].data(), codes[0].size());
	}

}//namespace Render


