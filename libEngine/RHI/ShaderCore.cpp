#include "ShaderCore.h"

#include "FileSystem.h"

namespace RenderGL
{
	bool RHIShader::loadFile(Shader::Type type, char const* path, char const* def)
	{
		std::vector< char > codeBuffer;
		if( !FileUtility::LoadToBuffer(path, codeBuffer, true) )
			return false;
		int num = 0;
		char const* src[2];
		if( def )
		{
			src[num] = def;
			++num;
		}
		src[num] = &codeBuffer[0];
		++num;

		bool result = compileCode(type, src, num);
		return result;

	}

	Shader::Type RHIShader::getType()
	{
		if( mHandle )
		{
			switch( getGLParam(GL_SHADER_TYPE) )
			{
			case GL_VERTEX_SHADER:   return Shader::eVertex;
			case GL_FRAGMENT_SHADER: return Shader::ePixel;
			case GL_GEOMETRY_SHADER: return Shader::eGeometry;
			case GL_COMPUTE_SHADER:  return Shader::eCompute;
			case GL_TESS_CONTROL_SHADER: return Shader::eHull;
			case GL_TESS_EVALUATION_SHADER: return Shader::eDomain;
			}
		}
		return Shader::eEmpty;
	}

	bool RHIShader::compileCode(Shader::Type type, char const* src[], int num)
	{
		if( !create(type) )
			return false;

		glShaderSource(mHandle, num, src, 0);
		glCompileShader(mHandle);

		if( getGLParam(GL_COMPILE_STATUS) == GL_FALSE )
		{
			return false;
		}
		return true;
	}

	bool RHIShader::create(Shader::Type type)
	{
		if( mHandle )
		{
			if( getType() == type )
				return true;
			destroy();
		}

		mHandle = glCreateShader(GLConvert::To(type));
		return mHandle != 0;
	}

	void RHIShader::destroy()
	{
		if( mHandle )
		{
			glDeleteShader(mHandle);
			mHandle = 0;
		}
	}

	GLuint RHIShader::getGLParam(GLuint val)
	{
		GLint status;
		glGetShaderiv(mHandle, val, &status);
		return status;
	}


	ShaderProgram::ShaderProgram()
	{
		mNeedLink = true;
	}

	ShaderProgram::~ShaderProgram()
	{

	}

	bool ShaderProgram::create()
	{
		return fetchHandle();
	}

	RHIShader* ShaderProgram::removeShader(Shader::Type type)
	{
		RHIShader* out = mShaders[type];
		if( mShaders[type] )
		{
			glDetachShader(mHandle, mShaders[type]->mHandle);
			mShaders[type] = NULL;
			mNeedLink = true;
		}
		return out;
	}

	void ShaderProgram::attachShader(RHIShader& shader)
	{
		Shader::Type type = shader.getType();
		if( mShaders[type] )
			glDetachShader(mHandle, mShaders[type]->mHandle);
		glAttachShader(mHandle, shader.mHandle);
		mShaders[type] = &shader;
		mNeedLink = true;
	}

	
	bool ShaderProgram::updateShader(bool bForce)
	{
		if( mNeedLink || bForce )
		{
			GLchar buffer[4096 * 32];

			glLinkProgram(mHandle);

			GLint value;
			glGetProgramiv(mHandle, GL_LINK_STATUS, &value);
			if( value != GL_TRUE )
			{
				GLsizei size;
				glGetProgramInfoLog(mHandle, ARRAY_SIZE(buffer), &size, buffer);
				LogMsg("Can't Link Program : %s", buffer);
				return false;
			}

			glValidateProgram(mHandle);
			glGetProgramiv(mHandle, GL_VALIDATE_STATUS, &value);
			if( value != GL_TRUE )
			{
				GLsizei size;
				glGetProgramInfoLog(mHandle, ARRAY_SIZE(buffer), &size, buffer);
				LogMsg("Can't Link Program : %s", buffer);
			}
			mNeedLink = false;
		}

		bindParameters();
		return true;
	}

	void ShaderProgram::bind()
	{
		updateShader();
		glUseProgram(mHandle);
		resetBindIndex();
	}

	void ShaderProgram::unbind()
	{
		glUseProgram(0);
	}

	void ShaderProgram::setBuffer(ShaderBufferParameter const& param, RHIUniformBuffer& buffer)
	{
		glUniformBlockBinding(mHandle, param.mIndex, mNextUniformSlot);
		glBindBufferBase(GL_UNIFORM_BUFFER, mNextUniformSlot, buffer.mHandle);
		++mNextUniformSlot;
	}

	void ShaderProgram::setBuffer(ShaderBufferParameter const& param, RHIStorageBuffer& buffer)
	{
		glShaderStorageBlockBinding(mHandle, param.mIndex, mNextStorageSlot);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mNextStorageSlot, buffer.mHandle);
		++mNextStorageSlot;
	}

	bool ShaderParameter::bind(ShaderProgram& program, char const* name)
	{
		mLoc = program.getParamLoc(name);
		return isBound();
	}

	bool ShaderBufferParameter::bindUniform(ShaderProgram& program, char const* name)
	{
		mIndex = glGetUniformBlockIndex(program.mHandle, name);
		return isBound();
	}

	bool ShaderBufferParameter::bindStorage(ShaderProgram& program, char const* name)
	{
		mIndex = glGetProgramResourceIndex(program.mHandle, GL_SHADER_STORAGE_BLOCK , name);
		return isBound();
	}

}//namespace RenderGL