#include "D3D12ShaderCommon.h"

#include "D3D12Utility.h"
#include "D3D12Common.h"
#include "D3DSharedCommon.h"

#include "ShaderManager.h"
#include "ShaderProgram.h"
#include "RHICommand.h"

#include "InlineString.h"
#include "FileSystem.h"
#include "CPreprocessor.h"

#include <streambuf>
#include <fstream>
#include <sstream>

#include <d3d12.h>
#include <D3D12Shader.h>
#include <D3Dcompiler.h>


#include "Serialize/StreamBuffer.h"

#include "Dxc/dxcapi.h"
#if TARGET_PLATFORM_64BITS
#pragma comment(lib , "dxcompiler.lib")
#endif

namespace Render
{

	ShaderFormatHLSL_D3D12::ShaderFormatHLSL_D3D12(TComPtr< ID3D12Device8 >& inDevice) :mDevice(inDevice)
	{

	}

	ShaderFormatHLSL_D3D12::~ShaderFormatHLSL_D3D12()
	{


	}

	void ShaderFormatHLSL_D3D12::setupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addMeta("ShaderFormat", getName());
	}

	void ShaderFormatHLSL_D3D12::getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry)
	{
		inoutCode += "#define COMPILER_HLSL 1\n";
	}

	bool ShaderFormatHLSL_D3D12::compileCode(ShaderCompileContext const& context)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());

		bool bSuccess;
		do
		{
			bSuccess = true;
			std::vector< uint8 > codeBuffer;
			if (!loadCode(context, codeBuffer))
				return false;


			uint32_t codePage = CP_UTF8;
			TComPtr<IDxcBlobEncoding> sourceBlob;
			VERIFY_D3D_RESULT_RETURN_FALSE( mLibrary->CreateBlobWithEncodingFromPinned( codeBuffer.data() , (uint32)codeBuffer.size(), codePage, &sourceBlob) );

			wchar_t const* profileName;
			switch (context.getType())
			{
			case EShader::Vertex: profileName = L"vs_6_1"; break;
			case EShader::Pixel: profileName = L"ps_6_1"; break;
			case EShader::Geometry: profileName = L"gs_6_1"; break;
			case EShader::Compute: profileName = L"cs_6_1"; break;
			case EShader::Hull: profileName = L"hs_6_1"; break;
			case EShader::Domain: profileName = L"ds_6_1"; break;
			case EShader::Task: profileName = L"as_6_1"; break;
			case EShader::Mesh: profileName = L"ms_6_1"; break;
			default:
				break;
			}

			wchar_t const* args[16];
			int numArgs = 0;
			if (GRHIPrefEnabled)
			{
				args[numArgs++] = L"-Zi";
			}
			char const* fileName = FFileUtility::GetFileName(context.getPath());
			TComPtr<IDxcOperationResult> compileResult;
			HRESULT hr = mCompiler->Compile(
				sourceBlob, // pSource
				FCString::CharToWChar( fileName ).c_str(), // pSourceName
				FCString::CharToWChar( context.getEntry() ).c_str(), // pEntryPoint
				profileName , // pTargetProfile
				args, numArgs, // pArguments, argCount
				NULL, 0, // pDefines, defineCount
				NULL, // pIncludeHandler
				&compileResult); // ppResult
			if (SUCCEEDED(hr))
			{
				compileResult->GetStatus(&hr);
				if (FAILED(hr))
				{
					bSuccess = false;
				}
			}
			else
			{
				bSuccess = false;
			}

			if (!bSuccess)
			{
				if (bUsePreprocess)
				{
					FFileUtility::SaveFromBuffer("temp" SHADER_FILE_SUBNAME, codeBuffer.data(), codeBuffer.size());
				}

				if (bRecompile)
				{
					TComPtr<IDxcBlobEncoding> errorsBlob;
					hr = compileResult->GetErrorBuffer(&errorsBlob);

					emitCompileError(context, (LPCSTR)errorsBlob->GetBufferPointer());
				}
				continue;
			}

			TComPtr<IDxcBlob> shaderCode;
			compileResult->GetResult(&shaderCode);

			if (context.programSetupData)
			{
				D3D12ShaderProgram& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(*context.programSetupData->resource);
				auto& shaderData = shaderProgramImpl.mShaderDatas[context.shaderIndex];
			
				if (!shaderData.initialize(shaderCode))
					return false;
				shaderData.type = context.getType();
			}
			else
			{
				auto* shaderImpl = static_cast<D3D12Shader*>(RHICreateShader(context.getType()));
				if (!shaderImpl->initialize(shaderCode))
				{
					LogWarning(0, "Can't create shader resource");
					return false;
				}
				context.shaderSetupData->resource = shaderImpl;
			}
		} 
		while (!bSuccess && bRecompile);

		return bSuccess;
#else
		return false;
#endif
	}

	bool ShaderFormatHLSL_D3D12::getBinaryCode(Shader& shader, ShaderSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		D3D12Shader& shaderImpl = static_cast<D3D12Shader&>(*shader.mRHIResource);
		outBinaryCode = shaderImpl.code;
		return true;
	}

	bool ShaderFormatHLSL_D3D12::getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode)
	{
		D3D12ShaderProgram& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(*shaderProgram.mRHIResource);
		auto serializer = CreateBufferSerializer<VectorWriteBuffer>(outBinaryCode);
		uint8 numShaders = shaderProgramImpl.mNumShaders;
		serializer.write(numShaders);
		for (int i = 0; i < numShaders; ++i)
		{
			auto& shaderData = shaderProgramImpl.mShaderDatas[i];
			uint8 shaderType = shaderData.type;
			serializer.write(shaderType);
			serializer.write(shaderData.code);
		}

		return true;
	}

	void ShaderFormatHLSL_D3D12::precompileCode(ShaderProgramSetupData& setupData)
	{
		D3D12ShaderProgram& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(*setupData.resource);
		shaderProgramImpl.initializeData(setupData.getShaderCount());
	}

	void ShaderFormatHLSL_D3D12::precompileCode(ShaderSetupData& setupData)
	{

	}

	bool ShaderFormatHLSL_D3D12::ensureDxcObjectCreation()
	{
#if TARGET_PLATFORM_64BITS
		if (!mLibrary)
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mLibrary)));
		}
		if (!mCompiler)
		{
			VERIFY_D3D_RESULT_RETURN_FALSE(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mCompiler)));
		}

		return true;
#else
		return false;
#endif
	}

	bool ShaderFormatHLSL_D3D12::initializeShader(Shader& shader, ShaderSetupData& setupData)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());
		shader.mRHIResource = setupData.resource;
		D3D12Shader& shaderImpl = static_cast<D3D12Shader&>(*shader.mRHIResource);

		D3D12Shader::GenerateParameterMap(shaderImpl.code, mLibrary, shaderImpl.mParameterMap, shaderImpl.rootSignature);
		D3D12Shader::SetupShader(shaderImpl.rootSignature, shaderImpl.mType);
		shader.bindParameters(shaderImpl.mParameterMap);
		return true;
#else
		return false;
#endif
	}

	bool ShaderFormatHLSL_D3D12::initializeShader(Shader& shader, ShaderCompileDesc const& desc, std::vector<uint8> const& binaryCode)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());

		shader.mRHIResource = RHICreateShader(desc.type);

		D3D12Shader& shaderImpl = static_cast<D3D12Shader&>(*shader.mRHIResource);
		std::vector<uint8> temp = binaryCode;
		if (!shaderImpl.initialize(std::move(temp)))
			return false;

		D3D12Shader::GenerateParameterMap(shaderImpl.code, mLibrary, shaderImpl.mParameterMap, shaderImpl.rootSignature);
		D3D12Shader::SetupShader(shaderImpl.rootSignature, shaderImpl.mType);
		shader.bindParameters(shaderImpl.mParameterMap);
		return true;
#else
		return false;
#endif
	}

	bool ShaderFormatHLSL_D3D12::initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());

		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(*shaderProgram.mRHIResource);
		shaderProgramImpl.mParameterMap.clear();
		shaderProgramImpl.initializeParameterMap(mLibrary);
		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);

		return true;
#else
		return false;
#endif
	}

	bool ShaderFormatHLSL_D3D12::initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileDesc > const& descList, std::vector<uint8> const& binaryCode)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());

		D3D12ShaderProgram& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(*shaderProgram.mRHIResource);

		auto serializer = CreateBufferSerializer<SimpleReadBuffer>(MakeConstView(binaryCode));
		uint8 numShaders = 0;
		serializer.read(numShaders);
		shaderProgramImpl.initializeData(numShaders);
		for (int i = 0; i < numShaders; ++i)
		{
			uint8 shaderType;
			serializer.read(shaderType);
			std::vector< uint8 > byteCode;
			serializer.read(byteCode);

			auto& shaderData = shaderProgramImpl.mShaderDatas[i];
			shaderData.type = EShader::Type(shaderType);
			shaderData.initialize(std::move(byteCode));
		}

		shaderProgramImpl.initializeParameterMap(mLibrary);
		shaderProgram.bindParameters(shaderProgramImpl.mParameterMap);
		return true;
#else
		return false;
#endif
	}



	bool D3D12Shader::GenerateParameterMap(std::vector< uint8 > const& byteCode, TComPtr<IDxcLibrary>& library, ShaderParameterMap& parameterMap, ShaderRootSignature& inOutSignature)
	{
#if TARGET_PLATFORM_64BITS
		TComPtr<IDxcContainerReflection> containerReflection;
		VERIFY_D3D_RESULT_RETURN_FALSE(DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&containerReflection)));

		TComPtr<IDxcBlobEncoding> byteCodeBlob;
		VERIFY_D3D_RESULT_RETURN_FALSE(library->CreateBlobWithEncodingFromPinned(byteCode.data(), byteCode.size(), 0, &byteCodeBlob));

		VERIFY_D3D_RESULT_RETURN_FALSE(containerReflection->Load(byteCodeBlob));
		
		UINT32 Part;
		VERIFY_D3D_RESULT_RETURN_FALSE(containerReflection->FindFirstPartKind(MAKEFOURCC('D', 'X', 'I', 'L'), &Part));

		TComPtr<ID3D12ShaderReflection> reflection;
		VERIFY_D3D_RESULT_RETURN_FALSE(containerReflection->GetPartReflection(Part, IID_PPV_ARGS(&reflection)));

		auto AddToParameterMap = [&](char const* name , ShaderParameterSlotInfo const& slot) -> ShaderParameter&
		{
			uint8 slotIndex = inOutSignature.slots.size();
			inOutSignature.slots.push_back(slot);
			auto& param = parameterMap.addParameter(name, slotIndex, 0, 0);
			return param;
		};

		inOutSignature.globalCBRegister = INDEX_NONE;
		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);
		for (int i = 0; i < shaderDesc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC bindDesc;
			reflection->GetResourceBindingDesc(i, &bindDesc);

			switch (bindDesc.Type)
			{
			case D3D_SIT_CBUFFER:
			case D3D_SIT_TBUFFER:
				{

					ID3D12ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByName(bindDesc.Name);
					assert(buffer);
					D3D12_SHADER_BUFFER_DESC bufferDesc;
					buffer->GetDesc(&bufferDesc);
					if (FCString::Compare(bufferDesc.Name, "$Globals") == 0)
					{
						ShaderParameterSlotInfo slot;
						slot.slotOffset = 0;
						//inOutSignature.descRanges.push_back(FD3D12Init::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, bindDesc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC));

						inOutSignature.globalCBRegister = bindDesc.BindPoint;
						inOutSignature.globalCBSize = bufferDesc.Size;

						for (int idxVar = 0; idxVar < bufferDesc.Variables; ++idxVar)
						{
							slot.type = ShaderParameterSlotInfo::eGlobalValue;

							ID3D12ShaderReflectionVariable* var = buffer->GetVariableByIndex(idxVar);
							if (var)
							{
								D3D12_SHADER_VARIABLE_DESC varDesc;
								var->GetDesc(&varDesc);		
								slot.dataOffset = varDesc.StartOffset;
								slot.dataSize = varDesc.Size;
								auto& param = AddToParameterMap(varDesc.Name, slot);

#if SHADER_DEBUG
								param.mbindType = EShaderParamBindType::Uniform;
#endif
								ID3D12ShaderReflectionType* varType = var->GetType();
								if (varType)
								{
									D3D12_SHADER_TYPE_DESC typeDesc;
									varType->GetDesc(&typeDesc);

									int lastOffset = varDesc.Size;
									for (int indexMember = typeDesc.Members - 1; indexMember >= 0; --indexMember)
									{
										ID3D12ShaderReflectionType* memberType = varType->GetMemberTypeByIndex(indexMember);
										D3D12_SHADER_TYPE_DESC memberTypeDesc;
										memberType->GetDesc(&memberTypeDesc);
										char const* memberName = varType->GetMemberTypeName(indexMember);

										InlineString<256> paramName;
										paramName.format("%s.%s", varDesc.Name, memberName);

										slot.dataOffset = varDesc.StartOffset + memberTypeDesc.Offset;
										slot.dataSize = lastOffset - memberTypeDesc.Offset;
										auto& param = AddToParameterMap(paramName, slot);
#if SHADER_DEBUG
										param.mbindType = EShaderParamBindType::Uniform;
#endif
										lastOffset = memberTypeDesc.Offset;
									}
								}
							}
						}
					}
					else
					{
						ShaderParameterSlotInfo slot;
						slot.slotOffset = inOutSignature.descRanges.size();
						inOutSignature.descRanges.push_back(FD3D12Init::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, bindDesc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC));
						//CBuffer TBuffer
						slot.type = ShaderParameterSlotInfo::eCVB;
						auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
						param.mbindType = EShaderParamBindType::UniformBuffer;
#endif
					}

					
				}
				break;

			case D3D_SIT_SAMPLER:
				{
#if 0
					auto& param = parameterMap.addParameter(bindDesc.Name, bindDesc.BindPoint, 0, bindDesc.BindCount);
					CHECK(bindDesc.BindCount == 1);

					D3D12_STATIC_SAMPLER_DESC sampler = {};
					sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
					sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
					sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
					sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
					sampler.MipLODBias = 0;
					sampler.MaxAnisotropy = 0;
					sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
					sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
					sampler.MinLOD = 0.0f;
					sampler.MaxLOD = D3D12_FLOAT32_MAX;
					sampler.ShaderRegister = bindDesc.BindPoint;
					sampler.RegisterSpace = 0;
					sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
					inOutSignature.samplers.push_back(sampler);
#else
					ShaderParameterSlotInfo slot;
					slot.slotOffset = inOutSignature.descRanges.size();
					inOutSignature.descRanges.push_back(FD3D12Init::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, bindDesc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE));
					slot.type = ShaderParameterSlotInfo::eSampler;
					auto& param = AddToParameterMap(bindDesc.Name, slot);
#endif

#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::Sampler;
#endif
				}
				break;
			case D3D_SIT_TEXTURE:
				{
					ShaderParameterSlotInfo slot;
					slot.slotOffset = inOutSignature.descRanges.size();
					inOutSignature.descRanges.push_back(FD3D12Init::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bindDesc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE));
					slot.type = ShaderParameterSlotInfo::eSRV;
					auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::Texture;
#endif
					
				}
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_STRUCTURED:
				{
					ShaderParameterSlotInfo slot;
					slot.slotOffset = inOutSignature.descRanges.size();
					inOutSignature.descRanges.push_back(FD3D12Init::DescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, bindDesc.BindPoint, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE));
					slot.type = ShaderParameterSlotInfo::eUAV;
					auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::StorageBuffer;
#endif
				}
				break;
			}
		}
		return true;
#else
		return false;
#endif
	}

	void D3D12Shader::SetupShader(ShaderRootSignature& inOutSignature, EShader::Type type)
	{
		auto visibility = D3D12Translate::ToVisibiltiy(type);
		inOutSignature.visibility = visibility;
		for (auto& sampler : inOutSignature.samplers)
		{
			sampler.ShaderVisibility = visibility;
		}

		if (inOutSignature.globalCBRegister != INDEX_NONE)
		{
			for (auto& slot : inOutSignature.slots)
			{
				if (slot.type != ShaderParameterSlotInfo::eGlobalValue)
				{
					slot.slotOffset += 1;	
				}
			}
		}
	}

	bool D3D12ShaderData::initialize(TComPtr<IDxcBlob>& shaderCode)
	{
		uint8 const* pCode = (uint8 const*)shaderCode->GetBufferPointer();
		code.assign(pCode, pCode + shaderCode->GetBufferSize());
		return true;
	}

	bool D3D12ShaderData::initialize(std::vector<uint8>&& binaryCode)
	{
		code = std::move(binaryCode);
		return true;
	}

	D3D12_ROOT_PARAMETER1 D3D12ShaderData::getRootParameter(int index, int num /*= 1*/) const
	{
		D3D12_ROOT_PARAMETER1 result;
		result.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		result.DescriptorTable.pDescriptorRanges = rootSignature.descRanges.data() + index;
		result.DescriptorTable.NumDescriptorRanges = num;
		result.ShaderVisibility = rootSignature.visibility;
		return result;
	}

}//namespace Render
