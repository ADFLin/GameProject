#include "ShaderCompiler.h"

#include "MaterialShader.h"
#include "GlobalShader.h"
#include "VertexFactory.h"

#include "FileSystem.h"
#include "CPreprocessor.h"
#include "BitUtility.h"
#include "StdUtility.h"

#include <fstream>
#include <sstream>
#include <iterator>

namespace Render
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

	ShaderManager::ShaderManager()
	{
		mDefaultVersion = 430;
	}

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


	//TODO: Remove
	std::string GetFilePath(char const* name)
	{
		std::string path("Material/");
		path += name;
		path += SHADER_FILE_SUBNAME;
		return path;
	}

	int ShaderManager::loadMaterialShaders(
		MaterialShaderCompileInfo const& info, 
		VertexFactoryType& vertexFactoryType,
		MaterialShaderPairVec& outShaders )
	{
		std::string path = GetFilePath(info.name);
		std::vector< char > materialCode;
		if( !FileUtility::LoadToBuffer(path.c_str(), materialCode, true) )
			return 0;

		int result = 0;
		for( auto pShaderClass : MaterialShaderProgramClass::ClassList )
		{
			ShaderCompileOption option;
			option.version = 430;
			option.addCode(&materialCode[0]);
			vertexFactoryType.getCompileOption(option);

			switch( info.tessellationMode )
			{
			case ETessellationMode::Flat:
				option.addDefine(SHADER_PARAM(USE_TESSELLATION), true);
				break;
			case ETessellationMode::PNTriangle:
				option.addDefine(SHADER_PARAM(USE_TESSELLATION), true);
				option.addDefine(SHADER_PARAM(USE_PN_TRIANGLE), true);
				break;
			}

			MaterialShaderProgram* shaderProgram = (MaterialShaderProgram*)constructShaderInternal(*pShaderClass, ShaderClassType::Material, option );

			if( shaderProgram )
			{
				outShaders.push_back({ pShaderClass , shaderProgram });
				++result;
			}
		}

		return result;
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
		ShaderCompileOption option;
		option.version = mDefaultVersion;
		return constructShaderInternal(shaderClass, ShaderClassType::Global, option);
	}

	GlobalShaderProgram* ShaderManager::constructShaderInternal(GlobalShaderProgramClass& shaderClass , ShaderClassType classType , ShaderCompileOption& option )
	{
		GlobalShaderProgram* result = (*shaderClass.funCreateShader)();
		if( result )
		{
			result->myClass = &shaderClass;
			if( option.version == 0 )
			{
				option.version = mDefaultVersion;
			}
			
			(*shaderClass.funSetupShaderCompileOption)(option);

			if( !loadInternal(*result, (*shaderClass.funGetShaderFileName)(), (*shaderClass.funGetShaderEntries)(), option, nullptr, true) )
			{
				LogWarning(0, "Can't Load Global Shader %s", (*shaderClass.funGetShaderFileName)());

				delete result;
				result = nullptr;
			}

			if( classType == ShaderClassType::Material )
			{
				removeFromShaderCompileMap(*result);
			}
			else
			{
				mShaderCompileMap[result]->classType = classType;
			}			
		}
		else
		{

			
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
		char const* filePaths[Shader::NUM_SHADER_TYPE];
		FixString< 256 > path;
		path.format("%s%s", fileName, SHADER_FILE_SUBNAME);
		for( int i = 0; entries[i].type != Shader::eEmpty; ++i )
			filePaths[i] = path;

		return loadInternal(shaderProgram, filePaths, entries, def, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[], ShaderCompileOption const& option, char const* additionalCode /*= nullptr*/)
	{
		return loadInternal(shaderProgram, fileName, entries, option, additionalCode, true);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile)
	{
		ShaderEntryInfo entries[Shader::NUM_SHADER_TYPE];
		MakeEntryInfos(entries, shaderMask, entryNames);
		return loadInternal(shaderProgram, fileName, entries, option, additionalCode, bSingleFile);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[] , ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile)
	{
		removeFromShaderCompileMap(shaderProgram);

		ShaderProgramCompileInfo* info = new ShaderProgramCompileInfo;

		info->shaderProgram = &shaderProgram;
		info->bShowComplieInfo = option.bShowComplieInfo;
		generateCompileSetup(*info, entries, option, additionalCode , fileName , bSingleFile );

		if( !updateShaderInternal(shaderProgram, *info) )
		{
			delete info;
			return false;
		}

		mShaderCompileMap.insert({ &shaderProgram , info });
		return true;
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def , char const* additionalCode, bool bSingleFile)
	{
		ShaderEntryInfo entries[Shader::NUM_SHADER_TYPE];
		MakeEntryInfos(entries, shaderMask, entryNames);
		char const* filePaths[Shader::NUM_SHADER_TYPE];
		FixString< 256 > paths[Shader::NUM_SHADER_TYPE];
		
		for( int i = 0; entries[i].type != Shader::eEmpty; ++i )
		{
			if( bSingleFile )
			{
				paths[i].format("%s%s", filePaths[i], SHADER_FILE_SUBNAME);
			}
			else
			{
				paths[i].format("%s%s", fileName, ShaderPosfixNames[entries[i].type]);
			}

			filePaths[i] = paths[i];
		}
		return loadInternal(shaderProgram, filePaths, entries, def, additionalCode);
	}

	static void AddCommonHeadCode(std::string& inoutHeadCode , uint32 version , ShaderEntryInfo const& entry )
	{
		inoutHeadCode += "#version ";
		FixString<128> str;
		inoutHeadCode += str.format("%u", version);
		inoutHeadCode += " compatibility\n";

		inoutHeadCode += "#define COMPILER_GLSL 1\n";
		inoutHeadCode += "#define SHADER_COMPILING 1\n";

		inoutHeadCode += gShaderDefines[entry.type];
		if( entry.name )
		{
			inoutHeadCode += "#define ";
			inoutHeadCode += entry.name;
			inoutHeadCode += " main\n";
		}
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* filePaths[], ShaderEntryInfo const entries[], char const* def, char const* additionalCode)
	{
		removeFromShaderCompileMap(shaderProgram);

		ShaderProgramCompileInfo* info = new ShaderProgramCompileInfo;

		info->shaderProgram = &shaderProgram;

		for( int i = 0; entries[i].type != Shader::eEmpty; ++i )
		{
			auto const& entry = entries[i];

			std::string headCode;
			AddCommonHeadCode(headCode, mDefaultVersion, entry);

			if( def )
			{
				headCode += def;
				headCode += '\n';
			}

			headCode += "#include \"Common" SHADER_FILE_SUBNAME "\"\n";

			if( additionalCode )
			{
				headCode += additionalCode;
				headCode += '\n';
			}

			info->shaders.push_back({ entry.type, filePaths[i] , std::move(headCode) });
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

	bool ShaderManager::loadSimple(ShaderProgram& shaderProgram, char const* fileNameVS, char const* fileNamePS, char const* def, char const* additionalCode)
	{
		ShaderEntryInfo entries[3] =
		{
			{ Shader::eVertex , "main" },
			{ Shader::ePixel , "main" },
			{ Shader::eEmpty , nullptr },
		};
		FixString< 256 > paths[2];
		paths[0].format("%s%s%s", mBaseDir.c_str(), fileNameVS, SHADER_FILE_SUBNAME);
		paths[1].format("%s%s%s", mBaseDir.c_str(), fileNamePS, SHADER_FILE_SUBNAME);
		char const* filePaths[2] = { paths[0] , paths[1] };

		return loadInternal(shaderProgram, filePaths, entries, def, additionalCode);
	}

	bool ShaderManager::reloadShader(ShaderProgram& shaderProgram)
	{
		auto iter = mShaderCompileMap.find(&shaderProgram);
		if( iter == mShaderCompileMap.end() )
			return false;

		ShaderProgramCompileInfo* info = iter->second;
		if( info->classType == ShaderClassType::Global )
		{
			ShaderCompileOption option;
			option.version = mDefaultVersion;

			GlobalShaderProgramClass& myClass = *static_cast<GlobalShaderProgram&>(shaderProgram).myClass;
			(*myClass.funSetupShaderCompileOption)(option);

			info->shaders.clear();
			generateCompileSetup(*info, (*myClass.funGetShaderEntries)(), option, nullptr,  (*myClass.funGetShaderFileName)(), true);
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
			RHIShaderRef shader = shaderProgram.mShaders[shaderInfo.type];
			if( shader == nullptr )
			{
				shader = new RHIShader;
				if( !mCompiler.compileCode(shaderInfo.type, *shader, shaderInfo.filePath.c_str(), shaderInfo.headCode.c_str()) )
					return false;
				shaderProgram.attachShader(*shader);
			}
			else
			{
				if( !mCompiler.compileCode(shaderInfo.type, *shader, shaderInfo.filePath.c_str(), shaderInfo.headCode.c_str()) )
					return false;
			}

			if( info.bShowComplieInfo )
			{

			}
			++indexCode;
		}

		if( !shaderProgram.updateShader(true) )
		{

		}
		return true;
	}

	void ShaderManager::generateCompileSetup(ShaderProgramCompileInfo& compileInfo, ShaderEntryInfo const entries[], ShaderCompileOption const& option, char const* additionalCode , char const* fileName , bool bSingleFile )
	{
		assert( fileName &&  compileInfo.shaders.empty());

		for( int i = 0; entries[i].type != Shader::eEmpty; ++i )
		{
			auto const& entry = entries[i];
			std::string defCode;

			AddCommonHeadCode(defCode, option.version ? option.version : mDefaultVersion, entry);

			std::string headCode = option.getCode(defCode.c_str(), additionalCode);

			FixString< 256 > path;
			if( bSingleFile )
			{
				path.format("%s%s%s", mBaseDir.c_str() , fileName, SHADER_FILE_SUBNAME);
			}
			else
			{
				path.format("%s%s%s", mBaseDir.c_str(), fileName, ShaderPosfixNames[entry.type]);
			}
			compileInfo.shaders.push_back({ entry.type , path.c_str() , std::move(headCode) });
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
				char const* dirPathEnd = FileUtility::GetFileName(path);
				if( dirPathEnd != path )
				{
					--dirPathEnd;
				}
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
				glGetShaderiv( OpenGLCast::GetHandle( shader ), GL_INFO_LOG_LENGTH, &maxLength);
				std::vector< char > buf(maxLength);
				int logLength = 0;
				glGetShaderInfoLog( OpenGLCast::GetHandle(shader ), maxLength, &logLength, &buf[0]);
				::MessageBoxA(NULL, &buf[0], "Shader Compile Error", 0);
			}

		} 
		while( !bSuccess && bRecompile );

		return bSuccess;
	}

	void ShaderManager::ShaderProgramCompileInfo::getDependentFilePaths(std::vector<std::wstring>& paths)
	{
		for( ShaderCompileInfo const& shaderInfo  : shaders )
		{
			AddUnique(paths, FCString::CharToWChar(shaderInfo.filePath.c_str()));
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

		result += "#include \"Common" SHADER_FILE_SUBNAME "\"\n" ;

		if( addionalCode )
		{
			result += addionalCode;
			result += '\n';
		}

		for( auto const& code : mCodes )
		{
			result += code;
			result += '\n';
		}

		for( auto& name : mIncludeFiles )
		{
			result += "#include \"";
			result += name;
			result += SHADER_FILE_SUBNAME;
			result += "\"\n";
		}

		return result;
	}


}//namespace GL


