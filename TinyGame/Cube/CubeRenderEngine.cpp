#include "CubePCH.h"
#include "CubeRenderEngine.h"

#include "CubeBlockRender.h"

#include "CubeWorld.h"

#include "WindowsHeader.h"
#include "OpenGLHeader.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/SimpleRenderState.h"

#include "Async/AsyncWork.h"
#include "RenderUtility.h"
#include "RHI/RHICommand.h"
#include "Math/GeometryPrimitive.h"


#include "GameGlobal.h"
#include "Renderer/SceneView.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIUtility.h"
#include "RenderDebug.h"

namespace Cube
{
	using namespace Render;

	class BlockRenderShaderProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(BlockRenderShaderProgram, Global);

		SHADER_PERMUTATION_TYPE_BOOL(DepthPass, SHADER_PARAM(DEPTH_PASS));
		SHADER_PERMUTATION_TYPE_BOOL(OverdrawPass, SHADER_PARAM(OVERDRAW_PASS));
		using PermutationDomain = TShaderPermutationDomain<DepthPass, OverdrawPass>;

		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
			if (domain.get< DepthPass >())
			{
				static ShaderEntryInfo const entries[] =
				{
					{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				};
				return entries;
			}

			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/CubeBlockRender";
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_TEXTURE_PARAM(parameterMap, BlockTexture);
		}

		DEFINE_TEXTURE_PARAM(BlockTexture);

		//DEFINE_SHADER_PARAM(VertexTransform);
	};


	IMPLEMENT_SHADER_PROGRAM(BlockRenderShaderProgram);

	class HZBGenerateCS : public GlobalShader
	{
		DECLARE_SHADER(HZBGenerateCS, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/HZB";
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_TEXTURE_PARAM(parameterMap, SourceTexture);
			BIND_SHADER_PARAM(parameterMap, DestTexture);
			BIND_SHADER_PARAM(parameterMap, SourceTextureSize);
			BIND_SHADER_PARAM(parameterMap, DestTextureSize);
			BIND_SHADER_PARAM(parameterMap, SourceMipLevel);
			BIND_SHADER_PARAM(parameterMap, DestMipLevel);
		}

		DEFINE_TEXTURE_PARAM(SourceTexture);
		DEFINE_SHADER_PARAM(DestTexture);
		DEFINE_SHADER_PARAM(SourceTextureSize);
		DEFINE_SHADER_PARAM(DestTextureSize);
		DEFINE_SHADER_PARAM(SourceMipLevel);
		DEFINE_SHADER_PARAM(DestMipLevel);
	};

	IMPLEMENT_SHADER(HZBGenerateCS, EShader::Compute, SHADER_ENTRY(GenerateHZBCS));

	class HZBCullCS : public GlobalShader
	{
		DECLARE_SHADER(HZBCullCS, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/HZB";
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_SHADER_PARAM(parameterMap, CullItems);
			BIND_TEXTURE_PARAM(parameterMap, HZBTexture);
			BIND_SHADER_PARAM(parameterMap, ResultTexture);
			BIND_SHADER_PARAM(parameterMap, VisibleCommands);
			BIND_SHADER_PARAM(parameterMap, WorldToClip);
			BIND_SHADER_PARAM(parameterMap, ViewSize);
			BIND_SHADER_PARAM(parameterMap, ResultTextureSize);
			BIND_SHADER_PARAM(parameterMap, NumMipLevel);
			BIND_SHADER_PARAM(parameterMap, NumItems);
			BIND_SHADER_PARAM(parameterMap, DepthBias);
		}

		DEFINE_SHADER_PARAM(CullItems);
		DEFINE_TEXTURE_PARAM(HZBTexture);
		DEFINE_SHADER_PARAM(ResultTexture);
		DEFINE_SHADER_PARAM(VisibleCommands);
		DEFINE_SHADER_PARAM(WorldToClip);
		DEFINE_SHADER_PARAM(ViewSize);
		DEFINE_SHADER_PARAM(ResultTextureSize);
		DEFINE_SHADER_PARAM(NumMipLevel);
		DEFINE_SHADER_PARAM(NumItems);
		DEFINE_SHADER_PARAM(DepthBias);
	};

	IMPLEMENT_SHADER(HZBCullCS, EShader::Compute, SHADER_ENTRY(HZBCullCSMain));

	static int constexpr MaxHZBCullItems = 4096;
	static int constexpr HZBCullResultTextureWidth = 4096;

	struct HZBCullGPUItem
	{
		Vector3 boxMin;
		float padding0;
		Vector3 boxMax;
		float padding1;
		uint32 outputIndex;
		uint32 indexCountPerInstance;
		uint32 startIndexLocation;
		int32  baseVertexLocation;
		uint32 instanceCount;
		uint32 startInstanceLocation;
		uint32 padding2;
		uint32 padding3;
	};

	bool GRenderPreDepth = true;

	RenderEngine::RenderEngine(int w, int h)
	{
		mClientWorld = NULL;
		mAspect = float(w) / h;
		mViewSize = Vec2i(w, h);


		mRenderWidth = 0;
		mRenderHeight = ChunkBlockMaxHeight;
		mRenderDepth = 0;

		mGereatePool = new QueueThreadPool;
		mGereatePool->init(4);
	}

	RenderEngine::~RenderEngine()
	{
		cleanupRenderData();
		delete mGereatePool;
	}

	bool RenderEngine::initializeRHI()
	{
		ShaderHelper::Get().init();

		mDebugPrimitives.initializeRHI();
		mProgBlockRender = ShaderManager::Get().getGlobalShaderT< BlockRenderShaderProgram >();

		BlockRenderShaderProgram::PermutationDomain permutationVector;
		permutationVector.set< BlockRenderShaderProgram::DepthPass >(true);
		mProgBlockRenderDepth = ShaderManager::Get().getGlobalShaderT< BlockRenderShaderProgram >(permutationVector);
		permutationVector = BlockRenderShaderProgram::PermutationDomain();
		permutationVector.set< BlockRenderShaderProgram::OverdrawPass >(true);
		mProgBlockRenderOverdraw = ShaderManager::Get().getGlobalShaderT< BlockRenderShaderProgram >(permutationVector);
		mProgHZBGenerate = ShaderManager::Get().getGlobalShaderT< HZBGenerateCS >();
		ShaderManager::Get().getGlobalShaderT< HZBCullCS >();

		mTexBlockAtlas = RHIUtility::LoadTexture2DFromFile("Cube/blocks.png", TextureLoadOption().AutoMipMap());

		InputLayoutDesc desc;
		desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::UByte4, true);
		desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD2, EVertex::UInt1);
		desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		mBlockInputLayout = RHICreateInputLayout(desc);

		mCmdBuildBuffer = RHICreateBuffer(sizeof(uint32), 4096 * 16 * 5, BCF_CreateSRV | BCF_CreateUAV);
		mCmdBuffer = RHICreateBuffer(sizeof(DrawCmdArgs), 4096 * 16, BCF_DrawIndirectArgs);
		mHZBCullItemBuffer = RHICreateBuffer(sizeof(HZBCullGPUItem), MaxHZBCullItems, BCF_Structured | BCF_CreateSRV | BCF_CpuAccessWrite);

		mSceneFrameBuffer = RHICreateFrameBuffer();
		mOccluderFrameBuffer = RHICreateFrameBuffer();
		resizeRenderTarget();

		return true;
	}

	void RenderEngine::releaseRHI()
	{
		mDebugPrimitives.releaseRHI();
		mProgBlockRender = nullptr;
		mProgBlockRenderDepth = nullptr;
		mProgBlockRenderOverdraw = nullptr;
		mProgHZBGenerate = nullptr;
		mTexBlockAtlas.release();
		mBlockInputLayout.release();
		mCmdBuildBuffer.release();
		mCmdBuffer.release();
		mHZBCullItemBuffer.release();
		mSceneFrameBuffer.release();
		mOccluderFrameBuffer.release();
		mHZBTexture.release();
		mHZBScratchTexture.release();
		mMipTestTexture.release();
		mSceneTexture.release();
		mSceneDepthTexture.release();
		mOccluderColorTexture.release();
		mOccluderDepthTexture.release();
		mHZBCullResultTexture.release();
	}

	void RenderEngine::setupWorld(World& world)
	{
		if (mClientWorld)
		{
			mClientWorld->removeListener(*this);
			mClientWorld->mChunkProvider->mListener = nullptr;
		}

		mClientWorld = &world;
		mClientWorld->mChunkProvider->mListener = this;
		mClientWorld->addListener(*this);
	}


	void RenderEngine::tick(float deltaTime)
	{
		for (auto const& pair : mChunkMap)
		{
			ChunkRenderData* data = pair.second;
			if (data == nullptr || !data->bNeedUpdate)
				continue;
			if (data->state == ChunkRenderData::eMeshGenerating)
				continue;

			resetChunkRenderData(data);
			updateRenderData(data);
		}

		if (!mPendingAddList.empty())
		{
			mMutexPedingAdd.lock();
			TArray<UpdatedRenderData> localAddList(std::move(mPendingAddList));
			mMutexPedingAdd.unlock();

			for (auto const& updateData : localAddList)
			{
				auto data = updateData.chunkData;
				auto& mesh = updateData.mesh;
				using namespace Render;

				data->bound.invalidate();
				data->numLayer = updateData.numLayer;
				data->bNeedUpdate = false;

				for (int i = 0; i < data->numLayer; ++i)
				{
					auto const& updateLayer = updateData.layers[i];
					auto& layer = data->layers[i];

					auto meshData = acquireMeshRenderData(updateLayer.vertexCount, updateLayer.indexCount);
					if (meshData == nullptr)
						continue;

					layer.meshPool = meshData;
					layer.bound = updateLayer.bound;
					layer.occluderBox = updateLayer.occluderBox;

					meshData->vertexAllocator.alloc(updateLayer.vertexCount, 0, layer.vertexAllocation);
					meshData->indexAllocator.alloc(updateLayer.indexCount, 0, layer.indexAlloction);
					RHIUpdateBuffer(*meshData->vertexBuffer, layer.vertexAllocation.pos, updateLayer.vertexCount, (void*)(mesh.mVertices.data() + updateLayer.vertexOffset));
					RHIUpdateBuffer(*meshData->indexBuffer, layer.indexAlloction.pos, updateLayer.indexCount, (void*)(mesh.mIndices.data() + updateLayer.indexOffset));

					layer.args.baseVertexLocation = int(layer.vertexAllocation.pos) - int(updateLayer.vertexOffset);
					layer.args.startIndexLocation = layer.indexAlloction.pos;
					layer.args.indexCountPerInstance = updateLayer.indexCount;
					layer.args.instanceCount = 1;
					layer.args.startInstanceLocation = 0;

					data->bound += layer.bound;
				}

				//data->posOffset = ChunkSize * Vec3f(data->chunk->getPos() , 0.0);
				//data->bound.translate(data->posOffset);
				data->state = ChunkRenderData::eMesh;
			}
		}
	}

	void RenderEngine::resetChunkRenderData(ChunkRenderData* data)
	{
		if (data == nullptr)
			return;

		for (int i = 0; i < data->numLayer; ++i)
		{
			auto& layer = data->layers[i];
			if (layer.meshPool)
			{
				layer.meshPool->vertexAllocator.deallocate(layer.vertexAllocation);
				layer.meshPool->indexAllocator.deallocate(layer.indexAlloction);
			}

			layer = ChunkRenderData::Layer();
		}

		data->numLayer = 0;
		data->bound.invalidate();
		data->state = ChunkRenderData::eNone;
	}

	void RenderEngine::onChunkAdded(Chunk* chunk)
	{
		for (int i = 0; i < 4; ++i)
		{
			Vec3i offset = GetFaceOffset(FaceSide(i));
			ChunkPos chunkPos = ChunkPos{ chunk->getPos() + Vec2i(offset.x, offset.y) };
			ChunkDataMap::iterator iter = mChunkMap.find(chunkPos.hash_value());
			if (iter != mChunkMap.end())
			{
				ChunkRenderData* data = iter->second;
				updateRenderData(data);
			}
		}

	}

	void RenderEngine::updateRenderData(ChunkRenderData* data)
	{
		if (data->state != ChunkRenderData::eNone)
			return;

		NeighborChunkAccess chunkAccess(*mClientWorld, data->chunk);
		for (int i = 0; i < 4; ++i)
		{
			if (chunkAccess.mNeighborChunks[i] == nullptr)
				return;
		}

		mGereatePool->addFunctionWork(
			[this, data, chunkAccess]()
		{
			PROFILE_ENTRY("Chunk Mesh Generate");

			Chunk* chunk = data->chunk;
			data->state = ChunkRenderData::eMeshGenerating;

			Mesh mesh;
			BlockRenderer renderer;

			int const ColorMap[] =
			{
				EColor::Red,
				EColor::Green,
				EColor::Blue,
				EColor::Cyan,
				EColor::Magenta,
				EColor::Yellow,

				EColor::Orange,
				EColor::Purple,
				EColor::Pink,
				EColor::Brown,
				EColor::Gold,
			};

			UpdatedRenderData updateData;
			updateData.chunkData = data;
			updateData.numLayer = 0;
			renderer.mMesh = &updateData.mesh;

			renderer.mDebugColor = RenderUtility::GetColor(ColorMap[(chunk->getPos().x + chunk->getPos().y) % ARRAY_SIZE(ColorMap)]);

			PaddedBlockAccess paddedAccess;
			renderer.mBlockAccess = &paddedAccess;

			for (int indexLayer = 0; indexLayer < ChunkRenderData::MaxLayerCount; ++indexLayer)
			{
				auto layer = chunk->mLayer[indexLayer];
				if (!layer)
					continue;

				paddedAccess.fill(chunkAccess, chunk, indexLayer);

				int indexStart = updateData.mesh.mIndices.size();
				int vertexStart = updateData.mesh.mVertices.size();

				int zOff = indexLayer * Chunk::LayerSize;
				renderer.setBasePos(Vec3i(chunk->getPos().x * ChunkSize, chunk->getPos().y * ChunkSize, zOff));
				renderer.drawLayer(*chunk, indexLayer);

				{
					TIME_SCOPE(mMergeTimeAcc);
					renderer.finalizeMesh();
				}

				if (indexStart != updateData.mesh.mIndices.size())
				{
					auto& layerData = updateData.layers[updateData.numLayer];
					updateData.numLayer += 1;

					layerData.bound = renderer.bound;
					layerData.index = indexLayer;
					layerData.occluderBox = renderer.mOccluderBox;
					layerData.vertexOffset = vertexStart;
					layerData.vertexCount = (uint32)updateData.mesh.mVertices.size() - layerData.vertexOffset;
					layerData.indexOffset = indexStart;
					layerData.indexCount = (uint32)updateData.mesh.mIndices.size() - layerData.indexOffset;
				}

				renderer.bound.invalidate();
			}

			{
				Mutex::Locker locker(mMutexPedingAdd);
				mPendingAddList.push_back(std::move(updateData));
			}
		}
		);
	}

	void RenderEngine::markChunkDirty(ChunkPos const& chunkPos)
	{
		ChunkDataMap::iterator iter = mChunkMap.find(chunkPos.hash_value());
		if (iter == mChunkMap.end())
			return;

		ChunkRenderData* data = iter->second;
		if (data == nullptr)
			return;

		data->bNeedUpdate = true;
	}

	void RenderEngine::resizeRenderTarget()
	{
		static constexpr Color4ub MipTestColors[] =
		{
			Color4ub(255, 255, 255, 255),
			Color4ub(255, 0, 0, 255),
			Color4ub(0, 255, 0, 255),
			Color4ub(0, 0, 255, 255),
		};

		int numSamples = 1;
		mSceneTexture = RHICreateTexture2D(ETexture::FloatRGBA, mViewSize.x, mViewSize.y, 1, numSamples, TCF_DefalutValue | TCF_RenderTarget);
		mSceneTexture->setDebugName("SceneTexture");
		mSceneDepthTexture = RHICreateTexture2D(ETexture::D32FS8, mViewSize.x, mViewSize.y, 1, numSamples, TCF_CreateSRV);
		int const occluderWidth = mViewSize.x / 2;
		int const occluderHeight = mViewSize.y / 2;
		mOccluderColorTexture = RHICreateTexture2D(ETexture::RGBA8, occluderWidth, occluderHeight, 1, numSamples, TCF_DefalutValue | TCF_RenderTarget);
		mOccluderDepthTexture = RHICreateTexture2D(ETexture::D32FS8, occluderWidth, occluderHeight, 1, numSamples, TCF_CreateSRV);
		mSceneFrameBuffer->setTexture(0, *mSceneTexture);
		mSceneFrameBuffer->setDepth(*mSceneDepthTexture);
		mOccluderFrameBuffer->setTexture(0, *mOccluderColorTexture);
		mOccluderFrameBuffer->setDepth(*mOccluderDepthTexture);

		mHZBTexture = RHICreateTexture2D(ETexture::R32F, mOccluderDepthTexture->getSizeX(), mOccluderDepthTexture->getSizeY(), 4, numSamples, TCF_CreateSRV | TCF_CreateUAV);
		mHZBScratchTexture = RHICreateTexture2D(ETexture::R32F, mOccluderDepthTexture->getSizeX(), mOccluderDepthTexture->getSizeY(), 4, numSamples, TCF_CreateSRV | TCF_CreateUAV);
		mHZBCullResultTexture = RHICreateTexture2D(ETexture::RGBA8, HZBCullResultTextureWidth, 1, 1, 1, TCF_CreateSRV | TCF_CreateUAV);
		mMipTestTexture = RHICreateTexture2D(ETexture::RGBA8, 8, 8, 4, 1, TCF_CreateSRV);
		for (int mipLevel = 0; mipLevel < 4; ++mipLevel)
		{
			int mipSize = Math::Max(1, 8 >> mipLevel);
			TArray<Color4ub> pixels;
			pixels.resize(mipSize * mipSize);
			std::fill(pixels.begin(), pixels.end(), MipTestColors[mipLevel]);
			RHIUpdateTexture(*mMipTestTexture, 0, 0, mipSize, mipSize, pixels.data(), mipLevel);
		}
		for (int mipLevel = 0; mipLevel < 4; ++mipLevel)
		{
			TArray<uint8> readbackData;
			RHIReadTexture(*mMipTestTexture, ETexture::RGBA8, mipLevel, readbackData);
			if (readbackData.size() >= sizeof(Color4ub))
			{
				Color4ub const& color = *reinterpret_cast<Color4ub const*>(readbackData.data());
				LogMsg("MipTestTexture mip %d = (%u, %u, %u, %u)", mipLevel, color.r, color.g, color.b, color.a);
			}
			else
			{
				LogWarning(0, "MipTestTexture mip %d readback failed", mipLevel);
			}
		}

		GTextureShowManager.registerTexture("SceneTexture", mSceneTexture);
		GTextureShowManager.registerTexture("SceneDepthTexture", mSceneDepthTexture);
		GTextureShowManager.registerTexture("OccluderDepthTexture", mOccluderDepthTexture);
		GTextureShowManager.registerTexture("HZBTexture", mHZBTexture);
		GTextureShowManager.registerTexture("HZBCullResultTexture", mHZBCullResultTexture);
		GTextureShowManager.registerTexture("MipTestTexture", mMipTestTexture);
	}

	void RenderEngine::generateHZB(RHICommandList& commandList, RHITexture2D& sourceDepthTexture)
	{
		if (mProgHZBGenerate == nullptr || !mHZBTexture.isValid() || !mHZBScratchTexture.isValid())
			return;

		int numMipLevel = mHZBTexture->getDesc().numMipLevel;
		if (numMipLevel <= 0)
			return;



		GPU_PROFILE("Generate HZB");

		RHIResourceTransition(commandList, { &sourceDepthTexture }, EResourceTransition::SRV);
		RHIResourceTransition(commandList, { mHZBTexture }, EResourceTransition::UAV);
		RHIResourceTransition(commandList, { mHZBScratchTexture }, EResourceTransition::SRV);

		for (int i = 0; i < numMipLevel; ++i)
		{
			uint32 srcW = Math::Max(1, mHZBTexture->getSizeX() >> Math::Max(0, i - 1));
			uint32 srcH = Math::Max(1, mHZBTexture->getSizeY() >> Math::Max(0, i - 1));
			uint32 destW = Math::Max(1, mHZBTexture->getSizeX() >> i);
			uint32 destH = Math::Max(1, mHZBTexture->getSizeY() >> i);

			if (i == 0)
			{
				srcW = sourceDepthTexture.getSizeX();
				srcH = sourceDepthTexture.getSizeY();
				destW = mHZBTexture->getSizeX();
				destH = mHZBTexture->getSizeY();
			}

			if (i == 0)
			{
				RHISetComputeShader(commandList, mProgHZBGenerate->getRHI());
				mProgHZBGenerate->setTexture(commandList, SHADER_PARAM(SourceTexture), sourceDepthTexture, SHADER_SAMPLER(SourceTexture), TStaticSamplerState<ESampler::Point>::GetRHI());
				mProgHZBGenerate->setParam(commandList, SHADER_PARAM(SourceTextureSize), IntVector2((int)srcW, (int)srcH));
				mProgHZBGenerate->setParam(commandList, SHADER_PARAM(DestTextureSize), IntVector2((int)destW, (int)destH));
				mProgHZBGenerate->setParam(commandList, SHADER_PARAM(SourceMipLevel), 0);
				mProgHZBGenerate->setParam(commandList, SHADER_PARAM(DestMipLevel), 0);
				mProgHZBGenerate->setRWTexture(commandList, SHADER_PARAM(DestTexture), *mHZBTexture, 0, EAccessOp::WriteOnly);
				RHIDispatchCompute(commandList, (destW + 7) / 8, (destH + 7) / 8, 1);
				mProgHZBGenerate->clearRWTexture(commandList, SHADER_PARAM(DestTexture));
				RHIResourceTransition(commandList, { mHZBTexture }, EResourceTransition::UAVBarrier);
				continue;
			}

			// D3D11 is unreliable when reading one mip and writing another mip of the same texture in a single dispatch.
			RHIResourceTransition(commandList, { mHZBTexture }, EResourceTransition::SRV);
			RHIResourceTransition(commandList, { mHZBScratchTexture }, EResourceTransition::UAV);
			RHISetComputeShader(commandList, mProgHZBGenerate->getRHI());
			mProgHZBGenerate->setTexture(commandList, SHADER_PARAM(SourceTexture), *mHZBTexture, SHADER_SAMPLER(SourceTexture), TStaticSamplerState<ESampler::Point>::GetRHI());
			mProgHZBGenerate->setParam(commandList, SHADER_PARAM(SourceTextureSize), IntVector2((int)srcW, (int)srcH));
			mProgHZBGenerate->setParam(commandList, SHADER_PARAM(DestTextureSize), IntVector2((int)srcW, (int)srcH));
			mProgHZBGenerate->setParam(commandList, SHADER_PARAM(SourceMipLevel), i - 1);
			mProgHZBGenerate->setParam(commandList, SHADER_PARAM(DestMipLevel), i - 1);
			mProgHZBGenerate->setRWTexture(commandList, SHADER_PARAM(DestTexture), *mHZBScratchTexture, i - 1, EAccessOp::WriteOnly);
			RHIDispatchCompute(commandList, (srcW + 7) / 8, (srcH + 7) / 8, 1);
			mProgHZBGenerate->clearRWTexture(commandList, SHADER_PARAM(DestTexture));
			RHIResourceTransition(commandList, { mHZBScratchTexture }, EResourceTransition::UAVBarrier);

			RHIResourceTransition(commandList, { mHZBScratchTexture }, EResourceTransition::SRV);
			RHIResourceTransition(commandList, { mHZBTexture }, EResourceTransition::UAV);
			RHISetComputeShader(commandList, mProgHZBGenerate->getRHI());
			mProgHZBGenerate->setTexture(commandList, SHADER_PARAM(SourceTexture), *mHZBScratchTexture, SHADER_SAMPLER(SourceTexture), TStaticSamplerState<ESampler::Point>::GetRHI());
			mProgHZBGenerate->setParam(commandList, SHADER_PARAM(SourceTextureSize), IntVector2((int)srcW, (int)srcH));
			mProgHZBGenerate->setParam(commandList, SHADER_PARAM(DestTextureSize), IntVector2((int)destW, (int)destH));
			mProgHZBGenerate->setParam(commandList, SHADER_PARAM(SourceMipLevel), i - 1);
			mProgHZBGenerate->setParam(commandList, SHADER_PARAM(DestMipLevel), i);
			mProgHZBGenerate->setRWTexture(commandList, SHADER_PARAM(DestTexture), *mHZBTexture, i, EAccessOp::WriteOnly);
			RHIDispatchCompute(commandList, (destW + 7) / 8, (destH + 7) / 8, 1);
			mProgHZBGenerate->clearRWTexture(commandList, SHADER_PARAM(DestTexture));
			RHIResourceTransition(commandList, { mHZBTexture }, EResourceTransition::UAVBarrier);
		}

		RHIResourceTransition(commandList, { mHZBTexture }, EResourceTransition::SRV);
		RHIResourceTransition(commandList, { mHZBScratchTexture }, EResourceTransition::SRV);
	}

	void RenderEngine::notifyViewSizeChanged(Vec2i const& newSize)
	{
		if (mSceneTexture.isValid() && mViewSize == newSize)
			return;

		mViewSize = newSize;
		resizeRenderTarget();
	}

	MeshRenderPoolData* RenderEngine::acquireMeshRenderData(Mesh const& mesh)
	{
		return acquireMeshRenderData(mesh.mVertices.size(), mesh.mIndices.size());
	}

	MeshRenderPoolData* RenderEngine::acquireMeshRenderData(uint32 vertexSize, uint32 indexSize)
	{
		for (auto meshData : mMeshPool)
		{
			if (!meshData->vertexAllocator.canAllocate(vertexSize, 0))
				continue;
			if (!meshData->indexAllocator.canAllocate(indexSize, 0))
				continue;
			return meshData;
		}


		auto meshData = new MeshRenderPoolData;
		meshData->initialize();
		mMeshPool.push_back(meshData);
		return meshData;
	}

	double GTriTime = 0.0;
	struct RnederContext
	{
		Matrix4 worldToClip;
		TransformStack stack;

#define USE_QUAD 0
		template< typename TShader >
		void setupShader(RHICommandList& commandList, TShader& shader)
		{
			SET_SHADER_PARAM(commandList, shader, LocalToWorld, stack.get());
			SET_SHADER_PARAM(commandList, shader, WorldToClip, worldToClip);

		}

		void setupShader(RHICommandList& commandList, LinearColor const& color = LinearColor(1, 1, 1, 1))
		{
			RHISetFixedShaderPipelineState(commandList, stack.get() * worldToClip, color);
		}
	};

	using Math::Plane;

	static void GetFrustomPlanes(Vector3 const& worldPos, Vector3 const& viewDir, Matrix4 const& clipToWorld, Plane outPlanes[])
	{
		Vector3 centerNearPos = (Vector4(0, 0, 0, 1) * clipToWorld).dividedVector();
		Vector3 centerFarPos = (Vector4(0, 0, 1, 1) * clipToWorld).dividedVector();

		Vector3 posRT = (Vector4(1, 1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posLB = (Vector4(-1, -1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posRB = (Vector4(1, -1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posLT = (Vector4(-1, 1, 1, 1) * clipToWorld).dividedVector();

		outPlanes[0] = Plane(viewDir, centerNearPos); //ZFar
		outPlanes[1] = Plane(-viewDir, centerFarPos); //ZNear
		outPlanes[2] = Plane(worldPos, posRT, posLT); //top
		outPlanes[3] = Plane(worldPos, posLB, posRB); //bottom
		outPlanes[4] = Plane(worldPos, posRB, posRT); //right
		outPlanes[5] = Plane(worldPos, posLT, posLB); //left
	}


	void RenderEngine::renderWorld(ICamera& camera)
	{
		GPU_PROFILE("Render World");

		++mRenderFrame;

		World& world = *mClientWorld;

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		Vec3f camPos = camera.getPos();
		Vec3f viewPos = camPos + camera.getViewDir();
		Vec3f upDir = camera.getUpDir();

		ICamera* cameraRender = mDebugCamera ? mDebugCamera : &camera;
		Matrix4 projectMatrix = PerspectiveMatrix(Math::DegToRad(100.0f / mAspect), mAspect, 0.01, 2000);
		Matrix4 worldToView = LookAtMatrix(camera.getPos(), camera.getViewDir(), camera.getUpDir());
		Matrix4 worldToClip = worldToView * projectMatrix;

		GTextureShowManager.setProjectMatrix(AdjustProjectionMatrixForRHI(projectMatrix));

		Matrix4 worldToViewRender;
		Matrix4 worldToClipRender;
		if (mDebugCamera)
		{
			worldToViewRender = LookAtMatrix(mDebugCamera->getPos(), mDebugCamera->getViewDir(), mDebugCamera->getUpDir());
			worldToClipRender = worldToViewRender * projectMatrix;
		}
		else
		{
			worldToViewRender = worldToView;
			worldToClipRender = worldToClip;
		}

		Matrix4 clipToWorld;
		float det;
		worldToClip.inverse(clipToWorld, det);

		Plane clipPlanes[6];
		GetFrustomPlanes(camPos, camera.getViewDir(), clipToWorld, clipPlanes);

		RnederContext context;
		context.worldToClip = AdjustProjectionMatrixForRHI(worldToViewRender * projectMatrix);

		if (mDebugCamera)
		{
			mDebugPrimitives.addCubeLine(camPos, Quaternion{ BasisMaterix::FromZY(camera.getViewDir(), camera.getUpDir()) }, Vec3f(0.5, 0.5, 0.5), LinearColor(1, 0, 0, 1), 4);


			Vector3 vertices[8];
			FViewUtils::GetFrustumVertices(clipToWorld, vertices, false);
			for (int i = 0; i < 4; ++i)
			{
				mDebugPrimitives.addLine(vertices[i], vertices[4 + i], Color3f(1, 0.5, 0), 2);
				mDebugPrimitives.addLine(vertices[i], vertices[(i + 1) % 4], Color3f(1, 0.5, 0), 2);
				mDebugPrimitives.addLine(vertices[i + 4], vertices[4 + (i + 1) % 4], Color3f(1, 0.5, 0), 2);
			}

		}

		int bx = Math::FloorToInt(camPos.x);
		int by = Math::FloorToInt(camPos.y);

		ChunkPos cPos;
		cPos.setBlockPos(bx, by);
		int viewDist = 20;

		int chunkRenderCount = 0;
		int triangleCount = 0;





		TArray<ChunkRenderData::Layer*> layerDrawList;
		struct HZBOccluderCandidate
		{
			ChunkRenderData::Layer* layer;
			float distSq;
		};
		TArray<HZBOccluderCandidate> hzbOccluderCandidates;
		int64 visTriangleCount = 0;
		int64 occludedTriangleCount = 0;
		struct OccluderInfo
		{
			AABBox box;
			AABBox boxProjected;
		};
		TArray<OccluderInfo> activeOccluders;

		auto ProjectBox = [&](Math::TAABBox<Vec3f> const& box, AABBox& outProjected)
		{
			Vector3 v[8];
			v[0] = Vector3(box.min.x, box.min.y, box.min.z);
			v[1] = Vector3(box.max.x, box.min.y, box.min.z);
			v[2] = Vector3(box.max.x, box.max.y, box.min.z);
			v[3] = Vector3(box.min.x, box.max.y, box.min.z);
			v[4] = Vector3(box.min.x, box.min.y, box.max.z);
			v[5] = Vector3(box.max.x, box.min.y, box.max.z);
			v[6] = Vector3(box.max.x, box.max.y, box.max.z);
			v[7] = Vector3(box.min.x, box.max.y, box.max.z);

			outProjected.max = Vector3(-2,-2,-2);
			outProjected.min = Vector3(2,2,2);
			for (int i = 0; i < ARRAY_SIZE(v); ++i)
			{
				Vector4 clip = Vector4(v[i], 1.0f) * worldToClipRender;
				if (clip.w <= 0.001f) return false; // Conservative: If any point is behind or on near plane, don't use as occluder

				Vector3 ndc = clip.dividedVector();
				outProjected.max = outProjected.max.max(ndc);
				outProjected.min = outProjected.min.min(ndc);
			}
			return (outProjected.max.x > outProjected.min.x && outProjected.max.y > outProjected.min.y);
		};

		auto ProcessChunk = [&](ChunkPos chunkPos)
		{
			ChunkDataMap::iterator iter = mChunkMap.find(chunkPos.hash_value());
			ChunkRenderData* data = NULL;
			if (iter == mChunkMap.end())
			{
				Chunk* chunk = world.getChunk(chunkPos, true);
				if (!chunk)
					return;

				data = new ChunkRenderData;
				data->chunk = chunk;
				mChunkMap.insert(std::make_pair(chunkPos.hash_value(), data));
				updateRenderData(data);
			}
			else
			{
				data = iter->second;
			}

			if (data == nullptr || data->state != ChunkRenderData::eMesh)
				return;


			int chunkVisibility = FViewUtils::IsVisible(clipPlanes, data->bound);
			if (mDebugCamera)
			{
				int const ColorMap[] =
				{
					EColor::Red,
					EColor::Green,
					EColor::Blue,
				};

				mDebugPrimitives.addCubeLine(data->bound.getCenter(), Quaternion::Identity(), data->bound.getSize(), LinearColor(RenderUtility::GetColor(ColorMap[chunkVisibility])), 2);
			}

			if (chunkVisibility == 0)
				return;

			if (!bUseHZBOcclusion)
			{
				AABBox cBox;
				if (ProjectBox(data->bound, cBox))
				{
					int testCount = (int)activeOccluders.size();
					for (int i = 0; i < testCount; ++i)
					{
						auto const& occluder = activeOccluders[i].boxProjected;
						if (cBox.min.z > occluder.max.z - 0.001f &&
							cBox.min.x >= occluder.min.x && cBox.max.x <= occluder.max.x &&
							cBox.min.y >= occluder.min.y && cBox.max.y <= occluder.max.y)
						{
							for (int k = 0; k < data->numLayer; ++k)
							{
								if (data->layers[k].meshPool)
									occludedTriangleCount += (data->layers[k].args.indexCountPerInstance / 3) * data->layers[k].args.instanceCount;
							}
							return;
						}
					}
				}
			}

			// Optimization: Determine layer traversal order (Top-to-Bottom for better occlusion seeding)
			bool bFromTop = true; // Always try top-to-bottom to catch surfaces as occluders
			int start = bFromTop ? data->numLayer - 1 : 0;
			int end = bFromTop ? -1 : data->numLayer;
			int step = bFromTop ? -1 : 1;

			for (int i = start; i != end; i += step)
			{
				auto& layer = data->layers[i];
				if (layer.meshPool == nullptr)
					continue;

				int layerVisibility = FViewUtils::IsVisible(clipPlanes, layer.bound);
				if (layerVisibility == 0)
					continue;

				// Hard Distance Clip: If an object is further than 256 units, 
				// just skip it. This is the ultimate way to save GPU vertex/pixel time.
				Vector3 center = layer.bound.getCenter();
				float distSq = (center - camPos).length2();
				if (distSq > 256.0f * 256.0f)
				{
					occludedTriangleCount += (layer.args.indexCountPerInstance / 3) * layer.args.instanceCount;
					continue;
				}

				layerDrawList.push_back(&layer);

				Math::TAABBox<Vec3f> const& hzbCandidateBound = layer.bound;
				if (distSq < 192.0f * 192.0f && hzbCandidateBound.isValid())
				{
					AABBox oBox;
					bool bProjected = ProjectBox(hzbCandidateBound, oBox);
					float area = 0.0f;
					bool bAcceptHZBOccluder = false;
					if (bProjected)
					{
						area = (oBox.max.x - oBox.min.x) * (oBox.max.y - oBox.min.y);
						bAcceptHZBOccluder = (area > 0.005f) || (distSq < 96.0f * 96.0f);
					}
					else
					{
						// Keep nearby occluders even if their AABB intersects the near plane.
						bAcceptHZBOccluder = (distSq < 64.0f * 64.0f);
					}

					if (bAcceptHZBOccluder)
					{
						hzbOccluderCandidates.push_back({ &layer, distSq });
					}

					AABBox occluderProjectedBox;
					if (layer.occluderBox.isValid() && ProjectBox(layer.occluderBox, occluderProjectedBox))
					{
						float occluderArea = (occluderProjectedBox.max.x - occluderProjectedBox.min.x) * (occluderProjectedBox.max.y - occluderProjectedBox.min.y);
						if (occluderArea > 0.005f)
						{
							if (activeOccluders.size() < 32)
							{
								activeOccluders.push_back({ layer.occluderBox, occluderProjectedBox });
							}
							else
							{
								// Replace the one that is FURTHEST away, as closer occluders are better
								int worstIdx = -1;
								float maxZ = -2.0f;
								for (int k = 0; k < 32; ++k)
								{
									if (activeOccluders[k].boxProjected.max.z > maxZ)
									{
										maxZ = activeOccluders[k].boxProjected.max.z;
										worstIdx = k;
									}
								}
								if (occluderProjectedBox.max.z < maxZ)
								{
									activeOccluders[worstIdx] = { layer.occluderBox, occluderProjectedBox };
								}
							}
						}
					}
				}
			}

			++chunkRenderCount;
		};


		int chunkMeshCount = 0;
		int chunkLayerCount = 0;
		int chunkLayerDepthPrepassCount = 0;
		{
			PROFILE_ENTRY("Render Chunks");


			ProcessChunk(cPos);

			for (int n = 1; n <= viewDist; ++n)
			{
				for (int i = -(n - 1); i <= (n - 1); ++i)
				{
					ProcessChunk(cPos + Vec2i(i, n));
					ProcessChunk(cPos + Vec2i(i, -n));
					ProcessChunk(cPos + Vec2i(n, i));
					ProcessChunk(cPos + Vec2i(-n, i));
				}

				ProcessChunk(cPos + Vec2i(n, n));
				ProcessChunk(cPos + Vec2i(-n, n));
				ProcessChunk(cPos + Vec2i(n, -n));
				ProcessChunk(cPos + Vec2i(-n, -n));
			}

			TArray< DrawCmdArgs > drawCmdList;
			TArray<int> meshPoolCmdBaseOffsets;
			TArray<int> meshPoolCmdCounts;
			TArray<int> hzbCullTriangleCounts;
			int hzbCullTestCount = 0;
			meshPoolCmdBaseOffsets.resize((int)mMeshPool.size());
			meshPoolCmdCounts.resize((int)mMeshPool.size());
			for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
			{
				meshPoolCmdBaseOffsets[poolIndex] = 0;
				meshPoolCmdCounts[poolIndex] = 0;
			}
			std::sort(hzbOccluderCandidates.begin(), hzbOccluderCandidates.end(), [](HZBOccluderCandidate const& a, HZBOccluderCandidate const& b)
			{
				return a.distSq < b.distSq;
			});
			if (hzbOccluderCandidates.size() > 128)
			{
				hzbOccluderCandidates.resize(128);
			}

			{
				GPU_PROFILE("HZB Occluder Depth");
				RHISetFrameBuffer(commandList, mOccluderFrameBuffer);
				RHISetViewport(commandList, 0, 0, mOccluderDepthTexture->getSizeX(), mOccluderDepthTexture->getSizeY());
				LinearColor clearColor(0, 0, 0, 0);
				RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &clearColor, 1);
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::DepthNear>::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());
				RHISetShaderProgram(commandList, mProgBlockRender->getRHI());
				context.setupShader(commandList, *mProgBlockRender);
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBlockRender, BlockTexture, *mTexBlockAtlas, TStaticSamplerState<ESampler::Bilinear>::GetRHI());

				for (HZBOccluderCandidate const& candidate : hzbOccluderCandidates)
				{
					auto* layer = candidate.layer;
					auto* meshPool = layer ? layer->meshPool : nullptr;
					if (meshPool == nullptr)
						continue;

					InputStreamInfo inputstream;
					inputstream.buffer = meshPool->vertexBuffer;
					inputstream.offset = 0;
					inputstream.stride = meshPool->vertexBuffer->getElementSize();
					RHISetInputStream(commandList, mBlockInputLayout, &inputstream, 1);
					RHISetIndexBuffer(commandList, meshPool->indexBuffer);
					RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, layer->args.startIndexLocation, layer->args.indexCountPerInstance, layer->args.baseVertexLocation);
					++chunkLayerDepthPrepassCount;
				}

				RHISetFrameBuffer(commandList, nullptr);
				RHIResourceTransition(commandList, { mOccluderDepthTexture }, EResourceTransition::SRV);
			}

			if (bUseHZBOcclusion)
			{
				generateHZB(commandList, *mOccluderDepthTexture);

				if (!layerDrawList.empty() && mHZBCullItemBuffer.isValid() && mHZBCullResultTexture.isValid())
				{
					for (auto mesh : mMeshPool)
					{
						mesh->drawCmdList.clear();
					}

					int totalCmdCount = 0;
					for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
					{
						meshPoolCmdBaseOffsets[poolIndex] = totalCmdCount;
						int count = 0;
						for (auto* layer : layerDrawList)
						{
							if (layer->meshPool == mMeshPool[poolIndex])
							{
								++count;
							}
						}
						meshPoolCmdCounts[poolIndex] = count;
						totalCmdCount += count;
					}

					int testCount = Math::Min<int>(totalCmdCount, MaxHZBCullItems);
					if (testCount > 0)
					{
						int remainingCount = testCount;
						for (int poolIndex = 0; poolIndex < (int)meshPoolCmdCounts.size(); ++poolIndex)
						{
							meshPoolCmdBaseOffsets[poolIndex] = testCount - remainingCount;
							meshPoolCmdCounts[poolIndex] = Math::Min(meshPoolCmdCounts[poolIndex], remainingCount);
							remainingCount -= meshPoolCmdCounts[poolIndex];
						}

						TArray<HZBCullGPUItem> cullItems;
						cullItems.resize(testCount);
						hzbCullTriangleCounts.resize(testCount);
						hzbCullTestCount = testCount;
						int outputIndex = 0;
						for (int poolIndex = 0; poolIndex < (int)mMeshPool.size() && outputIndex < testCount; ++poolIndex)
						{
							for (auto* layer : layerDrawList)
							{
								if (layer->meshPool != mMeshPool[poolIndex] || outputIndex >= testCount)
									continue;

								auto const& bound = layer->bound;
								auto& item = cullItems[outputIndex];
								item.boxMin = bound.min;
								item.boxMax = bound.max;
								item.padding0 = 0;
								item.padding1 = 0;
								item.outputIndex = outputIndex;
								item.indexCountPerInstance = layer->args.indexCountPerInstance;
								item.startIndexLocation = layer->args.startIndexLocation;
								item.baseVertexLocation = layer->args.baseVertexLocation;
								item.instanceCount = layer->args.instanceCount;
								item.startInstanceLocation = layer->args.startInstanceLocation;
								item.padding2 = 0;
								item.padding3 = 0;
								hzbCullTriangleCounts[outputIndex] = (layer->args.indexCountPerInstance / 3) * layer->args.instanceCount;
								++outputIndex;
							}
						}
						RHIUpdateBuffer(commandList, *mHZBCullItemBuffer, 0, testCount, cullItems.data());

						{
							GPU_PROFILE("HZB Cull");

							auto* progHZBCull = ShaderManager::Get().getGlobalShaderT< HZBCullCS >();
							RHIResourceTransition(commandList, { mHZBTexture }, EResourceTransition::SRV);
							RHIResourceTransition(commandList, { mHZBCullResultTexture, mCmdBuildBuffer}, EResourceTransition::UAV);
							RHISetComputeShader(commandList, progHZBCull->getRHI());
							progHZBCull->setStorageBuffer(commandList, SHADER_PARAM(CullItems), *mHZBCullItemBuffer, EAccessOp::ReadOnly);
							progHZBCull->setTexture(commandList, SHADER_PARAM(HZBTexture), *mHZBTexture);
							progHZBCull->setStorageBuffer(commandList, SHADER_PARAM(VisibleCommands), *mCmdBuildBuffer, EAccessOp::ReadAndWrite);
							progHZBCull->setParam(commandList, SHADER_PARAM(WorldToClip), context.worldToClip);
							progHZBCull->setParam(commandList, SHADER_PARAM(ViewSize), IntVector2(mOccluderDepthTexture->getSizeX(), mOccluderDepthTexture->getSizeY()));
							progHZBCull->setParam(commandList, SHADER_PARAM(ResultTextureSize), IntVector2(HZBCullResultTextureWidth, 1));
							progHZBCull->setParam(commandList, SHADER_PARAM(NumMipLevel), mHZBTexture->getDesc().numMipLevel);
							progHZBCull->setParam(commandList, SHADER_PARAM(NumItems), testCount);
							progHZBCull->setParam(commandList, SHADER_PARAM(DepthBias), 0.0005f);
							progHZBCull->setRWTexture(commandList, SHADER_PARAM(ResultTexture), *mHZBCullResultTexture, 0, EAccessOp::WriteOnly);
							RHIDispatchCompute(commandList, (testCount + 63) / 64, 1, 1);
							progHZBCull->clearRWTexture(commandList, SHADER_PARAM(ResultTexture));
							progHZBCull->clearBuffer(commandList, SHADER_PARAM(VisibleCommands), EAccessOp::ReadAndWrite);
							RHIResourceTransition(commandList, { mCmdBuildBuffer }, EResourceTransition::UAVBarrier);
							RHIResourceTransition(commandList, { mHZBCullResultTexture }, EResourceTransition::SRV);
						}

						RHICopyResource(commandList, *mCmdBuffer, *mCmdBuildBuffer);
					}
				}
			}

			RHISetFrameBuffer(commandList, mSceneFrameBuffer);
			RHISetViewport(commandList, 0, 0, mViewSize.x, mViewSize.y);
			LinearColor clearColor(0, 0, 0, 1);
			RHIClearRenderTargets(commandList, EClearBits::All, &clearColor, 1);

			// Front-to-Back Sorting: Closest first to maximize Early-Z culling
			std::sort(layerDrawList.begin(), layerDrawList.end(), [&](ChunkRenderData::Layer* a, ChunkRenderData::Layer* b)
			{
				float distA = (a->bound.getCenter() - camPos).length2();
				float distB = (b->bound.getCenter() - camPos).length2();
				return distA < distB;
			});

			triangleCount = 0;
			if (!bUseHZBOcclusion)
			{
				for (auto layer : layerDrawList)
				{
					triangleCount += (layer->args.indexCountPerInstance / 3) * layer->args.instanceCount;

					auto meshPool = layer->meshPool;
					if (meshPool->drawFrame != mRenderFrame)
					{
						meshPool->drawCmdList.clear();
						meshPool->drawFrame = mRenderFrame;
					}
					meshPool->drawCmdList.push_back(layer->args);
				}
			}

			drawCmdList.clear();
			if (!bUseHZBOcclusion)
			{
				drawCmdList.reserve(2048);
				for (auto mesh : mMeshPool)
				{
					if (mesh->drawFrame != mRenderFrame)
						continue;

					mesh->cmdOffset = sizeof(DrawCmdArgs) * (uint32)drawCmdList.size();
					drawCmdList.append(mesh->drawCmdList);
				}
			}
			else
			{
				for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
				{
					mMeshPool[poolIndex]->cmdOffset = sizeof(DrawCmdArgs) * meshPoolCmdBaseOffsets[poolIndex];
				}
			}

			bool bHaveIndirectDraw = bUseHZBOcclusion ? !layerDrawList.empty() : !drawCmdList.empty();
			if (bHaveIndirectDraw)
			{
				if (!bUseHZBOcclusion)
				{
					RHIUpdateBuffer(*mCmdBuffer, 0, (uint32)drawCmdList.size(), drawCmdList.data());
				}

				// 3. Z-Prepass (Depth Only)
				if ( GRenderPreDepth )
				{
					GPU_PROFILE("Z-Prepass");
					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
					RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::DepthNear>::GetRHI());
					RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());
					RHISetShaderProgram(commandList, mProgBlockRenderDepth->getRHI());
					context.setupShader(commandList, *mProgBlockRenderDepth);

					for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
					{
						auto mesh = mMeshPool[poolIndex];
						int numCommands = bUseHZBOcclusion ? meshPoolCmdCounts[poolIndex] : (int)mesh->drawCmdList.size();
						if (numCommands == 0) continue;

						InputStreamInfo inputstream;
						inputstream.buffer = mesh->vertexBuffer;
						inputstream.offset = 0;
						inputstream.stride = mesh->vertexBuffer->getElementSize();
						RHISetInputStream(commandList, mBlockInputLayout, &inputstream, 1);
						RHISetIndexBuffer(commandList, mesh->indexBuffer);

						RHIDrawIndexedPrimitiveIndirect(commandList, EPrimitive::TriangleList, mCmdBuffer, mesh->cmdOffset, numCommands);
					}
				}

				// 4. Base Pass / Overdraw
				{
					GPU_PROFILE("Base Pass");
					chunkLayerCount = (int)drawCmdList.size();
					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());

					if (bShowOverdraw)
					{
						RHISetDepthStencilState(commandList, TStaticDepthStencilState<false, ECompareFunc::Always>::GetRHI());
						RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::One>::GetRHI());
						RHISetShaderProgram(commandList, mProgBlockRenderOverdraw->getRHI());
						context.setupShader(commandList, *mProgBlockRenderOverdraw);
					}
					else if (GRenderPreDepth)
					{
						RHISetDepthStencilState(commandList, TStaticDepthStencilState<false, ECompareFunc::DepthNearEqual>::GetRHI());
						RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
						RHISetShaderProgram(commandList, mProgBlockRender->getRHI());
						context.setupShader(commandList, *mProgBlockRender);
						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBlockRender, BlockTexture, *mTexBlockAtlas, TStaticSamplerState<ESampler::Bilinear>::GetRHI());
					}
					else
					{
						RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::DepthNear>::GetRHI());
						RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
						RHISetShaderProgram(commandList, mProgBlockRender->getRHI());
						context.setupShader(commandList, *mProgBlockRender);
						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBlockRender, BlockTexture, *mTexBlockAtlas, TStaticSamplerState<ESampler::Bilinear>::GetRHI());
					}

					chunkMeshCount = 0; // Reset for accurate display
					for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
					{
						auto mesh = mMeshPool[poolIndex];
						int numCommands = bUseHZBOcclusion ? meshPoolCmdCounts[poolIndex] : (int)mesh->drawCmdList.size();
						if (!bUseHZBOcclusion && mesh->drawFrame != mRenderFrame) continue;
						if (numCommands == 0) continue;

						InputStreamInfo inputstream;
						inputstream.buffer = mesh->vertexBuffer;
						inputstream.offset = 0;
						inputstream.stride = mesh->vertexBuffer->getElementSize();
						RHISetInputStream(commandList, mBlockInputLayout, &inputstream, 1);
						RHISetIndexBuffer(commandList, mesh->indexBuffer);

						RHIDrawIndexedPrimitiveIndirect(commandList, EPrimitive::TriangleList, mCmdBuffer, mesh->cmdOffset, numCommands);
						++chunkMeshCount;
					}
				}
			}

			if (bUseHZBOcclusion && hzbCullTestCount > 0)
			{
				TArray<uint8> readbackData;
				RHIReadTexture(*mHZBCullResultTexture, ETexture::RGBA8, 0, readbackData);

				int const maxReadableItemCount = Math::Min<int>(hzbCullTestCount, readbackData.size() / sizeof(Color4ub));
				triangleCount = 0;
				for (int itemIndex = 0; itemIndex < maxReadableItemCount; ++itemIndex)
				{
					Color4ub const& color = *(reinterpret_cast<Color4ub const*>(readbackData.data()) + itemIndex);
					bool const bVisible = (color.r > 0);
					if (bVisible)
					{
						triangleCount += hzbCullTriangleCounts[itemIndex];
					}
					else
					{
						occludedTriangleCount += hzbCullTriangleCounts[itemIndex];
					}
				}
			}
		}

#if 0
		BlockPosInfo info;
		BlockId id = world.rayBlockTest(camPos, camera.getViewDir(), 100, &info);
		if (id)
		{
			context.stack.push();
			context.stack.translate(Vector3(info.x, info.y, info.z) + 0.1 * GetFaceNoraml(info.face));
			context.setupShader(commandList, LinearColor(1, 1, 1, 1));
			TRenderRT<RTVF_XYZ>::Draw(commandList, EPrimitive::LineLoop, GetFaceVertices(info.face), 4);
			context.stack.pop();
		}
#endif

		{
			context.stack.push();
			context.stack.translate(Vector3(0, 10, 0));
			float len = 10;
			context.setupShader(commandList);
			DrawUtility::AixsLine(commandList, len);
			context.stack.pop();
		}

		{
			mDebugPrimitives.drawDynamic(commandList, mViewSize, worldToClipRender, cameraRender->getViewDir().cross(cameraRender->getUpDir()), cameraRender->getUpDir());
		}

		mDebugPrimitives.clear();

		RHIResourceTransition(commandList, { mSceneTexture }, EResourceTransition::SRV);
		RHISetFrameBuffer(commandList, nullptr);
		RHISetViewport(commandList, 0, 0, mViewSize.x, mViewSize.y);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1);

		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());
		ShaderHelper::Get().copyTextureToBuffer(commandList, *mSceneTexture);


		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();
		g.drawTextF(Vector2(10, 10), "chunkCount = %d, chunkRenderCount = %d", mChunkMap.size(), chunkRenderCount);
		int64 totalPotential = triangleCount + occludedTriangleCount;
		double cullingRate = totalPotential > 0 ? (double)occludedTriangleCount * 100.0 / totalPotential : 0.0;
		g.drawTextF(Vector2(10, 25), "Total Tri: %lld", totalPotential);
		g.drawTextF(Vector2(10, 40), "Occluded Tri: %lld (%.1f%% culled)", occludedTriangleCount, cullingRate);
		g.drawTextF(Vector2(10, 55), "Drawn Tri: %lld", triangleCount);
		g.drawTextF(Vector2(10, 70), "MergeTimeAcc = %lf, Tri Time = %lf", mMergeTimeAcc, GTriTime);
		g.drawTextF(Vector2(10, 85), "chunkMeshCount = %d, chunkLayerCount = %d, chunkLayerDepthPrepassCount = %d", chunkMeshCount, chunkLayerCount, chunkLayerDepthPrepassCount);

#if 0

		g.drawCustomFunc([this, &g](RHICommandList& commandList, Matrix4 const& baseTransform, RenderBatchedElement& element)
		{
			Matrix4 debugProjectMatrix = GTextureShowManager.getProjectMatrix();
			Vector4 depthMappingParams(0, 100, debugProjectMatrix(2, 2), debugProjectMatrix(3, 2));
			DrawUtility::DrawDepthTexture(commandList, baseTransform, *mSceneDepthTexture, TStaticSamplerState<>::GetRHI(), Vector2(200, 200), Vector2(400, 200), depthMappingParams);
		});

		g.drawCustomFunc([this, &g](RHICommandList& commandList, Matrix4 const& baseTransform, RenderBatchedElement& element)
		{
			DrawUtility::DrawTexture(commandList, baseTransform, *mSceneTexture, TStaticSamplerState<>::GetRHI(), Vector2(200, 500), Vector2(400, 200));
		});
#endif
		g.endRender();

	}

	void RenderEngine::beginRender()
	{
	}

	void RenderEngine::endRender()
	{

	}

	void RenderEngine::onModifyBlock(int bx, int by, int bz)
	{
		ChunkPos cPos;
		cPos.setBlockPos(bx, by);
		markChunkDirty(cPos);
		cPos.setBlockPos(bx + 1, by);
		markChunkDirty(cPos);
		cPos.setBlockPos(bx - 1, by);
		markChunkDirty(cPos);
		cPos.setBlockPos(bx, by + 1);
		markChunkDirty(cPos);
		cPos.setBlockPos(bx, by - 1);
		markChunkDirty(cPos);
	}

	bool MeshRenderPoolData::initialize()
	{
		uint32 MaxVerticesCount = 131072 * 64 * 4;
		uint32 MaxIndicesCount = FBitUtility::NextNumberOfPow2(6 * (MaxVerticesCount / 4));
		vertexBuffer = RHICreateVertexBuffer(sizeof(Mesh::Vertex), MaxVerticesCount, BCF_DefalutValue);
		indexBuffer = RHICreateIndexBuffer(MaxIndicesCount, true, BCF_DefalutValue);
		vertexAllocator.initialize(MaxVerticesCount, 32);
		indexAllocator.initialize(MaxIndicesCount, 64);

		return true;
	}

}//namespace Cube
