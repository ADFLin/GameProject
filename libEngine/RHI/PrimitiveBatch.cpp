#include "PrimitiveBatch.h"

#include "RenderContext.h"
#include "ShaderCompiler.h"
#include "RHICommand.h"

namespace RenderGL
{
	void MeshBatch::draw(RenderContext& context)
	{
		GLenum primitivTypeGL = GLConvert::To(primitiveType);

		context.setMaterial(material);
		if( context.bBindAttrib )
		{
			if( VAOHandle == 0 )
			{
				glGenVertexArrays(1, &VAOHandle);
				glBindVertexArray(VAOHandle);
				vertexBuffer->bind();
				vertexDecl->bindAttrib();
				glBindVertexArray(0);
				vertexBuffer->unbind();
				vertexDecl->unbindAttrib();
			}

			glBindVertexArray(VAOHandle);

			for( int i = 0; i < elements.size(); ++i )
			{
				MeshBatchElement& meshElement = elements[i];
				context.setWorld(meshElement.world);
				if( meshElement.indexBuffer )
				{
					GL_BIND_LOCK_OBJECT(*meshElement.indexBuffer);
					RHIDrawIndexedPrimitive(primitiveType, meshElement.indexBuffer->mbIntIndex ? CVT_UInt : CVT_UShort, meshElement.idxStart, meshElement.numElement);
				}
				else
				{
					RHIDrawPrimitive(primitiveType, meshElement.idxStart, meshElement.numElement);
				}
			}
			//checkGLError();
			glBindVertexArray(0);
		}
		else
		{
			vertexBuffer->bind();
			vertexDecl->bindPointer();

			for( int i = 0; i < elements.size(); ++i )
			{
				MeshBatchElement& meshElement = elements[i];
				context.setWorld(meshElement.world);
				if( meshElement.indexBuffer )
				{
					GL_BIND_LOCK_OBJECT(*meshElement.indexBuffer);
					RHIDrawIndexedPrimitive(primitiveType, meshElement.indexBuffer->mbIntIndex ? CVT_UInt : CVT_UShort , meshElement.idxStart , meshElement.numElement );
				}
				else
				{
					RHIDrawPrimitive(primitiveType, meshElement.idxStart, meshElement.numElement);
				}
			}
			//checkGLError();
			
			vertexDecl->unbindPointer();
			vertexBuffer->unbind();
		}
	}

	bool SimpleElementRenderer::init()
	{
		mDecl.addElement(Vertex::ATTRIBUTE_POSITION, Vertex::eFloat4);
		mDecl.addElement(Vertex::ATTRIBUTE_COLOR, Vertex::eFloat4);

		mVertexBuffer = new RHIVertexBuffer;
		mVertexBuffer->create(sizeof(SimpleVertex), MaxVertexSize, nullptr, Buffer::eDynamic);
		glGenVertexArrays(1, &mVAO);
		glBindVertexArray(mVAO);
		mVertexBuffer->bind();
		mDecl.bindAttrib();
		glBindVertexArray(0);
		mVertexBuffer->unbind();
		mDecl.unbindAttrib();

		if( !ShaderManager::Get().loadFile(
			mShader, "Shader/SimpleElement",
			SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS) ) )
			return false;

		return true;
	}

	void SimpleElementRenderer::draw( RenderContext& context , SimpleVertex* vertices, int numVertices)
	{
		void* pData = mVertexBuffer->lock(ELockAccess::WriteOnly);
		memcpy(pData, vertices, numVertices * mVertexBuffer->mVertexSize);
		mVertexBuffer->unlock();

		glBindVertexArray(mVAO);
		context.setShader(mShader);
		mShader.setParam(SHADER_PARAM(VertexTransform), context.getView().worldToClip);
		glDrawArrays(GLConvert::To(PrimitiveType::TriangleList), 0, numVertices);
		glBindVertexArray(0);
	}


	SimpleElementRenderer gSimpleElementRender;

	void PrimitivesCollection::drawDynamic(RenderContext& context)
	{
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

			Vec2i screenSize = context.getView().getViewportSize();

			RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
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
			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
		}
	}

}//namespace RenderGL