#pragma once
#ifndef RHICommandListImpl_H_F072E14F_3189_4D4D_A307_B076818D2184
#define RHICommandListImpl_H_F072E14F_3189_4D4D_A307_B076818D2184

#include "RHICommand.h"


namespace Render
{
	class RHIShaderProgram;
	class RHIShader;
	class ShaderParameter;

	class RHIContext
	{
	public:
		RHI_FUNC(void RHISetRasterizerState(RHIRasterizerState& rasterizerState));
		RHI_FUNC(void RHISetBlendState(RHIBlendState& blendState));
		RHI_FUNC(void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef = -1));

		RHI_FUNC(void RHISetViewport(float x, float y, float w, float h, float zNear, float zFar));
		RHI_FUNC(void RHISetViewports(ViewportInfo const viewports[], int numViewports));
		RHI_FUNC(void RHISetScissorRect(int x, int y, int w, int h));

		RHI_FUNC(void RHIDrawPrimitive(EPrimitive type, int vStart, int nv));
		RHI_FUNC(void RHIDrawIndexedPrimitive(EPrimitive type, int indexStart, int nIndex, uint32 baseVertex));
		RHI_FUNC(void RHIDrawPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride));
		RHI_FUNC(void RHIDrawIndexedPrimitiveIndirect(EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride));
		RHI_FUNC(void RHIDrawPrimitiveInstanced(EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance));
		RHI_FUNC(void RHIDrawIndexedPrimitiveInstanced(EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance));
		
		RHI_FUNC(void RHIDrawPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numData));
		RHI_FUNC(void RHIDrawIndexedPrimitiveUP(EPrimitive type, int numVertices, VertexDataInfo dataInfos[], int numVertexData, uint32 const* pIndices, int numIndex));

		RHI_FUNC(void RHIDrawMeshTasks(int start, int count));
		RHI_FUNC(void RHIDrawMeshTasksIndirect(RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride));

		RHI_FUNC(void RHISetFrameBuffer(RHIFrameBuffer* frameBuffer));
		RHI_FUNC(void RHIClearRenderTargets(EClearBits clearBits, LinearColor colors[], int numColor, float depth, uint8 stenceil));

		RHI_FUNC(void RHISetInputStream(RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream));
		RHI_FUNC(void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer));
		RHI_FUNC(void RHISetFixedShaderPipelineState(Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler));

		RHI_FUNC(void RHIDispatchCompute(uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ));

		RHI_FUNC(void RHIFlushCommand());

		//Shader
		RHI_FUNC(void RHISetShaderProgram(RHIShaderProgram* shaderProgram));

		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, int32 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix3 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Matrix4 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector2 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector3 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShaderProgram& shaderProgram, ShaderParameter const& param, Vector4 const val[], int dim));
		RHI_FUNC(void setShaderMatrix22(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim));
		RHI_FUNC(void setShaderMatrix43(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim));
		RHI_FUNC(void setShaderMatrix34(RHIShaderProgram& shaderProgram, ShaderParameter const& param, float const val[], int dim));

		RHI_FUNC(void setShaderResourceView(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIShaderResourceView const& resourceView));

		RHI_FUNC(void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture));
		RHI_FUNC(void setShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler));
		RHI_FUNC(void clearShaderTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param));
		RHI_FUNC(void setShaderSampler(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHISamplerState& sampler));
		RHI_FUNC(void setShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op));
		RHI_FUNC(void clearShaderRWTexture(RHIShaderProgram& shaderProgram, ShaderParameter const& param));

		RHI_FUNC(void setShaderUniformBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer));
		RHI_FUNC(void setShaderStorageBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op));
		RHI_FUNC(void setShaderAtomicCounterBuffer(RHIShaderProgram& shaderProgram, ShaderParameter const& param, RHIVertexBuffer& buffer));

		RHI_FUNC(void RHISetGraphicsShaderBoundState(GraphicsShaderStateDesc const& stateDesc));
		RHI_FUNC(void RHISetMeshShaderBoundState(MeshShaderStateDesc const& stateDesc));
		RHI_FUNC(void RHISetComputeShader(RHIShader* shader));

		RHI_FUNC(void setShaderValue(RHIShader& shader, ShaderParameter const& param, int32 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShader& shader, ShaderParameter const& param, float const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix3 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShader& shader, ShaderParameter const& param, Matrix4 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector2 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector3 const val[], int dim));
		RHI_FUNC(void setShaderValue(RHIShader& shader, ShaderParameter const& param, Vector4 const val[], int dim));
		RHI_FUNC(void setShaderMatrix22(RHIShader& shader, ShaderParameter const& param, float const val[], int dim));
		RHI_FUNC(void setShaderMatrix43(RHIShader& shader, ShaderParameter const& param, float const val[], int dim));
		RHI_FUNC(void setShaderMatrix34(RHIShader& shader, ShaderParameter const& param, float const val[], int dim));

		RHI_FUNC(void setShaderResourceView(RHIShader& shader, ShaderParameter const& param, RHIShaderResourceView const& resourceView));

		RHI_FUNC(void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture));
		RHI_FUNC(void setShaderTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, ShaderParameter const& paramSampler, RHISamplerState & sampler));
		RHI_FUNC(void clearShaderTexture(RHIShader& shader, ShaderParameter const& param));
		RHI_FUNC(void setShaderSampler(RHIShader& shader, ShaderParameter const& param, RHISamplerState& sampler));
		RHI_FUNC(void setShaderRWTexture(RHIShader& shader, ShaderParameter const& param, RHITextureBase& texture, EAccessOperator op));
		RHI_FUNC(void clearShaderRWTexture(RHIShader& shader, ShaderParameter const& param));

		RHI_FUNC(void setShaderUniformBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer));
		RHI_FUNC(void setShaderStorageBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer, EAccessOperator op));
		RHI_FUNC(void setShaderAtomicCounterBuffer(RHIShader& shader, ShaderParameter const& param, RHIVertexBuffer& buffer));

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
