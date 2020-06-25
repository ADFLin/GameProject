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
#include "ProfileSystem.h"

extern CORE_API TConsoleVariable< bool > CVarShaderUseCache;
extern CORE_API TConsoleVariable< bool > CVarShaderDectectFileMoidfy;

#if CORE_SHARE_CODE
TConsoleVariable< bool > CVarShaderUseCache( true , "Shader.bUseCache" , CVF_CONFIG );
TConsoleVariable< bool > CVarShaderDectectFileMoidfy(true, "Shader.DetectFileModiy", CVF_CONFIG);
#endif

namespace Render
{
	struct ShaderCacheBinaryData
	{
	public:
		bool addFileDependences( ShaderProgramManagedData const& managedData )
		{
			std::set< HashString > assetFilePaths;
			for( auto const& compileInfo : managedData.compileInfos )
			{
				assetFilePaths.insert(compileInfo.filePath);
				assetFilePaths.insert(compileInfo.includeFiles.begin() , compileInfo.includeFiles.end() );
			}

			return addFileDependencesInternal(assetFilePaths);
		}
		bool addFileDependences(ShaderManagedData const& managedData)
		{
			std::set< HashString > assetFilePaths;
			auto const& compileInfo = managedData.compileInfo;
			assetFilePaths.insert(compileInfo.filePath);
			assetFilePaths.insert(compileInfo.includeFiles.begin(), compileInfo.includeFiles.end());

			return addFileDependencesInternal(assetFilePaths);
		}

		bool addFileDependencesInternal(std::set< HashString > const& assetFilePaths)
		{
			for (auto const& path : assetFilePaths)
			{
				FileAttributes fileAttributes;
				if (!FileSystem::GetFileAttributes(path.c_str(), fileAttributes))
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

		bool setupProgram(ShaderFormat& format, ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles )
		{
			if( codeBuffer.empty() )
				return false;

			if( !shaderProgram.mRHIResource.isValid() )
			{
				shaderProgram.mRHIResource = RHICreateShaderProgram();
				if( !shaderProgram.mRHIResource.isValid() )
					return false;
			}

			if( !format.initializeProgram(shaderProgram, shaderCompiles , codeBuffer) )
				return false;


			return true;
		}

		bool setupShader(ShaderFormat& format, Shader& shader, ShaderCompileInfo const& shaderCompile)
		{
			if (codeBuffer.empty())
				return false;

			if (!shader.mRHIResource.isValid())
			{
				shader.mRHIResource = RHICreateShader(shaderCompile.type);
				if (!shader.mRHIResource.isValid())
					return false;
			}

			if (!format.initializeShader(shader, shaderCompile, codeBuffer))
				return false;


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
		std::vector< uint8 > codeBuffer;

		template< class Op >
		void serialize(Op op)
		{
			op & assetDependences & codeBuffer;
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

		static void GetShaderCacheKey(ShaderFormat& format, ShaderProgramManagedData const& managedData, DataCacheKey& outKey)
		{
			switch( managedData.classType )
			{
			case ShaderClassType::Common:   outKey.typeName = "CSHADER"; break;
			case ShaderClassType::Global:   outKey.typeName = "GSHADER"; break; 
			case ShaderClassType::Material: outKey.typeName = "MSHADER"; break;
			default:
				break;
			}
			outKey.version = "4DAA6D50-0D2A-4955-BDE2-676C6D7353AE";
			if( managedData.classType == ShaderClassType::Material )
			{


			}
			for( auto const& compileInfo : managedData.compileInfos )
			{
				outKey.keySuffix.add((uint8)compileInfo.type, format.getName() , compileInfo.filePath.c_str(), compileInfo.headCode.c_str());
			}
		}

		static void GetShaderCacheKey(ShaderFormat& format, ShaderManagedData const& managedData, DataCacheKey& outKey)
		{
			switch (managedData.classType)
			{
			case ShaderClassType::Common:   outKey.typeName = "CSHADER"; break;
			case ShaderClassType::Global:   outKey.typeName = "GSHADER"; break;
			case ShaderClassType::Material: outKey.typeName = "MSHADER"; break;
			default:
				break;
			}
			outKey.version = "4DAA6D50-0D2A-4955-BDE2-676C6D7353AE";
			if (managedData.classType == ShaderClassType::Material)
			{



			}

			auto const& compileInfo = managedData.compileInfo;
			outKey.keySuffix.add((uint8)compileInfo.type, format.getName(), compileInfo.filePath.c_str(), compileInfo.headCode.c_str());
		
		}

		bool saveCacheData( ShaderFormat& format, ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
		{
			if( !format.isSupportBinaryCode() )
				return false;
			ShaderCacheBinaryData binaryData;
			DataCacheKey key;
			GetShaderCacheKey(format, *setupData.managedData, key);
			bool result = mDataCache->saveDelegate(key, [&shaderProgram,&setupData,&binaryData,&format](IStreamSerializer& serializer)
			{			
				if (!format.getBinaryCode(shaderProgram, setupData, binaryData.codeBuffer))
					return false;

				if (!binaryData.addFileDependences(*setupData.managedData))
					return false;

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

		bool saveCacheData(ShaderFormat& format, Shader& shader, ShaderSetupData& setupData)
		{
			if (!format.isSupportBinaryCode())
				return false;
			ShaderCacheBinaryData binaryData;
			DataCacheKey key;
			GetShaderCacheKey(format, *setupData.managedData, key);
			bool result = mDataCache->saveDelegate(key, [&shader, &setupData, &binaryData, &format](IStreamSerializer& serializer)
			{
				if (!format.getBinaryCode(shader, setupData, binaryData.codeBuffer))
					return false;

				if (!binaryData.addFileDependences(*setupData.managedData))
					return false;

				serializer << binaryData;
				return true;
			});
#if 0
			ShaderCacheBinaryData temp;
			if (result)
			{
				result = mDataCache->loadDelegate(key, [&info, &temp](IStreamSerializer& serializer)
				{
					serializer >> temp;
					return true;
				});
			}
#endif
			return result;
		}

		bool loadCacheData(ShaderFormat& format, ShaderProgramManagedData& managedData)
		{
			if (!CVarShaderUseCache.getValue())
				return false;

			if( !format.isSupportBinaryCode() )
				return false;

			DataCacheKey key;
			GetShaderCacheKey(format, managedData, key);

			bool result = mDataCache->loadDelegate(key, [&managedData,&format](IStreamSerializer& serializer)
			{
				ShaderCacheBinaryData binaryData;
				serializer >> binaryData;

				if( !binaryData.checkAssetNoModified() )
					return false;

				if( !binaryData.setupProgram(format, *managedData.shaderProgram, managedData.compileInfos ) )
					return false;

				for( auto const& asset : binaryData.assetDependences )
				{
					managedData.compileInfos.front().includeFiles.push_back(asset.path.c_str());
				}
				return true;
			});

			return result;
		}


		bool loadCacheData(ShaderFormat& format, ShaderManagedData& managedData)
		{
			if (!CVarShaderUseCache.getValue())
				return false;

			if (!format.isSupportBinaryCode())
				return false;

			DataCacheKey key;
			GetShaderCacheKey(format, managedData, key);

			bool result = mDataCache->loadDelegate(key, [&managedData, &format](IStreamSerializer& serializer)
			{
				ShaderCacheBinaryData binaryData;
				serializer >> binaryData;

				if (!binaryData.checkAssetNoModified())
					return false;

				if (!binaryData.setupShader(format, *managedData.shader, managedData.compileInfo))
					return false;

				for (auto const& asset : binaryData.assetDependences)
				{
					managedData.compileInfo.includeFiles.push_back(asset.path.c_str());
				}
				return true;
			});

			return result;
		}

		DataCacheInterface* mDataCache;
	};



	char const* const ShaderPosfixNames[] =
	{
		"VS" SHADER_FILE_SUBNAME ,
		"PS" SHADER_FILE_SUBNAME ,
		"GS" SHADER_FILE_SUBNAME ,
		"CS" SHADER_FILE_SUBNAME ,
		"HS" SHADER_FILE_SUBNAME ,
		"DS" SHADER_FILE_SUBNAME ,
	};

	 char const* const gShaderDefines[] =
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

	ShaderObject* ShaderManager::getGlobalShader(GlobalShaderObjectClass const& shaderObjectClass, bool bForceLoad)
	{
		ShaderObject* result = mGlobalShaderMap[&shaderObjectClass];
		if( result == nullptr && bForceLoad )
		{
			result = constructGlobalShader(shaderObjectClass);
			if( result )
			{
				mGlobalShaderMap[&shaderObjectClass] = result;
			}
		}
		return result;
	}

#endif //CORE_SHARE_CODE


	bool ShaderManager::initialize(ShaderFormat& shaderFormat)
	{
		mShaderFormat = &shaderFormat;
		return true;
	}

	ShaderManager::ShaderManager()
	{

	}

	ShaderManager::~ShaderManager()
	{
		for( auto& pair : mShaderDataMap )
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

	void ShaderManager::setAssetViewerRegister(IAssetViewerRegister* reigster)
	{
		if (reigster == nullptr)
		{
			if (mAssetViewerReigster)
			{
				unregisterShaderAssets();
			}
		}
		mAssetViewerReigster = reigster;
		if (mAssetViewerReigster)
		{
			registerShaderAssets();
		}
	}

	//TODO: Remove
	std::string GetFilePath(char const* name)
	{
		std::string path("Material/");
		path += name;
		return path;
	}

	int ShaderManager::loadMaterialShaders(
		MaterialShaderCompileInfo const& info, 
		VertexFactoryType& vertexFactoryType,
		MaterialShaderPairVec& outShaders )
	{
		LogDevMsg( 0 , "VertexFactory Type : %s" ,  vertexFactoryType.fileName );
		std::string path = GetFilePath(info.name);
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
			option.addInclude(path.c_str());
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

			option.addMeta("SourceFile", path.c_str());
			MaterialShaderProgram* shaderProgram = (MaterialShaderProgram*)constructShaderInternal(*pShaderClass, ShaderClassType::Material, option );

			if( shaderProgram )
			{
				outShaders.emplace_back(pShaderClass , shaderProgram);
				++result;
			}
		}

		return result;
	}



	bool ShaderManager::registerGlobalShader(GlobalShaderObjectClass& shaderClass)
	{
		mGlobalShaderMap.insert({ &shaderClass , nullptr });
		return true;
	}

	int ShaderManager::loadAllGlobalShaders()
	{
		int numFailLoad = 0;
		for (auto& pair : mGlobalShaderMap)
		{
			ShaderObject* shaderObject = pair.second;
			if (shaderObject == nullptr)
			{
				shaderObject = constructGlobalShader(*pair.first);
				mGlobalShaderMap[pair.first] = shaderObject;
			}

			if( shaderObject == nullptr )
			{
				++numFailLoad;
			}
		}
		return numFailLoad;
	}

	ShaderObject* ShaderManager::constructGlobalShader(GlobalShaderObjectClass const& shaderObjectClass)
	{
		ShaderCompileOption option;
		mShaderFormat->setupShaderCompileOption(option);
		return constructShaderInternal(shaderObjectClass, ShaderClassType::Global, option );
	}

	ShaderObject* ShaderManager::constructShaderInternal(GlobalShaderObjectClass const& shaderObjectClass , ShaderClassType classType , ShaderCompileOption& option )
	{
		ShaderObject* result = (*shaderObjectClass.CreateShaderObject)();
		if( result )
		{
			switch (result->getType())
			{
			case EShaderObjectType::Program:
				{
					GlobalShaderProgram* shaderProgram = static_cast<GlobalShaderProgram*>(result);
					GlobalShaderProgramClass const& shaderProgramClass = static_cast<GlobalShaderProgramClass const&>(shaderObjectClass);
					shaderProgram->myClass = &shaderProgramClass;

					shaderProgramClass.SetupShaderCompileOption(option);

					if (!loadInternal(*shaderProgram, 
						shaderProgramClass.GetShaderFileName(),
						shaderProgramClass.GetShaderEntries(),
						option, nullptr, true, classType))
					{
						LogWarning(0, "Can't Load Shader %s", shaderProgramClass.GetShaderFileName());
						delete result;
						result = nullptr;
					}
				}
				break;
			case EShaderObjectType::Shader:
				{
					GlobalShader* shader = static_cast<GlobalShader*>(result);
					GlobalShaderClass const& shaderClass = static_cast<GlobalShaderClass const&>(shaderObjectClass);
					shader->myClass = &shaderClass;

					shaderClass.SetupShaderCompileOption(option);

					if (!loadInternal(*shader,
						shaderClass.GetShaderFileName(),
						shaderClass.entry,
						option, nullptr, true, classType))
					{
						LogWarning(0, "Can't Load Shader %s", shaderClass.GetShaderFileName());
						delete result;
						result = nullptr;
					}

				}
				break;

			default:
				break;
			}
		}
		else
		{

			
		}

		if (result)
		{
			if (classType == ShaderClassType::Material)
			{
				removeFromShaderCompileMap(*result);
			}
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
			{ EShader::Vertex , SHADER_ENTRY(MainVS) } ,
			{ EShader::Pixel , SHADER_ENTRY(MainPS) } ,
		};
		return loadFile(shaderProgram , fileName, entries , def, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, char const* def, char const* additionalCode)
	{
		ShaderEntryInfo entries[] =
		{
			{ EShader::Vertex , vertexEntryName } ,
			{ EShader::Pixel , pixelEntryName } ,
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
			{ EShader::Vertex , vertexEntryName } ,
			{ EShader::Pixel , pixelEntryName } ,
		};
		return loadFile(shaderProgram, fileName, entries, option, additionalCode);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries, char const* def /*= nullptr*/, char const* additionalCode /*= nullptr*/)
	{
		char const* filePaths[EShader::Count];
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
		ShaderEntryInfo entries[EShader::Count];
		return loadInternal(shaderProgram, fileName, MakeEntryInfos(entries, shaderMask, entryNames) , option, additionalCode, bSingleFile);
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries , ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile , ShaderClassType classType )
	{
		removeFromShaderCompileMap(shaderProgram);

		ShaderProgramManagedData* managedData = new ShaderProgramManagedData;
		managedData->classType = classType;
		managedData->shaderProgram = &shaderProgram;
		managedData->bShowComplieInfo = option.bShowComplieInfo;
		char const* sourceFile = option.getMeta("SourceFile");
		if (sourceFile)
			managedData->sourceFile = sourceFile;
		generateCompileSetup(*managedData, entries, option, additionalCode , fileName , bSingleFile );

		if( !updateShaderInternal(shaderProgram, *managedData) )
		{
			delete managedData;
			return false;
		}

		postShaderLoaded(shaderProgram, *managedData);
		return true;
	}

	bool ShaderManager::loadInternal(Shader& shader, char const* fileName, ShaderEntryInfo const& entry, ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile, ShaderClassType classType)
	{
		removeFromShaderCompileMap(shader);

		ShaderManagedData* managedData = new ShaderManagedData;
		managedData->classType = classType;
		managedData->shader = &shader;
		managedData->bShowComplieInfo = option.bShowComplieInfo;
		char const* sourceFile = option.getMeta("SourceFile");
		if (sourceFile)
			managedData->sourceFile = sourceFile;

		generateCompileSetup(*managedData, entry, option, additionalCode, fileName, bSingleFile);

		if (!updateShaderInternal(shader, *managedData))
		{
			delete managedData;
			return false;
		}

		postShaderLoaded(shader, *managedData);
		return true;
	}

	bool ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def , char const* additionalCode, bool bSingleFile, ShaderClassType classType)
	{

		ShaderEntryInfo entries[EShader::Count];
		auto entriesView = MakeEntryInfos(entries, shaderMask, entryNames);

		char const* filePaths[EShader::Count];
		FixString< 256 > paths[EShader::Count];

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

		ShaderProgramManagedData* info = new ShaderProgramManagedData;
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

			info->compileInfos.emplace_back(entry.type, filePaths[i]  , std::move(headCode) , entry.name);
		}

		if( !updateShaderInternal(shaderProgram, *info) )
		{
			delete info;
			return false;
		}

		postShaderLoaded(shaderProgram, *info);
		return true;
	}

	bool ShaderManager::loadMultiFile(ShaderProgram& shaderProgram, char const* fileName, char const* def, char const* additionalCode)
	{
		return loadInternal(shaderProgram, fileName, BIT(EShader::Vertex) | BIT(EShader::Pixel), nullptr, def, additionalCode, false);
	}

	bool ShaderManager::loadSimple(ShaderProgram& shaderProgram, char const* fileNameVS, char const* fileNamePS, char const* entryVS , char const* entryPS , char const* def, char const* additionalCode)
	{
		ShaderEntryInfo entries[2] =
		{
			{ EShader::Vertex , entryVS ? entryVS : "main" },
			{ EShader::Pixel ,  entryPS ? entryPS : "main" },
		};
		FixString< 256 > paths[2];
		paths[0].format("%s%s%s", mBaseDir.c_str(), fileNameVS, SHADER_FILE_SUBNAME);
		paths[1].format("%s%s%s", mBaseDir.c_str(), fileNamePS, SHADER_FILE_SUBNAME);
		char const* filePaths[2] = { paths[0] , paths[1] };

		return loadInternal(shaderProgram, filePaths, entries, def, additionalCode);
	}

	bool ShaderManager::reloadShader(ShaderProgram& shaderProgram)
	{
		auto iter = mShaderDataMap.find(&shaderProgram);
		if( iter == mShaderDataMap.end() )
			return false;

		ShaderProgramManagedData* managedData = static_cast<ShaderProgramManagedData*>( iter->second );
		if(managedData->classType == ShaderClassType::Global )
		{
			ShaderCompileOption option;
			mShaderFormat->setupShaderCompileOption(option);

			GlobalShaderProgramClass const& myClass = *static_cast<GlobalShaderProgram&>(shaderProgram).myClass;
			myClass.SetupShaderCompileOption(option);

			managedData->compileInfos.clear();
			generateCompileSetup(*managedData, myClass.GetShaderEntries(), option, nullptr,  myClass.GetShaderFileName(), true);
		}

		return updateShaderInternal(shaderProgram, *managedData, true);
	}


	bool ShaderManager::reloadShader(Shader& shader)
	{
		auto iter = mShaderDataMap.find(&shader);
		if (iter == mShaderDataMap.end())
			return false;

		ShaderManagedData* info = static_cast<ShaderManagedData*>(iter->second);
		if (info->classType == ShaderClassType::Global)
		{
			ShaderCompileOption option;
			mShaderFormat->setupShaderCompileOption(option);

			GlobalShaderClass const& myClass = *static_cast<GlobalShader&>(shader).myClass;
			myClass.SetupShaderCompileOption(option);

			generateCompileSetup(*info, myClass.entry , option, nullptr, myClass.GetShaderFileName(), true);
		}

		return updateShaderInternal(shader, *info, true);

	}

	void ShaderManager::reloadAll()
	{
		for( auto pair : mShaderDataMap )
		{
			switch (pair.first->getType())
			{
			case EShaderObjectType::Shader:
				updateShaderInternal(
					*static_cast<Shader*>(pair.first),
					*static_cast<ShaderManagedData*>(pair.second),
					true);
				break;
			case EShaderObjectType::Program:
				updateShaderInternal(
					*static_cast<ShaderProgram*>(pair.first), 
					*static_cast<ShaderProgramManagedData*>( pair.second ), 
					true);
				break;
			}		
		}
	}

	void ShaderManager::registerShaderAssets()
	{
		if ( CVarShaderDectectFileMoidfy )
		{
			for (auto pair : mShaderDataMap)
			{
				mAssetViewerReigster->registerViewer(pair.second);
			}
		}
	}

	void ShaderManager::unregisterShaderAssets()
	{
		if (CVarShaderDectectFileMoidfy)
		{
			for (auto pair : mShaderDataMap)
			{
				mAssetViewerReigster->unregisterViewer(pair.second);
			}
		}
	}

	bool ShaderManager::updateShaderInternal(ShaderProgram& shaderProgram, ShaderProgramManagedData& managedData, bool bForceReload)
	{
		if ( !RHIIsInitialized() )
			return false;

		TIME_SCOPE("Update Shader");

		if( !bForceReload && getCache()->loadCacheData(*mShaderFormat, managedData) )
		{
			if (!managedData.sourceFile.empty())
			{
				LogDevMsg(0, "Use Cache Data : %s , source file : %s ", managedData.compileInfos[0].filePath.c_str(), managedData.sourceFile.c_str());
			}
			else
			{
				LogDevMsg(0, "Use Cache Data : %s ", managedData.compileInfos[0].filePath.c_str());
			}
		}
		else
		{
			if (!managedData.sourceFile.empty())
			{
				LogDevMsg(0, "Recompile shader : %s , source file : %s ", managedData.compileInfos[0].filePath.c_str(), managedData.sourceFile.c_str());
			}
			else
			{
				LogDevMsg(0, "Recompile shader : %s ", managedData.compileInfos[0].filePath.c_str());
			}

			//if (!shaderProgram.mRHIResource.isValid())
			{
				shaderProgram.mRHIResource = RHICreateShaderProgram();
				if (!shaderProgram.mRHIResource.isValid())
					return false;
			}

			ShaderProgramSetupData setupData;
			setupData.managedData = &managedData;

			mShaderFormat->precompileCode(setupData);

			ShaderResourceInfo shaders[EShader::Count];
			int numShaders = 0;
			bool bFailed = false;
			for (ShaderCompileInfo& shaderInfo : managedData.compileInfos)
			{
				ShaderCompileInput  compileInput;
				compileInput.type = shaderInfo.type;
				compileInput.entry = shaderInfo.entryName.c_str();
				compileInput.path = shaderInfo.filePath.c_str();
				compileInput.definition = shaderInfo.headCode.c_str();
				compileInput.programSetupData = &setupData;
				ShaderCompileOutput compileOutput;
				compileOutput.compileInfo = &shaderInfo;
				compileOutput.formatData = nullptr;
				if (!mShaderFormat->compileCode(compileInput, compileOutput))
				{
					bFailed = true;
					break;
				}

				if (managedData.bShowComplieInfo)
				{

				}

				ShaderResourceInfo shaderSetup;
				shaderSetup.type = shaderInfo.type;
				shaderSetup.entry = shaderInfo.entryName.c_str();
				shaderSetup.resource = compileOutput.resource;
				shaderSetup.formatData = compileOutput.formatData;
				setupData.shaderResources.push_back(shaderSetup);
				++numShaders;
			}

			if (bFailed)
			{
				return false;
			}
	
			if (!mShaderFormat->initializeProgram(shaderProgram, setupData))
			{
				return false;
			}

			if (CVarShaderUseCache && !getCache()->saveCacheData(*mShaderFormat, shaderProgram, setupData))
			{

			}
		}

		mShaderFormat->postShaderLoaded(shaderProgram);
		return true;
	}


	bool ShaderManager::updateShaderInternal(Shader& shader, ShaderManagedData& managedData, bool bForceReload)
	{
		if (!RHIIsInitialized())
			return false;


		TIME_SCOPE("Update Shader");

		if (!bForceReload && getCache()->loadCacheData(*mShaderFormat, managedData))
		{
			if (!managedData.sourceFile.empty())
			{
				LogDevMsg(0, "Use Cache Data : %s , source file : %s ", managedData.compileInfo.filePath.c_str(), managedData.sourceFile.c_str());
			}
			else
			{
				LogDevMsg(0, "Use Cache Data : %s ", managedData.compileInfo.filePath.c_str());
			}
		}
		else
		{
			if (!managedData.sourceFile.empty())
			{
				LogDevMsg(0, "Recompile shader : %s , source file : %s ", managedData.compileInfo.filePath.c_str(), managedData.sourceFile.c_str());
			}
			else
			{
				LogDevMsg(0, "Recompile shader : %s ", managedData.compileInfo.filePath.c_str());
			}

			//if (!shaderProgram.mRHIResource.isValid())
			{
				shader.mRHIResource = RHICreateShader(managedData.compileInfo.type);
				if (!shader.mRHIResource.isValid())
					return false;
			}

			ShaderSetupData setupData;
			setupData.managedData = &managedData;

			mShaderFormat->precompileCode(setupData);

			bool bFailed = false;
			ShaderCompileInfo& shaderInfo = managedData.compileInfo;

			ShaderCompileInput  compileInput;
			compileInput.type = shaderInfo.type;
			compileInput.entry = shaderInfo.entryName.c_str();
			compileInput.path = shaderInfo.filePath.c_str();
			compileInput.definition = shaderInfo.headCode.c_str();
			compileInput.shaderSetupData = &setupData;
			ShaderCompileOutput compileOutput;
			compileOutput.compileInfo = &shaderInfo;
			compileOutput.formatData = nullptr;
			if (!mShaderFormat->compileCode(compileInput, compileOutput))
			{
				bFailed = true;
			}

			if (managedData.bShowComplieInfo)
			{

			}

			ShaderResourceInfo& shaderResource = setupData.shaderResource;
			shaderResource.type = shaderInfo.type;
			shaderResource.entry = shaderInfo.entryName.c_str();
			shaderResource.resource = compileOutput.resource;
			shaderResource.formatData = compileOutput.formatData;

			if (bFailed)
			{
				return false;
			}

			if (!mShaderFormat->initializeShader(shader, setupData))
			{
				return false;
			}

			if (CVarShaderUseCache && !getCache()->saveCacheData(*mShaderFormat, shader, setupData))
			{

			}
		}

		mShaderFormat->postShaderLoaded(shader);

		return true;
	}

	void ShaderManager::removeFromShaderCompileMap(ShaderObject& shader)
	{
		auto iter = mShaderDataMap.find(&shader);

		if (iter != mShaderDataMap.end())
		{
			if (mAssetViewerReigster && CVarShaderDectectFileMoidfy)
			{
				mAssetViewerReigster->unregisterViewer(iter->second);
			}
			delete iter->second;
			mShaderDataMap.erase(iter);
		}
	}

	void ShaderManager::generateCompileSetup(ShaderProgramManagedData& managedData, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, char const* additionalCode, char const* fileName, bool bSingleFile)
	{
		assert( fileName &&  managedData.compileInfos.empty());

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
			managedData.compileInfos.push_back({ entry.type , path.c_str() , std::move(headCode) , entry.name });
		}
	}

	void ShaderManager::generateCompileSetup(ShaderManagedData& managedData, ShaderEntryInfo const& entry, ShaderCompileOption const& option, char const* additionalCode, char const* fileName, bool bSingleFile)
	{
		assert(fileName);

		std::string defCode;
		mShaderFormat->getHeadCode(defCode, option, entry);

		std::string headCode = option.getCode(entry, defCode.c_str(), additionalCode);

		FixString< 256 > path;
		if (bSingleFile)
		{
			path.format("%s%s%s", mBaseDir.c_str(), fileName, SHADER_FILE_SUBNAME);
		}
		else
		{
			path.format("%s%s%s", mBaseDir.c_str(), fileName, ShaderPosfixNames[entry.type]);
		}
		managedData.compileInfo = { entry.type , path.c_str() , std::move(headCode) , entry.name };

	}

	void ShaderManager::postShaderLoaded(ShaderObject& shader, ShaderManagedDataBase& managedData)
	{
		mShaderDataMap.insert({ &shader , &managedData });
		if (mAssetViewerReigster && CVarShaderDectectFileMoidfy)
		{
			mAssetViewerReigster->registerViewer(&managedData);
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

	void ShaderProgramManagedData::getDependentFilePaths(std::vector<std::wstring>& paths)
	{
		std::set< HashString > filePathSet;
		for( ShaderCompileInfo const& compileInfo : compileInfos )
		{
			filePathSet.insert(compileInfo.filePath);
			filePathSet.insert(compileInfo.includeFiles.begin(), compileInfo.includeFiles.end());
		}	
		for( auto const& filePath : filePathSet )
		{
			paths.push_back( FCString::CharToWChar(filePath) );
		}
	}

	void ShaderProgramManagedData::postFileModify(EFileAction action)
	{
		if ( action == EFileAction::Modify )
			ShaderManager::Get().updateShaderInternal(*shaderProgram, *this, true);
	}


	void ShaderManagedData::getDependentFilePaths(std::vector<std::wstring>& paths)
	{
		std::set< HashString > filePathSet;

		filePathSet.insert(compileInfo.filePath);
		filePathSet.insert(compileInfo.includeFiles.begin(), compileInfo.includeFiles.end());

		for (auto const& filePath : filePathSet)
		{
			paths.push_back(FCString::CharToWChar(filePath));
		}
	}

	void ShaderManagedData::postFileModify(EFileAction action)
	{
		if (action == EFileAction::Modify)
			ShaderManager::Get().updateShaderInternal(*shader, *this, true);
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


