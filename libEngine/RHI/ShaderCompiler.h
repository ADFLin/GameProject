#pragma once
#ifndef ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2
#define ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2

#include "GLCommon.h"
#include "ShaderCore.h"
#include "GlobalShader.h"
#include "Singleton.h"
#include "Asset.h"

#include "FixString.h"

#include <unordered_map>

#define SHADER_FILE_SUBNAME ".sgc"

namespace RenderGL
{
	class MaterialShaderProgramClass;
	class MaterialShaderProgram;
	class VertexFactoryType;


	enum ShaderFreature
	{



	};

	class ShaderCompileOption
	{
	public:
		ShaderCompileOption()
		{
			version = 0;
		}

		void addInclude(char const* name)
		{
			mIncludeFiles.push_back(name);
		}
		void addDefine(char const* name, bool value)
		{
			addDefine(name, value ? 1 : 0);
		}
		void addDefine(char const* name, int value)
		{
			ConfigVar var;
			var.name = name;
			FixString<128> str;
			var.value = str.format("%d", value);
			mConfigVars.push_back(var);
		}
		void addDefine(char const* name, float value)
		{
			ConfigVar var;
			var.name = name;
			FixString<128> str;
			var.value = str.format("%f", value);
			mConfigVars.push_back(var);
		}
		void addDefine(char const* name)
		{
			ConfigVar var;
			var.name = name;
			mConfigVars.push_back(var);
		}
		void addCode(char const* name)
		{
			mCodes.push_back(name);
		}
		struct ConfigVar
		{
			std::string name;
			std::string value;
		};

		std::string getCode(char const* defCode = nullptr, char const* addionalCode = nullptr) const;

		unsigned version;
		bool     bShowComplieInfo = false;

		std::vector< std::string > mCodes;
		std::vector< ConfigVar >   mConfigVars;
		std::vector< std::string > mIncludeFiles;
	};


	class ShaderCompiler
	{
	public:
		bool compileCode( Shader::Type type , RHIShader& shader , char const* path, char const* def = nullptr );

		bool bRecompile = true;
		bool bUsePreprocess = true;

	};

	struct ShaderEntryInfo
	{
		Shader::Type type;
		char const*  name;
	};

	enum class ShaderClassType
	{
		Common,
		Global,
		Material,
	};

	typedef std::vector< std::pair< MaterialShaderProgramClass*, MaterialShaderProgram* > > MaterialShaderPairVec;

	class ShaderManager : public SingletonT< ShaderManager >
	{
	public:
		ShaderManager();
		~ShaderManager();


		static CORE_API ShaderManager& Get();



		void clearnupRHIResouse();


		template< class ShaderType >
		ShaderType* getGlobalShaderT(bool bForceLoad = true)
		{
			return static_cast<ShaderType*>( getGlobalShader(ShaderType::GetShaderClass() , bForceLoad) );
		}

		int loadMaterialShaders(char const* fileName  , VertexFactoryType& vertexFactoryType , MaterialShaderPairVec& outShaders );

		GlobalShaderProgram* getGlobalShader(GlobalShaderProgramClass& shaderClass , bool bForceLoad );

		bool registerGlobalShader(GlobalShaderProgramClass& shaderClass);

		int  loadAllGlobalShaders();

		GlobalShaderProgram* constructGlobalShader(GlobalShaderProgramClass& shaderClass);
		GlobalShaderProgram* constructGlobalShaderInternal(GlobalShaderProgramClass& shaderClass, ShaderClassType classType, ShaderCompileOption& option );

		void cleanupGlobalShader();

		bool loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName,ShaderEntryInfo const entries[],
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


		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[],
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName,
					  uint8 shaderMask, char const* entryNames[],
					  ShaderCompileOption const& option, char const* additionalCode = nullptr);


		bool loadMultiFile(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, char const* additionalCode = nullptr);
		bool reloadShader(ShaderProgram& shaderProgram);

		void reloadAll();

		void registerShaderAssets(AssetManager& assetManager)
		{
			for( auto pair : mShaderCompileMap )
			{
				assetManager.registerAsset(pair.second);
			}
		}

		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], char const* def, char const* additionalCode, bool bSingle);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[], char const* def, char const* additionalCode, bool bSingle);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, ShaderEntryInfo const entries[], ShaderCompileOption const& option, char const* additionalCode, bool bSingle);
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingle);

		static void MakeEntryInfos(ShaderEntryInfo entries[], uint8 shaderMask, char const* entryNames[])
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
			entries[indexUsed].type = Shader::eEmpty;
			entries[indexUsed].name = nullptr;
		}

		struct ShaderCache
		{
			RHIShaderRef shader;
			std::string filePath;
			std::string define;
		};

		struct ShaderProgramCompileInfo;
		bool updateShaderInternal(ShaderProgram& shaderProgram, ShaderProgramCompileInfo& info);

		struct ShaderCompileInfo
		{
			Shader::Type type;
			std::string  headCode;

			template< class S >
			ShaderCompileInfo(Shader::Type inType, S&& inCode)
				:type(inType) , headCode( std::forward<S>(inCode) )
			{}

			ShaderCompileInfo(){}
		};



		struct ShaderProgramCompileInfo : public AssetBase
		{
			ShaderProgram* shaderProgram;
			std::string    fileName;
			std::vector< ShaderCompileInfo > shaders;
			bool           bSingleFile;
			bool           bShowComplieInfo = false;
			
			ShaderClassType classType = ShaderClassType::Common;
		protected:
			virtual void getDependentFilePaths(std::vector<std::wstring>& paths) override;
			virtual void postFileModify(FileAction action) override;
		};

		void removeFromShaderCompileMap( ShaderProgram& shader )
		{
			auto iter = mShaderCompileMap.find(&shader);

			if( iter != mShaderCompileMap.end() )
			{
				delete iter->second;
				mShaderCompileMap.erase(iter);
			}
		}

		void  generateCompileSetup( ShaderProgramCompileInfo& compileInfo , ShaderEntryInfo const entries[], ShaderCompileOption const& option, char const* additionalCode);

		uint32         mDefaultVersion;
		ShaderCompiler mCompiler;

		std::unordered_map< ShaderProgram*, ShaderProgramCompileInfo* > mShaderCompileMap;
		std::unordered_map< GlobalShaderProgramClass*, GlobalShaderProgram* > mGlobalShaderMap;

	};

}


#endif // ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2