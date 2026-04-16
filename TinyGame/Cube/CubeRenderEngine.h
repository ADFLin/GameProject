#ifndef CubeRenderEngine_h__
#define CubeRenderEngine_h__

#include "CubeBase.h"
#include "CubeMesh.h"
#include "CubeWorld.h"

#include "IWorldEventListener.h"
#include "RHI/PrimitiveBatch.h"
#include "Async/AsyncWork.h"
#include "Math/GeometryPrimitive.h"

#include <unordered_map>
#include "Memory/BuddyAllocator.h"


namespace Cube
{
	class BlockRenderer;
	class EntityRenderer;


	class ICamera
	{
	public:
		virtual Vec3f getPos() = 0;
		virtual Vec3f getViewDir() = 0;
		virtual Vec3f getUpDir() = 0;
	};


	class Texture2D
	{

		void release();
	};

	class RenderDevice
	{
		Texture2D* createEmptyTexture();
		Texture2D* createTexture( int w , int h , void* data );
	};

	struct MeshRenderData
	{
		Render::RHIBufferRef vertexBuffer;
		Render::RHIBufferRef indexBuffer;
	};

	struct DrawCmdArgs
	{
		uint indexCountPerInstance;
		uint instanceCount;
		uint startIndexLocation;
		int  baseVertexLocation;
		uint startInstanceLocation;
	};

	struct MeshRenderPoolData : MeshRenderData
	{
		BuddyAllocatorBase vertexAllocator;
		BuddyAllocatorBase indexAllocator;
		int poolIndex = -1;

		bool initialize();

		int drawFrame = 0;
		TArray< DrawCmdArgs > drawCmdList;
		int cmdOffset;
	};

	using AABBox = Math::TAABBox< Vec3f >;
	struct ChunkRenderData
	{
		ChunkRenderData()
			: state(eNone)
			, bNeedUpdate(false)
			, chunk(nullptr)
			, numLayer(0)
		{
			bound.invalidate();
		}

		enum EState
		{
			eNone,
			eMeshGenerating,
			eMesh,
		};


		EState   state;
		bool     bNeedUpdate = false;
		Chunk*   chunk;
		AABBox   bound;
		Vec3f    posOffset;

		static int constexpr MaxLayerCount = 32;

		struct Layer
		{
			Layer()
				: index(0)
				, meshPool(nullptr)
			{
				bound.invalidate();
				occluderBox.invalidate();
				FMemory::Zero(&args, sizeof(args));
				FMemory::Zero(&vertexAllocation, sizeof(vertexAllocation));
				FMemory::Zero(&indexAlloction, sizeof(indexAlloction));
			}

			int index;
			AABBox bound;
			AABBox occluderBox;

			MeshRenderPoolData* meshPool;
			DrawCmdArgs args;

			BuddyAllocatorBase::Allocation vertexAllocation;
			BuddyAllocatorBase::Allocation indexAlloction;
		};
		Layer layers[MaxLayerCount];
		int numLayer;
	};

	class RenderEngine : public IWorldEventListener , public IChunkEventListener
	{
	public:
		RenderEngine( int w , int h );
		~RenderEngine();

		bool initializeRHI();
		void releaseRHI();

		void beginRender();
		void renderWorld( ICamera& camera );

		void endRender();


		void onModifyBlock( int bx , int by , int bz );

		void setupWorld( World& world );

		void tick(float deltaTime);
	private:
		void cleanupRenderData()
		{
			mGereatePool->cencelAllWorks();
			mGereatePool->waitAllWorkComplete();
			for (auto const& pair : mChunkMap)
			{
				delete pair.second;
			}
			mChunkMap.clear();
		}

		virtual void onChunkAdded(Chunk* chunk);
		virtual void onPrevRemovChunk(Chunk* chunk){}

		void resetChunkRenderData(ChunkRenderData* data);
		void markChunkDirty(ChunkPos const& chunkPos);
		void updateRenderData(ChunkRenderData* data);

		typedef std::unordered_map< uint64 , ChunkRenderData* > ChunkDataMap;

		typedef TArray< ChunkRenderData* > WorldDataVec;

		struct UpdatedRenderData
		{
			Mesh mesh;
			ChunkRenderData* chunkData;
			AABBox bound;

			struct Layer
			{
				int index;
				AABBox bound;

				uint32 vertexOffset;
				uint32 vertexCount;
				uint32 indexOffset;
				uint32 indexCount;
			};

			Layer layers[ChunkRenderData::MaxLayerCount];
			int numLayer;
		};
	public:
		ICamera* mDebugCamera = nullptr;

		Mutex mMutexPedingAdd;
		TArray< UpdatedRenderData > mPendingAddList;

		QueueThreadPool* mGereatePool;
		World* mClientWorld;
		int    mRenderDepth;
		int    mRenderHeight;
		int    mRenderWidth;
		ChunkDataMap  mChunkMap;
		Render::PrimitivesCollection mDebugPrimitives;
		float  mAspect;
		Vec2i  mViewSize;

		class BlockRenderShaderProgram* mProgBlockRender;
		class BlockRenderShaderProgram* mProgBlockRenderDepth;
		class BlockRenderShaderProgram* mProgBlockRenderOverdraw;
		class HZBGenerateCS* mProgHZBGenerate;

		bool bShowOverdraw = false;
		bool bShowChunkLayerBoundOverDraw = false;
		int  mChunkRequestBudgetPerFrame = 256;
		bool bWireframeMode = false;
		double mMergeTimeAcc = 0;

		Render::RHIInputLayoutRef mBlockInputLayout;
		Render::RHITexture2DRef  mTexBlockAtlas;

		TArray< MeshRenderPoolData* > mMeshPool;

		Render::RHITexture2DRef mHZBTexture;
		Render::RHITexture2DRef mHZBScratchTexture;
		Render::RHITexture2DRef mMipTestTexture;
		Render::RHITexture2DRef mSceneTexture;
		Render::RHITexture2DRef mSceneDepthTexture;
		Render::RHIFrameBufferRef mSceneFrameBuffer;
		Render::RHITexture2DRef mOccluderColorTexture;
		Render::RHITexture2DRef mOccluderDepthTexture;
		Render::RHIFrameBufferRef mOccluderFrameBuffer;
		Render::RHIBufferRef mHZBCullItemBuffer;
		Render::RHITexture2DRef mHZBCullResultTexture;


		void resizeRenderTarget();
		void generateHZB(Render::RHICommandList& commandList, Render::RHITexture2D& sourceDepthTexture);

		void notifyViewSizeChanged(Vec2i const& newSize);

		MeshRenderPoolData* acquireMeshRenderData(Mesh const& mesh);
		MeshRenderPoolData* acquireMeshRenderData(uint32 vertexSize, uint32 indexSize);
		Render::RHIBufferRef mCmdBuildBuffer;
		Render::RHIBufferRef mCmdBuffer;
		int mRenderFrame = 0;


	};


}//namespace Cube

#endif // CubeRenderEngine_h__
