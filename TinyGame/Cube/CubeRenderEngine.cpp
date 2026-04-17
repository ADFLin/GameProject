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

#include <chrono>

namespace Cube
{
	using namespace Render;

	namespace
	{
		constexpr uint32 GetFullLayerMask()
		{
			return (ChunkRenderData::MaxLayerCount >= 32) ? 0xFFFFFFFFu : ((1u << ChunkRenderData::MaxLayerCount) - 1u);
		}
	}

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

	static int constexpr MaxHZBCullItems = 4 * 4096;
	static int constexpr HZBCullResultTextureWidth = 4 * 4096;

	struct HZBCullGPUItem
	{
		Vector3 boxMin;
		float padding0;
		Vector3 boxMax;
		float padding1;
		uint32 outputIndex;

		DrawCmdArgs cmdArgs;
	};

	bool GRenderPreDepth = false;

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
		mHighPriorityGeneratePool = new QueueThreadPool;
		mHighPriorityGeneratePool->init(2);
	}

	RenderEngine::~RenderEngine()
	{
		cleanupRenderData();
		delete mGereatePool;
		delete mHighPriorityGeneratePool;
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
		desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Short4, true);
		desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD2, EVertex::UInt1);
		desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Half2);
		mBlockInputLayout = RHICreateInputLayout(desc);

		mCmdBuildBuffer = RHICreateBuffer(sizeof(uint32), MaxHZBCullItems * 16 * 5, BCF_CreateSRV | BCF_CreateUAV);
		mCmdBuffer = RHICreateBuffer(sizeof(DrawCmdArgs), MaxHZBCullItems * 16, BCF_DrawIndirectArgs);
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
			updateRenderData(data, data->bHighPriorityUpdate);
		}

		if (!mPendingAddList.empty() || !mProcessingAddList.empty())
		{
			PROFILE_ENTRY("Update Mesh Data");
			processPendingMeshUpdates(mMeshUpdateBudgetPerFrame, mMeshUpdateTimeBudgetMS);
		}
	}

	void RenderEngine::processPendingMeshUpdates(int maxCount, double timeBudgetMS)
	{
		{
			PROFILE_START("Move UpdateList");
			Mutex::Locker locker(mMutexPedingAdd);
			mProcessingAddList.append(mPendingAddList);
			mPendingAddList.clear();
		}

		int actualMaxCount = (maxCount > 0) ? Math::Min<int>((int)mProcessingAddList.size(), maxCount) : (int)mProcessingAddList.size();
		int processedCount = 0;
		auto const startTime = std::chrono::steady_clock::now();
		for (; processedCount < actualMaxCount; ++processedCount)
		{
			PROFILE_ENTRY("Proc Chunk Data");
			auto updateData = mProcessingAddList[processedCount];
			auto data = updateData->chunkData;
			auto& mesh = updateData->mesh;
			using namespace Render;

			auto ReleaseLayerResources = [](ChunkRenderData::Layer& layer)
			{
				if (layer.meshPool)
				{
					layer.meshPool->vertexAllocator.deallocate(layer.vertexAllocation);
					layer.meshPool->indexAllocator.deallocate(layer.indexAlloction);
				}
				layer = ChunkRenderData::Layer();
			};

			auto FindLayerSlot = [&](int layerIndex)
			{
				for (int i = 0; i < data->numLayer; ++i)
				{
					if (data->layers[i].index == layerIndex)
						return i;
				}
				return -1;
			};

			auto RemoveLayerSlot = [&](int slot)
			{
				ReleaseLayerResources(data->layers[slot]);
				for (int i = slot; i + 1 < data->numLayer; ++i)
				{
					data->layers[i] = data->layers[i + 1];
				}
				data->layers[data->numLayer - 1] = ChunkRenderData::Layer();
				--data->numLayer;
			};

			auto InsertLayerSlot = [&](int layerIndex)
			{
				int insertPos = 0;
				while (insertPos < data->numLayer && data->layers[insertPos].index < layerIndex)
				{
					++insertPos;
				}
				for (int i = data->numLayer; i > insertPos; --i)
				{
					data->layers[i] = data->layers[i - 1];
				}
				data->layers[insertPos] = ChunkRenderData::Layer();
				data->layers[insertPos].index = layerIndex;
				++data->numLayer;
				return insertPos;
			};

			if (updateData->bFullRebuild)
			{
				resetChunkRenderData(data);
			}

			for (int i = 0; i < updateData->numLayer; ++i)
			{
				auto const& updateLayer = updateData->layers[i];
				int slot = FindLayerSlot(updateLayer.index);
				if (!updateLayer.bHasMesh)
				{
					if (slot != -1)
					{
						RemoveLayerSlot(slot);
					}
					continue;
				}

				if (slot == -1)
				{
					slot = InsertLayerSlot(updateLayer.index);
				}
				else
				{
					ReleaseLayerResources(data->layers[slot]);
					data->layers[slot].index = updateLayer.index;
				}

				auto& layer = data->layers[slot];
				auto meshData = acquireMeshRenderData(updateLayer.vertexCount, updateLayer.indexCount);
				if (meshData == nullptr)
				{
					RemoveLayerSlot(slot);
					continue;
				}

				layer.meshPool = meshData;
				layer.bound = updateLayer.bound;

				{
					PROFILE_ENTRY("Update Buffer");
					meshData->vertexAllocator.alloc(updateLayer.vertexCount, 0, layer.vertexAllocation);
					meshData->indexAllocator.alloc(updateLayer.indexCount, 0, layer.indexAlloction);
					RHIUpdateBuffer(*meshData->vertexBuffer, layer.vertexAllocation.pos, updateLayer.vertexCount, (void*)(mesh.mVertices.data() + updateLayer.vertexOffset));
					RHIUpdateBuffer(*meshData->indexBuffer, layer.indexAlloction.pos, updateLayer.indexCount, (void*)(mesh.mIndices.data() + updateLayer.indexOffset));
				}

				layer.args.baseVertexLocation = int(layer.vertexAllocation.pos) - int(updateLayer.vertexOffset);
				layer.args.startIndexLocation = layer.indexAlloction.pos;
				layer.args.indexCountPerInstance = updateLayer.indexCount;
				layer.args.instanceCount = 1;
				layer.args.startInstanceLocation = 0;
			}

			{
				PROFILE_ENTRY("Update Bound");
				data->bound.invalidate();
				for (int i = 0; i < data->numLayer; ++i)
				{
					data->bound += data->layers[i].bound;
				}
				data->state = ChunkRenderData::eMesh;
				data->bHighPriorityUpdate = false;
			}

			{
				PROFILE_ENTRY("Delete Data");
				releaseUpdateData(updateData);
			}

			if (timeBudgetMS > 0)
			{
				double elapsedMS = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
				if (elapsedMS >= timeBudgetMS)
				{
					++processedCount;
					break;
				}
			}
		}

		for (int i = processedCount; i < mProcessingAddList.size(); ++i)
		{
			mProcessingAddList[i - processedCount] = mProcessingAddList[i];
		}
		mProcessingAddList.resize(mProcessingAddList.size() - processedCount);
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
				markChunkDirty(chunkPos, GetFullLayerMask(), false);
			}
		}

	}

	void RenderEngine::updateRenderData(ChunkRenderData* data, bool bForceHighPriority)
	{
		if (data->state == ChunkRenderData::eMeshGenerating)
			return;

		bool const bFullRebuild = (data->state != ChunkRenderData::eMesh);
		uint32 const dirtyLayerMask = bFullRebuild ? GetFullLayerMask() : data->dirtyLayerMask;
		if (!bFullRebuild && dirtyLayerMask == 0)
			return;

		NeighborChunkAccess chunkAccess(*mClientWorld, data->chunk);
		for (int i = 0; i < 4; ++i)
		{
			if (chunkAccess.mNeighborChunks[i] == nullptr)
				return;
		}

		data->state = ChunkRenderData::eMeshGenerating;
		data->bHighPriorityUpdate = bForceHighPriority;
		if (!bFullRebuild)
		{
			data->dirtyLayerMask &= ~dirtyLayerMask;
			if (data->dirtyLayerMask == 0)
			{
				data->bNeedUpdate = false;
			}
		}
		else
		{
			data->bNeedUpdate = false;
			data->dirtyLayerMask = 0;
		}

		QueueThreadPool* targetPool = bForceHighPriority ? mHighPriorityGeneratePool : mGereatePool;
		targetPool->addFunctionWork(
			[this, data, chunkAccess, bFullRebuild, dirtyLayerMask]()
		{
			PROFILE_ENTRY("Chunk Mesh Generate");

			Chunk* chunk = data->chunk;

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

			UpdatedRenderData* updateData = acquireUpdateData();
			updateData->chunkData = data;
			updateData->numLayer = 0;
			updateData->bFullRebuild = bFullRebuild;
			renderer.mMesh = &updateData->mesh;

			//renderer.mDebugColor = RenderUtility::GetColor(ColorMap[(chunk->getPos().x + chunk->getPos().y) % ARRAY_SIZE(ColorMap)]);
			renderer.mDebugColor = RenderUtility::GetColor(EColor::White);
			
			PaddedBlockAccess paddedAccess;
			renderer.mBlockAccess = &paddedAccess;

			for (int indexLayer = 0; indexLayer < ChunkRenderData::MaxLayerCount; ++indexLayer)
			{
				if (!bFullRebuild && (dirtyLayerMask & (uint32(1) << indexLayer)) == 0)
					continue;

				auto layer = chunk->mLayer[indexLayer];
				auto& layerData = updateData->layers[updateData->numLayer];
				updateData->numLayer += 1;
				layerData.index = indexLayer;
				layerData.bound.invalidate();
				layerData.vertexOffset = (uint32)updateData->mesh.mVertices.size();
				layerData.indexOffset = (uint32)updateData->mesh.mIndices.size();
				layerData.vertexCount = 0;
				layerData.indexCount = 0;
				layerData.bHasMesh = false;

				if (layer)
				{
					paddedAccess.fill(chunkAccess, chunk, indexLayer);

					int zOff = indexLayer * Chunk::LayerSize;

					{
						TIME_SCOPE(mMeshBuildTimeAcc);
						renderer.setBasePos(Vec3i(chunk->getPos().x * ChunkSize, chunk->getPos().y * ChunkSize, zOff));
						renderer.drawLayer(*chunk, indexLayer);
						renderer.finalizeMesh();
					}

					layerData.bHasMesh = (layerData.indexOffset != updateData->mesh.mIndices.size());
					if (layerData.bHasMesh)
					{
						layerData.bound = renderer.bound;
						layerData.vertexCount = (uint32)updateData->mesh.mVertices.size() - layerData.vertexOffset;
						layerData.indexCount = (uint32)updateData->mesh.mIndices.size() - layerData.indexOffset;
					}
				}

				renderer.bound.invalidate();
			}

			{
				Mutex::Locker locker(mMutexPedingAdd);
				mPendingAddList.push_back(updateData);
			}
		}
		);
	}

	void RenderEngine::markChunkDirty(ChunkPos const& chunkPos)
	{
		markChunkDirty(chunkPos, GetFullLayerMask(), true);
	}

	void RenderEngine::markChunkDirty(ChunkPos const& chunkPos, uint32 layerMask, bool bHighPriority)
	{
		ChunkDataMap::iterator iter = mChunkMap.find(chunkPos.hash_value());
		if (iter == mChunkMap.end())
		{
			Chunk* chunk = mClientWorld ? mClientWorld->getChunk(chunkPos, true) : nullptr;
			if (chunk == nullptr)
				return;

			ChunkRenderData* data = new ChunkRenderData;
			data->chunk = chunk;
			data->dirtyLayerMask = layerMask;
			data->bNeedUpdate = true;
			data->bHighPriorityUpdate = bHighPriority;
			mChunkMap.insert(std::make_pair(chunkPos.hash_value(), data));
			return;
		}

		ChunkRenderData* data = iter->second;
		if (data == nullptr)
			return;

		data->dirtyLayerMask |= layerMask;
		data->bNeedUpdate = true;
		data->bHighPriorityUpdate = data->bHighPriorityUpdate || bHighPriority;
	}

	RenderEngine::UpdatedRenderData* RenderEngine::acquireUpdateData()
	{
		if (!mFreeUpdateDataList.empty())
		{
			UpdatedRenderData* data = mFreeUpdateDataList.back();
			mFreeUpdateDataList.pop_back();
			return data;
		}

		return new UpdatedRenderData;
	}

	void RenderEngine::releaseUpdateData(UpdatedRenderData* data)
	{
		data->mesh.clearBuffer();
		mFreeUpdateDataList.push_back(data);
	}

	void RenderEngine::resizeRenderTarget()
	{
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

		GTextureShowManager.registerTexture("SceneTexture", mSceneTexture);
		GTextureShowManager.registerTexture("SceneDepthTexture", mSceneDepthTexture);
		GTextureShowManager.registerTexture("OccluderDepthTexture", mOccluderDepthTexture);
		GTextureShowManager.registerTexture("HZBTexture", mHZBTexture);
		GTextureShowManager.registerTexture("HZBCullResultTexture", mHZBCullResultTexture);
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
				mProgHZBGenerate->setTexture(commandList, SHADER_PARAM(SourceTexture), sourceDepthTexture, SHADER_SAMPLER(SourceTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());
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
			mProgHZBGenerate->setTexture(commandList, SHADER_PARAM(SourceTexture), *mHZBTexture, SHADER_SAMPLER(SourceTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());
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
			mProgHZBGenerate->setTexture(commandList, SHADER_PARAM(SourceTexture), *mHZBScratchTexture, SHADER_SAMPLER(SourceTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());
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
		meshData->poolIndex = (int)mMeshPool.size();
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
		Vec2f viewDir2D = Vec2f(camera.getViewDir().x, camera.getViewDir().y);
		bool bHaveViewDir2D = (viewDir2D.length2() > 1e-4f);
		if (bHaveViewDir2D)
		{
			viewDir2D.normalize();
		}

		ICamera* cameraRender = mDebugCamera ? mDebugCamera : &camera;
		Matrix4 projectMatrix = PerspectiveMatrix(Math::DegToRad(100.0f / mAspect), mAspect, 0.01, 2000);
		Matrix4 worldToViewCull = LookAtMatrix(camera.getPos(), camera.getViewDir(), camera.getUpDir());
		Matrix4 worldToClipCull = worldToViewCull * projectMatrix;
		Matrix4 worldToViewCullRelative = LookAtMatrix(Vec3f::Zero(), camera.getViewDir(), camera.getUpDir());
		Matrix4 worldToClipCullRelative = worldToViewCullRelative * projectMatrix;

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
			worldToViewRender = worldToViewCull;
			worldToClipRender = worldToClipCull;
		}

		Matrix4 clipToWorld;
		float det;
		worldToClipCull.inverse(clipToWorld, det);

		Plane clipPlanes[6];
		GetFrustomPlanes(camPos, camera.getViewDir(), clipToWorld, clipPlanes);

		RnederContext contextCull;
		contextCull.worldToClip = AdjustProjectionMatrixForRHI(worldToClipCull);
		Matrix4 worldToClipCullRelativeRHI = AdjustProjectionMatrixForRHI(worldToClipCullRelative);

		RnederContext contextRender;
		contextRender.worldToClip = AdjustProjectionMatrixForRHI(worldToClipRender);

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
		int const nearUpdateDist = 2;
		bool bNeedSyncNearbyUpdate = false;
		bool bNeedSyncNearbyGeneralWork = false;
		for (int dy = -nearUpdateDist; dy <= nearUpdateDist; ++dy)
		{
			for (int dx = -nearUpdateDist; dx <= nearUpdateDist; ++dx)
			{
				ChunkPos nearPos = cPos + Vec2i(dx, dy);
				auto iter = mChunkMap.find(nearPos.hash_value());
				if (iter == mChunkMap.end())
					continue;

				ChunkRenderData* data = iter->second;
				if (data == nullptr)
					continue;

				if (data->bNeedUpdate)
				{
					updateRenderData(data, true);
				}
				if (data->state == ChunkRenderData::eMeshGenerating && data->bHighPriorityUpdate)
				{
					bNeedSyncNearbyUpdate = true;
					if (data->numLayer == 0)
					{
						bNeedSyncNearbyGeneralWork = true;
					}
				}
			}
		}
		if (bNeedSyncNearbyUpdate)
		{
			if (bNeedSyncNearbyGeneralWork)
			{
				mGereatePool->waitAllWorkComplete();
			}
			mHighPriorityGeneratePool->waitAllWorkComplete();
			processPendingMeshUpdates(0, 0);
		}
		int viewDist = 96 / 2;
		if (mChunkScanOffsetViewDist != viewDist)
		{
			mChunkScanOffsetViewDist = viewDist;
			mChunkScanOffsets.clear();
			int viewDistSq = viewDist * viewDist;
			for (int dy = -viewDist; dy <= viewDist; ++dy)
			{
				for (int dx = -viewDist; dx <= viewDist; ++dx)
				{
					if (dx == 0 && dy == 0)
						continue;
					int distSq = dx * dx + dy * dy;
					if (distSq > viewDistSq)
						continue;
					mChunkScanOffsets.push_back(Vec2i(dx, dy));
				}
			}
			std::sort(mChunkScanOffsets.begin(), mChunkScanOffsets.end(), [](Vec2i const& a, Vec2i const& b)
			{
				int distA = a.x * a.x + a.y * a.y;
				int distB = b.x * b.x + b.y * b.y;
				return distA < distB;
			});
		}

		int chunkRenderCount = 0;
		int triangleCount = 0;

		TArray<ChunkRenderData::Layer*> layerDrawList;
		layerDrawList.reserve(mChunkMap.size() * Chunk::NumLayer);

		struct HZBOccluderCandidate
		{
			ChunkRenderData::Layer* layer;
			float distSq;
			float projectedArea;
			float score;
		};
		static int constexpr MaxHZBPrimaryOccluders = 512;
		static int constexpr MaxHZBSecondaryOccluders = 256;
		TArray<HZBOccluderCandidate> hzbPrimaryOccluderCandidates;
		TArray<HZBOccluderCandidate> hzbOccluderCandidates;
		hzbPrimaryOccluderCandidates.reserve(MaxHZBPrimaryOccluders);
		hzbOccluderCandidates.reserve(MaxHZBSecondaryOccluders);

		int64 visTriangleCount = 0;
		int64 occludedTriangleCount = 0;
		struct OccluderInfo
		{
			AABBox box;
			AABBox boxProjected;
		};
		struct VisibleLayerItem
		{
			ChunkRenderData::Layer* layer;
			int poolIndex;
			HZBCullGPUItem cullItem;
			int triangleCount;
		};
		TArray<VisibleLayerItem> visibleLayerItems;
		visibleLayerItems.reserve(mChunkMap.size() * Chunk::NumLayer);

		int chunkRequestCount = 0;

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
				Vector3 localPos = v[i] - camPos;
				Vector4 clip = Vector4(localPos, 1.0f) * worldToClipCullRelative;
				if (clip.w <= 0.001f) return false; // Conservative: If any point is behind or on near plane, don't use as occluder

				Vector3 ndc = clip.dividedVector();
				outProjected.max = outProjected.max.max(ndc);
				outProjected.min = outProjected.min.min(ndc);
			}
			return (outProjected.max.x > outProjected.min.x && outProjected.max.y > outProjected.min.y);
		};

		auto IsPriorityScanOffset = [&](int dx, int dy)
		{
			int distSq = dx * dx + dy * dy;
			if (distSq > viewDist * viewDist)
				return false;
			int nearOmniDist = 24;
			int nearOmniDistSq = nearOmniDist * nearOmniDist;
			if (distSq <= nearOmniDistSq || !bHaveViewDir2D)
				return true;

			Vec2f dir = Vec2f(float(dx), float(dy));
			dir.normalize();
			float forwardDotThreshold = 0.35f;
			return Math::Dot(dir, viewDir2D) >= forwardDotThreshold;
		};

		auto ProcessChunk = [&](ChunkPos chunkPos)
		{
			ChunkDataMap::iterator iter = mChunkMap.find(chunkPos.hash_value());
			ChunkRenderData* data = NULL;
			if (iter == mChunkMap.end())
			{
				if (chunkRequestCount >= mChunkRequestBudgetPerFrame)
					return;

				Chunk* chunk = world.getChunk(chunkPos, true);
				++chunkRequestCount;
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

			if (data == nullptr)
				return;

			bool const bHasUsableMesh = (data->numLayer > 0);
			if (data->state == ChunkRenderData::eNone)
				return;
			if (data->state == ChunkRenderData::eMeshGenerating && !bHasUsableMesh)
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

			bool bFromTop = true;
			int start = bFromTop ? data->numLayer - 1 : 0;
			int end = bFromTop ? -1 : data->numLayer;
			int step = bFromTop ? -1 : 1;
			bool bAddedPrimaryHZBOccluder = false;

			for (int i = start; i != end; i += step)
			{
				auto& layer = data->layers[i];
				if (layer.meshPool == nullptr)
					continue;

				int layerVisibility = FViewUtils::IsVisible(clipPlanes, layer.bound);
				if (layerVisibility == 0)
				{
					if (mDebugCamera)
					{
						mDebugPrimitives.addCubeLine(layer.bound.getCenter(), Quaternion::Identity(), layer.bound.getSize(), LinearColor(RenderUtility::GetColor(EColor::Red)), 2);
					}
					continue;
				}


				Vector3 center = layer.bound.getCenter();
				float distSq = (center - camPos).length2();
#if 0

				if (distSq > 256.0f * 256.0f)
				{
					occludedTriangleCount += (layer.args.indexCountPerInstance / 3) * layer.args.instanceCount;
					continue;
				}
#endif

				layerDrawList.push_back(&layer);
				VisibleLayerItem visibleItem;
				visibleItem.layer = &layer;
				visibleItem.poolIndex = layer.meshPool->poolIndex;
				visibleItem.cullItem.boxMin = layer.bound.min - camPos;
				visibleItem.cullItem.boxMax = layer.bound.max - camPos;
				visibleItem.cullItem.padding0 = 0;
				visibleItem.cullItem.padding1 = 0;
				visibleItem.cullItem.outputIndex = 0;
				visibleItem.cullItem.cmdArgs = layer.args;
				visibleItem.triangleCount = (layer.args.indexCountPerInstance / 3) * layer.args.instanceCount;
				visibleLayerItems.push_back(visibleItem);

				if (layer.bound.isValid())
				{
					AABBox oBox;
					bool bProjected = ProjectBox(layer.bound, oBox);
					float area = 0.0f;
					bool bAcceptPrimaryHZBOccluder = false;
					bool bAcceptSecondaryHZBOccluder = false;
					if (bProjected)
					{
						float width = (oBox.max.x - oBox.min.x);
						float height = (oBox.max.y - oBox.min.y);
						area = width * height;

						// Prefer candidates that cover more screen space and stay near the center,
						// while still giving some weight to nearer occluders.
						Vector2 ndcCenter = Vector2(
							0.5f * (oBox.max.x + oBox.min.x),
							0.5f * (oBox.max.y + oBox.min.y));
						float centerDist = ndcCenter.length();
						float centerWeight = Math::Clamp(1.25f - 0.5f * centerDist, 0.35f, 1.25f);
						float distanceWeight = 1.0f / (1.0f + distSq / (256.0f * 256.0f));
						float score = area * centerWeight * (0.5f + 0.5f * distanceWeight);

						// Always try to keep the first visible layer in a chunk as an occluder.
						// This helps far terrain top layers occlude deeper layers from the same chunk.
						bAcceptPrimaryHZBOccluder = !bAddedPrimaryHZBOccluder && ((area > 0.0002f) || (distSq < 256.0f * 256.0f));
						if (bAcceptPrimaryHZBOccluder)
						{
							if (hzbPrimaryOccluderCandidates.size() < MaxHZBPrimaryOccluders)
							{
								hzbPrimaryOccluderCandidates.push_back({ &layer, distSq, area, score + 1000.0f });
								bAddedPrimaryHZBOccluder = true;
							}
						}

						// Additional occluders still favor large/central layers.
						bAcceptSecondaryHZBOccluder = (area > 0.0015f) || (distSq < 96.0f * 96.0f);
						if (!bAcceptPrimaryHZBOccluder && bAcceptSecondaryHZBOccluder)
						{
							hzbOccluderCandidates.push_back({ &layer, distSq, area, score });
						}
					}
					else
					{
						bAcceptPrimaryHZBOccluder = !bAddedPrimaryHZBOccluder && (distSq < 64.0f * 64.0f);
						if (bAcceptPrimaryHZBOccluder)
						{
							if (hzbPrimaryOccluderCandidates.size() < MaxHZBPrimaryOccluders)
							{
								hzbPrimaryOccluderCandidates.push_back({ &layer, distSq, 0.0f, 1000.0f });
								bAddedPrimaryHZBOccluder = true;
							}
						}
						else if (distSq < 64.0f * 64.0f)
						{
							// Near-plane fallback. Keep it, but score it low so proper projected
							// candidates dominate whenever available.
							hzbOccluderCandidates.push_back({ &layer, distSq, 0.0f, 0.0001f });
						}
					}
				}
			}

			++chunkRenderCount;
		};


		int chunkMeshCount = 0;
		int chunkLayerCount = 0;
		int chunkLayerHZBCount = 0;
		{
			PROFILE_ENTRY("Render Chunks");


			{
				PROFILE_ENTRY("Chunk Scan");
				auto ScanChunks = [&](bool bPriorityPass)
				{
					if (bPriorityPass)
					{
						ProcessChunk(cPos);
					}

					for (Vec2i const& offset : mChunkScanOffsets)
					{
						bool const bPriorityOffset = IsPriorityScanOffset(offset.x, offset.y);
						if (bPriorityPass != bPriorityOffset)
							continue;
						ProcessChunk(cPos + offset);
					}
				};

				ScanChunks(true);
				bool const bDoNonPriorityScan = ((mRenderFrame % mNonPriorityScanInterval) == 0) || (chunkRequestCount < mChunkRequestBudgetPerFrame / 2);
				if (bDoNonPriorityScan && chunkRequestCount < mChunkRequestBudgetPerFrame)
				{
					ScanChunks(false);
				}
			}

			{
				PROFILE_ENTRY("Release Far Chunks");
				int const releaseDist = viewDist + mChunkReleaseMargin;
				int const releaseDistSq = releaseDist * releaseDist;
				for (auto iter = mChunkMap.begin(); iter != mChunkMap.end();)
				{
					ChunkRenderData* data = iter->second;
					if (data == nullptr || data->chunk == nullptr)
					{
						delete data;
						iter = mChunkMap.erase(iter);
						continue;
					}

					ChunkPos const& pos = data->chunk->getPos();
					int dx = pos.x - cPos.x;
					int dy = pos.y - cPos.y;
					if (dx * dx + dy * dy <= releaseDistSq)
					{
						++iter;
						continue;
					}

					if (data->state == ChunkRenderData::eMeshGenerating || data->bNeedUpdate)
					{
						++iter;
						continue;
					}

					resetChunkRenderData(data);
					delete data;
					iter = mChunkMap.erase(iter);
				}
			}

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
			{
				PROFILE_ENTRY("Prepare HZB Candidates");
				auto SortByScore = [](HZBOccluderCandidate const& a, HZBOccluderCandidate const& b)
				{
					if (a.score != b.score)
						return a.score > b.score;
					if (a.projectedArea != b.projectedArea)
						return a.projectedArea > b.projectedArea;
					return a.distSq < b.distSq;
				};
				std::sort(hzbPrimaryOccluderCandidates.begin(), hzbPrimaryOccluderCandidates.end(), SortByScore);
				if (hzbPrimaryOccluderCandidates.size() > MaxHZBPrimaryOccluders)
				{
					hzbPrimaryOccluderCandidates.resize(MaxHZBPrimaryOccluders);
				}
				std::sort(hzbOccluderCandidates.begin(), hzbOccluderCandidates.end(), SortByScore);
				if (hzbOccluderCandidates.size() > MaxHZBSecondaryOccluders)
				{
					hzbOccluderCandidates.resize(MaxHZBSecondaryOccluders);
				}
				hzbOccluderCandidates.append(hzbPrimaryOccluderCandidates);
			}

			{
				PROFILE_ENTRY("HZB Occluder Depth CPU");
				GPU_PROFILE("HZB Occluder Depth");
				RHISetFrameBuffer(commandList, mOccluderFrameBuffer);
				RHISetViewport(commandList, 0, 0, mOccluderDepthTexture->getSizeX(), mOccluderDepthTexture->getSizeY());
				LinearColor clearColor(0, 0, 0, 0);
				RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &clearColor, 1);
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::DepthNear>::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());
				RHISetShaderProgram(commandList, mProgBlockRender->getRHI());
				contextCull.setupShader(commandList, *mProgBlockRender);
				auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBlockRender, BlockTexture, *mTexBlockAtlas, sampler);

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
					++chunkLayerHZBCount;
				}

				RHISetFrameBuffer(commandList, nullptr);
				RHIResourceTransition(commandList, { mOccluderDepthTexture }, EResourceTransition::SRV);
			}

			{
				{
					PROFILE_ENTRY("Generate HZB CPU");
					generateHZB(commandList, *mOccluderDepthTexture);
				}

				if (!visibleLayerItems.empty() && mHZBCullItemBuffer.isValid() && mHZBCullResultTexture.isValid())
				{
					{
						PROFILE_ENTRY("HZB Cull");
						int totalCmdCount = 0;
						for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
						{
							meshPoolCmdBaseOffsets[poolIndex] = 0;
							meshPoolCmdCounts[poolIndex] = 0;
						}

						for (auto const& visibleItem : visibleLayerItems)
						{
							int poolIndex = visibleItem.poolIndex;
							if (poolIndex < 0 || poolIndex >= (int)mMeshPool.size())
								continue;

							++meshPoolCmdCounts[poolIndex];
							++totalCmdCount;
						}

						if (totalCmdCount > MaxHZBCullItems)
						{
							LogWarning(0, "HZB cull item overflow: totalCmdCount=%d MaxHZBCullItems=%d", totalCmdCount, MaxHZBCullItems);
						}
						int testCount = Math::Min<int>(totalCmdCount, MaxHZBCullItems);
						if (testCount > 0)
						{
							TArray<HZBCullGPUItem> cullItems;
							{
								PROFILE_ENTRY("Prepare HZB Cull Command");
								int remainingCount = testCount;
								for (int poolIndex = 0; poolIndex < (int)meshPoolCmdCounts.size(); ++poolIndex)
								{
									meshPoolCmdBaseOffsets[poolIndex] = testCount - remainingCount;
									meshPoolCmdCounts[poolIndex] = Math::Min(meshPoolCmdCounts[poolIndex], remainingCount);
									remainingCount -= meshPoolCmdCounts[poolIndex];
								}

								cullItems.resize(testCount);
								hzbCullTriangleCounts.resize(testCount);
								hzbCullTestCount = testCount;

								TArray<int> writeOffsets;
								writeOffsets = meshPoolCmdBaseOffsets;

								for (auto const& visibleItem : visibleLayerItems)
								{
									int poolIndex = visibleItem.poolIndex;
									if (poolIndex < 0 || poolIndex >= (int)mMeshPool.size())
										continue;
									if (meshPoolCmdCounts[poolIndex] == 0)
										continue;

									int outputIndex = writeOffsets[poolIndex];
									int outputEnd = meshPoolCmdBaseOffsets[poolIndex] + meshPoolCmdCounts[poolIndex];
									if (outputIndex >= outputEnd)
										continue;

									auto& item = cullItems[outputIndex];
									item = visibleItem.cullItem;
									item.outputIndex = outputIndex;
									hzbCullTriangleCounts[outputIndex] = visibleItem.triangleCount;
									++writeOffsets[poolIndex];
								}
							}
							{
								PROFILE_ENTRY("HZB UpdateCullItemBuffer");
								RHIUpdateBuffer(commandList, *mHZBCullItemBuffer, 0, testCount, cullItems.data());
							}
							{
								PROFILE_ENTRY("HZB Cull Dispatch CPU");
								GPU_PROFILE("HZB Cull");

								auto* progHZBCull = ShaderManager::Get().getGlobalShaderT< HZBCullCS >();
								RHIResourceTransition(commandList, { mHZBTexture }, EResourceTransition::SRV);
								RHIResourceTransition(commandList, { mHZBCullResultTexture, mCmdBuildBuffer }, EResourceTransition::UAV);
								RHISetComputeShader(commandList, progHZBCull->getRHI());
								progHZBCull->setStorageBuffer(commandList, SHADER_PARAM(CullItems), *mHZBCullItemBuffer, EAccessOp::ReadOnly);
								progHZBCull->setTexture(commandList, SHADER_PARAM(HZBTexture), *mHZBTexture);
								progHZBCull->setStorageBuffer(commandList, SHADER_PARAM(VisibleCommands), *mCmdBuildBuffer, EAccessOp::ReadAndWrite);
								progHZBCull->setParam(commandList, SHADER_PARAM(WorldToClip), worldToClipCullRelativeRHI);
								progHZBCull->setParam(commandList, SHADER_PARAM(ViewSize), IntVector2(mOccluderDepthTexture->getSizeX(), mOccluderDepthTexture->getSizeY()));
								progHZBCull->setParam(commandList, SHADER_PARAM(ResultTextureSize), IntVector2(HZBCullResultTextureWidth, 1));
								progHZBCull->setParam(commandList, SHADER_PARAM(NumMipLevel), mHZBTexture->getDesc().numMipLevel);
								progHZBCull->setParam(commandList, SHADER_PARAM(NumItems), testCount);
								progHZBCull->setParam(commandList, SHADER_PARAM(DepthBias), 0.00001f);
								progHZBCull->setRWTexture(commandList, SHADER_PARAM(ResultTexture), *mHZBCullResultTexture, 0, EAccessOp::WriteOnly);
								RHIDispatchCompute(commandList, (testCount + 63) / 64, 1, 1);
								progHZBCull->clearRWTexture(commandList, SHADER_PARAM(ResultTexture));
								progHZBCull->clearBuffer(commandList, SHADER_PARAM(VisibleCommands), EAccessOp::ReadAndWrite);
								RHIResourceTransition(commandList, { mCmdBuildBuffer }, EResourceTransition::UAVBarrier);
								RHIResourceTransition(commandList, { mHZBCullResultTexture }, EResourceTransition::SRV);
							}

							{
								PROFILE_ENTRY("HZB Copy Visible Commands");
								RHICopyResource(commandList, *mCmdBuffer, *mCmdBuildBuffer);
							}
						}
					}
				}
			}

			{
				PROFILE_ENTRY("Setup Scene FrameBuffer");
				RHISetFrameBuffer(commandList, mSceneFrameBuffer);
				RHISetViewport(commandList, 0, 0, mViewSize.x, mViewSize.y);
				LinearColor clearColor(0, 0, 0, 1);
				RHIClearRenderTargets(commandList, EClearBits::All, &clearColor, 1);
			}

			// Front-to-Back Sorting: Closest first to maximize Early-Z culling
			{
				PROFILE_ENTRY("Sort Layer Draw List");
				std::sort(layerDrawList.begin(), layerDrawList.end(), [&](ChunkRenderData::Layer* a, ChunkRenderData::Layer* b)
				{
					float distA = (a->bound.getCenter() - camPos).length2();
					float distB = (b->bound.getCenter() - camPos).length2();
					return distA < distB;
				});
			}

			triangleCount = 0;
			{
				PROFILE_ENTRY("Setup Indirect Draw Offsets");
				for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
				{
					mMeshPool[poolIndex]->cmdOffset = sizeof(DrawCmdArgs) * meshPoolCmdBaseOffsets[poolIndex];
				}
			}

			bool bHaveIndirectDraw = !layerDrawList.empty();
			if (bHaveIndirectDraw)
			{
				// 3. Z-Prepass (Depth Only)
				if (!bShowChunkLayerBoundOverDraw && GRenderPreDepth)
				{
					PROFILE_ENTRY("Z-Prepass CPU");
					GPU_PROFILE("Z-Prepass");
					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
					RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::DepthNear>::GetRHI());
					RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());
					RHISetShaderProgram(commandList, mProgBlockRenderDepth->getRHI());
					contextRender.setupShader(commandList, *mProgBlockRenderDepth);

					for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
					{
						auto mesh = mMeshPool[poolIndex];
						int numCommands = meshPoolCmdCounts[poolIndex];
						if (numCommands == 0)
							continue;

						InputStreamInfo inputstream;
						inputstream.buffer = mesh->vertexBuffer;
						inputstream.offset = 0;
						inputstream.stride = mesh->vertexBuffer->getElementSize();
						RHISetInputStream(commandList, mBlockInputLayout, &inputstream, 1);
						RHISetIndexBuffer(commandList, mesh->indexBuffer);

						RHIDrawIndexedPrimitiveIndirect(commandList, EPrimitive::TriangleList, mCmdBuffer, mesh->cmdOffset, numCommands);
					}
				}

				// 4. Base Pass / Overdraw / Bound Overdraw
				{
					PROFILE_ENTRY("Base Pass CPU");
					GPU_PROFILE("Base Pass");
					chunkLayerCount = layerDrawList.size();

					if (bShowChunkLayerBoundOverDraw)
					{
						RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
						RHISetDepthStencilState(commandList, TStaticDepthStencilState<false, ECompareFunc::Always>::GetRHI());
						RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::One>::GetRHI());

						static uint8 FaceOrder[] = { FACE_X, FACE_NX, FACE_Y, FACE_NY, FACE_Z, FACE_NZ };
						for (auto* layer : layerDrawList)
						{
							if (layer == nullptr || !layer->bound.isValid())
								continue;

							contextRender.stack.push();
							contextRender.stack.translate(layer->bound.min);
							contextRender.stack.scale(layer->bound.getSize());
							contextRender.setupShader(commandList, LinearColor( 0.04, 0.04, 0.04, 1));

							for (uint8 face : FaceOrder)
							{
								TRenderRT<RTVF_XYZ>::Draw(commandList, EPrimitive::Quad, GetFaceVertices((FaceSide)face), 4);
							}
							contextRender.stack.pop();
						}
						chunkMeshCount = 0;
					}
					else
					{

						if (bShowOverdraw)
						{
							RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
							RHISetDepthStencilState(commandList, TStaticDepthStencilState<false, ECompareFunc::Always>::GetRHI());
							RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::One>::GetRHI());
							RHISetShaderProgram(commandList, mProgBlockRenderOverdraw->getRHI());
							contextRender.setupShader(commandList, *mProgBlockRenderOverdraw);
						}
						else if (GRenderPreDepth)
						{
							RHISetRasterizerState(commandList, bWireframeMode ? TStaticRasterizerState<ECullMode::Back, EFillMode::Wireframe >::GetRHI() : TStaticRasterizerState<ECullMode::Back>::GetRHI());
							RHISetDepthStencilState(commandList, TStaticDepthStencilState<false, ECompareFunc::DepthNearEqual>::GetRHI());
							RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
							RHISetShaderProgram(commandList, mProgBlockRender->getRHI());
							contextRender.setupShader(commandList, *mProgBlockRender);
							auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
							SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBlockRender, BlockTexture, *mTexBlockAtlas, sampler);
						}
						else
						{
							RHISetRasterizerState(commandList, bWireframeMode ? TStaticRasterizerState<ECullMode::Back, EFillMode::Wireframe >::GetRHI() : TStaticRasterizerState<ECullMode::Back>::GetRHI());
							RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::DepthNear>::GetRHI());
							RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
							RHISetShaderProgram(commandList, mProgBlockRender->getRHI());
							contextRender.setupShader(commandList, *mProgBlockRender);
							auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
							SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBlockRender, BlockTexture, *mTexBlockAtlas, sampler);
						}

						chunkMeshCount = 0; // Reset for accurate display
						for (int poolIndex = 0; poolIndex < (int)mMeshPool.size(); ++poolIndex)
						{
							auto mesh = mMeshPool[poolIndex];
							int numCommands = meshPoolCmdCounts[poolIndex];
							if (numCommands == 0) 
								continue;

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
			}

			if (mDebugCamera && hzbCullTestCount > 0)
			{
				PROFILE_ENTRY("HZB Readback");
				TArray<uint8> readbackData;
				{
					PROFILE_ENTRY("Read Texture");
					RHIReadTexture(*mHZBCullResultTexture, ETexture::RGBA8, 0, readbackData);
				}

				int const maxReadableItemCount = Math::Min<int>(hzbCullTestCount, readbackData.size() / sizeof(Color4ub));
				triangleCount = 0;
				chunkLayerCount = 0;
				for (int itemIndex = 0; itemIndex < maxReadableItemCount; ++itemIndex)
				{
					Color4ub const& color = *(reinterpret_cast<Color4ub const*>(readbackData.data()) + itemIndex);
					
					bool const bVisible = (color.r > 0);

					auto const& bound = visibleLayerItems[itemIndex].layer->bound;

					if (bVisible)
					{
						chunkLayerCount += 1;
						triangleCount += hzbCullTriangleCounts[itemIndex];
						if (mDebugCamera)
						{
							mDebugPrimitives.addCubeLine(bound.getCenter(), Quaternion::Identity(), bound.getSize(), LinearColor(RenderUtility::GetColor(EColor::Green)), 2);
						}
					}
					else
					{
						occludedTriangleCount += hzbCullTriangleCounts[itemIndex];
						if (itemIndex < visibleLayerItems.size() && visibleLayerItems[itemIndex].layer && visibleLayerItems[itemIndex].layer->bound.isValid())
						{
							if (mDebugCamera)
							{
								mDebugPrimitives.addCubeLine(bound.getCenter(), Quaternion::Identity(), bound.getSize(), LinearColor(RenderUtility::GetColor(EColor::Yellow)), 2);
							}
						}
					}
				}
			}
		}

#if 1
		BlockPosInfo info;
		BlockId id = world.rayBlockTest(camPos, camera.getViewDir(), 100, &info);
		if (id)
		{
			contextRender.stack.push();
			contextRender.stack.translate(Vector3(info.x, info.y, info.z) + 0.1 * GetFaceNoraml(info.face));
			contextRender.setupShader(commandList, LinearColor(1, 1, 1, 1));
			TRenderRT<RTVF_XYZ>::Draw(commandList, EPrimitive::LineLoop, GetFaceVertices(info.face), 4);
			contextRender.stack.pop();
		}
#endif

		{
			contextRender.stack.push();
			contextRender.stack.translate(Vector3(0, 10, 0));
			float len = 10;
			contextRender.setupShader(commandList);
			DrawUtility::AixsLine(commandList, len);
			contextRender.stack.pop();
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
		g.drawTextF(Vector2(10, 70), "MeshBuildTimeAcc = %lf, Tri Time = %lf", mMeshBuildTimeAcc, GTriTime);
		g.drawTextF(Vector2(10, 85), "chunkMeshCount = %d, chunkLayerCount = %d, chunkLayerHZBCount = %d", chunkMeshCount, chunkLayerCount, chunkLayerHZBCount);

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
		int const localX = bx & ChunkMask;
		int const localY = by & ChunkMask;
		int const localZ = bz & Chunk::LayerMask;
		int const layerIndex = bz >> Chunk::LayerBitCount;
		uint32 const currentLayerBit = uint32(1) << layerIndex;

		uint32 layerMask = currentLayerBit;
		if (localZ == 0 && layerIndex > 0)
		{
			layerMask |= (uint32(1) << (layerIndex - 1));
		}
		if (localZ == Chunk::LayerMask && layerIndex + 1 < Chunk::NumLayer)
		{
			layerMask |= (uint32(1) << (layerIndex + 1));
		}

		markChunkDirty(cPos, layerMask);

		if (localX == ChunkMask)
		{
			cPos.setBlockPos(bx + 1, by);
			markChunkDirty(cPos, currentLayerBit);
		}
		if (localX == 0)
		{
			cPos.setBlockPos(bx - 1, by);
			markChunkDirty(cPos, currentLayerBit);
		}
		if (localY == ChunkMask)
		{
			cPos.setBlockPos(bx, by + 1);
			markChunkDirty(cPos, currentLayerBit);
		}
		if (localY == 0)
		{
			cPos.setBlockPos(bx, by - 1);
			markChunkDirty(cPos, currentLayerBit);
		}
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
