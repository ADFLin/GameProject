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


		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_SHADER_PARAM(parameterMap, VertexTransform);
		}

		DEFINE_SHADER_PARAM(VertexTransform);
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

	ACCESS_SHADER_MEMBER_PARAM(VertexTransform);

	void SimpleElementRenderer::draw(RHICommandList& commandList, Matrix4 const& worldToClip, SimpleVertex* vertices, int numVertices)
	{
		void* pData = RHILockBuffer( mVertexBuffer , ELockAccess::WriteDiscard);
		if (pData == nullptr)
			return;

		memcpy(pData, vertices, numVertices * mVertexBuffer->getElementSize());
		RHIUnlockBuffer(mVertexBuffer);

		RHISetShaderProgram(commandList, mProgram->getRHI());
		SET_SHADER_PARAM_VALUE(commandList, *mProgram, VertexTransform, worldToClip);

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
#if 0

		virtual void restoreRHI() override
		{
			if (mRefcount > 0)
			{
				SimpleElementRenderer::initializeRHI();
			}
		}
		virtual void releaseRHI() override
		{
			if (mRefcount > 0)
			{
				SimpleElementRenderer::releaseRHI();
			}
		}
#endif
		TArray< SimpleVertex > cachedBuffer;
		int mRefcount = 0;
	};


	GlobalSimpleElementRenderer GSimpleElementRender;


	bool PrimitivesCollection::initializeRHI()
	{
		return GSimpleElementRender.initializeRHI();
	}

	void PrimitivesCollection::releaseRHI()
	{
		GSimpleElementRender.releaseRHI();
	}

	void PrimitivesCollection::addCubeLine(Vector3 const& pos, Quaternion const& rotation, Vector3 const& extents, LinearColor const& color, float thickness /*= 1*/)
	{
		static Vector3 const v[] =
		{
			Vector3(1,0,0),Vector3(1,1,0),Vector3(1,1,0),Vector3(1,1,1),
			Vector3(1,1,1),Vector3(1,0,1),Vector3(1,0,1),Vector3(1,0,0),
			Vector3(0,0,0),Vector3(0,1,0),Vector3(0,1,0),Vector3(0,1,1),
			Vector3(0,1,1),Vector3(0,0,1),Vector3(0,0,1),Vector3(0,0,0),
			Vector3(0,0,0),Vector3(1,0,0),Vector3(0,1,0),Vector3(1,1,0),
			Vector3(0,1,1),Vector3(1,1,1),Vector3(0,0,1),Vector3(1,0,1),
		};

		for (int i = 0; i < ARRAY_SIZE(v); i += 2)
		{
			Vector3 p0 = pos + rotation.rotate(extents * (v[i] - Vector3(0.5, 0.5, 0.5)));
			Vector3 p1 = pos + rotation.rotate(extents * (v[i + 1] - Vector3(0.5, 0.5, 0.5)));
			addLine(p0, p1, color, thickness);
		}
	}

	void PrimitivesCollection::drawDynamic(RenderContext& context)
	{
		drawDynamic(context.getCommnadList(), context.getView());
	}

	void PrimitivesCollection::drawDynamic(RHICommandList& commandList, ViewInfo& view)
	{
		drawDynamic(commandList, view.getViewportSize(), view.worldToClip, view.getViewRightDir(), view.getViewUpDir());
	}

	void PrimitivesCollection::drawDynamic(RHICommandList& commandList, IntVector2 const& screenSize, Matrix4 const& worldToClip, Vector3 const& cameraX, Vector3 const& cameraY)
	{
		if (!mLineBatchs.empty())
		{
			auto& cachedBuffer = GSimpleElementRender.cachedBuffer;
			cachedBuffer.clear();

			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

			{
				auto worldToClipRHI = AdjustProjectionMatrixForRHI(worldToClip);

				int idxLine = 0;
				for (; idxLine < mLineBatchs.size(); ++idxLine)
				{
					int LineVertexCount = 24;

					if (cachedBuffer.size() + LineVertexCount > SimpleElementRenderer::MaxVertexSize)
					{
						GSimpleElementRender.draw(commandList, worldToClipRHI, cachedBuffer.data(), cachedBuffer.size());
						cachedBuffer.clear();
					}

					LineBatch& line = mLineBatchs[idxLine];

					bool bScreenSpace = line.thickness >= 0;

					float tickness = Math::Abs(line.thickness);
					float offsetScaleS = tickness;
					float offsetScaleE = tickness;

					if (bScreenSpace)
					{
						float posWS = (Vector4(line.start, 1) * worldToClip).w;
						float posWE = (Vector4(line.end, 1) * worldToClip).w;

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
					GSimpleElementRender.draw(commandList, worldToClipRHI, &cachedBuffer[0], cachedBuffer.size());
				}

			}
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		}
	}


}//namespace Render