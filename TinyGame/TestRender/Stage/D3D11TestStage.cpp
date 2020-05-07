#include "TestRenderStageBase.h"

#include "RHI/RHIDefine.h"
#include "RHI/RHIType.h"
#include "RHI/RHICommon.h"
#include "RHI/D3D11Command.h"

#include "RHI/ShaderProgram.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"


namespace Render
{

	struct GPU_BUFFER_ALIGN ColourBuffer
	{
		DECLARE_BUFFER_STRUCT(ColourBufferBlock)
		float red;
		float green;
		float blue;
	};

	
	class TestD3D11Stage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		TestD3D11Stage() {}
		D3D11System* mRHISystem;

		ShaderProgram mProgTest;
		static int constexpr MaxConstBufferNum = 1;

		FrameSwapChain mSwapChain;
		RHITextureDepthRef mDepthTexture;
		RHITexture2DRef mTexture;
		
		TStructuredBuffer< ColourBuffer > mCBuffer;
		TComPtr< ID3D11RenderTargetView > renderTargetView;
		RHIInputLayoutRef  mInputLayout;
		RHIVertexBufferRef mVertexBuffer;
		RHIIndexBufferRef  mIndexBuffer;

		RHIInputLayoutRef  mAxisInputLayout;
		RHIVertexBufferRef mAxisVertexBuffer;

		RHITargetName getRHITargetName() override { return RHITargetName::D3D11; }

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

		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;

			VERIFY_RETURN_FALSE( createSimpleMesh() );

			mRHISystem = static_cast<D3D11System*>(gRHISystem);

			GameWindow& window = ::Global::GetDrawEngine().getWindow();
			::Global::GetDrawEngine().bUsePlatformBuffer = true;
			if( !mRHISystem->createFrameSwapChain(window.getHWnd(), window.getWidth(), window.getHeight(), true, mSwapChain) )
			{
				return false;
			}

			ShaderManager::Get().loadFile(mProgTest, "Shader/Game/HLSLTest", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS));

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
			if( !mTexture.isValid() )
				return false;

			HRESULT hr;
			TComPtr< ID3D11Device >& device = mRHISystem->mDevice;
			TComPtr< ID3D11Texture2D > backBuffer;
			VERIFY_D3D11RESULT_RETURN_FALSE( mSwapChain.ptr->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
			VERIFY_D3D11RESULT_RETURN_FALSE( device->CreateRenderTargetView( backBuffer , nullptr, &renderTargetView) );
			mDepthTexture = RHICreateTextureDepth(Texture::eDepth32F, window.getWidth(), window.getHeight() );


			TComPtr< ID3D11DeviceContext >& context = mRHISystem->mDeviceContext;

			{
				MyVertex vertices[] =
				{
					Vector2(0.5, 0.5),Vector3(1, 1, 1),Vector2(1, 1),
					Vector2(-0.5,0.5),Vector3(0, 1, 0),Vector2(0, 1),
					Vector2(-0.5,-0.5),Vector3(1, 0, 0),Vector2(0, 0),
					Vector2(0.5,-0.5),Vector3(0, 0, 1),Vector2(1, 0),
				};

				VERIFY_RETURN_FALSE( mVertexBuffer = RHICreateVertexBuffer(sizeof(MyVertex), ARRAY_SIZE(vertices), BCF_DefalutValue, vertices) );

				InputLayoutDesc desc;
				desc.addElement(0, Vertex::ATTRIBUTE_POSITION, Vertex::eFloat2);
				desc.addElement(0, Vertex::ATTRIBUTE_COLOR, Vertex::eFloat3);
				desc.addElement(0, Vertex::ATTRIBUTE_TEXCOORD, Vertex::eFloat2);
				VERIFY_RETURN_FALSE( mInputLayout = RHICreateInputLayout(desc) );

				int32 indices[] = { 0 , 1 , 2 , 0 , 2 , 3 };
				VERIFY_RETURN_FALSE(mIndexBuffer = RHICreateIndexBuffer( ARRAY_SIZE(indices) , true , BCF_DefalutValue , indices ) );
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
				desc.addElement(0, Vertex::ATTRIBUTE_POSITION, Vertex::eFloat3);
				desc.addElement(0, Vertex::ATTRIBUTE_COLOR, Vertex::eFloat3);
				VERIFY_RETURN_FALSE(mAxisInputLayout = RHICreateInputLayout(desc));
			}

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
		void tick() override {}
		void updateFrame(int frame) override {}


		float angle = 0;
		float worldTime = 0;

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);


			worldTime += 0.001 * time;
			angle += 0.001 * time;
		}



		void onRender(float dFrame) override
		{
			GPU_PROFILE("Render");

			Graphics2D& g = Global::GetGraphics2D();

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			GameWindow& window = Global::GetDrawEngine().getWindow();

			mView.rectOffset = IntVector2(0, 0);
			mView.rectSize = IntVector2(window.getWidth(), window.getHeight());

			Matrix4 matView = mCamera.getViewMatrix();
			mView.setupTransform(matView, mViewFrustum.getPerspectiveMatrix());
			mView.updateBuffer();

			TComPtr< ID3D11DeviceContext >& context = mRHISystem->mDeviceContext;
			TComPtr< ID3D11Device >& device = mRHISystem->mDevice;

			context->ClearRenderTargetView(renderTargetView ,Vector4(0.2, 0.2, 0.2,1));
			context->ClearDepthStencilView(D3D11Cast::To(mDepthTexture)->mDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
			context->OMSetRenderTargets(1, &renderTargetView, D3D11Cast::To(mDepthTexture)->mDSV );
			
			RHISetViewport(commandList, 0, 0, window.getWidth(), window.getHeight());

			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			RHISetShaderProgram(commandList, mProgTest.getRHIResource());

			float c = 0.5 * Math::Sin(worldTime) + 0.5;

			{
				GPU_PROFILE("Lock Buffer");
				auto pData = mCBuffer.lock();

				pData->red = 1;
				pData->green = c;
				pData->blue = 0.5;

				mCBuffer.unlock();
			}

			mProgTest.setTexture(commandList, SHADER_PARAM(Texture), *mTexture, SHADER_PARAM(TextureSampler), TStaticSamplerState < Sampler::eTrilinear >::GetRHI());
			mProgTest.setParam(commandList, SHADER_PARAM(Color), Vector3(c, c, c));
			mProgTest.setStructuredUniformBufferT< ColourBuffer >(commandList, *mCBuffer.getRHI());
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

				xform = Matrix4::Rotate(Vector3(0, 0, 1), angle) * Matrix4::Translate(Vector3(0, 0, 1));
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), xform);
				{
					InputStreamInfo inputStream;
					inputStream.buffer = mVertexBuffer;
					RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
					RHISetIndexBuffer(commandList, mIndexBuffer);
					RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, 0, mIndexBuffer->getNumElements(), 0);
				}
			}
		

			mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Identity());
			{
				InputStreamInfo inputStream;
				inputStream.buffer = mAxisVertexBuffer;
				RHISetInputStream(commandList, mAxisInputLayout, &inputStream , 1 );

				RHIDrawPrimitive(commandList, EPrimitive::LineList, 0, 6);
			}

			{
				GPU_PROFILE("Doughnut");
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(-10, 0, 10));
				{
					mSimpleMeshs[SimpleMeshId::Doughnut].draw(commandList);
				}
			}

			{
				GPU_PROFILE("Sphere");
				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(-10, 5, 5));
				{
					mSimpleMeshs[SimpleMeshId::Sphere].draw(commandList, Vector3(1, 0, 0));
				}
			}

			{
				GPU_PROFILE("Quad");

				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(10, 0, 0));
				{
					struct Vertex_XYZ_C
					{
						Vector3 pos;
						Vector3 color;
					};

					Vertex_XYZ_C vetices[] =
					{
						{ Vector3(0,0,0) , Vector3(1,0,0) },
						{ Vector3(2,0,0) , Vector3(0,1,0) },
						{ Vector3(2,2,0) , Vector3(1,1,1) },
						{ Vector3(0,2,0) , Vector3(0,0,1) },
					};
					TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::Quad, vetices, 4);
				}

				mProgTest.setParam(commandList, SHADER_PARAM(XForm), Matrix4::Translate(0, 10, 0));
				{
					struct Vertex_XYZ
					{
						Vector3 pos;
					};

					Vertex_XYZ vetices[] =
					{
						{ Vector3(0,0,0) },
						{ Vector3(2,0,0) },
						{ Vector3(2,2,0) },
						{ Vector3(0,2,0) },
					};
					TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::Quad, vetices, 4, Vector3(1, 0, 0));
				}

			}

			{
				GPU_PROFILE("Context Flush");

				context->Flush();
			}
			{
				GPU_PROFILE("Present");

				if( !::Global::GetDrawEngine().bUsePlatformBuffer )
				{
					mSwapChain.ptr->Present(1, 0);
				}
				else
				{
					TComPtr<IDXGISurface1> surface;
					VERIFY_D3D11RESULT(mSwapChain.ptr->GetBuffer(0, IID_PPV_ARGS(&surface)), );
					HDC hDC;
					VERIFY_D3D11RESULT(surface->GetDC(FALSE, &hDC), );

					int w = ::Global::GetDrawEngine().getScreenWidth();
					int h = ::Global::GetDrawEngine().getScreenHeight();
					::BitBlt(g.getRenderDC(), 0, 0, w, h, hDC, 0, 0, SRCCOPY);
					VERIFY_D3D11RESULT(surface->ReleaseDC(nullptr), );
				}
			}
		}


		bool onKey(KeyMsg const& msg) override
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
					return false;
				}
			}

			return BaseClass::onKey(msg);
		}

		bool onMouse(MouseMsg const& msg) override
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

	protected:
	};


	REGISTER_STAGE("D3D11 Test", TestD3D11Stage, EStageGroup::FeatureDev);

}