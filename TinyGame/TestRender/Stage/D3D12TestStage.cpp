#include "TestRenderStageBase.h"

#include "RHI/RHIDefine.h"
#include "RHI/RHIType.h"
#include "RHI/RHICommon.h"

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

		TComPtr<ID3D12Resource> m_texture;
		TComPtr<ID3D12DescriptorHeap> m_srvHeap;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

		RHIVertexBufferRef mVertexBuffer;
		RHIInputLayoutRef  mInputLayout;
		Shader mVertexShader;
		Shader mPixelShader;

		static const UINT TextureWidth = 256;
		static const UINT TextureHeight = 256;
		static const UINT TexturePixelSize = 4;

		std::vector<UINT8> GenerateTextureData()
		{
			const UINT rowPitch = TextureWidth * TexturePixelSize;
			const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
			const UINT cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
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
		static void MemcpySubresource(
			const D3D12_MEMCPY_DEST* pDest,
			const D3D12_SUBRESOURCE_DATA* pSrc,
			SIZE_T RowSizeInBytes,
			UINT NumRows,
			UINT NumSlices) noexcept
		{
			for (UINT z = 0; z < NumSlices; ++z)
			{
				auto pDestSlice = static_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
				auto pSrcSlice = static_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
				for (UINT y = 0; y < NumRows; ++y)
				{
					memcpy(pDestSlice + pDest->RowPitch * y,
						pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
						RowSizeInBytes);
				}
			}
		}

		inline UINT64 UpdateSubresources(
			ID3D12GraphicsCommandList* pCmdList,
			ID3D12Resource* pDestinationResource,
			ID3D12Resource* pIntermediate,
			UINT FirstSubresource,
			UINT NumSubresources,
			UINT64 RequiredSize,
			const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			const UINT* pNumRows,
			const UINT64* pRowSizesInBytes,
			const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
		{
			// Minor validation
			auto IntermediateDesc = pIntermediate->GetDesc();
			auto DestinationDesc = pDestinationResource->GetDesc();
			if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
				IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
				RequiredSize > SIZE_T(-1) ||
				(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
				(FirstSubresource != 0 || NumSubresources != 1)))
			{
				return 0;
			}

			BYTE* pData;
			HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
			if (FAILED(hr))
			{
				return 0;
			}

			for (UINT i = 0; i < NumSubresources; ++i)
			{
				if (pRowSizesInBytes[i] > SIZE_T(-1)) return 0;
				D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
				MemcpySubresource(&DestData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth);
			}
			pIntermediate->Unmap(0, nullptr);

			if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			{
				pCmdList->CopyBufferRegion(
					pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
			}
			else
			{
				for (UINT i = 0; i < NumSubresources; ++i)
				{
					D3D12_TEXTURE_COPY_LOCATION Dst;
					Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
					Dst.pResource = pDestinationResource;
					Dst.PlacedFootprint = {};
					Dst.SubresourceIndex = i + FirstSubresource;
					D3D12_TEXTURE_COPY_LOCATION Src;
					Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
					Src.pResource = pIntermediate;
					Src.PlacedFootprint = pLayouts[i];
					pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
				}
			}
			return RequiredSize;
		}

		inline UINT64 UpdateSubresources(
			ID3D12GraphicsCommandList* pCmdList,
			ID3D12Resource* pDestinationResource,
			ID3D12Resource* pIntermediate,
			UINT64 IntermediateOffset,
			UINT FirstSubresource,
			UINT NumSubresources,
			const D3D12_SUBRESOURCE_DATA* pSrcData)
		{
			UINT64 RequiredSize = 0;
			UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
			if (MemToAlloc > SIZE_MAX)
			{
				return 0;
			}
			void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
			if (pMem == nullptr)
			{
				return 0;
			}
			auto pLayouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
			UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
			UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

			auto Desc = pDestinationResource->GetDesc();
			ID3D12Device* pDevice = nullptr;
			pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void**>(&pDevice));
			pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
			pDevice->Release();

			UINT64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
			HeapFree(GetProcessHeap(), 0, pMem);
			return Result;
		}


		bool createPipelineState(GraphicsPipelineState const& pipelineState)
		{
			D3D12PipelineStateStream stateStream;

			std::vector< D3D12_STATIC_SAMPLER_DESC > rootSamplers;
			std::vector< D3D12_ROOT_PARAMETER1 > rootParameters;

			D3D12_ROOT_SIGNATURE_FLAGS denyShaderRootAccessFlags = 
				D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			if (pipelineState.shaderProgram)
			{
				auto shaderProgramImpl = static_cast<D3D12ShaderProgram*>(pipelineState.shaderProgram);
				for (int i = 0; i < shaderProgramImpl->mNumShaders; ++i)
				{
					switch (shaderProgramImpl->mShaders[i].type)
					{
					case EShader::Vertex:
						{
							auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>();
							data = shaderProgramImpl->getByteCode(i);
							denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
						}
						break;
					case EShader::Pixel:
						{
							auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>();
							data = shaderProgramImpl->getByteCode(i);
							denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
						}
						break;
					case EShader::Geometry:
						{
							auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS>();
							data = shaderProgramImpl->getByteCode(i);
							denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
						}
						break;
					case EShader::Hull:
						{
							auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS>();
							data = shaderProgramImpl->getByteCode(i);
							denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
						}
						break;
					case EShader::Domain:
						{
							auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS>();
							data = shaderProgramImpl->getByteCode(i);
							denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
						}
						break;
					}
				}
			}
			else
			{
				auto const& shaderBoundState = pipelineState.shaderBoundState;

				auto AddRootParamater = [&](D3D12Shader* shader) -> bool
				{
					bool result = false;
					D3D12_ROOT_PARAMETER1 parameter = shader->getRootParameter();
					if (parameter.DescriptorTable.NumDescriptorRanges)
					{
						rootParameters.push_back(parameter);
						result = true;
					}
					rootSamplers.insert(rootSamplers.end() , shader->mRootSignature.samplers.begin() , shader->mRootSignature.samplers.end());
					return result;
				};

				if (shaderBoundState.vertex)
				{
					auto shaderImpl = static_cast<D3D12Shader*>(shaderBoundState.vertex);
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>();
					data = shaderImpl->getByteCode();
					if ( AddRootParamater(shaderImpl) )
						denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
				}
				if (shaderBoundState.pixel)
				{
					auto shaderImpl = static_cast<D3D12Shader*>(shaderBoundState.pixel);
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>();
					data = shaderImpl->getByteCode();
					if (AddRootParamater(shaderImpl))
						denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
				}
				if (shaderBoundState.geometry)
				{
					auto shaderImpl = static_cast<D3D12Shader*>(shaderBoundState.geometry);
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS>();
					data = shaderImpl->getByteCode();
					if (AddRootParamater(shaderImpl))
						denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
				}
				if (shaderBoundState.hull)
				{
					auto shaderImpl = static_cast<D3D12Shader*>(shaderBoundState.hull);
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS>();
					data = shaderImpl->getByteCode();
					if (AddRootParamater(shaderImpl))
						denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
				}
				if (shaderBoundState.domain)
				{
					auto shaderImpl = static_cast<D3D12Shader*>(shaderBoundState.domain);
					auto& data = stateStream.addDataT<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS>();
					data = shaderImpl->getByteCode();
					if (AddRootParamater(shaderImpl))
						denyShaderRootAccessFlags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
				}
			}

			{
				D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
				rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
				rootSignatureDesc.Desc_1_1.NumParameters = rootParameters.size();
				rootSignatureDesc.Desc_1_1.pParameters = rootParameters.data();
				rootSignatureDesc.Desc_1_1.NumStaticSamplers = rootSamplers.size();
				rootSignatureDesc.Desc_1_1.pStaticSamplers = rootSamplers.data();
				rootSignatureDesc.Desc_1_1.Flags = denyShaderRootAccessFlags | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

				TComPtr<ID3DBlob> signature;
				TComPtr<ID3DBlob> error;
				if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error)))
				{
					LogWarning(0, (char const*)error->GetBufferPointer());
				}
				VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
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

			RHIRasterizerState* rasterizerStateUsage = pipelineState.rasterizerState ? pipelineState.rasterizerState : &TStaticRasterizerState<>::GetRHI();
			streamDesc.RasterizerState = static_cast<D3D12RasterizerState*>(rasterizerStateUsage)->mDesc;
			RHIDepthStencilState* depthStencielStateUsage = pipelineState.depthStencilState ? pipelineState.depthStencilState : &TStaticDepthStencilState<>::GetRHI();
			streamDesc.DepthStencilState = static_cast<D3D12DepthStencilState*>(depthStencielStateUsage)->mDesc;
			RHIBlendState* blendStateUsage = pipelineState.blendState ? pipelineState.blendState : &TStaticBlendState<>::GetRHI();
			streamDesc.BlendState = static_cast<D3D12BlendState*>(blendStateUsage)->mDesc;

			streamDesc.PrimitiveTopologyType = D3D12Translate::ToTopologyType(pipelineState.primitiveType);
			streamDesc.pRootSignature = m_rootSignature;

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


				// Initialize the vertex buffer view.
				m_vertexBufferView.BufferLocation = static_cast< D3D12VertexBuffer* >( mVertexBuffer.get() )->getResource()->GetGPUVirtualAddress();
				m_vertexBufferView.StrideInBytes = sizeof(Vertex);
				m_vertexBufferView.SizeInBytes = vertexBufferSize;
			}
			{

				D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
				srvHeapDesc.NumDescriptors = 1;
				srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

			}

			// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
			// the command list that references it has finished executing on the GPU.
			// We will flush the GPU at the end of this method to ensure the resource is not
			// prematurely destroyed.
			TComPtr<ID3D12Resource> textureUploadHeap;
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			// Create the texture.
			{

				// Describe and create a Texture2D.
				D3D12_RESOURCE_DESC textureDesc = {};
				textureDesc.MipLevels = 1;
				textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				textureDesc.Width = TextureWidth;
				textureDesc.Height = TextureHeight;
				textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				textureDesc.DepthOrArraySize = 1;
				textureDesc.SampleDesc.Count = 1;
				textureDesc.SampleDesc.Quality = 0;
				textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

				VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateCommittedResource(
					&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&textureDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS(&m_texture)));

				auto GetRequiredIntermediateSize = [=]( ID3D12Resource*pDestinationResource , uint32 firstSubresource , uint32 numSubresources )-> uint64
				{
					auto Desc = pDestinationResource->GetDesc();
					uint64 RequiredSize = 0;
					device->GetCopyableFootprints(&Desc, firstSubresource, numSubresources, 0, nullptr, nullptr, nullptr, &RequiredSize);
					return RequiredSize;
				};

				const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture, 0, 1);

				// Create the GPU upload buffer.
				VERIFY_D3D_RESULT_RETURN_FALSE(device->CreateCommittedResource(
					&FD3D12Init::HeapProperrties(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&FD3D12Init::BufferDesc(uploadBufferSize),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&textureUploadHeap)));

				// Copy data to the intermediate upload heap and then schedule a copy 
				// from the upload heap to the Texture2D.
				std::vector<UINT8> texture = GenerateTextureData();

				D3D12_SUBRESOURCE_DATA textureData = {};
				textureData.pData = &texture[0];
				textureData.RowPitch = TextureWidth * TexturePixelSize;
				textureData.SlicePitch = textureData.RowPitch * TextureHeight;

				UpdateSubresources(graphicsCmdList, m_texture , textureUploadHeap , 0, 0, 1, &textureData);
				graphicsCmdList->ResourceBarrier(1, &FD3D12Init::TransitionBarrier(m_texture , D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

				// Describe and create a SRV for the texture.
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Format = textureDesc.Format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				device->CreateShaderResourceView(m_texture, &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
			}

			RHIFlushCommand(commandList);

			renderContext->waitForGpu();
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

		}

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
					static_cast<D3D12System*>(GRHISystem)->mSwapChain->mRTViews[frameIndex],
					D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
				graphicsCmdList->ResourceBarrier(1, &barrier);
			}

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
			UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			rtvHandle.ptr = static_cast<D3D12System*>(GRHISystem)->mSwapChain->mRTVHeap->GetCPUDescriptorHandleForHeapStart().ptr + frameIndex * static_cast<D3D12System*>(GRHISystem)->mSwapChain->mRTVHeap * rtvDescriptorSize;
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

			graphicsCmdList->SetGraphicsRootSignature(m_rootSignature);
			ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap };
			graphicsCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			graphicsCmdList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

			
			InputStreamInfo inputStream;
			inputStream.buffer = mVertexBuffer;
			RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
			RHIDrawPrimitiveInstanced(commandList, EPrimitive::TriangleList, 0, 3, 2, 0);

			// Indicate that the back buffer will now be used to present.

			{
				D3D12_RESOURCE_BARRIER barrier = FD3D12Init::TransitionBarrier(
					static_cast<D3D12System*>(GRHISystem)->mSwapChain->mRTViews[frameIndex],
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