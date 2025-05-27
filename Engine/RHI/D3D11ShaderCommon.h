#pragma once
#ifndef D3D11Shader_H_9211AFBF_3162_4D63_A2AC_097ED054E79B
#define D3D11Shader_H_9211AFBF_3162_4D63_A2AC_097ED054E79B

#include "ShaderCore.h"
#include "ShaderFormat.h"
#include "RHICommon.h"

#include "Template/ArrayView.h"
#include "DataStructure/Array.h"
#include "Platform/Windows/ComUtility.h"

#include <D3D11.h>
#include <D3Dcompiler.h>

//#include <D3DX11async.h>
#pragma comment(lib , "D3dcompiler.lib")

namespace Render
{
	struct D3D11ShaderData
	{
		EShader::Type type;
		union
		{
			ID3D11DeviceChild*    ptr;
			ID3D11VertexShader*   vertex;
			ID3D11PixelShader*    pixel;
			ID3D11GeometryShader* geometry;
			ID3D11ComputeShader*  compute;
			ID3D11HullShader*     hull;
			ID3D11DomainShader*   domain;
		};
		D3D11ShaderData()
		{
			ptr = nullptr;
			globalConstBufferSize = 0;
		}

		int globalConstBufferSize;

		bool initialize(EShader::Type inType, TComPtr<ID3D11Device>& device, TArrayView< uint8 const > code);
		void release();
	};

	struct D3DShaderParamInfo
	{
		int globalConstBufferSize = 0;
	};

	class D3D11Shader : public TRefcountResource< RHIShader >
	{
	public:
		D3D11Shader(EShader::Type type)
		{
			initType(type);
			mResource.ptr = nullptr;
		}

		virtual void releaseResource()
		{
			mResource.release();
		}

		virtual bool initialize(TComPtr<ID3D11Device>& device, TArrayView<uint8 const>& inByteCode);
		virtual bool initialize(TComPtr<ID3D11Device>& device, TArray<uint8>&& inByteCode);
		virtual bool getParameter(char const* name, ShaderParameter& outParam) 
		{ 
			return outParam.bind(mParameterMap, name);
		}
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) final
		{
			return outParam.bind(mParameterMap , name);
		}
		virtual bool getResourceParameter(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo, ShaderParameter& outParam) final
		{
			return getResourceParameter(resourceType, resourceType == EShaderResourceType::Storage ? structInfo.variableName : structInfo.blockName, outParam);
		}

		static bool GenerateParameterMap(TArrayView<uint8 const> const& byteCode, ShaderParameterMap& parameterMap, D3DShaderParamInfo& outParamInfo);
		D3D11ShaderData      mResource;
		ShaderParameterMap   mParameterMap;
	};

	class D3D11VertexShader : public D3D11Shader
	{
		using BaseClass = D3D11Shader;
	public:
		D3D11VertexShader(): D3D11Shader(EShader::Vertex){}

		virtual bool initialize(TComPtr<ID3D11Device>& device, TArrayView<uint8 const>& inByteCode);

		virtual bool initialize(TComPtr<ID3D11Device>& device, TArray<uint8>&& inByteCode);

		TArray< uint8 >      byteCode;
	};

	class D3D11ShaderProgram : public TRefcountResource< RHIShaderProgram >
	{
	public:

		void initializeData(int numShaders)
		{
			mShaderDatas = new D3D11ShaderData[numShaders];
			mNumShaders = numShaders;
		}

		virtual bool getParameter(char const* name, ShaderParameter& outParam);
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) final;
		virtual bool getResourceParameter(EShaderResourceType resourceType, StructuredBufferInfo const& structInfo, ShaderParameter& outParam) final
		{
			return getResourceParameter(resourceType, resourceType == EShaderResourceType::Storage ? structInfo.variableName : structInfo.blockName, outParam);
		}
		virtual void releaseResource();

		template< class TFunc >
		void setupShader(ShaderParameter const& parameter, TFunc&& func)
		{
			mParameterMap.setupShader(parameter,
				[this, &func](int shaderIndex, ShaderParameter const& shaderParam)
				{
					func(mShaderDatas[shaderIndex].type, shaderParam);
				}
			);
		}

		ShaderPorgramParameterMap mParameterMap;

		TArray< uint8 > vertexByteCode;
		D3D11ShaderData* mShaderDatas;
		int mNumShaders;
	};

	class ShaderFormatHLSL : public ShaderFormat
	{
	public:
		ShaderFormatHLSL(TComPtr< ID3D11Device > inDevice )
			:mDevice( inDevice ){}

		virtual char const* getName() final { return "hlsl"; }
		virtual void setupShaderCompileOption(ShaderCompileOption& option) final;
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) final;
		virtual EShaderCompileResult compileCode(ShaderCompileContext const& context) final;

		virtual ShaderParameterMap* initializeProgram(RHIShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) final;
		virtual ShaderParameterMap* initializeProgram(RHIShaderProgram& shaderProgram, TArray< ShaderCompileDesc > const& descList, TArray<uint8> const& binaryCode) final;

		virtual void postShaderLoaded(RHIShaderProgram& shaderProgram) final;

		virtual bool doesSuppurtBinaryCode() const final;
		virtual bool getBinaryCode(ShaderProgramSetupData& setupData, TArray<uint8>& outBinaryCode) final;

		
		TComPtr< ID3D11Device > mDevice;

		virtual ShaderParameterMap* initializeShader(RHIShader& shader, ShaderSetupData& setupData) override;
		virtual ShaderParameterMap* initializeShader(RHIShader& shader, ShaderCompileDesc const& desc, TArray<uint8> const& binaryCode) override;


		virtual void postShaderLoaded(RHIShader& shader) override;
		virtual bool getBinaryCode(ShaderSetupData& setupData, TArray<uint8>& outBinaryCode) override;


		virtual void precompileCode(ShaderProgramSetupData& setupData) override;


		void precompileCode(ShaderSetupData& setupData) override;

	};

}//namespace Render

#endif // D3D11Shader_H_9211AFBF_3162_4D63_A2AC_097ED054E79B
