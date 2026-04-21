#include "ChunkAlgo.h"
#include "BitUtility.h"
#include "ProfileSystem.h"

namespace Life
{
	namespace
	{
		enum NeighborChunkIndex
		{
			NCI_Left,
			NCI_Right,
			NCI_Up,
			NCI_Down,
			NCI_UpLeft,
			NCI_UpRight,
			NCI_DownLeft,
			NCI_DownRight,
			NCI_Count,
		};


		ChunkAlgo::Chunk GEmpryChunk(ForceInit);

		ChunkAlgo::Chunk* ResolveChunk(TGrid2D<ChunkAlgo::Chunk*>& chunkMap, Rule const& rule, Vec2i cPos)
		{
			if (!ChunkAlgo::CheckWarp(cPos.x, chunkMap.getSizeX(), rule.bWarpX))
			{
				return nullptr;
			}
			if (!ChunkAlgo::CheckWarp(cPos.y, chunkMap.getSizeY(), rule.bWarpY))
			{
				return nullptr;
			}

			return chunkMap(cPos);
		}

		void AddBitboard(uint64 bits[], uint64 value)
		{
			uint64 carry0 = bits[0] & value;
			bits[0] ^= value;
			uint64 carry1 = bits[1] & carry0;
			bits[1] ^= carry0;
			uint64 carry2 = bits[2] & carry1;
			bits[2] ^= carry1;

			bits[3] ^= carry2;
		}

		uint64 BuildCountMask(uint64 bits[], uint32 count)
		{
			uint64 const maskBit0 = (count & 0x1) ? bits[0] : ~bits[0];
			uint64 const maskBit1 = (count & 0x2) ? bits[1] : ~bits[1];
			uint64 const maskBit2 = (count & 0x4) ? bits[2] : ~bits[2];
			uint64 const maskBit3 = (count & 0x8) ? bits[3] : ~bits[3];
			return maskBit0 & maskBit1 & maskBit2 & maskBit3;
		}

		uint64 GetRowBits(ChunkAlgo::Chunk const* chunk, int index, int row)
		{
			return chunk ? chunk->data[index][row] : 0;
		}

		uint64 GetCarryLeft(ChunkAlgo::Chunk const* chunk, int index, int row)
		{
			return chunk && (chunk->data[index][row] & ChunkAlgo::Chunk::ToBitMask(ChunkAlgo::ChunkLength - 1)) ? 1 : 0;
		}

		uint64 GetCarryRight(ChunkAlgo::Chunk const* chunk, int index, int row)
		{
			return chunk && (chunk->data[index][row] & ChunkAlgo::Chunk::ToBitMask(0)) ? ChunkAlgo::Chunk::ToBitMask(ChunkAlgo::ChunkLength - 1) : 0;
		}

		uint32 EvolveChunkRowsBitboard(ChunkAlgo::Chunk& chunk, ChunkAlgo::Chunk const* neighbors[NCI_Count], int indexPrev, int indexNext, Rule const& rule)
		{
			uint32 countAlive = 0;
			uint64* pChunkData = chunk.data[indexNext];
			uint64 const* pChunkDataPrev = chunk.data[indexPrev];

			ChunkAlgo::Chunk const* chunkL = neighbors[NCI_Left];
			ChunkAlgo::Chunk const* chunkR = neighbors[NCI_Right];
			ChunkAlgo::Chunk const* chunkU = neighbors[NCI_Up];
			ChunkAlgo::Chunk const* chunkD = neighbors[NCI_Down];
			ChunkAlgo::Chunk const* chunkUL = neighbors[NCI_UpLeft];
			ChunkAlgo::Chunk const* chunkUR = neighbors[NCI_UpRight];
			ChunkAlgo::Chunk const* chunkDL = neighbors[NCI_DownLeft];
			ChunkAlgo::Chunk const* chunkDR = neighbors[NCI_DownRight];

			for (int j = 0; j < ChunkAlgo::ChunkLength; ++j)
			{
				uint64 const up = (j > 0) ? pChunkDataPrev[j - 1] : GetRowBits(chunkU, indexPrev, ChunkAlgo::ChunkLength - 1);
				uint64 const mid = pChunkDataPrev[j];
				uint64 const down = (j + 1 < ChunkAlgo::ChunkLength) ? pChunkDataPrev[j + 1] : GetRowBits(chunkD, indexPrev, 0);

				int const upRow = (j > 0) ? (j - 1) : (ChunkAlgo::ChunkLength - 1);
				int const downRow = (j + 1 < ChunkAlgo::ChunkLength) ? (j + 1) : 0;

				uint64 const upLeft = (up << 1) | ((j > 0) ? GetCarryLeft(chunkL, indexPrev, upRow) : GetCarryLeft(chunkUL, indexPrev, upRow));
				uint64 const upRight = (up >> 1) | ((j > 0) ? GetCarryRight(chunkR, indexPrev, upRow) : GetCarryRight(chunkUR, indexPrev, upRow));
				uint64 const midLeft = (mid << 1) | GetCarryLeft(chunkL, indexPrev, j);
				uint64 const midRight = (mid >> 1) | GetCarryRight(chunkR, indexPrev, j);
				uint64 const downLeft = (down << 1) | ((j + 1 < ChunkAlgo::ChunkLength) ? GetCarryLeft(chunkL, indexPrev, downRow) : GetCarryLeft(chunkDL, indexPrev, downRow));
				uint64 const downRight = (down >> 1) | ((j + 1 < ChunkAlgo::ChunkLength) ? GetCarryRight(chunkR, indexPrev, downRow) : GetCarryRight(chunkDR, indexPrev, downRow));

				uint64 countBits[4] = { 0 };

				AddBitboard(countBits, upLeft);
				AddBitboard(countBits, up);
				AddBitboard(countBits, upRight);
				AddBitboard(countBits, midLeft);
				AddBitboard(countBits, midRight);
				AddBitboard(countBits, downLeft);
				AddBitboard(countBits, down);
				AddBitboard(countBits, downRight);

				uint64 next = 0;
				uint64 const aliveMask = mid;
				uint64 const deadMask = ~mid;
#if 1
				for (uint32 count = 0; count <= 8; ++count)
				{
					uint64 const countMask = BuildCountMask(countBits, count);
					if (rule.getEvoluteValue(count, 0))
					{
						next |= countMask & deadMask;
					}
					if (rule.getEvoluteValue(count, 1))
					{
						next |= countMask & aliveMask;
					}
				}
#else

				{
					uint64 const countMask = BuildCountMask(countBits, 2);
					next |= countMask & deadMask;
				}
				{
					uint64 const countMask = BuildCountMask(countBits, 3);
					next |= countMask & deadMask;
					next |= countMask & aliveMask;
				}
#endif

				pChunkData[j] = next;
				countAlive += FBitUtility::CountSet(next);
			}

			return countAlive;
		}

		bool HasAnyBit(uint64 value)
		{
			return !!value;
		}
	}

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

				if (!chunk->testCell(mIndex, ix, iy))
				{
					chunk->setCell(mIndex, ix, iy, true);
#if USE_CHUNK_COUNT
					++chunk->count;
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

					if (chunk->testCell(mIndex, ix, iy))
					{
						chunk->setCell(mIndex, ix, iy, false);
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
				return chunk->testCell(mIndex, ix, iy) ? 1 : 0;
			}
		}
		return 0;
	}

	void ChunkAlgo::clearCell()
	{
		while (!mActiveChunks.empty())
		{
			releaseChunk(mActiveChunks.back());
		}
		while (!mSleepingChunks.empty())
		{
			releaseChunk(mSleepingChunks.back());
		}
		mChunkBound.invalidate();
		mChunkBoundDirty = false;
	}

	BoundBox ChunkAlgo::getBound()
	{
		if (mChunkBoundDirty)
		{
			mChunkBoundDirty = false;
			rebuildChunkBound();
		}

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
		PROFILE_ENTRY("ChunkAlgo.Step");
		++mGeneration;
		releaseSleepingChunks(mGeneration);

		int indexPrev = mIndex;
		mIndex = 1 - mIndex;

		for (int indexChunk = 0; indexChunk < mActiveChunks.size();)
		{
			auto chunk = mActiveChunks[indexChunk];

			auto CheckNeedCreateEdgeChunk = [this, chunk, indexPrev](Vec2i cPosSrc, int edgeType) -> uint32
			{
				if (!checkBoundRule(cPosSrc))
					return 0;

				bool bNeedCreate = false;
				switch (edgeType)
				{
				case 0: bNeedCreate = HasAnyBit(chunk->data[indexPrev][0]); break;
				case 1: bNeedCreate = HasAnyBit(chunk->data[indexPrev][ChunkLength - 1]); break;
				case 2:
					for (int row = 0; row < ChunkLength; ++row)
					{
						if (chunk->data[indexPrev][row] & Chunk::ToBitMask(0))
						{
							bNeedCreate = true;
							break;
						}
					}
					break;
				case 3:
					for (int row = 0; row < ChunkLength; ++row)
					{
						if (chunk->data[indexPrev][row] & Chunk::ToBitMask(ChunkLength - 1))
						{
							bNeedCreate = true;
							break;
						}
					}
					break;
				}

				auto& chunkSrc = mChunkMap(cPosSrc);
				if (chunkSrc == nullptr)
				{
	#if USE_CHUNK_COUNT
					if (chunk->count == 0)
						return 0;
	#endif
					if (!bNeedCreate)
						return 0;

					chunkSrc = createChunk(cPosSrc);
					return 0;
				}
				if (bNeedCreate && chunkSrc->state == Chunk::State::Sleeping)
				{
					activateChunk(chunkSrc);
				}

				uint32 count = 0;
				switch (edgeType)
				{
				case 0: count = FBitUtility::CountSet(chunkSrc->data[indexPrev][ChunkLength - 1]); break;
				case 1: count = FBitUtility::CountSet(chunkSrc->data[indexPrev][0]); break;
				case 2:
					for (int row = 0; row < ChunkLength; ++row)
					{
						count += (chunkSrc->data[indexPrev][row] & Chunk::ToBitMask(ChunkLength - 1)) ? 1 : 0;
					}
					break;
				case 3:
					for (int row = 0; row < ChunkLength; ++row)
					{
						count += (chunkSrc->data[indexPrev][row] & Chunk::ToBitMask(0)) ? 1 : 0;
					}
					break;
				}
				return count;
			};

			uint32 neighborCount = 0;
			{
				//PROFILE_ENTRY("ChunkAlgo.Border");

				neighborCount += CheckNeedCreateEdgeChunk(chunk->pos + Vec2i(0, -1), 0);
				neighborCount += CheckNeedCreateEdgeChunk(chunk->pos + Vec2i(0, 1), 1);
				neighborCount += CheckNeedCreateEdgeChunk(chunk->pos + Vec2i(-1, 0), 2);
				neighborCount += CheckNeedCreateEdgeChunk(chunk->pos + Vec2i(1, 0), 3);

				auto CheckNeedCreateCornerChunk = [this, chunk, indexPrev](Vec2i cPosSrc, int cornerType) -> uint32
				{
					if (!checkBoundRule(cPosSrc))
						return 0;

					bool bNeedCreate = false;
					switch (cornerType)
					{
					case 0: bNeedCreate = (chunk->data[indexPrev][0] & Chunk::ToBitMask(0)) != 0; break;
					case 1: bNeedCreate = (chunk->data[indexPrev][0] & Chunk::ToBitMask(ChunkLength - 1)) != 0; break;
					case 2: bNeedCreate = (chunk->data[indexPrev][ChunkLength - 1] & Chunk::ToBitMask(0)) != 0; break;
					case 3: bNeedCreate = (chunk->data[indexPrev][ChunkLength - 1] & Chunk::ToBitMask(ChunkLength - 1)) != 0; break;
					}

					auto& chunkSrc = mChunkMap(cPosSrc);
					if (chunkSrc == nullptr)
					{
	#if USE_CHUNK_COUNT
						if (chunk->count == 0)
							return 0;
	#endif
						if (!bNeedCreate)
							return 0;

						chunkSrc = createChunk(cPosSrc);
						return 0;
					}
					if (bNeedCreate && chunkSrc->state == Chunk::State::Sleeping)
					{
						activateChunk(chunkSrc);
					}
					switch (cornerType)
					{
					case 0: return (chunkSrc->data[indexPrev][ChunkLength - 1] & Chunk::ToBitMask(ChunkLength - 1)) ? 1 : 0;
					case 1: return (chunkSrc->data[indexPrev][ChunkLength - 1] & Chunk::ToBitMask(0)) ? 1 : 0;
					case 2: return (chunkSrc->data[indexPrev][0] & Chunk::ToBitMask(ChunkLength - 1)) ? 1 : 0;
					default: return (chunkSrc->data[indexPrev][0] & Chunk::ToBitMask(0)) ? 1 : 0;
					}
				};
				neighborCount += CheckNeedCreateCornerChunk(chunk->pos + Vec2i(-1, -1), 0);
				neighborCount += CheckNeedCreateCornerChunk(chunk->pos + Vec2i(1, -1), 1);
				neighborCount += CheckNeedCreateCornerChunk(chunk->pos + Vec2i(-1, 1), 2);
				neighborCount += CheckNeedCreateCornerChunk(chunk->pos + Vec2i(1, 1), 3);
			}
	#if USE_CHUNK_COUNT
			if (chunk->count == 0 && neighborCount == 0)
			{
				FMemory::Set(chunk->data, 0, sizeof(chunk->data));
				sleepChunk(chunk, mGeneration);
				continue;
			}
			chunk->count = 0;
	#endif

			{
				//PROFILE_ENTRY("ChunkAlgo.Evolve");
				Chunk const* neighbors[NCI_Count] =
				{
					ResolveChunk(mChunkMap, mRule, chunk->pos + Vec2i(-1, 0)),
					ResolveChunk(mChunkMap, mRule, chunk->pos + Vec2i(1, 0)),
					ResolveChunk(mChunkMap, mRule, chunk->pos + Vec2i(0, -1)),
					ResolveChunk(mChunkMap, mRule, chunk->pos + Vec2i(0, 1)),
					ResolveChunk(mChunkMap, mRule, chunk->pos + Vec2i(-1, -1)),
					ResolveChunk(mChunkMap, mRule, chunk->pos + Vec2i(1, -1)),
					ResolveChunk(mChunkMap, mRule, chunk->pos + Vec2i(-1, 1)),
					ResolveChunk(mChunkMap, mRule, chunk->pos + Vec2i(1, 1)),
				};
#if USE_CHUNK_COUNT
				chunk->count = EvolveChunkRowsBitboard(*chunk, neighbors, indexPrev, mIndex, mRule);
#else
				EvolveChunkRowsBitboard(*chunk, neighbors, indexPrev, mIndex, mRule);
#endif
			}
			++indexChunk;
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
				renderer.drawBitCells(Vec2i(ox, oy), Vec2i(ChunkLength, ChunkLength), chunk->data[mIndex], 1);
			}
		}
	}

}//namespace Life
