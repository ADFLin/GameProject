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

	class OpenGLShaderObject
	{
	public:

		~OpenGLShaderObject()
		{
			assert(mHandle == 0);
		}

		bool create(Shader::Type type, char const* src[], int num)
		{
			mHandle = glCreateShader(OpenGLTranslate::To(type));
			if (mHandle == 0)
				return false;

			glShaderSource(mHandle, num, src, 0);
			glCompileShader(mHandle);

			GLint status;
			glGetShaderiv(mHandle, GL_COMPILE_STATUS, &status);
			if (GL_COMPILE_STATUS == GL_FALSE)
				return false;

			return true;
		}

		void release()
		{
			if (mHandle)
			{
				glDeleteShader(mHandle);
				mHandle = 0;
			}

		}

		GLuint mHandle = 0;
	};

	class OpenGLShader : public TOpenGLResource< RHIShader , RMPShader >
	{
	public:
		bool create(Shader::Type type);
		bool loadFile(Shader::Type type, char const* path, char const* def = NULL);
		Shader::Type getType();
		bool compileCode(Shader::Type type, char const* src[], int num);
		

		GLuint getGLParam(GLuint val);
		friend class ShaderProgram;
	};


	struct RMPShaderProgram
	{
		static void Create(GLuint& handle) { handle = glCreateProgram(); }
		static void Destroy(GLuint& handle) { glDeleteProgram(handle); }
	};

	class OpenGLShaderProgram : public TOpenGLResource< RHIShaderProgram, RMPShaderProgram >
	{
	public:
		OpenGLShaderProgram();
		virtual ~OpenGLShaderProgram();

		bool    create();

		bool    updateShader(bool bLinkShader = true);
		void    bind();
		void    unbind();
		void    generateParameterMap(ShaderParameterMap& parameterMap);
		int     getParamLoc(char const* name) { return glGetUniformLocation(getHandle(), name); }	
		virtual bool setupShaders(ShaderResourceInfo shaders[], int numShaders);

		virtual bool getParameter(char const* name, ShaderParameter& parameter) override;
		virtual bool getResourceParameter(EShaderResourceType type, char const* name, ShaderParameter& parameter) override;

		bool bValid = false;
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

		virtual void precompileCode(ShaderProgramSetupData& setupData) override;
		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) final;
		virtual bool initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode) final;
		virtual void postShaderLoaded(ShaderProgram& shaderProgram) override;

		virtual bool isSupportBinaryCode() const final;
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode) final;
	
		uint32  mDefaultVersion;



	

	};

}//namespace Render




#endif // OpenGLShaderCommon_H_2F05B361_75CB_443E_95D7_E1E52E6E67A8