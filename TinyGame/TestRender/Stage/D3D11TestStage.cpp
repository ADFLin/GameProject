#include "Stage/TestRenderStageBase.h"

#include "RHI/RHIDefine.h"
#include "RHI/RHIType.h"
#include "RHI/RHICommon.h"

#include "RHI/ShaderProgram.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"


namespace Render
{

	struct GPU_ALIGN ColorBuffer
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(ColourBufferBlock)
		float red;
		float green;
		float blue;
	};


	class TestD3D11Stage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		TestD3D11Stage() {}
		ShaderProgram mProgTest;
		static int constexpr MaxConstBufferNum = 1;

		RHITexture2DRef mTexture;
		
		TStructuredBuffer< ColorBuffer > mCBuffer;

		RHIInputLayoutRef  mInputLayout;
		RHIBufferRef mVertexBuffer;
		RHIBufferRef  mIndexBuffer;

		RHIInputLayoutRef  mAxisInputLayout;
		RHIBufferRef mAxisVertexBuffer;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		struct MyVertex
		{
			Vector2 pos;
			Vector3 color;
			Vector2 uv;
		};

		struct AxisVertex
		{
			Vector3 pos;
			Vector3 color;
		};



		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(createSimpleMesh());

			ShaderManager::Get().loadFile(mProgTest, "Shader/Test/HLSLTest", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS));

			{
				VERIFY_RETURN_FALSE(mCBuffer.initializeResource(1));
				{
					auto pData = mCBuffer.lock();

					pData->red = 1;
					pData->green = 0;
					pData->blue = 0.5;

					mCBuffer.unlock();
				}
			}

			mTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.png");
			if (!mTexture.isValid())
				return false;

			{
				MyVertex vertices[] =
				{
					Vector2(0.5, 0.5),Vector3(1, 1, 1),Vector2(1, 1),
					Vector2(-0.5,0.5),Vector3(0, 1, 0),Vector2(0, 1),
					Vector2(-0.5,-0.5),Vector3(1, 0, 0),Vector2(0, 0),
					Vector2(0.5,-0.5),Vector3(0, 0, 1),Vector2(1, 0),
				};

				VERIFY_RETURN_FALSE(mVertexBuffer = RHICreateVertexBuffer(sizeof(MyVertex), ARRAY_SIZE(vertices), BCF_DefalutValue, vertices));

				InputLayoutDesc desc;
				desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float2);
				desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::Float3);
				desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
				VERIFY_RETURN_FALSE(mInputLayout = RHICreateInputLayout(desc));

				uint32 indices[] = { 0 , 1 , 2 , 0 , 2 , 3 };
				VERIFY_RETURN_FALSE(mIndexBuffer = RHICreateIndexBuffer(ARRAY_SIZE(indices), true, BCF_DefalutValue, indices));
			}

			{
				AxisVertex vertices[] =
				{
					{ Vector3(0,0,0),Vector3(1,0,0) },
					{ Vector3(1,0,0),Vector3(1,0,0) },
					{ Vector3(0,0,0),Vector3(0,1,0) },
					{ Vector3(0,1,0),Vector3(0,1,0) },
					{ Vector3(0,0,0),Vector3(0,0,1) },
					{ Vector3(0,0,1),Vector3(0,0,1) },
				};

				VERIFY_RETURN_FALSE(mAxisVertexBuffer = RHICreateVertexBuffer(sizeof(AxisVertex), ARRAY_SIZE(vertices), BCF_DefalutValue, vertices));

				InputLayoutDesc desc;
				desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
				desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::Float3);
				VERIFY_RETURN_FALSE(mAxisInputLayout = RHICreateInputLayout(desc));
			}

			return true;
		}


		void preShutdownRenderSystem(bool bReInit) override
		{
			BaseClass::preShutdownRenderSystem(bReInit);

			releaseRHIResource(bReInit);

			mProgTest.releaseRHI();
			mTexture.release();
			mCBuffer.releaseResource();
			mInputLayout.release();
			mVertexBuffer.release();
			mIndexBuffer.release();
			mAxisInputLayout.release();
			mAxisVertexBuffer.release();
		}

		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;


			::Global::GUI().cleanupWidget();
			
			WidgetUtility::CreateDevFrame();
			restart();
			
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() override {}


		float angle = 0;
		float worldTime = 0;

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			worldTime += deltaTime;
			angle += deltaTime;
		}



		void onRender(float dFrame) override
		{
			GPU_PROFILE("Render");

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			Vec2i screenSize = ::Global::GetScreenSize();

			mView.rectOffset = IntVector2(0, 0);
			mView.rectSize = IntVector2(screenSize.x, screenSize.y);

			mView.setupTransform(mCamera.getPos(), mCamera.getRotation(), mViewFrustum.getPerspectiveMatrix());
			mView.updateRHIResource();

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0.2, 0.2, 0.2, 1), 1);

		
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			RHISetShaderProgram(commandList, mProgTest.getRHI());

			float c = 0.5 * Math::Sin(worldTime) + 0.5;

			{
				GPU_PROFILE("Lock Buffer");
				auto pData = mCBuffer.lock();
				if ( pData )
				{
					pData->red = 1;
					pData->green = c;
					pData->blue = 0.5;

					mCBuffer.unlock();
				}

			}

			mProgTest.setTexture(commandList, SHADER_PARAM(Texture), *mTexture, SHADER_SAMPLER(Texture), TStaticSamplerState < ESampler::Trilinear >::GetRHI());
			mProgTest.setParam(commandList, SHADER_PARAM(Color), Vector3(c, c, c));
			mProgTest.setStructuredUniformBufferT< ColorBuffer >(commandList, *mCBuffer.getRHI());
			mView.setupShader(commandList, mProgTest);

			{
				GPU_PROFILE("Triangle");

				Matrix4 xform = Matrix4::Translate(Vector3(0, 0, 0));
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), xform);
				{
					InputStreamInfo inputStream;
					inputStream.buffer = mVertexBuffer;
					RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
					RHISetIndexBuffer(commandList, mIndexBuffer);
					RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, 0, mIndexBuffer->getNumElements(), 0);
				}

				//xform = Matrix4::Rotate(Vector3(0, 0, 1), angle) * Matrix4::Translate(Vector3(0, 0, 1));
				//mProgTest.setParam(commandList, SHADER_PARAM(XForm), xform);
				//{
				//	InputStreamInfo inputStream;
				//	inputStream.buffer = mVertexBuffer;
				//	RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
				//	RHISetIndexBuffer(commandList, mIndexBuffer);
				//	RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, 0, mIndexBuffer->getNumElements(), 0);
				//}
			}


			mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Scale(10));
			{
				InputStreamInfo inputStream;
				inputStream.buffer = mAxisVertexBuffer;
				RHISetInputStream(commandList, mAxisInputLayout, &inputStream, 1);

				RHIDrawPrimitive(commandList, EPrimitive::LineList, 0, 6);
			}

#if 0
			{
				GPU_PROFILE("Doughnut");
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(-10, 0, 10));
				{
					mSimpleMeshs[SimpleMeshId::Doughnut].draw(commandList);
				}
			}
#endif

			{
				GPU_PROFILE("Sphere");
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(-10, 5, 5));
				{
					mSimpleMeshs[SimpleMeshId::Sphere].draw(commandList, LinearColor(1, 0, 0, 1));
				}
			}

			{
				GPU_PROFILE("Quad");

				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(10, 0, 0));
				{
					struct Vertex_XYZ_C
					{
						Vector3 pos;
						Color4f color;
					};

					Vertex_XYZ_C vetices[] =
					{
						{ Vector3(0,0,0) , Color4f(1,0,0) },
						{ Vector3(2,0,0) , Color4f(0,1,0) },
						{ Vector3(2,2,0) , Color4f(1,1,1) },
						{ Vector3(0,2,0) , Color4f(0,0,1) },
					};
					TRenderRT< RTVF_XYZ_CA >::Draw(commandList, EPrimitive::Quad, vetices, 4);
				}

				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(0, 0, 0));
				{
					struct Vertex_XYZ
					{
						Vector3 pos;
					};

					Vertex_XYZ vetices[] =
					{
						{ Vector3(2,0,0) },
						{ Vector3(0,2,0) },
						{ Vector3(0,0,2) },
					};
					TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::TriangleList, vetices, 3, LinearColor(1, 0, 0));
				}

			}


			Matrix4 projectMatrix = OrthoMatrix(0, screenSize.x, screenSize.y, 0, -1, 1);
			RHISetFixedShaderPipelineState(commandList, AdjustProjectionMatrixForRHI(projectMatrix), LinearColor(1, 0, 0, 1));
			{

				Vector2 v[] = { Vector2(1,1) , Vector2(100,100), Vector2(1,100), Vector2(100,1) };
				uint32 indices[] = { 0, 2, 2, 1,1,3,3,0 };
				TRenderRT< RTVF_XY >::DrawIndexed(commandList, EPrimitive::LineList, v, ARRAY_SIZE(v), indices, ARRAY_SIZE(indices));
			}
			{

				Vector2 v[] = { Vector2(100,100) , Vector2(200,200), Vector2(100,200), Vector2(200,100) };
				uint32 indices[] = { 0, 2, 2, 1,1,3,3,0 };
				TRenderRT< RTVF_XY >::DrawIndexed(commandList, EPrimitive::LineList, v, ARRAY_SIZE(v), indices, ARRAY_SIZE(indices));
			}

			{
				GPU_PROFILE("Context Flush");
				RHIFlushCommand(commandList);
			}

		}


		MsgReply onKey(KeyMsg const& msg) override
		{
			if(msg.isDown())
			{
				switch( msg.getCode() )
				{
				case EKeyCode::X:
					{
						auto pData = mCBuffer.lock();

						pData->red = 0;
						pData->green = 1;
						pData->blue = 0;

						mCBuffer.unlock();
					}
					return MsgReply::Handled();
				}
			}

			return BaseClass::onKey(msg);
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}


	protected:
	};


	REGISTER_STAGE_ENTRY("D3D11 Test", TestD3D11Stage, EExecGroup::FeatureDev, "Render|RHI");

}