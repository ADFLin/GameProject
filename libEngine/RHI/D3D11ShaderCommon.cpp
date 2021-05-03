#include "D3D11ShaderCommon.h"

#include "D3D11Common.h"
#include "ShaderProgram.h"

#include "FileSystem.h"
#include "CPreprocessor.h"

#include "Serialize/StreamBuffer.h"
#include "RHICommand.h"
#include "Platform/Windows/WindowsProcess.h"


#include <streambuf>
#include <fstream>
#include <sstream>

#include <D3D11.h>


namespace Render
{

	void ShaderFormatHLSL::setupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addMeta("ShaderFormat", getName());
	}

	void ShaderFormatHLSL::getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
	{
		inoutCode += "#define COMPILER_HLSL 1\n";
	}

	bool ShaderFormatHLSL::compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output)
	{
		bool bGLSLCodeConv = true;
		bool bSuccess;
		do
		{
			bSuccess = true;
			std::vector< char > codeBuffer;

			if( bUsePreprocess )
			{
				if( !FFileUtility::LoadToBuffer(input.path, codeBuffer, true) )
				{
					LogWarning(0, "Can't load shader file %s", input.path);
					return false;
				}

				if (!preprocessCode(input.path, output.compileInfo, input.definition, codeBuffer))
					return false;
#if 0
				if (bGLSLCodeConv)
				{
					codeBuffer.pop_back();
					std::string pathGLSL = "Temp/Temp.glsl";
					if (!FileUtility::SaveFromBuffer(pathGLSL.c_str(), codeBuffer.data(), codeBuffer.size()))
					{
						return false;
					}

				ChildProcess process;
					char const* command = "glslcc --%s = %s --output = Temp/Temp.hlsl --lang = hlsl --reflect";
					process.create("glslcc --vert = shader.vert --frag = shader.frag --output = shader.hlsl --lang = hlsl --reflect")

				}
#endif
			}
			else
			{
				if(input.definition)
				{
					int lenDef = strlen(input.definition);
					codeBuffer.resize(lenDef);
					codeBuffer.assign(input.definition, input.definition + lenDef);
				}

				if( !FFileUtility::LoadToBuffer(input.path, codeBuffer, true, true) )
				{
					LogWarning(0, "Can't load shader file %s", input.path);
					return false;
				}
			}

			TComPtr< ID3D10Blob > errorCode;
			TComPtr< ID3D10Blob > byteCode;
			InlineString<32> profileName = FD3D11Utility::GetShaderProfile( mDevice->GetFeatureLevel() , input.type);

			uint32 compileFlag = D3DCOMPILE_IEEE_STRICTNESS /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
			if (GRHIPrefEnabled)
				compileFlag |= D3DCOMPILE_DEBUG;

			VERIFY_D3D_RESULT(
				D3DCompile(codeBuffer.data(), codeBuffer.size(), "ShaderCode", NULL, NULL, output.compileInfo->entryName.c_str(),
										profileName, compileFlag, 0, &byteCode, &errorCode),
				{
					bSuccess = false;
				}
			);

			if( !bSuccess )
			{
				if( bUsePreprocess )
				{
					FFileUtility::SaveFromBuffer("temp" SHADER_FILE_SUBNAME, codeBuffer.data(), codeBuffer.size());
				}

				if( bRecompile )
				{
					OutputError((LPCSTR)errorCode->GetBufferPointer());
				}
				continue;
			}

			auto* shaderImpl = static_cast<D3D11Shader*>(RHICreateShader(input.type));
			output.resource = shaderImpl;

			if( !shaderImpl->initialize(mDevice, byteCode) )
			{
				LogWarning(0, "Can't create shader resource");
				return false;
			}

		} while( !bSuccess && bRecompile );

		return bSuccess;
	}

	bool ShaderFormatHLSL::initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);

		if (!shaderProgramImpl.setupShaders(setupData.shaderResources.data(), setupData.shaderResources.size()))
			return false;

		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);
		return true;
	}

	bool ShaderFormatHLSL::initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode)
	{
		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);

		auto serializer = CreateBufferSerializer<SimpleReadBuffer>(MakeView(binaryCode));
		uint8 numShaders = 0;
		serializer.read(numShaders);

		ShaderResourceInfo shaders[EShader::Count];
		for (int i = 0; i < numShaders; ++i)
		{
			D3D11Shader& shaderImpl = *shaderProgramImpl.mShaders[i];
			uint8 shaderType;
			serializer.read(shaderType);
			std::vector< uint8 > byteCode;
			serializer.read(byteCode);

			D3D11Shader* shader = (D3D11Shader*)RHICreateShader(EShader::Type(shaderType));
			shaders[i].resource = shader;
			shaders[i].type = EShader::Type(shaderType);
			shaders[i].entry = shaderCompiles[i].entryName.c_str();

			if (!shader->initialize(mDevice, std::move(byteCode)))
				return false;
		}

		if (!shaderProgramImpl.setupShaders(shaders, numShaders))
			return false;

		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);
		return true;
	}

	bool ShaderFormatHLSL::isBinaryCodeSupported() const
	{
		return true;
	}

	bool ShaderFormatHLSL::getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);
		auto serializer = CreateBufferSerializer<VectorWriteBuffer>(outBinaryCode);
		uint8 numShaders = shaderProgramImpl.mNumShaders;
		serializer.write(numShaders);
		for (int i = 0; i < numShaders; ++i)
		{
			D3D11Shader& shaderImpl = *shaderProgramImpl.mShaders[i];
			uint8 shaderType = shaderImpl.mResource.type;
			serializer.write(shaderType);
			serializer.write(shaderImpl.byteCode);
		}

		return true;
	}

	bool ShaderFormatHLSL::getBinaryCode(Shader& shader, ShaderSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		D3D11Shader& shaderImpl = static_cast<D3D11Shader&>(*shader.mRHIResource);
		outBinaryCode = shaderImpl.byteCode;
		return true;
	}

	bool ShaderFormatHLSL::initializeShader(Shader& shader, ShaderSetupData& setupData)
	{
		shader.mRHIResource = setupData.shaderResource.resource;
		D3D11Shader& shaderImpl = static_cast<D3D11Shader&>(*shader.mRHIResource);
		D3D11Shader::GenerateParameterMap(shaderImpl.byteCode, shaderImpl.mParameterMap);
		shader.bindParameters(shaderImpl.mParameterMap);
		return true;
	}

	bool ShaderFormatHLSL::initializeShader(Shader& shader, ShaderCompileInfo const& shaderCompile, std::vector<uint8> const& binaryCode)
	{
		shader.mRHIResource = RHICreateShader(shaderCompile.type);

		D3D11Shader& shaderImpl = static_cast<D3D11Shader&>(*shader.mRHIResource);
		std::vector<uint8> temp = binaryCode;
		if (!shaderImpl.initialize(mDevice, std::move(temp)))
			return false;

		D3D11Shader::GenerateParameterMap(shaderImpl.byteCode, shaderImpl.mParameterMap);
		shader.bindParameters(shaderImpl.mParameterMap);
		return true;
	}

	void ShaderFormatHLSL::postShaderLoaded(ShaderProgram& shaderProgram)
	{
		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);
		for (int i = 0; i < shaderProgramImpl.mNumShaders; ++i)
		{
			D3D11Shader& shader = *shaderProgramImpl.mShaders[i];
			if (shader.mResource.type != EShader::Vertex)
			{
				shader.byteCode.clear();
			}
		}
	}

	void ShaderFormatHLSL::postShaderLoaded(Shader& shader)
	{
		D3D11Shader& shaderImpl = static_cast<D3D11Shader&>(*shader.mRHIResource);
		if (shaderImpl.mResource.type != EShader::Vertex)
		{
			shaderImpl.byteCode.clear();
		}
	}

	bool D3D11Shader::initialize(TComPtr<ID3D11Device>& device, TComPtr<ID3D10Blob>& inByteCode)
	{
		if (!mResource.initialize(mType, device, (uint8*)inByteCode->GetBufferPointer(), inByteCode->GetBufferSize()))
			return false;

		byteCode.assign((uint8*)inByteCode->GetBufferPointer() , (uint8*)(inByteCode->GetBufferPointer()) + inByteCode->GetBufferSize());

		return true;
	}

	bool D3D11Shader::initialize(TComPtr<ID3D11Device>& device, std::vector<uint8>&& inByteCode)
	{
		if (!mResource.initialize(mType, device, inByteCode.data(), inByteCode.size()))
			return false;

		byteCode = std::move(inByteCode);
		return true;
	}

	bool D3D11Shader::GenerateParameterMap(std::vector< uint8 > const& byteCode, ShaderParameterMap& parameterMap)
	{
		TComPtr< ID3D11ShaderReflection > reflection;
		VERIFY_D3D_RESULT_RETURN_FALSE(D3DReflect(byteCode.data(), byteCode.size(), IID_PPV_ARGS(&reflection)));

		D3D11_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);
		for( int i = 0; i < shaderDesc.BoundResources; ++i )
		{
			D3D11_SHADER_INPUT_BIND_DESC bindDesc;
			reflection->GetResourceBindingDesc(i, &bindDesc);

			switch( bindDesc.Type )
			{
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER:
				{
					ID3D11ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByName(bindDesc.Name);
					assert(buffer);
					D3D11_SHADER_BUFFER_DESC bufferDesc;
					buffer->GetDesc(&bufferDesc);
					if( FCString::Compare(bufferDesc.Name, "$Globals") == 0 )
					{
						for( int idxVar = 0; idxVar < bufferDesc.Variables; ++idxVar )
						{
							ID3D11ShaderReflectionVariable* var = buffer->GetVariableByIndex(idxVar);
							if( var )
							{
								D3D11_SHADER_VARIABLE_DESC varDesc;
								var->GetDesc(&varDesc);
								auto& param = parameterMap.addParameter(varDesc.Name, bindDesc.BindPoint, varDesc.StartOffset, varDesc.Size);
#if _DEBUG
								param.mbindType = EShaderParamBindType::Uniform;
								param.mName = varDesc.Name;
#endif
								ID3D11ShaderReflectionType* varType = var->GetType();
								if (varType)
								{
									D3D11_SHADER_TYPE_DESC typeDesc;
									varType->GetDesc(&typeDesc);

									int lastOffset = varDesc.Size;
									for (int indexMember = typeDesc.Members - 1; indexMember >= 0; --indexMember)
									{
										ID3D11ShaderReflectionType* memberType = varType->GetMemberTypeByIndex(indexMember);
										D3D11_SHADER_TYPE_DESC memberTypeDesc;
										memberType->GetDesc(&memberTypeDesc);
										char const* memberName = varType->GetMemberTypeName(indexMember);

										InlineString<256> paramName;
										paramName.format("%s.%s", varDesc.Name, memberName);
										auto& param = parameterMap.addParameter(paramName, bindDesc.BindPoint, varDesc.StartOffset + memberTypeDesc.Offset, lastOffset - memberTypeDesc.Offset);

										lastOffset = memberTypeDesc.Offset;
									}
								}
							}
						}
					}
					else
					{
						//CBuffer TBuffer
						auto& param = parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
#if _DEBUG
						param.mbindType = EShaderParamBindType::UniformBuffer;
						param.mName = bindDesc.Name;
#endif
					}
				}
				break;

			case D3D_SIT_SAMPLER:
				{
					auto& param = parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
#if _DEBUG
					param.mbindType = EShaderParamBindType::Sampler;
					param.mName = bindDesc.Name;
#endif
				}
				break;
			case D3D_SIT_TEXTURE:
				{
					auto& param = parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
#if _DEBUG
					param.mbindType = EShaderParamBindType::Texture;
					param.mName = bindDesc.Name;
#endif
				}
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_STRUCTURED:
				{
					auto& param = parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
#if _DEBUG
					param.mbindType = EShaderParamBindType::StorageBuffer;
					param.mName = bindDesc.Name;
#endif
				}
				break;
			}
		}

		return true;
	}

	bool D3D11ShaderProgram::setupShaders(ShaderResourceInfo shaders[], int numShaders)
	{
		mParameterMap.clear();

		assert(ARRAY_SIZE(mShaders) >= numShaders);
		mNumShaders = numShaders;
		for (int i = 0; i < numShaders; ++i)
		{
			mShaders[i] = &static_cast< D3D11Shader& >(*shaders[i].resource);
			auto& shaderImpl = *mShaders[i];
			ShaderParameterMap parameterMap;
			D3D11Shader::GenerateParameterMap(shaderImpl.byteCode, parameterMap);
			mParameterMap.addShaderParameterMap(shaderImpl.mResource.type, parameterMap);
		}

		mParameterMap.finalizeParameterMap();
		return true;
	}

	bool D3D11ShaderProgram::getParameter(char const* name, ShaderParameter& outParam)
	{
		return outParam.bind(mParameterMap, name);
	}

	bool D3D11ShaderProgram::getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam)
	{
		return outParam.bind(mParameterMap, name);
	}

	void D3D11ShaderProgram::releaseResource()
	{
		for (int i = 0; i < mNumShaders; ++i)
		{
			mShaders[i].release();
		}
	}

	bool D3D11ShaderResource::initialize(EShader::Type inType, TComPtr<ID3D11Device>& device, uint8 const* pCode, size_t codeSize)
	{
		switch (inType)
		{
#define CASE_SHADER(  TYPE , FUNC ,VAR )\
			case TYPE:\
				VERIFY_D3D_RESULT_RETURN_FALSE( device->FUNC(pCode, codeSize, NULL, &VAR) );\
				break;

			CASE_SHADER(EShader::Vertex, CreateVertexShader, vertex);
			CASE_SHADER(EShader::Pixel, CreatePixelShader, pixel);
			CASE_SHADER(EShader::Geometry, CreateGeometryShader, geometry);
			CASE_SHADER(EShader::Compute, CreateComputeShader, compute);
			CASE_SHADER(EShader::Hull, CreateHullShader, hull);
			CASE_SHADER(EShader::Domain, CreateDomainShader, domain);

#undef CASE_SHADER
		default:
			NEVER_REACH("D3D11Shader::createResource unknown shader Type");
		}
		type = inType;
		return true;
	}

	void D3D11ShaderResource::release()
	{
		if (ptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
	}

}