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
		using PermutationDomain = TShaderPermutationDomain<DepthPass>;

		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
#if 0
			if (domain.get< DepthPass >())
			{
				static ShaderEntryInfo const entries[] =
				{
					{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				};
				return entries;
			}
#endif

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



	bool GPreRenderDepth = true;

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

		mTexBlockAtlas = RHIUtility::LoadTexture2DFromFile("Cube/blocks.png", TextureLoadOption().AutoMipMap());

		InputLayoutDesc desc;
		desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::UByte4, true);
		desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD2, EVertex::UInt1);
		desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		mBlockInputLayout = RHICreateInputLayout(desc);

		mCmdBuffer = RHICreateBuffer(sizeof(DrawCmdArgs), 4096 * 16, BCF_DrawIndirectArgs | BCF_CreateSRV);

		mSceneFrameBuffer = RHICreateFrameBuffer();
		resizeRenderTarget();

		return true;
	}

	void RenderEngine::releaseRHI()
	{
		mDebugPrimitives.releaseRHI();
		mProgBlockRender = nullptr;
		mTexBlockAtlas.release();
		mBlockInputLayout.release();
		mCmdBuffer.release();
		mSceneFrameBuffer.release();
		mSceneTexture.release();
		mSceneDepthTexture.release();
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

	void RenderEngine::resizeRenderTarget()
	{
		int numSamples = 1;
		mSceneTexture = RHICreateTexture2D(ETexture::FloatRGBA, mViewSize.x, mViewSize.y, 1, numSamples, TCF_DefalutValue | TCF_RenderTarget);
		mSceneTexture->setDebugName("SceneTexture");
		mSceneDepthTexture = RHICreateTexture2D(ETexture::D32FS8, mViewSize.x, mViewSize.y, 1, numSamples, TCF_CreateSRV);
		mSceneFrameBuffer->setTexture(0, *mSceneTexture);
		mSceneFrameBuffer->setDepth(*mSceneDepthTexture);

		mHZBTexture = RHICreateTexture2D(ETexture::R32F, mViewSize.x, mViewSize.y, 4, numSamples, TCF_CreateSRV | TCF_CreateUAV);

		GTextureShowManager.registerTexture("SceneTexture", mSceneTexture);
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
			SET_SHADER_PARAM_VALUE(commandList, shader, LocalToWorld, stack.get());
			SET_SHADER_PARAM_VALUE(commandList, shader, WorldToClip, worldToClip);

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

		RHISetFrameBuffer(commandList, mSceneFrameBuffer);
		RHISetViewport(commandList, 0, 0, mViewSize.x, mViewSize.y);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1);

		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetRasterizerState(commandList, GetStaticRasterizerState(ECullMode::Back, bWireframeMode ? EFillMode::Wireframe : EFillMode::Solid));

		Vec3f camPos = camera.getPos();
		Vec3f viewPos = camPos + camera.getViewDir();
		Vec3f upDir = camera.getUpDir();

		ICamera* cameraRender = mDebugCamera ? mDebugCamera : &camera;
		Matrix4 projectMatrix = PerspectiveMatrix(Math::DegToRad(100.0f / mAspect), mAspect, 0.01, 2000);
		Matrix4 worldToView = LookAtMatrix(camera.getPos(), camera.getViewDir(), camera.getUpDir());
		Matrix4 worldToClip = worldToView * projectMatrix;

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

		{
			context.stack.push();
			context.stack.translate(Vector3(0, 10, 0));
			float len = 10;
			context.setupShader(commandList);
			DrawUtility::AixsLine(commandList, len);
			context.stack.pop();
		}

		int bx = Math::FloorToInt(camPos.x);
		int by = Math::FloorToInt(camPos.y);

		ChunkPos cPos;
		cPos.setBlockPos(bx, by);
		int viewDist = 20;

		int chunkRenderCount = 0;
		int triangleCount = 0;





		TArray<ChunkRenderData::Layer*> layerDrawList;
		int64 visTriangleCount = 0;
		int64 occludedTriangleCount = 0;
		struct OccluderInfo
		{
			Math::TAABBox<Vec3f> box;
			float minX, minY, maxX, maxY, minZ, maxZ;
		};
		TArray<OccluderInfo> activeOccluders;

		auto ProjectBox = [&](Math::TAABBox<Vec3f> const& box, float& minX, float& minY, float& maxX, float& maxY, float& minZ, float& maxZ)
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

			minX = minY = minZ = 2.0f; maxX = maxY = maxZ = -2.0f;
			for (int i = 0; i < 8; ++i)
			{
				Vector4 clip = Vector4(v[i], 1.0f) * worldToClipRender;
				if (clip.w <= 0.001f) return false; // Conservative: If any point is behind or on near plane, don't use as occluder

				Vector3 ndc = clip.dividedVector();
				minX = Math::Min(minX, ndc.x); maxX = Math::Max(maxX, ndc.x);
				minY = Math::Min(minY, ndc.y); maxY = Math::Max(maxY, ndc.y);
				minZ = Math::Min(minZ, ndc.z); maxZ = Math::Max(maxZ, ndc.z);
			}
			return (maxX > minX && maxY > minY);
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
				data->state = ChunkRenderData::eNone;
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

			// Optimization: Chunk-Level Occlusion Culling
			{
				float cMinX, cMinY, cMaxX, cMaxY, cMinZ, cMaxZ;
				if (ProjectBox(data->bound, cMinX, cMinY, cMaxX, cMaxY, cMinZ, cMaxZ))
				{
					int testCount = (int)activeOccluders.size();
					for (int i = 0; i < testCount; ++i)
					{
						auto const& occluder = activeOccluders[i];
						if (cMinZ > occluder.maxZ - 0.001f && // Added epsilon
							cMinX >= occluder.minX && cMaxX <= occluder.maxX &&
							cMinY >= occluder.minY && cMaxY <= occluder.maxY)
						{
							// Statistics: Count all triangles in this chunk as occluded
							for (int k = 0; k < data->numLayer; ++k)
							{
								if (data->layers[k].meshPool)
									occludedTriangleCount += (data->layers[k].args.indexCountPerInstance / 3) * data->layers[k].args.instanceCount;
							}
							return; // Skip entire chunk
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

				if (distSq < 128.0f * 128.0f && layer.occluderBox.isValid())
				{
					float oMinX, oMinY, oMaxX, oMaxY, oMinZ, oMaxZ;
					if (ProjectBox(layer.occluderBox, oMinX, oMinY, oMaxX, oMaxY, oMinZ, oMaxZ))
					{
						float area = (oMaxX - oMinX) * (oMaxY - oMinY);
						if (area > 0.02f) // Lowered to 2% to catch more distant but important hills
						{
							OccluderInfo info = { layer.occluderBox, oMinX, oMinY, oMaxX, oMaxY, oMinZ, oMaxZ };
							if (activeOccluders.size() < 32) // Increased to 32
							{
								activeOccluders.push_back(info);
							}
							else
							{
								// Replace the one that is FURTHEST away, as closer occluders are better
								int worstIdx = -1;
								float maxZ = -2.0f;
								for (int k = 0; k < 32; ++k) {
									if (activeOccluders[k].maxZ > maxZ) {
										maxZ = activeOccluders[k].maxZ;
										worstIdx = k;
									}
								}
								if (oMaxZ < maxZ) activeOccluders[worstIdx] = info;
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
			// Front-to-Back Sorting: Closest first to maximize Early-Z culling
			std::sort(layerDrawList.begin(), layerDrawList.end(), [&](ChunkRenderData::Layer* a, ChunkRenderData::Layer* b)
			{
				float distA = (a->bound.getCenter() - camPos).length2();
				float distB = (b->bound.getCenter() - camPos).length2();
				return distA < distB;
			});

			triangleCount = 0;
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

			drawCmdList.clear();
			drawCmdList.reserve(2048); // Pre-reserve to kill the 17ms MergeTimeAcc
			for (auto mesh : mMeshPool)
			{
				if (mesh->drawFrame != mRenderFrame)
					continue;

				mesh->cmdOffset = sizeof(DrawCmdArgs) * (uint32)drawCmdList.size();
				drawCmdList.append(mesh->drawCmdList);
			}

			if (!drawCmdList.empty())
			{
				RHIUpdateBuffer(*mCmdBuffer, 0, (uint32)drawCmdList.size(), drawCmdList.data());

				// 3. Z-Prepass (Depth Only)
				{
					GPU_PROFILE("Z-Prepass");
					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
					RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::Less>::GetRHI());
					RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());
					RHISetShaderProgram(commandList, mProgBlockRenderDepth->getRHI());
					context.setupShader(commandList, *mProgBlockRenderDepth);

					for (auto mesh : mMeshPool)
					{
						if (mesh->drawCmdList.empty()) continue;

						InputStreamInfo inputstream;
						inputstream.buffer = mesh->vertexBuffer;
						inputstream.offset = 0;
						inputstream.stride = mesh->vertexBuffer->getElementSize();
						RHISetInputStream(commandList, mBlockInputLayout, &inputstream, 1);
						RHISetIndexBuffer(commandList, mesh->indexBuffer);

						RHIDrawIndexedPrimitiveIndirect(commandList, EPrimitive::TriangleList, mCmdBuffer, mesh->cmdOffset, (uint32)mesh->drawCmdList.size());
					}
				}

				// 4. Base Pass (Color + Lighting)
				{
					GPU_PROFILE("Base Pass");
					chunkLayerCount = (int)drawCmdList.size();
					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Back>::GetRHI());
					RHISetDepthStencilState(commandList, TStaticDepthStencilState<false, ECompareFunc::LessEqual>::GetRHI());
					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
					RHISetShaderProgram(commandList, mProgBlockRender->getRHI());
					context.setupShader(commandList, *mProgBlockRender);
					SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBlockRender, BlockTexture, *mTexBlockAtlas, TStaticSamplerState<ESampler::Bilinear>::GetRHI());

					chunkMeshCount = 0; // Reset for accurate display
					for (auto mesh : mMeshPool)
					{
						if (mesh->drawFrame != mRenderFrame) continue;
						if (mesh->drawCmdList.empty()) continue;

						InputStreamInfo inputstream;
						inputstream.buffer = mesh->vertexBuffer;
						inputstream.offset = 0;
						inputstream.stride = mesh->vertexBuffer->getElementSize();
						RHISetInputStream(commandList, mBlockInputLayout, &inputstream, 1);
						RHISetIndexBuffer(commandList, mesh->indexBuffer);

						RHIDrawIndexedPrimitiveIndirect(commandList, EPrimitive::TriangleList, mCmdBuffer, mesh->cmdOffset, (uint32)mesh->drawCmdList.size());
						++chunkMeshCount;
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
			DrawUtility::DrawDepthTexture(commandList, baseTransform, *mSceneDepthTexture, TStaticSamplerState<>::GetRHI(), Vector2(200, 200), Vector2(400, 200), 0, 100);
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
		{
			cPos.setBlockPos(bx + 1, by);
			ChunkDataMap::iterator iter = mChunkMap.find(cPos.hash_value());
			if (iter != mChunkMap.end())
				iter->second->bNeedUpdate = true;
		}
		{
			cPos.setBlockPos(bx - 1, by);
			ChunkDataMap::iterator iter = mChunkMap.find(cPos.hash_value());
			if (iter != mChunkMap.end())
				iter->second->bNeedUpdate = true;
		}
		{
			cPos.setBlockPos(bx, by + 1);
			ChunkDataMap::iterator iter = mChunkMap.find(cPos.hash_value());
			if (iter != mChunkMap.end())
				iter->second->bNeedUpdate = true;
		}
		{
			cPos.setBlockPos(bx, by - 1);
			ChunkDataMap::iterator iter = mChunkMap.find(cPos.hash_value());
			if (iter != mChunkMap.end())
				iter->second->bNeedUpdate = true;
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