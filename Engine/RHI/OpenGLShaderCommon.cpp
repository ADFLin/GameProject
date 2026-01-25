#include "OpenGLShaderCommon.h"

#include "RHICommand.h"

#include "FileSystem.h"
#include "Core\StringConv.h"
#include "CString.h"

#include "ConsoleSystem.h"
#include "TemplateMisc.h"

extern CORE_API TConsoleVariable< bool > CVarShaderUseCache;

namespace Render
{
	extern bool GbOpenglOutputDebugMessage;

	bool IsLineFilePathSupported()
	{

		struct TestHelper
		{
			TestHelper()
			{
				char const* code = CODE_STRING
				(
#line 2 "test"
vec4 OutColor;
void main()
{
	OutColor = vec4(1, 1, 1, 1);
}
				);


				OpenGLShaderObject object;
				TGuardValue<bool> ScopedValue(GbOpenglOutputDebugMessage, false);
				result = object.compile(EShader::Pixel, &code, 1);
			}
			bool result;
		};
		static TestHelper helper;
		return helper.result;
	}

	bool OpenGLShaderObject::compile(EShader::Type type, char const* src[], int num)
	{
		if (!fetchHandle(OpenGLTranslate::To(type)))
			return false;

		glShaderSource(mHandle, num, src, 0);
		glCompileShader(mHandle);

		if (getIntParameter(GL_COMPILE_STATUS) == GL_FALSE)
		{
			return false;
		}

		return true;
	}

	bool OpenGLShaderObject::loadFile(EShader::Type type, char const* path, char const* def)
	{
		TArray<uint8> codeBuffer;
		if (!FFileUtility::LoadToBuffer(path, codeBuffer, true))
			return false;
		int num = 0;
		char const* src[2];
		if (def)
		{
			src[num] = def;
			++num;
		}
		src[num] = (char const*)codeBuffer.data();
		++num;

		bool result = compile(type, src, num);
		return result;
	}

	GLint OpenGLShaderObject::getIntParameter(GLuint val)
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
		GLuint handle = getHandle();

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

	void OpenGLShaderProgram::attach(EShader::Type type, OpenGLShaderObject const& shaderObject)
	{
		glAttachShader(getHandle(), shaderObject.mHandle);
	}

	bool OpenGLShaderProgram::initialize()
	{
		if (CVarShaderUseCache)
		{
			glProgramParameteri(getHandle(), GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
		}

		bool result = updateShader(true);

		auto DetachAllShader = [this]()
		{
			GLuint shaders[EShader::MaxStorageSize];
			GLsizei numShaders = 0;
			glGetAttachedShaders(getHandle(), ARRAY_SIZE(shaders), &numShaders, shaders);
			for (int i = 0; i < numShaders; ++i)
			{
				glDetachShader(getHandle(), shaders[i]);
			}
		};
		DetachAllShader();

		return result;
	}

	bool OpenGLShaderProgram::initialize(TArray<uint8> const& binaryCode)
	{
		GLenum format = *(GLenum*)binaryCode.data();
		glProgramBinary(getHandle(), format, binaryCode.data() + sizeof(GLenum), binaryCode.size() - sizeof(GLenum));
		if (!VerifyOpenGLStatus())
		{
			return false;
		}
		if (!updateShader(false))
		{
			return false;
		}

		return true;
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
		initType(type);

		if (!mGLObject.fetchHandle())
			return false;

		if (type != EShader::Compute)
		{
			glProgramParameteri(getHandle(), GL_PROGRAM_SEPARABLE, TRUE);
		}
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

	void OpenGLShader::bind()
	{
		if (getHandle())
		{
			glUseProgram(getHandle());
		}
	}

	void OpenGLShader::unbind()
	{
		glUseProgram(0);
	}

	void ShaderFormatGLSL::setupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addMeta("ShaderFormat", getName());
	}

	void ShaderFormatGLSL::getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
	{
		uint32 usageVersion = mDefaultVersion;
		if (entry.type == EShader::Task || entry.type == EShader::Mesh)
		{
			usageVersion = 460;
		}

		inoutCode += "#version ";
		char const* versionString = option.getMeta("GLSLVersion");
		if( versionString )
		{
			inoutCode += option.getMeta("GLSLVersion");
		}
		else
		{
			inoutCode += FStringConv::From(usageVersion);
		}
		//inoutCode += " compatibility\n";

		inoutCode += "\n";
		inoutCode += "#define GLSL_VERSION ";
		inoutCode += FStringConv::From(usageVersion);
		inoutCode += "\n";

		char const* extensionString = option.getMeta("GLSLExtensions");

		if (extensionString)
		{
			//extensionString = FStringParse::SkipSpace(extensionString);

		}

		if (entry.type == EShader::Task || entry.type == EShader::Mesh)
		{
			inoutCode += "#extension GL_NV_mesh_shader : enable\n";
			inoutCode += "#extension GL_NV_gpu_shader5 : enable\n";
			inoutCode += "#extension GL_NV_shader_thread_group : enable\n";
			inoutCode += "#extension GL_NV_shader_thread_shuffle : enable\n";
		}

		inoutCode += "#define COMPILER_GLSL 1\n";

		if( entry.name && FCString::Compare(entry.name , "main") != 0 )
		{
			inoutCode += "#define ";
			inoutCode += entry.name;
			inoutCode += " main\n";

			inoutCode += "#define SHADER_ENTRY_main 1\n";

		}
	}

	class GLSLCompileIntermediates : public ShaderCompileIntermediates
	{
	public:
		ShaderParameterMap parameterMap;
	};

	void ShaderFormatGLSL::precompileCode(ShaderProgramSetupData& setupData)
	{
		setupData.intermediateData = std::make_unique<GLSLCompileIntermediates>();
	}

	void ShaderFormatGLSL::precompileCode(ShaderSetupData& setupData)
	{
		setupData.intermediateData = std::make_unique<GLSLCompileIntermediates>();
	}

	EShaderCompileResult ShaderFormatGLSL::compileCode(ShaderCompileContext const& context)
	{
		int numSourceCodes = 0;
		char const* sourceCodes[4];
		CHECK(context.codes.size() < ARRAY_SIZE(sourceCodes));

		for (auto code : context.codes)
		{
			sourceCodes[numSourceCodes] = code.data();
			++numSourceCodes;
		}

		auto ProcessCompileError = [&]( GLuint shaderHandle )
		{
			if (shaderHandle)
			{
				TArray< char > buf;
				FOpenGLShader::GetLogInfo(shaderHandle, buf);
				emitCompileError(context , buf.data());
			}
		};

		OpenGLShaderObject shaderObject;
		if (!shaderObject.compile(context.getType(), sourceCodes, numSourceCodes))
		{
			context.checkOuputDebugCode();
			ProcessCompileError(shaderObject.mHandle);

			return EShaderCompileResult::CodeError;
		}

		if (context.programSetupData)
		{
			//If a shader object is deleted while it is attached to a program object, it will be flagged for deletion, 
			//and deletion will not occur until glDetachShader is called to detach it from all program objects to which it is attached
			auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(*context.programSetupData->resource);
			shaderProgramImpl.attach(context.getType(), shaderObject);
		}
		else
		{
			auto& shaderImpl = static_cast<OpenGLShader&>(*context.shaderSetupData->resource);
			if (!shaderImpl.attach(shaderObject))
			{
				LogWarning(0, "Can't create shader resource");
				return EShaderCompileResult::ResourceError;
			}
		}

		return EShaderCompileResult::Ok;
	}

	ShaderParameterMap* ShaderFormatGLSL::initializeProgram(RHIShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(shaderProgram);
		if (!shaderProgramImpl.initialize())
			return nullptr;

		auto& intermediatesImpl = static_cast<GLSLCompileIntermediates&>(*setupData.intermediateData);

		shaderProgramImpl.generateParameterMap(intermediatesImpl.parameterMap);
		return &intermediatesImpl.parameterMap;
	}

	ShaderParameterMap* ShaderFormatGLSL::initializeProgram(RHIShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& descList, TArray<uint8> const& binaryCode)
	{
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(shaderProgram);
		if (!shaderProgramImpl.initialize(binaryCode))
			return nullptr;

		mTempParameterMap.clear();
		shaderProgramImpl.generateParameterMap(mTempParameterMap);
		return &mTempParameterMap;
	}

	void ShaderFormatGLSL::postShaderLoaded(ShaderProgram& shaderProgram)
	{

	}

	ShaderParameterMap* ShaderFormatGLSL::initializeShader(RHIShader& shader, ShaderSetupData& setupData)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);

		auto& intermediatesImpl = static_cast<GLSLCompileIntermediates&>(*setupData.intermediateData);

		shaderImpl.generateParameterMap(intermediatesImpl.parameterMap);
		return &intermediatesImpl.parameterMap;
	}

	ShaderParameterMap* ShaderFormatGLSL::initializeShader(RHIShader& shader, ShaderCompileDesc const& desc, TArray<uint8> const& binaryCode)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(shader);

		GLenum format = *(GLenum*)binaryCode.data();
		glProgramBinary(shaderImpl.getHandle(), format, binaryCode.data() + sizeof(GLenum), binaryCode.size() - sizeof(GLenum));
		if (!VerifyOpenGLStatus())
		{
			return nullptr;
		}
		if (!shaderImpl.validate())
		{
			return nullptr;
		}

		mTempParameterMap.clear();
		shaderImpl.generateParameterMap(mTempParameterMap);
		return &mTempParameterMap;
	}

	void ShaderFormatGLSL::postShaderLoaded(RHIShader& shader)
	{

	}

	bool ShaderFormatGLSL::doesSuppurtBinaryCode() const
	{
		int numFormat = 0;
		glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numFormat);
		return numFormat != 0;
	}

	bool ShaderFormatGLSL::getBinaryCode(ShaderProgramSetupData& setupData, TArray<uint8>& outBinaryCode)
	{
		auto& shaderProgramImpl = static_cast<OpenGLShaderProgram&>(*setupData.resource);
		return FOpenGLShader::GetProgramBinary(shaderProgramImpl.getHandle(), outBinaryCode);
	}

	bool ShaderFormatGLSL::getBinaryCode(ShaderSetupData& setupData, TArray<uint8>& outBinaryCode)
	{
		auto& shaderImpl = static_cast<OpenGLShader&>(*setupData.resource);
		return FOpenGLShader::GetProgramBinary(shaderImpl.getHandle(), outBinaryCode);
	}

	ShaderPreprocessSettings ShaderFormatGLSL::getPreprocessSettings()
	{
		ShaderPreprocessSettings result;
		result.bSupportLineFilePath = IsLineFilePathSupported();
		if (result.bSupportLineFilePath == false)
			result.bAddLineMacro = false;

		return result;
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
		auto GetIndex = [&](char const* parameterName) -> int
		{ 
			switch (type)
			{
			case EShaderResourceType::Uniform:
				return glGetProgramResourceIndex(handle, GL_UNIFORM_BLOCK, parameterName);
			case EShaderResourceType::Storage:
				return glGetProgramResourceIndex(handle, GL_SHADER_STORAGE_BLOCK, parameterName);
			}
			return -1;
		};

		int index = -1;
		if (type == EShaderResourceType::AtomicCounter)
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
		else
		{
			index = GetIndex(name);
			if (index == -1 && type == EShaderResourceType::Storage)
			{
				// Fallback: try with "Block" suffix
				std::string nameBlock = std::string(name) + "Block";
				index = GetIndex(nameBlock.c_str());
			}
		}
		if (index == -1)
			return false;
		outParameter.mLoc = index;
		return true;
	}

	void FOpenGLShader::GenerateParameterMap(GLuint handle, ShaderParameterMap& parameterMap)
	{
		auto GetBlockParameter = [&](GLenum BlockTypeInterface)
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
				auto& param = parameterMap.addParameter(name, idxBlock);
#if SHADER_DEBUG
				param.mbindType = BlockTypeInterface == GL_UNIFORM_BLOCK ? EShaderParamBindType::UniformBuffer : EShaderParamBindType::StorageBuffer;
#endif
				// Secret Rename: Register alias without "Block" suffix
				int len = FCString::Strlen(name);
				if (BlockTypeInterface == GL_SHADER_STORAGE_BLOCK && len > 5 && FCString::Compare(name + len - 5, "Block") == 0)
				{
					std::string strippedName(name, len - 5);
					auto& param2 = parameterMap.addParameter(strippedName.c_str(), idxBlock);
#if SHADER_DEBUG
					param2.mbindType = param.mbindType;
#endif
				}
			}
		};

		GetBlockParameter(GL_UNIFORM_BLOCK);
		GetBlockParameter(GL_SHADER_STORAGE_BLOCK);

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
					auto& param = parameterMap.addParameter(name, values[2]);
#if SHADER_DEBUG
					param.mName = name;
#endif
				}
			}
			else
			{
				auto& param = parameterMap.addParameter(name, values[1]);
#if SHADER_DEBUG
				param.mbindType = EShaderParamBindType::Uniform;
#endif
			}
		}
#endif
	}

	bool FOpenGLShader::GetProgramBinary(GLuint handle, TArray<uint8>& outBinary)
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

	int FOpenGLShader::GetLogInfo(GLuint handle, TArray<char>& outBuffer)
	{
		int maxLength = 0;
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &maxLength);
		outBuffer.resize(maxLength);
		int logLength = 0;
		glGetShaderInfoLog(handle, maxLength, &logLength, &outBuffer[0]);
		return logLength;
	}

}