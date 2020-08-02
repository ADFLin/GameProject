#pragma once
#ifndef ShaderProgram_H_42BDD1A7_E1E5_43BA_975D_3F9B06FF0657
#define ShaderProgram_H_42BDD1A7_E1E5_43BA_975D_3F9B06FF0657

#include "ShaderCore.h"

namespace Render
{
	enum class EShaderObjectType
	{
		Shader,
		Program,
	};
	class ShaderObject
	{
	public:
		virtual EShaderObjectType getType() const = 0;
		enum EClassTypeName
		{
			Global,
			Material,
		};
		virtual void bindParameters(ShaderParameterMap const& parameterMap) {}

		virtual ~ShaderObject(){}

		std::string  shaderName;
	};

	template< class RHIResourceType >
	class TShaderFuncHelper : public ShaderObject
	{
	public:

		RHIResourceType* getRHIResource() { return mRHIResource; }
		void releaseRHI() { mRHIResource.release(); }

		void setParam(RHICommandList& commandList, char const* name, int v1) { setParamT(commandList, name, v1); }
		void setParam(RHICommandList& commandList, char const* name, int v1, int v2) { setParamT(commandList, name, IntVector2(v1, v2)); }
		void setParam(RHICommandList& commandList, char const* name, int v1, int v2, int v3) { setParamT(commandList, name, IntVector3(v1, v2, v3)); }
		void setParam(RHICommandList& commandList, char const* name, int v1, int v2, int v3, int v4) { setParamT(commandList, name, IntVector4(v1, v2, v3, v4)); }

		void setParam(RHICommandList& commandList, char const* name, float v1) { setParamT(commandList, name, v1); }
		void setParam(RHICommandList& commandList, char const* name, float v1, float v2) { setParamT(commandList, name, Vector2(v1, v2)); }
		void setParam(RHICommandList& commandList, char const* name, float v1, float v2, float v3) { setParamT(commandList, name, Vector3(v1, v2, v3)); }
		void setParam(RHICommandList& commandList, char const* name, float v1, float v2, float v3, float v4) { setParamT(commandList, name, Vector4(v1, v2, v3, v4)); }

		void setMatrix33(RHICommandList& commandList, char const* name, float const* value, int num = 1);
		void setMatrix44(RHICommandList& commandList, char const* name, float const* value, int num = 1);
		void setParam(RHICommandList& commandList, char const* name, float const v[], int num);
		void setVector3(RHICommandList& commandList, char const* name, float const v[], int num);
		void setVector4(RHICommandList& commandList, char const* name, float const v[], int num);

		void setParam(RHICommandList& commandList, char const* name, IntVector2 const& v) { setParamT(commandList, name, v); }
		void setParam(RHICommandList& commandList, char const* name, IntVector3 const& v) { setParamT(commandList, name, v); }
		void setParam(RHICommandList& commandList, char const* name, IntVector4 const& v) { setParamT(commandList, name, v); }

		void setParam(RHICommandList& commandList, char const* name, Vector2 const& v) { setParamT(commandList, name, v); }
		void setParam(RHICommandList& commandList, char const* name, Vector3 const& v) { setParamT(commandList, name, v); }
		void setParam(RHICommandList& commandList, char const* name, Vector4 const& v) { setParamT(commandList, name, v); }
		void setParam(RHICommandList& commandList, char const* name, Matrix4 const& m) { setMatrix44(commandList, name, m, 1); }
		void setParam(RHICommandList& commandList, char const* name, Matrix3 const& m) { setMatrix33(commandList, name, m, 1); }
		void setParam(RHICommandList& commandList, char const* name, Matrix4 const v[], int num) { setMatrix44(commandList, name, v[0], num); }
		void setParam(RHICommandList& commandList, char const* name, Vector3 const v[], int num) { setVector3(commandList, name, (float*)&v[0], num); }
		void setParam(RHICommandList& commandList, char const* name, Vector4 const v[], int num) { setVector4(commandList, name, (float*)&v[0], num); }

		template< class T >
		void setParamT(RHICommandList& commandList, char const* name, T const& v)
		{
			ShaderParameter param;
			if (!getParameter(name, param))
				return;
			setParam(commandList, param, v);
		}


		void setRWTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture, EAccessOperator op = AO_READ_AND_WRITE);
		void clearRWTexture(RHICommandList& commandList, char const* name);
		void setTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture, char const* samplerName, RHISamplerState& sampler);

		template< class RHITextureType >
		void setTexture(RHICommandList& commandList, char const* name, TRefCountPtr<RHITextureType>& texturePtr, char const* samplerName, RHISamplerState& sampler)
		{
			setTexture(commandList, name, *texturePtr, samplerName, sampler);
		}

		void setTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture);

		template< class RHITextureType >
		void setTexture(RHICommandList& commandList, char const* name, TRefCountPtr<RHITextureType>& texturePtr)
		{
			setTexture(commandList, name, *texturePtr);
		}


		void setParam(RHICommandList& commandList, ShaderParameter const& param, int v1);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector2 const& v);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector3 const& v);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector4 const& v);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, float v1);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector2 const& v);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector3 const& v);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector4 const& v);

		void setParam(RHICommandList& commandList, ShaderParameter const& param, LinearColor const& v);

		void setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix4 const& m);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix3 const& m);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix4 const v[], int num);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector3 const v[], int num);
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector4 const v[], int num);

		void setRWTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op = AO_READ_AND_WRITE, int idx = -1);
		void clearRWTexture(RHICommandList& commandList, ShaderParameter const& param);

		void setTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture);
		void setTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler);

		template< class RHITextureType >
		void setTexture(RHICommandList& commandList, ShaderParameter const& param, TRefCountPtr<RHITextureType>& texturePtr)
		{
			setTexture(commandList, param, *texturePtr);
		}

		template< class RHITextureType >
		void setTexture(RHICommandList& commandList, ShaderParameter const& param, TRefCountPtr<RHITextureType>& texturePtr, ShaderParameter const& paramSampler, RHISamplerState& sampler)
		{
			setTexture(commandList, param, *texturePtr, paramSampler, sampler);
		}

		void setUniformBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setStorageBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setAtomicCounterBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setAtomicCounterBuffer(RHICommandList& commandList, char const* name, RHIVertexBuffer& buffer);

		struct StructuredBlockInfo
		{
			ShaderParameter param;
			StructuredBufferInfo* structInfo = nullptr;
		};

		template< class TStruct >
		void setStructuredUniformBufferT(RHICommandList& commandList, RHIVertexBuffer& buffer)
		{
			auto& bufferStruct = TStruct::GetStructInfo();
			for (auto const& block : mBoundedBlocks)
			{
				if (block.structInfo == &bufferStruct)
				{
					setUniformBuffer(commandList, block.param, buffer);
					return;
				}
			}

			StructuredBlockInfo block;
			block.structInfo = &bufferStruct;
			if (mRHIResource->getResourceParameter(EShaderResourceType::Uniform, bufferStruct.blockName, block.param))
			{
				mBoundedBlocks.push_back(block);
				setUniformBuffer(commandList, block.param, buffer);
			}
		}

		template< class TStruct >
		void setStructuredStorageBufferT(RHICommandList& commandList, RHIVertexBuffer& buffer)
		{
			auto& bufferStruct = TStruct::GetStructInfo();
			for (auto const& block : mBoundedBlocks)
			{
				if (block.structInfo == &bufferStruct)
				{
					setStorageBuffer(commandList, block.param, buffer);
					return;
				}
			}

			StructuredBlockInfo block;
			block.structInfo = &bufferStruct;
			if (mRHIResource->getResourceParameter(EShaderResourceType::Storage, bufferStruct.blockName, block.param))
			{
				mBoundedBlocks.push_back(block);
				setStorageBuffer(commandList, block.param, buffer);
			}
		}
		bool getParameter(char const* name, ShaderParameter& outParam);

		std::vector< StructuredBlockInfo > mBoundedBlocks;

	public:
		TRefCountPtr< RHIResourceType > mRHIResource;
	};


	extern template class TShaderFuncHelper< RHIShaderProgram >;
	extern template class TShaderFuncHelper< RHIShader >;

	class ShaderProgram : public TShaderFuncHelper< RHIShaderProgram >
	{
	public:
		virtual EShaderObjectType getType() const override { return EShaderObjectType::Program; }
	};

	class Shader : public TShaderFuncHelper< RHIShader >
	{
	public:
		virtual EShaderObjectType getType() const override { return EShaderObjectType::Shader; }
	};


#define SHADER_MEMBER_PARAM( NAME ) mParam##NAME
#define DEFINE_SHADER_PARAM( NAME ) ShaderParameter SHADER_MEMBER_PARAM( NAME )

	template < class T >
	constexpr T GetType(T&);

	template< class TShader, ShaderParameter TShader::*MemberParam, class T >
	FORCEINLINE void SetShaderParamT(RHICommandList& commandList, TShader& shader, T const& value )
	{
		shader.setParam(commandList, shader.*MemberParam, value);
	}

	template< class TShader, ShaderParameter TShader::*MemberParam >
	FORCEINLINE void SetShaderTextureT(RHICommandList& commandList, TShader& shader, RHITextureBase& value)
	{
		shader.setTexture(commandList, shader.*MemberParam, value);
	}

#define BIND_SHADER_PARAM( MAP , NAME ) SHADER_MEMBER_PARAM( NAME ).bind( MAP , SHADER_PARAM(NAME) )
#define SET_SHADER_PARAM( COMMANDLIST, SHADER , NAME , VALUE ) SetShaderParamT< decltype(GetType(SHADER)) , &decltype(GetType(SHADER))::SHADER_MEMBER_PARAM(NAME) >( COMMANDLIST , SHADER , VALUE ) 
#define SET_SHADER_TEXTURE( COMMANDLIST, SHADER , NAME , VALUE ) SetShaderTextureT< decltype(GetType(SHADER)) , &decltype(GetType(SHADER))::SHADER_MEMBER_PARAM(NAME) >( COMMANDLIST , SHADER , VALUE ) 
}//namespace Render


#endif // ShaderProgram_H_42BDD1A7_E1E5_43BA_975D_3F9B06FF0657
