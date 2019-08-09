#include "ShaderManager.h"

#include "RHICommand.h"
#include "MaterialShader.h"
#include "GlobalShader.h"
#include "VertexFactory.h"

#include "FileSystem.h"
#include "BitUtility.h"
#include "StdUtility.h"
#include "DataCacheInterface.h"
#include "ConsoleSystem.h"

#include <iterator>

extern CORE_API TConsoleVariable< bool > gbUseShaderCacheCom;
#if CORE_SHARE_CODE
TConsoleVariable< bool > gbUseShaderCacheCom( true , "bUseShaderCache" );
#endif

namespace Render
{
	struct ShaderCacheBinaryData
	{
	public:
		bool initialize( ShaderFormat& format , ShaderProgramCompileInfo const& info)
		{
			if( !format.getBinaryCode( *info.shaderProgram->mRHIResource , binaryCode) )
				return false;

			std::set< HashString > assetFilePaths;
			for( auto const& shaderInfo : info.shaders )
			{
				assetFilePaths.insert(shaderInfo.filePath);
				assetFilePaths.insert(shaderInfo.includeFiles.begin() , shaderInfo.includeFiles.end() );
			}
			for( auto path : assetFilePaths )
			{
				FileAttributes fileAttributes;
				if( !FileSystem::GetFileAttributes(path.c_str(), fileAttributes) )
					return false;
				ShaderFile file;
				file.lastModifyTime = fileAttributes.lastWrite;
				file.path = path.c_str();
				assetDependences.push_back(std::move(file));
			}
			
			return true;
		}


		bool checkAssetNoModified()
		{
			for( auto const& asset : assetDependences )
			{
				FileAttributes fileAttributes;
				if( !FileSystem::GetFileAttributes(asset.path.c_str(), fileAttributes) )
					return false;
				if( fileAttributes.lastWrite > asset.lastModifyTime )
					return false;
			}
			return true;
		}

		bool setupProgram(ShaderFormat& format, ShaderProgram& program)
		{
			if( binaryCode.empty() )
				return false;

			if( !program.mRHIResource.isValid() )
			{
				program.mRHIResource = RHICreateShaderProgram();
				if( !program.mRHIResource.isValid() )
					return false;
			}

			if( !format.setupProgram(*program.getRHIResource(), binaryCode) )
				return false;

			format.setupParameters(program);
			return true;
		}


		struct ShaderFile
		{
			std::string path;
			DateTime    lastModifyTime;

			template< class Op >
			void serialize(Op op)
			{
				op & path & lastModifyTime;
			}
		};

		std::vector< ShaderFile > assetDependences;
		std::vector< uint8 > binaryCode;

		template< class Op >
		void serialize(Op op)
		{
			op & assetDependences & binaryCode;
		}
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(ShaderCacheBinaryData);
	TYPE_SUPPORT_SERIALIZE_FUNC(ShaderCacheBinaryData::ShaderFile);

	class ShaderCache : public IAssetViewer
	{
	public:
		ShaderCache()
		{

			


		}

		static void GetShaderCacheKey(ShaderFormat& format, ShaderProgramCompileInfo const& info, DataCacheKey& outKey)
		{
			switch( info.classType )
			{
			case ShaderClassType::Common:   outKey.typeName = "CSHADER"; break;
			case ShaderClassType::Global:   outKey.typeName = "GSHADER"; break; 
			case ShaderClassType::Material: outKey.typeName = "MSHADER"; break;
			default:
				break;
			}
			outKey.version = "4DAA6D50-0D2A-4955-BDE2-676C6D7353AE";
			if( info.classType == ShaderClassType::Material )
			{



			}
			for( auto const& shaderInfo : info.shaders )
			{
				outKey.keySuffix.add((uint8)shaderInfo.type, format.getName() , shaderInfo.filePath.c_str(), shaderInfo.headCode.c_str());
			}

		}
		bool saveCacheData( ShaderFormat& format, ShaderProgramCompileInfo const& info)
		{
			if( !format.isSupportBinaryCode() )
				return false;
			ShaderCacheBinaryData binaryData;
			DataCacheKey key;
			GetShaderCacheKey(format, info, key);
			bool result = mDataCache->saveDelegate(key, [&info,&binaryData,&format](IStreamSerializer& serializer)
			{			
				if( !binaryData.initialize(format, info) )
				{
					return false;
				}
				serializer << binaryData;
				return true;
			});
#if 0
			ShaderCacheBinaryData temp;
			if ( result )
			{
				
				result = mDataCache->loadDelegate(key, [&info,&temp](IStreamSerializer& serializer)
				{
					serializer >> temp;
					return true;
				});
			}
#endif
			return result;
		}


		bool loadCacheData(ShaderFormat& format, ShaderProgramCompileInfo& info)
		{
			if( !format.isSupportBinaryCode() )
				return false;

			if( !gbUseShaderCacheCom.getValue() )
				return false;

			DataCacheKey key;
			GetShaderCacheKey(format, info, key);

			bool result = mDataCache->loadDelegate(key, [&info,&format](IStreamSerializer& serializer)
			{
				ShaderCacheBinaryData binaryData;
				serializer >> binaryData;

				if( !binaryData.checkAssetNoModified() )
					return false;

				if( !binaryData.setupProgram(format, *info.shaderProgram) )
					return false;

				for( auto const& asset : binaryData.assetDependences )
				{
					info.shaders.front().includeFiles.push_back(asset.path.c_str());
				}
				return true;
			});

			return result;
		}

		DataCacheInterface* mDataCache;
	};



	const char const* ShaderPosfixNames[] =
	{
		"VS" SHADER_FILE_SUBNAME ,
		"PS" SHADER_FILE_SUBNAME ,
		"GS" SHADER_FILE_SUBNAME ,
		"CS" SHADER_FILE_SUBNAME ,
		"HS" SHADER_FILE_SUBNAME ,
		"DS" SHADER_FILE_SUBNAME ,
	};

	const char const* gShaderDefines[] =
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

	GlobalShaderProgram* ShaderManager::getGlobalShader(GlobalShaderProgramClass const& shaderClass, bool bForceLoad)
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

#endif //CORE_SHARE_CODE



	ShaderManager::ShaderManager()
	{

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
		if( mShaderFormat )
		{
			delete mShaderFormat;
			mShaderFormat = nullptr;
		}
		//cleanupGlobalShader();
	}


	void ShaderManager::setDataCache(DataCacheInterface* dataCache)
	{
		getCache()->mDataCache = dataCache;
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
#if 0
		std::vector< std::string > classNames;
		for( auto pShaderClass : MaterialShaderProgramClass::ClassList )
		{
			classNames.push_back((pShaderClass->funGetShaderFileName)());
		}
#endif
		int result = 0;
		for( auto pShaderClass : MaterialShaderProgramClass::ClassList )
		{
			ShaderCompileOption option;
			mShaderFormat->setupShaderCompileOption(option);
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
			GlobalShaderProgramClass const* shaderClass = pair.first;
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

	GlobalShaderProgram* ShaderManager::constructGlobalShader(GlobalShaderProgramClass const& shaderClass)
	{
		ShaderCompileOption option;
		mShaderFormat->setupShaderCompileOption(option);
		return constructShaderInternal(shaderClass, ShaderClassType::Global, option);
	}

	GlobalShaderProgram* ShaderManager::constructShaderInternal(GlobalShaderProgramClass const& shaderClass , ShaderClassType classType , ShaderCompileOption& option )
	{
		GlobalShaderProgram* result = (*shaderClass.funCreateShader)();
		if( result )
		{
			result->myClass = &shaderClass;
			
			(*shaderClass.funSetupShaderCompileOption)(option);

			if( !loadInternal(*result, (*shaderClass.funGetShaderFileName)(), (*shaderClass.funGetShaderEntries)(), option, nullptr, true , classType ) )
			{
				LogWarning(0, "Can't Load Shader %s", (*shaderClass.funGetShaderFileName)());

				delete result;
				result = nullptr;
			}

			if ( result )
			{
				if( classType == ShaderClassType::Material )
				{
					removeFromShaderCompileMap(*result);
				}
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
		};
		return loadFile(shaderProgram , fileName, entries , def, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, char const* def, char const* additionalCode)
	{
		ShaderEntryInfo entries[] =
		{
			{ Shader::eVertex , vertexEntryName } ,
			{ Shader::ePixel , pixelEntryName } ,
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
		};
		return loadFile(shaderProgram, fileName, entries, option, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries, char const* def /*= nullptr*/, char const* additionalCode /*= nullptr*/)
	{
		char const* filePaths[Shader::Count];
		FixString< 256 > path;
		path.format("%s%s", fileName, SHADER_FILE_SUBNAME);
		for( int i = 0; i < entries.size(); ++i )
			filePaths[i] = path;

		return loadInternal(shaderProgram, filePaths, entries, def, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, char const* additionalCode /*= nullptr*/)
	{
		return loadInternal(shaderProgram, fileName, entries, option, additionalCode, true);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile, ShaderClassType classType)
	{
		ShaderEntryInfo entries[Shader::Count];
		return loadInternal(shaderProgram, fileName, MakeEntryInfos(entries, shaderMask, entryNames) , option, additionalCode, bSingleFile);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries , ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile , ShaderClassType classType )
	{
		removeFromShaderCompileMap(shaderProgram);

		ShaderProgramCompileInfo* info = new ShaderProgramCompileInfo;
		info->classType = classType;
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

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def , char const* additionalCode, bool bSingleFile, ShaderClassType classType)
	{

		ShaderEntryInfo entries[Shader::Count];
		auto entriesView = MakeEntryInfos(entries, shaderMask, entryNames);

		char const* filePaths[Shader::Count];
		FixString< 256 > paths[Shader::Count];

		for( int i = 0; i < entriesView.size() ; ++i )
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
		return loadInternal(shaderProgram, filePaths, entriesView , def, additionalCode, classType);
	}

	static void AddCommonHeadCode(std::string& inoutHeadCode , uint32 version , ShaderEntryInfo const& entry )
	{
		inoutHeadCode += "#version ";
		FixString<128> str;
		inoutHeadCode += str.format("%u", version);
		inoutHeadCode += " compatibility\n";

		inoutHeadCode += "#define COMPILER_GLSL 1\n";
		inoutHeadCode += "#define SHADER_COMPILING 1\n";

		
		if( entry.name )
		{
			inoutHeadCode += "#define ";
			inoutHeadCode += entry.name;
			inoutHeadCode += " main\n";
		}
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* filePaths[], TArrayView< ShaderEntryInfo const > entries, char const* def, char const* additionalCode, ShaderClassType classType)
	{
		removeFromShaderCompileMap(shaderProgram);

		ShaderProgramCompileInfo* info = new ShaderProgramCompileInfo;
		info->classType = classType;
		info->shaderProgram = &shaderProgram;

		for( int i = 0; i < entries.size(); ++i )
		{
			auto const& entry = entries[i];

			ShaderCompileOption option;
			mShaderFormat->setupShaderCompileOption(option);
			std::string headCode;
			mShaderFormat->getHeadCode(headCode, option, entry);

			if( def )
			{
				headCode += def;
			}
			headCode = option.getCode(entry, headCode.c_str(), additionalCode);

			info->shaders.push_back({ entry.type, filePaths[i]  , std::move(headCode) , entry.name });
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
		ShaderEntryInfo entries[2] =
		{
			{ Shader::eVertex , "main" },
			{ Shader::ePixel , "main" },
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
			mShaderFormat->setupShaderCompileOption(option);

			GlobalShaderProgramClass const& myClass = *static_cast<GlobalShaderProgram&>(shaderProgram).myClass;
			(*myClass.funSetupShaderCompileOption)(option);

			info->shaders.clear();
			generateCompileSetup(*info, (*myClass.funGetShaderEntries)(), option, nullptr,  (*myClass.funGetShaderFileName)(), true);
		}

		return updateShaderInternal(shaderProgram, *info , true);
	}


	void ShaderManager::reloadAll()
	{
		for( auto pair : mShaderCompileMap )
		{
			updateShaderInternal(*pair.first, *pair.second, true);
		}
	}

	bool ShaderManager::updateShaderInternal(ShaderProgram& shaderProgram, ShaderProgramCompileInfo& info , bool bForceReload )
	{
		if( !bForceReload && getCache()->loadCacheData(*mShaderFormat, info) )
		{
			LogMsg("%s Use Cache Data", info.shaders[0].filePath.c_str());
			return true;
		}
		if( !shaderProgram.mRHIResource.isValid() )
		{
			shaderProgram.mRHIResource = RHICreateShaderProgram();
			if( !shaderProgram.mRHIResource.isValid() )
				return false;
		}
		
		RHIShaderRef shaders[Shader::Count];
		int numShader = 0;
		bool bFailed = false;
		for( ShaderCompileInfo& shaderInfo : info.shaders )
		{
			shaders[numShader] = RHICreateShader( shaderInfo.type );
			if( !mShaderFormat->compileCode(shaderInfo.type, *shaders[numShader] , shaderInfo.filePath.c_str(), &shaderInfo, shaderInfo.headCode.c_str()) )
			{
				bFailed = true;
				break;
			}

			if( info.bShowComplieInfo )
			{

			}
			++numShader;
		}

		if( bFailed )
		{
			return false;
		}

		if( !shaderProgram.mRHIResource->setupShaders(shaders, numShader) )
		{
			return false;
		}

		mShaderFormat->setupParameters(shaderProgram);
		getCache()->saveCacheData(*mShaderFormat, info);
		return true;
	}

	void ShaderManager::generateCompileSetup(ShaderProgramCompileInfo& compileInfo, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, char const* additionalCode , char const* fileName , bool bSingleFile )
	{
		assert( fileName &&  compileInfo.shaders.empty());

		for( auto const& entry : entries )
		{
			std::string defCode;
			mShaderFormat->getHeadCode(defCode, option, entry);

			std::string headCode = option.getCode( entry , defCode.c_str(), additionalCode);

			FixString< 256 > path;
			if( bSingleFile )
			{
				path.format("%s%s%s", mBaseDir.c_str() , fileName, SHADER_FILE_SUBNAME);
			}
			else
			{
				path.format("%s%s%s", mBaseDir.c_str(), fileName, ShaderPosfixNames[entry.type]);
			}
			compileInfo.shaders.push_back({ entry.type , path.c_str() , std::move(headCode) , entry.name });
		}
	}

	ShaderCache* ShaderManager::getCache()
	{
		if( mShaderCache == nullptr )
		{
			mShaderCache = new ShaderCache;
		}
		return mShaderCache;
	}


	void ShaderProgramCompileInfo::getDependentFilePaths(std::vector<std::wstring>& paths)
	{
		std::set< HashString > filePathSet;
		for( ShaderCompileInfo const& shaderInfo : shaders )
		{
			filePathSet.insert(shaderInfo.filePath);
			filePathSet.insert(shaderInfo.includeFiles.begin(), shaderInfo.includeFiles.end());
		}
		
		for( auto const& filePath : filePathSet )
		{
			paths.push_back( FCString::CharToWChar(filePath) );
		}
	}

	void ShaderProgramCompileInfo::postFileModify(FileAction action)
	{
		if ( action == FileAction::Modify )
			ShaderManager::Get().updateShaderInternal(*shaderProgram, *this, true);
	}

	std::string ShaderCompileOption::getCode( ShaderEntryInfo const& entry , char const* defCode /*= nullptr */, char const* addionalCode /*= nullptr */) const
	{
		std::string result;
		if( defCode )
		{
			result += defCode;
		}

		result += "#define SHADER_COMPILING 1\n";
		result += gShaderDefines[entry.type];

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

		result += "#include \"Common" SHADER_FILE_SUBNAME "\"\n";

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


}//namespace Render


