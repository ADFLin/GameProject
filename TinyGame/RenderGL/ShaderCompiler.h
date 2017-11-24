#pragma once
#ifndef ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2
#define ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2

#include "GLCommon.h"
#include "Singleton.h"
#include "Asset.h"

#include <unordered_map>

#define SHADER_FILE_SUBNAME ".glsl"

namespace RenderGL
{
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
		void addDefine(char const* name)
		{
			ConfigVar var;
			var.name = name;
			mConfigVars.push_back(var);
		}
		struct ConfigVar
		{
			std::string name;
			std::string value;
		};

		std::string getCode( char const* defCode = nullptr , char const* addionalCode = nullptr ) const;

		unsigned version;
		std::vector< ConfigVar >   mConfigVars;
		std::vector< std::string > mIncludeFiles;
	};

	class ShaderCompiler
	{
	public:
		bool compileCode( RHIShader::Type type , RHIShader& shader , char const* path, char const* def = nullptr );

		bool bRecompile = true;
		bool bUsePreprocess = true;

	};

	class ShaderManager : public SingletonT< ShaderManager >
	{
	public:

		~ShaderManager();
		bool loadFileSimple(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
					  char const* def = nullptr, char const* additionalCode = nullptr );

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  uint8 shaderMask, char const* entryNames[], 
					  char const* def = nullptr, char const* additionalCode = nullptr);

		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, 
					  char const* vertexEntryName, char const* pixelEntryName, 
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
		bool loadInternal(ShaderProgram& shaderProgram, char const* fileName, uint8 shaderMask, char const* entryNames[], ShaderCompileOption const& option, char const* additionalCode, bool bSingle);

		struct ShaderCache
		{
			RHIShaderRef shader;
			std::string filePath;
			std::string define;
		};

		struct ShaderCompileInfo;
		bool updateShaderInternal(ShaderProgram& shaderProgram, ShaderCompileInfo& info);

		struct ShaderCompileInfo : public AssetBase
		{
			ShaderProgram* shaderProgram;
			uint8        shaderMask;
			std::string  fileName;
			std::vector< std::string >  headCodes;
			bool         bSingle;

			
		protected:
			virtual void getDependentFilePaths(std::vector<std::wstring>& paths) override;
			virtual void postFileModify(FileAction action) override;
		};

		uint32         mDefaultVersion = 430;

		ShaderCompiler mCompiler;
		std::unordered_map< ShaderProgram*, ShaderCompileInfo* > mShaderCompileMap;
	};

}


#endif // ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2