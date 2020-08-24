#include "PrimitiveBatch.h"

#include "RenderContext.h"
#include "ShaderManager.h"
#include "RHICommand.h"

namespace Render
{
	void MeshBatch::draw(RenderContext& context)
	{
		RHICommandList& commandList = context.getCommnadList();

		context.setMaterial(material);

		InputStreamInfo inputSteam;
		inputSteam.buffer = vertexBuffer;
		inputSteam.offset = 0;
		RHISetInputStream(commandList, inputLayout, &inputSteam, 1);

		for( int i = 0; i < elements.size(); ++i )
		{
			MeshBatchElement& meshElement = elements[i];
			context.setWorld(meshElement.world);
			if( meshElement.indexBuffer )
			{
				RHISetIndexBuffer(commandList, meshElement.indexBuffer);
				RHIDrawIndexedPrimitive(commandList, primitiveType, meshElement.idxStart, meshElement.numElement);
			}
			else
			{
				RHIDrawPrimitive(commandList, primitiveType, meshElement.idxStart, meshElement.numElement);
			}
		}
	}

	bool SimpleElementRenderer::init()
	{
		InputLayoutDesc desc;
		desc.addElement(0, Vertex::ATTRIBUTE_POSITION, Vertex::eFloat4);
		desc.addElement(0, Vertex::ATTRIBUTE_COLOR, Vertex::eFloat4);

		mInputLayout = RHICreateInputLayout(desc);
		mVertexBuffer = RHICreateVertexBuffer(sizeof(SimpleVertex), MaxVertexSize, BCF_CpuAccessWrite);

		if( !ShaderManager::Get().loadFile(
			mShader, "Shader/SimpleElement",
			SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS) ) )
			return false;

		return true;
	}

	void SimpleElementRenderer::draw( RenderContext& context , SimpleVertex* vertices, int numVertices)
	{
		RHICommandList& commandList = context.getCommnadList();

		void* pData = RHILockBuffer( mVertexBuffer , ELockAccess::WriteOnly);
		memcpy(pData, vertices, numVertices * mVertexBuffer->getElementSize());
		RHIUnlockBuffer(mVertexBuffer);

		context.setShader(mShader);
		mShader.setParam(commandList, SHADER_PARAM(VertexTransform), context.getView().worldToClip);

		InputStreamInfo inputStream;
		inputStream.buffer = mVertexBuffer;
		inputStream.offset = 0;
		RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
		RHIDrawPrimitive(commandList, EPrimitive::TriangleList, 0, numVertices);
	}


	SimpleElementRenderer gSimpleElementRender;

	void PrimitivesCollection::drawDynamic(RenderContext& context)
	{
		RHICommandList& commandList = context.getCommnadList();
		static bool bInit = false;
		if( !bInit )
		{
			mCacheBuffer.reserve(SimpleElementRenderer::MaxVertexSize);
			gSimpleElementRender.init();
			bInit = true;
		}

		if ( !mLineBatchs.empty() )
		{
			mCacheBuffer.clear();

			Vector3 cameraX = context.getView().getViewRightDir();
			Vector3 cameraY = context.getView().getViewUpDir();

			IntVector2 screenSize = context.getView().getViewportSize();

			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
			int idxLine = 0;
			{
				for( ; idxLine < mLineBatchs.size(); ++idxLine )
				{
					LineBatch& line = mLineBatchs[idxLine];

					bool bScreenSpace = line.thickness >= 0;

					float tickness = Math::Abs(line.thickness);
					float offsetScaleS = tickness;
					float offsetScaleE = tickness;

					if( bScreenSpace )
					{
						float posWS = (Vector4(line.start, 1) * context.getView().worldToClip).w;
						float posWE = (Vector4(line.end, 1) * context.getView().worldToClip).w;

						offsetScaleS *= 0.5 * posWS / float(screenSize.x);
						offsetScaleE *= 0.5 * posWE / float(screenSize.x);
					}


					Vector3 offsetSR = offsetScaleS * cameraX;
					Vector3 offsetST = offsetScaleS * cameraY;

					Vector3 offsetER = offsetScaleE * cameraX;
					Vector3 offsetET = offsetScaleE * cameraY;


					SimpleVertex vSRT = SimpleVertex(line.start + offsetSR + offsetST, line.color);
					SimpleVertex vSLT = SimpleVertex(line.start - offsetSR + offsetST, line.color);
					SimpleVertex vSRB = SimpleVertex(line.start + offsetSR - offsetST, line.color);
					SimpleVertex vSLB = SimpleVertex(line.start - offsetSR - offsetST, line.color);

					SimpleVertex vERT = SimpleVertex(line.end + offsetER + offsetET, line.color);
					SimpleVertex vELT = SimpleVertex(line.end - offsetER + offsetET, line.color);
					SimpleVertex vERB = SimpleVertex(line.end + offsetER - offsetET, line.color);
					SimpleVertex vELB = SimpleVertex(line.end - offsetER - offsetET, line.color);

					mCacheBuffer.push_back(vSLB); mCacheBuffer.push_back(vSRB); mCacheBuffer.push_back(vSRT);
					mCacheBuffer.push_back(vSLB); mCacheBuffer.push_back(vSRT); mCacheBuffer.push_back(vSLT);

					mCacheBuffer.push_back(vELB); mCacheBuffer.push_back(vERB); mCacheBuffer.push_back(vERT);
					mCacheBuffer.push_back(vELB); mCacheBuffer.push_back(vERT); mCacheBuffer.push_back(vELT);

					mCacheBuffer.push_back(vSLT); mCacheBuffer.push_back(vSRB); mCacheBuffer.push_back(vERB);
					mCacheBuffer.push_back(vSLT); mCacheBuffer.push_back(vERB); mCacheBuffer.push_back(vELT);

					mCacheBuffer.push_back(vSRT); mCacheBuffer.push_back(vERT); mCacheBuffer.push_back(vELB);
					mCacheBuffer.push_back(vSRT); mCacheBuffer.push_back(vELB); mCacheBuffer.push_back(vSLB);

				}


				gSimpleElementRender.draw(context, &mCacheBuffer[0], mCacheBuffer.size());

			}
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		}
	}

}//namespace Render