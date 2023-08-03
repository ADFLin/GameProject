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

	class SimpleElementShaderProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(SimpleElementShaderProgram, Global);

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
		static char const* GetShaderFileName()
		{
			return "Shader/SimpleElement";
		}
	};

	IMPLEMENT_SHADER_PROGRAM(SimpleElementShaderProgram);

	bool SimpleElementRenderer::initializeRHI()
	{
		InputLayoutDesc desc;
		desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float4);
		desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::Float4);

		mInputLayout = RHICreateInputLayout(desc);
		mVertexBuffer = RHICreateVertexBuffer(sizeof(SimpleVertex), MaxVertexSize, BCF_CpuAccessWrite);

		VERIFY_RETURN_FALSE( mProgram = ShaderManager::Get().getGlobalShaderT< SimpleElementShaderProgram >() );
		
		return true;
	}

	void SimpleElementRenderer::releaseRHI()
	{
		mProgram = nullptr;
		mInputLayout.release();
		mVertexBuffer.release();
	}

	void SimpleElementRenderer::draw(RHICommandList& commandList, ViewInfo& view,  SimpleVertex* vertices, int numVertices)
	{
		void* pData = RHILockBuffer( mVertexBuffer , ELockAccess::WriteDiscard);
		if (pData == nullptr)
			return;

		memcpy(pData, vertices, numVertices * mVertexBuffer->getElementSize());
		RHIUnlockBuffer(mVertexBuffer);

		RHISetShaderProgram(commandList, mProgram->getRHI());
		mProgram->setParam(commandList, SHADER_PARAM(VertexTransform), view.worldToClip);

		InputStreamInfo inputStream;
		inputStream.buffer = mVertexBuffer;
		inputStream.offset = 0;
		RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
		RHIDrawPrimitive(commandList, EPrimitive::TriangleList, 0, numVertices);
	}



	class GlobalSimpleElementRenderer : public SimpleElementRenderer
	{
	public:

		bool initializeRHI()
		{
			++mRefcount;
			if (mRefcount == 1)
			{
				cachedBuffer.reserve(SimpleElementRenderer::MaxVertexSize);
				return SimpleElementRenderer::initializeRHI();
			}
			return true;
		}

		void releaseRHI()
		{
			--mRefcount;
			if (mRefcount == 0)
			{
				SimpleElementRenderer::releaseRHI();
			}
		}
		TArray< SimpleVertex > cachedBuffer;
		int mRefcount = 0;
	};


	GlobalSimpleElementRenderer GSimpleElementRender;


	void PrimitivesCollection::initializeRHI()
	{
		GSimpleElementRender.initializeRHI();
	}

	void PrimitivesCollection::releaseRHI()
	{
		GSimpleElementRender.releaseRHI();
	}

	void PrimitivesCollection::drawDynamic(RenderContext& context)
	{
		drawDynamic(context.getCommnadList(), context.getView());
	}

	void PrimitivesCollection::drawDynamic(RHICommandList& commandList, ViewInfo& view)
	{

		if (!mLineBatchs.empty())
		{
			auto& cachedBuffer = GSimpleElementRender.cachedBuffer;
			cachedBuffer.clear();

			Vector3 cameraX = view.getViewRightDir();
			Vector3 cameraY = view.getViewUpDir();

			IntVector2 screenSize = view.getViewportSize();

			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

			{
				int idxLine = 0;
				for (; idxLine < mLineBatchs.size(); ++idxLine)
				{
					int LineVertexCount = 24;

					if (cachedBuffer.size() + LineVertexCount > SimpleElementRenderer::MaxVertexSize)
					{
						GSimpleElementRender.draw(commandList, view, cachedBuffer.data(), cachedBuffer.size());
						cachedBuffer.clear();
					}

					LineBatch& line = mLineBatchs[idxLine];

					bool bScreenSpace = line.thickness >= 0;

					float tickness = Math::Abs(line.thickness);
					float offsetScaleS = tickness;
					float offsetScaleE = tickness;

					if (bScreenSpace)
					{
						float posWS = (Vector4(line.start, 1) * view.worldToClip).w;
						float posWE = (Vector4(line.end, 1) * view.worldToClip).w;

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


					cachedBuffer.push_back(vSLB); cachedBuffer.push_back(vSRB); cachedBuffer.push_back(vSRT);
					cachedBuffer.push_back(vSLB); cachedBuffer.push_back(vSRT); cachedBuffer.push_back(vSLT);

					cachedBuffer.push_back(vELB); cachedBuffer.push_back(vERB); cachedBuffer.push_back(vERT);
					cachedBuffer.push_back(vELB); cachedBuffer.push_back(vERT); cachedBuffer.push_back(vELT);

					cachedBuffer.push_back(vSLT); cachedBuffer.push_back(vSRB); cachedBuffer.push_back(vERB);
					cachedBuffer.push_back(vSLT); cachedBuffer.push_back(vERB); cachedBuffer.push_back(vELT);

					cachedBuffer.push_back(vSRT); cachedBuffer.push_back(vERT); cachedBuffer.push_back(vELB);
					cachedBuffer.push_back(vSRT); cachedBuffer.push_back(vELB); cachedBuffer.push_back(vSLB);

				}
				if (cachedBuffer.size())
				{
					GSimpleElementRender.draw(commandList, view, &cachedBuffer[0], cachedBuffer.size());
				}

			}
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		}
	}


}//namespace Render