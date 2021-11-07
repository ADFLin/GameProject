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
		uint32       permutationId = 0;
	};

	template< class RHIResourceType >
	class TShaderFuncHelper : public ShaderObject
	{
	public:

		RHIResourceType* getRHIResource() { return mRHIResource; }
		void releaseRHI()
		{
			mBoundedBlocks.clear();
			mRHIResource.release(); 
		}

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
		void setVector2(RHICommandList& commandList, char const* name, float const v[], int num);
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

		void setParam(RHICommandList& commandList, char const* name, Vector2 const v[], int num) { setVector2(commandList, name, (float*)&v[0], num); }
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
		void setParam(RHICommandList& commandList, ShaderParameter const& param, Vector2 const v[], int num);
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

		void clearTexture(RHICommandList& commandList, ShaderParameter const& param);


		void setUniformBuffer(RHICommandList& commandList, char const* name, RHIVertexBuffer& buffer);
		void setUniformBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer);
		void setStorageBuffer(RHICommandList& commandList, char const* name, RHIVertexBuffer& buffer, EAccessOperator op = AO_READ_ONLY);
		void setStorageBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op = AO_READ_ONLY);
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
			char const* name = mRHIResource->getStructParameterName(EShaderResourceType::Uniform, bufferStruct);
			if (name && mRHIResource->getResourceParameter(EShaderResourceType::Uniform, name, block.param))
			{
				mBoundedBlocks.push_back(block);
				setUniformBuffer(commandList, block.param, buffer);
			}
		}

		template< class TStruct >
		void setStructuredStorageBufferT(RHICommandList& commandList, RHIVertexBuffer& buffer, EAccessOperator op = AO_READ_ONLY)
		{
			auto& bufferStruct = TStruct::GetStructInfo();
			for (auto const& block : mBoundedBlocks)
			{
				if (block.structInfo == &bufferStruct)
				{
					setStorageBuffer(commandList, block.param, buffer, op);
					return;
				}
			}

			StructuredBlockInfo block;
			block.structInfo = &bufferStruct;
			char const* name = mRHIResource->getStructParameterName(EShaderResourceType::Storage, bufferStruct);
			if (name && mRHIResource->getResourceParameter(EShaderResourceType::Storage, name, block.param))
			{
				mBoundedBlocks.push_back(block);
				setStorageBuffer(commandList, block.param, buffer, op);
			}
		}

		void setStructuredStorageBuffer(RHICommandList& commandList, StructuredBufferInfo const& info, RHIVertexBuffer& buffer, EAccessOperator op = AO_READ_ONLY)
		{
			ShaderParameter param;
			char const* name = mRHIResource->getStructParameterName(EShaderResourceType::Storage, info);
			if (name && mRHIResource->getResourceParameter(EShaderResourceType::Storage, name, param))
			{
				setStorageBuffer(commandList, param, buffer, op);
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
#define DEFINE_TEXTURE_PARAM( NAME )\
	DEFINE_SHADER_PARAM(NAME);\
	DEFINE_SHADER_PARAM(NAME##Sampler)

	template < class T >
	constexpr T GetType(T&);

	template< class TShader, class T >
	FORCEINLINE void SetShaderParamT(RHICommandList& commandList, TShader& shader, ShaderParameter const& param, T const& value)
	{
		shader.setParam(commandList, param, value);
	}

	template< class TShader>
	FORCEINLINE void SetShaderTextureT(RHICommandList& commandList, TShader& shader, ShaderParameter const& param, RHITextureBase& value)
	{
		shader.setTexture(commandList, param, value);
	}

	template< class TShader >
	FORCEINLINE void SetShaderTextureT(RHICommandList& commandList, TShader& shader, ShaderParameter const& param, RHITextureBase& value, ShaderParameter const& paramSampler, RHISamplerState& sampler)
	{
		shader.setTexture(commandList, param, value, paramSampler, sampler);
	}

	template< class TShader >
	FORCEINLINE void ClearShaderTextureT(RHICommandList& commandList, TShader& shader, ShaderParameter const& param)
	{
		shader.clearTexture(commandList, param);
	}

#define BIND_SHADER_PARAM( MAP , NAME ) SHADER_MEMBER_PARAM( NAME ).bind( MAP , SHADER_PARAM(NAME) )
#define BIND_TEXTURE_PARAM( MAP , NAME ) BIND_SHADER_PARAM( MAP, NAME ); BIND_SHADER_PARAM( MAP, NAME##Sampler)

#define SET_SHADER_PARAM( COMMANDLIST, SHADER , NAME , VALUE )\
	 SetShaderParamT( COMMANDLIST, SHADER, (SHADER).SHADER_MEMBER_PARAM(NAME), VALUE ) 
#define SET_SHADER_TEXTURE( COMMANDLIST, SHADER , NAME , VALUE )\
	 SetShaderTextureT( COMMANDLIST, SHADER, (SHADER).SHADER_MEMBER_PARAM(NAME),VALUE )
#define SET_SHADER_TEXTURE_AND_SAMPLER( COMMANDLIST, SHADER , NAME , TEXTURE, SAMPLER )\
	 SetShaderTextureT( COMMANDLIST, SHADER, (SHADER).SHADER_MEMBER_PARAM(NAME), TEXTURE, (SHADER).SHADER_MEMBER_PARAM(NAME##Sampler), SAMPLER )
#define CLEAR_SHADER_TEXTURE(COMMANDLIST, SHADER, NAME)\
	 ClearShaderTextureT( COMMANDLIST, SHADER, (SHADER).SHADER_MEMBER_PARAM(NAME) )
}//namespace Render


#endif // ShaderProgram_H_42BDD1A7_E1E5_43BA_975D_3F9B06FF0657
