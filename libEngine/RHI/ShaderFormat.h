#pragma once
#ifndef ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6
#define ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6

#include "ShaderCore.h"

#include "HashString.h"

#include <vector>
#include <string>

#define SHADER_FILE_SUBNAME ".sgc"

namespace Render
{
	class Shader;
	class ShaderProgram;
	class ShaderManagedData;
	class ShaderProgramManagedData;

	enum ShaderFreature
	{



	};

	struct ShaderCompileInfo
	{
		HashString   filePath;
		EShader::Type type;
		
		std::string  headCode;
		std::vector< HashString > includeFiles;
		std::string  entryName;

		template< class S1 , class S2 >
		ShaderCompileInfo(EShader::Type inType, HashString inFilePath, S1&& inCode , S2&& inEntryName )
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
		EShader::Type type;
		char const* path;
		char const* entry;
		char const* definition;

		struct ShaderProgramSetupData* programSetupData;
		struct ShaderSetupData* shaderSetupData;

		ShaderCompileInput()
		{
			programSetupData = nullptr;
			shaderSetupData = nullptr;
		}
	};

	struct ShaderCompileOutput
	{
		RHIShaderRef resource;
		ShaderCompileInfo* compileInfo;
		void*   formatData;
	};

	struct ShaderResourceInfo
	{
		EShader::Type type;
		RHIShaderRef resource;
		char const*  entry;
		void*        formatData;
	};

	struct ShaderProgramSetupData
	{
		ShaderProgramManagedData*      managedData;
		std::unique_ptr<ShaderCompileIntermediates>  intermediateData;
		std::vector< ShaderResourceInfo > shaderResources;
	};

	struct ShaderSetupData
	{
		ShaderManagedData*   managedData;
		std::unique_ptr<ShaderCompileIntermediates>  intermediateData;
		ShaderResourceInfo   shaderResource;
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
		
		virtual void precompileCode(ShaderSetupData& setupData) {}
		virtual bool initializeShader(Shader& shader, ShaderSetupData& setupData) { return false; }
		virtual bool initializeShader(Shader& shader, ShaderCompileInfo const& shaderCompile, std::vector<uint8> const& binaryCode) { return false; }
		virtual void postShaderLoaded(Shader& shader) {}

		virtual bool isSupportBinaryCode() const { return false; }
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode) = 0;
		virtual bool getBinaryCode(Shader& shader, ShaderSetupData& setupData, std::vector<uint8>& outBinaryCode)
		{
			return false;
		}

		bool preprocessCode(char const* path, ShaderCompileInfo* compileInfo, char const* def, std::vector<char>& inoutCodes );
		static void OutputError(char const* text);
		

		bool bOuputPreprocessedCode = true;
		bool bRecompile = true;
		bool bUsePreprocess = true;
	};

}//namespace Render

#endif // ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6