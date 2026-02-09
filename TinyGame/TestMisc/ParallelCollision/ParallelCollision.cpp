#include "TestMiscPCH.h"
#include "ParallelCollision.h"
#include "Math/Math2D.h"
#include <mutex>
#include <algorithm>
#include <atomic>
#include "ProfileSystem.h"

#include <thread>
#include "Math/SIMD.h"

#define USE_COLLISION_COLORING 0
#define USE_SIMD_LEGACY 1

namespace ParallelCollision
{
	using namespace Math;
	using FloatVector = SIMD::TFloatVector<4>;
	using IntVector = SIMD::TIntVector<4>;

	bool IntersectionTests::IntersectRectCircle(Vector2 const& rectPos, Vector2 const& rectHalfSize, float rectRot, Vector2 const& circlePos, float circleRadius)
	{
		float s, c;
		Math::SinCos(-rectRot, s, c);
		Vector2 delta = circlePos - rectPos;
		Vector2 localPos = Vector2(delta.x * c - delta.y * s, delta.x * s + delta.y * c);
		Vector2 closest = Vector2(Math::Clamp(localPos.x, -rectHalfSize.x, rectHalfSize.x),
								  Math::Clamp(localPos.y, -rectHalfSize.y, rectHalfSize.y));
		return (localPos - closest).length2() < (circleRadius * circleRadius);
	}

	bool IntersectionTests::IntersectArcCircle(Vector2 const& arcPos, float arcRadius, float arcRot, float halfSweep, Vector2 const& circlePos, float circleRadius)
	{
		Vector2 delta = circlePos - arcPos;
		float distSq = delta.length2();
		float maxDist = arcRadius + circleRadius;

		if (distSq > maxDist * maxDist) return false;
		if (distSq < 1e-6f) return true;

		float angle = Math::ATan2(delta.y, delta.x);
		float angleDiff = Math::Abs(Math::ATan2(Math::Sin(angle - arcRot), Math::Cos(angle - arcRot)));

		if (angleDiff <= halfSweep) return true;
		return false;
	}

	void SpatialHashGrid::init(Vector2 const& min, Vector2 const& max, float cellSize, int entityCount)
	{
		minBound = min;
		invCellSize = 1.0f / cellSize;
		gridWidth = (int)Math::Ceil((max.x - min.x) * invCellSize);
		gridHeight = (int)Math::Ceil((max.y - min.y) * invCellSize);

		cellHeads.resize(gridWidth * gridHeight);
		nextIndices.resize(entityCount);
		entityCellIndices.resize(entityCount);
	}

	void SpatialHashGrid::build(TArray<Vector2> const& positions, int count)
	{
		for (int& head : cellHeads) head = -1;

		for (int i = 0; i < count; ++i)
		{
			Vector2 const& pos = positions[i];
			int cx = Math::Clamp((int)((pos.x - minBound.x) * invCellSize), 0, gridWidth - 1);
			int cy = Math::Clamp((int)((pos.y - minBound.y) * invCellSize), 0, gridHeight - 1);

			int cellIdx = cx + cy * gridWidth;
			nextIndices[i] = cellHeads[cellIdx];
			cellHeads[cellIdx] = i;
			entityCellIndices[i] = cellIdx;
		}
	}

	template< typename TFunc >
	class TFrameWork : public IQueuedWork
	{
	public:
		TFrameWork(TFunc&& func) : mFunc(std::forward<TFunc>(func)) {}
		void executeWork() override { mFunc(); }
		// 關鍵：使用 FrameAllocator 分配時，release 只執行析構，不 delete 指標
		void release() override { this->~TFrameWork(); }
		TFunc mFunc;
	};

	template< typename TFunc >
	static void ParallelFor(QueueThreadPool& threadPool, FrameAllocator& allocator, char const* taskName, int count, TFunc&& func, int batchSize = 64)
	{
		if (count <= 0) return;

		StackMaker marker(allocator);


		int numTasks = (count + batchSize - 1) / batchSize;

		struct TaskRunner
		{
			int start, end;
			TFunc func;
			char const* name;
			
			void operator()()
			{
				PROFILE_ENTRY(name);
				for (int k = start; k < end; ++k) func(k);
			}
		};

		using WorkType = TFrameWork<TaskRunner>;
		size_t workSize = sizeof(WorkType);
		size_t checkAlign = alignof(WorkType) > 16 ? alignof(WorkType) : 16;
		
		uint8* chunkStart = nullptr;
		{
			///std::lock_guard<std::mutex> lock(allocMutex);
			chunkStart = (uint8*)allocator.alloc(workSize * numTasks, checkAlign);
		}

		TArray<IQueuedWork*> works;
		works.reserve(numTasks);

		for (int i = 0; i < numTasks; ++i)
		{
			int start = i * batchSize;
			int end = std::min(start + batchSize, count);
			
			TaskRunner runner = { start, end, func , taskName };
			WorkType* work = new (chunkStart + i * workSize) WorkType(std::move(runner));
			works.push_back(work);
		}

		threadPool.addWorks(works.data(), (int)works.size());
		threadPool.waitAllWorkCompleteInWorker();
	}

	void ParallelCollisionSolver::clearEntities()
	{
		positions.clear();
		velocities.clear();
		radii.clear();
		masses.clear();
		isStatic.clear();
		categoryIds.clear();
		targetMasks.clear();
		queryFlags.clear();
	}

	void ParallelCollisionSolver::clearBullets()
	{
		bulletPositions.clear();
		bulletVelocities.clear();
		bulletRadii.clear();
		bulletTargetMasks.clear();
		bulletShapeTypes.clear();
		bulletShapeParams.clear();
	}

	void ParallelCollisionSolver::schedule(float dt, int entityCount, int bulletCount, QueueThreadPool& threadPool)
	{
		// 關鍵：在每一幀開始時清空上一幀的所有分配，這時上一幀的背景執行緒絕對已經 release 了
		mTaskAllocator.clearFrame();
		mSolveEvent.reset();
		assert(!mIsSolving);
		mIsSolving = true;
		mEntityCount = entityCount;
		mBulletCount = bulletCount;

		if (grid.nextIndices.size() < (int)mEntityCount)
		{
			grid.nextIndices.resize(mEntityCount);
			grid.entityCellIndices.resize(mEntityCount);
		}

		auto pipelineTask = [this, dt, &threadPool]()
		{
			auto start = std::chrono::high_resolution_clock::now();
			solveInternal(dt, threadPool);
			auto end = std::chrono::high_resolution_clock::now();
			mLastSolveTime = std::chrono::duration<float, std::milli>(end - start).count();
			mSolveEvent.fire();
			mIsSolving = false;
		};

		// 這裡是頂層物理管線任務，同樣使用 mTaskAllocator
		TFrameWork< decltype(pipelineTask) >* work;
		{
			size_t alignedOffset = (mTaskAllocator.mOffset + 15) & ~15;
			if (alignedOffset > mTaskAllocator.mOffset) mTaskAllocator.alloc(alignedOffset - mTaskAllocator.mOffset);
			work = new (mTaskAllocator.alloc(sizeof(TFrameWork< decltype(pipelineTask) >))) TFrameWork< decltype(pipelineTask) >(std::move(pipelineTask));
		}
		
		threadPool.addWork(work);
	}

	void ParallelCollisionSolver::ensureComplete()
	{
		if (mIsSolving)
		{
			mSolveEvent.wait();
			mIsSolving = false;
		}
	}

	void ParallelCollisionSolver::updateMove(float dt, QueueThreadPool& threadPool)
	{
		Vector2* __restrict pPos = positions.data();
		Vector2 const* __restrict pVel = velocities.data();
		bool const* __restrict pStatic = isStatic.data();

		ParallelFor(threadPool, mTaskAllocator, "UpdateMove", mEntityCount, [pPos, pVel, pStatic, dt](int i)
		{
			if (!pStatic[i])
			{
				pPos[i] += pVel[i] * dt;
			}
		}, 1024);
	}

	void ParallelCollisionSolver::fillNeighborCache(QueueThreadPool& threadPool)
	{
		int cellCount = grid.gridWidth * grid.gridHeight;
		cellNeighborCounts.resize(cellCount);
		globalNeighborCache.resize(cellCount * fixedCapacity);

		int const* __restrict pCellHeads = grid.cellHeads.data();
		int const* __restrict pNextIndices = grid.nextIndices.data();
		int* __restrict pNeighborCounts = cellNeighborCounts.data();
		int* __restrict pGlobalCache = globalNeighborCache.data();
		int gridWidth = grid.gridWidth;
		int gridHeight = grid.gridHeight;
		int fixedCap = fixedCapacity;

		ParallelFor(threadPool, mTaskAllocator, "FillNeighborCache", cellCount, 
			[pCellHeads, pNextIndices, pNeighborCounts, pGlobalCache, gridWidth, gridHeight, fixedCap](int i)
		{
			if (pCellHeads[i] != -1)
			{
				int cx = i % gridWidth;
				int cy = i / gridWidth;

				int baseIdx = i * fixedCap;
				int currentCount = 0;

				int xMin = Math::Max(cx - 1, 0);
				int xMax = Math::Min(cx + 1, gridWidth - 1);
				int yMin = Math::Max(cy - 1, 0);
				int yMax = Math::Min(cy + 1, gridHeight - 1);

				for (int ny = yMin; ny <= yMax; ++ny)
				{
					int rowOffset = ny * gridWidth;
					for (int nx = xMin; nx <= xMax; ++nx)
					{
						int otherIdx = pCellHeads[nx + rowOffset];
						while (otherIdx != -1)
						{
							if (currentCount < fixedCap)
							{
								pGlobalCache[baseIdx + currentCount] = otherIdx;
								currentCount++;
							}
							else
							{
								goto EndFill;
							}
							otherIdx = pNextIndices[otherIdx];
						}
					}
				}
			EndFill:
				pNeighborCounts[i] = currentCount;
			}
			else
			{
				pNeighborCounts[i] = 0;
			}
		}, 512);
	}


	void ParallelCollisionSolver::solveInternal(float dt, QueueThreadPool& threadPool)
	{
		updateMove(dt, threadPool);
		grid.build(positions, mEntityCount);

#if USE_COLLISION_COLORING
		for (int i = 0; i < 4; ++i) mColorEntities[i].clear();
		for (int i = 0; i < mEntityCount; ++i)
		{
			int cellIdx = grid.entityCellIndices[i];
			int cx = cellIdx % grid.gridWidth;
			int cy = cellIdx / grid.gridWidth;
			int color = (cx & 1) | ((cy & 1) << 1);
			mColorEntities[color].push_back(i);
		}
#endif
		fillNeighborCache(threadPool);

		if (settings.useXPBD)
		{
			solveXPBD(dt, threadPool);
		}
		else
		{
			solveLegacy(dt, threadPool);
		}

		solveQueries(threadPool);
	}

	void ParallelCollisionSolver::solveXPBD(float dt, QueueThreadPool& threadPool)
	{
		float safeDt = Math::Max(dt, 1e-6f);
		float alpha = settings.compliance / (safeDt * safeDt);

		Vector2* __restrict pPos = positions.data();
		float* __restrict pRadii = radii.data();
		float* __restrict pMasses = masses.data();
		bool* __restrict pStatic = isStatic.data();
		int* __restrict pCellIndices = grid.entityCellIndices.data();
		int* __restrict pNeighborCounts = cellNeighborCounts.data();
		int* __restrict pGlobalCache = globalNeighborCache.data();
		int fixedCap = fixedCapacity;
		int neighborCountsSize = (int)cellNeighborCounts.size();

		for (int iter = 0; iter < settings.iterations; ++iter)
		{
#if USE_COLLISION_COLORING
			for (int color = 0; color < 4; ++color)
			{
				int* pColorIndices = mColorEntities[color].data();
				int colorCount = (int)mColorEntities[color].size();

				ParallelFor(threadPool, mTaskAllocator, "SolveXPBD", colorCount, 
					[pPos, pRadii, pMasses, pStatic, pCellIndices, pNeighborCounts, pGlobalCache, fixedCap, neighborCountsSize, alpha, pColorIndices](int idx)
				{
					int i = pColorIndices[idx];
					int cellIdx = pCellIndices[i];
					if (cellIdx < 0 || cellIdx >= neighborCountsSize) return;

					Vector2 posA = pPos[i];
					float rA = pRadii[i];
					float wA = 1.0f / pMasses[i];
					Vector2 totalPush = Vector2::Zero();

					int startIdx = cellIdx * fixedCap;
					int count = pNeighborCounts[cellIdx];

					for (int n = 0; n < count; ++n)
					{
						int bIdx = pGlobalCache[startIdx + n];
						if (i == bIdx) continue;

						Vector2 delta = posA - pPos[bIdx];
						float distSq = delta.length2();
						float minDist = rA + pRadii[bIdx];

						if (distSq >= minDist * minDist || distSq < 1e-8f) continue;

						float invDist = 1.0f / Math::Sqrt(distSq);
						float dist = distSq * invDist;
						float overlap = minDist - dist;
						float wB = 1.0f / pMasses[bIdx];

						float lambda = overlap / (wA + wB + alpha);
						totalPush += (delta * invDist) * (lambda * wA);
					}

					if (!pStatic[i]) pPos[i] += totalPush;
				}, 400);
			}
#else
			ParallelFor(threadPool, mTaskAllocator, "SolveXPBD", mEntityCount, 
				[pPos, pRadii, pMasses, pStatic, pCellIndices, pNeighborCounts, pGlobalCache, fixedCap, neighborCountsSize, alpha](int i)
			{
				int cellIdx = pCellIndices[i];
				if (cellIdx < 0 || cellIdx >= neighborCountsSize) return;

				Vector2 posA = pPos[i];
				float rA = pRadii[i];
				float wA = 1.0f / pMasses[i];
				Vector2 totalPush = Vector2::Zero();

				int startIdx = cellIdx * fixedCap;
				int count = pNeighborCounts[cellIdx];

				for (int n = 0; n < count; ++n)
				{
					int bIdx = pGlobalCache[startIdx + n];
					if (i == bIdx) continue;

					Vector2 delta = posA - pPos[bIdx];
					float distSq = delta.length2();
					float minDist = rA + pRadii[bIdx];

					if (distSq >= minDist * minDist || distSq < 1e-8f) continue;

					float invDist = 1.0f / Math::Sqrt(distSq);
					float dist = distSq * invDist;
					float overlap = minDist - dist;
					float wB = 1.0f / pMasses[bIdx];

					float lambda = overlap / (wA + wB + alpha);
					totalPush += (delta * invDist) * (lambda * wA);
				}

				if (!pStatic[i]) pPos[i] += totalPush;
			}, 400);
#endif
		}
	}

	void ParallelCollisionSolver::solveLegacy(float dt, QueueThreadPool& threadPool)
	{
		Vector2* __restrict pPos = positions.data();
		float* __restrict pRadii = radii.data();
		float* __restrict pMasses = masses.data();
		bool* __restrict pStatic = isStatic.data();
		int* __restrict pCellIndices = grid.entityCellIndices.data();
		int* __restrict pNeighborCounts = cellNeighborCounts.data();
		int* __restrict pGlobalCache = globalNeighborCache.data();

		int fixedCap = fixedCapacity;
		int neighborCountsSize = (int)cellNeighborCounts.size();
		float relaxation = settings.relaxation;
		float maxPushDistance = settings.maxPushDistance;

		for (int iter = 0; iter < settings.iterations; ++iter)
		{
#if USE_COLLISION_COLORING
			for (int color = 0; color < 4; ++color)
			{
				int* pColorIndices = mColorEntities[color].data();
				int colorCount = (int)mColorEntities[color].size();

				ParallelFor(threadPool, mTaskAllocator, "SolveCollision", colorCount, 
					[pPos, pRadii, pMasses, pStatic, pCellIndices, pNeighborCounts, pGlobalCache, fixedCap, neighborCountsSize, relaxation, maxPushDistance, pColorIndices](int idx)
				{
					int i = pColorIndices[idx];
#else
			ParallelFor(threadPool, mTaskAllocator, "SolveCollision", mEntityCount, 
				[pPos, pRadii, pMasses, pStatic, pCellIndices, pNeighborCounts, pGlobalCache, fixedCap, neighborCountsSize, relaxation, maxPushDistance](int i)
			{
#endif
#if !USE_SIMD_LEGACY
					int cellIdx = pCellIndices[i];
					if (cellIdx < 0 || cellIdx >= neighborCountsSize) return;

					Vector2 posA = pPos[i];
					float rA = pRadii[i];
					float mA = pMasses[i];
					Vector2 totalPush = Vector2::Zero();

					int startIdx = cellIdx * fixedCap;
					int count = pNeighborCounts[cellIdx];

					for (int n = 0; n < count; ++n)
					{
						int bIdx = pGlobalCache[startIdx + n];
						if (i == bIdx) continue;

						Vector2 delta = posA - pPos[bIdx];
						float distSq = delta.length2();
						float minDist = rA + pRadii[bIdx];

						if (distSq >= minDist * minDist || distSq < 1e-8f) continue;

						float invDist = 1.0f / Math::Sqrt(distSq);
						float dist = distSq * invDist;
						float overlap = (minDist - dist) * relaxation;

						if (overlap < minDist * 0.1f) overlap = Math::Min(overlap, maxPushDistance);

						Vector2 normal = delta * invDist;
						float ratio = pMasses[bIdx] / (mA + pMasses[bIdx]);
						totalPush += normal * (overlap * ratio);
					}

					if (!pStatic[i]) pPos[i] += totalPush;
#else
					int cellIdx = pCellIndices[i];
					if (cellIdx < 0 || cellIdx >= neighborCountsSize) return;

					Vector2 posA = pPos[i];
					float rA = pRadii[i];
					float mA = pMasses[i];

					FloatVector vPosX_A(posA.x);
					FloatVector vPosY_A(posA.y);
					FloatVector vRadii_A(rA);
					FloatVector vMass_A(mA);

					FloatVector vTotalPushX(0.0f);
					FloatVector vTotalPushY(0.0f);

					FloatVector vRelaxation(relaxation);
					FloatVector vMaxPush(maxPushDistance);

					int startIdx = cellIdx * fixedCap;
					int count = pNeighborCounts[cellIdx];

					FloatVector vRamp;
					for (int k = 0; k < FloatVector::Size; ++k) vRamp[k] = (float)k;
					FloatVector vEpsilon(1e-8f);

					for (int n = 0; n < count; n += FloatVector::Size)
					{
						IntVector vIndices = IntVector::load(pGlobalCache + startIdx + n);
						
						// 1. Phase 1: Lazy Gather - Gather only Pos and Radii
						FloatVector vPosX_B = FloatVector::gather<8>((float*)pPos, vIndices);
						FloatVector vPosY_B = FloatVector::gather<8>((float*)pPos + 1, vIndices);
						FloatVector vRadii_B = FloatVector::gather<4>(pRadii, vIndices);

						FloatVector vN = vRamp + FloatVector((float)n);
						auto vValidMask = vN < FloatVector((float)count);

						FloatVector vDeltaX = vPosX_A - vPosX_B;
						FloatVector vDeltaY = vPosY_A - vPosY_B;
						FloatVector vDistSq = vDeltaX * vDeltaX + vDeltaY * vDeltaY;
						FloatVector vMinDist = vRadii_A + vRadii_B;
						FloatVector vMinDistSq = vMinDist * vMinDist;

						// 2. Initial Collision Mask
						auto vCollisionMask = (vDistSq < vMinDistSq) & (vDistSq > vEpsilon) & vValidMask;

						// 3. Early Out: If no collision in these 8 neighbors, skip expensive work
						if (!vCollisionMask.isAnyActive())
							continue;

						// 4. Phase 2: Gather Mass and perform full calculation
						FloatVector vMass_B = FloatVector::gather<4>(pMasses, vIndices);
						FloatVector vInvDist = SIMD::rsqrt(vDistSq);
						FloatVector vDist = vDistSq * vInvDist;
						FloatVector vOverlap = (vMinDist - vDist) * vRelaxation;

						auto vSmallMask = vOverlap < (vMinDist * 0.1f);
						vOverlap = SIMD::select(vSmallMask, SIMD::min(vOverlap, vMaxPush), vOverlap);

						FloatVector vFactor = vInvDist * vOverlap * (vMass_B / (vMass_A + vMass_B));

						vTotalPushX = vTotalPushX + SIMD::select(vCollisionMask, vDeltaX * vFactor, FloatVector(0.0f));
						vTotalPushY = vTotalPushY + SIMD::select(vCollisionMask, vDeltaY * vFactor, FloatVector(0.0f));
					}

					if (!pStatic[i])
					{
						pPos[i].x += vTotalPushX.sum();
						pPos[i].y += vTotalPushY.sum();
					}
#endif
#if USE_COLLISION_COLORING
				}, 400);
			}
#else
				}, 400);
#endif
		}
	}

	void ParallelCollisionSolver::solveQueries(QueueThreadPool& threadPool)
	{
		entityHitPairs.clear();
		bulletHitPairs.clear();
		std::mutex hitMutex;
		std::mutex bulletHitMutex;

		const int batchSize = 512;

		int entityNumTasks = (mEntityCount + batchSize - 1) / batchSize;
		ParallelFor(threadPool, mTaskAllocator, "EntityQuery", entityNumTasks, [this, batchSize, &hitMutex](int taskId)
		{
			int start = taskId * batchSize;
			int end = std::min(start + batchSize, mEntityCount);

			TArray<TVector2<int>, TInlineAllocator<16>> taskHits;
			for (int i = start; i < end; ++i)
			{
				if (!queryFlags[i]) 
					continue;

				Vector2 posA = positions[i];
				float rA = radii[i];
				int maskA = targetMasks[i];

				int cellIdx = grid.entityCellIndices[i];
				if (cellIdx < 0 || cellIdx >= (int)cellNeighborCounts.size()) 
					continue;

				int startIdx = cellIdx * fixedCapacity;
				int count = cellNeighborCounts[cellIdx];

				for (int n = 0; n < count; ++n)
				{
					int bIdx = globalNeighborCache[startIdx + n];
					if (i == bIdx) continue;

					if ((maskA & categoryIds[bIdx]) != 0)
					{
						float minDist = rA + radii[bIdx];
						if ((posA - positions[bIdx]).length2() < minDist * minDist)
						{
							taskHits.emplace_back(i, bIdx);
						}
					}
				}
			}

			if (taskHits.size() > 0)
			{
				std::lock_guard<std::mutex> lock(hitMutex);
				entityHitPairs.append(taskHits.begin(), taskHits.end());
			}
		}, 1);

		if (mBulletCount > 0)
		{
			int bulletNumTasks = (mBulletCount + batchSize - 1) / batchSize;
			ParallelFor(threadPool, mTaskAllocator, "BulletQuery", bulletNumTasks, [this, batchSize, &bulletHitMutex](int taskId)
			{
				int start = taskId * batchSize;
				int end = std::min(start + batchSize, mBulletCount);

				TArray<TVector2<int>, TInlineAllocator<16>> taskHits;
				for (int i = start; i < end; ++i)
				{
					Vector2 bPos = bulletPositions[i];
					float bRadius = bulletRadii[i];
					int bMask = bulletTargetMasks[i];

					int cx = Math::Clamp((int)((bPos.x - grid.minBound.x) * grid.invCellSize), 0, grid.gridWidth - 1);
					int cy = Math::Clamp((int)((bPos.y - grid.minBound.y) * grid.invCellSize), 0, grid.gridHeight - 1);

					int xMin = Math::Max(cx - 1, 0);
					int xMax = Math::Min(cx + 1, grid.gridWidth - 1);
					int yMin = Math::Max(cy - 1, 0);
					int yMax = Math::Min(cy + 1, grid.gridHeight - 1);

					for (int ny = yMin; ny <= yMax; ++ny)
					{
						int rowOffset = ny * grid.gridWidth;
						for (int nx = xMin; nx <= xMax; ++nx)
						{
							int entityIdx = grid.cellHeads[nx + rowOffset];
							while (entityIdx != -1)
							{
								if ((bMask & categoryIds[entityIdx]) != 0)
								{
									float minDist = bRadius + radii[entityIdx];
									if ((bPos - positions[entityIdx]).length2() < minDist * minDist)
									{
										taskHits.emplace_back(i, entityIdx);
									}
								}
								entityIdx = grid.nextIndices[entityIdx];
							}
						}
					}
				}

				if (taskHits.size() > 0)
				{
					std::lock_guard<std::mutex> lock(bulletHitMutex);
					bulletHitPairs.append(taskHits.begin(), taskHits.end());
				}
			}, 1);
		}
	}

	ParallelCollisionManager::ParallelCollisionManager(QueueThreadPool& threadPool)
		: mThreadPool(threadPool)
	{
		mActiveBitset.resize(1000, false);
		mReceivers.resize(1000);
		mEntityDataBuffer.resize(1000);
	}

	ParallelCollisionManager::~ParallelCollisionManager()
	{
		cleanup();
		for (auto* ptr : mEventDataPool) delete ptr;
		mEventDataPool.clear();
	}

	void ParallelCollisionManager::init(Vector2 const& min, Vector2 const& max, float cellSize)
	{
		mSolver.grid.init(min, max, cellSize, 1000);
	}

	int ParallelCollisionManager::registerEntity(IPhysicsEntity* entity, CollisionEntityData const& data)
	{
		int id = mEntityCount++;

		if (id >= (int)mReceivers.size())
		{
			mReceivers.resize(id * 2 + 1);
			mEntityDataBuffer.resize(id * 2 + 1);
			mActiveBitset.resize(id * 2 + 1, false);
		}

		mEntityDataBuffer[id] = data;
		mReceivers[id].entity = entity;
		
		if (data.bQueryCollision)
		{
			if (mReceivers[id].eventData == nullptr)
			{
				mReceivers[id].eventData = allocEventData();
			}
			mReceivers[id].eventData->clearCurrent();
		}
		else
		{
			if (mReceivers[id].eventData)
			{
				freeEventData(mReceivers[id].eventData);
				mReceivers[id].eventData = nullptr;
			}
		}

		mReceivers[id].isPendingRemove = false;
		entity->mPhysicsId = id;
		return id;
	}

	void ParallelCollisionManager::unregisterEntity(IPhysicsEntity* entity)
	{
		int id = entity->mPhysicsId;
		if (id < 0 || id >= mEntityCount) return;

		if (mbDataLocked)
		{
			if (!mReceivers[id].isPendingRemove)
			{
				mReceivers[id].isPendingRemove = true;
				mPendingRemoveEntityIds.push_back(id);
			}
		}
		else
		{
			doUnregisterEntity(id);
		}
	}

	void ParallelCollisionManager::doUnregisterEntity(int id)
	{
		if (id < 0 || id >= mEntityCount) return;

		int lastIdx = mEntityCount - 1;
		if (id != lastIdx)
		{

			mEntityDataBuffer[id] = mEntityDataBuffer[lastIdx];
			
			mReceivers[id].entity = mReceivers[lastIdx].entity;
			
			// Move event data ownership
			if (mReceivers[id].eventData) freeEventData(mReceivers[id].eventData);
			mReceivers[id].eventData = mReceivers[lastIdx].eventData;
			mReceivers[lastIdx].eventData = nullptr;

			mReceivers[id].isPendingRemove = mReceivers[lastIdx].isPendingRemove;

			if (mReceivers[id].entity)
				mReceivers[id].entity->mPhysicsId = id;

			mLastActiveEntityIndices.erase(id);
			if (mLastActiveEntityIndices.erase(lastIdx))
			{
				mLastActiveEntityIndices.insert(id);
			}
		}
		else
		{
			if (mReceivers[id].eventData)
			{
				freeEventData(mReceivers[id].eventData);
				mReceivers[id].eventData = nullptr;
			}
			mReceivers[id].entity->mPhysicsId = -1;
			mLastActiveEntityIndices.erase(id);
		}

		mReceivers[lastIdx].entity = nullptr;
		mReceivers[lastIdx].isPendingRemove = false;

		mEntityCount--;
	}

	int ParallelCollisionManager::registerBullet(IPhysicsBullet* bullet, CollisionBulletData const& data)
	{
		int id = mBulletCount++;
		
		if (id >= (int)mBulletReceivers.size())
		{
			mBulletReceivers.resize(id * 2 + 1);
			mBulletDataBuffer.resize(id * 2 + 1);
		}

		mBulletDataBuffer[id] = data;
		mBulletReceivers[id].bullet = bullet;
		bullet->mBulletId = id;
		return id;
	}

	void ParallelCollisionManager::unregisterBullet(IPhysicsBullet* bullet)
	{
		int id = bullet->mBulletId;
		if (id < 0 || id >= mBulletCount) return;

		if (mbDataLocked)
		{
			mPendingRemoveBulletIds.push_back(id);
		}
		else
		{
			doUnregisterBullet(id);
		}
	}

	void ParallelCollisionManager::doUnregisterBullet(int id)
	{
		if (id < 0 || id >= mBulletCount) return;

		int lastIdx = mBulletCount - 1;
		if (id != lastIdx)
		{
			mBulletDataBuffer[id] = mBulletDataBuffer[lastIdx];
			mBulletReceivers[id] = mBulletReceivers[lastIdx];
			if (mBulletReceivers[id].bullet)
				mBulletReceivers[id].bullet->mBulletId = id;
		}

		mBulletReceivers[lastIdx].bullet = nullptr;
		mBulletCount--;
	}

	void ParallelCollisionManager::cleanup()
	{
		mSolver.ensureComplete();
		mSolver.clearEntities();
		mSolver.clearBullets();
		mEntityCount = 0;
		mBulletCount = 0;
		for (auto& receiver : mReceivers) 
		{
			if (receiver.entity) receiver.entity->mPhysicsId = -1;
			receiver.entity = nullptr;
			if (receiver.eventData)
			{
				freeEventData(receiver.eventData);
				receiver.eventData = nullptr;
			}
		}
		for (auto& receiver : mBulletReceivers) receiver.bullet = nullptr;
		std::fill(mActiveBitset.begin(), mActiveBitset.end(), false);
		mActiveList.clear();
		mLastActiveEntityIndices.clear();
		mPendingRemoveEntityIds.clear();
		mPendingRemoveBulletIds.clear();
	}

	void ParallelCollisionManager::syncFromSimulator()
	{
		// Critical: Only sync entities that the solver actually processed in the previous frame.
		// Newly spawned entities (index >= mSolver.positions.size()) have valid initial data in Buffer,
		// and must NOT be overwritten by OOB/garbage values from the Solver.
		int numToSync = std::min((int)mSolver.positions.size(), mEntityCount);
		for (int i = 0; i < numToSync; ++i)
		{
			mEntityDataBuffer[i].position = mSolver.positions[i];
		}

		int numBulletsToSync = std::min((int)mSolver.bulletPositions.size(), mBulletCount);
		for (int i = 0; i < numBulletsToSync; ++i)
		{
			mBulletDataBuffer[i].position = mSolver.bulletPositions[i];
		}
	}

	void ParallelCollisionManager::syncToSolver()
	{
		if (mEntityCount > (int)mSolver.positions.size())
		{
			mSolver.positions.resize(mEntityCount);
			mSolver.velocities.resize(mEntityCount);
			mSolver.radii.resize(mEntityCount);
			mSolver.masses.resize(mEntityCount);
			mSolver.isStatic.resize(mEntityCount);
			mSolver.categoryIds.resize(mEntityCount);
			mSolver.targetMasks.resize(mEntityCount);
			mSolver.queryFlags.resize(mEntityCount);
		}

		for (int i = 0; i < mEntityCount; ++i)
		{
			CollisionEntityData const& data = mEntityDataBuffer[i];
			mSolver.positions[i] = data.position;
			mSolver.velocities[i] = data.velocity;
			mSolver.radii[i] = data.radius; 
			mSolver.masses[i] = data.mass;
			mSolver.isStatic[i] = data.isStatic;
			mSolver.categoryIds[i] = data.categoryId;
			mSolver.targetMasks[i] = data.targetMask;
			mSolver.queryFlags[i] = data.bQueryCollision;
		}

		if (mBulletCount > (int)mSolver.bulletPositions.size())
		{
			mSolver.bulletPositions.resize(mBulletCount);
			mSolver.bulletVelocities.resize(mBulletCount);
			mSolver.bulletRadii.resize(mBulletCount);
			mSolver.bulletTargetMasks.resize(mBulletCount);
			mSolver.bulletShapeTypes.resize(mBulletCount);
			mSolver.bulletShapeParams.resize(mBulletCount);
		}

		for (int i = 0; i < mBulletCount; ++i)
		{
			CollisionBulletData const& data = mBulletDataBuffer[i];
			mSolver.bulletPositions[i] = data.position;
			mSolver.bulletVelocities[i] = data.velocity;
			mSolver.bulletRadii[i] = data.radius;
			mSolver.bulletTargetMasks[i] = data.targetMask;
			mSolver.bulletShapeTypes[i] = data.shapeType;
			mSolver.bulletShapeParams[i] = data.shapeParams;
		}
	}

	void ParallelCollisionManager::processPendingRemovals()
	{
		if (mPendingRemoveEntityIds.empty() && mPendingRemoveBulletIds.empty()) return;

		if (!mPendingRemoveEntityIds.empty())
		{
			std::sort(mPendingRemoveEntityIds.begin(), mPendingRemoveEntityIds.end(), std::greater<int>());
			int prevIdx = -1;
			for (int id : mPendingRemoveEntityIds)
			{
				if (id == prevIdx) continue;
				prevIdx = id;
				doUnregisterEntity(id);
			}
			mPendingRemoveEntityIds.clear();
		}

		if (!mPendingRemoveBulletIds.empty())
		{
			std::sort(mPendingRemoveBulletIds.begin(), mPendingRemoveBulletIds.end(), std::greater<int>());
			int prevIdx = -1;
			for (int id : mPendingRemoveBulletIds)
			{
				if (id == prevIdx) continue;
				prevIdx = id;
				doUnregisterBullet(id);
			}
			mPendingRemoveBulletIds.clear();
		}
	}

	void ParallelCollisionManager::beginFrame()
	{
        if (mbDataLocked)
        {
            mSolver.ensureComplete();
            syncFromSimulator();
            processCollisionEvents();
            mbDataLocked = false;
            processPendingRemovals();
        }
	}

	void ParallelCollisionManager::processCollisionEvents()
	{
		mActiveList.clear();

		for (auto const& pair : mSolver.entityHitPairs)
		{
			int idA = pair.x;
			int idB = pair.y;
			if (idA < mEntityCount && idB < mEntityCount)
			{
				IPhysicsEntity* entA = mReceivers[idA].entity;
				IPhysicsEntity* entB = mReceivers[idB].entity;
				if (entA && entB)
				{
					if (mReceivers[idA].eventData)
					{
						mReceivers[idA].eventData->currEntities.insert(entB);
					}

					if (!mActiveBitset[idA])
					{ 
						mActiveBitset[idA] = true; 
						mActiveList.push_back(idA);
					}
				}
			}
		}

		for (auto const& pair : mSolver.bulletHitPairs)
		{
			int bulletIdx = pair.x;
			int entityIdx = pair.y;
			if (bulletIdx < mBulletCount && entityIdx < mEntityCount)
			{
				IPhysicsBullet* bullet = mBulletReceivers[bulletIdx].bullet;
				if (bullet && mReceivers[entityIdx].entity)
				{
					if (mReceivers[entityIdx].eventData)
					{
						mReceivers[entityIdx].eventData->currBullets.insert(bullet);
					}
					if (!mActiveBitset[entityIdx]) 
					{ 
						mActiveBitset[entityIdx] = true; 
						mActiveList.push_back(entityIdx);
					}
				}
			}
		}

		for (int prevIdx : mLastActiveEntityIndices)
		{
			if (prevIdx < mEntityCount && !mActiveBitset[prevIdx])
			{
				mActiveBitset[prevIdx] = true;
				mActiveList.push_back(prevIdx);
			}
		}
		mLastActiveEntityIndices.clear();

		for (int id : mActiveList)
		{
			EntityData& data = mReceivers[id];
			if (!data.entity || data.isPendingRemove) continue;

			if (!data.eventData) continue;
			
			CollisionEventData& events = *data.eventData;

			for (auto* other : events.currEntities)
			{
				int cat = mSolver.categoryIds[other->mPhysicsId];
				if (events.prevEntities.count(other))
				{
					data.entity->handleCollisionStay(other, cat);
				}
				else
				{
					data.entity->handleCollisionEnter(other, cat);
				}
			}
			for (auto* prev : events.prevEntities)
			{
				if (events.currEntities.count(prev) == 0)
				{
					int cat = mSolver.categoryIds[prev->mPhysicsId];
					data.entity->handleCollisionExit(prev, cat);
				}
			}

			for (auto* bullet : events.currBullets)
			{
				int cat = mSolver.bulletTargetMasks[bullet->mBulletId];
				if (events.prevBullets.count(bullet))
				{
					data.entity->handleCollisionStay(bullet, cat);
				}
				else
				{
					data.entity->handleCollisionEnter(bullet, cat);
				}
			}
			for (auto* prev : events.prevBullets)
			{
				if (events.currBullets.count(prev) == 0)
				{
					int cat = mSolver.bulletTargetMasks[prev->mBulletId];
					data.entity->handleCollisionExit(prev, cat);
				}
			}
		}

		for (int id : mActiveList)
		{
			mActiveBitset[id] = false;
			
			if (id >= mEntityCount) 
				continue;
			
			if (mReceivers[id].eventData)
			{
				auto& events = *mReceivers[id].eventData;

				if (!events.currEntities.empty() || !events.currBullets.empty()) 
					mLastActiveEntityIndices.insert(id);

				events.swapSets();
			}
		}
	}

	void ParallelCollisionManager::endFrame(float dt)
	{
		mbDataLocked = true;
		syncToSolver();
		mSolver.schedule(dt, mEntityCount, mBulletCount, mThreadPool);
	}
}
