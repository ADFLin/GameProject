#include "ShaderCompiler.h"

#include "FileSystem.h"

#include "CPreprocessor.h"
#include "BitUtility.h"

#include <fstream>
#include <sstream>
#include <iterator>

namespace RenderGL
{

	char const* ShaderPosfixNames[] =
	{
		"VS" SHADER_FILE_SUBNAME ,
		"PS" SHADER_FILE_SUBNAME ,
		"GS" SHADER_FILE_SUBNAME ,
		"CS" SHADER_FILE_SUBNAME ,
		"HS" SHADER_FILE_SUBNAME ,
		"DS" SHADER_FILE_SUBNAME ,
	};

	char const* gShaderDefines[] =
	{
		"#define VERTEX_SHADER 1\n" ,
		"#define PIXEL_SHADER 1\n" ,
		"#define GEOMETRY_SHADER 1\n" ,
		"#define COMPUTE_SHADER 1\n" ,
		"#define HULL_SHADER 1\n" ,
		"#define DOMAIN_SHADER 1\n" ,
	};

#if CORE_SHARE_CODE
	ShaderManager& ShaderManager::Get()
	{
		static ShaderManager sInatance;
		return sInatance;
	}
#endif //CORE_SHARE_CODE

	ShaderManager::~ShaderManager()
	{
		for( auto& pair : mShaderCompileMap )
		{
			delete pair.second;
		}
	}

	void ShaderManager::clearnupRHIResouse()
	{
		cleanupGlobalShader();
	}

	GlobalShaderProgram* ShaderManager::getGlobalShader(GlobalShaderProgramClass& shaderClass, bool bForceLoad)
	{
		GlobalShaderProgram* result = mGlobalShaderMap[&shaderClass];
		if( result == nullptr && bForceLoad )
		{
			result = constructGlobalShader(shaderClass);
			if( result )
			{
				mGlobalShaderMap[&shaderClass] = result;
			}
		}
		return result;
	}

	bool ShaderManager::registerGlobalShader(GlobalShaderProgramClass& shaderClass)
	{
		mGlobalShaderMap.insert({ &shaderClass , nullptr });
		return true;
	}

	int ShaderManager::loadAllGlobalShaders()
	{
		int numFailLoad = 0;
		for( auto& pair : mGlobalShaderMap )
		{
			GlobalShaderProgram* program = pair.second;
			GlobalShaderProgramClass* shaderClass = pair.first;
			if( program == nullptr )
			{
				program = constructGlobalShader(*shaderClass);
				mGlobalShaderMap[shaderClass] = program;
			}
			if( program == nullptr )
			{
				++numFailLoad;
			}
		}
		return numFailLoad;
	}

	GlobalShaderProgram* ShaderManager::constructGlobalShader(GlobalShaderProgramClass& shaderClass)
	{
		GlobalShaderProgram* result = (*shaderClass.funCreateShader)();
		if( result )
		{
			result->myClass = &shaderClass;

			ShaderCompileOption option;
			option.version = mDefaultVersion;
			(*shaderClass.funSetupShaderCompileOption)(option);

			if( !loadInternal(*result, (*shaderClass.funGetShaderFileName)(), (*shaderClass.funGetShaderEntries)(), option, nullptr, true) )
			{
				LogWarning(0, "Can't Load Global Shader %s", (*shaderClass.funGetShaderFileName)());

				delete result;
				result = nullptr;
			}

			mShaderCompileMap[result]->bGlobalShader = true;
		}

		return result;
	}

	void ShaderManager::cleanupGlobalShader()
	{
		for( auto& pair : mGlobalShaderMap )
		{
			delete pair.second;
			pair.second = nullptr;
		}
	}

	bool ShaderManager::loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def, char const* additionalCode)
	{
		ShaderEntryInfo entries[] =
		{
			{ Shader::eVertex , SHADER_ENTRY(MainVS) } ,
			{ Shader::ePixel , SHADER_ENTRY(MainPS) } ,
			{ Shader::eEmpty , nullptr } ,
		};
		return loadFile(shaderProgram , fileName, entries , def, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, char const* def, char const* additionalCode)
	{
		ShaderEntryInfo entries[] =
		{
			{ Shader::eVertex , vertexEntryName } ,
			{ Shader::ePixel , pixelEntryName } ,
			{ Shader::eEmpty , nullptr } ,
		};
		return loadFile(shaderProgram, fileName, entries, def, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def, char const* additionalCode)
	{
		return loadInternal(shaderProgram, fileName, shaderMask, entryNames, def, additionalCode, true);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode /*= nullptr*/)
	{
		return loadInternal(shaderProgram, fileName, shaderMask, entryNames, option, additionalCode, true);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, ShaderCompileOption const& option, char const* additionalCode /*= nullptr*/)
	{
		ShaderEntryInfo entries[] =
		{
			{ Shader::eVertex , vertexEntryName } ,
			{ Shader::ePixel , pixelEntryName } ,
			{ Shader::eEmpty , nullptr } ,
		};
		return loadFile(shaderProgram, fileName, entries, option, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[], char const* def /*= nullptr*/, char const* additionalCode /*= nullptr*/)
	{
		return loadInternal(shaderProgram, fileName, entries, def, additionalCode, true);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[], ShaderCompileOption const& option, char const* additionalCode /*= nullptr*/)
	{
		return loadInternal(shaderProgram, fileName, entries, option, additionalCode, true);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingle)
	{
		ShaderEntryInfo entries[Shader::NUM_SHADER_TYPE];
		MakeEntryInfos(entries, shaderMask, entryNames);
		return loadInternal(shaderProgram, fileName, entries, option, additionalCode, bSingle);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[] , ShaderCompileOption const& option, char const* additionalCode, bool bSingle)
	{
		ShaderProgramCompileInfo* info = new ShaderProgramCompileInfo;

		info->shaderProgram = &shaderProgram;
		info->bSingleFile = bSingle;
		info->fileName = fileName;

		generateCompileSetup(*info, entries, option, additionalCode);

		if( !updateShaderInternal(shaderProgram, *info) )
		{
			delete info;
			return false;
		}

		mShaderCompileMap.insert({ &shaderProgram , info });
		return true;
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def , char const* additionalCode, bool bSingle )
	{
		ShaderEntryInfo enties[Shader::NUM_SHADER_TYPE];
		MakeEntryInfos(enties, shaderMask, entryNames);
		return loadInternal(shaderProgram, fileName, enties, def, additionalCode, bSingle);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[], char const* def, char const* additionalCode, bool bSingle)
	{
		ShaderProgramCompileInfo* info = new ShaderProgramCompileInfo;

		info->shaderProgram = &shaderProgram;
		info->bSingleFile = bSingle;
		info->fileName = fileName;

		int indexNameUsed = 0;
		for( int i = 0; entries[i].type != Shader::eEmpty; ++i )
		{
			auto const& entry = entries[i];

			std::string headCode;
			{
				headCode += "#version ";
				FixString<128> str;
				headCode += str.format("%u", mDefaultVersion);
				headCode += " compatibility\n";
			}

			headCode += "#define COMPILER_GLSL 1\n";
			headCode += gShaderDefines[entry.type];

			if( def )
			{
				headCode += def;
				headCode += '\n';
			}
			
			if( entry.name )
			{
				headCode += "#define ";
				headCode += entry.name;
				headCode += " main\n";
				++indexNameUsed;
			}

			if( additionalCode )
			{
				headCode += additionalCode;
				headCode += '\n';
			}

			info->shaders.push_back({ entry.type, std::move(headCode) });
		}

		if( !updateShaderInternal(shaderProgram, *info) )
		{
			delete info;
			return false;
		}

		mShaderCompileMap.insert({ &shaderProgram , info });
		return true;
	}

	bool ShaderManager::loadMultiFile(ShaderProgram& shaderProgram, char const* fileName, char const* def, char const* additionalCode)
	{
		return loadInternal(shaderProgram, fileName, BIT(Shader::eVertex) | BIT(Shader::ePixel), nullptr, def, additionalCode, false);
	}

	bool ShaderManager::reloadShader(ShaderProgram& shaderProgram)
	{
		auto iter = mShaderCompileMap.find(&shaderProgram);
		if( iter == mShaderCompileMap.end() )
			return false;

		ShaderProgramCompileInfo* info = iter->second;
		if( info->bGlobalShader )
		{
			ShaderCompileOption option;
			option.version = mDefaultVersion;

			GlobalShaderProgramClass& myClass = *static_cast<GlobalShaderProgram&>(shaderProgram).myClass;
			(*myClass.funSetupShaderCompileOption)(option);

			info->shaders.clear();
			generateCompileSetup(*info, (*myClass.funGetShaderEntries)(), option, nullptr);
		}

		return updateShaderInternal(shaderProgram, *info);
	}


	void ShaderManager::reloadAll()
	{
		for( auto pair : mShaderCompileMap )
		{
			updateShaderInternal(*pair.first, *pair.second);
		}
	}

	bool ShaderManager::updateShaderInternal(ShaderProgram& shaderProgram, ShaderProgramCompileInfo& info)
	{
		if( !shaderProgram.isValid() )
		{
			if( !shaderProgram.create() )
				return false;
		}

		int indexCode = 0;
		for( ShaderCompileInfo const& shaderInfo : info.shaders )
		{

			FixString< 256 > path;
			if( info.bSingleFile )
			{
				path.format("%s%s", info.fileName.c_str(), SHADER_FILE_SUBNAME );
			}
			else
			{
				path.format("%s%s", info.fileName.c_str(), ShaderPosfixNames[shaderInfo.type]);
			}
			
			RHIShaderRef shader = shaderProgram.mShaders[shaderInfo.type];
			if( shader == nullptr )
			{
				shader = new RHIShader;
				if( !mCompiler.compileCode(shaderInfo.type, *shader, path, shaderInfo.headCode.c_str()) )
					return false;
				shaderProgram.attachShader(*shader);
			}
			else
			{
				if( !mCompiler.compileCode(shaderInfo.type, *shader, path, shaderInfo.headCode.c_str()) )
					return false;
			}
			++indexCode;
		}
		shaderProgram.updateShader(true);

		return true;
	}

	void ShaderManager::generateCompileSetup(ShaderProgramCompileInfo& compileInfo, ShaderEntryInfo const entries[], ShaderCompileOption const& option, char const* additionalCode)
	{
		assert(compileInfo.shaders.empty());

		for( int i = 0; entries[i].type != Shader::eEmpty; ++i )
		{
			auto const& entry = entries[i];
			std::string defCode;
			defCode += "#define COMPILER_GLSL 1\n";
			defCode += gShaderDefines[entry.type];
			if( entry.name )
			{
				defCode += "#define ";
				defCode += entry.name;
				defCode += " main\n";
			}

			std::string headCode = option.getCode(defCode.c_str(), additionalCode);
			compileInfo.shaders.push_back({ entry.type , std::move(headCode) });
		}
	}

	bool ShaderCompiler::compileCode(Shader::Type type, RHIShader& shader, char const* path, char const* def)
	{
		bool bSuccess;
		do
		{
			bSuccess = false;
			std::vector< char > codeBuffer;
			if( !FileUtility::LoadToBuffer(path, codeBuffer, true) )
				return false;
			int numSourceCodes = 0;
			char const* sourceCodes[2];

			if( bUsePreprocess )
			{
				CPP::CodeInput sourceInput;

				if( def )
				{
					sourceInput.appendString(def);
				}
				sourceInput.appendString(&codeBuffer[0]);

				sourceInput.resetSeek();

				CPP::Preprocessor preporcessor;

				std::stringstream oss;
				CPP::CodeOutput codeOutput(oss);

				char const* DefaultDir = "Shader";
				preporcessor.setOutput(codeOutput);
				preporcessor.addSreachDir(DefaultDir);
				char const* dirPathEnd = FileUtility::GetDirPathPos(path);
				if( strncmp(DefaultDir, path, dirPathEnd - path) != 0 )
				{
					std::string dir(path, dirPathEnd);
					preporcessor.addSreachDir(dir.c_str());
				}

				try
				{
					preporcessor.translate(sourceInput);
				}
				catch( std::exception& e )
				{
					e.what();
					return false;
				}
#if 1
				codeBuffer.assign(std::istreambuf_iterator< char >(oss), std::istreambuf_iterator< char >());
#else
				std::string code = oss.str();
				codeBuffer.assign(code.begin(), code.end());
#endif
				codeBuffer.push_back(0);
				sourceCodes[numSourceCodes] = &codeBuffer[0];
				++numSourceCodes;
			}
			else
			{
				if( def )
				{
					sourceCodes[numSourceCodes] = def;
					++numSourceCodes;
				}
				sourceCodes[numSourceCodes] = &codeBuffer[0];
				++numSourceCodes;
			}

			bSuccess = shader.compileCode(type, sourceCodes, numSourceCodes);

			if( !bSuccess && bUsePreprocess )
			{
				{
					std::ofstream of("temp" SHADER_FILE_SUBNAME, std::ios::binary);
					if( of.is_open() )
					{
						of.write(&codeBuffer[0], codeBuffer.size());
					}
				}

				int maxLength;
				glGetShaderiv(shader.mHandle, GL_INFO_LOG_LENGTH, &maxLength);
				std::vector< char > buf(maxLength);
				int logLength = 0;
				glGetShaderInfoLog(shader.mHandle, maxLength, &logLength, &buf[0]);
				::MessageBoxA(NULL, &buf[0], "Shader Compile Error", 0);
			}

		} 
		while( !bSuccess && bRecompile );

		return bSuccess;
	}



	void ShaderManager::ShaderProgramCompileInfo::getDependentFilePaths(std::vector<std::wstring>& paths)
	{
		FixString< 256 > path;
		if( bSingleFile )
		{
			path.format("%s%s", fileName.c_str(), SHADER_FILE_SUBNAME);
			paths.push_back(FCString::CharToWChar(path));
		}
		else
		{
			for( ShaderCompileInfo const& shaderInfo  : shaders )
			{
				path.format("%s%s", fileName.c_str(), ShaderPosfixNames[shaderInfo.type]);
				paths.push_back(FCString::CharToWChar(path));
			}
		}
	}

	void ShaderManager::ShaderProgramCompileInfo::postFileModify(FileAction action)
	{
		if ( action == FileAction::Modify )
			ShaderManager::Get().updateShaderInternal(*shaderProgram, *this);
	}

	std::string ShaderCompileOption::getCode(char const* defCode /*= nullptr */, char const* addionalCode /*= nullptr */) const
	{
		std::string result;
		if( version )
		{
			result += "#version ";
			FixString<128> str;
			result += str.format("%u", version);
			result += " compatibility\n";
		}

		if( defCode )
		{
			result += defCode;
		}

		for( auto const& var : mConfigVars )
		{
			result += "#define ";
			result += var.name;
			if( var.name.length() )
			{
				result += " ";
				result += var.value;
			}
			result += "\n";
		}

		if( addionalCode )
		{
			result += addionalCode;
			result += '\n';
		}

		for( auto& name : mIncludeFiles )
		{
			result += "#include \"";
			result += name;
			result += SHADER_FILE_SUBNAME;
			result += "\"\n";
		}

		for( auto code : mCodes )
		{
			result += code;
			result += '\n';
		}

		return result;
	}

	GlobalShaderProgramClass::GlobalShaderProgramClass(
		FunCreateShader inFunCreateShader ,
		FunSetupShaderCompileOption inFunSetupShaderCompileOption, 
		FunGetShaderFileName inFunGetShaderFileName, 
		FunGetShaderEntries inFunGetShaderEntries) 
		:funCreateShader(inFunCreateShader)
		,funSetupShaderCompileOption(inFunSetupShaderCompileOption)
		,funGetShaderFileName(inFunGetShaderFileName)
		,funGetShaderEntries(inFunGetShaderEntries)
	{
		ShaderManager::Get().registerGlobalShader(*this);
	}

}//namespace GL


