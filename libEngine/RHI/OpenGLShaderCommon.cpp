#include "OpenGLShaderCommon.h"

#include "ShaderCore.h"
#include "ShaderProgram.h"

#include <fstream>
#include <sstream>
#include "FileSystem.h"
#include "Core\StringConv.h"
#include "CString.h"

#include "RHICommand.h"

namespace Render
{

	OpenGLShaderProgram::OpenGLShaderProgram()
	{

	}

	OpenGLShaderProgram::~OpenGLShaderProgram()
	{

	}

	bool OpenGLShaderProgram::create()
	{
		if( !mGLObject.fetchHandle() )
			return false;

		return true;
	}


	bool OpenGLShaderProgram::updateShader( bool bLinkShader )
	{
		uint32 handle = getHandle();

		auto CheckStatus = [handle](GLenum statusName, char const* errorTitle) -> bool
		{
			GLchar buffer[4096 * 32];
			GLint value = 0;
			glGetProgramiv(handle, statusName, &value);
			if (value != GL_TRUE)
			{
				GLsizei size = 0;
				glGetProgramInfoLog(handle, ARRAY_SIZE(buffer), &size, buffer);
				LogMsg("%s : %s", errorTitle , buffer);
				return false;
			}
			return true;
		};
		if(bLinkShader)
		{
			glLinkProgram(handle);
			if (!CheckStatus(GL_LINK_STATUS, "Can't Link Program"))
				return false;
		}

		glValidateProgram(handle);
		if (!CheckStatus(GL_VALIDATE_STATUS, "Can't Validate Program"))
			return false;

		bValid = true;
		return true;
	}

	bool OpenGLShaderProgram::setupShaders(ShaderResourceInfo shaders[], int numShaders)
	{
		auto DetachAllShader =  [this]()
		{
			GLuint shaders[Shader::Count];
			GLsizei numShaders = 0;
			glGetAttachedShaders(getHandle(), ARRAY_SIZE(shaders), &numShaders, shaders);
			for (int i = 0; i < numShaders; ++i)
			{
				glDetachShader(getHandle(), shaders[i]);
			}
		};

		for( int i = 0; i < numShaders; ++i )
		{
			assert(shaders[i].formatData);
			glAttachShader(getHandle(), static_cast<OpenGLShaderObject*>(shaders[i].formatData)->mHandle );
		}

		bool result = updateShader(true);

		DetachAllShader();

		return result;
	}

	bool OpenGLShaderProgram::getParameter(char const* name, ShaderParameter& parameter)
	{
		int loc = getParamLoc(name);
		if( loc == -1 )
			return false;
		parameter.mLoc = loc;
		return true;
	}

	bool OpenGLShaderProgram::getResourceParameter(EShaderResourceType type, char const* name, ShaderParameter& parameter)
	{
		int index = -1;
		switch( type )
		{
		case EShaderResourceType::Uniform:
			index = glGetProgramResourceIndex(getHandle(), GL_UNIFORM_BLOCK, name);
			break;
		case EShaderResourceType::Storage:
			index = glGetProgramResourceIndex(getHandle(), GL_SHADER_STORAGE_BLOCK, name);
			break;
		case EShaderResourceType::AtomicCounter:
			{
				int indexTemp = glGetProgramResourceIndex(getHandle(), GL_UNIFORM, name);
				if( indexTemp != -1 )
				{
					GLint loc = -1;
					const GLenum properties[] = { GL_ATOMIC_COUNTER_BUFFER_INDEX };
					glGetProgramResourceiv(getHandle(), GL_UNIFORM, indexTemp, ARRAY_SIZE(properties), properties, ARRAY_SIZE(properties), NULL, &loc);
					index = loc;
				}
			}
			break;
		}
		if( index == -1 )
			return false;
		parameter.mLoc = index;
		return true;
	}



	void OpenGLShaderProgram::generateParameterMap(ShaderParameterMap& parameterMap)
	{
		uint32 handle = getHandle();

		auto GetBlockParameterFun = [&](GLenum BlockTypeInterface)
		{
			GLint numBlocks = 0;
			glGetProgramInterfaceiv(handle, BlockTypeInterface, GL_ACTIVE_RESOURCES, &numBlocks);
			for( int idxBlock = 0; idxBlock < numBlocks; ++idxBlock )
			{
				GLint values[1] = { 0 };
				const GLenum blockProperties[] = { GL_NAME_LENGTH };
				glGetProgramResourceiv(handle, BlockTypeInterface, idxBlock, ARRAY_SIZE(blockProperties), blockProperties, ARRAY_SIZE(blockProperties), NULL, values);

				char name[1024];
				assert(values[0] < ARRAY_SIZE(name));
				glGetProgramResourceName(handle, BlockTypeInterface, idxBlock, ARRAY_SIZE(name), NULL, &name[0]);
				parameterMap.addParameter(name, idxBlock);
			}
		};

		GetBlockParameterFun(GL_UNIFORM_BLOCK);
		GetBlockParameterFun(GL_SHADER_STORAGE_BLOCK);

#if 0

		GLint numParam = 0;
		glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &numParam);
		for( int i = 0; i < numParam; ++i )
		{
			int name_len = -1, num = -1;
			GLenum type = GL_ZERO;
			char name[100];
			glGetActiveUniform(handle, GLuint(i), sizeof(name) - 1,
							   &name_len, &num, &type, name);

			GLuint location = glGetUniformLocation(handle, name);

			if( location == -1 )
				continue;

			parameterMap.addParameter(name, location);

		}
#else

		GLint numParam = 0;
		glGetProgramInterfaceiv(handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numParam);
		for( int paramIndex = 0; paramIndex < numParam; ++paramIndex )
		{
			GLint values[5] = { 0,0 ,0 ,0,0 };
			const GLenum properties[] = { GL_NAME_LENGTH , GL_LOCATION , GL_ATOMIC_COUNTER_BUFFER_INDEX , GL_OFFSET , GL_BLOCK_INDEX };
			glGetProgramResourceiv(handle, GL_UNIFORM, paramIndex, ARRAY_SIZE(properties), properties, ARRAY_SIZE(properties), NULL, values);

			char name[1024];
			assert(values[0] < ARRAY_SIZE(name));
			glGetProgramResourceName(handle, GL_UNIFORM, paramIndex, ARRAY_SIZE(name), NULL, &name[0]);

			char const* pFind = FCString::FindChar(name, '[');
			if (*pFind)
			{
				*(char*)pFind = 0;
			}

			if( values[1] == -1 )
			{
				if( values[2] != -1 )
				{
					parameterMap.addParameter(name, values[2]);
				}
			}
			else
			{
				parameterMap.addParameter(name, values[1]);
			}
		}
#endif
	}

	void OpenGLShaderProgram::bind()
	{
		if( getHandle() )
		{
			glUseProgram(getHandle());
		}
	}

	void OpenGLShaderProgram::unbind()
	{
		glUseProgram(0);
	}


	Shader::Type OpenGLShader::getType()
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


	bool OpenGLShader::loadFile(Shader::Type type, char const* path, char const* def)
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

	bool OpenGLShader::compileCode(Shader::Type type, char const* src[], int num)
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

	bool OpenGLShader::create(Shader::Type type)
	{
		if( getHandle() )
		{
			if( getType() == type )
				return true;
			if (!mGLObject.destroyHandle())
			{

			}
		}
		return mGLObject.fetchHandle(OpenGLTranslate::To(type));
	}

	GLuint OpenGLShader::getGLParam(GLuint val)
	{
		GLint status;
		glGetShaderiv(getHandle(), val, &status);
		return status;
	}


	static bool GetProgramBinary(GLuint handle, std::vector<uint8>& outBinary)
	{
		//glProgramParameteri(handle, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

		GLint binaryLength = -1;
		glGetProgramiv(handle, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
		if( binaryLength <= 0 )
		{

			return false;
		}

		outBinary.resize(binaryLength + sizeof(GLenum));
		uint8* pData = outBinary.data();
		// BinaryFormat is stored at the start of ProgramBinary array
		glGetProgramBinary(handle, binaryLength, &binaryLength, (GLenum*)pData, pData + sizeof(GLenum));
		if( !VerifyOpenGLStatus() )
		{

			return false;
		}
		return true;
	}


	void ShaderFormatGLSL::setupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addMeta("ShaderFormat", getName());
	}

	void ShaderFormatGLSL::getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
	{
		inoutCode += "#version ";
		char const* versionString = option.getMeta("GLSLVersion");
		if( versionString )
		{
			inoutCode += option.getMeta("GLSLVersion");
		}
		else
		{
			inoutCode += FStringConv::From(mDefaultVersion);
		}
		inoutCode += " compatibility\n";

		inoutCode += "#define COMPILER_GLSL 1\n";

		if( entry.name && FCString::Compare(entry.name , "main") != 0 )
		{
			inoutCode += "#define ";
			inoutCode += entry.name;
			inoutCode += " main\n";
		}
	}

	class GLSLCompileIntermediates : public ShaderCompileIntermediates
	{
	public:
		~GLSLCompileIntermediates()
		{
			for (int i = 0; i < numShaders; ++i)
			{
				shaders[i].release();
			}
		}
		OpenGLShaderObject shaders[Shader::MaxStorageSize];
		int numShaders = 0;
	};

	bool ShaderFormatGLSL::compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output)
	{
		bool bSuccess;
		do
		{
			bSuccess = false;
			std::vector< char > codeBuffer;
			if( !FileUtility::LoadToBuffer(input.path, codeBuffer, true) )
				return false;
			int numSourceCodes = 0;
			char const* sourceCodes[2];

			if( bUsePreprocess )
			{
				if (!PreprocessCode(input.path, output.compileInfo, input.definition, codeBuffer))
					return false;

				sourceCodes[numSourceCodes] = &codeBuffer[0];
				++numSourceCodes;
			}
			else
			{
				if(input.definition)
				{
					sourceCodes[numSourceCodes] = input.definition;
					++numSourceCodes;
				}
				sourceCodes[numSourceCodes] = &codeBuffer[0];
				++numSourceCodes;
			}

			auto ProcessCompileError = [&]( GLuint shaderHandle )
			{
				{
					std::ofstream of("temp" SHADER_FILE_SUBNAME, std::ios::binary);
					if (of.is_open())
					{
						of.write(&codeBuffer[0], codeBuffer.size());
					}
				}

				int maxLength;
				glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &maxLength);
				std::vector< char > buf(maxLength);
				int logLength = 0;
				glGetShaderInfoLog(shaderHandle, maxLength, &logLength, &buf[0]);
				OutputError(buf.data());
			};

			if (input.setupData)
			{
				GLSLCompileIntermediates* myIntermediates = static_cast<GLSLCompileIntermediates*>(input.setupData->intermediateData.get());

				OpenGLShaderObject& shaderObject = myIntermediates->shaders[myIntermediates->numShaders];

				bSuccess = shaderObject.create(input.type, sourceCodes, numSourceCodes);

				if (bSuccess)
				{
					output.formatData = &shaderObject;
					++myIntermediates->numShaders;
				}
				else
				{
					ProcessCompileError(shaderObject.mHandle);
					shaderObject.release();
				}
			}
			else
			{
				auto* shaderImpl = static_cast<OpenGLShader*>(RHICreateShader(input.type));
				output.resource = shaderImpl;
				bSuccess = shaderImpl->compileCode(input.type, sourceCodes, numSourceCodes);

				if (!bSuccess && bUsePreprocess)
				{
					ProcessCompileError(shaderImpl->getHandle());
				}
			}

		} while( !bSuccess && bRecompile );

		return bSuccess;
	}


	void ShaderFormatGLSL::precompileCode(ShaderProgramSetupData& setupData)
	{
		setupData.intermediateData = std::make_unique<GLSLCompileIntermediates>();
	}

	bool ShaderFormatGLSL::initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(*shaderProgram.mRHIResource);

		if (!shaderProgramImpl.setupShaders(setupData.shaders.data(), setupData.shaders.size()))
			return false;

		ShaderParameterMap parameterMap;
		shaderProgramImpl.generateParameterMap(parameterMap);
		shaderProgram.bindParameters(parameterMap);
		return true;
	}

	bool ShaderFormatGLSL::initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode)
	{
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(*shaderProgram.mRHIResource);

		GLenum format = *(GLenum*)binaryCode.data();
		glProgramBinary(shaderProgramImpl.getHandle(), format, binaryCode.data() + sizeof(GLenum), binaryCode.size() - sizeof(GLenum));
		if (!VerifyOpenGLStatus())
		{
			return false;
		}
		if (!shaderProgramImpl.updateShader(false))
		{
			return false;
		}

		ShaderParameterMap parameterMap;
		shaderProgramImpl.generateParameterMap(parameterMap);
		shaderProgram.bindParameters(parameterMap);
		return true;
	}

	bool ShaderFormatGLSL::getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(*shaderProgram.mRHIResource);
		return GetProgramBinary(shaderProgramImpl.getHandle(), outBinaryCode);
	}

	void ShaderFormatGLSL::postShaderLoaded(ShaderProgram& shaderProgram)
	{
	
	}

	bool ShaderFormatGLSL::isSupportBinaryCode() const
	{
		int numFormat = 0;
		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormat);
		return numFormat != 0;
	}

}