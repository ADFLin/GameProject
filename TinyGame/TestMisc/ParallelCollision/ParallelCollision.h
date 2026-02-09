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

	struct CollisionSettings
	{
		float relaxation = 0.5f;
		float maxPushDistance = 10.0f;
		float minSeparation = 0.05f;
		bool  enableMassRatio = true;
		int   iterations = 8;
		float cellSize = 2.0f;
		bool  useXPBD = false;
		float compliance = 0.0001f;
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
		Vector2 velocity;
		float   radius;
		float   mass;
		bool    isStatic;
		bool    bQueryCollision;
		int     categoryId;
		int     targetMask;
	};

	struct CollisionBulletData
	{
		Vector2 position;
		Vector2 velocity;
		float   rotation;
		float   radius;
		int     categoryId;
		int     targetMask;
		ShapeType shapeType;
		Vector3  shapeParams; // x: radius/halfWidth, y: halfHeight/sweep, z: unused
	};

	class IPhysicsBullet
	{
	public:
		virtual ~IPhysicsBullet() {}
		int mBulletId = -1;
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

		void init(Vector2 const& min, Vector2 const& max, float cellSize, int entityCount);
		void build(TArray<Vector2> const& positions, int count);
	};

	class ParallelCollisionSolver
	{
	public:
		ParallelCollisionSolver() : mTaskAllocator(16 * 1024 * 1024) {}

		CollisionSettings settings;
		SpatialHashGrid   grid;

		// SoA Data
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

		// Bullet SoA Data
		TArray<Vector2>   bulletPositions;
		TArray<Vector2>   bulletVelocities;
		TArray<float>     bulletRadii;
		TArray<int>       bulletTargetMasks;
		TArray<ShapeType> bulletShapeTypes;
		TArray<Vector3>   bulletShapeParams;

		// Neighbor Cache
		int               fixedCapacity = 128;
		TArray<int>       cellNeighborCounts;
		TArray<int>       globalNeighborCache;

		// Spatial Coloring
		TArray<int>       mColorEntities[4];

		// Query Output
		TArray<TVector2<int>>      entityHitPairs;
		TArray<TVector2<int>>      bulletHitPairs; // x: bulletIndex, y: entityIndex

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
		void solveQueries(QueueThreadPool& threadPool);

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

		// Lifecycle logic
		void beginFrame();
		void endFrame(float dt);

		float getLastSolveTime() const { return mSolver.getLastSolveTime(); }

		ParallelCollisionSolver& getSolver() { return mSolver; }

		CollisionEntityData& getEntityData(int id) { return mEntityDataBuffer[id]; }
		CollisionBulletData& getBulletData(int id) { return mBulletDataBuffer[id]; }

	private:
		void processCollisionEvents();

		struct CollisionEventData
		{
			std::unordered_set<IPhysicsEntity*> prevEntities;
			std::unordered_set<IPhysicsEntity*> currEntities;
			std::unordered_set<IPhysicsBullet*> prevBullets;
			std::unordered_set<IPhysicsBullet*> currBullets;

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
			bool isPendingRemove = false;
			CollisionEventData* eventData = nullptr;
		};

		struct BulletData
		{
			IPhysicsBullet* bullet = nullptr;
		};

		void syncToSolver();
		void syncFromSimulator(); // Implementation of SyncDataFromSimulator
		void processPendingRemovals();
		
		void doUnregisterEntity(int id);
		void doUnregisterBullet(int id);

		QueueThreadPool&  mThreadPool;
		ParallelCollisionSolver mSolver;

		TArray<CollisionEntityData> mEntityDataBuffer; // Managed buffer like C#
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
