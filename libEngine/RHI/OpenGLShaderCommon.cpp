#include "OpenGLShaderCommon.h"

#include "ShaderCore.h"
#include "ShaderProgram.h"

#include <fstream>
#include <sstream>
#include "FileSystem.h"
#include "Core\StringConv.h"
#include "CString.h"

#include "RHICommand.h"
#include "ShaderManager.h"
#include "ConsoleSystem.h"

extern CORE_API TConsoleVariable< bool > CVarShaderUseCache;

namespace Render
{

	bool OpenGLShaderObject::compile(EShader::Type type, char const* src[], int num)
	{
		if (!fetchHandle(OpenGLTranslate::To(type)))
			return false;

		glShaderSource(mHandle, num, src, 0);
		glCompileShader(mHandle);

		if (getIntParam(GL_COMPILE_STATUS) == GL_FALSE)
		{
			return false;
		}

		return true;
	}


	bool OpenGLShaderObject::loadFile(EShader::Type type, char const* path, char const* def)
	{
		std::vector< char > codeBuffer;
		if (!FileUtility::LoadToBuffer(path, codeBuffer, true))
			return false;
		int num = 0;
		char const* src[2];
		if (def)
		{
			src[num] = def;
			++num;
		}
		src[num] = &codeBuffer[0];
		++num;

		bool result = compile(type, src, num);
		return result;
	}

	GLuint OpenGLShaderObject::getIntParam(GLuint val)
	{
		GLint status;
		glGetShaderiv(mHandle, val, &status);
		return status;
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

		if(bLinkShader)
		{
			glLinkProgram(handle);
			if (!FOpenGLShader::CheckProgramStatus(handle, GL_LINK_STATUS, "Can't Link Program"))
				return false;
		}

#if 0
		glValidateProgram(handle);
		if (!FOpenGLShader::CheckProgramStatus(handle, GL_VALIDATE_STATUS, "Can't Validate Program"))
			return false;
#endif

		return true;
	}

	bool OpenGLShaderProgram::setupShaders(ShaderResourceInfo shaders[], int numShaders)
	{
		auto DetachAllShader =  [this]()
		{
			GLuint shaders[EShader::Count];
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

		if (CVarShaderUseCache)
		{
			glProgramParameteri(getHandle(), GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
		}

		bool result = updateShader(true);

		DetachAllShader();

		return result;
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

	bool OpenGLShader::create(EShader::Type type)
	{
		if (!mGLObject.fetchHandle())
			return false;

		glProgramParameteri(getHandle(), GL_PROGRAM_SEPARABLE, TRUE);
		mType = type;
		return true;
	}

	bool OpenGLShader::attach(OpenGLShaderObject& shaderObject)
	{
		glAttachShader(getHandle(), shaderObject.mHandle);
		glLinkProgram(getHandle());
		glDetachShader(getHandle(), shaderObject.mHandle);

		if (!FOpenGLShader::CheckProgramStatus(getHandle(), GL_LINK_STATUS, "Can't Link Program"))
			return false;

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
			for (int i = 0; i < shaders.size(); ++i)
			{
				shaders[i].destroyHandle();
			}
		}
		std::vector< OpenGLShaderObject > shaders;
	};

	void ShaderFormatGLSL::precompileCode(ShaderProgramSetupData& setupData)
	{
		auto data = std::make_unique<GLSLCompileIntermediates>();
		data->shaders.reserve(setupData.managedData->compileInfos.size());
		setupData.intermediateData = std::move(data);

	}
	void ShaderFormatGLSL::precompileCode(ShaderSetupData& setupData)
	{
		auto data = std::make_unique<GLSLCompileIntermediates>();
		data->shaders.reserve(1);
		setupData.intermediateData = std::move(data);
	}

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
				bool bOutputCode = true;
				if (bOutputCode)
				{

					FileSystem::CreateDirectory("ShaderOutput");
					std::string outputPath = "ShaderOutput/";
					outputPath += FileUtility::GetBaseFileName(input.path).toCString();
					char const* const ShaderPosfixNames[] =
					{
						"VS" SHADER_FILE_SUBNAME ,
						"PS" SHADER_FILE_SUBNAME ,
						"GS" SHADER_FILE_SUBNAME ,
						"CS" SHADER_FILE_SUBNAME ,
						"HS" SHADER_FILE_SUBNAME ,
						"DS" SHADER_FILE_SUBNAME ,
					};
					outputPath += ShaderPosfixNames[input.type];
					FileUtility::SaveFromBuffer(outputPath.c_str(), &codeBuffer[0], codeBuffer.size() - 1);
				}
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
					FileUtility::SaveFromBuffer("temp" SHADER_FILE_SUBNAME, &codeBuffer[0], codeBuffer.size() - 1);
				}

				if (shaderHandle)
				{
					int maxLength = 0;
					glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &maxLength);
					std::vector< char > buf(maxLength);
					int logLength = 0;
					glGetShaderInfoLog(shaderHandle, maxLength, &logLength, &buf[0]);
					OutputError(buf.data());
				}
			};

			OpenGLShaderObject shaderObject;
			bSuccess = shaderObject.compile(input.type, sourceCodes, numSourceCodes);

			if (bSuccess)
			{

				GLSLCompileIntermediates* myIntermediates;
				if (input.programSetupData)
					myIntermediates = static_cast<GLSLCompileIntermediates*>(input.programSetupData->intermediateData.get());
				else
					myIntermediates = static_cast<GLSLCompileIntermediates*>(input.shaderSetupData->intermediateData.get());

				myIntermediates->shaders.push_back(std::move(shaderObject));
				output.formatData = &myIntermediates->shaders.back();
			}

			if (!bSuccess && bUsePreprocess)
			{
				ProcessCompileError(shaderObject.mHandle);
			}

		} while( !bSuccess && bRecompile );

		return bSuccess;
	}


	bool ShaderFormatGLSL::initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(*shaderProgram.mRHIResource);

		if (!shaderProgramImpl.setupShaders(setupData.shaderResources.data(), setupData.shaderResources.size()))
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

	void ShaderFormatGLSL::postShaderLoaded(ShaderProgram& shaderProgram)
	{

	}


	bool ShaderFormatGLSL::initializeShader(Shader& shader, ShaderSetupData& setupData)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(*shader.mRHIResource);
		if (!shaderImpl.attach(*static_cast<OpenGLShaderObject*>(setupData.shaderResource.formatData)))
			return false;

		ShaderParameterMap parameterMap;
		shaderImpl.generateParameterMap(parameterMap);
		shader.bindParameters(parameterMap);

		return true;
	}

	bool ShaderFormatGLSL::initializeShader(Shader& shader, ShaderCompileInfo const& shaderCompile, std::vector<uint8> const& binaryCode)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(*shader.mRHIResource);

		GLenum format = *(GLenum*)binaryCode.data();
		glProgramBinary(shaderImpl.getHandle(), format, binaryCode.data() + sizeof(GLenum), binaryCode.size() - sizeof(GLenum));
		if (!VerifyOpenGLStatus())
		{
			return false;
		}
		if (!shaderImpl.validate())
		{
			return false;
		}

		ShaderParameterMap parameterMap;
		shaderImpl.generateParameterMap(parameterMap);
		shader.bindParameters(parameterMap);

		return true;
	}

	void ShaderFormatGLSL::postShaderLoaded(Shader& shader)
	{

	}

	bool ShaderFormatGLSL::isSupportBinaryCode() const
	{
		int numFormat = 0;
		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormat);
		return numFormat != 0;
	}

	bool ShaderFormatGLSL::getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(*shaderProgram.mRHIResource);
		return FOpenGLShader::GetProgramBinary(shaderProgramImpl.getHandle(), outBinaryCode);
	}

	bool ShaderFormatGLSL::getBinaryCode(Shader& shader, ShaderSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(*shader.mRHIResource);
		return FOpenGLShader::GetProgramBinary(shaderImpl.getHandle(), outBinaryCode);
	}

	bool FOpenGLShader::CheckProgramStatus(GLuint handle, GLenum statusName, char const* errorTitle)
	{
		GLchar buffer[4096 * 32];
		GLint value = 0;
		glGetProgramiv(handle, statusName, &value);
		if (value != GL_TRUE)
		{
			GLsizei size = 0;
			glGetProgramInfoLog(handle, ARRAY_SIZE(buffer), &size, buffer);
			LogMsg("%s : %s", errorTitle, buffer);
			return false;
		}
		return true;
	}

	bool FOpenGLShader::GetParameter(GLuint handle, char const* name, ShaderParameter& outParameter)
	{
		int loc = glGetUniformLocation(handle, name);
		if (loc == -1)
			return false;
		outParameter.mLoc = loc;
		return true;
	}

	bool FOpenGLShader::GetResourceParameter(GLuint handle, EShaderResourceType type, char const* name, ShaderParameter& outParameter)
	{
		int index = -1;
		switch (type)
		{
		case EShaderResourceType::Uniform:
			index = glGetProgramResourceIndex(handle, GL_UNIFORM_BLOCK, name);
			break;
		case EShaderResourceType::Storage:
			index = glGetProgramResourceIndex(handle, GL_SHADER_STORAGE_BLOCK, name);
			break;
		case EShaderResourceType::AtomicCounter:
			{
				int indexTemp = glGetProgramResourceIndex(handle, GL_UNIFORM, name);
				if (indexTemp != -1)
				{
					GLint loc = -1;
					const GLenum properties[] = { GL_ATOMIC_COUNTER_BUFFER_INDEX };
					glGetProgramResourceiv(handle, GL_UNIFORM, indexTemp, ARRAY_SIZE(properties), properties, ARRAY_SIZE(properties), NULL, &loc);
					index = loc;
				}
			}
			break;
		}
		if (index == -1)
			return false;
		outParameter.mLoc = index;
		return true;
	}

	void FOpenGLShader::GenerateParameterMap(GLuint handle, ShaderParameterMap& parameterMap)
	{
		auto GetBlockParameterFun = [&](GLenum BlockTypeInterface)
		{
			GLint numBlocks = 0;
			glGetProgramInterfaceiv(handle, BlockTypeInterface, GL_ACTIVE_RESOURCES, &numBlocks);
			for (int idxBlock = 0; idxBlock < numBlocks; ++idxBlock)
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
		for (int i = 0; i < numParam; ++i)
		{
			int name_len = -1, num = -1;
			GLenum type = GL_ZERO;
			char name[100];
			glGetActiveUniform(handle, GLuint(i), sizeof(name) - 1,
				&name_len, &num, &type, name);

			GLuint location = glGetUniformLocation(handle, name);

			if (location == -1)
				continue;

			parameterMap.addParameter(name, location);

		}
#else

		GLint numParam = 0;
		glGetProgramInterfaceiv(handle, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numParam);
		for (int paramIndex = 0; paramIndex < numParam; ++paramIndex)
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

			if (values[1] == -1)
			{
				if (values[2] != -1)
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

	bool FOpenGLShader::GetProgramBinary(GLuint handle, std::vector<uint8>& outBinary)
	{
		GLint binaryLength = -1;
		glGetProgramiv(handle, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
		if (binaryLength <= 0)
		{

			return false;
		}

		outBinary.resize(binaryLength + sizeof(GLenum));
		uint8* pData = outBinary.data();
		// BinaryFormat is stored at the start of ProgramBinary array
		glGetProgramBinary(handle, binaryLength, &binaryLength, (GLenum*)pData, pData + sizeof(GLenum));
		if (!VerifyOpenGLStatus())
		{

			return false;
		}
		return true;
	}

}