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

		TComPtr<ID3D12RootSignature> m_rootSignature;
		TComPtr<ID3D12PipelineState> m_pipelineState;

		D3D12PooledHeapHandle mTextureSRV;

		RHITexture2DRef mTexture;
		RHITexture2DRef mTexture1;


		RHIVertexBufferRef mVertexBuffer;
		RHIInputLayoutRef  mInputLayout;
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


		bool createPipelineState(GraphicsPipelineState const& pipelineState)
		{
			D3D12PipelineStateStream stateStream;

			auto boundState = static_cast<D3D12System*>(GRHISystem)->getShaderBoundState(pipelineState.shaderBoundState);

			for (auto& shaderInfo : boundState->mShaders)
			{
				switch ( shaderInfo.resource->getType() )
				{
				case EShader::Vertex:
					{
						auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>();
						data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
					}
					break;
				case EShader::Pixel:
					{
						auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>();
						data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
					}
					break;
				case EShader::Geometry:
					{
						auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS>();
						data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
					}
					break;
				case EShader::Hull:
					{
						auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS>();
						data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
					}
					break;
				case EShader::Domain:
					{
						auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS>();
						data = static_cast<D3D12Shader*>(shaderInfo.resource.get())->getByteCode();
					}
					break;
				default:
					{



					}
				}
			}

			if (pipelineState.inputLayout)
			{
				auto& inputLayout = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT>();
				inputLayout = static_cast<D3D12InputLayout*>(pipelineState.inputLayout)->getDesc();
			}

			D3D12_VIEW_INSTANCE_LOCATION viewInstanceLocations[D3D12_MAX_VIEW_INSTANCE_COUNT];
			if (true)
			{
				auto& viewInstancing = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING>();
				for (int i = 0; i < 4; ++i)
				{
					viewInstanceLocations[i].RenderTargetArrayIndex = 0;
					viewInstanceLocations[i].ViewportArrayIndex = i;
				}
				viewInstancing.pViewInstanceLocations = viewInstanceLocations;
				viewInstancing.ViewInstanceCount = ARRAY_SIZE(viewInstanceLocations);
				viewInstancing.Flags = D3D12_VIEW_INSTANCING_FLAG_NONE;
			}

			struct FixedPipelineStateStream
			{
				TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE > pRootSignature;
				TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS > RTFormatArray;
				TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY > PrimitiveTopologyType;
				TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER > RasterizerState;
				TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_RHI > DepthStencilState;
				TPSSubobjectStreamData< D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND > BlendState;
			};

			auto& streamDesc = stateStream.addDataT< FixedPipelineStateStream >();

			streamDesc.pRootSignature = boundState->mRootSignature;

			RHIRasterizerState* rasterizerStateUsage = pipelineState.rasterizerState ? pipelineState.rasterizerState : &TStaticRasterizerState<>::GetRHI();
			streamDesc.RasterizerState = static_cast<D3D12RasterizerState*>(rasterizerStateUsage)->mDesc;
			RHIDepthStencilState* depthStencielStateUsage = pipelineState.depthStencilState ? pipelineState.depthStencilState : &TStaticDepthStencilState<>::GetRHI();
			streamDesc.DepthStencilState = static_cast<D3D12DepthStencilState*>(depthStencielStateUsage)->mDesc;
			RHIBlendState* blendStateUsage = pipelineState.blendState ? pipelineState.blendState : &TStaticBlendState<>::GetRHI();
			streamDesc.BlendState = static_cast<D3D12BlendState*>(blendStateUsage)->mDesc;

			streamDesc.PrimitiveTopologyType = D3D12Translate::ToTopologyType(pipelineState.primitiveType);


			streamDesc.RTFormatArray.RTFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
			streamDesc.RTFormatArray.NumRenderTargets = 1;

			VERIFY_D3D_RESULT_RETURN_FALSE(device->CreatePipelineState(&stateStream.getDesc(), IID_PPV_ARGS(&m_pipelineState)));

			return true;
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

				InputLayoutDesc desc;
				desc.addElement(0, Vertex::ATTRIBUTE_POSITION, Vertex::eFloat3);
				desc.addElement(0, Vertex::ATTRIBUTE_COLOR, Vertex::eFloat4);
				desc.addElement(0, Vertex::ATTRIBUTE_TEXCOORD, Vertex::eFloat2);
				mInputLayout = RHICreateInputLayout(desc);

				GraphicsPipelineState pipelineState;
				pipelineState.shaderBoundState.vertex = mVertexShader.getRHIResource();
				pipelineState.shaderBoundState.pixel = mPixelShader.getRHIResource();
				pipelineState.inputLayout = mInputLayout;
				pipelineState.primitiveType = EPrimitive::TriangleList;



				VERIFY_RETURN_FALSE(createPipelineState(pipelineState));
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

				const UINT vertexBufferSize = sizeof(triangleVertices);

				// Note: using upload heaps to transfer static data like vert buffers is not 
				// recommended. Every time the GPU needs it, the upload heap will be marshalled 
				// over. Please read up on Default Heap usage. An upload heap is used here for 
				// code simplicity and because there are very few verts to actually transfer.

				mVertexBuffer = RHICreateVertexBuffer(sizeof(Vertex) , ARRAY_SIZE(triangleVertices) , BCF_DefalutValue , triangleVertices );
			}
			// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
			// the command list that references it has finished executing on the GPU.
			// We will flush the GPU at the end of this method to ensure the resource is not
			// prematurely destroyed.
			TComPtr<ID3D12Resource> textureUploadHeap;
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			// Create the texture.
			{
				std::vector<uint8> texData = GenerateTextureData();
				mTexture = RHICreateTexture2D(Texture::eRGBA8, TextureWidth, TextureHeight, 1, 1, TCF_DefalutValue, texData.data());
			}
			{
				std::vector<uint8> texData = GenerateTextureData(4);
				mTexture1 = RHICreateTexture2D(Texture::eRGBA8, TextureWidth, TextureHeight, 1, 1, TCF_DefalutValue, texData.data());
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
			// However, when ExecuteCommandList() is called on a particular command 
			// list, that command list can then be reset at any time and must be before 
			// re-recording.


			int32 frameIndex = renderContext->mFrameIndex;
			{
				// Indicate that the back buffer will be used as a render target.
				D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier( 
					static_cast<D3D12System*>(GRHISystem)->mSwapChain->mTextures[frameIndex],
					D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
				graphicsCmdList->ResourceBarrier(1, &barrier);
			}

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = static_cast<D3D12System*>(GRHISystem)->mSwapChain->mRTVHandles[frameIndex].getCPUHandle();
			graphicsCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

			// Record commands.
			const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
			graphicsCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

			ViewportInfo viewports[4];
			for( int i = 0 ; i < 4; ++i )
			{
				viewports[i] = ViewportInfo(
					float(i % 2) * screenSize.x / 2,
					float(i / 2) * screenSize.y / 2,
					screenSize.x / 2,
					screenSize.y / 2);
			}

			RHISetViewports(commandList, viewports, ARRAY_SIZE(viewports));
			//graphicsCmdList->SetViewInstanceMask(BIT(4) - 1);
			// Set necessary state.
			graphicsCmdList->SetPipelineState(m_pipelineState);

			GraphicsShaderBoundState boundState;
			boundState.vertex = mVertexShader.getRHIResource();
			boundState.pixel = mPixelShader.getRHIResource();
			RHISetGraphicsShaderBoundState(commandList, boundState);

			mVertexShader.setParam(commandList, SHADER_PARAM(Values), Vector4(Offset, 0, 0, 0));
	
			mPixelShader.setTexture(commandList, SHADER_PARAM(BaseTexture), *mTexture, SHADER_PARAM(BaseTextureSampler), TStaticSamplerState<>::GetRHI());
			mPixelShader.setTexture(commandList, SHADER_PARAM(BaseTexture1), *mTexture1, SHADER_PARAM(BaseTexture1Sampler), TStaticSamplerState<>::GetRHI());
	

			InputStreamInfo inputStream;
			inputStream.buffer = mVertexBuffer;
			RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
			RHIDrawPrimitiveInstanced(commandList, EPrimitive::TriangleList, 0, 3, 4, 0);

			// Indicate that the back buffer will now be used to present.

			{
				D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(
					static_cast<D3D12System*>(GRHISystem)->mSwapChain->mTextures[frameIndex],
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
				graphicsCmdList->ResourceBarrier(1, &barrier);
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