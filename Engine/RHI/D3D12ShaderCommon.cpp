#include "D3D12ShaderCommon.h"

#include "D3D12Utility.h"
#include "D3D12Common.h"
#include "D3DSharedCommon.h"

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

	ShaderFormatHLSL_D3D12::ShaderFormatHLSL_D3D12(TComPtr< ID3D12DeviceRHI >& inDevice) :mDevice(inDevice)
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

	EShaderCompileResult ShaderFormatHLSL_D3D12::compileCode(ShaderCompileContext const& context)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_FAILCODE(ensureDxcObjectCreation(), return EShaderCompileResult::ResourceError);

		CHECK(context.codes.size() == 1);

		uint32_t codePage = CP_UTF8;
		TComPtr<IDxcBlobEncoding> sourceBlob;
		VERIFY_D3D_RESULT(mLibrary->CreateBlobWithEncodingFromPinned(context.codes[0].data(), (uint32)context.codes[0].size(), codePage, &sourceBlob),
			return EShaderCompileResult::ResourceError; );

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
		// Ray Tracing
		case EShader::RayGen:
		case EShader::RayMiss:
		case EShader::RayClosestHit:
		case EShader::RayAnyHit:
		case EShader::RayIntersection: 
		case EShader::Callable:
			profileName = L"lib_6_3"; break;
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
			FCString::CharToWChar(fileName).c_str(), // pSourceName
			FCString::CharToWChar(context.getEntry()).c_str(), // pEntryPoint
			profileName, // pTargetProfile
			args, numArgs, // pArguments, argCount
			NULL, 0, // pDefines, defineCount
			NULL, // pIncludeHandler
			&compileResult); // ppResult

		bool bSuccess = true;
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
			context.checkOuputDebugCode();
			TComPtr<IDxcBlobEncoding> errorsBlob;
			hr = compileResult->GetErrorBuffer(&errorsBlob);
			emitCompileError(context, (LPCSTR)errorsBlob->GetBufferPointer());
			return EShaderCompileResult::CodeError;
		}

		TComPtr<IDxcBlob> shaderCode;
		compileResult->GetResult(&shaderCode);

		if (context.programSetupData)
		{
			auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(*context.programSetupData->resource);
			auto& shaderData = shaderProgramImpl.mShaderDatas[context.shaderIndex];

			if (!shaderData.initialize(shaderCode))
				return EShaderCompileResult::ResourceError;
			shaderData.type = context.getType();
			shaderData.entryPoint = context.getEntry();
		}
		else
		{
			auto& shaderImpl = static_cast<D3D12Shader&>(*context.shaderSetupData->resource);
			if (!shaderImpl.initialize(shaderCode))
			{
				LogWarning(0, "Can't create shader resource");
				return EShaderCompileResult::ResourceError;
			}
			shaderImpl.entryPoint = context.getEntry();
		}

		return EShaderCompileResult::Ok;
#else
		return EShaderCompileResult::ResourceError;
#endif
	}

	bool ShaderFormatHLSL_D3D12::getBinaryCode(ShaderSetupData& setupData, TArray<uint8>& outBinaryCode)
	{
		D3D12Shader& shaderImpl = static_cast<D3D12Shader&>(*setupData.resource);
		outBinaryCode = shaderImpl.code;
		return true;
	}

	bool ShaderFormatHLSL_D3D12::getBinaryCode(ShaderProgramSetupData& setupData, TArray<uint8>& outBinaryCode)
	{
		D3D12ShaderProgram& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(*setupData.resource);
		auto serializer = CreateBufferSerializer<ArrayWriteBuffer>(outBinaryCode);
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
		shaderProgramImpl.initializeData(setupData.descList.size());
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

	ShaderParameterMap* ShaderFormatHLSL_D3D12::initializeShader(RHIShader& shader, ShaderSetupData& setupData)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());

		D3D12Shader& shaderImpl = static_cast<D3D12Shader&>(shader);

		D3D12Shader::GenerateParameterMap(shaderImpl.mType, shaderImpl.entryPoint.c_str(), shaderImpl.code, mLibrary, shaderImpl.mParameterMap, shaderImpl.rootSignature);
		return &shaderImpl.mParameterMap;
#else
		return nullptr;
#endif
	}

	ShaderParameterMap* ShaderFormatHLSL_D3D12::initializeShader(RHIShader& shader, ShaderCompileDesc const& desc, TArray<uint8> const& binaryCode)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());

		D3D12Shader& shaderImpl = static_cast<D3D12Shader&>(shader);
		TArray<uint8> temp = binaryCode;
		if (!shaderImpl.initialize(std::move(temp)))
			return nullptr;

		D3D12Shader::GenerateParameterMap(shaderImpl.mType, shaderImpl.entryPoint.c_str(), shaderImpl.code, mLibrary, shaderImpl.mParameterMap, shaderImpl.rootSignature);
		return &shaderImpl.mParameterMap;;
#else
		return nullptr;
#endif
	}

	ShaderParameterMap* ShaderFormatHLSL_D3D12::initializeProgram(RHIShaderProgram& shaderProgram, ShaderProgramSetupData& setupData)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());

		auto& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);
		shaderProgramImpl.mParameterMap.clear();
		shaderProgramImpl.initializeParameterMap(mLibrary);
		return &shaderProgramImpl.mParameterMap;
#else
		return nullptr;
#endif
	}

	ShaderParameterMap* ShaderFormatHLSL_D3D12::initializeProgram(RHIShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& descList, TArray<uint8> const& binaryCode)
	{
#if TARGET_PLATFORM_64BITS
		VERIFY_RETURN_FALSE(ensureDxcObjectCreation());

		D3D12ShaderProgram& shaderProgramImpl = static_cast<D3D12ShaderProgram&>(shaderProgram);

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

			auto& shaderData = shaderProgramImpl.mShaderDatas[i];
			shaderData.type = EShader::Type(shaderType);
			shaderData.initialize(std::move(byteCode));
		}

		shaderProgramImpl.initializeParameterMap(mLibrary);
		return &shaderProgramImpl.mParameterMap;
#else
		return nullptr;
#endif
	}



	template< typename TFunc >
	void AddCBVariablesToParameterMap(ID3D12ShaderReflectionConstantBuffer* buffer, ShaderParameterSlotInfo const& inSlot, ShaderParameterMap& parameterMap, TFunc&& AddToParameterMap)
	{
		D3D12_SHADER_BUFFER_DESC bufferDesc;
		buffer->GetDesc(&bufferDesc);

		ShaderParameterSlotInfo slot = inSlot;
		for (int idxVar = 0; idxVar < (int)bufferDesc.Variables; ++idxVar)
		{
			ID3D12ShaderReflectionVariable* var = buffer->GetVariableByIndex(idxVar);
			if (var)
			{
				D3D12_SHADER_VARIABLE_DESC varDesc;
				var->GetDesc(&varDesc);
				slot.dataOffset = inSlot.dataOffset + varDesc.StartOffset;
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

						slot.dataOffset = inSlot.dataOffset + varDesc.StartOffset + memberTypeDesc.Offset;
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

	bool D3D12Shader::GenerateParameterMap(EShader::Type type, char const* entryName, TArray< uint8 > const& byteCode, TComPtr<IDxcLibrary>& library, ShaderParameterMap& parameterMap, ShaderRootSignature& inOutSignature)
	{
		auto visibility = D3D12Translate::ToVisibiltiy(type);
		inOutSignature.visibility = visibility;

#if TARGET_PLATFORM_64BITS
		TComPtr<IDxcContainerReflection> containerReflection;
		VERIFY_D3D_RESULT_RETURN_FALSE(DxcCreateInstance(CLSID_DxcContainerReflection, IID_PPV_ARGS(&containerReflection)));

		TComPtr<IDxcBlobEncoding> byteCodeBlob;
		VERIFY_D3D_RESULT_RETURN_FALSE(library->CreateBlobWithEncodingFromPinned(byteCode.data(), byteCode.size(), 0, &byteCodeBlob));

		VERIFY_D3D_RESULT_RETURN_FALSE(containerReflection->Load(byteCodeBlob));

		UINT32 Part;
		VERIFY_D3D_RESULT_RETURN_FALSE(containerReflection->FindFirstPartKind(MAKEFOURCC('D', 'X', 'I', 'L'), &Part));



		auto AddToParameterMap = [&](char const* name, ShaderParameterSlotInfo const& slot) -> ShaderParameter&
		{
			uint8 slotIndex = (uint8)inOutSignature.slots.size();
			inOutSignature.slots.push_back(slot);
			auto& param = parameterMap.addParameter(name, slotIndex, 0, 0);
			LogMsg("  [%s] Type: %d, Slot: %d, DataOffset: %u, Size: %u, Local: %d", 
				name, (int)slot.type, (int)slot.slotOffset, slot.dataOffset, slot.dataSize, (int)slot.bLocal);
			return param;
		};

		bool const bIsRayTracing = EShader::IsRayTracing(type);
		D3D12_SHADER_VISIBILITY const rtVisibility = D3D12_SHADER_VISIBILITY_ALL;

		TComPtr<ID3D12ShaderReflection> reflection;
		TComPtr<ID3D12LibraryReflection> libReflection;

		if (bIsRayTracing)
		{
			containerReflection->GetPartReflection(Part, IID_PPV_ARGS(&libReflection));
		}
		else
		{
			containerReflection->GetPartReflection(Part, IID_PPV_ARGS(&reflection));
		}

		inOutSignature.globalCBRegister = INDEX_NONE;

		auto CheckIsGlobalCB = [&](char const* name, bool bLocal)
		{
			return FCString::Compare(name, "$Globals") == 0 || FCString::Compare(name, "__Globals") == 0;
		};

		// Pre-scan for Global Constant Buffer to reserve slot 0
		bool bFoundGlobals = false;
		ID3D12FunctionReflection* targetFuncRef = nullptr;

		bool bScanAll = false;

		if (libReflection)
		{
			auto ScanFunc = [&](ID3D12FunctionReflection* funcRef)
			{
				D3D12_FUNCTION_DESC funcDesc;
				funcRef->GetDesc(&funcDesc);
				for (uint32 j = 0; j < funcDesc.BoundResources; ++j)
				{
					D3D12_SHADER_INPUT_BIND_DESC bindDesc;
					funcRef->GetResourceBindingDesc(j, &bindDesc);
					if (bindDesc.Type == D3D_SIT_CBUFFER)
					{
						bool bLocal = (bindDesc.Space == 1);
						if (CheckIsGlobalCB(bindDesc.Name, bLocal) && !bFoundGlobals)
						{
							D3D12_ROOT_PARAMETER1 parameter = {};
							parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							parameter.Descriptor.ShaderRegister = bindDesc.BindPoint;
							parameter.Descriptor.RegisterSpace = bindDesc.Space;
							inOutSignature.parameters.push_back(parameter);
							inOutSignature.descRanges.push_back({}); // 1:1 dummy
							inOutSignature.parameterNames.push_back(bindDesc.Name);
							bFoundGlobals = true;
						}
					}
				}
			};

			D3D12_LIBRARY_DESC libDesc;
			libReflection->GetDesc(&libDesc);
			for (uint32 i = 0; i < libDesc.FunctionCount; ++i)
			{
				ScanFunc(libReflection->GetFunctionByIndex(i));
			}
		}
		else if (reflection)
		{
			D3D12_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);
			for (int i = 0; i < shaderDesc.ConstantBuffers; ++i)
			{
				ID3D12ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByIndex(i);
				D3D12_SHADER_BUFFER_DESC bufferDesc;
				buffer->GetDesc(&bufferDesc);
				if (CheckIsGlobalCB(bufferDesc.Name, false))
				{
					D3D12_ROOT_PARAMETER1 parameter = {};
					parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
					// For standard shaders, globals are usually at b0
					parameter.Descriptor.ShaderRegister = 0; 
					inOutSignature.parameters.push_back(parameter);
					inOutSignature.descRanges.push_back({}); // 1:1 dummy
					inOutSignature.parameterNames.push_back(bufferDesc.Name);
					bFoundGlobals = true;
					break;
				}
			}
		}

		int slotOffset = (int)inOutSignature.parameters.size();
		int slotOffsetLocal = (int)inOutSignature.localParameters.size();
		TArray<std::string> processedNames;

		auto ProcessResource = [&](D3D12_SHADER_INPUT_BIND_DESC const& bindDesc, ID3D12ShaderReflectionConstantBuffer* buffer)
		{
			bool bLocal = (bindDesc.Space == 1);
			if (bIsRayTracing)
			{
				for (auto const& name : processedNames)
				{
					if (name == bindDesc.Name) 
						return;
				}
				processedNames.push_back(bindDesc.Name);
			}

			int* pcurrentSlotOffset = bLocal ? &slotOffsetLocal : &slotOffset;
			auto* pcurrentParameters = (bIsRayTracing && bLocal) ? &inOutSignature.localParameters : &inOutSignature.parameters;
			auto* pcurrentDescRanges = (bIsRayTracing && bLocal) ? &inOutSignature.localDescRanges : &inOutSignature.descRanges;
			auto* pcurrentNames = (bIsRayTracing && bLocal) ? &inOutSignature.localParameterNames : &inOutSignature.parameterNames;
			D3D12_SHADER_VISIBILITY currentVisibility = bIsRayTracing ? rtVisibility : visibility;

			switch (bindDesc.Type)
			{
			case D3D_SIT_CBUFFER:
				{
					if (CheckIsGlobalCB(bindDesc.Name, bLocal))
					{
						inOutSignature.globalCBRegister = bindDesc.BindPoint;
						if (buffer)
						{
							D3D12_SHADER_BUFFER_DESC bufferDesc;
							buffer->GetDesc(&bufferDesc);
							inOutSignature.globalCBSize = bufferDesc.Size;
						}

						D3D12_ROOT_PARAMETER1* pParameter = bLocal ? &inOutSignature.localParameters[0] : &inOutSignature.parameters[0];
						pParameter->ShaderVisibility = currentVisibility;
						pParameter->ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
						pParameter->Descriptor.ShaderRegister = bindDesc.BindPoint;
						pParameter->Descriptor.RegisterSpace = bindDesc.Space;

						ShaderParameterSlotInfo slot;
						slot.slotOffset = 0;
						slot.type = ShaderParameterSlotInfo::eGlobalValue;
						slot.dataOffset = 0;
						slot.bLocal = bLocal;

						if (buffer)
						{
							AddCBVariablesToParameterMap(buffer, slot, parameterMap, AddToParameterMap);
						}
					}
					else 
					{
						ShaderParameterSlotInfo slot;
						slot.bLocal = bLocal;
						slot.slotOffset = (*pcurrentSlotOffset)++;
						slot.dataOffset = 0;
						slot.dataSize = 0;

						D3D12_ROOT_PARAMETER1 parameter = {};
						parameter.ShaderVisibility = currentVisibility;
						if (bIsRayTracing && bLocal && FCString::Compare(bindDesc.Name, "__LocalConstants") == 0)
						{
							parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
							parameter.Constants.ShaderRegister = bindDesc.BindPoint;
							parameter.Constants.RegisterSpace = bindDesc.Space;
							uint32 bufferSize = 0;
							if (buffer)
							{
								D3D12_SHADER_BUFFER_DESC bufferDesc;
								buffer->GetDesc(&bufferDesc);
								bufferSize = bufferDesc.Size;
							}
							parameter.Constants.Num32BitValues = bufferSize / 4;
							slot.type = ShaderParameterSlotInfo::eGlobalValue;
							slot.dataOffset = 0;
							slot.dataSize = bufferSize;

							if (buffer)
							{
								AddCBVariablesToParameterMap(buffer, slot, parameterMap, AddToParameterMap);
							}
						}
						else
						{
							parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							parameter.Descriptor.ShaderRegister = bindDesc.BindPoint;
							parameter.Descriptor.RegisterSpace = bindDesc.Space;
							slot.type = ShaderParameterSlotInfo::eCBV;
							slot.dataSize = 0;
						}
						pcurrentParameters->push_back(parameter);
						pcurrentDescRanges->push_back({}); // Support 1:1 mapping
						pcurrentNames->push_back(bindDesc.Name);

						auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
						param.mbindType = EShaderParamBindType::UniformBuffer;
#endif
					}
				}
				break;
			case D3D_SIT_TBUFFER:
				{
					ShaderParameterSlotInfo slot;
					slot.bLocal = bLocal;
					slot.slotOffset = (*pcurrentSlotOffset)++;
					slot.type = ShaderParameterSlotInfo::eDescriptorTable_SRV;
					slot.dataOffset = 0;
					slot.dataSize = 0;

					D3D12_DESCRIPTOR_RANGE1 range = {};
					range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					range.NumDescriptors = 1;
					range.BaseShaderRegister = bindDesc.BindPoint;
					range.RegisterSpace = bindDesc.Space;
					range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
					range.OffsetInDescriptorsFromTableStart = 0;
					pcurrentDescRanges->push_back(range);
 
 					D3D12_ROOT_PARAMETER1 parameter = {};
 					parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
 					parameter.DescriptorTable.pDescriptorRanges = nullptr;
 					parameter.DescriptorTable.NumDescriptorRanges = 1;
 					parameter.ShaderVisibility = currentVisibility;
					pcurrentParameters->push_back(parameter);
					pcurrentNames->push_back(bindDesc.Name);

					auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::TextureBuffer;
#endif
				}
				break;
			case D3D_SIT_SAMPLER:
				{
					ShaderParameterSlotInfo slot;
					slot.bLocal = bLocal;
					slot.slotOffset = (*pcurrentSlotOffset)++;
					slot.type = ShaderParameterSlotInfo::eSampler;
					slot.dataOffset = 0;
					slot.dataSize = 0;

					D3D12_DESCRIPTOR_RANGE1 range = {};
					range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
					range.NumDescriptors = 1;
					range.BaseShaderRegister = bindDesc.BindPoint;
					range.RegisterSpace = bindDesc.Space;
					range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
					range.OffsetInDescriptorsFromTableStart = 0;
					pcurrentDescRanges->push_back(range);
 
 					D3D12_ROOT_PARAMETER1 parameter = {};
 					parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
 					parameter.DescriptorTable.pDescriptorRanges = nullptr;
 					parameter.DescriptorTable.NumDescriptorRanges = 1;
 					parameter.ShaderVisibility = currentVisibility;
					pcurrentParameters->push_back(parameter);
					pcurrentNames->push_back(bindDesc.Name);

					auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::Sampler;
#endif
				}
				break;
			case D3D_SIT_TEXTURE:
				{
					ShaderParameterSlotInfo slot;
					slot.bLocal = bLocal;
					slot.slotOffset = (*pcurrentSlotOffset)++;
					slot.type = ShaderParameterSlotInfo::eDescriptorTable_SRV;
					slot.dataOffset = 0;
					slot.dataSize = 0;

					D3D12_DESCRIPTOR_RANGE1 range = {};
					range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					range.NumDescriptors = 1;
					range.BaseShaderRegister = bindDesc.BindPoint;
					range.RegisterSpace = bindDesc.Space;
					range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
					range.OffsetInDescriptorsFromTableStart = 0;
					pcurrentDescRanges->push_back(range);
 
 					D3D12_ROOT_PARAMETER1 parameter = {};
 					parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
 					parameter.DescriptorTable.pDescriptorRanges = nullptr;
 					parameter.DescriptorTable.NumDescriptorRanges = 1;
 					parameter.ShaderVisibility = currentVisibility;
					pcurrentParameters->push_back(parameter);
					pcurrentNames->push_back(bindDesc.Name);

					auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::Texture;
#endif
				}
				break;
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_BYTEADDRESS:
			case (D3D_SHADER_INPUT_TYPE)12: // D3D_SIT_RTACCELERATIONSTRUCTURE
				{
					ShaderParameterSlotInfo slot;
					slot.bLocal = bLocal;
					slot.slotOffset = (*pcurrentSlotOffset)++;
					slot.type = ShaderParameterSlotInfo::eSRV;
					slot.dataOffset = 0;
					slot.dataSize = 0;

					D3D12_ROOT_PARAMETER1 parameter = {};
					parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
					parameter.Descriptor.ShaderRegister = bindDesc.BindPoint;
					parameter.Descriptor.RegisterSpace = bindDesc.Space;
					parameter.ShaderVisibility = currentVisibility;
					pcurrentParameters->push_back(parameter);
 
					pcurrentDescRanges->push_back({}); // Support 1:1 mapping
					pcurrentNames->push_back(bindDesc.Name);
					auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::StorageBuffer;
#endif
				}
				break;
			case D3D_SIT_UAV_RWTYPED:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_RWBYTEADDRESS:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
				{
					ShaderParameterSlotInfo slot;
					slot.bLocal = bLocal;
					slot.slotOffset = (*pcurrentSlotOffset)++;
					slot.dataOffset = 0;
					slot.dataSize = 0;
					
					if (bindDesc.Type == D3D_SIT_UAV_RWTYPED)
					{
						slot.type = ShaderParameterSlotInfo::eDescriptorTable_UAV;
						D3D12_DESCRIPTOR_RANGE1 range = {};
						range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
						range.NumDescriptors = 1;
						range.BaseShaderRegister = bindDesc.BindPoint;
						range.RegisterSpace = bindDesc.Space;
						range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
						range.OffsetInDescriptorsFromTableStart = 0;
						pcurrentDescRanges->push_back(range);
 
 						D3D12_ROOT_PARAMETER1 parameter = {};
 						parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
 						parameter.DescriptorTable.pDescriptorRanges = nullptr;
 						parameter.DescriptorTable.NumDescriptorRanges = 1;
 						parameter.ShaderVisibility = currentVisibility;
						pcurrentParameters->push_back(parameter);
						pcurrentNames->push_back(bindDesc.Name);
					}
					else
					{
						slot.type = ShaderParameterSlotInfo::eUAV;
						D3D12_ROOT_PARAMETER1 parameter = {};
						parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
						parameter.Descriptor.ShaderRegister = bindDesc.BindPoint;
						parameter.Descriptor.RegisterSpace = bindDesc.Space;
						parameter.ShaderVisibility = currentVisibility;
						pcurrentParameters->push_back(parameter);
						pcurrentDescRanges->push_back({}); // Support 1:1 mapping
						pcurrentNames->push_back(bindDesc.Name);
					}

					auto& param = AddToParameterMap(bindDesc.Name, slot);
#if SHADER_DEBUG
					param.mbindType = EShaderParamBindType::StorageBuffer;
#endif
				}
				break;
			default:
				break;
			}
		};

		if (libReflection)
		{
			auto ProcessFunc = [&](ID3D12FunctionReflection* funcRef)
			{
				D3D12_FUNCTION_DESC funcDesc;
				funcRef->GetDesc(&funcDesc);
				for (uint32 j = 0; j < funcDesc.BoundResources; ++j)
				{
					D3D12_SHADER_INPUT_BIND_DESC bindDesc;
					funcRef->GetResourceBindingDesc(j, &bindDesc);
					ID3D12ShaderReflectionConstantBuffer* buffer = nullptr;
					if (bindDesc.Type == D3D_SIT_CBUFFER)
					{
						buffer = funcRef->GetConstantBufferByName(bindDesc.Name);
					}
					ProcessResource(bindDesc, buffer);
				}
			};

			if (targetFuncRef && !bScanAll)
			{
				ProcessFunc(targetFuncRef);
			}
			else
			{
				D3D12_LIBRARY_DESC libDesc;
				libReflection->GetDesc(&libDesc);
				for (uint32 i = 0; i < libDesc.FunctionCount; ++i)
				{
					ProcessFunc(libReflection->GetFunctionByIndex(i));
				}
			}
		}
		else if (reflection)
		{
			D3D12_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);
			for (int i = 0; i < shaderDesc.BoundResources; ++i)
			{
				D3D12_SHADER_INPUT_BIND_DESC bindDesc;
				reflection->GetResourceBindingDesc(i, &bindDesc);
				ID3D12ShaderReflectionConstantBuffer* buffer = nullptr;
				if (bindDesc.Type == D3D_SIT_CBUFFER)
				{
					buffer = reflection->GetConstantBufferByName(bindDesc.Name);
				}
				ProcessResource(bindDesc, buffer);
			}
		}

		// Final Pass: Fix up descriptor table pointers
		for (int i = 0; i < (int)inOutSignature.parameters.size(); ++i)
		{
			auto& parameter = inOutSignature.parameters[i];
			if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				parameter.DescriptorTable.pDescriptorRanges = &inOutSignature.descRanges[i];
			}
		}
		for (int i = 0; i < (int)inOutSignature.localParameters.size(); ++i)
		{
			auto& parameter = inOutSignature.localParameters[i];
			if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				parameter.DescriptorTable.pDescriptorRanges = &inOutSignature.localDescRanges[i];
			}
		}

		if (bIsRayTracing)
		{
			// Calc local data offsets
			uint32 localDataSize = 0;
			for (int i = 0; i < (int)inOutSignature.localParameters.size(); ++i)
			{
				auto& parameter = inOutSignature.localParameters[i];
				if (parameter.ParameterType != D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					localDataSize = Math::AlignUp(localDataSize, 8u);
				else
					localDataSize = Math::AlignUp(localDataSize, 4u);

				uint32 entryOffset = localDataSize;
				for (auto& slot : inOutSignature.slots)
				{
					if (slot.bLocal && slot.slotOffset == i)
					{
						if (slot.type == ShaderParameterSlotInfo::eGlobalValue)
							slot.dataOffset += entryOffset;
						else
							slot.dataOffset = entryOffset;
					}
				}

				if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
					localDataSize += parameter.Constants.Num32BitValues * 4;
				else
					localDataSize += 8;
			}
			inOutSignature.localDataSize = localDataSize;
		}

		return true;
#else
		return false;
#endif
	}

	bool D3D12ShaderData::initialize(TComPtr<IDxcBlob>& shaderCode)
	{
		uint8 const* pCode = (uint8 const*)shaderCode->GetBufferPointer();
		code.assign(pCode, pCode + shaderCode->GetBufferSize());
		return true;
	}

	bool D3D12ShaderData::initialize(TArray<uint8>&& binaryCode)
	{
		code = std::move(binaryCode);
		return true;
	}

}//namespace Render
