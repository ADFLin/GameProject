#pragma once

#ifndef D3D12ShaderCommon_H_D3F31F2B_0ADA_4339_AEEC_B308A897E7AA
#define D3D12ShaderCommon_H_D3F31F2B_0ADA_4339_AEEC_B308A897E7AA

#include "ShaderCore.h"
#include "ShaderFormat.h"
#include "RHICommon.h"

#include "Platform/Windows/ComUtility.h"
#include <d3d12.h>

class IDxcLibrary;
class IDxcCompiler;
class IDxcBlob;

namespace Render
{

	class D3D12ShaderProgram : public TRefcountResource< RHIShaderProgram >
	{
	public:
		D3D12_SHADER_BYTECODE getByteCode(int index) { return { mShaders[index].code.data() , mShaders[index].code.size() }; }
		struct ShaderInfo
		{
			EShader::Type type;
			std::vector< uint8 > code;
		};
		ShaderPorgramParameterMap mParameterMap;
		ShaderInfo mShaders[EShader::MaxStorageSize];
		int mNumShaders;
	};

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
		std::vector< D3D12_DESCRIPTOR_RANGE1 >   descRanges;
		std::vector< D3D12_STATIC_SAMPLER_DESC > samplers;
		std::vector< ShaderParameterSlotInfo >   Slots;
	};

	class D3D12Shader : public TRefcountResource< RHIShader >
	{
	public:
		D3D12Shader(EShader::Type type)
		{
			initType(type);
		}

		bool initialize(TComPtr<IDxcBlob>& shaderCode);
		bool initialize(std::vector<uint8>&& binaryCode);

		D3D12_SHADER_BYTECODE getByteCode() { return { code.data() , code.size() }; }
		D3D12_ROOT_PARAMETER1 getRootParameter(int index , int num = 1) const
		{
			D3D12_ROOT_PARAMETER1 result;
			result.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			result.DescriptorTable.pDescriptorRanges = mRootSignature.descRanges.data() + index;
			result.DescriptorTable.NumDescriptorRanges = num;
			result.ShaderVisibility = mRootSignature.visibility;
			return result;
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
		virtual char const* getStructParameterName(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo) { return structInfo.variableName; }

		ShaderParameterMap   mParameterMap;
		ShaderRootSignature  mRootSignature;
		std::vector<uint8>   code;
	};

	class ShaderFormatHLSL_D3D12 : public ShaderFormat
	{
	public:
		ShaderFormatHLSL_D3D12(TComPtr< ID3D12Device8 >& inDevice);
		~ShaderFormatHLSL_D3D12();
		virtual char const* getName() final { return "hlsl"; }
		virtual void setupShaderCompileOption(ShaderCompileOption& option) final;
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) final;
		virtual bool compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output) final;

		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) final
		{
			return false;
		}
		virtual bool initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode) final
		{
			return false;
		}

		virtual void postShaderLoaded(ShaderProgram& shaderProgram) final
		{

		}

		virtual bool isSupportBinaryCode() const final { return true; }
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode) final
		{
			return false;
		}


		TComPtr<ID3D12Device8> mDevice;
		TComPtr<IDxcLibrary>  mLibrary;
		TComPtr<IDxcCompiler> mCompiler;


		bool ensureDxcObjectCreation();


		virtual bool initializeShader(Shader& shader, ShaderSetupData& setupData) override;
		virtual bool initializeShader(Shader& shader, ShaderCompileInfo const& shaderCompile, std::vector<uint8> const& binaryCode) override;


		virtual void postShaderLoaded(Shader& shader) override
		{

		}

		virtual bool getBinaryCode(Shader& shader, ShaderSetupData& setupData, std::vector<uint8>& outBinaryCode) override;


		virtual void precompileCode(ShaderProgramSetupData& setupData) override;


		virtual void precompileCode(ShaderSetupData& setupData) override;

	};

}//namespace Render



#endif // D3D12ShaderCommon_H_D3F31F2B_0ADA_4339_AEEC_B308A897E7AA
