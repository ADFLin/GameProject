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

namespace Cube
{
	using namespace Render;

	class BlockRenderShaderProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(BlockRenderShaderProgram, Global);

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
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




	RenderEngine::RenderEngine( int w , int h )
	{
		mClientWorld = NULL;
		mAspect        = float( w ) / h ;


		mRenderWidth  = 0;
		mRenderHeight = ChunkBlockMaxHeight;
		mRenderDepth  = 0;

		mGereatePool = new QueueThreadPool;
		mGereatePool->init(1);
	}

	RenderEngine::~RenderEngine()
	{
		cleanupRenderData();
		delete mGereatePool;
	}

	bool RenderEngine::initializeRHI()
	{
		mDebugPrimitives.initializeRHI();
		mProgBlockRender = ShaderManager::Get().getGlobalShaderT< BlockRenderShaderProgram >();

		mTexBlockAtlas = RHIUtility::LoadTexture2DFromFile("Cube/blocks.png");

		InputLayoutDesc desc;
		desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
		desc.addElement(0, EVertex::ATTRIBUTE_COLOR, EVertex::UByte4, true);
		desc.addElement(0, EVertex::ATTRIBUTE_NORMAL, EVertex::Float3);
		desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD2, EVertex::UInt1);
		desc.addElement(0, EVertex::ATTRIBUTE_TEXCOORD, EVertex::Float2);
		mBlockInputLayout = RHICreateInputLayout(desc);

		mCmdBuffer = RHICreateBuffer(sizeof(DrawCmdArgs), 4096 * 16, BCF_DrawIndirectArgs);

		return true;
	}

	void RenderEngine::releaseRHI()
	{
		mDebugPrimitives.releaseRHI();
		mProgBlockRender = nullptr;
		mTexBlockAtlas.release();
		mBlockInputLayout.release();
		mCmdBuffer.release();
	}

	void RenderEngine::setupWorld(World& world)
	{
		if ( mClientWorld )
		{
			mClientWorld->removeListener( *this );
			mClientWorld->mChunkProvider->mListener = nullptr;
		}

		mClientWorld = &world;
		mClientWorld->mChunkProvider->mListener = this;
		mClientWorld->addListener( *this );
	}


	void RenderEngine::tick(float deltaTime)
	{
		{
			Mutex::Locker locker(mMutexPedingAdd);
			for (auto const& updateData : mPendingAddList)
			{
				auto data = updateData.chunkData;
				auto& mesh = updateData.mesh;
				using namespace Render;
				auto meshData = acquireMeshRenderData(mesh);

				if (meshData == nullptr)
					continue;

				data->meshPool = meshData;
				data->indicesCount = mesh.mIndices.size();
				meshData->vertexAllocator.alloc(mesh.mVertices.size(), 0, data->vertexAllocation);
				meshData->indexAllocator.alloc(mesh.mIndices.size(), 0, data->indexAlloction);
				RHIUpdateBuffer(*meshData->vertexBuffer, data->vertexAllocation.pos, mesh.mVertices.size(), (void*)mesh.mVertices.data());
				RHIUpdateBuffer(*meshData->indexBuffer, data->indexAlloction.pos, mesh.mIndices.size(), (void*)mesh.mIndices.data());


				//data->posOffset = ChunkSize * Vec3f(data->chunk->getPos() , 0.0);
				data->state = ChunkRenderData::eMesh;
				data->bound = updateData.bound;
				//data->bound.translate(data->posOffset);

			}
			mPendingAddList.clear();
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


	class NeighborChunkAccess : public IBlockAccess
	{
	public:
		NeighborChunkAccess(World& world, Chunk* chunk)
		{
			mChunk = chunk;
			for (int i = 0; i < 4; ++i)
			{
				Vec3i offset = GetFaceOffset(FaceSide(i));
				mNeighborChunks[i] = world.getChunk(ChunkPos(chunk->getPos() + Vec2i(offset.x, offset.y)), true);
			}

			mChunkOffset = ChunkSize * chunk->getPos();
		}

		Chunk* getChunk(int x, int y)
		{
			Vec2i offset = Vec2i(x, y) - mChunkOffset;
			if (offset.x >= ChunkSize)
			{
				return mNeighborChunks[FaceSide::FACE_X];
			}
			if (offset.x < 0)
			{
				return mNeighborChunks[FaceSide::FACE_NX];
			}
			if (offset.y >= ChunkSize)
			{
				return mNeighborChunks[FaceSide::FACE_Y];
			}
			if (offset.y < 0)
			{
				return mNeighborChunks[FaceSide::FACE_NY];
			}
			return mChunk;
		}

		bool IsInRange(Chunk* chunk, int x, int y)
		{
			Vec2i offset = Vec2i(x, y) - ChunkSize * chunk->getPos();
			return 0 <= offset.x && offset.x < ChunkSize &&
				0 <= offset.y && offset.y < ChunkSize;
		}

		virtual BlockId  getBlockId(int x, int y, int z) final
		{
			if (x == -17 && y == 0)
			{

				int aa = 1;
			}
			auto chunk = getChunk(x, y);
			if (chunk == nullptr)
				return BLOCK_NULL;


			CHECK(IsInRange(chunk, x, y));

			return chunk->getBlockId(x, y, z);
		}
		virtual MetaType getBlockMeta(int x, int y, int z)
		{
			auto chunk = getChunk(x, y);
			if (chunk == nullptr)
				return 0;
			CHECK(IsInRange(chunk, x, y));
			return chunk->getBlockMeta(x, y, z);
		}

		void  getNeighborBlockIds(Vec3i const& pos, BlockId outIds[])
		{
			for (int i = 0; i < FaceSide::COUNT; ++i)
			{
				Vec3i nPos = pos + GetFaceOffset(FaceSide(i));
				outIds[i] = getBlockId(nPos.x, nPos.y, nPos.z);
			}
		}


		Vec2i  mChunkOffset;

		Chunk* mChunk;
		Chunk* mNeighborChunks[4];
	};

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

				renderer.mMesh = &updateData.mesh;


				renderer.mDebugColor = RenderUtility::GetColor(ColorMap[(chunk->getPos().x + chunk->getPos().y) % ARRAY_SIZE(ColorMap)]);
				renderer.mBlockAccess = const_cast<NeighborChunkAccess*>(&chunkAccess);
				chunk->render(renderer);
				{
					TIME_SCOPE(mMergeTimeAcc);
					renderer.finalizeMesh();
				}
				updateData.bound = renderer.bound;
				{
					Mutex::Locker locker(mMutexPedingAdd);
					mPendingAddList.push_back(std::move(updateData));
				}
			}
		);
	}

	MeshRenderPoolData* RenderEngine::acquireMeshRenderData(Mesh const& mesh)
	{
		for (auto meshData : mMeshPool)
		{
			if (!meshData->vertexAllocator.canAllocate(mesh.mVertices.size(), 0))
				continue;
			if (!meshData->indexAllocator.canAllocate(mesh.mIndices.size(), 0))
				continue;
			return meshData;
		}


		auto meshData = new MeshRenderPoolData;
		meshData->initialize();
		mMeshPool.push_back(meshData);
		return meshData;
	}

	ACCESS_SHADER_MEMBER_PARAM(LocalToWorld);
	ACCESS_SHADER_MEMBER_PARAM(WorldToClip);

	double GTriTime = 0.0;
	struct RnederContext
	{

		Matrix4 worldToClip;
		TransformStack stack;


		template< typename TShader >
		void setupShader(RHICommandList& commandList, TShader& shader)
		{
			SET_SHADER_PARAM_VALUE(commandList, shader, LocalToWorld, stack.get());
			SET_SHADER_PARAM_VALUE(commandList, shader, WorldToClip, worldToClip);
			
		}

		void setupShader(RHICommandList& commandList, LinearColor const& color = LinearColor(1,1,1,1))
		{
			RHISetFixedShaderPipelineState(commandList, stack.get() * worldToClip, color);
		}
	};

	using Math::Plane;

	static void GetFrustomPlanes(Vector3 const& worldPos, Vector3 const& viewDir,  Matrix4 const& clipToWorld, Plane outPlanes[])
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


	void RenderEngine::renderWorld( ICamera& camera )
	{
		GPU_PROFILE("Render World");

		++mRenderFrame;

		World& world = *mClientWorld;

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0,0,0,1), 1);

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
			mDebugPrimitives.addCubeLine(camPos, Quaternion{ BasisMaterix::FromZY(camera.getViewDir(), camera.getUpDir()) }, Vec3f(0.5,0.5,0.5), LinearColor(1, 0, 0, 1), 4);


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
			context.stack.translate(Vector3(0,10,0));
			float len = 10;
			context.setupShader(commandList);
			DrawUtility::AixsLine(commandList, len);
			context.stack.pop();
		}

		int bx = Math::FloorToInt( camPos.x );
		int by = Math::FloorToInt( camPos.y );

		ChunkPos cPos;
		cPos.setBlockPos( bx , by );
		int viewDist = 100;

		int chunkRenderCount = 0;
		int triangleCount = 0;

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


			int visibilityType = FViewUtils::IsVisible(clipPlanes, data->bound);
			if ( mDebugCamera )
			{
				int const ColorMap[] =
				{
					EColor::Red,
					EColor::Green,
					EColor::Blue,
				};

				mDebugPrimitives.addCubeLine(data->bound.getCenter(), Quaternion::Identity(), data->bound.getSize(), LinearColor(RenderUtility::GetColor(ColorMap[visibilityType])), 2);
			}

			if (visibilityType == 0)
				return;

			if ( data->meshPool->drawFrame != mRenderFrame )
			{
				data->meshPool->drawCmdList.clear();
				data->meshPool->drawFrame = mRenderFrame;
			}

			DrawCmdArgs args;
			args.baseVertexLocation = data->vertexAllocation.pos;
			args.startIndexLocation = data->indexAlloction.pos;
			args.indexCountPerInstance = data->indicesCount;
			args.instanceCount = 1;
			args.startInstanceLocation = 0;
			data->meshPool->drawCmdList.push_back(args);

			triangleCount += data->indicesCount / 3;
			++chunkRenderCount;
		};


		int chunkMeshCount = 0;
		{
			PROFILE_ENTRY("Render Chunks");

			RHISetShaderProgram(commandList, mProgBlockRender->getRHI());

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
			for (auto mesh : mMeshPool)
			{
				if (mesh->drawFrame != mRenderFrame)
					continue;

				mesh->cmdOffset = sizeof(DrawCmdArgs) * drawCmdList.size();
				drawCmdList.append(mesh->drawCmdList);
			}

			if ( !drawCmdList.empty() )
			{
				RHIUpdateBuffer(*mCmdBuffer, 0, drawCmdList.size(), drawCmdList.data());

				context.setupShader(commandList, *mProgBlockRender);
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBlockRender, BlockTexture, *mTexBlockAtlas, TStaticSamplerState<>::GetRHI());
				for (auto mesh : mMeshPool)
				{
					if (mesh->drawFrame != mRenderFrame)
						continue;

					InputStreamInfo inputstream;
					inputstream.buffer = mesh->vertexBuffer;
					inputstream.offset = 0;
					inputstream.stride = mesh->vertexBuffer->getElementSize();

					RHISetInputStream(commandList, mBlockInputLayout, &inputstream, 1);
					RHISetIndexBuffer(commandList, mesh->indexBuffer);

					RHIDrawIndexedPrimitiveIndirect(commandList, EPrimitive::TriangleList, mCmdBuffer, mesh->cmdOffset, mesh->drawCmdList.size());
					++chunkMeshCount;
				}
			}
		}

#if 1
		BlockPosInfo info;
		BlockId id = world.rayBlockTest( camPos , camera.getViewDir() , 100 , &info );
		if ( id )
		{
			context.stack.push();
			context.stack.translate(Vector3(info.x, info.y, info.z) + 0.1 * GetFaceNoraml(info.face));
			context.setupShader(commandList, LinearColor(1,1,1,1));
			TRenderRT<RTVF_XYZ>::Draw(commandList, EPrimitive::LineLoop, GetFaceVertices(info.face), 4);
			context.stack.pop();
		}
#endif

		{
			mDebugPrimitives.drawDynamic(commandList, ::Global::GetScreenSize(), worldToClipRender, cameraRender->getViewDir().cross(cameraRender->getUpDir()), cameraRender->getUpDir());
		}

		mDebugPrimitives.clear();



		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();
		g.drawTextF(Vector2(10, 10), "chunkCount = %d, chunkRenderCount = %d", mChunkMap.size(), chunkRenderCount);
		g.drawTextF(Vector2(10, 25), "triangleCount = %d", triangleCount);
		g.drawTextF(Vector2(10, 40), "MergeTimeAcc = %lf, Tri Time = %lf", mMergeTimeAcc, GTriTime);
		g.drawTextF(Vector2(10, 55), "chunkMeshCount = %d", chunkMeshCount);
		g.endRender();

	}

	void RenderEngine::beginRender()
	{
	}

	void RenderEngine::endRender()
	{

	}

	void RenderEngine::onModifyBlock( int bx , int by , int bz )
	{
		ChunkPos cPos; 
		{
			cPos.setBlockPos( bx + 1 , by );
			ChunkDataMap::iterator iter = mChunkMap.find( cPos.hash_value() );
			if ( iter != mChunkMap.end() )
				iter->second->bNeedUpdate = true;
		}
		{
			cPos.setBlockPos( bx - 1 , by );
			ChunkDataMap::iterator iter = mChunkMap.find( cPos.hash_value() );
			if ( iter != mChunkMap.end() )
				iter->second->bNeedUpdate = true;
		}
		{
			cPos.setBlockPos( bx , by + 1 );
			ChunkDataMap::iterator iter = mChunkMap.find( cPos.hash_value() );
			if ( iter != mChunkMap.end() )
				iter->second->bNeedUpdate = true;
		}
		{
			cPos.setBlockPos( bx , by - 1 );
			ChunkDataMap::iterator iter = mChunkMap.find( cPos.hash_value() );
			if ( iter != mChunkMap.end() )
				iter->second->bNeedUpdate = true;
		}
	}

	bool MeshRenderPoolData::initialize()
	{
		uint32 MaxVerticesCount = 131072 * 64;
		uint32 MaxIndicesCount = FBitUtility::NextNumberOfPow2(6 * (MaxVerticesCount / 4));
		vertexBuffer = RHICreateVertexBuffer(sizeof(Mesh::Vertex), MaxVerticesCount, BCF_DefalutValue);
		indexBuffer = RHICreateIndexBuffer(MaxIndicesCount, true, BCF_DefalutValue);
		vertexAllocator.initialize(MaxVerticesCount, 32);
		indexAllocator.initialize(MaxIndicesCount, 64);

		return true;
	}

}//namespace Cube