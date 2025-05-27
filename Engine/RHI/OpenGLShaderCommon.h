#pragma once
#ifndef OpenGLShaderCommon_H_2F05B361_75CB_443E_95D7_E1E52E6E67A8
#define OpenGLShaderCommon_H_2F05B361_75CB_443E_95D7_E1E52E6E67A8

#include "ShaderCore.h"
#include "ShaderFormat.h"

#include "OpenGLCommon.h"

namespace Render
{
	namespace GLFactory
	{
		struct Shader
		{
			static void Create(GLuint& handle, GLenum type) { handle = glCreateShader(type); }
			static void Destroy(GLuint& handle) { glDeleteShader(handle); }
		};

		struct ShaderProgram
		{
			static void Create(GLuint& handle) { handle = glCreateProgram(); }
			static void Destroy(GLuint& handle) { glDeleteProgram(handle); }
		};
	}

	class OpenGLShaderObject : public TOpenGLObject< GLFactory::Shader >
	{
	public:

		bool compile(EShader::Type type, char const* src[], int num);
		bool loadFile(EShader::Type type, char const* path, char const* def);

		GLint getIntParameter(GLuint val);

	};

	struct FOpenGLShader
	{
		static bool CheckProgramStatus( GLuint handle , GLenum statusName, char const* errorTitle);
		
		static  bool GetParameter(GLuint handle, char const* name, ShaderParameter& outParameter);

		static bool GetResourceParameter(GLuint handle, EShaderResourceType type, char const* name, ShaderParameter& outParameter);

		static void GenerateParameterMap(GLuint handle, ShaderParameterMap& parameterMap);

		static bool GetProgramBinary(GLuint handle, TArray<uint8>& outBinary);

		static int GetLogInfo(GLuint handle, TArray<char>& outBuffer);
	};

	class OpenGLShader : public TOpenGLResource< RHIShader , GLFactory::ShaderProgram >
	{
	public:
		bool create(EShader::Type type);
		bool attach(OpenGLShaderObject& shaderObject);

		void bind();
		void unbind();

		virtual bool  getParameter(char const* name, ShaderParameter& outParameter)
		{
			return FOpenGLShader::GetParameter(getHandle(), name, outParameter);
		}

		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParameter) final
		{ 
			return FOpenGLShader::GetResourceParameter(getHandle(), resourceType, name, outParameter);
		}

		virtual bool getResourceParameter(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo, ShaderParameter& outParam) final
		{
			return getResourceParameter(resourceType, structInfo.blockName, outParam);
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


	class OpenGLShaderProgram : public TOpenGLResource< RHIShaderProgram, GLFactory::ShaderProgram >
	{
	public:
		bool    create();

		void    attach(EShader::Type type, OpenGLShaderObject const& shaderObject);
		bool    initialize();
		bool    initialize( TArray<uint8> const& binaryCode);

		bool    updateShader(bool bLinkShader = true);
		
		void    bind();
		void    unbind();


		virtual bool  getParameter(char const* name, ShaderParameter& outParameter)
		{
			return FOpenGLShader::GetParameter(getHandle(), name, outParameter);
		}

		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParameter) final
		{
			return FOpenGLShader::GetResourceParameter(getHandle(), resourceType, name, outParameter);
		}
		virtual bool getResourceParameter(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo, ShaderParameter& outParam) final
		{
			return getResourceParameter(resourceType, structInfo.blockName, outParam);
		}

		void    generateParameterMap(ShaderParameterMap& parameterMap)
		{
			FOpenGLShader::GenerateParameterMap(getHandle(), parameterMap);
		}
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
		virtual EShaderCompileResult compileCode(ShaderCompileContext const& context);

		virtual void precompileCode(ShaderProgramSetupData& setupData) final;
		virtual ShaderParameterMap* initializeProgram(RHIShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) final;
		virtual ShaderParameterMap* initializeProgram(RHIShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& descList, TArray<uint8> const& binaryCode) final;
		virtual void postShaderLoaded(ShaderProgram& shaderProgram) final;

		virtual void precompileCode(ShaderSetupData& setupData) final;
		virtual ShaderParameterMap* initializeShader(RHIShader& shader, ShaderSetupData& setupData) override;
		virtual ShaderParameterMap* initializeShader(RHIShader& shader, ShaderCompileDesc const& desc, TArray<uint8> const& binaryCode) override;
		virtual void postShaderLoaded(RHIShader& shader) final;

		virtual bool doesSuppurtBinaryCode() const final;
		virtual bool getBinaryCode(ShaderProgramSetupData& setupData, TArray<uint8>& outBinaryCode) final;
		virtual bool getBinaryCode(ShaderSetupData& setupData, TArray<uint8>& outBinaryCode) override;

		virtual ShaderPreprocessSettings getPreprocessSettings();
		virtual bool isMultiCodesCompileSupported() const { return true; }

		ShaderParameterMap mTempParameterMap;
		uint32  mDefaultVersion;

	};

}//namespace Render




#endif // OpenGLShaderCommon_H_2F05B361_75CB_443E_95D7_E1E52E6E67A8