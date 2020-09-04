#pragma once
#ifndef OpenGLShaderCommon_H_2F05B361_75CB_443E_95D7_E1E52E6E67A8
#define OpenGLShaderCommon_H_2F05B361_75CB_443E_95D7_E1E52E6E67A8

#include "ShaderCore.h"
#include "ShaderFormat.h"

#include "OpenGLCommon.h"

namespace Render
{
	struct RMPShader
	{
		static void Create(GLuint& handle, GLenum type) { handle = glCreateShader(type); }
		static void Destroy(GLuint& handle) { glDeleteShader(handle); }
	};

	class OpenGLShaderObject : public TOpenGLObject< RMPShader >
	{
	public:

		bool compile(EShader::Type type, char const* src[], int num);
		bool loadFile(EShader::Type type, char const* path, char const* def);

		GLuint getIntParam(GLuint val);
	};

	struct FOpenGLShader
	{
		static bool CheckProgramStatus( GLuint handle , GLenum statusName, char const* errorTitle);
		
		static  bool GetParameter(GLuint handle, char const* name, ShaderParameter& outParameter);

		static bool GetResourceParameter(GLuint handle, EShaderResourceType type, char const* name, ShaderParameter& outParameter);

		static void GenerateParameterMap(GLuint handle, ShaderParameterMap& parameterMap);

		static bool GetProgramBinary(GLuint handle, std::vector<uint8>& outBinary);
	};


	struct RMPShaderProgram
	{
		static void Create(GLuint& handle) { handle = glCreateProgram(); }
		static void Destroy(GLuint& handle) { glDeleteProgram(handle); }
	};


	class OpenGLShader : public TOpenGLResource< RHIShader , RMPShaderProgram >
	{
	public:
		bool create(EShader::Type type);
		bool attach(OpenGLShaderObject& shaderObject);

		void bind();
		void unbind();

		EShader::Type mType = EShader::Empty;
		EShader::Type getType() const { return mType; }
		virtual bool  getParameter(char const* name, ShaderParameter& outParameter)
		{
			return FOpenGLShader::GetParameter(getHandle(), name, outParameter);
		}

		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParameter) 
		{ 
			return FOpenGLShader::GetResourceParameter(getHandle(), resourceType, name, outParameter);
		}

		void    generateParameterMap(ShaderParameterMap& parameterMap)
		{
			FOpenGLShader::GenerateParameterMap(getHandle(), parameterMap);
		}


		bool validate()
		{
			uint32 handle = getHandle();

			glValidateProgram(handle);
			if (!FOpenGLShader::CheckProgramStatus(handle, GL_VALIDATE_STATUS, "Can't Validate Program"))
				return false;

			return true;
		}
	};


	class OpenGLShaderProgram : public TOpenGLResource< RHIShaderProgram, RMPShaderProgram >
	{
	public:
		bool    create();

		bool    updateShader(bool bLinkShader = true);
		void    bind();
		void    unbind();
		virtual bool setupShaders(ShaderResourceInfo shaders[], int numShaders);

		virtual bool  getParameter(char const* name, ShaderParameter& outParameter)
		{
			return FOpenGLShader::GetParameter(getHandle(), name, outParameter);
		}

		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParameter)
		{
			return FOpenGLShader::GetResourceParameter(getHandle(), resourceType, name, outParameter);
		}

		void    generateParameterMap(ShaderParameterMap& parameterMap)
		{
			FOpenGLShader::GenerateParameterMap(getHandle(), parameterMap);
		}
		uint32 mShaderMask;
	};

	class ShaderFormatGLSL : public ShaderFormat
	{
	public:
		ShaderFormatGLSL()
		{
			mDefaultVersion = 430;
		}
		virtual char const* getName() final { return "glsl"; }
		virtual void setupShaderCompileOption(ShaderCompileOption& option) final;
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) final;
		virtual bool compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output) final;

		virtual void precompileCode(ShaderProgramSetupData& setupData) final;
		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) final;
		virtual bool initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode) final;
		virtual void postShaderLoaded(ShaderProgram& shaderProgram) final;

		virtual void precompileCode(ShaderSetupData& setupData) final;
		virtual bool initializeShader(Shader& shader, ShaderSetupData& setupData) override;
		virtual bool initializeShader(Shader& shader, ShaderCompileInfo const& shaderCompile, std::vector<uint8> const& binaryCode) override;
		virtual void postShaderLoaded(Shader& shader) final;

		virtual bool isSupportBinaryCode() const final;
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode) final;
		virtual bool getBinaryCode(Shader& shader, ShaderSetupData& setupData, std::vector<uint8>& outBinaryCode) override;

		uint32  mDefaultVersion;

	};

}//namespace Render




#endif // OpenGLShaderCommon_H_2F05B361_75CB_443E_95D7_E1E52E6E67A8