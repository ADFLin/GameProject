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
#include "CPreprocessor.h"
#include "ProfileSystem.h"

//#TODO remove
#include "Renderer/BasePassRendering.h"
#include "Renderer/ShadowDepthRendering.h"

extern CORE_API TConsoleVariable< bool > CVarShaderUseCache;
extern CORE_API TConsoleVariable< bool > CVarShaderDectectFileModify;

#if CORE_SHARE_CODE
TConsoleVariable< bool > CVarShaderUseCache( true , "Shader.bUseCache" , CVF_CONFIG );
TConsoleVariable< bool > CVarShaderDectectFileModify(true, "Shader.DetectFileModify", CVF_CONFIG);
#endif

namespace Render
{
	struct ShaderCacheBinaryData
	{
	public:
		bool addFileDependences( ShaderProgramManagedData const& managedData )
		{
			std::set< HashString > assetFilePaths;


			for( auto const& desc : managedData.descList )
			{
				assetFilePaths.insert(desc.filePath);		
			}
			assetFilePaths.insert(managedData.includeFiles.begin(), managedData.includeFiles.end());
			return addFileDependencesInternal(assetFilePaths);
		}
		bool addFileDependences(ShaderManagedData const& managedData)
		{
			std::set< HashString > assetFilePaths;
			auto const& desc = managedData.desc;
			assetFilePaths.insert(desc.filePath);
			assetFilePaths.insert(managedData.includeFiles.begin(), managedData.includeFiles.end());
			return addFileDependencesInternal(assetFilePaths);
		}

		bool addFileDependencesInternal(std::set< HashString > const& assetFilePaths)
		{
			for (auto const& path : assetFilePaths)
			{
				FileAttributes fileAttributes;
				if (!FFileSystem::GetFileAttributes(path.c_str(), fileAttributes))
					return false;
				ShaderFile file;
				file.lastModifyTime = fileAttributes.lastWrite;
				file.path = path.c_str();
				assetDependences.push_back(std::move(file));
			}

			return true;
		}

		bool checkAssetNotModified()
		{
			for( auto const& asset : assetDependences )
			{
				FileAttributes fileAttributes;
				if( !FFileSystem::GetFileAttributes(asset.path.c_str(), fileAttributes) )
					return false;
				if( fileAttributes.lastWrite > asset.lastModifyTime )
					return false;
			}
			return true;
		}

		bool setupProgram(ShaderFormat& format, ShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& shaderCompiles )
		{
			if( codeBuffer.empty() )
				return false;

			if( !shaderProgram.mRHIResource.isValid() )
			{
				shaderProgram.mRHIResource = RHICreateShaderProgram();
				if( !shaderProgram.mRHIResource.isValid() )
					return false;
			}

			ShaderParameterMap* parameterMap = format.initializeProgram(*shaderProgram.mRHIResource, shaderCompiles, codeBuffer);
			if (parameterMap == nullptr)
			{
				shaderProgram.mRHIResource.release();
				return false;
			}
			shaderProgram.bindParameters(*parameterMap);
			return true;
		}

		bool setupShader(ShaderFormat& format, Shader& shader, ShaderCompileDesc const& shaderCompile)
		{
			if (codeBuffer.empty())
				return false;

			if (!shader.mRHIResource.isValid())
			{
				shader.mRHIResource = RHICreateShader(shaderCompile.type);
				if (!shader.mRHIResource.isValid())
					return false;
			}

			ShaderParameterMap* parameterMap = format.initializeShader(*shader.mRHIResource, shaderCompile, codeBuffer);
			if (parameterMap == nullptr)
			{
				shader.mRHIResource.release();
				return false;
			}
			shader.bindParameters(*parameterMap);
			return true;
		}


		struct ShaderFile
		{
			std::string path;
			DateTime    lastModifyTime;

			template< class Op >
			void serialize(Op& op)
			{
				op & path & lastModifyTime;
			}
		};

		TArray< ShaderFile > assetDependences;
		TArray< uint8 > codeBuffer;

		template< class Op >
		void serialize(Op& op)
		{
			op & assetDependences & codeBuffer;
		}
	};

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
			for( auto const& compileInfo : managedData.descList )
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

			auto const& compileInfo = managedData.desc;
			outKey.keySuffix.add((uint8)compileInfo.type, format.getName(), compileInfo.filePath.c_str(), compileInfo.headCode.c_str());
		
		}

		bool saveCacheData( ShaderFormat& format, ShaderProgramSetupData& setupData, ShaderProgramManagedData const& managedData)
		{
			if( !format.doesSuppurtBinaryCode() )
				return false;

			ShaderCacheBinaryData binaryData;
			DataCacheKey key;
			GetShaderCacheKey(format, managedData, key);
			bool result = mDataCache->saveDelegate(key, [&setupData,&binaryData,&format,&managedData](IStreamSerializer& serializer)
			{			
				if (!format.getBinaryCode(setupData, binaryData.codeBuffer))
					return false;

				if (!binaryData.addFileDependences(managedData))
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

		bool saveCacheData(ShaderFormat& format, ShaderSetupData& setupData, ShaderManagedData const& managedData)
		{
			if (!format.doesSuppurtBinaryCode())
				return false;
			ShaderCacheBinaryData binaryData;
			DataCacheKey key;
			GetShaderCacheKey(format, managedData, key);
			bool result = mDataCache->saveDelegate(key, [&setupData, &binaryData, &format,&managedData](IStreamSerializer& serializer)
			{
				if (!format.getBinaryCode(setupData, binaryData.codeBuffer))
					return false;

				if (!binaryData.addFileDependences(managedData))
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

		bool canUseCacheData(ShaderFormat& format)
		{
			if (GRHIPrefEnabled)
				return false;

			if (!CVarShaderUseCache.getValue())
				return false;

			if (!format.doesSuppurtBinaryCode())
				return false;

			return true;
		}

		bool loadCacheData(ShaderFormat& format, ShaderProgramManagedData& managedData)
		{
			if (!canUseCacheData(format))
				return false;

			DataCacheKey key;
			GetShaderCacheKey(format, managedData, key);

			bool result = mDataCache->loadDelegate(key, [&managedData,&format](IStreamSerializer& serializer)
			{
				ShaderCacheBinaryData binaryData;
				serializer >> binaryData;

				if( !binaryData.checkAssetNotModified() )
					return false;

				if( !binaryData.setupProgram(format, *managedData.shaderProgram, managedData.descList ) )
					return false;

				for( auto const& asset : binaryData.assetDependences )
				{
					managedData.includeFiles.insert(asset.path.c_str());
				}
				return true;
			});

			return result;
		}


		bool loadCacheData(ShaderFormat& format, ShaderManagedData& managedData)
		{
			if (!canUseCacheData(format))
				return false;

			DataCacheKey key;
			GetShaderCacheKey(format, managedData, key);

			bool result = mDataCache->loadDelegate(key, [&managedData, &format](IStreamSerializer& serializer)
			{
				ShaderCacheBinaryData binaryData;
				serializer >> binaryData;

				if (!binaryData.checkAssetNotModified())
					return false;

				if (!binaryData.setupShader(format, *managedData.shader, managedData.desc))
					return false;

				for (auto const& asset : binaryData.assetDependences)
				{
					managedData.includeFiles.insert(asset.path.c_str());
				}
				return true;
			});

			return result;
		}

		DataCacheInterface* mDataCache;
	};

	char const* const ShaderPosfixNames[] =
	{
		"VS", "PS", "GS", "CS", "HS", "DS", "TS", "MS",
	};

#if CORE_SHARE_CODE

	ShaderManager& ShaderManager::Get()
	{
		static ShaderManager sInatance;
		return sInatance;
	}

	ShaderObject* ShaderManager::getGlobalShader(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, bool bForceLoad)
	{
		GlobalShaderEntry entry;
		entry.shaderClass = &shaderObjectClass;
		entry.permutationId = permutationId;
		ShaderObject* result = mGlobalShaderMap[entry];
		if( result == nullptr && bForceLoad )
		{
			ShaderCompileOption option;
			result = constructGlobalShader(shaderObjectClass, permutationId, option);
			if( result )
			{
				mGlobalShaderMap[entry] = result;
			}
		}
		return result;
	}

	void ShaderManager::cleanupLoadedSource()
	{
		if (mSourceLibrary)
		{
			delete mSourceLibrary;
			mSourceLibrary = nullptr;
		}
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
		cleanupLoadedSource();
	}

	void ShaderManager::clearnupRHIResouse()
	{	
		if (mAssetViewerReigster)
		{
			unregisterShaderAssets();
		}

		if( mShaderFormat )
		{
			delete mShaderFormat;
			mShaderFormat = nullptr;
		}

		cleanupGlobalShader();

		for (auto& pair : mShaderDataMap)
		{	
			delete pair.second;
		}
		mShaderDataMap.clear();
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

	int ShaderManager::loadMeshMaterialShaders(MaterialShaderCompileInfo const& info, TArray< MaterialLoadResult>& outShaders )
	{
		int result = 0;


#if 0
		TArray< std::string > classNames;
		for (auto pShaderClass : MaterialShaderProgramClass::ClassList)
		{
			classNames.push_back((pShaderClass->GetShaderFileName)());
		}
#endif

		for (auto pVertexFactoryType : VertexFactoryType::TypeList)
		{
			if (pVertexFactoryType != &LocalVertexFactory::StaticType)
			{
				continue;
			}
			LogDevMsg(0, "VertexFactory Type : %s", pVertexFactoryType->fileName);

			for (auto pShaderClass : MaterialShaderProgramClass::ClassList)
			{
				//#REMOVE
				//if (GRHISystem->getName() == RHISytemName::D3D11)
				{
					if (pShaderClass != &DeferredBasePassProgram::GetShaderClass()
						&& pShaderClass != &ShadowDepthProgram::GetShaderClass()
						&& pShaderClass != &ShadowDepthPositionOnlyProgram::GetShaderClass()
						)
						continue;
				}

				for (uint32 permutationId = 0; permutationId < pShaderClass->permutationCount; ++permutationId)
				{
					ShaderCompileOption option;
					pVertexFactoryType->getCompileOption(option);
					info.setup(option);
					option.addMeta("SourceFile", info.name);
					MaterialShaderProgram* shaderProgram = (MaterialShaderProgram*)constructShaderInternal(*pShaderClass, permutationId, option, ShaderClassType::Material);

					if (shaderProgram)
					{
						outShaders.push_back({ pShaderClass, pVertexFactoryType, shaderProgram, permutationId });
						++result;
					}
				}
			}
		}

		return result;
	}

	bool ShaderManager::registerGlobalShader(GlobalShaderObjectClass& shaderClass, uint32 permutationCount)
	{
		GlobalShaderEntry entry;
		entry.shaderClass = &shaderClass;
		for (uint id = 0; id < permutationCount; ++id)
		{
			entry.permutationId = id;
			mGlobalShaderMap.insert({ entry , nullptr });
		}
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
				GlobalShaderEntry const& entry = pair.first;
				ShaderCompileOption option;
				shaderObject = constructGlobalShader(*entry.shaderClass, entry.permutationId, option);
				mGlobalShaderMap[entry] = shaderObject;
			}

			if( shaderObject == nullptr )
			{
				++numFailLoad;
			}
		}
		return numFailLoad;
	}

	ShaderObject* ShaderManager::constructGlobalShader(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, ShaderCompileOption& option)
	{
		return constructShaderInternal(shaderObjectClass, permutationId, option, ShaderClassType::Global);
	}

	ShaderObject* ShaderManager::constructShaderInternal(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, ShaderCompileOption& option, ShaderClassType classType )
	{
		ShaderObject* result = (*shaderObjectClass.CreateShaderObject)();
		if( result )
		{
			switch (result->getObjectType())
			{
			case EShaderObjectType::Program:
				{
					GlobalShaderProgram* shaderProgram = static_cast<GlobalShaderProgram*>(result);
					GlobalShaderProgramClass const& shaderProgramClass = static_cast<GlobalShaderProgramClass const&>(shaderObjectClass);
					shaderProgramClass.SetupShaderCompileOption(option, permutationId);

					ShaderProgramManagedData* managedData = loadInternal(
						*shaderProgram, shaderProgramClass.GetShaderFileName(),
						shaderProgramClass.GetShaderEntries(permutationId), option,
						true, classType);

					if ( managedData )
					{
						if (classType == ShaderClassType::Material)
						{
							delete managedData;
						}
						else
						{
							managedData->shaderClass = &shaderObjectClass;
							managedData->permutationId = permutationId;
						}
					}
					else
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

					shaderClass.SetupShaderCompileOption(option, permutationId);

					ShaderManagedData* managedData = loadInternal(
						*shader, shaderClass.GetShaderFileName(), shaderClass.entry, option, classType);

					if ( managedData )
					{
						if (classType == ShaderClassType::Material)
						{
							delete managedData;
						}
						else
						{
							managedData->shaderClass = &shaderObjectClass;
							managedData->permutationId = permutationId;
						}
					}
					else
					{
						LogWarning(0, "Can't Load Shader %s", shaderClass.GetShaderFileName());
						delete result;
						result = nullptr;
					}
				}
				break;

			default:
				NEVER_REACH("Unknow Shader Object Type");
				break;
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

	bool ShaderManager::loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def)
	{
		ShaderEntryInfo entries[] =
		{
			{ EShader::Vertex , SHADER_ENTRY(MainVS) } ,
			{ EShader::Pixel , SHADER_ENTRY(MainPS) } ,
		};
		return loadFile(shaderProgram , fileName, entries , def);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, char const* def)
	{
		ShaderEntryInfo entries[] =
		{
			{ EShader::Vertex , vertexEntryName } ,
			{ EShader::Pixel , pixelEntryName } ,
		};
		return loadFile(shaderProgram, fileName, entries, def);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries, char const* def)
	{
		char const* filePaths[EShader::MaxStorageSize];
		InlineString< 256 > path;
		for (int i = 0; i < entries.size(); ++i)
		{
			filePaths[i] = fileName;
		}

		return !!loadInternal(shaderProgram, filePaths, entries, def);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, ShaderCompileOption const& option)
	{
		ShaderEntryInfo entries[] =
		{
			{ EShader::Vertex , vertexEntryName } ,
			{ EShader::Pixel , pixelEntryName } ,
		};
		return !!loadInternal(shaderProgram, fileName, entries, option, true);
	}

	bool ShaderManager::loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option)
	{
		return !!loadInternal(shaderProgram, fileName, entries, option, true);
	}

	ShaderProgramManagedData* ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, bool bSingleFile, ShaderClassType classType)
	{
		removeFromShaderCompileMap(shaderProgram);

		ShaderProgramManagedData* managedData = new ShaderProgramManagedData;
		managedData->classType = classType;
		managedData->shaderProgram = &shaderProgram;
		setupManagedData(*managedData, entries, option, fileName, bSingleFile );

		if( !buildShader(*managedData) )
		{
			delete managedData;
			return nullptr;
		}

		postShaderLoaded(shaderProgram, *managedData, classType);
		return managedData;
	}

	bool ShaderManager::loadFile(Shader& shader, char const* fileName, EShader::Type type, char const* entryName)
	{
		ShaderCompileOption option;
		ShaderEntryInfo entry;
		entry.name = entryName;
		entry.type = type;
		return !!loadInternal(shader, fileName, entry, option);
	}

	bool ShaderManager::loadFile(Shader& shader, char const* fileName, ShaderEntryInfo const& entry, ShaderCompileOption const& option)
	{
		return !!loadInternal(shader, fileName, entry, option); 
	}

	ShaderManagedData* ShaderManager::loadInternal(Shader& shader, char const* fileName, ShaderEntryInfo const& entry, ShaderCompileOption const& option, ShaderClassType classType)
	{
		removeFromShaderCompileMap(shader);

		ShaderManagedData* managedData = new ShaderManagedData;
		managedData->classType = classType;
		managedData->shader = &shader;
		setupManagedData(*managedData, entry, option, fileName);

		if (!buildShader(*managedData))
		{
			delete managedData;
			return nullptr;
		}

		postShaderLoaded(shader, *managedData, classType);
		return managedData;
	}

	ShaderProgramManagedData* ShaderManager::loadInternal(ShaderProgram& shaderProgram, char const* filePaths[], TArrayView< ShaderEntryInfo const > entries, char const* def, ShaderClassType classType)
	{
		removeFromShaderCompileMap(shaderProgram);

		ShaderProgramManagedData* managedData = new ShaderProgramManagedData;
		managedData->classType = classType;
		managedData->shaderProgram = &shaderProgram;

		ShaderCompileOption option;
		if (def)
		{
			option.addCode(def, EShaderCodePos::BeforeInclude);
		}
		setupManagedData(*managedData, entries, option, filePaths);

		if( !buildShader(*managedData) )
		{
			delete managedData;
			return nullptr;
		}

		postShaderLoaded(shaderProgram, *managedData, classType);
		return managedData;
	}

	bool ShaderManager::loadSimple(ShaderProgram& shaderProgram, char const* fileNameVS, char const* fileNamePS, char const* entryVS , char const* entryPS , char const* def)
	{
		ShaderEntryInfo entries[2] =
		{
			{ EShader::Vertex , entryVS ? entryVS : "main" },
			{ EShader::Pixel ,  entryPS ? entryPS : "main" },
		};

		char const* filePaths[2] = { fileNameVS , fileNamePS };
		return !!loadInternal(shaderProgram, filePaths, entries, def);
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
			GlobalShaderProgramClass const& myClass = *static_cast<GlobalShaderProgramClass const*>(managedData->shaderClass);
			myClass.SetupShaderCompileOption(option, managedData->permutationId);

			managedData->descList.clear();
			setupManagedData(*managedData, myClass.GetShaderEntries(managedData->permutationId), option, myClass.GetShaderFileName(), true);
		}

		return buildShader(*managedData, true);
	}


	bool ShaderManager::reloadShader(Shader& shader)
	{
		auto iter = mShaderDataMap.find(&shader);
		if (iter == mShaderDataMap.end())
			return false;

		ShaderManagedData* managedData = static_cast<ShaderManagedData*>(iter->second);
		if (managedData->classType == ShaderClassType::Global)
		{
			ShaderCompileOption option;
			GlobalShaderClass const& myClass = *static_cast<GlobalShaderClass const*>(managedData->shaderClass);
			myClass.SetupShaderCompileOption(option, managedData->permutationId);

			setupManagedData(*managedData, myClass.entry , option, myClass.GetShaderFileName());
		}

		return buildShader(*managedData, true);

	}

	void ShaderManager::reloadAll()
	{
		for( auto pair : mShaderDataMap )
		{
			switch (pair.first->getObjectType())
			{
			case EShaderObjectType::Shader:
				reloadShader(*static_cast<Shader*>(pair.first));
				break;
			case EShaderObjectType::Program:
				reloadShader(*static_cast<ShaderProgram*>(pair.first));
				break;
			}		
		}
	}

	void ShaderManager::registerShaderAssets()
	{
		if ( CVarShaderDectectFileModify )
		{
			for (auto pair : mShaderDataMap)
			{
				mAssetViewerReigster->registerViewer(pair.second);
			}
		}
	}

	void ShaderManager::unregisterShaderAssets()
	{
		if (CVarShaderDectectFileModify)
		{
			for (auto pair : mShaderDataMap)
			{
				mAssetViewerReigster->unregisterViewer(pair.second);
			}
		}
	}

	bool LoadCode(ShaderCompileContext& context, ShaderFormat& format, TArray<uint8>& outCodeBuffer)
	{
		if (context.bUsePreprocess)
		{
			if (!format.preprocessCode(context.haveFile() ? context.getPath() : nullptr, context.desc, context.getDefinition(), context.sourceLibrary, outCodeBuffer, context.includeFiles, context.bOuputPreprocessedCode))
				return false;

			context.codes.push_back(StringView((char const*)outCodeBuffer.data(), outCodeBuffer.size()));
		}
		else if (format.isMultiCodesCompileSupported())
		{
			StringView definition = context.getDefinition();
			if (definition.size())
			{
				context.codes.push_back(definition);
			}

			if (context.haveFile())
			{
				if (!FFileUtility::LoadToBuffer(context.getPath(), outCodeBuffer, true))
				{
					return false;
				}
				context.codes.push_back(StringView((char const*)outCodeBuffer.data(), outCodeBuffer.size()));
			}
		}
		else	
		{
			StringView definition = context.getDefinition();
			if (definition.size())
			{
				outCodeBuffer.resize(definition.size());
				outCodeBuffer.assign(definition.data(), definition.data() + definition.size());
			}

			if (context.haveFile())
			{
				if (!FFileUtility::LoadToBuffer(context.getPath(), outCodeBuffer, true, true))
				{
					LogWarning(0, "Can't load shader file %s", context.getPath());
					return false;
				}
			}

			context.codes.push_back(StringView((char const*)outCodeBuffer.data(), outCodeBuffer.size()));
		}

		return true;
	}



	bool CompileCode(ShaderCompileContext &context, ShaderFormat& shaderFormat)
	{
		do
		{
			TArray< uint8 > codeBuffer;
			if (!LoadCode(context, shaderFormat, codeBuffer))
			{
				return false;
			}

			switch (shaderFormat.compileCode(context))
			{
			case EShaderCompileResult::Ok:
				break;
			case EShaderCompileResult::CodeError:
				if (context.bAllowRecompile)
					continue;
			case EShaderCompileResult::ResourceError:
				return false;
			}
		} 
		while (0);

		return true;
	}


	bool ShaderManager::buildShader(ShaderProgramManagedData& managedData, bool bForceReload)
	{
		if ( !RHIIsInitialized() )
			return false;

		TIME_SCOPE("Build Shader Program");
		
		ShaderProgram& shaderProgram = *managedData.shaderProgram;

		//bForceReload = true;
		if( !bForceReload && getCache()->loadCacheData(*mShaderFormat, managedData) )
		{
			if (!managedData.sourceFile.empty())
			{
				LogDevMsg(0, "Use Cache Data : %s , source file : %s ", managedData.descList[0].filePath.c_str(), managedData.sourceFile.c_str());
			}
			else
			{
				LogDevMsg(0, "Use Cache Data : %s ", managedData.descList[0].filePath.c_str());
			}
		}
		else
		{
			if (!managedData.sourceFile.empty())
			{
				LogDevMsg(0, "Recompile shader : %s , source file : %s ", managedData.descList[0].filePath.c_str(), managedData.sourceFile.c_str());
			}
			else
			{
				LogDevMsg(0, "Recompile shader : %s ", managedData.descList[0].filePath.c_str());
			}

			ShaderProgramSetupData setupData;

			shaderProgram.mRHIResource.release();
			setupData.resource = RHICreateShaderProgram();
			if (!setupData.resource.isValid())
				return false;
	
			setupData.descList = MakeConstView(managedData.descList);
			mShaderFormat->precompileCode(setupData);

			ShaderResourceInfo shaders[EShader::MaxStorageSize];
			int shaderIndex = 0;
			bool bFailed = false;

			for (ShaderCompileDesc& desc : managedData.descList)
			{
				ShaderCompileContext  context;
				context.programSetupData = &setupData;
				context.shaderIndex = shaderIndex;
				context.desc = &desc;
				context.bAllowRecompile = context.haveFile();
				context.includeFiles = &managedData.includeFiles;
				if (context.bAllowRecompile)
				{
					if (mSourceLibrary == nullptr)
					{
						mSourceLibrary = new CPP::CodeSourceLibrary;
					}
					context.sourceLibrary = mSourceLibrary;
				}

				if (!CompileCode(context, *mShaderFormat))
				{
					bFailed = true;
				}

				if (managedData.bShowComplieInfo)
				{

				}

				++shaderIndex;
			}

			if (bFailed)
			{
				return false;
			}
	
			shaderProgram.mRHIResource = setupData.resource;
			shaderProgram.preInitialize();

			ShaderParameterMap* parameterMap = mShaderFormat->initializeProgram(*shaderProgram.mRHIResource, setupData);
			if (parameterMap == nullptr)
			{
				shaderProgram.mRHIResource.release();
				return false;
			}
			shaderProgram.bindParameters(*parameterMap);

			if (CVarShaderUseCache && !getCache()->saveCacheData(*mShaderFormat, setupData, managedData))
			{
				LogWarning(0, "Can't Save ShaderProgram Cache");
			}
		}

		mShaderFormat->postShaderLoaded(*shaderProgram.mRHIResource);
		return true;
	}


	std::string GetCompileInfo(ShaderManagedData& managedData)
	{
		std::string result = managedData.desc.filePath;
		result += " ";
		result += managedData.desc.entryName;
		if (!managedData.sourceFile.empty())
		{
			result += ", source file :";
			result += managedData.sourceFile;
		}
		return result;
	}

	bool ShaderManager::buildShader(ShaderManagedData& managedData, bool bForceReload)
	{
		if (!RHIIsInitialized())
			return false;


		TIME_SCOPE("Build Shader");

		Shader& shader = *managedData.shader;

		if (!bForceReload && getCache()->loadCacheData(*mShaderFormat, managedData))
		{
			LogDevMsg(0, "Use Cache Data : %s", GetCompileInfo(managedData).c_str());
		}
		else
		{
			LogDevMsg(0, "Recompile shader : %s", GetCompileInfo(managedData).c_str());

			ShaderSetupData setupData;
			shader.mRHIResource.release();
			setupData.resource = RHICreateShader(managedData.desc.type);
			if (!setupData.resource.isValid())
				return false;

			setupData.desc = &managedData.desc;

			mShaderFormat->precompileCode(setupData);

			bool bFailed = false;

			{
				ShaderCompileContext  context;
				context.shaderSetupData = &setupData;
				context.shaderIndex = 0;
				context.desc = &managedData.desc;
				context.includeFiles = &managedData.includeFiles;
				context.bAllowRecompile = context.haveFile();
				if (context.bAllowRecompile)
				{
					if (mSourceLibrary == nullptr)
					{
						mSourceLibrary = new CPP::CodeSourceLibrary;
					}
					context.sourceLibrary = mSourceLibrary;
				}

				if (!CompileCode(context, *mShaderFormat))
				{
					bFailed = true;
				}

				if (managedData.bShowComplieInfo)
				{

				}
			}

			if (bFailed)
			{
				return false;
			}

			shader.mRHIResource = setupData.resource;
			shader.preInitialize();

			ShaderParameterMap* parameterMap = mShaderFormat->initializeShader(*shader.mRHIResource, setupData);
			if (parameterMap == nullptr)
			{
				shader.mRHIResource.release();
				return false;
			}
			shader.bindParameters(*parameterMap);

			if (CVarShaderUseCache && !getCache()->saveCacheData(*mShaderFormat, setupData, managedData))
			{
				LogWarning(0, "Can't Save Shader Cache");
			}

			mShaderFormat->postShaderLoaded(*shader.mRHIResource);
		}

		return true;
	}

	void ShaderManager::removeFromShaderCompileMap(ShaderObject& shader)
	{
		auto iter = mShaderDataMap.find(&shader);

		if (iter != mShaderDataMap.end())
		{
			if (mAssetViewerReigster && CVarShaderDectectFileModify)
			{
				mAssetViewerReigster->unregisterViewer(iter->second);
			}
			delete iter->second;
			mShaderDataMap.erase(iter);
		}
	}

	void ShaderManager::setupManagedData(ShaderProgramManagedData& managedData, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, char const* fileName, bool bSingleFile)
	{
		CHECK( managedData.descList.empty());

		option.setup(*mShaderFormat);
		managedData.bShowComplieInfo = option.bShowComplieInfo;
		char const* sourceFile = option.getMeta("SourceFile");
		if (sourceFile)
		{
			managedData.sourceFile = sourceFile;
		}
		for( auto const& entry : entries )
		{
			std::string defCode;
			mShaderFormat->getHeadCode(defCode, option, entry);

			std::string headCode = option.getCode( entry , defCode.c_str());

			if (fileName)
			{
				InlineString< 256 > path;
				if (bSingleFile)
				{
					path.format("%s%s%s", mBaseDir.c_str(), fileName, SHADER_FILE_SUBNAME);
				}
				else
				{
					path.format("%s%s%s%s", mBaseDir.c_str(), fileName, ShaderPosfixNames[entry.type], SHADER_FILE_SUBNAME);
				}
				managedData.descList.push_back({ entry.type , path.c_str() , std::move(headCode) , entry.name });
			}
			else
			{
				managedData.descList.push_back({ entry.type , "" , std::move(headCode) , entry.name });
			}
		}
	}


	void ShaderManager::setupManagedData(ShaderProgramManagedData& managedData, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, char const* fileNames[])
	{
		CHECK(managedData.descList.empty());

		option.setup(*mShaderFormat);
		managedData.bShowComplieInfo = option.bShowComplieInfo;
		char const* sourceFile = option.getMeta("SourceFile");
		if (sourceFile)
		{
			managedData.sourceFile = sourceFile;
		}
		int index = 0;
		for (auto const& entry : entries)
		{
			std::string defCode;
			mShaderFormat->getHeadCode(defCode, option, entry);

			std::string headCode = option.getCode(entry, defCode.c_str());

			if (fileNames[index])
			{
				InlineString< 256 > path;
				path.format("%s%s%s", mBaseDir.c_str(), fileNames[index], SHADER_FILE_SUBNAME);
				managedData.descList.push_back({ entry.type , path.c_str() , std::move(headCode) , entry.name });
			}
			else
			{
				managedData.descList.push_back({ entry.type , "" , std::move(headCode) , entry.name });
			}
			++index;
		}
	}


	void ShaderManager::setupManagedData(ShaderManagedData& managedData, ShaderEntryInfo const& entry, ShaderCompileOption const& option, char const* fileName)
	{
		option.setup(*mShaderFormat);
		
		managedData.bShowComplieInfo = option.bShowComplieInfo;
		char const* sourceFile = option.getMeta("SourceFile");
		if (sourceFile)
		{
			managedData.sourceFile = sourceFile;
		}

		std::string defCode;
		mShaderFormat->getHeadCode(defCode, option, entry);

		std::string headCode = option.getCode(entry, defCode.c_str());

		if (fileName)
		{
			InlineString< 256 > path;
			path.format("%s%s%s", mBaseDir.c_str(), fileName, SHADER_FILE_SUBNAME);
			managedData.desc = { entry.type , path.c_str() , std::move(headCode) , entry.name };
		}
		else
		{
			managedData.desc = { entry.type , "" , std::move(headCode) , entry.name };
		}
	}

	void ShaderManager::postShaderLoaded(ShaderObject& shader, ShaderManagedDataBase& managedData, ShaderClassType classType)
	{
		if ( classType == ShaderClassType::Material )
			return;

		mShaderDataMap.insert({ &shader , &managedData });
		if (mAssetViewerReigster && CVarShaderDectectFileModify)
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

	void ShaderProgramManagedData::getDependentFilePaths(TArray<std::wstring>& paths)
	{
		std::set< HashString > filePathSet;
#if 0
		if (!sourceFile.empty())
		{
			filePathSet.insert(sourceFile.c_str());
		}
#endif
		for( ShaderCompileDesc const& desc : descList )
		{
			if ( desc.filePath.empty() )
				continue;
			filePathSet.insert(desc.filePath);	
		}	
		filePathSet.insert(includeFiles.begin(), includeFiles.end());
		for( auto const& filePath : filePathSet )
		{
			paths.push_back( FCString::CharToWChar(filePath) );
		}
	}

	void ShaderProgramManagedData::postFileModify(EFileAction action)
	{
		if (action == EFileAction::Modify || action == EFileAction::Rename)
		{
			ShaderManager::Get().buildShader(*this, true);
			ShaderManager::Get().cleanupLoadedSource();
		}
	}

	void ShaderManagedData::getDependentFilePaths(TArray<std::wstring>& paths)
	{
		std::set< HashString > filePathSet;
#if 0
		if (!sourceFile.empty())
		{
			filePathSet.insert(sourceFile.c_str());
		}
#endif
		if (!desc.filePath.empty())
		{
			filePathSet.insert(desc.filePath);
		}

		filePathSet.insert(includeFiles.begin(), includeFiles.end());
		for (auto const& filePath : filePathSet)
		{
			paths.push_back(FCString::CharToWChar(filePath));
		}
	}

	void ShaderManagedData::postFileModify(EFileAction action)
	{
		if (action == EFileAction::Modify || action == EFileAction::Rename)
		{
			ShaderManager::Get().buildShader(*this, true);
			ShaderManager::Get().cleanupLoadedSource();
		}
	}

}//namespace Render


