#pragma once
#ifndef ChunkAlgo_H_05DCDD15_EFBB_4D5A_8B94_02ECE60FF9EE
#define ChunkAlgo_H_05DCDD15_EFBB_4D5A_8B94_02ECE60FF9EE

#include "LifeCore.h"
#include "DataStructure/Grid2D.h"

#define USE_CHUNK_COUNT 1

namespace Life
{
	template< typename T >
	T  AlignCount(T  value, T  align)
	{
		return (value + align - 1) / align;
	}

	class ChunkAlgo : public IAlgorithm, public IRenderProxy
	{
	public:
		ChunkAlgo(int32 x, int32 y)
		{
			mChunkMap.resize(AlignCount(x, ChunkLength), AlignCount(y, ChunkLength));
			mChunkMap.fillValue(nullptr);
			mChunkBound.invalidate();
		}

		virtual bool setCell(int x, int y, uint8 value) final;
		virtual uint8 getCell(int x, int y) final;
		virtual void clearCell() final;
		virtual BoundBox getBound() final;

		virtual BoundBox getLimitBound() final;

		static bool CheckWarp(int& inoutValue, int size, bool bCanWarp)
		{
			if (inoutValue < 0)
			{
				if (!bCanWarp)
					return false;

				inoutValue = (inoutValue + size) % size;
			}
			else if (inoutValue >= size)
			{
				if (!bCanWarp)
					return false;

				inoutValue = inoutValue % size;
			}
			return true;
		}

		bool checkBoundRule(Vec2i& inoutCPos)
		{
#if 0
			if (!mChunkMap.checkRange(inoutCPos))
				return false;
#else
			if (!CheckWarp(inoutCPos.x, mChunkMap.getSizeX(), mRule.bWarpX))
				return false;
			if (!CheckWarp(inoutCPos.y, mChunkMap.getSizeY(), mRule.bWarpY))
				return false;
#endif
			return true;
		}


		virtual void step() final;

		virtual void draw(IRenderer& renderer, Viewport const& viewport, BoundBox const& boundRender);

		IRenderProxy* getRenderProxy() { return this; }


		virtual void  getPattern(TArray<Vec2i>& outList) final
		{
			for (int indexChunk = 0; indexChunk < mChunks.size(); ++indexChunk)
			{
				auto chunk = mChunks[indexChunk];

#if USE_CHUNK_COUNT
				if (chunk->count == 0)
					continue;
#endif
				Vec2i chunkPos = ChunkLength * chunk->pos;

				uint8 const* pChunkData = chunk->data[mIndex];

				for (int j = 0; j < ChunkLength; ++j)
				{
					for (int i = 0; i < ChunkLength; ++i)
					{
						if (pChunkData[Chunk::ToDataIndex(i, j)])
						{
							outList.push_back(chunkPos + Vec2i(i, j));
						}
					}
				}
			}
		}


		virtual void  getPattern(BoundBox const& bound, TArray<Vec2i>& outList) final
		{
			BoundBox boundClip = bound.intersection(getBound());

			BoundBox chunkBound;
			chunkBound.min = ToChunkPos(boundClip.min);
			chunkBound.max = ToChunkPos(boundClip.max);

			for (int indexChunk = 0; indexChunk < mChunks.size(); ++indexChunk)
			{
				auto chunk = mChunks[indexChunk];
#if USE_CHUNK_COUNT
				if (chunk->count == 0)
					continue;
#endif
				if (!chunkBound.isInside(chunk->pos))
					continue;

				Vec2i chunkPos = ChunkLength * chunk->pos;
				uint8 const* pChunkData = chunk->data[mIndex];

				for (int j = 0; j < ChunkLength; ++j)
				{
					for (int i = 0; i < ChunkLength; ++i)
					{
						Vec2i pos = chunkPos + Vec2i(i, j);
						if (!bound.isInside(pos))
							continue;

						if (pChunkData[Chunk::ToDataIndex(i, j)])
						{
							outList.push_back(pos);
						}
					}
				}
			}
		}


		static constexpr int32 ChunkLength = 64; //1 << 8;
		static constexpr int32 ChunkDataStrideLength = ChunkLength + 2;
		static constexpr int32 ChunkDataSize = ChunkDataStrideLength * ChunkDataStrideLength;

		static constexpr int ToChunkValue(int value)
		{
			return Math::ToTileValue(value, ChunkLength);
		}
		static constexpr Vec2i ToChunkPos(Vec2i const& pos)
		{
			return Vec2i(ToChunkValue(pos.x), ToChunkValue(pos.y));
		}
		static constexpr Vec2i ToChunkPos(int x, int y)
		{
			return Vec2i(ToChunkValue(x), ToChunkValue(y));
		}

		static bool CheckChunkPos()
		{
			static_assert(ToChunkPos(0, ChunkLength - 1) == Vec2i(0, 0));
			static_assert(ToChunkPos(ChunkLength, 2 * ChunkLength - 1) == Vec2i(1, 1));
			static_assert(ToChunkPos(-1, -ChunkLength) == Vec2i(-1, -1));
			static_assert(ToChunkPos(-ChunkLength - 1, -2 * ChunkLength) == Vec2i(-2, -2));
			static_assert(ToChunkPos(-2 * ChunkLength - 1, -3 * ChunkLength) == Vec2i(-3, -3));
		}

		struct Chunk
		{
			uint8 data[2][ChunkDataSize];

#if USE_CHUNK_COUNT
			uint32 count;
			bool   bHadSleeped;
#endif
			Vec2i pos;

			static int ToDataIndex(int i, int j)
			{
				return (i + 1) + (j + 1) * ChunkDataStrideLength;
			}
		};

		Chunk* createChunk(Vec2i const& cPos)
		{

			Chunk* chunk = new Chunk;
			FMemory::Set(chunk, 0, sizeof(Chunk));
			//std::fill_n(chunk->data[0], ARRAY_SIZE(chunk->data[0]), 0);
			//std::fill_n(chunk->data[1], ARRAY_SIZE(chunk->data[1]), 0);
			chunk->pos = cPos;
#if USE_CHUNK_COUNT
			chunk->bHadSleeped = true;
#endif
			mChunks.push_back(chunk);
			mChunkBound.addPoint(cPos);
			return chunk;
		}

		Chunk* fetchChunk(Vec2i const& cPos)
		{
			auto& chunk = mChunkMap(cPos);
			if (chunk == nullptr)
			{
				chunk = createChunk(cPos);
			}
			return chunk;
		}

		Rule  mRule;
		int   mIndex = 0;
		BoundBox mChunkBound;
		TGrid2D< Chunk* > mChunkMap;
		TArray< Chunk* > mChunks;
	};

}//namespace Life

#endif // ChunkAlgo_H_05DCDD15_EFBB_4D5A_8B94_02ECE60FF9EE
