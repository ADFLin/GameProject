#include "RHICommandListImpl.h"

namespace Render
{
#define RHI_COMMAND_FUNC( COMMANDLIST , CODE ) static_cast< RHICommandListImpl& >( COMMANDLIST ).getExecutionContext().CODE

	void RHISetRasterizerState(RHICommandList& commandList, RHIRasterizerState& rasterizerState)
	{
		RHI_COMMAND_FUNC(commandList, RHISetRasterizerState(rasterizerState));
	}
	void RHISetBlendState(RHICommandList& commandList, RHIBlendState& blendState)
	{
		RHI_COMMAND_FUNC(commandList, RHISetBlendState(blendState));
	}
	void RHISetDepthStencilState(RHICommandList& commandList, RHIDepthStencilState& depthStencilState, uint32 stencilRef)
	{
		RHI_COMMAND_FUNC(commandList, RHISetDepthStencilState(depthStencilState, stencilRef));
	}

	void RHISetViewport(RHICommandList& commandList, int x, int y, int w, int h)
	{
		RHI_COMMAND_FUNC(commandList, RHISetViewport(x, y, w, h));
	}

	void RHISetScissorRect(RHICommandList& commandList, int x, int y, int w, int h)
	{
		RHI_COMMAND_FUNC(commandList, RHISetScissorRect(x, y, w, h));
	}

	void RHIDrawPrimitive(RHICommandList& commandList, PrimitiveType type, int start, int nv)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitive(type, start, nv));
	}

	void RHIDrawIndexedPrimitive(RHICommandList& commandList, PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex, uint32 baseVertex)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawIndexedPrimitive(type, indexType, indexStart, nIndex, baseVertex));
	}

	void RHIDrawPrimitiveIndirect(RHICommandList& commandList, PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitiveIndirect(type, commandBuffer, offset, numCommand, commandStride));
	}

	void RHIDrawIndexedPrimitiveIndirect(RHICommandList& commandList, PrimitiveType type, ECompValueType indexType, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawIndexedPrimitiveIndirect(type, indexType, commandBuffer, offset, numCommand, commandStride));
	}

	void RHIDrawPrimitiveInstanced(RHICommandList& commandList, PrimitiveType type, int vStart, int nv, int numInstance)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitiveInstanced(type, vStart, nv, numInstance));
	}

	void RHIDrawPrimitiveUP(RHICommandList& commandList, PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitiveUP(type, numPrimitive, pVertices, numVerex, vetexStride));
	}

	void RHIDrawIndexedPrimitiveUP(RHICommandList& commandList, PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride, int* pIndices, int numIndex)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawIndexedPrimitiveUP(type, numPrimitive, pVertices, numVerex, vetexStride, pIndices, numIndex));
	}

	void RHISetupFixedPipelineState(RHICommandList& commandList, Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture /*= 0*/, RHITexture2D const** textures /*= nullptr*/)
	{
		RHI_COMMAND_FUNC(commandList, RHISetupFixedPipelineState(matModelView, matProj, numTexture, textures));
	}

	void RHISetFrameBuffer(RHICommandList& commandList, RHIFrameBuffer& frameBuffer, RHITextureDepth* overrideDepthTexture /*= nullptr*/)
	{
		RHI_COMMAND_FUNC(commandList, RHISetFrameBuffer(frameBuffer, overrideDepthTexture));
	}

	void RHISetIndexBuffer(RHICommandList& commandList, RHIIndexBuffer* indexBuffer)
	{
		RHI_COMMAND_FUNC(commandList, RHISetIndexBuffer(indexBuffer));
	}

	void RHIDispatchCompute(RHICommandList& commandList, uint32 numGroupX, uint32 numGroupY, uint32 numGroupZ)
	{
		RHI_COMMAND_FUNC(commandList, RHIDispatchCompute(numGroupX, numGroupY, numGroupZ));
	}

	void RHISetShaderProgram(RHICommandList& commandList, RHIShaderProgram* shaderProgram)
	{
		RHI_COMMAND_FUNC(commandList, RHISetShaderProgram(shaderProgram));
	}


}//namespace Render