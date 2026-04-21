#pragma once
#ifndef ChunkAlgo_H_05DCDD15_EFBB_4D5A_8B94_02ECE60FF9EE
#define ChunkAlgo_H_05DCDD15_EFBB_4D5A_8B94_02ECE60FF9EE

#include "LifeCore.h"
#include "BitUtility.h"
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
			for (int indexChunk = 0; indexChunk < mActiveChunks.size(); ++indexChunk)
			{
				auto chunk = mActiveChunks[indexChunk];

#if USE_CHUNK_COUNT
				if (chunk->count == 0)
					continue;
#endif
				Vec2i chunkPos = ChunkLength * chunk->pos;

				for (int j = 0; j < ChunkLength; ++j)
				{
					uint64 bits = chunk->data[mIndex][j];
					int i;
					int count;
					while (FBitUtility::IterateMaskRange<64>(bits, i, count))
					{
						for (int n = 0; n < count; ++n)
						{
							outList.push_back(chunkPos + Vec2i(i + n, j));
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

			for (int indexChunk = 0; indexChunk < mActiveChunks.size(); ++indexChunk)
			{
				auto chunk = mActiveChunks[indexChunk];
#if USE_CHUNK_COUNT
				if (chunk->count == 0)
					continue;
#endif
				if (!chunkBound.isInside(chunk->pos))
					continue;

				Vec2i chunkPos = ChunkLength * chunk->pos;

				for (int j = 0; j < ChunkLength; ++j)
				{
					uint64 bits = chunk->data[mIndex][j];
					int i;
					int count;
					while (FBitUtility::IterateMaskRange<64>(bits, i, count))
					{
						for (int n = 0; n < count; ++n)
						{
							Vec2i pos = chunkPos + Vec2i(i + n, j);
							if (!bound.isInside(pos))
								continue;
							outList.push_back(pos);
						}
					}
				}
			}
		}


		static constexpr int32 ChunkLength = 64; //1 << 8;

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
			Chunk() = default;
			Chunk(EForceInit)
			{
				FMemory::Zero(this, sizeof(*this));
			}

			enum class State : uint8
			{
				Active,
				Sleeping,
				Free,
			};

			uint64 data[2][ChunkLength];

#if USE_CHUNK_COUNT
			uint32 count;
			uint32 sleepGeneration;
#endif
			Vec2i pos;
			State state = State::Free;
			int   listIndex = INDEX_NONE;

			static uint64 ToBitMask(int i)
			{
				return uint64(1) << i;
			}

			bool testCell(int index, int i, int j) const
			{
				return (data[index][j] & ToBitMask(i)) != 0;
			}

			void setCell(int index, int i, int j, bool value)
			{
				uint64& row = data[index][j];
				uint64 const mask = ToBitMask(i);
				if (value)
				{
					row |= mask;
				}
				else
				{
					row &= ~mask;
				}
			}
		};

		static constexpr uint32 SleepReleaseDelay = 120;

		void markChunkBoundDirty()
		{
			mChunkBoundDirty = true;
		}

		static void RemoveChunkFromList(TArray<Chunk*>& list, Chunk* chunk)
		{
			int index = chunk->listIndex;
			CHECK(index != INDEX_NONE);
			CHECK(list[index] == chunk);
			
			list.removeIndexSwap(index);
			if (index < list.size())
			{
				CHECK(list[index] != nullptr);
				list[index]->listIndex = index;
			}
			chunk->listIndex = INDEX_NONE;
		}

		void addChunkToList(TArray<Chunk*>& list, Chunk* chunk)
		{
			chunk->listIndex = list.size();
			list.push_back(chunk);
		}

		void activateChunk(Chunk* chunk)
		{
			if (chunk->state == Chunk::State::Active)
				return;

			if (chunk->state == Chunk::State::Sleeping)
			{
				RemoveChunkFromList(mSleepingChunks, chunk);
			}
			else if (chunk->state == Chunk::State::Free)
			{
				return;
			}

			chunk->state = Chunk::State::Active;
			addChunkToList(mActiveChunks, chunk);
			mChunkBound.addPoint(chunk->pos);
		}

		void sleepChunk(Chunk* chunk, uint32 generation)
		{
			if (chunk->state != Chunk::State::Active)
				return;

			RemoveChunkFromList(mActiveChunks, chunk);
			chunk->state = Chunk::State::Sleeping;
#if USE_CHUNK_COUNT
			chunk->sleepGeneration = generation;
#endif
			addChunkToList(mSleepingChunks, chunk);
			markChunkBoundDirty();
		}

		void releaseChunk(Chunk* chunk)
		{
			if (chunk->state == Chunk::State::Active)
			{
				RemoveChunkFromList(mActiveChunks, chunk);
			}
			else if (chunk->state == Chunk::State::Sleeping)
			{
				RemoveChunkFromList(mSleepingChunks, chunk);
			}

			mChunkMap(chunk->pos) = nullptr;
			FMemory::Set(chunk->data, 0, sizeof(chunk->data));
#if USE_CHUNK_COUNT
			chunk->count = 0;
			chunk->sleepGeneration = 0;
#endif
			chunk->state = Chunk::State::Free;
			chunk->listIndex = INDEX_NONE;
			mFreeChunks.push_back(chunk);
			markChunkBoundDirty();
		}

		void releaseSleepingChunks(uint32 generation)
		{
#if USE_CHUNK_COUNT
			for (int index = 0; index < mSleepingChunks.size();)
			{
				Chunk* chunk = mSleepingChunks[index];
				if (generation - chunk->sleepGeneration >= SleepReleaseDelay)
				{
					releaseChunk(chunk);
					continue;
				}
				++index;
			}
#endif
		}

		void rebuildChunkBound()
		{
			mChunkBound.invalidate();
			for (Chunk* chunk : mActiveChunks)
			{
				mChunkBound.addPoint(chunk->pos);
			}
		}

		Chunk* createChunk(Vec2i const& cPos)
		{
			Chunk* chunk = nullptr;
			if (!mFreeChunks.empty())
			{
				chunk = mFreeChunks.back();
				mFreeChunks.pop_back();
			}
			else
			{
				chunk = new Chunk;
			}
			FMemory::Set(chunk, 0, sizeof(Chunk));
			chunk->pos = cPos;
			chunk->state = Chunk::State::Active;
#if USE_CHUNK_COUNT
			chunk->sleepGeneration = 0;
#endif
			addChunkToList(mActiveChunks, chunk);
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
			else
			{
				activateChunk(chunk);
			}
			return chunk;
		}

		Rule  mRule;
		int   mIndex = 0;
		uint32 mGeneration = 0;
		BoundBox mChunkBound;
		bool mChunkBoundDirty = false;
		TGrid2D< Chunk* > mChunkMap;
		TArray< Chunk* > mActiveChunks;
		TArray< Chunk* > mSleepingChunks;
		TArray< Chunk* > mFreeChunks;
	};

}//namespace Life

#endif // ChunkAlgo_H_05DCDD15_EFBB_4D5A_8B94_02ECE60FF9EE
