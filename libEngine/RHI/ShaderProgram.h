#pragma once
#ifndef ShaderProgram_H_42BDD1A7_E1E5_43BA_975D_3F9B06FF0657
#define ShaderProgram_H_42BDD1A7_E1E5_43BA_975D_3F9B06FF0657

#include "ShaderCore.h"

namespace Render
{

	class ShaderProgram : public ShaderResource
	{
	public:
		enum EClassTypeName
		{
			Global,
			Material,
		};

		ShaderProgram();

		RHIShaderProgram* getRHIResource() { return mRHIResource; }
		void releaseRHI() { mRHIResource.release(); }

		virtual ~ShaderProgram();
		virtual void bindParameters(ShaderParameterMap const& parameterMap) {}


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
			if( !getParameter(name, param) )
				return;
			setParam(commandList, param, v);
		}


		void setRWTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture, EAccessOperator op = AO_READ_AND_WRITE);


		void setTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture, char const* samplerName, RHISamplerState& sampler);

		template< class RHITextureType >
		void setTexture(RHICommandList& commandList, char const* name, TRefCountPtr<RHITextureType>& texturePtr, char const* samplerName, RHISamplerState& sampler)
		{
			setTexture(commandList, name, *texturePtr, samplerName , sampler);
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


		void setStructedUniformBuffer(RHICommandList& commandList, StructuredBlockInfo const& blockInfo, RHIVertexBuffer& buffer)
		{
			setUniformBuffer(commandList, blockInfo.param, buffer);
		}

		void setStructedStorageBuffer(RHICommandList& commandList, StructuredBlockInfo const& blockInfo, RHIVertexBuffer& buffer)
		{
			setStorageBuffer(commandList, blockInfo.param, buffer);
		}


		template< class TStruct >
		void setStructuredUniformBufferT(RHICommandList& commandList, RHIVertexBuffer& buffer)
		{
			auto& bufferStruct = TStruct::GetStructInfo();
			for( auto const& block : mBoundedBlocks )
			{
				if( block.structInfo == &bufferStruct )
				{
					setStructedUniformBuffer(commandList, block, buffer);
					return;
				}
			}

			StructuredBlockInfo block;
			if( mRHIResource->getResourceParameter(EShaderResourceType::Uniform, bufferStruct.blockName, block.param) )
			{
				mBoundedBlocks.push_back(block);
				setStructedUniformBuffer(commandList, block, buffer);
			}
		}

		template< class TStruct >
		void setStructuredStorageBufferT(RHICommandList& commandList, RHIVertexBuffer& buffer)
		{
			auto& bufferStruct = TStruct::GetStructInfo();
			for( auto const& block : mBoundedBlocks )
			{
				if( block.structInfo == &bufferStruct )
				{
					setStructedStorageBuffer(commandList, block, buffer);
					return;
				}
			}

			StructuredBlockInfo block;
			if( mRHIResource->getResourceParameter(EShaderResourceType::Storage, bufferStruct.blockName, block.param) )
			{
				mBoundedBlocks.push_back(block);
				setStructedStorageBuffer(commandList, block, buffer);
			}
		}

		bool getParameter(char const* name, ShaderParameter& outParam);

		std::vector< StructuredBlockInfo > mBoundedBlocks;

		std::string  shaderName;
		//#TODO
	public:
		RHIShaderProgramRef mRHIResource;
	};


	class MaterialShaderProgramClass;
	class GlobalShaderProgramClass;

#define DECLARE_EXPORTED_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , API , ...)\
	public: \
		using ShaderClassType = CALSS_TYPE_NAME##ShaderProgramClass; \
		static API ShaderClassType ShaderClass; \
		static ShaderClassType const& GetShaderClass(){ return ShaderClass; }\
		static CLASS* CreateShader() { return new CLASS; }

#define DECLARE_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , ...) DECLARE_EXPORTED_SHADER_PROGRAM( CLASS , CALSS_TYPE_NAME , , __VA_ARGS__ )

#define IMPLEMENT_SHADER_PROGRAM( CLASS )\
	CLASS::ShaderClassType CLASS::ShaderClass\
	(\
		(GlobalShaderProgramClass::FunCreateShader) &CLASS::CreateShader,\
		CLASS::SetupShaderCompileOption,\
		CLASS::GetShaderFileName, \
		CLASS::GetShaderEntries \
	);


#define IMPLEMENT_SHADER_PROGRAM_T( TEMPLATE_ARGS , CLASS )\
	TEMPLATE_ARGS \
	IMPLEMENT_SHADER_PROGRAM( CLASS )

}//namespace Render


#endif // ShaderProgram_H_42BDD1A7_E1E5_43BA_975D_3F9B06FF0657
