#include "TestRenderStageBase.h"

#include "RHI/RHIDefine.h"
#include "RHI/RHIType.h"
#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"

#include "RHI/ShaderProgram.h"
#include "RHI/ShaderManager.h"
#include "RHI/RenderContext.h"



#include "RHI/D3D12Command.h"

#pragma comment(lib , "D3D12.lib")
#pragma comment(lib , "DXGI.lib")
#pragma comment(lib , "dxguid.lib")


#include "RHI/D3D12ShaderCommon.h"
#include "RHI/D3D12Utility.h"
#include "RHI/RHIGraphics2D.h"

namespace Render
{

	struct GPU_ALIGN ColourBuffer
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(ColourBufferBlock)
		float red;
		float green;
		float blue;
	};


	struct ConstBuffer
	{
		Vector4 value;
	};


	class SimpleProgram : public GlobalShaderProgram
	{
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(SimpleProgram, Global);
	public:

		static char const* GetShaderFileName()
		{
			return "Shader/Test/D3D12Simple";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};

	IMPLEMENT_SHADER_PROGRAM(SimpleProgram);

	class TestD3D12Stage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		TestD3D12Stage() {}

		RHITexture2DRef mTexture;
		RHITexture2DRef mTexture1;

		RHIVertexBufferRef mVertexBuffer;
		RHIIndexBufferRef  mIndexBuffer;
		RHIInputLayoutRef  mInputLayout;

		bool bUseProgram = false;

		SimpleProgram* mProgTriangle;
		Shader mVertexShader;
		Shader mPixelShader;
	
		static const UINT TextureWidth = 256;
		static const UINT TextureHeight = 256;
		static const UINT TexturePixelSize = 4;

		std::vector<UINT8> GenerateTextureData(int cellSize = 3)
		{
			const UINT rowPitch = TextureWidth * TexturePixelSize;
			const UINT cellPitch = rowPitch >> cellSize;        // The width of a cell in the checkboard texture.
			const UINT cellHeight = TextureWidth >> cellSize;    // The height of a cell in the checkerboard texture.
			const UINT textureSize = rowPitch * TextureHeight;

			std::vector<UINT8> data(textureSize);
			UINT8* pData = &data[0];

			for (UINT n = 0; n < textureSize; n += TexturePixelSize)
			{
				UINT x = n % rowPitch;
				UINT y = n / rowPitch;
				UINT i = x / cellPitch;
				UINT j = y / cellHeight;

				if (i % 2 == j % 2)
				{
					pData[n] = 0x00;        // R
					pData[n + 1] = 0x00;    // G
					pData[n + 2] = 0x00;    // B
					pData[n + 3] = 0xff;    // A
				}
				else
				{
					pData[n] = 0xff;        // R
					pData[n + 1] = 0xff;    // G
					pData[n + 2] = 0xff;    // B
					pData[n + 3] = 0xff;    // A
				}
			}

			return data;
		}


		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D12;
		}

		bool isRenderSystemSupported(ERenderSystem systemName) override
		{
			if (systemName == ERenderSystem::D3D11 || systemName == ERenderSystem::D3D12)
				return true;

			return false;
		}

		virtual bool setupRenderSystem(ERenderSystem systemName) override
		{
			// Create the pipeline state, which includes compiling and loading shaders.
			{
				char const* shaderPath = "Shader/Test/D3D12Simple";
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mVertexShader, shaderPath, EShader::Vertex, SHADER_ENTRY(MainVS)));
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mPixelShader, shaderPath, EShader::Pixel, SHADER_ENTRY(MainPS)));


				mProgTriangle = ShaderManager::Get().getGlobalShaderT< SimpleProgram >();

				InputLayoutDesc desc;
				desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
				desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::Float4);
				desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
				mInputLayout = RHICreateInputLayout(desc);
			}

			// Create the vertex buffer.
			{
				// Define the geometry for a triangle.
				struct Vertex
				{
					Vector3 position;
					Vector4 color;
					Vector2 uv;
				};

				Vertex vertices[] =
				{
					{ { -0.25f, -0.25f , 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } , { 0.0f, 0.0f } },
					{ {  0.25f, -0.25f , 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } , { 1.0f, 0.0f } },
					{ {  0.25f,  0.25f , 0.0f }, { 0.0f, 1.0f, 1.0f, 1.0f } , { 1.0f, 1.0f } },
					{ { -0.25f,  0.25f , 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } , { 0.0f, 1.0f } }
				};
				mVertexBuffer = RHICreateVertexBuffer(sizeof(Vertex) , ARRAY_SIZE(vertices) , BCF_DefalutValue , vertices);

				uint32 indices[] = { 0, 2 ,1 , 0, 3, 2 };
				mIndexBuffer = RHICreateIndexBuffer(6, true, BCF_DefalutValue, indices);
			}

			{
				std::vector<uint8> texData = GenerateTextureData();
				mTexture = RHICreateTexture2D(ETexture::RGBA8, TextureWidth, TextureHeight, 5, 1, TCF_DefalutValue | TCF_GenerateMips, texData.data());
			}
			{
				std::vector<uint8> texData = GenerateTextureData(4);
				mTexture1 = RHICreateTexture2D(ETexture::RGBA8, TextureWidth, TextureHeight, 5, 1, TCF_DefalutValue | TCF_GenerateMips, texData.data());
			}

			return true;
		}



		void preShutdownRenderSystem(bool bReInit) override
		{
			mTexture.release();
			mTexture1.release();
			mVertexBuffer.release();
			mIndexBuffer.release();
			mInputLayout.release();
			mProgTriangle;
			mVertexShader.releaseRHI();
			mPixelShader.releaseRHI();

			BaseClass::preShutdownRenderSystem(bReInit);
		}


		struct AxisVertex
		{
			Vector3 pos;
			Vector3 color;
		};

		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;

			::Global::GUI().cleanupWidget();


			GButton* button = new GButton(UI_ANY, Vec2i(300, 300), Vec2i(200, 20), nullptr);
			::Global::GUI().addWidget(button);

			auto frame = WidgetUtility::CreateDevFrame();
			frame->addText("aaa");
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

		
			worldTime += time / float(1000);

			Offset = sin(worldTime);
		}

		float Offset;


		void onRender(float dFrame) override
		{
			initializeRenderState();

			auto& commandList = RHICommandList::GetImmediateList();

			IntVector2 screenSize = ::Global::GetScreenSize();

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.0f, 0.2f, 0.4f, 1.0f), 1);

			ViewportInfo viewports[1];
			for( int i = 0 ; i < ARRAY_SIZE(viewports); ++i )
			{
				viewports[i] = ViewportInfo( 0,0,screenSize.x ,screenSize.y );
			}

			RHISetViewports(commandList, viewports, ARRAY_SIZE(viewports));

			if (bUseProgram)
			{
				RHISetShaderProgram(commandList, mProgTriangle->getRHI());
				mProgTriangle->setParam(commandList, SHADER_PARAM(Values), Vector4(Offset, 0, 0, 0));
				auto& samplerState = TStaticSamplerState<ESampler::Trilinear>::GetRHI();
				mProgTriangle->setTexture(commandList, SHADER_PARAM(BaseTexture), *mTexture, SHADER_PARAM(BaseTextureSampler), samplerState);
				mProgTriangle->setTexture(commandList, SHADER_PARAM(BaseTexture1), *mTexture1, SHADER_PARAM(BaseTexture1Sampler), samplerState);
				mView.setupShader(commandList, *mProgTriangle);
			}
			else
			{
				GraphicsShaderStateDesc stateDesc;
				stateDesc.vertex = mVertexShader.getRHI();
				stateDesc.pixel = mPixelShader.getRHI();
				RHISetGraphicsShaderBoundState(commandList, stateDesc);
				mVertexShader.setParam(commandList, SHADER_PARAM(Values), Vector4(Offset, 0, 0, 0));
				mView.setupShader(commandList, mVertexShader);

				auto& samplerState = TStaticSamplerState<ESampler::Trilinear>::GetRHI();
				mPixelShader.setTexture(commandList, SHADER_PARAM(BaseTexture), *mTexture, SHADER_PARAM(BaseTextureSampler), samplerState);
				mPixelShader.setTexture(commandList, SHADER_PARAM(BaseTexture1), *mTexture1, SHADER_PARAM(BaseTexture1Sampler), samplerState);
			}

			InputStreamInfo inputStream;
			inputStream.buffer = mVertexBuffer;
			RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
			RHISetIndexBuffer(commandList, mIndexBuffer);
			RHIDrawIndexedPrimitiveInstanced(commandList, EPrimitive::TriangleList, 0, 6, 4, 0);


			Matrix4 projectMatrix = OrthoMatrix(0, screenSize.x, 0, screenSize.y, -1, 1);
			RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(projectMatrix), LinearColor(1, 0, 0, 1));


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


			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
#if 1
			g.beginRender();

			g.beginBlend(0.5);

			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::White);
			RenderUtility::SetFont(g, FONT_S12);
			g.setTextColor(Color3f(1, 0, 0));
			g.drawTexture(*mTexture, Vector2(0, 0), Vector2(100, 100));
			g.drawTexture(*mTexture1, Vector2(100, 100), Vector2(100, 100));

			g.endBlend();
			//g.drawRect(Vector2(100, 100), Vector2(100, 100));
			g.drawText(10, 10, "AVAVAVAVAVAVAYa");
			g.drawGradientRect(Vector2(300, 0), Color3f(1, 0, 0), Vector2(400, 100), Color3f(0, 1, 0), true);
			g.endRender();
#endif

		}

		bool onKey(KeyMsg const& msg) override
		{
			if(msg.isDown())
			{
				switch( msg.getCode() )
				{
				case EKeyCode::X:
					{

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


	REGISTER_STAGE_ENTRY("D3D12 Test", TestD3D12Stage, EExecGroup::FeatureDev, "Render|RHI");

}