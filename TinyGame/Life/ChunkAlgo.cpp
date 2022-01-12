#include "ChunkAlgo.h"

namespace Life
{
	bool ChunkAlgo::setCell(int x, int y, uint8 value)
	{
		Vec2i cPos = ToChunkPos(x, y);

		if (mChunkMap.checkRange(cPos))
		{
			if (value)
			{
				auto chunk = fetchChunk(cPos);
				int ix = x - cPos.x * ChunkLength;
				int iy = y - cPos.y * ChunkLength;

				uint8& data = chunk->data[mIndex][Chunk::ToDataIndex(ix, iy)];
				if (data == 0)
				{
					data = value;
#if USE_CHUNK_COUNT
					++chunk->count;
					chunk->bHadSleeped = false;
#endif
				}
			}
			else
			{
				auto chunk = mChunkMap(cPos);
				if (chunk)
				{
					int ix = x - cPos.x * ChunkLength;
					int iy = y - cPos.y * ChunkLength;

					uint8& data = chunk->data[mIndex][Chunk::ToDataIndex(ix, iy)];
					if (data)
					{
						data = 0;
#if USE_CHUNK_COUNT
						--chunk->count;
#endif
					}
				}
			}

			return true;
		}
		return false;
	}

	uint8 ChunkAlgo::getCell(int x, int y)
	{
		Vec2i cPos = ToChunkPos(x, y);

		if (mChunkMap.checkRange(cPos))
		{
			auto chunk = mChunkMap(cPos);
			if (chunk && chunk->count)
			{
				int ix = x - cPos.x * ChunkLength;
				int iy = y - cPos.y * ChunkLength;
				return chunk->data[mIndex][Chunk::ToDataIndex(ix, iy)];
			}
		}
		return 0;
	}

	void ChunkAlgo::clearCell()
	{
		for (auto chunk : mChunks)
		{
#if USE_CHUNK_COUNT
			if (chunk->bHadSleeped)
				continue;

			chunk->bHadSleeped = true;
			chunk->count = 0;
#endif
			uint8* pChunkData = chunk->data[0];
			memset(pChunkData, 0, 2 * ChunkDataSize);
		}
	}

	BoundBox ChunkAlgo::getBound()
	{
		BoundBox result;
		if (mChunkBound.isValid())
		{
			result.min = ChunkLength * mChunkBound.min;
			result.max = ChunkLength * mChunkBound.max;
		}
		else
		{
			result.invalidate();
		}
		return result;
	}

	BoundBox ChunkAlgo::getLimitBound()
	{
		BoundBox result;
		result.min = Vec2i::Zero();
		result.max = ChunkLength * mChunkMap.getSize() - Vec2i(1, 1);
		return result;
	}

	void ChunkAlgo::step()
	{
		int indexPrev = mIndex;
		mIndex = 1 - mIndex;

		for (int indexChunk = 0; indexChunk < mChunks.size(); ++indexChunk)
		{
			auto chunk = mChunks[indexChunk];

			auto CheckEdge = [this, chunk, indexPrev](Vec2i cPosSrc, uint32 srcOffset, uint32 destOffset, uint32 checkOffst, uint32 offset) -> uint32
			{
				if (!checkBoundRule(cPosSrc))
					return 0;

				auto& chunkSrc = mChunkMap(cPosSrc);
				if (chunkSrc == nullptr)
				{
	#if USE_CHUNK_COUNT
					if (chunk->count == 0)
						return 0;
	#endif
					uint8 const* check = chunk->data[indexPrev] + checkOffst;
					int index = 0;
					for (; index < ChunkLength; ++index)
					{
						if (*check)
							break;
						check += offset;
					}
					if (index == ChunkLength)
						return 0;

					chunkSrc = createChunk(cPosSrc);
				}

				uint8 const* src = chunkSrc->data[indexPrev] + srcOffset;
				uint8* dest = chunk->data[indexPrev] + destOffset;
				uint32 count = 0;
				for (int i = 0; i < ChunkLength; ++i)
				{
					*dest = *src;
					if (*dest)
					{
						++count;
					}

					dest += offset;
					src += offset;
				}
				return count;
			};

			uint32 neighborCount = 0;
			neighborCount += CheckEdge(chunk->pos + Vec2i(0, -1), Chunk::ToDataIndex(0, ChunkLength - 1),
				Chunk::ToDataIndex(0, -1), Chunk::ToDataIndex(0, 0), 1);
			neighborCount += CheckEdge(chunk->pos + Vec2i(0, 1), Chunk::ToDataIndex(0, 0),
				Chunk::ToDataIndex(0, ChunkLength), Chunk::ToDataIndex(0, ChunkLength - 1), 1);
			neighborCount += CheckEdge(chunk->pos + Vec2i(-1, 0), Chunk::ToDataIndex(ChunkLength - 1, 0),
				Chunk::ToDataIndex(-1, 0), Chunk::ToDataIndex(0, 0), ChunkLength + 2);
			neighborCount += CheckEdge(chunk->pos + Vec2i(1, 0), Chunk::ToDataIndex(0, 0),
				Chunk::ToDataIndex(ChunkLength, 0), Chunk::ToDataIndex(ChunkLength - 1, 0), ChunkLength + 2);

			auto CheckCorner = [this, chunk, indexPrev](Vec2i cPosSrc, uint32 srcOffset, uint32 destOffset, uint32 checkOffst) -> uint32
			{
				if (!checkBoundRule(cPosSrc))
					return 0;

				auto& chunkSrc = mChunkMap(cPosSrc);
				if (chunkSrc == nullptr)
				{
	#if USE_CHUNK_COUNT
					if (chunk->count == 0)
						return 0;
	#endif
					uint8* check = chunk->data[indexPrev] + checkOffst;
					if (*check == 0)
						return 0;

					chunkSrc = createChunk(cPosSrc);
				}

				uint8* dest = chunk->data[indexPrev] + destOffset;
				uint8 const* src = chunkSrc->data[indexPrev] + srcOffset;
				*dest = *src;

				return (*dest) ? 1 : 0;
			};
			neighborCount += CheckCorner(chunk->pos + Vec2i(-1, -1), Chunk::ToDataIndex(ChunkLength - 1, ChunkLength - 1),
				Chunk::ToDataIndex(-1, -1), Chunk::ToDataIndex(0, 0));
			neighborCount += CheckCorner(chunk->pos + Vec2i(1, -1), Chunk::ToDataIndex(0, ChunkLength - 1),
				Chunk::ToDataIndex(ChunkLength, -1), Chunk::ToDataIndex(ChunkLength - 1, 0));
			neighborCount += CheckCorner(chunk->pos + Vec2i(-1, 1), Chunk::ToDataIndex(ChunkLength - 1, 0),
				Chunk::ToDataIndex(-1, ChunkLength), Chunk::ToDataIndex(0, ChunkLength - 1));
			neighborCount += CheckCorner(chunk->pos + Vec2i(1, 1), Chunk::ToDataIndex(0, 0),
				Chunk::ToDataIndex(ChunkLength, ChunkLength), Chunk::ToDataIndex(ChunkLength - 1, ChunkLength - 1));
	#if USE_CHUNK_COUNT
			if (chunk->count == 0 && neighborCount == 0)
			{
				if (!chunk->bHadSleeped)
				{
					uint8* pChunkData = chunk->data[0];
					memset(pChunkData, 0, 2 * ChunkDataSize);
					chunk->bHadSleeped = true;
				}
				continue;
			}
			chunk->bHadSleeped = false;
			chunk->count = 0;
	#endif

			constexpr int NegihborIndexOffsets[] =
			{
				1, ChunkDataStrideLength + 1 ,ChunkDataStrideLength ,ChunkDataStrideLength - 1,
				-1,-ChunkDataStrideLength - 1,-ChunkDataStrideLength,1 - ChunkDataStrideLength,
			};

			uint8* pChunkData = chunk->data[mIndex];
			uint8 const* pChunkDataPrev = chunk->data[indexPrev];

			for (int j = 0; j < ChunkLength; ++j)
			{
				int dataIndexStart = Chunk::ToDataIndex(0, j);
				for (int i = 0; i < ChunkLength; ++i)
				{
					int dataIndex = dataIndexStart + i;
					uint32 count = 0;
					for (int offset : NegihborIndexOffsets)
					{
						if (pChunkDataPrev[dataIndex + offset])
						{
							++count;
						}
					}

					pChunkData[dataIndex] = mRule.getEvoluteValue(count, pChunkDataPrev[dataIndex]);
	#if USE_CHUNK_COUNT
					if (pChunkData[dataIndex])
					{
						++chunk->count;
					}
	#endif
				}
			}
		}
	}

	void ChunkAlgo::draw(IRenderer& renderer, Viewport const& viewport, BoundBox const& boundRender)
	{
		BoundBox chunkBound;
		chunkBound.min = ToChunkPos(boundRender.min);
		chunkBound.max = ToChunkPos(boundRender.max);
		for (int cy = chunkBound.min.y; cy <= chunkBound.max.y; ++cy)
		{
			for (int cx = chunkBound.min.x; cx <= chunkBound.max.x; ++cx)
			{
				if (!mChunkMap.checkRange(cx, cy))
					continue;

				Chunk* chunk = mChunkMap(cx, cy);
	#if USE_CHUNK_COUNT
				if (chunk == nullptr || chunk->count == 0)
					continue;
	#else
				if (chunk == nullptr)
					continue;
	#endif
				int ox = cx * ChunkLength;
				int oy = cy * ChunkLength;

				uint8 const* pChunkData = chunk->data[mIndex];

#if 0
				for (int j = 0; j < ChunkLength; ++j)
				{
					for (int i = 0; i < ChunkLength; ++i)
					{
						if (pChunkData[Chunk::ToDataIndex(i, j)])
						{
							renderer.drawCell(ox + i, oy + j);
						}
					}
				}
#else
				renderer.drawCells(Vec2i(ox, oy), Vec2i(ChunkLength, ChunkLength), pChunkData + Chunk::ToDataIndex(0,0), ChunkDataStrideLength);
#endif
			}
		}
	}

}//namespace Life
