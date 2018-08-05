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
		if( getHandle() )
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

		glShaderSource(getHandle(), num, src, 0);
		glCompileShader(getHandle());

		if( getGLParam(GL_COMPILE_STATUS) == GL_FALSE )
		{
			return false;
		}
		return true;
	}

	bool RHIShader::create(Shader::Type type)
	{
		if( getHandle() )
		{
			if( getType() == type )
				return true;
			mGLObject.destroyHandle();
		}
		return mGLObject.fetchHandle(GLConvert::To(type));
	}

	GLuint RHIShader::getGLParam(GLuint val)
	{
		GLint status;
		glGetShaderiv(getHandle(), val, &status);
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
			glDetachShader(mHandle,  OpenGLCast::GetHandle( mShaders[type] ) );
			mShaders[type] = NULL;
			mNeedLink = true;
		}
		return out;
	}

	void ShaderProgram::attachShader(RHIShader& shader)
	{
		Shader::Type type = shader.getType();
		if( mShaders[type] )
			glDetachShader(mHandle, OpenGLCast::GetHandle(mShaders[type]) );
		glAttachShader(mHandle, OpenGLCast::GetHandle( shader ) );
		mShaders[type] = &shader;
		mNeedLink = true;
	}

	
	bool ShaderProgram::updateShader(bool bForce)
	{
		if( !mNeedLink && !bForce )
			return true;

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

		ShaderParameterMap parameterMap;
		generateParameterMap(parameterMap);
		bindParameters(parameterMap);

		return true;
	}

	void ShaderProgram::generateParameterMap(ShaderParameterMap& parameterMap)
	{
		auto GetBlockParameterFun = [&](GLenum BlockTypeInterface)
		{
			GLint numBlocks = 0;
			glGetProgramInterfaceiv(mHandle, BlockTypeInterface, GL_ACTIVE_RESOURCES, &numBlocks);
			for( int idxBlock = 0; idxBlock < numBlocks; ++idxBlock )
			{
				GLint values[1] = { 0 };
				const GLenum blockProperties[] = { GL_NAME_LENGTH };
				glGetProgramResourceiv(mHandle, BlockTypeInterface, idxBlock, ARRAY_SIZE(blockProperties), blockProperties, ARRAY_SIZE(blockProperties), NULL, values);

				char name[1024];
				assert(values[0] < ARRAY_SIZE(name));
				glGetProgramResourceName(mHandle, BlockTypeInterface, idxBlock, ARRAY_SIZE(name), NULL, &name[0]);
				parameterMap.addParameter(name, idxBlock);
			}
		};

		GetBlockParameterFun(GL_UNIFORM_BLOCK);
		GetBlockParameterFun(GL_SHADER_STORAGE_BLOCK);

#if 0

		GLint numParam = 0;
		glGetProgramiv(mHandle, GL_ACTIVE_UNIFORMS, &numParam);
		for( int i = 0; i < numParam; ++i )
		{
			int name_len = -1, num = -1;
			GLenum type = GL_ZERO;
			char name[100];
			glGetActiveUniform(mHandle, GLuint(i), sizeof(name) - 1,
							   &name_len, &num, &type, name);

			GLuint location = glGetUniformLocation(mHandle, name);

			if( location == -1 )
				continue;

			parameterMap.addParameter(name, location);

		}
#else

		GLint numParam = 0;
		glGetProgramInterfaceiv(mHandle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numParam);
		for( int paramIndex = 0; paramIndex < numParam; ++paramIndex )
		{
			GLint values[4] = { 0,0 ,0 ,0 };
			const GLenum properties[] = { GL_NAME_LENGTH , GL_LOCATION , GL_BLOCK_INDEX , GL_OFFSET };
			glGetProgramResourceiv(mHandle, GL_UNIFORM, paramIndex, ARRAY_SIZE(properties), properties, ARRAY_SIZE(properties), NULL, values);

			char name[1024];
			assert(values[0] < ARRAY_SIZE(name));
			glGetProgramResourceName(mHandle, GL_UNIFORM, paramIndex, ARRAY_SIZE(name), NULL, &name[0]);

			if( values[1] == -1 )
			{
				if( values[3] != 0 )
					continue;

				parameterMap.addParameter(name, values[2]);
			}
			else
			{
				parameterMap.addParameter(name, values[1]);
			}
		}
#endif
	}

	void ShaderProgram::bind()
	{
		if ( mHandle )
		{
			updateShader();
			glUseProgram(mHandle);
			resetBindIndex();
		}
	}

	void ShaderProgram::unbind()
	{
		glUseProgram(0);
	}


	void ShaderProgram::setBuffer(ShaderParameter const& param, RHIUniformBuffer& buffer)
	{
		glUniformBlockBinding(mHandle, param.mLoc, mNextUniformSlot);
		glBindBufferBase(GL_UNIFORM_BUFFER, mNextUniformSlot, OpenGLCast::GetHandle(buffer));
		++mNextUniformSlot;
	}

	void ShaderProgram::setBuffer(ShaderParameter const& param, RHIStorageBuffer& buffer)
	{
		glShaderStorageBlockBinding(mHandle, param.mLoc, mNextStorageSlot);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mNextStorageSlot, OpenGLCast::GetHandle(buffer));
		++mNextStorageSlot;
	}

	void ShaderProgram::setBuffer(ShaderParameter const& param, RHIAtomicCounterBuffer& buffer)
	{
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, param.mLoc, OpenGLCast::GetHandle(buffer));
	}

	void ShaderProgram::setBuffer(StructuredBlockInfo const& param, RHIUniformBuffer& buffer)
	{
		glUniformBlockBinding(mHandle, param.index, mNextUniformSlot);
		glBindBufferBase(GL_UNIFORM_BUFFER, mNextUniformSlot, OpenGLCast::GetHandle(buffer));
		++mNextUniformSlot;
	}

	void ShaderProgram::setBuffer(StructuredBlockInfo const& param, RHIStorageBuffer& buffer)
	{
		glShaderStorageBlockBinding(mHandle, param.index, mNextStorageSlot);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, mNextStorageSlot, OpenGLCast::GetHandle(buffer));
		++mNextStorageSlot;
	}

	void ShaderProgram::setBuffer(char const* name, RHIAtomicCounterBuffer& buffer)
	{
		int loc = getParamLoc(name);
		if( loc != -1 )
		{
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, loc, OpenGLCast::GetHandle(buffer));
		}
	}

}//namespace RenderGL