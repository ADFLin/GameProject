#include "RHICommandListImpl.h"

namespace Render
{
#define RHI_COMMAND_FUNC( COMMANDLIST , CODE ) static_cast< RHICommandListImpl& >( COMMANDLIST ).getExecutionContext().CODE

#if CORE_SHARE_CODE
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

	void RHISetViewport(RHICommandList& commandList, float x, float y, float w, float h, float zNear, float zFar)
	{
		RHI_COMMAND_FUNC(commandList, RHISetViewport(x, y, w, h, zNear, zFar));
	}

	void RHISetViewports(RHICommandList& commandList, ViewportInfo const viewports[], int numViewports)
	{
		RHI_COMMAND_FUNC(commandList, RHISetViewports(viewports, numViewports));
	}

	void RHISetScissorRect(RHICommandList& commandList, int x, int y, int w, int h)
	{
		RHI_COMMAND_FUNC(commandList, RHISetScissorRect(x, y, w, h));
	}

	void RHIDrawPrimitive(RHICommandList& commandList, EPrimitive type, int start, int nv)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitive(type, start, nv));
	}

	void RHIDrawIndexedPrimitive(RHICommandList& commandList, EPrimitive type, int indexStart, int nIndex, uint32 baseVertex)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawIndexedPrimitive(type, indexStart, nIndex, baseVertex));
	}

	void RHIDrawPrimitiveIndirect(RHICommandList& commandList, EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitiveIndirect(type, commandBuffer, offset, numCommand, commandStride));
	}

	void RHIDrawIndexedPrimitiveIndirect(RHICommandList& commandList, EPrimitive type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawIndexedPrimitiveIndirect(type, commandBuffer, offset, numCommand, commandStride));
	}

	void RHIDrawPrimitiveInstanced(RHICommandList& commandList, EPrimitive type, int vStart, int nv, uint32 numInstance, uint32 baseInstance)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitiveInstanced(type, vStart, nv, numInstance, baseInstance));
	}

	void RHIDrawIndexedPrimitiveInstanced(RHICommandList& commandList, EPrimitive type, int indexStart, int nIndex, uint32 numInstance, uint32 baseVertex, uint32 baseInstance)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawIndexedPrimitiveInstanced(type, indexStart, nIndex, numInstance, baseVertex , baseInstance));
	}

	void RHIDrawPrimitiveUP(RHICommandList& commandList, EPrimitive type, void const* pVertices, int numVertex, int vetexStride)
	{
		VertexDataInfo info;
		info.ptr = pVertices;
		info.size = vetexStride * numVertex;
		info.stride = vetexStride;
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitiveUP(type, numVertex, &info , 1 ));
	}

	void RHIDrawPrimitiveUP(RHICommandList& commandList, EPrimitive type, int numVertex, VertexDataInfo dataInfos[], int numData)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawPrimitiveUP(type, numVertex, dataInfos, numData));
	}

	void RHIDrawIndexedPrimitiveUP(RHICommandList& commandList, EPrimitive type, void const* pVertices, int numVertex, int vetexStride, int const* pIndices, int numIndex)
	{
		VertexDataInfo info;
		info.ptr = pVertices;
		info.size = vetexStride * numVertex;
		info.stride = vetexStride;
		RHI_COMMAND_FUNC(commandList, RHIDrawIndexedPrimitiveUP(type, numVertex, &info, 1, pIndices, numIndex));
	}

	void RHIDrawIndexedPrimitiveUP(RHICommandList& commandList, EPrimitive type, int numVerex, VertexDataInfo dataInfos[], int numVertexData, int const* pIndices, int numIndex)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawIndexedPrimitiveUP(type, numVerex, dataInfos, numVertexData, pIndices, numIndex));
	}

	void RHIDrawMeshTasks(RHICommandList& commandList, int start, int count)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawMeshTasks(start, count));
	}

	void RHIDrawMeshTasksIndirect(RHICommandList& commandList, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride)
	{
		RHI_COMMAND_FUNC(commandList, RHIDrawMeshTasksIndirect(commandBuffer, offset, numCommand, commandStride));
	}

	void RHISetFixedShaderPipelineState(RHICommandList& commandList, Matrix4 const& transform, LinearColor const& color, RHITexture2D* texture, RHISamplerState* sampler)
	{
		RHI_COMMAND_FUNC(commandList, RHISetFixedShaderPipelineState(transform, color, texture, sampler));
	}

	void RHISetFrameBuffer(RHICommandList& commandList, RHIFrameBuffer* frameBuffer)
	{
		RHI_COMMAND_FUNC(commandList, RHISetFrameBuffer(frameBuffer));
	}

	void RHIClearRenderTargets(RHICommandList& commandList, EClearBits clearBits, LinearColor colors[], int numColor, float depth , uint8 stenceil )
	{
		RHI_COMMAND_FUNC(commandList, RHIClearRenderTargets(clearBits, colors, numColor, depth, stenceil));
	}

	void RHISetInputStream(RHICommandList& commandList, RHIInputLayout* inputLayout, InputStreamInfo inputStreams[], int numInputStream)
	{
		RHI_COMMAND_FUNC(commandList, RHISetInputStream(inputLayout, inputStreams, numInputStream));
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

	void RHISetGraphicsShaderBoundState(RHICommandList& commandList, GraphicsShaderBoundState const& state)
	{
		RHI_COMMAND_FUNC(commandList, RHISetGraphicsShaderBoundState(state));
	}

	void RHISetMeshShaderBoundState(RHICommandList& commandList, MeshShaderBoundState const& state)
	{
		RHI_COMMAND_FUNC(commandList, RHISetMeshShaderBoundState(state));
	}

	void RHISetComputeShader(RHICommandList& commandList, RHIShader* shader)
	{
		RHI_COMMAND_FUNC(commandList, RHISetComputeShader(shader));
	}

	void RHIFlushCommand(RHICommandList& commandList)
	{
		RHI_COMMAND_FUNC(commandList, RHIFlushCommand());
	}
#endif


}//namespace Render