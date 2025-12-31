#pragma once
#ifndef SimpleCollision_H_
#define SimpleCollision_H_

#include "Math/Vector2.h"
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

    // Collision resolution settings
    struct CollisionSettings
    {
        float relaxation = 0.5f;        // Relaxation factor (Recommended 0.4 - 0.6 for immediate mode)
        float maxPushDistance = 4.0f;   // Maximum push distance per iteration
        float minSeparation = 0.01f;    // Minimum separation distance
        bool enableMassRatio = true;
        bool shufflePairs = true;       // Whether to shuffle pair order (Eliminates bias)
    };

    // Spatial Hash Collision System - Chained Array (Zero Allocation)
    // Implements static polymorphism via Template, avoiding virtual calls
    template <typename TCollidableBase>
    class SpatialHashSystem
    {
    public:
        SpatialHashSystem(float cellSize = 64.0f)
            : mCellSize(cellSize)
            , mInvCellSize(1.0f / cellSize)
        {
            // Initialize random number generator
            std::random_device rd;
            mRng.seed(rd());
            
            // Default to a large boundary (Dynamic or fixed size recommended for C# version)
            setBounds(Vector2(0, 0), Vector2(4096, 4096));
        }

        void setCellSize(float size)
        {
            mCellSize = size;
            mInvCellSize = 1.0f / size;
            // Recompute Grid dimensions when CellSize changes
            setBounds(minBound, maxBound);
        }

        void setBounds(Vector2 min, Vector2 max)
        {
            minBound = min;
            maxBound = max;
            
            width = (int)std::ceil((max.x - min.x) * mInvCellSize) + 1;
            height = (int)std::ceil((max.y - min.y) * mInvCellSize) + 1;
            
            // Reset Grid Head array
            mCells.resize(width * height);
            std::fill(mCells.begin(), mCells.end(), -1);
        }

        void setSettings(CollisionSettings const& settings) { mSettings = settings; }
        CollisionSettings& getSettings() { return mSettings; }

        void clear()
        {
            // Only clear counts, no memory deallocation needed
            mObjects.clear();
            mBigObjects.clear();
        }

        void insert(TCollidableBase* entity)
        {
            if (entity == nullptr) return;

            // Determine if it is a large object
            if (entity->getRadius() * 2.0f > mCellSize)
            {
                // Set index relative to big objects start? Or separate? 
                // Let's use a negative index or a flag for big objects if needed, 
                // but for simplicity, let's assume separate indices or user handles it.
                // For now, simpler: indices are for mObjects (small) only, or handled carefully.
                // To support both, we might need an offset or separate field.
                // Let's assume getSpatialIndex works for the specific list it's in.
                // Or: Big objects are rare, we mainly optimize small object removal.
                entity->setSpatialIndex(int(mBigObjects.size()));
                mBigObjects.push_back(entity);
            }
            else
            {
                entity->setSpatialIndex(int(mObjects.size()));
                mObjects.push_back(entity);
            }
        }

        // Optimized Remove (O(1) Search + O(1) Remove)
        // Requires TCollidableBase to implement: int getSpatialIndex() const; void setSpatialIndex(int);
        void remove(TCollidableBase* entity)
        {
            if (entity == nullptr) return;

            // 1. Try Small Objects
            // We can check if index is valid for mObjects logic
            // A safer check: verify if entity at index is indeed 'us'
            int idx = entity->getSpatialIndex();
            
            if (idx >= 0 && idx < mObjects.size() && mObjects[idx] == entity)
            {
                // Swap with last
                if (idx != mObjects.size() - 1)
                {
                    TCollidableBase* lastEntity = mObjects.back();
                    mObjects[idx] = lastEntity; // Move last to hole
                    lastEntity->setSpatialIndex(idx); // Update last entity's index
                }
                mObjects.pop_back();
                return;
            }

            // 2. Try Big Objects
            // If reusing same index field, we need to know WHICH list it is in. 
            // Fallback: If not found in small, check big using index? 
            // Risky if indices overlap. A robust system needs logic to distinguish.
            // Assumption: User won't mix small/big dynamic resizing often, or we check both safely.
            
            if (idx >= 0 && idx < mBigObjects.size() && mBigObjects[idx] == entity)
            {
                 if (idx != mBigObjects.size() - 1)
                {
                    TCollidableBase* lastEntity = mBigObjects.back();
                    mBigObjects[idx] = lastEntity;
                     lastEntity->setSpatialIndex(idx);
                }
                mBigObjects.pop_back();
            }
        }

        // Step 1: Build Chained Linked List (Small objects only)
        void buildGrid()
        {
            // 1. Reset Grid Head
            std::fill(mCells.begin(), mCells.end(), -1);
            
            // 2. Adjust Next array size (Match small object count)
            if (mNext.size() < mObjects.size())
            {
                mNext.resize(mObjects.size());
            }

            // 3. Populate data (Center insertion)
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
                else
                {
                    mNext[i] = -1; 
                }
            }
        }

        // Step 2: Resolve collisions
        void resolveCollisions(int iterations = 4)
        {
            size_t totalObjects = mObjects.size() + mBigObjects.size();
            // Estimate Pair count (BigObjects may cause more collisions, overestimate slightly)
            if (mPairs.capacity() < totalObjects * 4) 
                mPairs.reserve(totalObjects * 4);

            collectPairs();
            
            if (mPairs.empty()) return;

            for (int iter = 0; iter < iterations; ++iter)
            {
                if (mSettings.shufflePairs)
                {
                     std::shuffle(mPairs.begin(), mPairs.end(), mRng);
                }

                for (const auto& pair : mPairs)
                {
                    resolveOverlapImmediate(pair.a, pair.b);
                }
            }
        }

        struct ContactPair { TCollidableBase* a; TCollidableBase* b; };

        void collectPairs()
        {
            buildGrid();
            mPairs.clear();
            
            // 1. Small vs Small (Grid Neighbor Search)
            int cellCount = width * height;
            for (int i = 0; i < cellCount; ++i)
            {
                int a_idx = mCells[i];
                while (a_idx != -1)
                {
                    int b_idx = mNext[a_idx]; 
                    while (b_idx != -1)
                    {
                        mPairs.push_back({mObjects[a_idx], mObjects[b_idx]});
                        b_idx = mNext[b_idx];
                    }
                    
                    checkNeighbor(i, a_idx, 1, 0);  // Right
                    checkNeighbor(i, a_idx, -1, 1); // Bottom-left
                    checkNeighbor(i, a_idx, 0, 1);  // Bottom
                    checkNeighbor(i, a_idx, 1, 1);  // Bottom-right
                    
                    a_idx = mNext[a_idx];
                }
            }

            // 2. Big vs Big (Brute Force)
            // Assume few large objects (e.g. < 50), O(N^2) is acceptable
            for (size_t i = 0; i < mBigObjects.size(); ++i)
            {
                for (size_t j = i + 1; j < mBigObjects.size(); ++j)
                {
                    mPairs.push_back({mBigObjects[i], mBigObjects[j]});
                }
            }

            // 3. Big vs Small (Query Grid)
            // Let each large object query the Grid cells it covers
            if (!mObjects.empty())
            {
                for (TCollidableBase* big : mBigObjects)
                {
                    Vector2 pos = big->getPosition();
                    float r = big->getRadius();
                    
                    // Calculate AABB covered grid range
                    int minCX = (int)((pos.x - r - minBound.x) * mInvCellSize);
                    int maxCX = (int)((pos.x + r - minBound.x) * mInvCellSize);
                    int minCY = (int)((pos.y - r - minBound.y) * mInvCellSize);
                    int maxCY = (int)((pos.y + r - minBound.y) * mInvCellSize);

                    // Clamp to map bounds
                    minCX = std::max(0, minCX);
                    maxCX = std::min(width - 1, maxCX);
                    minCY = std::max(0, minCY);
                    maxCY = std::min(height - 1, maxCY);

                    for (int cy = minCY; cy <= maxCY; ++cy)
                    {
                        for (int cx = minCX; cx <= maxCX; ++cx)
                        {
                            int idx = mCells[cx + cy * width];
                            // Iterate through all small objects in the cell
                            while (idx != -1)
                            {
                                mPairs.push_back({big, mObjects[idx]});
                                idx = mNext[idx];
                            }
                        }
                    }
                }
            }
        }

        void checkNeighbor(int cellIndex, int a_idx, int dx, int dy)
        {
            int cx = cellIndex % width;
            int cy = cellIndex / width;
            
            int nx = cx + dx;
            int ny = cy + dy;
            
            if (nx >= 0 && nx < width && ny >= 0 && ny < height)
            {
                int neighborCell = nx + ny * width;
                int b_idx = mCells[neighborCell];
                
                while (b_idx != -1)
                {
                    mPairs.push_back({mObjects[a_idx], mObjects[b_idx]});
                    b_idx = mNext[b_idx];
                }
            }
        }

        // Range Query
        void queryCircle(Vector2 const& center, float radius,
                         std::vector<TCollidableBase*>& outResults) const
        {
            outResults.clear();
            
            // 1. Query Grid (Small objects)
            int minCX = std::max(0, std::min(width - 1, (int)((center.x - radius - minBound.x) * mInvCellSize)));
            int maxCX = std::max(0, std::min(width - 1, (int)((center.x + radius - minBound.x) * mInvCellSize)));
            int minCY = std::max(0, std::min(height - 1, (int)((center.y - radius - minBound.y) * mInvCellSize)));
            int maxCY = std::max(0, std::min(height - 1, (int)((center.y + radius - minBound.y) * mInvCellSize)));
            
            float rSq = radius * radius;
            
            for (int cy = minCY; cy <= maxCY; ++cy)
            {
                for (int cx = minCX; cx <= maxCX; ++cx)
                {
                    int idx = mCells[cx + cy * width];
                    while (idx != -1)
                    {
                        TCollidableBase* ent = mObjects[idx];
                        if ((ent->getPosition() - center).length2() <= rSq)
                        {
                            outResults.push_back(ent);
                        }
                        idx = mNext[idx];
                    }
                }
            }

            // 2. Query Big Objects (Brute Force)
            for (TCollidableBase* big : mBigObjects)
            {
                if ((big->getPosition() - center).length2() <= rSq) // Simple center distance check only; consider big->getRadius() for precise circle intersection
                {
                    outResults.push_back(big);
                }
            }
        }

        size_t getEntityCount() const { return mObjects.size() + mBigObjects.size(); }
        size_t getPairCount() const { return mPairs.size(); }

    private:
        float mCellSize;
        float mInvCellSize;
        CollisionSettings mSettings;
        std::default_random_engine mRng;
        
        Vector2 minBound, maxBound;
        int width, height;

        // SoA (Structure of Arrays)
        TArray<TCollidableBase*> mObjects;    // Small objects (Put in Grid)
		TArray<TCollidableBase*> mBigObjects; // Large objects (Handled separately)
		TArray<int> mCells;            // Grid Head index
		TArray<int> mNext;             // Next index (Linked List)
		TArray<ContactPair> mPairs;     // Collision pair cache

        // Immediate collision resolution (Sequential)
        void resolveOverlapImmediate(TCollidableBase* a, TCollidableBase* b)
        {
            Vector2 posA = a->getPosition();
            Vector2 posB = b->getPosition();
            
            Vector2 delta = posB - posA;
            float distSq = delta.length2();
            float minDist = a->getRadius() + b->getRadius();
            float minDistSq = minDist * minDist;

            if (distSq >= minDistSq)
                return;

            float dist = std::sqrt(distSq);
            
            Vector2 pushDir;
            if (dist < 0.001f)
            {
                // Deterministic random direction (Pointer Based)
                // Use pointer address as seed - simple and fast
                float angle = (float)((uintptr_t)a % 628) * 0.01f;
                pushDir = Vector2(std::cos(angle), std::sin(angle));
                dist = 0.001f;
            }
            else
            {
                pushDir = delta / dist;
            }
            
            float overlap = minDist - dist;
            if (overlap < mSettings.minSeparation) return;
            
            float pushAmount = overlap * mSettings.relaxation;
            pushAmount = std::min(pushAmount, mSettings.maxPushDistance);
            
            bool staticA = a->isStatic();
            bool staticB = b->isStatic();
            
            if (staticA && staticB) return;
            
            if (staticA)
            {
                b->setPosition(posB + pushDir * pushAmount);
            }
            else if (staticB)
            {
                a->setPosition(posA - pushDir * pushAmount);
            }
            else
            {
                float ratioA = 0.5f, ratioB = 0.5f;
                if (mSettings.enableMassRatio)
                {
                    float totalMass = a->getMass() + b->getMass();
                    if (totalMass > 0)
                    {
                        ratioA = b->getMass() / totalMass;
                        ratioB = a->getMass() / totalMass;
                    }
                }
                a->setPosition(posA - pushDir * (pushAmount * ratioA));
                b->setPosition(posB + pushDir * (pushAmount * ratioB));
            }
        }
    };


} // namespace SimpleCollision

#endif // SimpleCollision_H_
