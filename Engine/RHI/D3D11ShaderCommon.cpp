#include "D3D11ShaderCommon.h"

#include "D3D11Common.h"

#include "RHICommand.h"
#include "ShaderProgram.h"

#include "FileSystem.h"
#include "CPreprocessor.h"

#include "Serialize/StreamBuffer.h"

#include <D3D11.h>

namespace Render
{

	class D3D11ShaderCompileIntermediates : public ShaderCompileIntermediates
	{
	public:
		TArray< TComPtr< ID3DBlob > > codeList;
	};


	void ShaderFormatHLSL::setupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addMeta("ShaderFormat", getName());
	}

	void ShaderFormatHLSL::getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
	{
		inoutCode += "#define COMPILER_HLSL 1\n";
	}

	bool ShaderFormatHLSL::compileCode(ShaderCompileContext const& context)
	{
		bool bGLSLCodeConv = true;
		bool bSuccess;
		do
		{
			bSuccess = true;
			TArray< uint8 > codeBuffer;

			if (!loadCode(context, codeBuffer))
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


			TComPtr< ID3DBlob > errorCode;
			TComPtr< ID3DBlob > byteCode;
			InlineString<32> profileName = FD3D11Utility::GetShaderProfile( mDevice->GetFeatureLevel() , context.getType());

			uint32 compileFlag = D3DCOMPILE_IEEE_STRICTNESS /*| D3D10_SHADER_PACK_MATRIX_ROW_MAJOR*/;
			if (GRHIPrefEnabled)
				compileFlag |= D3DCOMPILE_DEBUG;

			VERIFY_D3D_RESULT(
				D3DCompile(codeBuffer.data(), codeBuffer.size(), "ShaderCode", NULL, NULL, context.desc->entryName.c_str(),
										profileName, compileFlag, 0, &byteCode, &errorCode),
				{
					bSuccess = false;
				}
			);

			if( !bSuccess )
			{
				if( context.bUsePreprocess )
				{
					FFileUtility::SaveFromBuffer("temp" SHADER_FILE_SUBNAME, codeBuffer.data(), codeBuffer.size());
				}


				emitCompileError(context, (LPCSTR)errorCode->GetBufferPointer());
				continue;
			}

			if (context.programSetupData)
			{
				auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*context.programSetupData->resource);

				auto& shaderIntermediates = static_cast<D3D11ShaderCompileIntermediates&>(*context.programSetupData->intermediateData.get());
				shaderIntermediates.codeList.push_back(byteCode);

				if (!shaderProgramImpl.mShaderDatas[context.shaderIndex].initialize(context.getType(), mDevice, byteCode))
				{
					return false;
				}

				if (context.getType() == EShader::Vertex)
				{
					uint8 const* code = (uint8 const*)byteCode->GetBufferPointer();
					shaderProgramImpl.vertexByteCode.assign(code, code + byteCode->GetBufferSize());
				}
			}
			else
			{
				auto& shaderIntermediates = static_cast<D3D11ShaderCompileIntermediates&>(*context.shaderSetupData->intermediateData.get());
				shaderIntermediates.codeList.push_back(byteCode);

				context.shaderSetupData->resource = RHICreateShader(context.getType());
				auto* shaderImpl = static_cast<D3D11Shader*>(context.shaderSetupData->resource.get());
				if (!shaderImpl->initialize(mDevice, byteCode))
				{
					LogWarning(0, "Can't create shader resource");
					return false;
				}
			}


		} while( !bSuccess && context.bRecompile );

		return bSuccess;
	}

	bool ShaderFormatHLSL::initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
		auto& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);
		shaderProgramImpl.mParameterMap.clear();
		auto& shaderIntermediates = static_cast<D3D11ShaderCompileIntermediates&>(*setupData.intermediateData.get());
		for (int i = 0; i < shaderProgramImpl.mNumShaders; ++i)
		{
			TComPtr<ID3DBlob>& byteCode = shaderIntermediates.codeList[i];
			ShaderParameterMap parameterMap;
			D3DShaderParamInfo paramInfo;
			D3D11Shader::GenerateParameterMap(TArrayView<uint8 const>((uint8*)byteCode->GetBufferPointer(), byteCode->GetBufferSize()), parameterMap, paramInfo);
			shaderProgramImpl.mParameterMap.addShaderParameterMap(i, parameterMap);
			shaderProgramImpl.mShaderDatas[i].globalConstBufferSize = paramInfo.globalConstBufferSize;
		}
		shaderProgramImpl.mParameterMap.finalizeParameterMap();
		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);
		return true;
	}

	bool ShaderFormatHLSL::initializeProgram(ShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& descList, TArray<uint8> const& binaryCode)
	{
		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);
		shaderProgramImpl.mParameterMap.clear();

		auto serializer = CreateBufferSerializer<SimpleReadBuffer>(MakeConstView(binaryCode));
		uint8 numShaders = 0;
		serializer.read(numShaders);
		shaderProgramImpl.initializeData(numShaders);
		for (int i = 0; i < numShaders; ++i)
		{
			uint8 shaderType;
			serializer.read(shaderType);
			TArray< uint8 > byteCode;
			serializer.read(byteCode);

			if (!shaderProgramImpl.mShaderDatas[i].initialize(EShader::Type(shaderType), mDevice, byteCode.data(), byteCode.size()))
				return false;

			ShaderParameterMap parameterMap;
			D3DShaderParamInfo paramInfo;
			if (!D3D11Shader::GenerateParameterMap( MakeConstView( byteCode ) , parameterMap, paramInfo))
				return false;

			shaderProgramImpl.mParameterMap.addShaderParameterMap(i, parameterMap);
			shaderProgramImpl.mShaderDatas[i].globalConstBufferSize = paramInfo.globalConstBufferSize;
			if (shaderType == EShader::Vertex)
			{
				shaderProgramImpl.vertexByteCode = std::move(byteCode);
			}
		}

		shaderProgramImpl.mParameterMap.finalizeParameterMap();
		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);
		return true;
	}

	bool ShaderFormatHLSL::doesSuppurtBinaryCode() const
	{
		return true;
	}

	bool ShaderFormatHLSL::getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, TArray<uint8>& outBinaryCode)
	{
		auto& shaderIntermediates = static_cast<D3D11ShaderCompileIntermediates&>(*setupData.intermediateData.get());

		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*shaderProgram.mRHIResource);
		auto serializer = CreateBufferSerializer<ArrayWriteBuffer>(outBinaryCode);
		uint8 numShaders = shaderProgramImpl.mNumShaders;
		serializer.write(numShaders);
		for (int i = 0; i < numShaders; ++i)
		{
			auto& shaderData = shaderProgramImpl.mShaderDatas[i];
			TComPtr<ID3DBlob>& byteCode = shaderIntermediates.codeList[i];
			uint8 shaderType = shaderData.type;
			serializer.write(shaderType);
			serializer.write(byteCode->GetBufferPointer(), byteCode->GetBufferSize());
		}

		return true;
	}

	bool ShaderFormatHLSL::getBinaryCode(Shader& shader, ShaderSetupData& setupData, TArray<uint8>& outBinaryCode)
	{
		auto& shaderIntermediates = static_cast<D3D11ShaderCompileIntermediates&>(*setupData.intermediateData.get());
		TComPtr<ID3DBlob>& byteCode = shaderIntermediates.codeList[0];
		D3D11Shader& shaderImpl = static_cast<D3D11Shader&>(*shader.mRHIResource);
		outBinaryCode.assign((uint8*)byteCode->GetBufferPointer(), (uint8*)(byteCode->GetBufferPointer()) + byteCode->GetBufferSize());
		return true;
	}

	void ShaderFormatHLSL::precompileCode(ShaderProgramSetupData& setupData)
	{
		setupData.intermediateData = std::make_unique<D3D11ShaderCompileIntermediates>();

		D3D11ShaderProgram& shaderProgramImpl = static_cast<D3D11ShaderProgram&>(*setupData.resource);
		shaderProgramImpl.initializeData(setupData.getShaderCount());
		
	}

	void ShaderFormatHLSL::precompileCode(ShaderSetupData& setupData)
	{
		setupData.intermediateData = std::make_unique<D3D11ShaderCompileIntermediates>();
	}

	bool ShaderFormatHLSL::initializeShader(Shader& shader, ShaderSetupData& setupData)
	{
		D3D11Shader& shaderImpl = static_cast<D3D11Shader&>(*shader.mRHIResource);
		D3D11ShaderCompileIntermediates* intermediateData = static_cast<D3D11ShaderCompileIntermediates*>(setupData.intermediateData.get());

		TComPtr<ID3DBlob>& code = intermediateData->codeList[0];

		D3DShaderParamInfo paramInfo;
		if (!D3D11Shader::GenerateParameterMap(TArrayView<uint8 const>((uint8*)code->GetBufferPointer(), code->GetBufferSize()), shaderImpl.mParameterMap, paramInfo))
			return false;

		shader.bindParameters(shaderImpl.mParameterMap);
		shaderImpl.mResource.globalConstBufferSize = paramInfo.globalConstBufferSize;
		return true;
	}

	bool ShaderFormatHLSL::initializeShader(Shader& shader, ShaderCompileDesc const& desc, TArray<uint8> const& binaryCode)
	{
		shader.mRHIResource = RHICreateShader(desc.type);

		D3D11Shader& shaderImpl = static_cast<D3D11Shader&>(*shader.mRHIResource);
		//#TODO:
		TArray<uint8> temp{ binaryCode.begin() , binaryCode.end() };
		if (!shaderImpl.initialize(mDevice, std::move(temp)))
			return false;

		D3DShaderParamInfo paramInfo;
		if (!D3D11Shader::GenerateParameterMap(MakeConstView(binaryCode), shaderImpl.mParameterMap, paramInfo))
			return false;

		shader.bindParameters(shaderImpl.mParameterMap);
		shaderImpl.mResource.globalConstBufferSize = paramInfo.globalConstBufferSize;
		return true;
	}

	void ShaderFormatHLSL::postShaderLoaded(ShaderProgram& shaderProgram)
	{

	}

	void ShaderFormatHLSL::postShaderLoaded(Shader& shader)
	{

	}

	bool D3D11Shader::initialize(TComPtr<ID3D11Device>& device, TComPtr<ID3DBlob>& inByteCode)
	{
		if (!mResource.initialize(mType, device, (uint8*)inByteCode->GetBufferPointer(), inByteCode->GetBufferSize()))
			return false;

		if (getType() == EShader::Vertex)
		{
			byteCode.assign((uint8*)inByteCode->GetBufferPointer(), (uint8*)(inByteCode->GetBufferPointer()) + inByteCode->GetBufferSize());
		}
		return true;
	}

	bool D3D11Shader::initialize(TComPtr<ID3D11Device>& device, TArray<uint8>&& inByteCode)
	{
		if (!mResource.initialize(mType, device, inByteCode.data(), inByteCode.size()))
			return false;

		if (getType() == EShader::Vertex)
		{
			byteCode = std::move(inByteCode);
		}
		return true;
	}

	bool D3D11Shader::GenerateParameterMap(TArrayView<uint8 const> const& byteCode, ShaderParameterMap& parameterMap, D3DShaderParamInfo& outParamInfo)
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
						outParamInfo.globalConstBufferSize = bufferDesc.Size;
						for( int idxVar = 0; idxVar < bufferDesc.Variables; ++idxVar )
						{
							ID3D11ShaderReflectionVariable* var = buffer->GetVariableByIndex(idxVar);
							if( var )
							{
								D3D11_SHADER_VARIABLE_DESC varDesc;
								var->GetDesc(&varDesc);
								auto& param = parameterMap.addParameter(varDesc.Name, bindDesc.BindPoint, varDesc.StartOffset, varDesc.Size);
#if SHADER_DEBUG
								param.mbindType = EShaderParamBindType::Uniform;
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
#if SHADER_DEBUG
						param.mbindType = EShaderParamBindType::UniformBuffer;
#endif
					}
				}
				break;

			case D3D_SIT_SAMPLER:
				{
					auto& param = parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::Sampler;
#endif
				}
				break;
			case D3D_SIT_TEXTURE:
				{
					auto& param = parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::Texture;
#endif
				}
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_STRUCTURED:
				{
					auto& param = parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::StorageBuffer;
#endif
				}
				break;
			}
		}

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
			mShaderDatas[i].release();
		}

		delete [] mShaderDatas;
		mNumShaders = 0;
	}

	bool D3D11ShaderData::initialize(EShader::Type inType, TComPtr<ID3D11Device>& device, TComPtr<ID3DBlob>& inByteCode)
	{
		auto pCode = (uint8 const*)inByteCode->GetBufferPointer();
		auto codeSize = inByteCode->GetBufferSize();

		return initialize(inType, device, pCode, codeSize);
	}

	bool D3D11ShaderData::initialize(EShader::Type inType, TComPtr<ID3D11Device>& device, uint8 const* pCode, uint32 codeSize )
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

	void D3D11ShaderData::release()
	{
		if (ptr)
		{
			ptr->Release();
			ptr = nullptr;
		}
	}

}