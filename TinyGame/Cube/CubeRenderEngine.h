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
		uint32 indexCountPerInstance;
		uint32 instanceCount;
		uint32 startIndexLocation;
		int32  baseVertexLocation;
		uint32 startInstanceLocation;
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
		bool     bHighPriorityUpdate = false;
		uint32   dirtyLayerMask = 0;
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
			mHighPriorityGeneratePool->cencelAllWorks();
			mHighPriorityGeneratePool->waitAllWorkComplete();
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
		void markChunkDirty(ChunkPos const& chunkPos, uint32 layerMask, bool bHighPriority = true);
		void updateRenderData(ChunkRenderData* data, bool bForceHighPriority = false);
		void processPendingMeshUpdates(int maxCount, double timeBudgetMS);

		typedef std::unordered_map< uint64 , ChunkRenderData* > ChunkDataMap;

		typedef TArray< ChunkRenderData* > WorldDataVec;

		struct UpdatedRenderData
		{
			Mesh mesh;
			ChunkRenderData* chunkData;
			AABBox bound;
			bool bFullRebuild = false;

			struct Layer
			{
				int index;
				AABBox bound;
				bool bHasMesh = false;

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
		TArray< UpdatedRenderData* > mPendingAddList;
		TArray< UpdatedRenderData* > mProcessingAddList;
		TArray< UpdatedRenderData* > mFreeUpdateDataList;

		UpdatedRenderData* acquireUpdateData();

		void releaseUpdateData(UpdatedRenderData* data);
		
		int    mMeshUpdateBudgetPerFrame = 8;
		double mMeshUpdateTimeBudgetMS = 1.0;

		QueueThreadPool* mGereatePool;
		QueueThreadPool* mHighPriorityGeneratePool;
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
		class HZBOccluderTileVoteCS* mProgHZBOccluderTileVote;
		class HZBOccluderSelectCS* mProgHZBOccluderSelect;

		bool bShowOverdraw = false;
		bool bShowChunkLayerBoundOverDraw = false;
		int  mChunkRequestBudgetPerFrame = 256;
		bool bWireframeMode = false;
		double mMeshBuildTimeAcc = 0;

		Render::RHIInputLayoutRef mBlockInputLayout;
		Render::RHITexture2DRef  mTexBlockAtlas;

		TArray< MeshRenderPoolData* > mMeshPool;

		Render::RHITexture2DRef mHZBTexture;
		Render::RHITexture2DRef mHZBScratchTexture;
		Render::RHITexture2DRef mSceneTexture;
		Render::RHITexture2DRef mSceneDepthTexture;
		Render::RHIFrameBufferRef mSceneFrameBuffer;
		Render::RHITexture2DRef mOccluderColorTexture;
		Render::RHITexture2DRef mOccluderDepthTexture;
		Render::RHIFrameBufferRef mOccluderFrameBuffer;
		Render::RHIBufferRef mHZBCullItemBuffer;
		Render::RHIBufferRef mHZBOccluderSelectItemBuffer;
		Render::RHIBufferRef mHZBOccluderVoteBuffer;
		Render::RHIBufferRef mOccluderCmdBuildBuffer;
		Render::RHIBufferRef mOccluderCmdBuffer;
		Render::RHIBufferRef mOccluderPoolCounterBuffer;
		Render::RHITexture2DRef mHZBCullResultTexture;


		void resizeRenderTarget();
		void generateHZB(Render::RHICommandList& commandList, Render::RHITexture2D& sourceDepthTexture);

		void notifyViewSizeChanged(Vec2i const& newSize);

		MeshRenderPoolData* acquireMeshRenderData(Mesh const& mesh);
		MeshRenderPoolData* acquireMeshRenderData(uint32 vertexSize, uint32 indexSize);
		Render::RHIBufferRef mCmdBuildBuffer;
		Render::RHIBufferRef mCmdBuffer;
		int mRenderFrame = 0;
		int mChunkScanOffsetViewDist = 0;
		int mChunkReleaseMargin = 8;
		int mNonPriorityScanInterval = 4;
		TArray<Vec2i> mChunkScanOffsets;


	};


}//namespace Cube

#endif // CubeRenderEngine_h__
