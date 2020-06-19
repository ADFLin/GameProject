#pragma once
#ifndef ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2
#define ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2

#include "ShaderFormat.h"
#include "GlobalShader.h"

#include "Singleton.h"
#include "AssetViewer.h"

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


	struct ShaderManagedDataBase : public IAssetViewer
	{
		ShaderClassType classType = ShaderClassType::Common;
		union 
		{
			ShaderProgram*  shaderProgram;
			Shader* shader;
		};

		bool           bShowComplieInfo = false;
		std::string    sourceFile;
		
	};

	struct ShaderProgramManagedData : public ShaderManagedDataBase
	{

		std::vector< ShaderCompileInfo > compileInfos;
		bool           bShowComplieInfo = false;
		std::string    sourceFile;
	protected:
		virtual void getDependentFilePaths(std::vector<std::wstring>& paths) override;
		virtual void postFileModify(EFileAction action) override;
	};

	struct ShaderManagedData : public ShaderManagedDataBase
	{
		ShaderCompileInfo compileInfo;
	protected:
		virtual void getDependentFilePaths(std::vector<std::wstring>& paths) override;
		virtual void postFileModify(EFileAction action) override;
	};

	typedef std::vector< std::pair< MaterialShaderProgramClass*, MaterialShaderProgram* > > MaterialShaderPairVec;

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
			return static_cast<TShaderType*>( getGlobalShader(TShaderType::GetShaderClass() , bForceLoad) );
		}

		template< class TShaderType >
		TShaderType* loadGlobalShaderT( ShaderCompileOption& option)
		{
			return static_cast<TShaderType*>(constructShaderInternal(TShaderType::GetShaderClass(), ShaderClassType::Common , option ));
		}

		int loadMaterialShaders(MaterialShaderCompileInfo const& info, VertexFactoryType& vertexFactoryType , MaterialShaderPairVec& outShaders );

		CORE_API ShaderObject* getGlobalShader(GlobalShaderObjectClass const& shaderObjectClass, bool bForceLoad );

		bool registerGlobalShader(GlobalShaderObjectClass& shaderClass);

		int  loadAllGlobalShaders();

		ShaderObject* constructGlobalShader(GlobalShaderObjectClass const& shaderObjectClass);
		ShaderObject* constructShaderInternal(GlobalShaderObjectClass const& shaderObjectClass, ShaderClassType classType, ShaderCompileOption& option);

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
		bool reloadShader(Shader& shader);

		void reloadAll();

		void registerShaderAssets();
		void unregisterShaderAssets();

		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def, char const* additionalCode, bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, TArrayView< ShaderEntryInfo const > entries, ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);
		bool loadInternal(ShaderProgram& shaderProgram, char const* filePaths[], TArrayView< ShaderEntryInfo const > entries, char const* def, char const* additionalCode, ShaderClassType classType = ShaderClassType::Common);

		bool loadInternal(Shader& shader, char const* fileName, ShaderEntryInfo const& entry, ShaderCompileOption const& option, char const* additionalCode, bool bSingleFile, ShaderClassType classType = ShaderClassType::Common);
		
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
		bool updateShaderInternal(ShaderProgram& shaderProgram, ShaderProgramManagedData& managedData, bool bForceReload = false );
		bool updateShaderInternal(Shader& shader, ShaderManagedData& managedData, bool bForceReload = false);

		void removeFromShaderCompileMap(ShaderObject& shader);

		void  generateCompileSetup(
			ShaderProgramManagedData& managedData, TArrayView< ShaderEntryInfo const > entries,
			ShaderCompileOption const& option, char const* additionalCode ,
			char const* fileName , bool bSingleFile );

		void generateCompileSetup(ShaderManagedData& managedData, ShaderEntryInfo const& entry, 
			ShaderCompileOption const& option, char const* additionalCode, 
			char const* fileName, bool bSingleFile);

		void postShaderLoaded(ShaderObject& shader, ShaderManagedDataBase& managedData);

		uint32          mDefaultVersion;
		ShaderFormat*   mShaderFormat = nullptr;
		std::string     mBaseDir;
		class ShaderCache* mShaderCache = nullptr;

		IAssetViewerRegister* mAssetViewerReigster;
		ShaderCache* getCache();

#if 1
		std::unordered_map< ShaderObject*, ShaderManagedDataBase* > mShaderDataMap;
		std::unordered_map< GlobalShaderObjectClass const*, ShaderObject* > mGlobalShaderMap;
#else
		std::map< ShaderObject*, ShaderProgramManagedData* > mShaderDataMap;
		std::map< GlobalShaderObjectClass const*, GlobalShaderProgram* > mGlobalShaderMap;
#endif

	};

}


#endif // ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2