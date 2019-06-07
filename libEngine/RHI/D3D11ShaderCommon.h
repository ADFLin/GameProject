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
		Shader::Type type;
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

		bool initialize( Shader::Type type , TComPtr<ID3D11Device>& device , TComPtr<ID3D10Blob>& inByteCode );
		
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

		static bool GenerateParameterMap(std::vector< uint8 > const& byteCode , ShaderParameterMap& parameterMap);
		std::vector< uint8 > byteCode;
		D3D11ShaderResource  mResource;
	};


	class D3D11ShaderProgram : public RHIShaderProgram
	{
	public:

		virtual bool setupShaders(RHIShaderRef shaders[], int numShader)
		{
			assert(ARRAY_SIZE(mShaders) >= numShader);
			for( int i = 0; i < numShader; ++i )
			{
				mShaders[i] =  shaders[i];
				auto& shaderImpl = static_cast<D3D11Shader&>(*mShaders[i]);
				ShaderParameterMap parameterMap;
				D3D11Shader::GenerateParameterMap(shaderImpl.byteCode, parameterMap);
				addShaderParameterMap(shaderImpl.mResource.type, parameterMap);
			}
			finalizeParameterMap();
			return true;
		}
		virtual bool getParameter(char const* name, ShaderParameter& outParam)
		{
			return mParameterMap.bind(outParam, name);
		}
		virtual bool getResourceParameter(EShaderResourceType resourceType, char const* name, ShaderParameter& outParam)
		{
			return mParameterMap.bind(outParam, name);
		}


		virtual void incRef()
		{
			++mRefcount;
		}

		virtual bool decRef()
		{
			--mRefcount;
			return mRefcount <= 0;
		}

		virtual void releaseResource()
		{
			for( int i = 0; i < ARRAY_SIZE(mShaders); ++i )
			{
				mShaders[i].release();
			}
		}

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

		void addShaderParameterMap(Shader::Type shaderType, ShaderParameterMap const& parameterMap)
		{
			for( auto const& pair : parameterMap.mMap )
			{
				auto& param = mParameterMap.mMap[pair.first];

				if( param.mLoc == -1 )
				{
					param.mLoc = mParamEntryMap.size();
					ParameterEntry entry;
					entry.numParam = 0;
					mParamEntryMap.push_back(entry);
				}

				ParameterEntry& entry = mParamEntryMap[param.mLoc];

				entry.numParam += 1;
				ShaderParamEntry paramEntry;
				paramEntry.type  = shaderType;
				paramEntry.param = pair.second;
				paramEntry.loc   = param.mLoc;
				mParamEntries.push_back(paramEntry);
			}
		}

		void finalizeParameterMap()
		{
			std::sort(mParamEntries.begin(), mParamEntries.end(), [](ShaderParamEntry const& lhs, ShaderParamEntry const& rhs)
			{
				if( lhs.loc < rhs.loc )
					return true;
				else if( lhs.loc > rhs.loc )
					return false;
				return lhs.type < rhs.type;
			});

			int curLoc = -1;
			for( int index = 0 ; index < mParamEntries.size() ; ++index )
			{
				auto& paramEntry = mParamEntries[index];
				if( paramEntry.loc != curLoc )
				{
					mParamEntryMap[paramEntry.loc].paramIndex = index;
					curLoc = paramEntry.loc;
				}
			}
		}

		ShaderParameterMap mParameterMap;

		struct ParameterEntry
		{
			uint16  numParam;
			uint16  paramIndex;
		};

		struct ShaderParamEntry
		{
			int loc;
			Shader::Type    type;
			ShaderParameter param;
		};

		int mRefcount = 0;
		std::vector< ParameterEntry >   mParamEntryMap;
		std::vector< ShaderParamEntry > mParamEntries;
		RHIShaderRef mShaders[Shader::Count - 1];
	};

	class ShaderFormatHLSL : public ShaderFormat
	{
	public:
		ShaderFormatHLSL(TComPtr< ID3D11Device > inDevice )
			:mDevice( inDevice ){}

		virtual char const* getName() final { return "hlsl"; }
		virtual void setupShaderCompileOption(ShaderCompileOption& option)  final
		{
			option.addMeta("ShaderFormat", getName());
		}
		virtual void getHeadCode(std::string& inoutCode, ShaderCompileOption const& option, ShaderEntryInfo const& entry) final
		{
			inoutCode += "#define COMPILER_HLSL 1\n";
		}

		virtual bool isSupportBinaryCode() const final { return true; }
		virtual bool getBinaryCode(RHIShaderProgram& shaderProgram, std::vector<uint8>& outBinaryCode) final
		{
			return false;
		}
		virtual bool setupProgram(RHIShaderProgram& shaderProgram, std::vector<uint8> const& binaryCode) final
		{

			return false;
		}

		virtual void setupParameters(ShaderProgram& shaderProgram) final;

		virtual bool compileCode(Shader::Type type, RHIShader& shader, char const* path, ShaderCompileInfo* compileInfo, char const* def) final;

		TComPtr< ID3D11Device > mDevice;



	};

}//namespace Render

#endif // D3D11Shader_H_9211AFBF_3162_4D63_A2AC_097ED054E79B
