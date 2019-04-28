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

		bool    updateShader(bool bNoLink = false);
		void    bind();
		void    unbind();
		void    generateParameterMap(ShaderParameterMap& parameterMap);
		int     getParamLoc(char const* name) { return glGetUniformLocation(getHandle(), name); }	
		virtual bool setupShaders(RHIShaderRef shaders[], int numShader);
		virtual bool getParameter(char const* name, ShaderParameter& parameter) override;
		virtual bool getResourceParameter(EShaderResourceType type, char const* name, ShaderParameter& parameter) override;

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

		virtual bool isSupportBinaryCode() const final;
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) final;
		virtual bool getBinaryCode(RHIShaderProgram& shaderProgram, std::vector<uint8>& outBinaryCode) final;
		virtual bool setupProgram(RHIShaderProgram& shaderProgram, std::vector<uint8> const& binaryCode) final;
		virtual void setupParameters(ShaderProgram& shaderProgram) override final;
		virtual bool compileCode(Shader::Type type, RHIShader& shader, char const* path, ShaderCompileInfo* compileInfo, char const* def) final;


		uint32  mDefaultVersion;
	};

}//namespace Render




#endif // OpenGLShaderCommon_H_2F05B361_75CB_443E_95D7_E1E52E6E67A8