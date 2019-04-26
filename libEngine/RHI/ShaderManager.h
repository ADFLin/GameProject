#pragma once
#ifndef ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2
#define ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2

#include "ShaderFormat.h"
#include "GlobalShader.h"

#include "Singleton.h"
#include "Asset.h"

#include "FixString.h"

#include <unordered_map>
#include <map>



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

	struct ShaderEntryInfo
	{
		Shader::Type type;
		char const*  name;
	};

	struct ShaderProgramCompileInfo : public IAssetViewer
	{
		ShaderProgram*  shaderProgram;
		ShaderClassType classType = ShaderClassType::Common;
		std::vector< ShaderCompileInfo > shaders;
		bool           bShowComplieInfo = false;
	protected:
		virtual void getDependentFilePaths(std::vector<std::wstring>& paths) override;
		virtual void postFileModify(FileAction action) override;
	};


	typedef std::vector< std::pair< MaterialShaderProgramClass*, MaterialShaderProgram* > > MaterialShaderPairVec;

	class ShaderManager : public SingletonT< ShaderManager >
	{
	public:
		ShaderManager();
		~ShaderManager();


		static CORE_API ShaderManager& Get();

		void setBaseDir(char const* dir){  mBaseDir = dir;  }
		void clearnupRHIResouse();

		void setDataCache(DataCacheInterface* dataCache);

		template< class ShaderType >
		ShaderType* getGlobalShaderT(bool bForceLoad = true)
		{
			return static_cast<ShaderType*>( getGlobalShader(ShaderType::GetShaderClass() , bForceLoad) );
		}

		template< class ShaderType >
		ShaderType* loadGlobalShaderT( ShaderCompileOption& option)
		{
			return static_cast<ShaderType*>(constructShaderInternal(ShaderType::GetShaderClass(), ShaderClassType::Common , option ));
		}

		int loadMaterialShaders(MaterialShaderCompileInfo const& info, VertexFactoryType& vertexFactoryType , MaterialShaderPairVec& outShaders );

		CORE_API GlobalShaderProgram* getGlobalShader(GlobalShaderProgramClass const& shaderClass , bool bForceLoad );

		bool registerGlobalShader(GlobalShaderProgramClass& shaderClass);

		int  loadAllGlobalShaders();

		GlobalShaderProgram* constructGlobalShader(GlobalShaderProgramClass const& shaderClass);
		GlobalShaderProgram* constructShaderInternal(GlobalShaderProgramClass const& shaderClass, ShaderClassType classType, ShaderCompileOption& option );

		void cleanupGlobalShader();

		bool loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries,
					  char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  char const* def = nullptr, char const* additionalCode = nullptr );

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  uint8 shaderMask, char const* entryNames[], 
					  char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);


		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries,
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName,
					  uint8 shaderMask, char const* entryNames[],
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);


		bool loadMultiFile(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadSimple(ShaderProgram& shaderProgram, char const* fileNameVS, char const* fileNamePS, char const* def = nullptr, char const* additionalCode = nullptr);

		bool reloadShader(ShaderProgram& shaderProgram);

		void reloadAll();

		void registerShaderAssets(AssetManager& assetManager)
		{
			for( auto pair : mShaderCompileMap )
			{
				assetManager.registerViewer(pair.second);
			}
		}

		void unregisterShaderAssets(AssetManager& assetManager)
		{
			for( auto pair : mShaderCompileMap )
			{
				assetManager.unregisterViewer(pair.second);
			}
		}

		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def, char const* additionalCode, bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);
		bool loadInternal(ShaderProgram& shaderProgram, char const* filePaths[], TArrayView< ShaderEntryInfo const > entries, char const* def, char const* additionalCode, ShaderClassType classType = ShaderClassType::Common);

		static TArrayView< ShaderEntryInfo const > MakeEntryInfos(ShaderEntryInfo entries[], uint8 shaderMask, char const* entryNames[])
		{
			int indexUsed = 0;
			for( int i = 0; i < Shader::NUM_SHADER_TYPE; ++i )
			{
				if( (shaderMask & BIT(i)) == 0 )
					continue;
				entries[indexUsed].type = Shader::Type(i);
				entries[indexUsed].name = (entryNames) ? entryNames[indexUsed] : nullptr;
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
		bool updateShaderInternal(ShaderProgram& shaderProgram, ShaderProgramCompileInfo& info , bool bForceReload = false );


		void removeFromShaderCompileMap( ShaderProgram& shader )
		{
			auto iter = mShaderCompileMap.find(&shader);

			if( iter != mShaderCompileMap.end() )
			{
				delete iter->second;
				mShaderCompileMap.erase(iter);
			}
		}

		void  generateCompileSetup( 
			ShaderProgramCompileInfo& compileInfo , TArrayView< ShaderEntryInfo const > entries,
			ShaderCompileOption const& option, char const* additionalCode ,
			char const* fileName , bool bSingleFile );

		uint32          mDefaultVersion;
		ShaderFormat*   mShaderFormat = nullptr;
		std::string     mBaseDir;
		class ShaderCache* mShaderCache = nullptr;

		ShaderCache* getCache();

#if 1
		std::unordered_map< ShaderProgram*, ShaderProgramCompileInfo* > mShaderCompileMap;
		std::unordered_map< GlobalShaderProgramClass const*, GlobalShaderProgram* > mGlobalShaderMap;
#else
		std::map< ShaderProgram*, ShaderProgramCompileInfo* > mShaderCompileMap;
		std::map< GlobalShaderProgramClass const*, GlobalShaderProgram* > mGlobalShaderMap;
#endif

	};

}


#endif // ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2