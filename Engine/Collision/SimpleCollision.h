#pragma once
#ifndef SimpleCollision_H_
#define SimpleCollision_H_

#include "Math/Vector2.h"
#include "Async/AsyncWork.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <random>

namespace SimpleCollision
{
    using Vector2 = Math::Vector2;

    struct TCollidableBase
    {
        float getMass() const { return 1.0f; }
        bool isStatic() const { return false; }

        float getRadius() const { return 1.0f; }
        Vector2 getPosition() const { return Vector2(0, 0); }
        void setPosition(Vector2 const& pos) { }

		int getSpatialIndex();
		void setSpatialIndex(int index);
    };

    struct CollisionSettings
    {
        float relaxation = 1.0f;        
        float maxPushDistance = 4.0f;   
        float minSeparation = 0.01f;    
        bool enableMassRatio = true;
        bool shufflePairs = true;       
    };

    template <typename TCollidableBase>
    class SpatialHashSystem
    {
    public:
        SpatialHashSystem(float cellSize = 64.0f)
            : mCellSize(cellSize)
            , mInvCellSize(1.0f / cellSize)
        {
            std::random_device rd;
            mRng.seed(rd());
            setBounds(Vector2(0, 0), Vector2(4096, 4096));
        }

        void setCellSize(float size)
        {
            mCellSize = size;
            mInvCellSize = 1.0f / size;
            setBounds(minBound, maxBound);
        }

        void setBounds(Vector2 min, Vector2 max)
        {
            minBound = min;
            maxBound = max;
            width = (int)std::ceil((max.x - min.x) * mInvCellSize) + 1;
            height = (int)std::ceil((max.y - min.y) * mInvCellSize) + 1;
            mCells.resize(width * height);
            std::fill(mCells.begin(), mCells.end(), -1);
        }

        void setSettings(CollisionSettings const& settings) { mSettings = settings; }
        CollisionSettings& getSettings() { return mSettings; }

        void clear()
        {
            mObjects.clear();
            mBigObjects.clear();
        }

        void insert(TCollidableBase* entity)
        {
            if (entity == nullptr) return;
            if (entity->getRadius() * 2.0f > mCellSize)
            {
                entity->setSpatialIndex(int(mBigObjects.size()));
                mBigObjects.push_back(entity);
            }
            else
            {
                entity->setSpatialIndex(int(mObjects.size()));
                mObjects.push_back(entity);
            }
        }

        void remove(TCollidableBase* entity)
        {
            if (entity == nullptr) return;
            int idx = entity->getSpatialIndex();
            if (idx >= 0 && idx < (int)mObjects.size() && mObjects[idx] == entity)
            {
                if (idx != (int)mObjects.size() - 1)
                {
                    TCollidableBase* lastEntity = mObjects.back();
                    mObjects[idx] = lastEntity;
                    lastEntity->setSpatialIndex(idx);
                }
                mObjects.pop_back();
                return;
            }
            if (idx >= 0 && idx < (int)mBigObjects.size() && mBigObjects[idx] == entity)
            {
                 if (idx != (int)mBigObjects.size() - 1)
                {
                    TCollidableBase* lastEntity = mBigObjects.back();
                    mBigObjects[idx] = lastEntity;
                    lastEntity->setSpatialIndex(idx);
                }
                mBigObjects.pop_back();
            }
        }

        void buildGrid()
        {
            std::fill(mCells.begin(), mCells.end(), -1);
            if (mNext.size() < mObjects.size()) mNext.resize(mObjects.size());
            for (int i = 0; i < (int)mObjects.size(); ++i)
            {
                TCollidableBase* entity = mObjects[i];
                Vector2 pos = entity->getPosition();
                int cx = (int)((pos.x - minBound.x) * mInvCellSize);
                int cy = (int)((pos.y - minBound.y) * mInvCellSize);
                if (cx >= 0 && cx < width && cy >= 0 && cy < height)
                {
                    int cellParams = cx + cy * width;
                    mNext[i] = mCells[cellParams];
                    mCells[cellParams] = i;
                }
                else { mNext[i] = -1; }
            }
        }

        // Step 2: Resolve collisions with internal Sub-stepping logic
        void resolveCollisions(int totalIterations = 32, int subSteps = 4)
        {
            int itersPerStep = std::max(1, totalIterations / subSteps);
            size_t totalObjects = mObjects.size() + mBigObjects.size();
            if (mPairs.capacity() < totalObjects * 4) mPairs.reserve(totalObjects * 4);

            for (int s = 0; s < subSteps; ++s)
            {
                // Re-calculate broad-phase because objects were pushed in previous sub-step
                collectPairs();
                if (mPairs.empty()) break;

                for (int iter = 0; iter < itersPerStep; ++iter)
                {
                    if (mSettings.shufflePairs) std::shuffle(mPairs.begin(), mPairs.end(), mRng);
                    for (const auto& pair : mPairs)
                    {
                        resolveOverlapImmediate(pair.a, pair.b);
                    }
                }
            }
        }

        // Step 2 (Parallel): Optimized Parallel internal sub-stepping
        void parallelResolveCollisions(class ::QueueThreadPool& threadPool, int totalIterations = 32, int subSteps = 4)
        {
            if (mObjects.empty() && mBigObjects.empty()) return;

            int itersPerStep = std::max(1, totalIterations / subSteps);
            int numObjects = (int)mObjects.size();

            mAccumulatedPushes.resize(numObjects);
            mPosCache.resize(numObjects);
            mMassCache.resize(numObjects);
            mRadiusCache.resize(numObjects);
            mNeighborOffsets.resize(numObjects + 1);

            int numThreads = std::max(1, threadPool.getAllThreadNum());
            int rangePerThread = (numObjects + numThreads - 1) / numThreads;

            for (int s = 0; s < subSteps; ++s)
            {
                buildGrid();
                mNeighborCache.clear();
                if (mNeighborCache.capacity() < numObjects * 8) mNeighborCache.reserve(numObjects * 8);

                // 1. Refresh Neighbor Cache (Broad-phase sub-step)
                for (int i = 0; i < numObjects; ++i)
                {
                    mNeighborOffsets[i] = (int)mNeighborCache.size();
                    Vector2 pos = mObjects[i]->getPosition();
                    int cx = (int)((pos.x - minBound.x) * mInvCellSize);
                    int cy = (int)((pos.y - minBound.y) * mInvCellSize);
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            int nx = cx + dx; int ny = cy + dy;
                            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                                int b_idx = mCells[nx + ny * width];
                                while (b_idx != -1) { if (i != b_idx) mNeighborCache.push_back(b_idx); b_idx = mNext[b_idx]; }
                            }
                        }
                    }
                }
                mNeighborOffsets[numObjects] = (int)mNeighborCache.size();

                for (int i = 0; i < numObjects; ++i) {
                    mPosCache[i] = mObjects[i]->getPosition();
                    mMassCache[i] = mObjects[i]->getMass();
                    mRadiusCache[i] = mObjects[i]->getRadius();
                }

                for (int iter = 0; iter < itersPerStep; ++iter)
                {
                    for (int t = 0; t < numThreads; ++t) {
                        int startIdx = t * rangePerThread;
                        int endIdx = std::min(startIdx + rangePerThread, numObjects);
                        if (startIdx >= endIdx) break;
                        threadPool.addFunctionWork([this, startIdx, endIdx]() {
                            for (int k = startIdx; k < endIdx; ++k) {
                                Vector2 totalPush(0, 0);
                                Vector2 posA = mPosCache[k]; float rA = mRadiusCache[k]; float mA = mMassCache[k];
                                int start = mNeighborOffsets[k]; int end = mNeighborOffsets[k + 1];
                                for (int n = start; n < end; ++n) {
                                    int b = mNeighborCache[n];
                                    Vector2 delta = posA - mPosCache[b]; float distSq = delta.length2();
                                    float minDist = rA + mRadiusCache[b];
                                    if (distSq >= minDist * minDist || distSq < 1e-8f) continue;
                                    float dist = std::sqrt(distSq);
                                    float pushAmount = (minDist - dist) * mSettings.relaxation;
                                    float ratio = mSettings.enableMassRatio ? (mMassCache[b] / (mA + mMassCache[b])) : 0.5f;
                                    totalPush += (delta / dist) * (std::min(pushAmount, mSettings.maxPushDistance) * ratio);
                                }
                                for (auto* big : mBigObjects) {
                                    Vector2 delta = posA - big->getPosition(); float distSq = delta.length2();
                                    float minDist = rA + big->getRadius();
                                    if (distSq >= minDist * minDist || distSq < 1e-8f) continue;
                                    float dist = std::sqrt(distSq);
                                    float pushAmount = (minDist - dist) * mSettings.relaxation;
                                    float ratio = 1.0f;
                                    if (mSettings.enableMassRatio) { float mB = big->getMass(); ratio = mB / (mA + mB); }
                                    totalPush += (delta / dist) * std::min(pushAmount, mSettings.maxPushDistance) * ratio;
                                }
                                mAccumulatedPushes[k] = totalPush;
                            }
                        });
                    }
                    threadPool.waitAllWorkComplete();
                    for (int k = 0; k < numObjects; ++k) {
                        if (!mObjects[k]->isStatic() && mAccumulatedPushes[k].length2() > 1e-10f) {
                            mPosCache[k] += mAccumulatedPushes[k];
                            mObjects[k]->setPosition(mPosCache[k]);
                        }
                    }
                }
            }
        }

        struct ContactPair { TCollidableBase* a; TCollidableBase* b; };
        void collectPairs()
        {
            buildGrid();
            mPairs.clear();
            for (int i = 0; i < (int)mCells.size(); ++i) {
                int a_idx = mCells[i];
                while (a_idx != -1) {
                    int b_idx = mNext[a_idx]; 
                    while (b_idx != -1) { mPairs.push_back({mObjects[a_idx], mObjects[b_idx]}); b_idx = mNext[b_idx]; }
                    checkNeighbor(i, a_idx, 1, 0); checkNeighbor(i, a_idx, -1, 1);
                    checkNeighbor(i, a_idx, 0, 1); checkNeighbor(i, a_idx, 1, 1);
                    a_idx = mNext[a_idx];
                }
            }
            for (size_t i = 0; i < mBigObjects.size(); ++i) {
                for (size_t j = i + 1; j < mBigObjects.size(); ++j) mPairs.push_back({mBigObjects[i], mBigObjects[j]});
            }
            if (!mObjects.empty()) {
                for (TCollidableBase* big : mBigObjects) {
                    Vector2 pos = big->getPosition(); float r = big->getRadius();
                    int minCX = std::max(0, (int)((pos.x - r - minBound.x) * mInvCellSize));
                    int maxCX = std::min(width - 1, (int)((pos.x + r - minBound.x) * mInvCellSize));
                    int minCY = std::max(0, (int)((pos.y - r - minBound.y) * mInvCellSize));
                    int maxCY = std::min(height - 1, (int)((pos.y + r - minBound.y) * mInvCellSize));
                    for (int cy = minCY; cy <= maxCY; ++cy) {
                        for (int cx = minCX; cx <= maxCX; ++cx) {
                            int idx = mCells[cx + cy * width];
                            while (idx != -1) { mPairs.push_back({big, mObjects[idx]}); idx = mNext[idx]; }
                        }
                    }
                }
            }
        }

        void checkNeighbor(int cellIndex, int a_idx, int dx, int dy)
        {
            int cx = cellIndex % width; int cy = cellIndex / width;
            int nx = cx + dx; int ny = cy + dy;
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                int neighborCell = nx + ny * width; int b_idx = mCells[neighborCell];
                while (b_idx != -1) { mPairs.push_back({mObjects[a_idx], mObjects[b_idx]}); b_idx = mNext[b_idx]; }
            }
        }

        void queryCircle(Vector2 const& center, float radius, std::vector<TCollidableBase*>& outResults) const
        {
            outResults.clear();
            int minCX = std::max(0, std::min(width - 1, (int)((center.x - radius - minBound.x) * mInvCellSize)));
            int maxCX = std::max(0, std::min(width - 1, (int)((center.x + radius - minBound.x) * mInvCellSize)));
            int minCY = std::max(0, std::min(height - 1, (int)((center.y - radius - minBound.y) * mInvCellSize)));
            int maxCY = std::max(0, std::min(height - 1, (int)((center.y + radius - minBound.y) * mInvCellSize)));
            float rSq = radius * radius;
            for (int cy = minCY; cy <= maxCY; ++cy) {
                for (int cx = minCX; cx <= maxCX; ++cx) {
                    int idx = mCells[cx + cy * width];
                    while (idx != -1) {
                        TCollidableBase* ent = mObjects[idx];
                        if ((ent->getPosition() - center).length2() <= rSq) outResults.push_back(ent);
                        idx = mNext[idx];
                    }
                }
            }
            for (TCollidableBase* big : mBigObjects) { if ((big->getPosition() - center).length2() <= rSq) outResults.push_back(big); }
        }

        size_t getEntityCount() const { return mObjects.size() + mBigObjects.size(); }
        size_t getPairCount() const { return mPairs.size(); }

    private:
        float mCellSize; float mInvCellSize;
        CollisionSettings mSettings; std::default_random_engine mRng;
        Vector2 minBound, maxBound; int width, height;

        TArray<TCollidableBase*> mObjects;    
		TArray<TCollidableBase*> mBigObjects; 
		TArray<int> mCells;            
		TArray<int> mNext;             
		TArray<ContactPair> mPairs;     

        TArray<Vector2> mAccumulatedPushes; TArray<Vector2> mPosCache;
        TArray<float> mMassCache; TArray<float> mRadiusCache;
        TArray<int> mNeighborCache; TArray<int> mNeighborOffsets;

        void resolveOverlapImmediate(TCollidableBase* a, TCollidableBase* b)
        {
            Vector2 posA = a->getPosition(); Vector2 posB = b->getPosition();
            Vector2 delta = posB - posA; float distSq = delta.length2();
            float minDist = a->getRadius() + b->getRadius();
            if (distSq >= minDist * minDist) return;
            float dist = std::sqrt(distSq);
            Vector2 pushDir = (dist < 0.001f) ? ([](void* ptr) {
                float angle = (float)((uintptr_t)ptr % 628) * 0.01f;
                return Vector2(std::cos(angle), std::sin(angle));
            }(a)) : (delta / dist);
            float overlap = minDist - dist;
            if (overlap < mSettings.minSeparation) return;
            float pushAmount = std::min(overlap * mSettings.relaxation, mSettings.maxPushDistance);
            bool staticA = a->isStatic(); bool staticB = b->isStatic();
            if (staticA && staticB) return;
            if (staticA) b->setPosition(posB + pushDir * pushAmount);
            else if (staticB) a->setPosition(posA - pushDir * pushAmount);
            else {
                float ratioA = 0.5f, ratioB = 0.5f;
                if (mSettings.enableMassRatio) {
                    float totalMass = a->getMass() + b->getMass();
                    if (totalMass > 0) { ratioA = b->getMass() / totalMass; ratioB = a->getMass() / totalMass; }
                }
                a->setPosition(posA - pushDir * (pushAmount * ratioA));
                b->setPosition(posB + pushDir * (pushAmount * ratioB));
            }
        }
    };
} // namespace SimpleCollision
#endif // SimpleCollision_H_
