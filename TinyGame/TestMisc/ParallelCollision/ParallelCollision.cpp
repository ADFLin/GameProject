#include "TestMiscPCH.h"
#include "ParallelCollision.h"
#include "Math/Math2D.h"
#include <mutex>
#include <algorithm>
#include <atomic>
#include <cstring>
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
		float diff = angle - arcRot;
		while (diff > Math::PI) diff -= 2.0f * Math::PI;
		while (diff < -Math::PI) diff += 2.0f * Math::PI;

		if (Math::Abs(diff) <= halfSweep) return true;
		return false;
	}

	bool IntersectionTests::Intersect(Vector2 const& posA, float rA, ShapeType shapeTypeA, Vector3 const& shapeParamA, Vector2 const& posB, float rB)
	{
		switch (shapeTypeA)
		{
		case ShapeType::Circle:
		{
			float rSum = shapeParamA.x + rB;
			return (posA - posB).length2() < rSum * rSum;
		}
		case ShapeType::Rect:
			return IntersectRectCircle(posA, Vector2(shapeParamA.x, shapeParamA.y), shapeParamA.z, posB, rB);
		case ShapeType::Arc:
			return IntersectArcCircle(posA, shapeParamA.x, shapeParamA.y, shapeParamA.z, posB, rB);
		}
		return false;
	}

	void SpatialHashGrid::init(Vector2 const& min, Vector2 const& max, float cellSize, int entityCount, int bulletCount)
	{
		minBound = min;
		invCellSize = 1.0f / cellSize;
		gridWidth = (int)Math::Ceil((max.x - min.x) * invCellSize);
		gridHeight = (int)Math::Ceil((max.y - min.y) * invCellSize);

		int cellCount = gridWidth * gridHeight;
		cellHeads.resize(cellCount);
		nextIndices.resize(entityCount);
		entityCellIndices.resize(entityCount);

		bulletHeads.resize(cellCount);
		bulletNextIndices.resize(bulletCount);
		bulletCellIndices.resize(bulletCount);
	}

	void SpatialHashGrid::buildEntities(TArray<Vector2> const& positions, int count)
	{
		if (nextIndices.size() < count)
		{
			nextIndices.resize(count);
			entityCellIndices.resize(count);
		}
		FMemory::Set(cellHeads.data(), 0xff, cellHeads.size() * sizeof(int));
		activeCellIndices.clear();

		for (int i = 0; i < count; ++i)
		{
			Vector2 const& pos = positions[i];
			int cx = Math::Clamp((int)((pos.x - minBound.x) * invCellSize), 0, gridWidth - 1);
			int cy = Math::Clamp((int)((pos.y - minBound.y) * invCellSize), 0, gridHeight - 1);

			int cellIdx = cx + cy * gridWidth;
			if (cellHeads[cellIdx] == -1)
			{
				activeCellIndices.push_back(cellIdx);
			}

			nextIndices[i] = cellHeads[cellIdx];
			cellHeads[cellIdx] = i;
			entityCellIndices[i] = cellIdx;
		}
	}

	void SpatialHashGrid::buildBullets(TArray<Vector2> const& positions, int count)
	{
		if (bulletNextIndices.size() < count)
		{
			bulletNextIndices.resize(count);
			bulletCellIndices.resize(count);
		}
		FMemory::Set(bulletHeads.data(), 0xff, bulletHeads.size() * sizeof(int));
		bulletActiveIndices.clear();

		for (int i = 0; i < count; ++i)
		{
			Vector2 const& pos = positions[i];
			int cx = Math::Clamp((int)((pos.x - minBound.x) * invCellSize), 0, gridWidth - 1);
			int cy = Math::Clamp((int)((pos.y - minBound.y) * invCellSize), 0, gridHeight - 1);

			int cellIdx = cx + cy * gridWidth;
			if (bulletHeads[cellIdx] == -1)
			{
				bulletActiveIndices.push_back(cellIdx);
			}

			bulletNextIndices[i] = bulletHeads[cellIdx];
			bulletHeads[cellIdx] = i;
			bulletCellIndices[i] = cellIdx;
		}
	}

	template< typename TFunc >
	class TFrameWork : public IQueuedWork
	{
	public:
		TFrameWork(TFunc&& func) : mFunc(std::forward<TFunc>(func)) {}
		void executeWork() override { mFunc(); }
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

		uint8* chunkStart = (uint8*)allocator.alloc(workSize * numTasks, checkAlign);
		IQueuedWork** works;
		works = (IQueuedWork**)allocator.alloc(sizeof(IQueuedWork*) * numTasks);

		for (int i = 0; i < numTasks; ++i)
		{
			int start = i * batchSize;
			int end = Math::Min(start + batchSize, count);
			TaskRunner runner = { start, end, func , taskName };
			WorkType* work = new (chunkStart + i * workSize) WorkType(std::move(runner));
			works[i] = work;
		}

		threadPool.addWorks(works , numTasks);
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
		bulletCategoryIds.clear();
		bulletQueryFlags.clear();
		bulletShapeTypes.clear();
		bulletShapeParams.clear();
	}

	void ParallelCollisionSolver::schedule(float dt, int entityCount, int bulletCount, QueueThreadPool& threadPool)
	{
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
			// 並行執行 deferred queries（在後台線程）
			solveDeferredQueries(threadPool);

			int neededSteps = (int)Math::Ceil(dt / settings.frameTargetSubStep);
			int stepCount = Math::Clamp(neededSteps, 1, settings.maxSteps);
			float stepDt = dt / (float)stepCount;

			auto start = std::chrono::high_resolution_clock::now();
			entityHitPairs.clear();
			bulletHitPairs.clear();
			bulletBulletHitPairs.clear();


			largeEntityIndices.clear();
			float thres = settings.cellSize * 0.5f;
			for (int i = 0; i < mEntityCount; ++i)
			{
				if (radii[i] > thres) largeEntityIndices.push_back(i);
			}

			for (int i = 0; i < stepCount; ++i)
			{
				solveInternal(stepDt, threadPool);
				solveQueries(threadPool);
			}

			auto Deduplicate = [](TArray<TVector2<int>>& pairs)
			{
				if (pairs.size() > 1)
				{
					std::sort(pairs.begin(), pairs.end(), [](TVector2<int> const& a, TVector2<int> const& b)
					{
						if (a.x != b.x) return a.x < b.x;
						return a.y < b.y;
					});
					auto endIt = std::unique(pairs.begin(), pairs.end(), [](TVector2<int> const& a, TVector2<int> const& b) {
						return a.x == b.x && a.y == b.y;
					});
					pairs.resize(endIt - pairs.begin());
				}
			};
			Deduplicate(entityHitPairs);
			Deduplicate(bulletHitPairs);
			Deduplicate(bulletBulletHitPairs);

			auto end = std::chrono::high_resolution_clock::now();
			mLastSolveTime = std::chrono::duration<float, std::milli>(end - start).count();
			mSolveEvent.fire();
			mIsSolving = false;
		};

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
		ParallelFor(threadPool, mTaskAllocator, "UpdateMove", mEntityCount, [=](int i)
		{
			if (!pStatic[i]) pPos[i] += pVel[i] * dt;
		}, 512);
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

		// Fast clear neighbor counts
		FMemory::Set(pNeighborCounts, 0, cellCount * sizeof(int));

		int activeCount = (int)grid.activeCellIndices.size();
		int const* pActiveIndices = grid.activeCellIndices.data();

		ParallelFor(threadPool, mTaskAllocator, "FillNeighborCache", activeCount, [=](int idx)
		{
			int i = pActiveIndices[idx];
			int cx = i % gridWidth, cy = i / gridWidth, baseIdx = i * fixedCap, currentCount = 0;
			int xMin = Math::Max(cx - 1, 0), xMax = Math::Min(cx + 1, gridWidth - 1), yMin = Math::Max(cy - 1, 0), yMax = Math::Min(cy + 1, gridHeight - 1);
			for (int ny = yMin; ny <= yMax; ++ny)
			{
				int row = ny * gridWidth;
				for (int nx = xMin; nx <= xMax; ++nx)
				{
					int otherIdx = pCellHeads[nx + row];
					while (otherIdx != -1)
					{
						if (currentCount < fixedCap) pGlobalCache[baseIdx + currentCount++] = otherIdx;
						else goto EndFill;
						otherIdx = pNextIndices[otherIdx];
					}
				}
			}
		EndFill: 
			pNeighborCounts[i] = currentCount;
			// Pad with safe indices (self index i) to prevent SIMD gather from reading garbage/invalid memory
			int padEnd = (currentCount + 7) & ~7; 
			if (padEnd > fixedCap) padEnd = fixedCap;
			int safeIdx = (currentCount > 0) ? pGlobalCache[baseIdx] : 0;
			for (int k = currentCount; k < padEnd; ++k)
			{
				pGlobalCache[baseIdx + k] = safeIdx; 
			}
		}, 512);
	}

	void ParallelCollisionSolver::solveInternal(float dt, QueueThreadPool& threadPool)
	{
		updateMove(dt, threadPool);
		grid.buildEntities(positions, mEntityCount);
		grid.buildBullets(bulletPositions, mBulletCount);
#if USE_COLLISION_COLORING
		for (int i = 0; i < 4; ++i) mColorEntities[i].clear();
		for (int i = 0; i < mEntityCount; ++i)
		{
			int idx = grid.entityCellIndices[i];
			int color = (idx % grid.gridWidth & 1) | ((idx / grid.gridWidth & 1) << 1);
			mColorEntities[color].push_back(i);
		}
#endif
		fillNeighborCache(threadPool);
		if (settings.useXPBD) solveXPBD(dt, threadPool);
		else solveLegacy(dt, threadPool);
		solveWalls(dt, threadPool);
	}

	void ParallelCollisionSolver::solveWalls(float dt, QueueThreadPool& threadPool)
	{
		if (walls.size() == 0) return;
		Vector2* __restrict pPos = positions.data();
		float* __restrict pRadii = radii.data();
		bool const* __restrict pStatic = isStatic.data();
		CollisionWall const* __restrict pWalls = walls.data();
		int wallCount = (int)walls.size();
		ParallelFor(threadPool, mTaskAllocator, "SolveWalls", mEntityCount, [=](int i)
		{
			if (pStatic[i]) return;
			Vector2 pos = pPos[i];
			float r = pRadii[i];
			for (int w = 0; w < wallCount; ++w)
			{
				Vector2 const& min = pWalls[w].min, &max = pWalls[w].max;
				Vector2 close = Vector2(Math::Clamp(pos.x, min.x, max.x), Math::Clamp(pos.y, min.y, max.y)), delta = pos - close;
				float d2 = delta.length2();
				if (d2 < r * r && d2 > 1e-8f)
				{
					float d = Math::Sqrt(d2);
					pPos[i] += (delta / d) * (r - d);
				}
				else if (d2 < 1e-8f)
				{
					float dx1 = pos.x - min.x, dx2 = max.x - pos.x, dy1 = pos.y - min.y, dy2 = max.y - pos.y;
					float mD = Math::Min(Math::Min(dx1, dx2), Math::Min(dy1, dy2));
					if (mD == dx1) pPos[i].x -= r;
					else if (mD == dx2) pPos[i].x += r;
					else if (mD == dy1) pPos[i].y -= r;
					else pPos[i].y += r;
				}
			}
		}, 1024);
	}

	void ParallelCollisionSolver::solveXPBD(float dt, QueueThreadPool& threadPool)
	{
		float alpha = settings.compliance / (Math::Max(dt, 1e-6f) * Math::Max(dt, 1e-6f));
		Vector2* __restrict pPos = positions.data();
		float* __restrict pRadii = radii.data(), * __restrict pMasses = masses.data();
		bool* __restrict pStatic = isStatic.data();
		int* __restrict pCellIdx = grid.entityCellIndices.data(), * __restrict pNCounts = cellNeighborCounts.data(), * __restrict pGCache = globalNeighborCache.data();
		int fixedCap = fixedCapacity, nCSize = (int)cellNeighborCounts.size(), largeCount = (int)largeEntityIndices.size();
		float maxPush = settings.maxPushDistance, minSep = settings.minSeparation;
		int const* pLargeIdx = largeEntityIndices.data();

		for (int iter = 0; iter < settings.iterations; ++iter)
		{
#if USE_COLLISION_COLORING
			for (int color = 0; color < 4; ++color)
			{
				int* pColorIdx = mColorEntities[color].data(), colorCount = (int)mColorEntities[color].size();
				ParallelFor(threadPool, mTaskAllocator, "SolveXPBD", colorCount, [=](int idx) {
					int i = pColorIdx[idx];
					int cellIdx = pCellIdx[i];
					if (cellIdx < 0 || cellIdx >= nCSize) return;

					Vector2 posA = pPos[i];
					float rA = pRadii[i], wA = 1.0f / pMasses[i], totalPushX = 0, totalPushY = 0;
					int startIdx = cellIdx * fixedCap, count = pNCounts[cellIdx];
					for (int n = 0; n < count; ++n)
					{
						int bIdx = pGCache[startIdx + n];
						if (i == bIdx) continue;
						Vector2 delta = posA - pPos[bIdx];
						float d2 = delta.length2(), mD = rA + pRadii[bIdx] - minSep;
						if (d2 >= mD * mD || d2 < 1e-8f) continue;
						float invD = 1.0f / Math::Sqrt(d2), lambda = (mD - d2 * invD) / (wA + 1.0f / pMasses[bIdx] + alpha);
						totalPushX += delta.x * invD * lambda * wA;
						totalPushY += delta.y * invD * lambda * wA;
					}
					for (int k = 0; k < largeCount; ++k)
					{
						int lIdx = pLargeIdx[k];
						if (i == lIdx) continue;
						Vector2 delta = posA - pPos[lIdx];
						float d2 = delta.length2(), mD = rA + pRadii[lIdx] - minSep;
						if (d2 >= mD * mD || d2 < 1e-8f) continue;
						float invD = 1.0f / Math::Sqrt(d2), lambda = (mD - d2 * invD) / (wA + 1.0f / pLargeIdx[k] + alpha);
						Vector2 push(delta.x * invD * lambda * wA, delta.y * invD * lambda * wA);
						if (push.length2() > maxPush * maxPush) push = push.normalize() * maxPush;
						totalPushX += push.x;
						totalPushY += push.y;
					}
					if (!pStatic[i])
					{
						pPos[i].x += totalPushX;
						pPos[i].y += totalPushY;
					}
				}, 512);
			}
#else
			ParallelFor(threadPool, mTaskAllocator, "SolveXPBD", mEntityCount, [=](int i)
			{
				int cellIdx = pCellIdx[i];
				if (cellIdx < 0 || cellIdx >= nCSize) return;

				Vector2 posA = pPos[i];
				float rA = pRadii[i], wA = 1.0f / pMasses[i], totalPushX = 0, totalPushY = 0;
				int startIdx = cellIdx * fixedCap, count = pNCounts[cellIdx];
				for (int n = 0; n < count; ++n)
				{
					int bIdx = pGCache[startIdx + n];
					if (i == bIdx) continue;
					Vector2 delta = posA - pPos[bIdx];
					float d2 = delta.length2(), mD = rA + pRadii[bIdx] - minSep;
					if (d2 >= mD * mD || d2 < 1e-8f) continue;
					float invD = 1.0f / Math::Sqrt(d2), lambda = (mD - d2 * invD) / (wA + 1.0f / pMasses[bIdx] + alpha);
					totalPushX += delta.x * invD * lambda * wA;
					totalPushY += delta.y * invD * lambda * wA;
				}
				for (int k = 0; k < largeCount; ++k)
				{
					int lIdx = pLargeIdx[k];
					if (i == lIdx) continue;
					Vector2 delta = posA - pPos[lIdx];
					float d2 = delta.length2(), mD = rA + pRadii[lIdx] - minSep;
					if (d2 >= mD * mD || d2 < 1e-8f) continue;
					float invD = 1.0f / Math::Sqrt(d2), lambda = (mD - d2 * invD) / (wA + 1.0f / pMasses[lIdx] + alpha);
					float pX = delta.x * invD * lambda * wA;
					float pY = delta.y * invD * lambda * wA;
					if (pX*pX + pY*pY > maxPush * maxPush) {
						float f = maxPush / Math::Sqrt(pX*pX + pY*pY);
						pX *= f; pY *= f;
					}
					totalPushX += pX;
					totalPushY += pY;
				}
				if (!pStatic[i])
				{
					pPos[i].x += totalPushX;
					pPos[i].y += totalPushY;
				}
			}, 512);
#endif
		}
	}

	void ParallelCollisionSolver::solveLegacy(float dt, QueueThreadPool& threadPool)
	{
		Vector2* __restrict pPos = positions.data();
		float* __restrict pRadii = radii.data(), * __restrict pMasses = masses.data();
		bool* __restrict pStatic = isStatic.data();
		int* __restrict pCellIdx = grid.entityCellIndices.data(), * __restrict pNCounts = cellNeighborCounts.data(), * __restrict pGCache = globalNeighborCache.data();
		int fixedCap = fixedCapacity, nCSize = (int)cellNeighborCounts.size(), largeCount = (int)largeEntityIndices.size();
		float relax = settings.relaxation, maxPush = settings.maxPushDistance, minSep = settings.minSeparation;
		int const* pLargeIdx = largeEntityIndices.data();

		for (int iter = 0; iter < settings.iterations; ++iter)
		{
#if USE_COLLISION_COLORING
			for (int color = 0; color < 4; ++color)
			{
				int* pColorIdx = mColorEntities[color].data(), colorCount = (int)mColorEntities[color].size();
				ParallelFor(threadPool, mTaskAllocator, "SolveCollision", colorCount, [=](int idx) {
					int i = pColorIdx[idx];
					int cellIdx = pCellIdx[i];
					if (cellIdx < 0 || cellIdx >= nCSize) return;

					Vector2 posA = pPos[i];
					float rA = pRadii[i], mA = pMasses[i], totalPushX = 0, totalPushY = 0;
					int startIdx = cellIdx * fixedCap, count = pNCounts[cellIdx];
					for (int n = 0; n < count; ++n)
					{
						int bIdx = pGCache[startIdx + n];
						if (i == bIdx) continue;
						Vector2 delta = posA - pPos[bIdx];
						float d2 = delta.length2(), mD = rA + pRadii[bIdx] - minSep;
						if (d2 >= mD * mD || d2 < 1e-8f) continue;
						float invD = 1.0f / Math::Sqrt(d2), ov = Math::Min((mD - d2 * invD) * relax, maxPush);
						float f = ov * invD * (pMasses[bIdx] / (mA + pMasses[bIdx]));
						totalPushX += delta.x * f;
						totalPushY += delta.y * f;
					}
					for (int k = 0; k < largeCount; ++k)
					{
						int lIdx = pLargeIdx[k];
						if (i == lIdx) continue;
						Vector2 delta = posA - pPos[lIdx];
						float d2 = delta.length2(), mD = rA + pRadii[lIdx] - minSep;
						if (d2 >= mD * mD || d2 < 1e-8f) continue;
						float invD = 1.0f / Math::Sqrt(d2), ov = Math::Min((mD - d2 * invD) * relax, maxPush);
						float f = ov * invD * (pMasses[lIdx] / (mA + pMasses[lIdx]));
						totalPushX += delta.x * f;
						totalPushY += delta.y * f;
					}
					if (!pStatic[i])
					{
						pPos[i].x += totalPushX;
						pPos[i].y += totalPushY;
					}
				}, 512);
			}
#else
			ParallelFor(threadPool, mTaskAllocator, "SolveCollision", mEntityCount, [=](int i)
			{
#if !USE_SIMD_LEGACY
				int cellIdx = pCellIdx[i];
				if (cellIdx < 0 || cellIdx >= nCSize) return;

				Vector2 posA = pPos[i];
				float rA = pRadii[i], mA = pMasses[i], totalPushX = 0, totalPushY = 0;
				int startIdx = cellIdx * fixedCap, count = pNCounts[cellIdx];
				for (int n = 0; n < count; ++n)
				{
					int bIdx = pGCache[startIdx + n];
					if (i == bIdx) continue;
					Vector2 delta = posA - pPos[bIdx];
					float d2 = delta.length2(), mD = rA + pRadii[bIdx] - minSep;
					if (d2 >= mD * mD || d2 < 1e-8f) continue;
					float invD = 1.0f / Math::Sqrt(d2), ov = Math::Min((mD - d2 * invD) * relax, maxPush);
					float f = ov * invD * (pMasses[bIdx] / (mA + pMasses[bIdx]));
					totalPushX += delta.x * f;
					totalPushY += delta.y * f;
				}
				for (int k = 0; k < largeCount; ++k)
				{
					int lIdx = pLargeIdx[k];
					if (i == lIdx) continue;
					Vector2 delta = posA - pPos[lIdx];
					float d2 = delta.length2(), mD = rA + pRadii[lIdx] - minSep;
					if (d2 >= mD * mD || d2 < 1e-8f) continue;
					float invD = 1.0f / Math::Sqrt(d2), ov = Math::Min((mD - d2 * invD) * relax, maxPush);
					float f = ov * invD * (pMasses[lIdx] / (mA + pMasses[lIdx]));
					totalPushX += delta.x * f;
					totalPushY += delta.y * f;
				}
				if (!pStatic[i])
				{
					pPos[i].x += totalPushX;
					pPos[i].y += totalPushY;
				}
#else
				// SIMD Path
				int cellIdx = pCellIdx[i];
				if (cellIdx < 0 || cellIdx >= nCSize) return;

				FloatVector vPosX_A( (float)pPos[i].x ), vPosY_A( (float)pPos[i].y ), vRadii_A( (float)pRadii[i] ), vMass_A( (float)pMasses[i] );
				FloatVector vTPX(0.0f), vTPY(0.0f), vRelax(relax), vMaxP(maxPush), vMinS(minSep), vEps(1e-8f), vRamp;
				for (int k = 0; k < (int)FloatVector::Size; ++k) vRamp[k] = (float)k;
				int startIdx = cellIdx * fixedCap, count = pNCounts[cellIdx];
				for (int n = 0; n < count; n += (int)FloatVector::Size)
				{
					IntVector vIdx = IntVector::load(pGCache + startIdx + n);
					FloatVector vPosX_B = FloatVector::gather<8>((float*)pPos, vIdx), vPosY_B = FloatVector::gather<8>((float*)pPos + 1, vIdx), vRadii_B = FloatVector::gather<4>(pRadii, vIdx);
					auto vValid = (vRamp + FloatVector((float)n)) < FloatVector((float)count);
					FloatVector vDX = vPosX_A - vPosX_B, vDY = vPosY_A - vPosY_B, vD2 = vDX * vDX + vDY * vDY, vMD = vRadii_A + vRadii_B - vMinS;
					auto vColl = (vD2 < vMD * vMD) & (vD2 > vEps) & vValid;
					if (!vColl.isAnyActive()) continue;
					FloatVector vInvD = SIMD::rsqrt(vD2), vOv = SIMD::min((vMD - vD2 * vInvD) * vRelax, vMaxP);
					FloatVector vF = vInvD * vOv * (FloatVector::gather<4>(pMasses, vIdx) / (vMass_A + FloatVector::gather<4>(pMasses, vIdx)));
					FloatVector vZero(0.0f);
					vTPX = vTPX + SIMD::select(vColl, vDX * vF, vZero);
					vTPY = vTPY + SIMD::select(vColl, vDY * vF, vZero);
				}
				Vector2 totalPush(vTPX.sum(), vTPY.sum());
				for (int k = 0; k < largeCount; ++k)
				{
					int lIdx = pLargeIdx[k];
					if (i == lIdx) continue;
					Vector2 delta = pPos[i] - pPos[lIdx];
					float d2 = delta.length2(), mD = pRadii[i] + pRadii[lIdx] - minSep;
					if (d2 < mD * mD && d2 > 1e-8f)
					{
						float invD = 1.0f / Math::Sqrt(d2), ov = Math::Min((mD - d2 * invD) * relax, maxPush);
						totalPush += delta * (invD * ov * (pMasses[lIdx] / (pMasses[i] + pMasses[lIdx])));
					}
				}
				if (!pStatic[i]) pPos[i] += totalPush;
#endif
			}, 512);
#endif
		}
	}

	void ParallelCollisionSolver::solveDeferredQueries(QueueThreadPool& threadPool)
	{
		deferredQueryResults.clear();
		if (deferredQueries.empty()) return;

		deferredQueryResults.resize(deferredQueries.size());
		int queryCount = (int)deferredQueries.size();
		DeferredQueryRequest const* pQueries = deferredQueries.data();
		DeferredQueryResult* pResults = deferredQueryResults.data();

		ParallelFor(threadPool, mTaskAllocator, "DeferredQueries", queryCount, [this, pQueries, pResults](int idx)
		{
			auto const& query = pQueries[idx];
			auto& result = pResults[idx];
			queryEntities(query.position, query.rotation, query.shapeType, query.shapeParams, query.targetMask, result.results);
			result.callback = query.callback;
			result.userData = query.userData;
		}, 1);
		deferredQueries.clear();
	}

	int ParallelCollisionSolver::queryEntities(Vector2 const& pos, float rot, ShapeType type, Vector3 const& params, int mask, TArray<int>& results)
	{
		int cx = Math::Clamp((int)((pos.x - grid.minBound.x) * grid.invCellSize), 0, grid.gridWidth - 1);
		int cy = Math::Clamp((int)((pos.y - grid.minBound.y) * grid.invCellSize), 0, grid.gridHeight - 1);
		for (int ny = Math::Max(cy - 1, 0); ny <= Math::Min(cy + 1, grid.gridHeight - 1); ++ny)
		{
			int row = ny * grid.gridWidth;
			for (int nx = Math::Max(cx - 1, 0); nx <= Math::Min(cx + 1, grid.gridWidth - 1); ++nx)
			{
				int idx = grid.cellHeads[nx + row];
				while (idx != -1)
				{
					if ((mask & categoryIds[idx]) != 0)
					{
						bool hit = (type == ShapeType::Circle) ? (pos - positions[idx]).length2() < (params.x + radii[idx]) * (params.x + radii[idx]) : IntersectionTests::Intersect(pos, 0, type, params, positions[idx], radii[idx]);
						if (hit) results.push_back(idx);
					}
					idx = grid.nextIndices[idx];
				}
			}
		}
		for (int idx : largeEntityIndices)
		{
			if ((mask & categoryIds[idx]) != 0 && IntersectionTests::Intersect(pos, 0, type, params, positions[idx], radii[idx]))
			{
				results.push_back(idx);
			}
		}
		return (int)results.size();
	}

	void ParallelCollisionSolver::solveQueries(QueueThreadPool& threadPool)
	{
		std::mutex entityHitMutex, bulletHitMutex, bulletBulletHitMutex;
		const int batch = 1024;
		int const* pLIdx = largeEntityIndices.data();
		int lCount = (int)largeEntityIndices.size();

		ParallelFor(threadPool, mTaskAllocator, "EntityQuery", (mEntityCount + batch - 1) / batch, [=, &entityHitMutex](int tId)
		{
			int start = tId * batch, end = Math::Min(start + batch, mEntityCount);
			TArray<TVector2<int>, TInlineAllocator<16>> hits;
			for (int i = start; i < end; ++i)
			{
				if (!queryFlags[i]) continue;
				int cIdx = grid.entityCellIndices[i];
				if (cIdx < 0 || cIdx >= (int)cellNeighborCounts.size()) continue;

				Vector2 pA = positions[i];
				float rA = radii[i];
				int mA = targetMasks[i], sIdx = cIdx * fixedCapacity, nCount = cellNeighborCounts[cIdx];
				for (int k = 0; k < nCount; ++k)
				{
					int b = globalNeighborCache[sIdx + k];
					if (i != b && (mA & categoryIds[b]) != 0 && (pA - positions[b]).length2() < (rA + radii[b]) * (rA + radii[b]))
					{
						hits.emplace_back(i, b);
					}
				}
				for (int k = 0; k < lCount; ++k)
				{
					int l = pLIdx[k];
					if (i != l && (mA & categoryIds[l]) != 0 && (pA - positions[l]).length2() < (rA + radii[l]) * (rA + radii[l]))
					{
						hits.emplace_back(i, l);
					}
				}
			}
			if (!hits.empty())
			{
				std::lock_guard<std::mutex> lock(entityHitMutex);
				entityHitPairs.append(hits.begin(), hits.end());
			}
		}, 1);

		if (mBulletCount > 0)
		{
			ParallelFor(threadPool, mTaskAllocator, "BulletQuery", (mBulletCount + batch - 1) / batch, [this, batch, &bulletHitMutex, &bulletBulletHitMutex, pLIdx, lCount](int tId)
			{
				int start = tId * batch, end = Math::Min(start + batch, mBulletCount);
				TArray<TVector2<int>, TInlineAllocator<16>> bhits, bbhits;
				for (int i = start; i < end; ++i)
				{
					Vector2 bP = bulletPositions[i];
					float bR = bulletRadii[i];
					int bM = bulletTargetMasks[i], bC = bulletCategoryIds[i];
					int cx = Math::Clamp((int)((bP.x - grid.minBound.x) * grid.invCellSize), 0, grid.gridWidth - 1), cy = Math::Clamp((int)((bP.y - grid.minBound.y) * grid.invCellSize), 0, grid.gridHeight - 1);
					for (int ny = Math::Max(cy - 1, 0); ny <= Math::Min(cy + 1, grid.gridHeight - 1); ++ny)
					{
						int row = ny * grid.gridWidth;
						for (int nx = Math::Max(cx - 1, 0); nx <= Math::Min(cx + 1, grid.gridWidth - 1); ++nx)
						{
							int eIdx = grid.cellHeads[nx + row];
							while (eIdx != -1)
							{
								if ((bM & categoryIds[eIdx]) != 0 && (bP - positions[eIdx]).length2() < (bR + radii[eIdx]) * (bR + radii[eIdx]))
								{
									bhits.emplace_back(i, eIdx);
								}
								eIdx = grid.nextIndices[eIdx];
							}
						}
					}
					for (int k = 0; k < lCount; ++k)
					{
						int lIdx = pLIdx[k];
						if ((bM & categoryIds[lIdx]) != 0 && (bP - positions[lIdx]).length2() < (bR + radii[lIdx]) * (bR + radii[lIdx]))
						{
							bhits.emplace_back(i, lIdx);
						}
					}

					if (bulletQueryFlags[i])
					{
						int bcx = Math::Clamp((int)((bP.x - grid.minBound.x) * grid.invCellSize), 0, grid.gridWidth - 1);
						int bcy = Math::Clamp((int)((bP.y - grid.minBound.y) * grid.invCellSize), 0, grid.gridHeight - 1);
						for (int bny = Math::Max(bcy - 1, 0); bny <= Math::Min(bcy + 1, grid.gridHeight - 1); ++bny)
						{
							int brow = bny * grid.gridWidth;
							for (int bnx = Math::Max(bcx - 1, 0); bnx <= Math::Min(bcx + 1, grid.gridWidth - 1); ++bnx)
							{
								int bIdx = grid.bulletHeads[bnx + brow];
								while (bIdx != -1)
								{
									// Only check pairs where i < bIdx to avoid duplicate pairs
									if (i < bIdx && ((bM & bulletCategoryIds[bIdx]) != 0 || (bulletTargetMasks[bIdx] & bC) != 0))
									{
										float rSum = bR + bulletRadii[bIdx];
										if ((bP - bulletPositions[bIdx]).length2() < rSum * rSum) bbhits.emplace_back(i, bIdx);
									}
									bIdx = grid.bulletNextIndices[bIdx];
								}
							}
						}
					}
				}
				if (!bhits.empty()) { std::lock_guard<std::mutex> lock(bulletHitMutex); bulletHitPairs.append(bhits.begin(), bhits.end()); }
				if (!bbhits.empty()) { std::lock_guard<std::mutex> lock(bulletBulletHitMutex); bulletBulletHitPairs.append(bbhits.begin(), bbhits.end()); }
			}, 1);
		}
	}

	ParallelCollisionManager::ParallelCollisionManager(QueueThreadPool& threadPool) : mThreadPool(threadPool)
	{
		mActiveBitset.resize(1000, false);
		mReceivers.resize(1000);
		mBulletReceivers.resize(1000);
		mEntityDataBuffer.resize(1000);
		mBulletDataBuffer.resize(1000);
	}

	ParallelCollisionManager::~ParallelCollisionManager()
	{
		cleanup();
		for (auto* p : mEventDataPool) delete p;
		mEventDataPool.clear();
	}

	void ParallelCollisionManager::init(Vector2 const& min, Vector2 const& max, float cellSize)
	{
		mSolver.grid.init(min, max, cellSize, 1000, 1000);
		mSolver.settings.cellSize = cellSize;
	}

	void ParallelCollisionManager::addWall(Vector2 const& min, Vector2 const& max)
	{
		mSolver.walls.push_back({ min, max });
	}

	void ParallelCollisionManager::clearWalls()
	{
		mSolver.walls.clear();
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
		mReceivers[id].eventHandle = allocEntityEventHandle(entity);
		if (data.bQueryCollision)
		{
			if (!mReceivers[id].eventData) mReceivers[id].eventData = allocEventData();
			mReceivers[id].eventData->clearCurrent();
		}
		else if (mReceivers[id].eventData)
		{
			freeEventData(mReceivers[id].eventData);
			mReceivers[id].eventData = nullptr;
		}
		mReceivers[id].isPendingRemove = false;
		entity->mPhysicsId = id;
		return id;
	}

	void ParallelCollisionManager::unregisterEntity(IPhysicsEntity* entity)
	{
		int id = entity->mPhysicsId;
		if (id >= 0 && id < mEntityCount)
		{
			freeEntityEventHandle(mReceivers[id].eventHandle);
			mReceivers[id].entity->mPhysicsId = -1;
			if (mbDataLocked)
			{
				if (!mReceivers[id].isPendingRemove)
				{
					mReceivers[id].entity = nullptr; 
					mReceivers[id].isPendingRemove = true;
					mPendingRemoveEntityIds.push_back(id);
				}
			}
			else doUnregisterEntity(id);
		}
	}

	void ParallelCollisionManager::doUnregisterEntity(int id)
	{
		int lastIdx = mEntityCount - 1;

		if (id != lastIdx)
		{
			mEntityDataBuffer[id] = mEntityDataBuffer[lastIdx];
			mReceivers[id].entity = mReceivers[lastIdx].entity;
			mReceivers[id].eventHandle = mReceivers[lastIdx].eventHandle;

			if (mReceivers[id].eventData) 
				freeEventData(mReceivers[id].eventData);

			mReceivers[id].eventData = mReceivers[lastIdx].eventData;
			mReceivers[lastIdx].eventData = nullptr;
			mReceivers[id].isPendingRemove = mReceivers[lastIdx].isPendingRemove;

			if (mReceivers[id].entity) 
				mReceivers[id].entity->mPhysicsId = id;
			mLastActiveEntityIndices.erase(id);
			if (mLastActiveEntityIndices.erase(lastIdx)) 
				mLastActiveEntityIndices.insert(id);
		}
		else
		{
			if (mReceivers[id].eventData)
			{
				freeEventData(mReceivers[id].eventData);
				mReceivers[id].eventData = nullptr;
			}
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
		mBulletReceivers[id].eventHandle = allocBulletEventHandle(bullet);
		if (data.bQueryBulletCollision)
		{
			if (!mBulletReceivers[id].eventData) mBulletReceivers[id].eventData = allocEventData();
			mBulletReceivers[id].eventData->clearCurrent();
		}
		else if (mBulletReceivers[id].eventData)
		{
			freeEventData(mBulletReceivers[id].eventData);
			mBulletReceivers[id].eventData = nullptr;
		}
		bullet->mBulletId = id;
		return id;
	}

	void ParallelCollisionManager::unregisterBullet(IPhysicsBullet* bullet)
	{
		int id = bullet->mBulletId;
		if (id >= 0 && id < mBulletCount)
		{
			freeBulletEventHandle(mBulletReceivers[id].eventHandle);
			mBulletReceivers[id].bullet->mBulletId = -1;
			if (mbDataLocked) 
			{
				mBulletReceivers[id].bullet = nullptr;
				mBulletReceivers[id].isPendingRemove = true;
				mPendingRemoveBulletIds.push_back(id);
			}
			else doUnregisterBullet(id);
		}
	}

	void ParallelCollisionManager::doUnregisterBullet(int id)
	{
		int lastIdx = mBulletCount - 1;

		if (id != lastIdx)
		{
			mBulletDataBuffer[id] = mBulletDataBuffer[lastIdx];
			if (mBulletReceivers[id].eventData) 
				freeEventData(mBulletReceivers[id].eventData);

			mBulletReceivers[id] = mBulletReceivers[lastIdx];
			mBulletReceivers[lastIdx].eventData = nullptr;
			mBulletReceivers[id].isPendingRemove = mBulletReceivers[lastIdx].isPendingRemove;

			if (mBulletReceivers[id].bullet) 
				mBulletReceivers[id].bullet->mBulletId = id;

			mLastActiveBulletIndices.erase(id);

			if (mLastActiveBulletIndices.erase(lastIdx)) 
				mLastActiveBulletIndices.insert(id);
		}
		else
		{
			if (mBulletReceivers[id].eventData)
			{
				freeEventData(mBulletReceivers[id].eventData);
				mBulletReceivers[id].eventData = nullptr;
			}
			mLastActiveBulletIndices.erase(id);
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
		for (auto& r : mReceivers)
		{
			if (r.entity) r.entity->mPhysicsId = -1;
			r.entity = nullptr;
			if (r.eventData)
			{
				freeEventData(r.eventData);
				r.eventData = nullptr;
			}
		}
		for (auto& r : mBulletReceivers)
		{
			if (r.bullet) r.bullet->mBulletId = -1;
			r.bullet = nullptr;
			if (r.eventData)
			{
				freeEventData(r.eventData);
				r.eventData = nullptr;
			}
		}
		for (int i = 0; i < (int)mActiveBitset.size(); ++i)
		{
			mActiveBitset[i] = false;
		}
		mActiveList.clear();
		mLastActiveEntityIndices.clear();
		mLastActiveBulletIndices.clear();
		mPendingRemoveEntityIds.clear();
		mPendingRemoveBulletIds.clear();
	}

	void ParallelCollisionManager::syncFromSimulator()
	{
		PROFILE_FUNCTION();
		int numE = Math::Min((int)mSolver.positions.size(), mEntityCount);
		for (int i = 0; i < numE; ++i)
		{
			mEntityDataBuffer[i].position = mSolver.positions[i];
		}
		
		int numB = Math::Min((int)mSolver.bulletPositions.size(), mBulletCount);
		for (int i = 0; i < numB; ++i)
		{
			mBulletDataBuffer[i].position = mSolver.bulletPositions[i];
		}
	}

	void ParallelCollisionManager::syncToSolver()
	{
		PROFILE_FUNCTION();
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
			auto const& d = mEntityDataBuffer[i];
			mSolver.positions[i] = d.position;
			mSolver.velocities[i] = d.velocity;
			mSolver.radii[i] = d.radius;
			mSolver.masses[i] = d.mass;
			mSolver.isStatic[i] = d.isStatic;
			mSolver.categoryIds[i] = d.categoryId;
			mSolver.targetMasks[i] = d.targetMask;
			mSolver.queryFlags[i] = d.bQueryCollision;
		}
		if (mBulletCount > (int)mSolver.bulletPositions.size())
		{
			mSolver.bulletPositions.resize(mBulletCount);
			mSolver.bulletVelocities.resize(mBulletCount);
			mSolver.bulletRadii.resize(mBulletCount);
			mSolver.bulletTargetMasks.resize(mBulletCount);
			mSolver.bulletCategoryIds.resize(mBulletCount);
			mSolver.bulletQueryFlags.resize(mBulletCount);
			mSolver.bulletShapeTypes.resize(mBulletCount);
			mSolver.bulletShapeParams.resize(mBulletCount);
		}
		for (int i = 0; i < mBulletCount; ++i)
		{
			auto const& d = mBulletDataBuffer[i];
			mSolver.bulletPositions[i] = d.position;
			mSolver.bulletVelocities[i] = d.velocity;
			
			float radius = 0;
			switch (d.shapeType)
			{
			case ShapeType::Circle:
				radius = d.shapeParams.x;
				break;
			case ShapeType::Rect:
				radius = Math::Sqrt(d.shapeParams.x * d.shapeParams.x + d.shapeParams.y * d.shapeParams.y) * 0.5f;
				break;
			case ShapeType::Arc:
				radius = d.shapeParams.x;
				break;
			}
			mSolver.bulletRadii[i] = radius;
			mSolver.bulletTargetMasks[i] = d.targetMask;
			mSolver.bulletCategoryIds[i] = d.categoryId;
			mSolver.bulletQueryFlags[i] = d.bQueryBulletCollision;
			mSolver.bulletShapeTypes[i] = d.shapeType;
			mSolver.bulletShapeParams[i] = d.shapeParams;
		}
	}

	void ParallelCollisionManager::processPendingRemovals()
	{
		PROFILE_FUNCTION();
		if (!mPendingRemoveEntityIds.empty())
		{
			std::sort(mPendingRemoveEntityIds.begin(), mPendingRemoveEntityIds.end(), std::greater<int>());
			int prev = -1;
			for (int id : mPendingRemoveEntityIds)
			{
				if (id != prev) doUnregisterEntity(id);
				prev = id;
			}
			mPendingRemoveEntityIds.clear();
		}
		if (!mPendingRemoveBulletIds.empty())
		{
			std::sort(mPendingRemoveBulletIds.begin(), mPendingRemoveBulletIds.end(), std::greater<int>());
			int prev = -1;
			for (int id : mPendingRemoveBulletIds)
			{
				if (id != prev) doUnregisterBullet(id);
				prev = id;
			}
			mPendingRemoveBulletIds.clear();
		}
	}

	void ParallelCollisionManager::processDeferredQueries()
	{
		PROFILE_FUNCTION();
		TArray<IPhysicsEntity*> entityResults;
		for (auto& result : mSolver.deferredQueryResults)
		{
			if (result.callback)
			{
				entityResults.clear();
				for (int idx : result.results)
				{
					entityResults.push_back(mReceivers[idx].entity);
				}
				result.callback(entityResults, result.userData);
			}
		}
		mSolver.deferredQueryResults.clear();
	}

	int ParallelCollisionManager::allocEntityEventHandle(IPhysicsEntity* entity)
	{
		int id = -1;
		// Only reuse indices when not in the middle of simulation/event processing
		if (!mbDataLocked && !mFreeEntityEventHandleIndices.empty())
		{
			id = mFreeEntityEventHandleIndices.back();
			mFreeEntityEventHandleIndices.pop_back();
		}
		
		if (id == -1)
		{
			id = (int)mStableEntityMap.size();
			mStableEntityMap.resize(id + 1);
			mStableEntityMap[id].version = 1;
		}

		mStableEntityMap[id].ptr = entity;
		return id | (mStableEntityMap[id].version << HandleIndexBits);
	}

	void ParallelCollisionManager::freeEntityEventHandle(int handle)
	{
		int id = handle & HandleIndexMask;
		int ver = handle >> HandleIndexBits;
		if (id < mStableEntityMap.size() && mStableEntityMap[id].version == ver)
		{
			mStableEntityMap[id].ptr = nullptr;
			mStableEntityMap[id].version++;
			mFreeEntityEventHandleIndices.push_back(id);
		}
	}

	int  ParallelCollisionManager::allocBulletEventHandle(IPhysicsBullet* bullet)
	{
		int id = -1;
		if (!mbDataLocked && !mFreeBulletEventHandleIndices.empty())
		{
			id = mFreeBulletEventHandleIndices.back();
			mFreeBulletEventHandleIndices.pop_back();
		}

		if (id == -1)
		{
			id = (int)mStableBulletMap.size();
			mStableBulletMap.resize(id + 1);
			mStableBulletMap[id].version = 1;
		}

		mStableBulletMap[id].ptr = bullet;
		return id | (mStableBulletMap[id].version << HandleIndexBits);
	}

	void ParallelCollisionManager::freeBulletEventHandle(int handle)
	{
		int id = handle & HandleIndexMask;
		int ver = handle >> HandleIndexBits;
		if (id < mStableBulletMap.size() && mStableBulletMap[id].version == ver)
		{
			mStableBulletMap[id].ptr = nullptr;
			mStableBulletMap[id].version++;
			mFreeBulletEventHandleIndices.push_back(id);
		}
	}

	IPhysicsEntity* ParallelCollisionManager::getEntityFromHandle(int handle)
	{
		int id = handle & HandleIndexMask;
		int ver = handle >> HandleIndexBits;
		if (id < mStableEntityMap.size() && mStableEntityMap[id].version == ver)
			return mStableEntityMap[id].ptr;
		return nullptr;
	}

	IPhysicsBullet* ParallelCollisionManager::getBulletFromHandle(int handle)
	{
		int id = handle & HandleIndexMask;
		int ver = handle >> HandleIndexBits;
		if (id < mStableBulletMap.size() && mStableBulletMap[id].version == ver)
			return mStableBulletMap[id].ptr;
		return nullptr;
	}

	void ParallelCollisionManager::beginFrame()
	{
		if (mbDataLocked)
		{
			mSolver.ensureComplete();
			syncFromSimulator();
			processCollisionEvents();
			processDeferredQueries();
			mbDataLocked = false;
			processPendingRemovals();
		}
	}

	void ParallelCollisionManager::processCollisionEvents()
	{
		PROFILE_FUNCTION();
		mActiveList.clear();
		for (int i = 0; i < (int)mActiveBitset.size(); ++i)
		{
			mActiveBitset[i] = false;
		}
		auto Activate = [&](int id, bool isBullet)
		{
			int fullId = isBullet ? (id + mEntityCount) : id;
			if (fullId >= (int)mActiveBitset.size()) mActiveBitset.resize(fullId + 1, false);
			if (!mActiveBitset[fullId])
			{
				mActiveBitset[fullId] = true;
				mActiveList.push_back(fullId);
			}
		};

		for (auto const& p : mSolver.entityHitPairs)
		{
			if (p.x < mEntityCount && p.y < mEntityCount)
			{
				if (mReceivers[p.x].eventData) mReceivers[p.x].eventData->currEntities.insert(mReceivers[p.y].eventHandle);
				Activate(p.x, false);
			}
		}
		for (auto const& p : mSolver.bulletHitPairs)
		{
			if (p.x < mBulletCount && p.y < mEntityCount)
			{
				if (mReceivers[p.y].eventData) mReceivers[p.y].eventData->currBullets.insert(mBulletReceivers[p.x].eventHandle);
				Activate(p.y, false);
				if (mBulletReceivers[p.x].eventData) mBulletReceivers[p.x].eventData->currEntities.insert(mReceivers[p.y].eventHandle);
				Activate(p.x, true);
			}
		}
		for (auto const& pair : mSolver.bulletBulletHitPairs)
		{
			if (pair.x < mBulletCount && pair.y < mBulletCount)
			{
				if (mBulletReceivers[pair.x].eventData) mBulletReceivers[pair.x].eventData->currBullets.insert(mBulletReceivers[pair.y].eventHandle);
				Activate(pair.x, true);
				if (mBulletReceivers[pair.y].eventData) mBulletReceivers[pair.y].eventData->currBullets.insert(mBulletReceivers[pair.x].eventHandle);
				Activate(pair.y, true);
			}
		}
		for (int idx : mLastActiveEntityIndices) Activate(idx, false);
		for (int idx : mLastActiveBulletIndices) Activate(idx, true);
		mLastActiveEntityIndices.clear();
		mLastActiveBulletIndices.clear();

		for (int id : mActiveList)
		{
			bool isB = (id >= mEntityCount);
			int actualId = isB ? (id - mEntityCount) : id;
			if (isB)
			{
				auto& d = mBulletReceivers[actualId];
				if (!d.bullet || !d.eventData) 
                    continue;
				
                auto& ev = *d.eventData;
				for (int otherHandle : ev.currEntities)
				{
					IPhysicsEntity* other = getEntityFromHandle(otherHandle);
					if (!other) continue;
					if (other->mPhysicsId < 0 || other->mPhysicsId >= mEntityCount) continue;

					if (ev.prevEntities.count(otherHandle)) d.bullet->handleCollisionStay(other, mSolver.categoryIds[other->mPhysicsId]);
					else d.bullet->handleCollisionEnter(other, mSolver.categoryIds[other->mPhysicsId]);
				}
				for (int prevHandle : ev.prevEntities)
				{
					IPhysicsEntity* prev = getEntityFromHandle(prevHandle);
					if (ev.currEntities.count(prevHandle) == 0) 
					{
						if (prev && prev->mPhysicsId >= 0 && prev->mPhysicsId < mEntityCount)
							d.bullet->handleCollisionExit(prev, mSolver.categoryIds[prev->mPhysicsId]);
					}
				}
				for (int bulletHandle : ev.currBullets)
				{
					IPhysicsBullet* bullet = getBulletFromHandle(bulletHandle);
					if (!bullet) continue;
					if (bullet->mBulletId < 0 || bullet->mBulletId >= mBulletCount) continue;

					if (ev.prevBullets.count(bulletHandle)) d.bullet->handleCollisionStay(bullet, mSolver.bulletCategoryIds[bullet->mBulletId]);
					else d.bullet->handleCollisionEnter(bullet, mSolver.bulletCategoryIds[bullet->mBulletId]);
				}
				for (int prevHandle : ev.prevBullets)
				{
					IPhysicsBullet* prev = getBulletFromHandle(prevHandle);
					if (ev.currBullets.count(prevHandle) == 0) 
					{
						if (prev && prev->mBulletId >= 0 && prev->mBulletId < mBulletCount)
							d.bullet->handleCollisionExit(prev, mSolver.bulletCategoryIds[prev->mBulletId]);
					}
				}
			}
			else
			{
				auto& d = mReceivers[actualId];
				if (!d.entity || d.isPendingRemove || !d.eventData) 
					continue;
				
				auto& ev = *d.eventData;
				for (int otherHandle : ev.currEntities)
				{
					IPhysicsEntity* other = getEntityFromHandle(otherHandle);
					if (!other) continue;
					if (other->mPhysicsId < 0 || other->mPhysicsId >= mEntityCount) continue;

					if (ev.prevEntities.count(otherHandle)) d.entity->handleCollisionStay(other, mSolver.categoryIds[other->mPhysicsId]);
					else d.entity->handleCollisionEnter(other, mSolver.categoryIds[other->mPhysicsId]);
				}
				for (int prevHandle : ev.prevEntities)
				{
					IPhysicsEntity* prev = getEntityFromHandle(prevHandle);
					if (ev.currEntities.count(prevHandle) == 0) 
					{
						if (prev && prev->mPhysicsId >= 0 && prev->mPhysicsId < mEntityCount)
							d.entity->handleCollisionExit(prev, mSolver.categoryIds[prev->mPhysicsId]);
					}
				}
				for (int bulletHandle : ev.currBullets)
				{
					IPhysicsBullet* bullet = getBulletFromHandle(bulletHandle);
					if (!bullet) continue;
					if (bullet->mBulletId < 0 || bullet->mBulletId >= mBulletCount) continue;

					if (ev.prevBullets.count(bulletHandle)) d.entity->handleCollisionStay(bullet, mSolver.bulletCategoryIds[bullet->mBulletId]);
					else d.entity->handleCollisionEnter(bullet, mSolver.bulletCategoryIds[bullet->mBulletId]);
				}
				for (int prevHandle : ev.prevBullets)
				{
					IPhysicsBullet* prev = getBulletFromHandle(prevHandle);
					if (ev.currBullets.count(prevHandle) == 0) 
					{
						if (prev && prev->mBulletId >= 0 && prev->mBulletId < mBulletCount)
							d.entity->handleCollisionExit(prev, mSolver.bulletCategoryIds[prev->mBulletId]);
					}
				}
			}
		}

		for (int id : mActiveList)
		{
			bool isB = (id >= mEntityCount);
			int actualId = isB ? (id - mEntityCount) : id;
			if (isB)
			{
				if (actualId < mBulletCount && mBulletReceivers[actualId].eventData)
				{
					auto& ev = *mBulletReceivers[actualId].eventData;
					if (!ev.currEntities.empty() || !ev.currBullets.empty()) 
                        mLastActiveBulletIndices.insert(actualId);
					ev.swapSets();
				}
			}
			else
			{
				if (actualId < mEntityCount && mReceivers[actualId].eventData)
				{
					auto& ev = *mReceivers[actualId].eventData;
					if (!ev.currEntities.empty() || !ev.currBullets.empty()) 
                        mLastActiveEntityIndices.insert(actualId);
					ev.swapSets();
				}
			}
		}
	}

	void ParallelCollisionManager::endFrame(float dt)
	{
		mbDataLocked = true;
		syncToSolver();
		
		// 啟動物理模擬（異步）
		mSolver.schedule(dt, mEntityCount, mBulletCount, mThreadPool);
	}

	int ParallelCollisionManager::queryEntities(Vector2 const& pos, float rot, ShapeType type, Vector3 const& params, int mask, TArray<IPhysicsEntity*>& results)
	{
		mSolver.ensureComplete();
		int cx = Math::Clamp((int)((pos.x - mSolver.grid.minBound.x) * mSolver.grid.invCellSize), 0, mSolver.grid.gridWidth - 1);
		int cy = Math::Clamp((int)((pos.y - mSolver.grid.minBound.y) * mSolver.grid.invCellSize), 0, mSolver.grid.gridHeight - 1);
		for (int ny = Math::Max(cy - 1, 0); ny <= Math::Min(cy + 1, mSolver.grid.gridHeight - 1); ++ny)
		{
			int row = ny * mSolver.grid.gridWidth;
			for (int nx = Math::Max(cx - 1, 0); nx <= Math::Min(cx + 1, mSolver.grid.gridWidth - 1); ++nx)
			{
				int idx = mSolver.grid.cellHeads[nx + row];
				while (idx != -1)
				{
					if ((mask & mSolver.categoryIds[idx]) != 0)
					{
						bool hit = (type == ShapeType::Circle) ? (pos - mSolver.positions[idx]).length2() < (params.x + mSolver.radii[idx]) * (params.x + mSolver.radii[idx]) : IntersectionTests::Intersect(pos, 0, type, params, mSolver.positions[idx], mSolver.radii[idx]);
						if (hit) results.push_back(mReceivers[idx].entity);
					}
					idx = mSolver.grid.nextIndices[idx];
				}
			}
		}
		for (int idx : mSolver.largeEntityIndices)
		{
			if ((mask & mSolver.categoryIds[idx]) != 0 && IntersectionTests::Intersect(pos, 0, type, params, mSolver.positions[idx], mSolver.radii[idx]))
			{
				results.push_back(mReceivers[idx].entity);
			}
		}
		return (int)results.size();
	}

	void ParallelCollisionManager::queryEntitiesDeferred(Vector2 const& position, float rotation, ShapeType shapeType, Vector3 const& shapeParams, int targetMask, DeferredQueryCallback callback, void* userData)
	{
		mSolver.deferredQueries.push_back({ position, rotation, shapeType, shapeParams, targetMask, callback, userData });
	}
}
