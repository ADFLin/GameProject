#pragma once
#ifndef ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2
#define ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2

#include "GLCommon.h"
#include "Singleton.h"

#include <unordered_map>

namespace GL
{
	enum ShaderVersion
	{



	};



	class ShaderCompiler
	{
	public:
		bool compileSource( RHIShader::Type type , RHIShader& shader , char const* path, char const* def = nullptr );

		std::string getConfigDefine();

		struct ConfigVar
		{
			std::string name;
			std::string value;
		};


		bool bRecompile = true;
		bool bUsePreprocess = true;
		std::vector< ConfigVar > mConfigVars;
	};

	class ShaderManager : public SingletonT< ShaderManager >
	{
	public:

		~ShaderManager();
		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr);
		bool loadFile(ShaderProgram& shaderProgram, char const* fileName, char const* vertexEntryName, char const* pixelEntryName, char const* def = nullptr, int version = -1);

		bool loadMultiFile(ShaderProgram& shaderProgram, char const* fileName, char const* def = nullptr, int version = -1);
		bool reloadShader(ShaderProgram& shaderProgram);

		struct ShaderCache
		{
			RHIShaderRef shader;
			std::string filePath;
			std::string define;
		};


		struct ShaderCompileInfo
		{
			std::string  fileName;
			std::string  defines[2];
			bool         bSingle;
		};

		bool updateShaderInternal(ShaderProgram& shaderProgram, ShaderCompileInfo& info);
		bool updateShaderMultiInternal(ShaderProgram& shaderProgram, ShaderCompileInfo& info);

		ShaderCompiler mCompiler;
		std::unordered_map< ShaderProgram*, ShaderCompileInfo* > mShaderCompileMap;
	};

}


#endif // ShaderCompiler_H_7817AD1B_28BB_41DC_B037_0D75389E42A2