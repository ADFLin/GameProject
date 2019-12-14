#include "ShaderProgram.h"

#include "RHI/RHICommandListImpl.h"

#include "LogSystem.h"

namespace Render
{
#define CHECK_PARAMETER( PARAM ) { static bool bOnce = true; if ( !PARAM.isBound() && bOnce ){  bOnce = false; LogWarning( 0 ,"Shader Param not bounded" ); return; } }

	static RHIContext& GetContext(RHICommandList& commandList)
	{
		return static_cast<RHICommandListImpl&>(commandList).getExecutionContext();
	}

	ShaderProgram::ShaderProgram()
	{

	}

	ShaderProgram::~ShaderProgram()
	{

	}

	void ShaderProgram::setTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture, char const* samplerName, RHISamplerState& sampler)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		ShaderParameter paramSampler;
		getParameter(samplerName, paramSampler);
		GetContext(commandList).setShaderTexture(*mRHIResource, param, texture, paramSampler, sampler);
	}

	void ShaderProgram::setTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		setTexture(commandList, param, texture);
	}

	void ShaderProgram::setTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderTexture(*mRHIResource, param, texture);
	}

	void ShaderProgram::setTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState& sampler)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderTexture(*mRHIResource, param, texture, paramSampler, sampler);
	}

	void ShaderProgram::setRWTexture(RHICommandList& commandList, char const* name, RHITextureBase& texture, EAccessOperator op /*= AO_READ_AND_WRITE*/)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderRWTexture(*mRHIResource, param, texture, op);
	}

	void ShaderProgram::setRWTexture(RHICommandList& commandList, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op /*= AO_READ_AND_WRITE*/, int idx /*= -1*/)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderRWTexture(*mRHIResource, param, texture, op);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, char const* name, float const v[], int num)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, v, num);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, int v1)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, &v1, 1);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector2 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (int const*)&v, 2);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector3 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (int const*)&v, 3);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, IntVector4 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (int const*)&v, 4);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, float v1)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, &v1, 1);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector2 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (float const*)&v, 2);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector3 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (float const*)&v, 3);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector4 const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (float const*)&v, 4);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, LinearColor const& v)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, (float const*)&v, 4);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix4 const& m)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, &m, 1);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix3 const& m)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, &m, 1);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, Matrix4 const v[], int num)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, v, num);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector3 const v[], int num)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, v, num);
	}

	void ShaderProgram::setParam(RHICommandList& commandList, ShaderParameter const& param, Vector4 const v[], int num)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderValue(*mRHIResource, param, v, num);
	}


	void ShaderProgram::setVector3(RHICommandList& commandList, char const* name, float const v[], int num)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, (Vector3 const*)v, num);
	}

	void ShaderProgram::setVector4(RHICommandList& commandList, char const* name, float const v[], int num)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, (Vector4 const*)v, num);
	}

	void ShaderProgram::setMatrix33(RHICommandList& commandList, char const* name, float const* value, int num /*= 1*/)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, (Matrix3 const*)value, num);
	}


	bool ShaderProgram::getParameter(char const* name, ShaderParameter& outParam)
	{
		return mRHIResource->getParameter(name, outParam);
	}

	void ShaderProgram::setUniformBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderUniformBuffer(*mRHIResource, param, buffer);
	}

	void ShaderProgram::setStorageBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderStorageBuffer(*mRHIResource, param, buffer);
	}

	void ShaderProgram::setAtomicCounterBuffer(RHICommandList& commandList, ShaderParameter const& param, RHIVertexBuffer& buffer)
	{
		CHECK_PARAMETER(param);
		GetContext(commandList).setShaderAtomicCounterBuffer(*mRHIResource, param, buffer);
	}

	void ShaderProgram::setAtomicCounterBuffer(RHICommandList& commandList, char const* name, RHIVertexBuffer& buffer)
	{
		ShaderParameter param;
		if( !mRHIResource->getResourceParameter(EShaderResourceType::AtomicCounter, name, param) )
			return;
		setAtomicCounterBuffer(commandList, param, buffer);
	}

	void ShaderProgram::setMatrix44(RHICommandList& commandList, char const* name, float const* value, int num /*= 1*/)
	{
		ShaderParameter param;
		if( !getParameter(name, param) )
			return;
		GetContext(commandList).setShaderValue(*mRHIResource, param, (Matrix4 const*)value, num);
	}

}//namespace Render