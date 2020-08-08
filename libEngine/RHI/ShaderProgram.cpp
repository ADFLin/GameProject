#include "ShaderProgram.h"

#include "RHI/RHICommandListImpl.h"

#include "LogSystem.h"

namespace Render
{
#if _DEBUG
	void LogOuptput(char const* text, ShaderParameter const& param)
	{
		LogWarning(0, text, param.mName.c_str());
	}
#define CHECK_PARAMETER( PARAM ) { if ( !PARAM.isBound() ){  if (!PARAM.bHasLogWarning){ const_cast<ShaderParameter&>( PARAM ).bHasLogWarning = true; LogOuptput( "Shader Param is not bounded : %s" , PARAM ); } return; } }
#else
#define CHECK_PARAMETER( PARAM ) { if ( !PARAM.isBound() ){ return; } }
#endif
	static RHIContext& GetContext(RHICommandList& commandList)
	{
		return static_cast<RHICommandListImpl&>(commandList).getExecutionContext();
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture, char const* samplerName, RHISamplerState& sampler)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		ShaderParameter paramSampler;
		getParameter(samplerName, paramSampler);
		GetContext(commandList).setShaderTexture(*mRHIResource, param, texture, paramSampler, sampler);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		setTexture(commandList, param, texture);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderTexture(*mRHIResource, param, texture);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderTexture(*mRHIResource, param, texture, paramSampler, sampler);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper<RHIResourceType>::clearTexture(RHICommandList& commandList, ShaderParameter const& param)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).clearShaderTexture(*mRHIResource, param);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setRWTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture, EAccessOperator op /*= AO_READ_AND_WRITE*/)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderRWTexture(*mRHIResource, param, texture, op);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setRWTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op /*= AO_READ_AND_WRITE*/, int idx /*= -1*/)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderRWTexture(*mRHIResource, param, texture, op);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper<RHIResourceType>::clearRWTexture(RHICommandList& commandList, char const* name)
	{
		ShaderParameter param;
		if (!getParameter(name, param))
			return;
		GetContext(commandList).clearShaderRWTexture(*mRHIResource, param);
	}

	template< class RHIResourceType >
	void Render::TShaderFuncHelper<RHIResourceType>::clearRWTexture(RHICommandList& commandList, ShaderParameter const& param)
	{
		GetContext(commandList).clearShaderRWTexture(*mRHIResource, param);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, char const* name, float const v[], int num)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, v, num);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, int v1)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, &v1, 1);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector2 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (int const*)&v, 2);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector3 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (int const*)&v, 3);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector4 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (int const*)&v, 4);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, float v1)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, &v1, 1);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector2 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (float const*)&v, 2);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector3 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (float const*)&v, 3);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector4 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (float const*)&v, 4);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, LinearColor const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (float const*)&v, 4);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix4 const& m)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, &m, 1);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix3 const& m)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, &m, 1);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix4 const v[], int num)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, v, num);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector3 const v[], int num)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, v, num);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector4 const v[], int num)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, v, num);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setVector3(RHICommandList& commandList, char const* name, float const v[], int num)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, (Vector3 const*)v, num);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setVector4(RHICommandList& commandList, char const* name, float const v[], int num)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, (Vector4 const*)v, num);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setMatrix33(RHICommandList& commandList, char const* name, float const* value, int num /*= 1*/)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, (Matrix3 const*)value, num);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setUniformBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderUniformBuffer(*mRHIResource, param, buffer);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setStorageBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderStorageBuffer(*mRHIResource, param, buffer, op);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setAtomicCounterBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderAtomicCounterBuffer(*mRHIResource, param, buffer);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setAtomicCounterBuffer(RHICommandList& commandList, char const* name, RHIVertexBuffer& buffer)
	{
		ShaderParameter param;
		if( !mRHIResource->getResourceParameter(EShaderResourceType::AtomicCounter, name, param) )
			return;
		setAtomicCounterBuffer(commandList, param, buffer);
	}

	template< class RHIResourceType >
	void TShaderFuncHelper< RHIResourceType>::setMatrix44(RHICommandList& commandList, char const* name, float const* value, int num /*= 1*/)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, (Matrix4 const*)value, num);
	}

	template< class RHIResourceType >
	bool TShaderFuncHelper< RHIResourceType>::getParameter(char const* name, ShaderParameter& outParam)
	{
		return mRHIResource->getParameter(name, outParam);
	}

	template class TShaderFuncHelper<RHIShaderProgram>;
	template class TShaderFuncHelper<RHIShader>;

}//namespace Render