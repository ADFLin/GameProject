#include "CubePCH.h"
#include "CubeRenderEngine.h"

#include "CubeBlockRenderer.h"

#include "CubeWorld.h"

#include "WindowsHeader.h"
#include "OpenGLHeader.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/SimpleRenderState.h"

#include "Async/AsyncWork.h"
#include "RenderUtility.h"

namespace Cube
{
	using namespace Render;

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

	void RenderEngine::setupWorld(World& world)
	{
		if ( mClientWorld )
			mClientWorld->removeListener( *this );

		mClientWorld = &world;
		mClientWorld->mChunkProvider->mListener = this;
		mClientWorld->addListener( *this );
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
				if (data->state != EDataState::Unintialized)
				{



				}

			}
		}

	}

	struct RnederContext
	{

		Matrix4 worldToClip;
		TransformStack stack;

		void setupShader(RHICommandList& commandList, LinearColor const& color = LinearColor(1,1,1,1))
		{
			RHISetFixedShaderPipelineState(commandList, stack.get() * worldToClip, color);
		}
	};



	void RenderEngine::renderWorld( ICamera& camera )
	{
		GPU_PROFILE("Render World");

		World& world = *mClientWorld;

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0,0,0,1), 1);

		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::Back , EFillMode::Wireframe >::GetRHI());

		Vec3f camPos = camera.getPos();
		Vec3f viewPos = camPos + camera.getViewDir();
		Vec3f upDir = camera.getUpDir();


		Matrix4 worldToView = LookAtMatrix(camera.getPos(), camera.getViewDir(), camera.getUpDir());
		Matrix4 projectMatrix = PerspectiveMatrixZBuffer(Math::DegToRad(100.0f / mAspect), mAspect, 0.01, 1000);

		RnederContext context;
		context.worldToClip = AdjProjectionMatrixForRHI(worldToView * projectMatrix);

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
		int len = 10;

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
				data->state = EDataState::Unintialized;
				mChunkMap.insert(std::make_pair(chunkPos.hash_value(), data));
				RangeChunkAccess chunkAccess(*mClientWorld, chunkPos);
				mGereatePool->addFunctionWork(
					[this, chunk, data, chunkAccess]()
					{
						data->state = EDataState::Generating;
						data->mesh.clearBuffer();

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
						renderer.mDebugColor = RenderUtility::GetColor(ColorMap[(chunk->getPos().x + chunk->getPos().y) % ARRAY_SIZE(ColorMap)]);
						renderer.mBlockAccess = const_cast<RangeChunkAccess*>(&chunkAccess);
						renderer.mMesh = &data->mesh;
						chunk->render(renderer);
						data->state = EDataState::Ok;
					}
				);
			}
			else
			{
				data = iter->second;
			}

			if (data && data->state == EDataState::Ok)
			{
				context.stack.push();
				int bx = ChunkSize * chunkPos.x;
				int by = ChunkSize * chunkPos.y;
				context.stack.translate(Vector3(bx, by, 0));
				context.setupShader(commandList);
				TRenderRT<RTVF_XYZ_CA8>::DrawIndexed(commandList, EPrimitive::TriangleList, data->mesh.mVtxBuffer.data(), data->mesh.mVtxBuffer.size(), data->mesh.mIndexBuffer.data(), data->mesh.mIndexBuffer.size());
				context.stack.pop();
			}
		};

		ProcessChunk(cPos);

		for( int n = 1 ; n <= len ; ++n )
		{
			for (int i = -(n - 1); i <= (n - 1); ++i)
			{
				ProcessChunk(cPos + Vec2i(i, n));
				ProcessChunk(cPos + Vec2i(i, -n));
				ProcessChunk(cPos + Vec2i(n, i));
				ProcessChunk(cPos + Vec2i(-n, i));
			}

			ProcessChunk(cPos + Vec2i( n,  n));
			ProcessChunk(cPos + Vec2i(-n,  n));
			ProcessChunk(cPos + Vec2i( n, -n));
			ProcessChunk(cPos + Vec2i(-n, -n));
		}

#if 1
		BlockPosInfo info;
		BlockId id = world.rayBlockTest( camPos , camera.getViewDir() , 100 , &info );
		if ( id )
		{
			context.stack.push();
			context.stack.translate(Vector3(info.x, info.y, info.z) + 0.1 * Vec3f(GetFaceOffset(info.face)));

			static Vector3 FaceVertexMap[FaceSide::COUNT][4] =
			{
				{ Vector3(1 , 0 , 0), Vector3(1 , 1 , 0), Vector3(1 , 1 , 1), Vector3(1 , 0 , 1) },
				{ Vector3(0 , 0 , 0), Vector3(0 , 1 , 0), Vector3(0 , 1 , 1), Vector3(0 , 0 , 1) },
				{ Vector3(0 , 1 , 0), Vector3(1 , 1 , 0), Vector3(1 , 1 , 1), Vector3(0 , 1 , 1) },
				{ Vector3(0 , 0 , 0), Vector3(1 , 0 , 0), Vector3(1 , 0 , 1), Vector3(0 , 0 , 1) },
				{ Vector3(0 , 0 , 1), Vector3(1 , 0 , 1), Vector3(1 , 1 , 1), Vector3(0 , 1 , 1) },
				{ Vector3(0 , 0 , 0), Vector3(1 , 0 , 0), Vector3(1 , 1 , 0), Vector3(0 , 1 , 0) },
			};
			context.setupShader(commandList, LinearColor(1,1,1,1));
			TRenderRT<RTVF_XYZ>::Draw(commandList, EPrimitive::LineLoop, FaceVertexMap[info.face], 4);
			context.stack.pop();
		}
#endif
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
				iter->second->needUpdate = true;
		}
		{
			cPos.setBlockPos( bx - 1 , by );
			ChunkDataMap::iterator iter = mChunkMap.find( cPos.hash_value() );
			if ( iter != mChunkMap.end() )
				iter->second->needUpdate = true;
		}
		{
			cPos.setBlockPos( bx , by + 1 );
			ChunkDataMap::iterator iter = mChunkMap.find( cPos.hash_value() );
			if ( iter != mChunkMap.end() )
				iter->second->needUpdate = true;
		}
		{
			cPos.setBlockPos( bx , by - 1 );
			ChunkDataMap::iterator iter = mChunkMap.find( cPos.hash_value() );
			if ( iter != mChunkMap.end() )
				iter->second->needUpdate = true;
		}
	}


}//namespace Cube