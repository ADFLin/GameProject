#pragma once
#ifndef ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6
#define ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6

#include "ShaderCore.h"

#include "HashString.h"

#include <vector>

#define SHADER_FILE_SUBNAME ".sgc"

namespace Render
{
	enum ShaderFreature
	{



	};

	struct ShaderCompileInfo
	{
		HashString   filePath;
		Shader::Type type;
		std::string  headCode;
		std::vector< HashString > includeFiles;

		template< class S2 >
		ShaderCompileInfo(Shader::Type inType, HashString inFilePath, S2&& inCode)
			:filePath(inFilePath), type(inType), headCode(std::forward<S2>(inCode))
		{
		}

		ShaderCompileInfo() {}
	};


	class ShaderFormat
	{
	public:
		virtual char const* getName() = 0;
		virtual bool getBinaryCode(RHIShaderProgram& shaderProgram, std::vector<uint8>& outBinaryCode) = 0;
		virtual bool setupProgram(RHIShaderProgram& shaderProgram, std::vector<uint8> const& binaryCode) = 0;

		virtual void setupParameters(ShaderProgram& shaderProgram) = 0;
		virtual bool compileCode(Shader::Type type, RHIShader& shader, char const* path, ShaderCompileInfo* compileInfo, char const* def) = 0;
		
		
		bool bRecompile = true;
		bool bUsePreprocess = true;
	};

}//namespace Render

#endif // ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6