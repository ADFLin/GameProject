#pragma once
#ifndef RHICommandListImpl_H_F072E14F_3189_4D4D_A307_B076818D2184
#define RHICommandListImpl_H_F072E14F_3189_4D4D_A307_B076818D2184

#include "RHICommand.h"


namespace Render
{

	class RHIContext
	{
	public:
		RHI_FUNC(void RHISetRasterizerState(RHIRasterizerState& rasterizerState));
		RHI_FUNC(void RHISetBlendState(RHIBlendState& blendState));
		RHI_FUNC(void RHISetDepthStencilState(RHIDepthStencilState& depthStencilState, uint32 stencilRef = -1));

		RHI_FUNC(void RHISetViewport(int x, int y, int w, int h));
		RHI_FUNC(void RHISetScissorRect(bool bEnable, int x, int y, int w, int h));

		RHI_FUNC(void RHIDrawPrimitive(PrimitiveType type, int vStart, int nv));
		RHI_FUNC(void RHIDrawIndexedPrimitive(PrimitiveType type, ECompValueType indexType, int indexStart, int nIndex));
		RHI_FUNC(void RHIDrawPrimitiveIndirect(PrimitiveType type, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride));
		RHI_FUNC(void RHIDrawIndexedPrimitiveIndirect(PrimitiveType type, ECompValueType indexType, RHIVertexBuffer* commandBuffer, int offset, int numCommand, int commandStride));
		RHI_FUNC(void RHIDrawPrimitiveInstanced(PrimitiveType type, int vStart, int nv, int numInstance));

		RHI_FUNC(void RHIDrawPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride));
		RHI_FUNC(void RHIDrawIndexedPrimitiveUP(PrimitiveType type, int numPrimitive, void* pVertices, int numVerex, int vetexStride, int* pIndices, int numIndex));

		RHI_FUNC(void RHISetFrameBuffer(RHIFrameBuffer& frameBuffer, RHITextureDepth* overrideDepthTexture));
		RHI_FUNC(void RHISetIndexBuffer(RHIIndexBuffer* indexBuffer));
		RHI_FUNC(void RHISetupFixedPipelineState(Matrix4 const& matModelView, Matrix4 const& matProj, int numTexture, RHITexture2D const** textures));
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
