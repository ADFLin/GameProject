#pragma once
#ifndef ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6
#define ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6

#include "ShaderCore.h"

#include "HashString.h"

#include <string>
#include <unordered_set>

#define SHADER_FILE_SUBNAME ".sgc"

namespace CPP
{
	class CodeSourceLibrary;
}

namespace Render
{
	class Shader;
	class ShaderProgram;
	class ShaderManagedData;
	class ShaderProgramManagedData;

	enum ShaderFreature
	{



	};

	struct ShaderCompileDesc
	{
		HashString   filePath;
		EShader::Type type;
		
		std::string  headCode;
		std::string  entryName;

		template< class S1 , class S2 >
		ShaderCompileDesc(EShader::Type inType, HashString inFilePath, S1&& inCode , S2&& inEntryName )
			:filePath(inFilePath), type(inType)
			,headCode(std::forward<S1>(inCode))
			,entryName(std::forward<S2>(inEntryName))
		{
		}

		ShaderCompileDesc() {}
	};

	class ShaderCompileIntermediates
	{
	public:
		virtual ~ShaderCompileIntermediates() {}
	};

	struct ShaderResourceInfo
	{
		EShader::Type type;
		void*        formatData;
	};


	struct ShaderProgramSetupData
	{
		RHIShaderProgramRef       resource;
		ShaderProgramManagedData* managedData;

		int getShaderCount() const;

		std::unique_ptr<ShaderCompileIntermediates>  intermediateData;
	};

	struct ShaderSetupData
	{
		RHIShaderRef         resource;
		ShaderManagedData*   managedData;

		std::unique_ptr<ShaderCompileIntermediates>  intermediateData;
		ShaderResourceInfo   shaderResource;
	};

	struct ShaderCompileContext
	{
		bool bOuputPreprocessedCode = true;
		bool bUsePreprocess = true;
		bool bRecompile = true;
		int  shaderIndex;
		ShaderCompileDesc* desc;

		bool haveFile() const { return desc->filePath.empty() == false; }
		EShader::Type getType() const { return desc->type; }
		char const* getPath() const { return desc->filePath.c_str(); }
		char const* getEntry() const {  return desc->entryName.c_str();  }
		StringView  getDefinition() const { return desc->headCode; }
		
		CPP::CodeSourceLibrary* sourceLibrary;

		ShaderProgramSetupData* programSetupData;
		ShaderSetupData* shaderSetupData;

		ShaderCompileContext()
		{
			programSetupData = nullptr;
			shaderSetupData = nullptr;
			sourceLibrary = nullptr;
		}
	};

	struct ShaderPreprocessSettings
	{
		bool bSupportLineFilePath;


		ShaderPreprocessSettings()
		{
			bSupportLineFilePath = true;
		}
	};
	class ShaderFormat
	{
	public:

		virtual char const* getName() = 0;

		virtual void setupShaderCompileOption(ShaderCompileOption& context) = 0;
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) = 0;

		virtual bool compileCode(ShaderCompileContext const& context) = 0;
		virtual void precompileCode(ShaderProgramSetupData& setupData){}
		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) = 0;
		virtual bool initializeProgram(ShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& descList, TArray<uint8> const& binaryCode) = 0;
		virtual void postShaderLoaded(ShaderProgram& shaderProgram){}
		
		virtual void precompileCode(ShaderSetupData& setupData) {}
		virtual bool initializeShader(Shader& shader, ShaderSetupData& setupData) { return false; }
		virtual bool initializeShader(Shader& shader, ShaderCompileDesc const& desc, TArray<uint8> const& binaryCode) { return false; }
		virtual void postShaderLoaded(Shader& shader) {}

		virtual bool doesSuppurtBinaryCode() const { return false; }
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, TArray<uint8>& outBinaryCode) = 0;
		virtual bool getBinaryCode(Shader& shader, ShaderSetupData& setupData, TArray<uint8>& outBinaryCode)
		{
			return false;
		}
		bool preprocessCode(char const* path, ShaderCompileDesc* compileDesc, StringView const& definition, CPP::CodeSourceLibrary* sourceLibrary, TArray<uint8>& inoutCodes, std::unordered_set<HashString>* outIncludeFiles, bool bOuputPreprocessedCode);
		bool loadCode(ShaderCompileContext const& context, TArray<uint8>& outCodes);

		virtual ShaderPreprocessSettings getPreprocessSettings()
		{
			return ShaderPreprocessSettings();
		}

		void emitCompileError(ShaderCompileContext const& context, char const* errorCode);
		static void OutputError(char const* title, char const* text);
		
	};

}//namespace Render

#endif // ShaderCompiler_H_21634DC3_3CA4_4798_A9AC_58E1E97A2CD6