#pragma once
#ifndef D3D11Shader_H_9211AFBF_3162_4D63_A2AC_097ED054E79B
#define D3D11Shader_H_9211AFBF_3162_4D63_A2AC_097ED054E79B

#include "ShaderCore.h"
#include "ShaderFormat.h"

#include "Platform/Windows/ComUtility.h"

#include <D3Dcompiler.h>
#include <D3DX11async.h>
#pragma comment(lib , "D3dcompiler.lib")

namespace Render
{
	struct D3D11ShaderResource
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
	};


	class D3D11Shader : public RHIShader
	{
	public:
		D3D11Shader()
		{
			mResource.ptr = nullptr;
		}

		bool initialize( EShader::Type type, TComPtr<ID3D11Device>& device, TComPtr<ID3D10Blob>& inByteCode );
		bool initialize( EShader::Type type, TComPtr<ID3D11Device>& device, std::vector<uint8>&& inByteCode );

		bool createResource(EShader::Type type, TComPtr<ID3D11Device>& device, uint8 const* pCode, size_t codeSize);
		virtual void incRef()
		{
			if( mResource.ptr )
			{
				mResource.ptr->AddRef();
			}
		}
		
		virtual bool decRef()
		{
			if( mResource.ptr )
			{
				return mResource.ptr->Release() == 1;
			}
			return false;
		}
		
		virtual void releaseResource()
		{
			if( mResource.ptr )
			{
				mResource.ptr->Release();
				mResource.ptr = nullptr;
			}
		}

		virtual bool getParameter(char const* name, ShaderParameter& outParam) { return false; }
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam) { return false; }

		static bool GenerateParameterMap(std::vector< uint8 > const& byteCode , ShaderParameterMap& parameterMap);
		std::vector< uint8 > byteCode;
		D3D11ShaderResource  mResource;
	};

	class D3D11ShaderProgram : public TRefcountResource< RHIShaderProgram >
	{
	public:

		bool setupShaders(ShaderResourceInfo shaders[], int numShaders);

		virtual bool getParameter(char const* name, ShaderParameter& outParam);
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam);

		virtual void releaseResource();

		template< class TFunc >
		void setupShader(ShaderParameter const& parameter, TFunc&& func)
		{
			assert(0 <= parameter.mLoc && parameter.mLoc < mParamEntryMap.size());
			auto const& entry = mParamEntryMap[parameter.mLoc];

			ShaderParamEntry* pParamEntry = &mParamEntries[entry.paramIndex];
			for( int i = entry.numParam; i ; --i )
			{
				func( pParamEntry->type , pParamEntry->param );
				++pParamEntry;
			}
		}

		void addShaderParameterMap(EShader::Type shaderType, ShaderParameterMap const& parameterMap);
		void finalizeParameterMap();

		ShaderParameterMap mParameterMap;

		struct ParameterEntry
		{
			uint16  numParam;
			uint16  paramIndex;
		};

		struct ShaderParamEntry
		{
			int loc;
			EShader::Type    type;
			ShaderParameter param;
		};

		std::vector< ParameterEntry >   mParamEntryMap;
		std::vector< ShaderParamEntry > mParamEntries;
		TRefCountPtr< D3D11Shader >     mShaders[EShader::Count - 1];
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
		virtual bool compileCode(ShaderCompileInput const& input, ShaderCompileOutput& output) final;

		virtual bool initializeProgram(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData) final;
		virtual bool initializeProgram(ShaderProgram& shaderProgram, std::vector< ShaderCompileInfo > const& shaderCompiles, std::vector<uint8> const& binaryCode) final;

		virtual void postShaderLoaded(ShaderProgram& shaderProgram) final;

		virtual bool isSupportBinaryCode() const final;
		virtual bool getBinaryCode(ShaderProgram& shaderProgram, ShaderProgramSetupData& setupData, std::vector<uint8>& outBinaryCode) final;

		
		TComPtr< ID3D11Device > mDevice;
	};

}//namespace Render

#endif // D3D11Shader_H_9211AFBF_3162_4D63_A2AC_097ED054E79B
