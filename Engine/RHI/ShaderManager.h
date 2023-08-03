#pragma once
#ifndef ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2
#define ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2

#include "ShaderFormat.h"
#include "GlobalShader.h"

#include "Singleton.h"
#include "AssetViewer.h"

#include "InlineString.h"

#include <unordered_map>
#include <map>
#include <unordered_set>

namespace CPP
{
	class CodeSourceLibrary;
}

class DataCacheInterface;

namespace Render
{
	class ShaderProgram;
	class MaterialShaderProgramClass;
	class MaterialShaderProgram;
	class VertexFactoryType;

	enum class ShaderClassType
	{
		Common,
		Global,
		Material,
	};

	struct ShaderManagedDataBase : public IAssetViewer
	{
		ShaderClassType classType = ShaderClassType::Common;
		GlobalShaderObjectClass const* shaderClass = nullptr;

		uint32         permutationId = 0;
		bool           bShowComplieInfo = false;
		std::string    sourceFile;
		std::unordered_set< HashString > includeFiles;
	};

	struct ShaderProgramManagedData : public ShaderManagedDataBase
	{
		ShaderProgram*  shaderProgram = nullptr;
		TArray< ShaderCompileDesc > descList;
	protected:
		virtual void getDependentFilePaths(TArray<std::wstring>& paths) override;
		virtual void postFileModify(EFileAction action) override;
	};

	struct ShaderManagedData : public ShaderManagedDataBase
	{
		Shader* shader = nullptr;
		ShaderCompileDesc desc;
	protected:
		virtual void getDependentFilePaths(TArray<std::wstring>& paths) override;
		virtual void postFileModify(EFileAction action) override;
	};

	typedef TArray< std::pair< MaterialShaderProgramClass*, MaterialShaderProgram* > > MaterialShaderPairVec;

	class ShaderManager : public SingletonT< ShaderManager >
	{
	public:
		ShaderManager();
		~ShaderManager();

		static CORE_API ShaderManager& Get();

		bool initialize(ShaderFormat& shaderFormat);

		void setBaseDir(char const* dir){  mBaseDir = dir;  }
		void clearnupRHIResouse();

		void setDataCache(DataCacheInterface* dataCache);
		void setAssetViewerRegister(IAssetViewerRegister* reigster);

		template< class TShaderType >
		TShaderType* getGlobalShaderT(bool bForceLoad = true)
		{
			uint32 permutationId = 0;
			return static_cast<TShaderType*>( getGlobalShader(TShaderType::GetShaderClass(), permutationId, bForceLoad) );
		}

		template< class TShaderType >
		TShaderType* getGlobalShaderT(typename TShaderType::PermutationDomain const& domain, bool bForceLoad = true)
		{
			return static_cast<TShaderType*>(getGlobalShader(TShaderType::GetShaderClass(), domain.getPermutationId(), bForceLoad));
		}

		template< class TShaderType >
		TShaderType* getGlobalShaderT(uint32 permutationId, bool bForceLoad = true)
		{
			return static_cast<TShaderType*>(getGlobalShader(TShaderType::GetShaderClass(), permutationId, bForceLoad));
		}

		template< class TShaderType >
		TShaderType* loadGlobalShaderT(ShaderCompileOption& option)
		{
			uint32 permutationId = 0;
			return static_cast<TShaderType*>(constructShaderInternal(TShaderType::GetShaderClass(), permutationId, option, ShaderClassType::Common));
		}

		int loadMeshMaterialShaders(MaterialShaderCompileInfo const& info, VertexFactoryType& vertexFactoryType , MaterialShaderPairVec& outShaders );

		MaterialShaderProgram* loadMaterialShader(MaterialShaderCompileInfo const& info, MaterialShaderProgramClass const& materialClass );

		CORE_API ShaderObject* getGlobalShader(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, bool bForceLoad );
		CORE_API void cleanupLoadedSource();

		bool registerGlobalShader(GlobalShaderObjectClass& shaderClass, uint32 permutationCount);

		int  loadAllGlobalShaders();

		ShaderObject* constructGlobalShader(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, ShaderCompileOption& option);
		ShaderObject* constructShaderInternal(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, ShaderCompileOption& option, ShaderClassType classType);

		void cleanupGlobalShader();

		bool loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries,
					  char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  char const* def = nullptr, char const* additionalCode = nullptr );

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName,
					  TArrayView< ShaderEntryInfo const > entries,
			          ShaderCompileOption const& option, char const* additionalCode = nullptr);


		bool loadFile(Shader& shader, char const* fileName, EShader::Type type, char const* entryName, char const* additionalCode = nullptr);

		bool loadSimple(ShaderProgram& shaderProgram, char const* fileNameVS, char const* fileNamePS, 
			            char const* entryVS = nullptr, char const* entryPS = nullptr, char const* def = nullptr, char const* additionalCode = nullptr);


		bool reloadShader(ShaderProgram& shaderProgram);
		bool reloadShader(Shader& shader);

		void reloadAll();

		void registerShaderAssets();
		void unregisterShaderAssets();

		struct LoadParam
		{
			TArrayView< ShaderEntryInfo const > entries;
			char const* define;
			char const* additionalCode;
		};

		ShaderProgramManagedData* loadInternal(ShaderProgram& shaderProgram, char const* filePaths[], 
			TArrayView< ShaderEntryInfo const > entries, char const* def, char const* additionalCode,
			ShaderClassType classType = ShaderClassType::Common);

		ShaderProgramManagedData* loadInternal(ShaderProgram& shaderProgram, char const* fileName, 
			TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, 
			char const* additionalCode, bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);

		ShaderManagedData* loadInternal(Shader& shader, char const* fileName, 
			ShaderEntryInfo const& entry, ShaderCompileOption const& option, 
			char const* additionalCode, ShaderClassType classType = ShaderClassType::Common);
		
		static TArrayView< ShaderEntryInfo const > MakeEntryInfos(ShaderEntryInfo entries[], uint8 shaderMask, char const* entryNames[])
		{
			int indexUsed = 0;
			for( int i = 0; i < EShader::Count; ++i )
			{
				if( (shaderMask & BIT(i)) == 0 )
					continue;
				entries[indexUsed].type = EShader::Type(i);
				entries[indexUsed].name = (entryNames) ? entryNames[indexUsed] : "main";
				++indexUsed;
			}
			return{ entries , indexUsed };
		}

		struct ShaderCacheData
		{
			RHIShaderRef shader;
			std::string filePath;
			std::string define;
		};
		bool buildShader(ShaderProgram& shaderProgram, ShaderProgramManagedData& managedData, bool bForceReload = false );
		bool buildShader(Shader& shader, ShaderManagedData& managedData, bool bForceReload = false);

		void removeFromShaderCompileMap(ShaderObject& shader);

		void  setupManagedData(
			ShaderProgramManagedData& managedData, TArrayView< ShaderEntryInfo const > entries,
			ShaderCompileOption const& option, char const* additionalCode ,
			char const* fileName , bool bSingleFile );

		void  setupManagedData(
			ShaderProgramManagedData& managedData, TArrayView< ShaderEntryInfo const > entries,
			ShaderCompileOption const& option, char const* additionalCode,
			char const* fileNames[]);

		void setupManagedData(ShaderManagedData& managedData, ShaderEntryInfo const& entry, 
			ShaderCompileOption const& option, char const* additionalCode, 
			char const* fileName);

		void postShaderLoaded(ShaderObject& shader, ShaderManagedDataBase& managedData);

		uint32          mDefaultVersion;
		ShaderFormat*   mShaderFormat = nullptr;
		std::string     mBaseDir;
		class ShaderCache* mShaderCache = nullptr;

		IAssetViewerRegister* mAssetViewerReigster;
		ShaderCache* getCache();

		CPP::CodeSourceLibrary* mSourceLibrary = nullptr;
		struct GlobalShaderEntry
		{
			uint32 permutationId;
			GlobalShaderObjectClass const* shaderClass;

			bool operator == (GlobalShaderEntry const& rhs) const
			{
				return permutationId == rhs.permutationId &&
					shaderClass == rhs.shaderClass;
			}

			uint32 getTypeHash() const
			{
				uint32 result = HashValue(permutationId);
				result = HashCombine(result, shaderClass);
				return result;
			}
		};
#if 1
		std::unordered_map< ShaderObject*, ShaderManagedDataBase* > mShaderDataMap;
		std::unordered_map< GlobalShaderEntry, ShaderObject* , MemberFuncHasher > mGlobalShaderMap;
#else
		std::map< ShaderObject*, ShaderProgramManagedData* > mShaderDataMap;
		std::map<GlobalShaderEntry, GlobalShaderProgram* > mGlobalShaderMap;
#endif

	};

}


#endif // ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2