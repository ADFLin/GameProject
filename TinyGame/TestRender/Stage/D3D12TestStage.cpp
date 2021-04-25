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


	class TriangleProgram : public GlobalShaderProgram
	{
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(TriangleProgram, Global);
	public:

		static char const* GetShaderFileName()
		{
			return "Shader/Test/TriangleTest";
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

	IMPLEMENT_SHADER_PROGRAM(TriangleProgram);

	class TestD3D12Stage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		TestD3D12Stage() {}

		D3D12System* mD3D12System;

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D12;
		}

		ID3D12DeviceRHI* device;
		D3D12Context* renderContext;

		RHITexture2DRef mTexture;
		RHITexture2DRef mTexture1;

		RHIVertexBufferRef mVertexBuffer;
		RHIInputLayoutRef  mInputLayout;

		bool bUseProgram = true;

		TriangleProgram* mProgTriangle;
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


		virtual bool setupRenderSystem(ERenderSystem systemName) override
		{
			if (systemName != ERenderSystem::D3D12)
				return false;
			device = static_cast<D3D12System*>(GRHISystem)->mDevice;
			renderContext = &static_cast<D3D12System*>(GRHISystem)->mRenderContext;

			ID3D12GraphicsCommandListRHI* graphicsCmdList = renderContext->mGraphicsCmdList;

			// Create the pipeline state, which includes compiling and loading shaders.
			{
				char const* shaderPath = "Shader/Test/TriangleTest";
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mVertexShader, shaderPath, EShader::Vertex, SHADER_ENTRY(MainVS)));
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mPixelShader, shaderPath, EShader::Pixel, SHADER_ENTRY(MainPS)));


				mProgTriangle = ShaderManager::Get().getGlobalShaderT< TriangleProgram >();

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

				Vertex triangleVertices[] =
				{
					{ { 0.0f, 0.25f , 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } , { 0.5f, 0.0f} },
					{ { 0.25f, -0.25f , 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } , {1.0f, 1.0f}},
					{ { -0.25f, -0.25f , 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } , { 0.0f, 1.0f } }
				};
				mVertexBuffer = RHICreateVertexBuffer(sizeof(Vertex) , ARRAY_SIZE(triangleVertices) , BCF_DefalutValue , triangleVertices );
			}

			{
				std::vector<uint8> texData = GenerateTextureData();
				mTexture = RHICreateTexture2D(ETexture::RGBA8, TextureWidth, TextureHeight, 1, 1, TCF_DefalutValue, texData.data());
			}
			{
				std::vector<uint8> texData = GenerateTextureData(4);
				mTexture1 = RHICreateTexture2D(ETexture::RGBA8, TextureWidth, TextureHeight, 1, 1, TCF_DefalutValue, texData.data());
			}
			return true;
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

		
			worldTime += time / float(1000);

			Offset = sin(worldTime);
		}

		float Offset;


		void onRender(float dFrame) override
		{

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			ID3D12GraphicsCommandListRHI* graphicsCmdList = renderContext->mGraphicsCmdList;

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
				RHISetShaderProgram(commandList, mProgTriangle->getRHIResource());
				mProgTriangle->setParam(commandList, SHADER_PARAM(Values), Vector4(Offset, 0, 0, 0));
				mProgTriangle->setTexture(commandList, SHADER_PARAM(BaseTexture), *mTexture, SHADER_PARAM(BaseTextureSampler), TStaticSamplerState<>::GetRHI());
				mProgTriangle->setTexture(commandList, SHADER_PARAM(BaseTexture1), *mTexture1, SHADER_PARAM(BaseTexture1Sampler), TStaticSamplerState<>::GetRHI());
			}
			else
			{
				GraphicsShaderStateDesc stateDesc;
				stateDesc.vertex = mVertexShader.getRHIResource();
				stateDesc.pixel = mPixelShader.getRHIResource();
				RHISetGraphicsShaderBoundState(commandList, stateDesc);
				mVertexShader.setParam(commandList, SHADER_PARAM(Values), Vector4(Offset, 0, 0, 0));
				mPixelShader.setTexture(commandList, SHADER_PARAM(BaseTexture), *mTexture, SHADER_PARAM(BaseTextureSampler), TStaticSamplerState<>::GetRHI());
				mPixelShader.setTexture(commandList, SHADER_PARAM(BaseTexture1), *mTexture1, SHADER_PARAM(BaseTexture1Sampler), TStaticSamplerState<>::GetRHI());
			}

			InputStreamInfo inputStream;
			inputStream.buffer = mVertexBuffer;
			RHISetInputStream(commandList, mInputLayout, &inputStream, 1);

			RHIDrawPrimitiveInstanced(commandList, EPrimitive::TriangleList, 0, 3, 4, 0);


			Matrix4 projectMatrix = OrthoMatrix(0, screenSize.x, 0, screenSize.y, -1, 1);
			RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(projectMatrix), LinearColor(1, 0, 0, 1));
			{

				Vector2 v[] = { Vector2(1,1) , Vector2(100,100), Vector2(1,100), Vector2(100,1) };
				uint32 indices[] = { 0, 2, 2, 1,1,3,3,0 };
				TRenderRT< RTVF_XY >::DrawIndexed(commandList, EPrimitive::LineList, v, ARRAY_SIZE(v), indices, ARRAY_SIZE(indices));
			}
			RHIFlushCommand(commandList);
			{

				Vector2 v[] = { Vector2(100,100) , Vector2(200,200), Vector2(100,200), Vector2(200,100) };
				uint32 indices[] = { 0, 2, 2, 1,1,3,3,0 };
				TRenderRT< RTVF_XY >::DrawIndexed(commandList, EPrimitive::LineList, v, ARRAY_SIZE(v), indices, ARRAY_SIZE(indices));
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


	REGISTER_STAGE("D3D12 Test", TestD3D12Stage, EStageGroup::FeatureDev);

}