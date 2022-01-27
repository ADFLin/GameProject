#pragma once

#ifndef D3D12ShaderCommon_H_D3F31F2B_0ADA_4339_AEEC_B308A897E7AA
#define D3D12ShaderCommon_H_D3F31F2B_0ADA_4339_AEEC_B308A897E7AA

#include "ShaderCore.h"
#include "ShaderFormat.h"
#include "RHICommon.h"

#include "Platform/Windows/ComUtility.h"
#include <d3d12.h>

#include "RHICommand.h"

class IDxcLibrary;
class IDxcCompiler;
class IDxcBlob;

namespace Render
{


	struct ShaderParameterSlotInfo
	{
		enum EType : uint8
		{
			eGlobalValue ,
			eCVB ,
			eSRV ,
			eUAV ,
			eSampler ,
		};

		EType  type;
		uint8  slotOffset;
		uint16 dataOffset;
		uint32 dataSize;

	};

	struct ShaderRootSignature 
	{
		D3D12_SHADER_VISIBILITY visibility;
		int    globalCBRegister = INDEX_NONE;
		uint32 globalCBSize = 0;
		std::vector< D3D12_DESCRIPTOR_RANGE1 >   descRanges;
		std::vector< D3D12_STATIC_SAMPLER_DESC > samplers;
		std::vector< ShaderParameterSlotInfo >   slots;
	};

	struct D3D12ShaderData
	{
		ShaderRootSignature  rootSignature;
		std::vector<uint8>   code;

		bool initialize(TComPtr<IDxcBlob>& shaderCode);
		bool initialize(std::vector<uint8>&& binaryCode);

		D3D12_SHADER_BYTECODE getByteCode() { return { code.data() , code.size() }; }
		D3D12_ROOT_PARAMETER1 getRootParameter(int index, int num = 1) const;

	};

	class D3D12Shader : public TRefcountResource< RHIShader >
		              , public D3D12ShaderData
	{
	public:
		D3D12Shader(EShader::Type type)
		{
			initType(type);
		}


		static bool GenerateParameterMap(std::vector< uint8 > const& byteCode, TComPtr<IDxcLibrary>& library, ShaderParameterMap& parameterMap , ShaderRootSignature& inOutSignature);
		static void SetupShader(ShaderRootSignature& inOutSignature, EShader::Type type);
		
		virtual bool getParameter(char const* name, ShaderParameter& outParam)
		{
			return outParam.bind(mParameterMap, name);
		}

		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam)
		{
			return outParam.bind(mParameterMap, name);
		}

		virtual char const* getStructParameterName(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo) 
		{
			if (resourceType == EShaderResourceType::Storage)
			{
				return structInfo.variableName;
			}
			return structInfo.blockName;
		}

		ShaderParameterMap   mParameterMap;
	};

	class D3D12ShaderProgram : public TRefcountResource< RHIShaderProgram >
	{
	public:
		virtual bool getParameter(char const* name, ShaderParameter& outParam)
		{
			return outParam.bind(mParameterMap, name);
		}
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam)
		{
			return outParam.bind(mParameterMap, name);
		}
		virtual char const* getStructParameterName(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo)
		{
			if (resourceType == EShaderResourceType::Storage)
			{
				return structInfo.variableName;
			}
			return structInfo.blockName;
		}

		void initializeParameterMap(TComPtr<IDxcLibrary>& library)
		{
			mParameterMap.clear();
			for (int i = 0; i < mNumShaders; ++i)
			{
				auto& shaderData = mShaderDatas[i];
				ShaderParameterMap parameterMap;
				D3D12Shader::GenerateParameterMap(shaderData.code, library, parameterMap, shaderData.rootSignature);
				D3D12Shader::SetupShader(shaderData.rootSignature, shaderData.type);
				mParameterMap.addShaderParameterMap(i, parameterMap);
			}

			mParameterMap.finalizeParameterMap();
		}

		template< class TFunc >
		void setupShader(ShaderParameter const& parameter, TFunc&& func)
		{
			mParameterMap.setupShader(parameter,
				[this, &func](int shaderIndex, ShaderParameter const& shaderParam)
				{
					func(mShaderDatas[shaderIndex].type, mShaderDatas[shaderIndex], shaderParam);
				}
			);
		}
		virtual void releaseResource()
		{
			delete[] mShaderDatas;
		}

		void initializeData(int numShaders)
		{
			CHECK(mShaderDatas == nullptr);
			mNumShaders = numShaders;
			mShaderDatas = new ShaderData[numShaders];
		}


		class D3D12ShaderBoundState* cachedState = nullptr;
		ShaderPorgramParameterMap mParameterMap;
		struct ShaderData : D3D12ShaderData
		{
			EShader::Type type;
		};
		ShaderData* mShaderDatas = nullptr;
		int mNumShaders = 0;
	};

	class ShaderFormatHLSL_D3D12 final : public ShaderFormat
	{
	public:
		ShaderFormatHLSL_D3D12(TComPtr< ID3D12Device8 >& inDevice);
		~ShaderFormatHLSL_D3D12();
		virtual char const* getName() final { return "hlsl_dxc"; }
		virtual void setupShaderCompileOption(ShaderCompileOption& option);
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry);
		virtual bool compileCode(ShaderCompileContext const& context);

		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData);
		virtual bool initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileDesc > const& descList, std::vector<uint8> const& binaryCode);

		virtual void postShaderLoaded(ShaderProgram& shaderProgram)
		{

		}

		virtual bool doesSuppurtBinaryCode() const { return true; }
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode);
		
		TComPtr<ID3D12Device8> mDevice;
#if TARGET_PLATFORM_64BITS
		TComPtr<IDxcLibrary>   mLibrary;
		TComPtr<IDxcCompiler>  mCompiler;
#endif

		bool ensureDxcObjectCreation();


		virtual bool initializeShader(Shader& shader, ShaderSetupData& setupData) override;
		virtual bool initializeShader(Shader& shader, ShaderCompileDesc const& desc, std::vector<uint8> const& binaryCode) override;


		virtual void postShaderLoaded(Shader& shader) override
		{

		}

		virtual bool getBinaryCode(Shader& shader, ShaderSetupData& setupData, std::vector<uint8>& outBinaryCode) override;
		virtual void precompileCode(ShaderProgramSetupData& setupData) override;
		virtual void precompileCode(ShaderSetupData& setupData) override;

	};

}//namespace Render



#endif // D3D12ShaderCommon_H_D3F31F2B_0ADA_4339_AEEC_B308A897E7AA
