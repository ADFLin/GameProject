#include "TestRenderStageBase.h"

#include "RHI/RHIDefine.h"
#include "RHI/BaseType.h"
#include "RHI/RHICommon.h"
#include "RHI/D3D11Command.h"

#include "RHI/ShaderProgram.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"


namespace Render
{
	template < uint32 VertexFormat >
	class TRenderRT_D3D
	{

	public:
		FORCEINLINE static void Draw(PrimitiveType type, void const* vetrices, int nV, int vertexStride = GetVertexSize())
		{

		}

		template< uint32 VF, uint32 SEMANTIC >
		struct VertexElementOffset
		{
			enum { Result = sizeof(float) * (VF & RTS_ELEMENT_MASK) + VertexElementOffset< (VF >> RTS_ELEMENT_BIT_OFFSET), SEMANTIC - 1 >::Result };
		};

		template< uint32 VF >
		struct VertexElementOffset< VF, 0 >
		{
			enum { Result = 0 };
		};

#define USE_SEMANTIC( S ) ( ( VertexFormat ) & RTS_ELEMENT( S , RTS_ELEMENT_MASK ) )
#define VERTEX_ELEMENT_SIZE( S ) ( USE_SEMANTIC( S ) >> ( RTS_ELEMENT_BIT_OFFSET * S ) )
#define VETEX_ELEMENT_OFFSET( S ) VertexElementOffset< VertexFormat , S >::Result


#undef USE_SEMANTIC
#undef VERTEX_ELEMENT_SIZE
#undef VETEX_ELEMENT_OFFSET
	};

	struct GPU_BUFFER_ALIGN ColourBuffer
	{
		DECLARE_BUFFER_STRUCT(ColourBufferBlock)
		float red;
		float green;
		float blue;
	};

	


	class TestD3D11Stage : public TestRenderStageBase
	{
		typedef TestRenderStageBase BaseClass;
	public:
		TestD3D11Stage() {}
		D3D11System* mRHISystem;

		ShaderProgram mProgTest;
		static int constexpr MaxConstBufferNum = 1;

		FrameSwapChain mSwapChain;

		RHITexture2DRef mTexture;
		TStructuredBuffer< ColourBuffer > mCBuffer;

		virtual RHITargetName getRHITargetName() { return RHITargetName::D3D11; }

		struct MyVertex
		{
			Vector2 pos;
			Vector3 color;
			Vector2 uv;
		};

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			//VERIFY_RETURN_FALSE( createSimpleMesh() );

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
				auto pData = mCBuffer.lock();

				pData->red = 1;
				pData->green = 0;
				pData->blue = 0.5;

				mCBuffer.unlock();
			}

			mTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.png");
			if( !mTexture.isValid() )
				return false;

			HRESULT hr;
			TComPtr< ID3D11Device >& device = mRHISystem->mDevice;
			TComPtr< ID3D11Texture2D > backBuffer;
			VERIFY_D3D11RESULT_RETURN_FALSE( mSwapChain.ptr->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

			VERIFY_D3D11RESULT_RETURN_FALSE( device->CreateRenderTargetView( backBuffer , NULL, &renderTargetView) );
			TComPtr< ID3D11DeviceContext >& context = mRHISystem->mDeviceContext;

			{
				MyVertex vertices[] =
				{
					Vector2(0.5, 0.5),Vector3(1, 1, 1),Vector2(1, 1),
					Vector2(0.5,-0.5),Vector3(0, 0, 1),Vector2(1, 0),
					Vector2(-0.5,0.5),Vector3(0, 1, 0),Vector2(0, 1),
					Vector2(-0.5,-0.5),Vector3(1, 0, 0),Vector2(0, 0),			
				};

				VERIFY_RETURN_FALSE( mVertexBuffer = RHICreateVertexBuffer(sizeof(MyVertex), ARRAY_SIZE(vertices), BCF_DefalutValue, vertices) );

				InputLayoutDesc desc;
				desc.addElement(0, Vertex::ATTRIBUTE0, Vertex::eFloat2);
				desc.addElement(0, Vertex::ATTRIBUTE1, Vertex::eFloat3);
				desc.addElement(0, Vertex::ATTRIBUTE2, Vertex::eFloat2);
				VERIFY_RETURN_FALSE( mInputLayout = RHICreateInputLayout(desc) );
			}

			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}
		TComPtr< ID3D11RenderTargetView > renderTargetView;
		RHIInputLayoutRef mInputLayout;
		RHIVertexBufferRef mVertexBuffer;
		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}


		float angle = 0;
		float worldTime = 0;

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);


			worldTime += 0.001 * time;
			angle += 0.001 * time;
		}



		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			GameWindow& window = Global::GetDrawEngine().getWindow();

			mView.rectOffset = IntVector2(0, 0);
			mView.rectSize = IntVector2(window.getWidth(), window.getHeight());

			Matrix4 matView = mCamera.getViewMatrix();
			mView.setupTransform(matView, mViewFrustum.getPerspectiveMatrix());

			TComPtr< ID3D11DeviceContext >& context = mRHISystem->mDeviceContext;
			TComPtr< ID3D11Device >& device = mRHISystem->mDevice;

			context->ClearRenderTargetView(renderTargetView ,Vector4(0.2, 0.2, 0.2,1));
			context->OMSetRenderTargets(1, &renderTargetView, NULL);

			RHISetViewport(commandList, 0, 0, window.getWidth(), window.getHeight());

			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());

			RHISetShaderProgram(commandList, mProgTest.getRHIResource());


			float c = 0.5 * Math::Sin(worldTime) + 0.5;
			Matrix4 xform = Matrix4::Rotate(Vector3(0, 0, 1), angle) * Matrix4::Translate(Vector3(0.2, 0, 0));
			mProgTest.setTexture(commandList, SHADER_PARAM(Texture), *mTexture, SHADER_PARAM(TextureSampler), TStaticSamplerState < Sampler::eTrilinear >::GetRHI());
			mProgTest.setParam(commandList, SHADER_PARAM(XForm), xform);
			mProgTest.setParam(commandList, SHADER_PARAM(Color), Vector3(c, c, c));
			mProgTest.setStructuredUniformBufferT< ColourBuffer >(commandList, *mCBuffer.getRHI());
			mView.setupShader(commandList, mProgTest);

			{
				UINT stride = sizeof(MyVertex);
				UINT offset = 0;
				context->IASetInputLayout(D3D11Cast::GetResource(mInputLayout));
				ID3D11Buffer* buffers[] = { D3D11Cast::GetResource(mVertexBuffer) };
				context->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
				RHIDrawPrimitive(commandList, PrimitiveType::TriangleStrip, 0, 4);
			}


			context->Flush();

			if( !::Global::GetDrawEngine().bUsePlatformBuffer )
			{
				mSwapChain.ptr->Present(1, 0);
			}
			else
			{
				TComPtr<IDXGISurface1> surface;
				VERIFY_D3D11RESULT( mSwapChain.ptr->GetBuffer(0, IID_PPV_ARGS(&surface)) , );
				HDC hDC;
				VERIFY_D3D11RESULT( surface->GetDC(FALSE, &hDC) , );

				int w = ::Global::GetDrawEngine().getScreenWidth();
				int h = ::Global::GetDrawEngine().getScreenHeight();
				::BitBlt(g.getRenderDC(), 0, 0, w, h, hDC, 0, 0, SRCCOPY);
				VERIFY_D3D11RESULT( surface->ReleaseDC(NULL) , );
			}
		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

	protected:
	};


	REGISTER_STAGE("D3D11 Test", TestD3D11Stage, EStageGroup::FeatureDev);

}