#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "DataStructure/Array.h"
#include "Async/AsyncWork.h"
#include "Memory/FrameAllocator.h"
#include "PlatformThread.h"
#include <mutex>

namespace ParallelCollision
{
	using Math::Vector2;
	using Math::Vector3;

	class IPhysicsBullet
	{
	public:
		virtual ~IPhysicsBullet() {}
		int mBulletId = -1;

		virtual void handleCollisionEnter(class IPhysicsEntity* other, int category) = 0;
		virtual void handleCollisionStay(class IPhysicsEntity* other, int category) = 0;
		virtual void handleCollisionExit(class IPhysicsEntity* other, int category) = 0;

		virtual void handleCollisionEnter(IPhysicsBullet* bullet, int category) = 0;
		virtual void handleCollisionStay(IPhysicsBullet* bullet, int category) = 0;
		virtual void handleCollisionExit(IPhysicsBullet* bullet, int category) = 0;
	};

	class IPhysicsEntity
	{
	public:
		virtual ~IPhysicsEntity() {}
		int mPhysicsId = -1;

		virtual void handleCollisionEnter(IPhysicsEntity* other, int category) = 0;
		virtual void handleCollisionStay(IPhysicsEntity* other, int category) = 0;
		virtual void handleCollisionExit(IPhysicsEntity* other, int category) = 0;

		virtual void handleCollisionEnter(IPhysicsBullet* bullet, int category) = 0;
		virtual void handleCollisionStay(IPhysicsBullet* bullet, int category) = 0;
		virtual void handleCollisionExit(IPhysicsBullet* bullet, int category) = 0;
	};

	struct CollisionSettings
	{
		float relaxation = 0.99f;
		float maxPushDistance = 10.0f;
		float minSeparation = 0.05f;
		bool  enableMassRatio = true;
		int   iterations = 8;
		float cellSize = 2.0f;
		bool  useXPBD = false;
		float compliance = 0.0001f;
		float frameTargetSubStep = 1.0f / 60.0f;
		int   maxSteps = 4;
	};

	struct CollisionWall
	{
		Vector2 min;
		Vector2 max;
	};

	enum class ShapeType : uint8
	{
		Circle = 0,
		Rect = 1,
		Arc = 2
	};

	struct CollisionEntityData
	{
		Vector2 position;
		Vector2 velocity = Vector2::Zero();
		float   radius;
		float   mass;
		bool    isStatic = false;
		bool    bQueryCollision = false;
		int     categoryId = 0;
		int     targetMask = 0;
	};

	struct CollisionBulletData
	{
		Vector2 position;
		Vector2 velocity = Vector2::Zero();
		float   rotation;
		int     categoryId;
		int     targetMask = 0;
		bool    bQueryBulletCollision = false;
		ShapeType shapeType = ShapeType::Circle;
		Vector3  shapeParams;
	};

	typedef void(*DeferredQueryCallback)(TArray<IPhysicsEntity*> const& results, void* userData);

	struct DeferredQueryRequest
	{
		Vector2 position;
		float   rotation;
		ShapeType shapeType;
		Vector3  shapeParams;
		int     targetMask;
		DeferredQueryCallback callback;
		void* userData;
	};

	struct DeferredQueryResult
	{
		TArray<int> results;
		DeferredQueryCallback callback;
		void* userData;
	};

	struct IntersectionTests
	{
		static bool IntersectRectCircle(Vector2 const& rectPos, Vector2 const& rectHalfSize, float rectRot, Vector2 const& circlePos, float circleRadius);
		static bool IntersectArcCircle(Vector2 const& arcPos, float arcRadius, float arcRot, float halfSweep, Vector2 const& circlePos, float circleRadius);
		static bool Intersect(Vector2 const& posA, float rA, ShapeType shapeTypeA, Vector3 const& shapeParamA, Vector2 const& posB, float rB);
	};

	struct SpatialHashGrid
	{
		Vector2 minBound;
		float   invCellSize;
		int     gridWidth;
		int     gridHeight;

		TArray<int> cellHeads;
		TArray<int> nextIndices;
		TArray<int> entityCellIndices;
		TArray<int> activeCellIndices;

		TArray<int> bulletHeads;
		TArray<int> bulletNextIndices;
		TArray<int> bulletCellIndices;
		TArray<int> bulletActiveIndices;

		void init(Vector2 const& min, Vector2 const& max, float cellSize, int entityCount, int bulletCount);
		void buildEntities(TArray<Vector2> const& positions, int count);
		void buildBullets(TArray<Vector2> const& positions, int count);
	};

	class ParallelCollisionSolver
	{
	public:
		ParallelCollisionSolver() : mTaskAllocator(16 * 1024 * 1024) {}

		CollisionSettings settings;
		SpatialHashGrid   grid;

		TArray<Vector2>   positions;
		TArray<Vector2>   velocities;
		TArray<float>     radii;
		TArray<float>     masses;
		TArray<float>     invMasses;
		TArray<bool>      isStatic;
		TArray<int>       categoryIds;
		TArray<int>       targetMasks;
		TArray<bool>      queryFlags;

		int mEntityCount = 0;
		int mBulletCount = 0;

		TArray<Vector2>   bulletPositions;
		TArray<Vector2>   bulletVelocities;
		TArray<float>     bulletRadii;
		TArray<int>       bulletTargetMasks;
		TArray<int>       bulletCategoryIds;
		TArray<bool>      bulletQueryFlags;
		TArray<ShapeType> bulletShapeTypes;
		TArray<Vector3>   bulletShapeParams;

		int               fixedCapacity = 512;
		TArray<int>       cellNeighborCounts;
		TArray<int>       globalNeighborCache;
		TArray<CollisionWall> walls;

		TArray<int>       mColorEntities[4];
		TArray<int>       largeEntityIndices;

		TArray<TVector2<int>>      entityHitPairs;
		TArray<TVector2<int>>      bulletHitPairs;
		TArray<TVector2<int>>      bulletBulletHitPairs;

		TArray<DeferredQueryRequest> deferredQueries;
		TArray<DeferredQueryResult>  deferredQueryResults;

		void clearEntities();
		void clearBullets();

		void schedule(float dt, int entityCount, int bulletCount, QueueThreadPool& threadPool);
		void ensureComplete();

		FrameAllocator& getTaskAllocator() { return mTaskAllocator; }
		float getLastSolveTime() const { return mLastSolveTime; }

	private:
		friend class ParallelCollisionManager;
		void solveInternal(float dt, QueueThreadPool& threadPool);
		void updateMove(float dt, QueueThreadPool& threadPool);
		void fillNeighborCache(QueueThreadPool& threadPool);
		void solveXPBD(float dt, QueueThreadPool& threadPool);
		void solveLegacy(float dt, QueueThreadPool& threadPool);
		void solveWalls(float dt, QueueThreadPool& threadPool);
		void solveQueries(QueueThreadPool& threadPool);
		void solveDeferredQueries(QueueThreadPool& threadPool);
		int  queryEntities(Vector2 const& pos, float rot, ShapeType type, Vector3 const& params, int mask, TArray<int>& results);

		ThreadEvent mSolveEvent;
		bool        mIsSolving = false;
		float       mLastSolveTime = 0.0f;
		FrameAllocator mTaskAllocator;
	};

	class ParallelCollisionManager
	{
	public:
		ParallelCollisionManager(QueueThreadPool& threadPool);
		~ParallelCollisionManager();

		void init(Vector2 const& min, Vector2 const& max, float cellSize);

		int  registerEntity(IPhysicsEntity* entity, CollisionEntityData const& data);
		void unregisterEntity(IPhysicsEntity* entity);

		int  registerBullet(IPhysicsBullet* bullet, CollisionBulletData const& data);
		void unregisterBullet(IPhysicsBullet* bullet);
		void cleanup();

		void addWall(Vector2 const& min, Vector2 const& max);
		void clearWalls();

		int  queryEntities(Vector2 const& position, float rotation, ShapeType shapeType, Vector3 const& shapeParams, int targetMask, TArray<IPhysicsEntity*>& results);
		void queryEntitiesDeferred(Vector2 const& position, float rotation, ShapeType shapeType, Vector3 const& shapeParams, int targetMask, DeferredQueryCallback callback, void* userData);

		void beginFrame();
		void endFrame(float dt);

		float getLastSolveTime() const { return mSolver.getLastSolveTime(); }

		ParallelCollisionSolver& getSolver() { return mSolver; }

		CollisionEntityData& getEntityData(int id)
		{
			if (id < 0 || id >= mEntityCount)
			{
				static CollisionEntityData dummyData;
				LogWarning(0, "getEntityData error : id = %d", id);
				return dummyData;
			}
			return mEntityDataBuffer[id]; 
		}
		CollisionBulletData& getBulletData(int id)
		{
			if (id < 0 || id >= mBulletCount)
			{
				static CollisionBulletData dummyData;
				LogWarning(0, "getBulletData error : id = %d", id);
				return dummyData;
			}
			return mBulletDataBuffer[id]; 
		}

	public:
		enum
		{
			HandleIndexBits = 20,
			HandleIndexMask = (1 << HandleIndexBits) - 1,
		};

		void processCollisionEvents();

		struct CollisionEventData
		{
			std::unordered_set<int> prevEntities;
			std::unordered_set<int> currEntities;
			std::unordered_set<int> prevBullets;
			std::unordered_set<int> currBullets;

			void clearCurrent() { currEntities.clear(); currBullets.clear(); }
			void swapSets()
			{
				prevEntities = std::move(currEntities); currEntities.clear();
				prevBullets = std::move(currBullets); currBullets.clear();
			}
		};

		struct EntityData
		{
			IPhysicsEntity* entity = nullptr;
			int eventHandle = -1;
			bool isPendingRemove = false;
			CollisionEventData* eventData = nullptr;
		};

		struct BulletData
		{
			IPhysicsBullet* bullet = nullptr;
			int eventHandle = -1;
			bool isPendingRemove = false;
			CollisionEventData* eventData = nullptr;
		};

		template<typename T>
		struct StableEntry
		{
			T* ptr = nullptr;
			int version = 0;
		};

		TArray<StableEntry<IPhysicsEntity>> mStableEntityMap;
		TArray<int> mFreeEntityEventHandleIndices;

		TArray<StableEntry<IPhysicsBullet>> mStableBulletMap;
		TArray<int> mFreeBulletEventHandleIndices;

		// Helpers for Stable ID
		int  allocEntityEventHandle(IPhysicsEntity* entity);
		void freeEntityEventHandle(int id);
		IPhysicsEntity* getEntityFromHandle(int id);

		int allocBulletEventHandle(IPhysicsBullet* bullet);
		void freeBulletEventHandle(int id);
		IPhysicsBullet* getBulletFromHandle(int id);

		void flushPendingFreeEventHandles();

		void syncToSolver();
		void syncFromSimulator();
		void processPendingRemovals();
		void processDeferredQueries();
		
		void doUnregisterEntity(int id);
		void doUnregisterBullet(int id);

		QueueThreadPool&  mThreadPool;
		ParallelCollisionSolver mSolver;

		TArray<CollisionEntityData> mEntityDataBuffer;
		TArray<EntityData> mReceivers;
		TArray<CollisionBulletData> mBulletDataBuffer;
		TArray<BulletData> mBulletReceivers;

		int mEntityCount = 0;
		int mBulletCount = 0;

		bool mbDataLocked = false;
		TArray<int> mPendingRemoveEntityIds;
		TArray<int> mPendingRemoveBulletIds;

		TArray<bool> mActiveBitset;
		TArray<int>  mActiveList;
		std::unordered_set<int> mLastActiveEntityIndices;
		std::unordered_set<int> mLastActiveBulletIndices;

		TArray<CollisionEventData*> mEventDataPool;
		CollisionEventData* allocEventData()
		{
			if (mEventDataPool.empty()) return new CollisionEventData();
			CollisionEventData* ptr = mEventDataPool.back();
			mEventDataPool.pop_back();
			return ptr;
		}
		void freeEventData(CollisionEventData* ptr)
		{
			if (!ptr) return;
			ptr->prevEntities.clear();
			ptr->currEntities.clear();
			ptr->prevBullets.clear();
			ptr->currBullets.clear();
			mEventDataPool.push_back(ptr);
		}
	};
}
