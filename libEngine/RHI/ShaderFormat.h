#pragma once
#ifndef ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6
#define ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6

#include "ShaderCore.h"

#include "HashString.h"

#include <vector>

#define SHADER_FILE_SUBNAME ".sgc"

namespace Render
{
	class ShaderProgramCompileInfo;

	enum ShaderFreature
	{



	};

	struct ShaderCompileInfo
	{
		HashString   filePath;
		Shader::Type type;
		
		std::string  headCode;
		std::vector< HashString > includeFiles;
		std::string  entryName;

		template< class S1 , class S2 >
		ShaderCompileInfo(Shader::Type inType, HashString inFilePath, S1&& inCode , S2&& inEntryName )
			:filePath(inFilePath), type(inType)
			,headCode(std::forward<S1>(inCode))
			,entryName(std::forward<S2>(inEntryName))
		{
		}

		ShaderCompileInfo() {}
	};

	class ShaderCompileIntermediates
	{
	public:
		virtual ~ShaderCompileIntermediates() {}
	};

	struct ShaderCompileInput
	{
		Shader::Type type;
		char const* path;
		char const* definition;
		struct ShaderProgramSetupData* setupData;
	};

	struct ShaderCompileOutput
	{
		RHIShaderRef resource;
		ShaderCompileInfo* compileInfo;
		void*   formatData;
	};

	struct ShaderResourceInfo
	{
		Shader::Type type;
		RHIShaderRef resource;
		char const*  entry;
		void*        formatData;
	};

	struct ShaderProgramSetupData
	{
		ShaderProgramCompileInfo*      compileInfo;
		std::unique_ptr<ShaderCompileIntermediates>  intermediateData;
		std::vector< ShaderResourceInfo > shaders;
	};

	class ShaderFormat
	{
	public:

		virtual char const* getName() = 0;

		virtual void setupShaderCompileOption(ShaderCompileOption& option) = 0;
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) = 0;

		virtual bool compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output) = 0;
		virtual void precompileCode(ShaderProgramSetupData& setupData){}
		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) = 0;
		virtual bool initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode) = 0;

		virtual void postShaderLoaded(ShaderProgram& shaderProgram){}

		virtual bool isSupportBinaryCode() const { return false; }
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode) = 0;

		static bool PreprocessCode(char const* path, ShaderCompileInfo* compileInfo, char const* def, std::vector<char>& inoutCodes );
		static void OutputError(char const* text);
		
		bool bRecompile = true;
		bool bUsePreprocess = true;
	};

}//namespace Render

#endif // ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6