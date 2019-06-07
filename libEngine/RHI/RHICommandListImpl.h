#pragma once
#ifndef RHICommandListImpl_H_F072E14F_3189_4D4D_A307_B076818D2184
#define RHICommandListImpl_H_F072E14F_3189_4D4D_A307_B076818D2184

#include "RHICommand.h"


namespace Render
{
	class RHIShaderProgram;
	class ShaderParameter;

	class RHIContext
	{
	public:
		RHI_FUNC(void RHISetRasterizerState(RHIRasterizerState& rasterizerState));
		RHI_FUNC(void RHISetBlendState(RHIBlendState& blendState));
		RHI_FUNC(void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef = -1));

		RHI_FUNC(void RHISetViewport(int x, int y, int w, int h));
		RHI_FUNC(void RHISetScissorRect(int x, int y, int w, int h));

		RHI_FUNC(void RHIDrawPrimitive(PrimitiveType type, int vStart, int nv));
		RHI_FUNC(void RHIDrawIndexedPrimitive(PrimitiveType type, int indexStart, int nIndex, uint32 baseVertex));
		RHI_FUNC(void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride));
		RHI_FUNC(void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride));
		RHI_FUNC(void RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, uint32 numInstance, uint32 baseInstance));
		RHI_FUNC(void RHIDrawIndexedPrimitiveInstanced(PrimitiveType type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance));
		
		RHI_FUNC(void RHIDrawPrimitiveUP(PrimitiveType type, int numVertex, VertexDataInfo dataInfos[], int numData));
		RHI_FUNC(void RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numVertex, VertexDataInfo dataInfos[], int numVertexData, int const* pIndices, int numIndex));

		RHI_FUNC(void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer, RHITextureDepth* overrideDepthTexture));
		RHI_FUNC(void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream));
		RHI_FUNC(void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer));
		RHI_FUNC(void RHISetupFixedPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* textures[], int numTexture));

		RHI_FUNC(void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ));

		//Shader
		RHI_FUNC(void RHISetShaderProgram(RHIShaderProgram* shaderProgram));

		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& parameter, float const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim));
		RHI_FUNC(void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim));
		RHI_FUNC(void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim));
		RHI_FUNC(void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim));

		RHI_FUNC(void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView));

		RHI_FUNC(void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture));
		RHI_FUNC(void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler));
		RHI_FUNC(void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler));
		RHI_FUNC(void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op));

		RHI_FUNC(void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer));
		RHI_FUNC(void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer));
		RHI_FUNC(void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer));

	};


	class RHICommandListImpl : public RHICommandList
	{
	public:
		RHICommandListImpl(RHIContext& context)
			:mExecutionContext(context){}

		RHIContext& getExecutionContext() { return mExecutionContext; }
	private:
		RHIContext& mExecutionContext;
	};


}

#endif // RHICommandListImpl_H_F072E14F_3189_4D4D_A307_B076818D2184
