#pragma once
#ifndef ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2
#define ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2

#include "ShaderFormat.h"
#include "GlobalShader.h"

#include "Singleton.h"
#include "AssetViewer.h"

#include "InlineString.h"
#include "Misc/Format.h"

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

	class ShaderManagedDataBase : public IAssetViewer
	{
	public:
		ShaderClassType classType = ShaderClassType::Common;
		GlobalShaderObjectClass const* shaderClass = nullptr;

		uint32         permutationId = 0;
		bool           bShowComplieInfo = false;
		std::string    sourceFile;
		std::unordered_set< HashString > includeFiles;
	};

	class ShaderProgramManagedData : public ShaderManagedDataBase
	{
	public:
		ShaderProgram*  shaderProgram = nullptr;
		TArray< ShaderCompileDesc > descList;
	protected:
		virtual void getDependentFilePaths(TArray<std::wstring>& paths) override;
		virtual void postFileModify(EFileAction action) override;
	};

	class ShaderManagedData : public ShaderManagedDataBase
	{
	public:
		Shader* shader = nullptr;
		ShaderCompileDesc desc;
	protected:
		virtual void getDependentFilePaths(TArray<std::wstring>& paths) override;
		virtual void postFileModify(EFileAction action) override;
	};

	struct MaterialLoadResult
	{
		MaterialShaderProgramClass* programClass;
		VertexFactoryType*          factoryType;
		MaterialShaderProgram*      program;
		uint32  permutationId;
	};

	typedef TArray< std::pair< MaterialShaderProgramClass*, MaterialShaderProgram* > > MaterialShaderPairVec;

	class ShaderManager : public SingletonT< ShaderManager >
	{
	public:
		ShaderManager();
		~ShaderManager();

		static CORE_API ShaderManager& Get();

		template< typename TShader, typename ...TShaderArgs>
		static void ReloadShader(TShader& shader , TShaderArgs& ...shaders)
		{
			Get().reloadShader(shader);
			ReloadShader(shaders...);
		}
		template< typename TShader>
		static void ReloadShader(TShader& shader)
		{
			Get().reloadShader(shader);
		}

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

		int loadMeshMaterialShaders(MaterialShaderCompileInfo const& info, TArray< MaterialLoadResult>& outShaders );

		CORE_API ShaderObject* getGlobalShader(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, bool bForceLoad );
		CORE_API void cleanupLoadedSource();

		bool registerGlobalShader(GlobalShaderObjectClass& shaderClass, uint32 permutationCount);

		int  loadAllGlobalShaders();

		ShaderObject* constructGlobalShader(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, ShaderCompileOption& option);
		ShaderObject* constructShaderInternal(GlobalShaderObjectClass const& shaderObjectClass, uint32 permutationId, ShaderCompileOption& option, ShaderClassType classType);

		void cleanupGlobalShader();

		bool loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries,
					  char const* def = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  char const* def = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  ShaderCompileOption const& option);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName,
					  TArrayView< ShaderEntryInfo const > entries,
			          ShaderCompileOption const& option);


		bool loadFile(Shader& shader, char const* fileName, EShader::Type type, char const* entryName);
		bool loadFile(Shader& shader, char const* fileName, ShaderEntryInfo const& entry, ShaderCompileOption const& option);

		bool loadSimple(ShaderProgram& shaderProgram, char const* fileNameVS, char const* fileNamePS, 
			            char const* entryVS = nullptr, char const* entryPS = nullptr, char const* def = nullptr);


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

		ShaderProgramManagedData* loadInternal(ShaderProgram& shaderProgram, char const* fileName, 
			TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, 
			bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);

		ShaderProgramManagedData* loadInternal(ShaderProgram& shaderProgram, char const* filePaths[], 
			TArrayView< ShaderEntryInfo const > entries, char const* def, ShaderClassType classType = ShaderClassType::Common);

		ShaderManagedData* loadInternal(Shader& shader, char const* fileName, 
			ShaderEntryInfo const& entry, ShaderCompileOption const& option, 
			ShaderClassType classType = ShaderClassType::Common);
		

		static TArrayView< ShaderEntryInfo const > MakeEntryInfos(ShaderEntryInfo entries[], uint8 shaderMask, char const* entryNames[])
		{
			size_t indexUsed = 0;
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
		bool buildShader(ShaderProgramManagedData& managedData, bool bForceReload = false );
		bool buildShader(ShaderManagedData& managedData, bool bForceReload = false);

		void removeFromShaderCompileMap(ShaderObject& shader);

		void  setupManagedData(
			ShaderProgramManagedData& managedData, TArrayView< ShaderEntryInfo const > entries,
			ShaderCompileOption const& option, char const* fileName , bool bSingleFile );

		void  setupManagedData(
			ShaderProgramManagedData& managedData, TArrayView< ShaderEntryInfo const > entries,
			ShaderCompileOption const& option, char const* fileNames[]);

		void setupManagedData(ShaderManagedData& managedData, ShaderEntryInfo const& entry, 
			ShaderCompileOption const& option, char const* fileName);

		void postShaderLoaded(ShaderObject& shader, ShaderManagedDataBase& managedData, ShaderClassType classType);

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
				uint32 result = HashValues(permutationId, shaderClass);
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

	template< typename TShaderType >
	static bool LoadRuntimeShader(TShaderType& shader, char const* path, TArrayView< ShaderEntryInfo const > entries, TArrayView< StringView > codes, ShaderCompileOption& option)
	{
		std::string code;
		std::vector<uint8> codeTemplate;
		if (!FFileUtility::LoadToBuffer(path, codeTemplate, true))
		{
			return false;
		}
		Text::Format((char const*)codeTemplate.data(), codes, code);
		option.addCode((char const*)code.data());

		if constexpr (std::is_base_of_v<TShaderType, Shader>)
		{
			if (!ShaderManager::Get().loadFile(shader, nullptr, entries[0], option))
			{
				return false;
			}
		}
		else
		{
			if (!ShaderManager::Get().loadFile(shader, nullptr, entries, option))
			{
				return false;
			}
		}

		return true;
	}

	template< typename TShaderType >
	bool LoadRuntimeShader(TShaderType& shader, char const* path, TArray< StringView > codes, ShaderCompileOption& option)
	{
		return LoadRuntimeShader(shader, TShaderType::GetShaderFileName(), TShaderType::GetShaderEntries(), codes, option);
	}
}


#endif // ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2