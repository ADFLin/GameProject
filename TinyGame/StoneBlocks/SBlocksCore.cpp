#include "SBlocksCore.h"

#include "LogSystem.h"
#include "Core/TypeHash.h"
#include "StdUtility.h"

namespace SBlocks
{
	void PieceShapeData::initialize(PieceShapeDesc const& desc)
	{
		blocks.clear();

		for (int index = 0; index < desc.data.size(); ++index)
		{
			uint8 data = desc.data[index];
			if ( data & 0x1 )
			{
				Int16Point2D pos;
				pos.x = index % desc.sizeX;
				pos.y = index / desc.sizeX;
				blocks.push_back(PieceShapeData::Block(pos, data >> 1));
			}
		}

		standardizeBlocks();
	}

	void PieceShapeData::standardizeBlocks()
	{
		AABB bound;
		bound.invalidate();
		for (auto const& block : blocks)
		{
			bound.addPoint(block);
		}

		if (!bound.isValid())
		{
			LogWarning(0, "Piece Shape no block");
		}

		boundSize = bound.max - bound.min + Vec2i(1, 1);
		if (bound.min != Vec2i::Zero())
		{
			for (auto& block : blocks)
			{
				block.pos -= bound.min;
			}
		}

		std::sort(blocks.begin(), blocks.end(), [](Int16Point2D const& lhs, Int16Point2D const& rhs)
		{
			if (lhs.x < rhs.x)
				return true;
			if (lhs.x == rhs.x && lhs.y < rhs.y)
				return true;

			return false;
		});

#if SBLOCK_SHPAEDATA_USE_BLOCK_HASH
		blockHash = 0xcab129de1;
		blockHashNoType = 0xcab129de1;
		for (auto const& block : blocks)
		{
			HashCombine(blockHash, block.x);
			HashCombine(blockHash, block.y);
			HashCombine(blockHash, block.type);

			HashCombine(blockHashNoType, block.x);
			HashCombine(blockHashNoType, block.y);
		}
#endif
	}



	void PieceShapeData::generateOuterConPosList(std::vector< Int16Point2D >& outPosList)
	{
		for (auto const& block : blocks)
		{
			for (int i = 0; i < 4; ++i)
			{
				Int16Point2D pos = block.pos + GConsOffsets[i];

				auto iter = std::find_if(blocks.begin(), blocks.end(), [&pos](auto const& block) { return block.pos == pos; });
				if (iter == blocks.end())
				{
					outPosList.push_back(pos);
				}
			}
		}
	}

	bool PieceShapeData::compareBlockPos(PieceShapeData const& rhs) const
	{

#if SBLOCK_SHPAEDATA_USE_BLOCK_HASH
		if (blockHashNoType != rhs.blockHashNoType)
			return false;
#endif

		if (blocks.size() != rhs.blocks.size())
			return false;

		if (boundSize != rhs.boundSize)
			return false;

		for (int i = 0; i < blocks.size(); ++i)
		{
			if (blocks[i].pos != rhs.blocks[i].pos)
			{
#if SBLOCK_SHPAEDATA_USE_BLOCK_HASH
				LogWarning(0, "PieceShapeData of BlockHash work fail");
#endif
				return false;
			}
		}

		return true;
	}

	bool PieceShapeData::operator==(PieceShapeData const& rhs) const
	{
#if SBLOCK_SHPAEDATA_USE_BLOCK_HASH
		if (blockHash != rhs.blockHash)
			return false;
#endif

		if (blocks.size() != rhs.blocks.size())
			return false;

		if (boundSize != rhs.boundSize)
			return false;

		for (int i = 0; i < blocks.size(); ++i)
		{
			if (blocks[i] != rhs.blocks[i])
			{
#if SBLOCK_SHPAEDATA_USE_BLOCK_HASH
				LogWarning(0, "PieceShapeData of BlockHash work fail");
#endif
				return false;
			}
		}

		return true;
	}


	void PieceShapeData::generateOutline(std::vector<Line>& outlines)
	{
		auto FindBlock = [&](Int16Point2D const& inBlock) -> bool
		{
			for (auto const& block : blocks)
			{
				if (block.pos == inBlock)
					return true;
			}
			return false;
		};

		std::vector< Line > HLines;
		std::vector< Line > VLines;
		for (auto const& block : blocks)
		{
			if (FindBlock(block.pos + Int16Point2D(0, -1)) == false)
			{
				Line line;
				line.start = block.pos;
				line.end = block.pos + Int16Point2D(1, 0);
				HLines.push_back(line);
			}
			if (FindBlock(block.pos + Int16Point2D(0, 1)) == false)
			{
				Line line;
				line.start = block.pos + Int16Point2D(0, 1);
				line.end = block.pos + Int16Point2D(1, 1);
				HLines.push_back(line);
			}
			if (FindBlock(block.pos + Int16Point2D(-1, 0)) == false)
			{
				Line line;
				line.start = block.pos;
				line.end = block.pos + Int16Point2D(0, 1);
				VLines.push_back(line);
			}
			if (FindBlock(block.pos + Int16Point2D(1, 0)) == false)
			{
				Line line;
				line.start = block.pos + Int16Point2D(1, 0);
				line.end = block.pos + Int16Point2D(1, 1);
				VLines.push_back(line);
			}
		}

		auto MergeLine = [](std::vector< Line >& lines)
		{
			int numLines = lines.size();
			for (int i = 0; i < numLines; ++i)
			{
				for (int j = i + 1; j < numLines; ++j)
				{
					Line& lineA = lines[i];
					Line& lineB = lines[j];

					bool bMerged = false;
					if (lineA.end == lineB.start)
					{
						if (lineA.start.x == lineB.end.x ||
							lineA.start.y == lineB.end.y)
						{
							lineA.end = lineB.end;
							bMerged = true;
						}
					}
					else if (lineB.end == lineA.start)
					{
						if (lineB.start.x == lineA.end.x ||
							lineB.start.y == lineA.end.y)
						{
							lineA.start = lineB.start;
							bMerged = true;
						}
					}

					if (bMerged)
					{
						--numLines;
						if (numLines != j)
						{
							lines[j] = lines[numLines];
							--j;
						}
					}
				}
			}

			lines.resize(numLines);
		};
		MergeLine(VLines);
		MergeLine(HLines);

		outlines = std::move(HLines);
		outlines.insert(outlines.end(), VLines.begin(), VLines.end());
	}

	int PieceShape::findSameShape(PieceShapeData const& data)
	{
		for (int i = 0; i < DirType::RestNumber; ++i)
		{
			if (mDataMap[i] == data)
				return i;
		}
		return INDEX_NONE;
	}

	int PieceShape::findSameShapeIgnoreBlockType(PieceShapeData const& data)
	{
		for (int i = 0; i < DirType::RestNumber; ++i)
		{
			if (mDataMap[i].compareBlockPos(data))
				return i;
		}
		return INDEX_NONE;
	}

	int PieceShape::getDifferentShapeDirs(int outDirs[4])
	{
		int numDir = 1;
		outDirs[0] = 0;
		for (int dir = 1; dir < DirType::RestNumber; ++dir)
		{
			bool bOk = true;
			for (int indexDir = 0; indexDir < numDir; ++indexDir)
			{
				int otherDir = outDirs[indexDir];
				if (mDataMap[otherDir] == mDataMap[dir])
				{
					bOk = false;
					break;
				}
			}
			if (bOk)
			{
				outDirs[numDir] = dir;
				++numDir;
			}
		}

		return numDir;
	}

	void PieceShape::exportDesc(PieceShapeDesc& outDesc)
	{
		outDesc.bUseCustomPivot = (2.0 * pivot != Vector2(boundSize));
		outDesc.customPivot = pivot;
		outDesc.sizeX = boundSize.x;

		outDesc.data.resize(boundSize.x * boundSize.y, 0);
		for (auto block : mDataMap[0].blocks)
		{
			int index = boundSize.x * block.pos.y + block.pos.x;
			outDesc.data[index] = ( block.type << 1 ) | 0x1;
		}
	}

	void PieceShape::importDesc(PieceShapeDesc const& desc)
	{		
		{
			auto& shapeData = mDataMap[0];
			shapeData.initialize(desc);
			boundSize = shapeData.boundSize;
			shapeData.generateOutline(outlines);
		}

		for (int dir = 1; dir < DirType::RestNumber; ++dir)
		{
			auto const& shapeDataPrev = mDataMap[dir - 1];

			auto& shapeData = mDataMap[dir];
			shapeData.blocks.clear();
			for (auto const& block : shapeDataPrev.blocks)
			{
				PieceShapeData::Block blockAdd;
				blockAdd.pos.x = -block.pos.y;
				blockAdd.pos.y = block.pos.x;
				blockAdd.type = block.type;
				shapeData.blocks.push_back(blockAdd);
			}
			shapeData.standardizeBlocks();
		}

		pivot = desc.getPivot();
	}

	bool MarkMap::tryLock(Vec2i const& pos, PieceShapeData const& shapeData)
	{
		if (!canLock(pos, shapeData))
			return false;

		lockChecked(pos, shapeData);
		return true;
	}

	void MarkMap::unlock(Vec2i const& pos, PieceShapeData const& shapeData)
	{
		for (auto const& block : shapeData.blocks)
		{
			Vec2i mapPos = pos + block.pos;
			uint8& data = mData.getData(mapPos.x, mapPos.y);
			CHECK(IsLocked(data));
			RemoveLock(data);
		}

		numBlockLocked -= shapeData.blocks.size();
	}

	bool MarkMap::tryLockAssumeInBound(Vec2i const& pos, PieceShapeData const& shapeData)
	{
		if (!canLockAssumeInBound(pos, shapeData))
			return false;

		lockChecked(pos, shapeData);
		return true;
	}

	bool MarkMap::canLockAssumeInBound(Vec2i const& pos, PieceShapeData const& shapeData)
	{
		for (auto const& block : shapeData.blocks)
		{
			Vec2i mapPos = pos + block.pos;
			CHECK(mData.checkRange(mapPos.x, mapPos.y));
			uint8 data = mData.getData(mapPos.x, mapPos.y);
			if (!CanLock(data))
				return false;
			if (GetType(data) != block.type)
				return false;
		}
		return true;
	}

	void MarkMap::lockChecked(Vec2i const& pos, PieceShapeData const& shapeData)
	{
		for (auto const& block : shapeData.blocks)
		{
			Vec2i mapPos = pos + block.pos;
			uint8& data = mData.getData(mapPos.x, mapPos.y);
			CHECK(CanLock(data));
			AddLock(data);
		}
		numBlockLocked += shapeData.blocks.size();
	}

	bool MarkMap::canLock(Vec2i const& pos, PieceShapeData const& shapeData)
	{
		{
			//check boundary
			if (!mData.checkRange(pos.x, pos.y))
				return false;
			Vec2i maxPos = pos + shapeData.boundSize - Vec2i(1, 1);
			if (!mData.checkRange(maxPos.x, maxPos.y))
				return false;
		}
		return canLockAssumeInBound(pos, shapeData);
	}

	void MarkMap::importDesc(MapDesc const& desc)
	{
		uint32 sizeX = desc.sizeX;
		uint32 sizeY = (desc.data.size() + sizeX - 1) / sizeX;
		mPos = desc.pos;
		mData.resize(sizeX, sizeY);
		mData.fillValue(0);
		numTotalBlocks = 0;
		for (int index = 0; index < desc.data.size(); ++index)
		{
			uint32 value = desc.data[index];
			if (value == 0x1)
			{
				mData[index] = MAP_BLOCK;
			}
			else
			{
				mData[index] = value << 1;
				++numTotalBlocks;
			}
		}
	}

	void MarkMap::exportDesc(MapDesc& outDesc)
	{
		outDesc.pos = mPos;
		outDesc.sizeX = mData.getSizeX();
		outDesc.data.resize(mData.getRawDataSize(), 0);
		for (int index = 0; index < mData.getRawDataSize(); ++index)
		{
			uint8 data = mData[index];
			if (HaveBlock(data))
			{
				outDesc.data[index] = 0x1;
			}
			else
			{
				outDesc.data[index] = (GetType(data) << 1);
			}
		}
	}

	void MarkMap::resize(int sizeX, int sizeY)
	{
		TGrid2D< uint8 > oldData = std::move(mData);
		mData.resize(sizeX, sizeY);

		numTotalBlocks = 0;
		for (int j = 0; j < sizeY; ++j)
		{
			for (int i = 0; i < sizeX; ++i)
			{
				if (oldData.checkRange(i, j))
				{
					mData(i, j) = oldData(i, j);
				}
				else
				{
					mData(i, j) = MAP_BLOCK;
				}

				if (mData(i, j) == 0)
				{
					++numTotalBlocks;
				}
			}
		}
	}

	void MarkMap::toggleBlock(Vec2i const& pos)
	{
		uint8& data = mData(pos.x, pos.y);
		if (HaveBlock(data))
		{
			RemoveBlock(data);
		}
		else
		{
			CHECK(IsLocked(data) == false);
			AddBlock(data);
			--numTotalBlocks;
		}
	}

	void MarkMap::copyFrom(MarkMap const& rhs, bool bInitState)
	{
		numTotalBlocks = rhs.numTotalBlocks;
		if (bInitState)
		{
			mData.resize(rhs.mData.getSizeX(), rhs.mData.getSizeY());
			mData.fillValue(0);
			for (int i = 0; i < mData.getRawDataSize(); ++i)
			{
				if (rhs.mData[i] == MAP_BLOCK)
				{
					mData[i] = MAP_BLOCK;
				}
			}
			numBlockLocked = 0;
		}
		else
		{
			mData = rhs.mData;
			numBlockLocked = rhs.numBlockLocked;
		}
	}

	int Level::findShapeID(PieceShape* shape)
	{
		for (int i = 0; i < mShapes.size(); ++i)
		{
			if (shape == mShapes[i].get())
				return i;
		}
		return INDEX_NONE;
	}

	void Level::importDesc(LevelDesc const& desc)
	{
		mShapes.clear();
		mPieces.clear();

		mMaps.resize(desc.maps.size());
		int index = 0;
		for (auto const& mapDesc : desc.maps)
		{
			mMaps[index].importDesc(mapDesc);
			++index;
		}
	
		for (auto const& shapeDesc : desc.shapes)
		{
			createPieceShape(shapeDesc);
		}
		for (auto const& pieceDesc : desc.pieces)
		{
			createPiece(pieceDesc);
		}
	}

	void Level::exportDesc(LevelDesc& outDesc)
	{
		for (int i = 0; i < mMaps.size(); ++i)
		{
			MarkMap& map = mMaps[i];
			MapDesc desc;
			map.exportDesc(desc);
			outDesc.maps.push_back(std::move(desc));
		}

		for (int i = 0; i < mShapes.size(); ++i)
		{
			PieceShape* shape = mShapes[i].get();
			PieceShapeDesc desc;
			shape->exportDesc(desc);
			outDesc.shapes.push_back(std::move(desc));

		}

		for (int i = 0; i < mPieces.size(); ++i)
		{
			Piece* piece = mPieces[i].get();
			PieceDesc desc;
			desc.id = findShapeID(piece->shape);
			desc.pos = piece->pos;
			desc.dir = piece->dir;
			outDesc.pieces.push_back(std::move(desc));
		}
	}

	Piece* Level::createPiece(PieceShape& shape, DirType dir)
	{
		std::unique_ptr< Piece > piece = std::make_unique< Piece >();

		piece->shape = &shape;
		piece->dir = dir;
		piece->angle = 0;
		piece->indexMapLocked = INDEX_NONE;
		piece->pos = Vector2::Zero();
		piece->updateTransform();
		piece->index = mPieces.size();

		Piece* result = piece.get();
		mPieces.push_back(std::move(piece));
		return result;
	}

	Piece* Level::createPiece(PieceDesc const& desc)
	{
		if (IsValidIndex(mShapes, desc.id) == false)
			return nullptr;
		
		std::unique_ptr< Piece > piece = std::make_unique< Piece >();

		piece->shape = mShapes[desc.id].get();
		piece->dir.setValue(desc.dir);
		piece->angle = desc.dir * Math::PI / 2;
		piece->indexMapLocked = INDEX_NONE;
		piece->pos = desc.pos;
		piece->updateTransform();
		piece->index = mPieces.size();

		Piece* result = piece.get();
		mPieces.push_back(std::move(piece));
		return result;
	}

	PieceShape* Level::createPieceShape(PieceShapeDesc const& desc)
	{
		std::unique_ptr< PieceShape > shape = std::make_unique< PieceShape >();

		shape->importDesc(desc);
		PieceShape* result = shape.get();
		mShapes.push_back(std::move(shape));
		return result;
	}

	PieceShape* Level::findPieceShape(PieceShapeDesc const& desc, int& outDir)
	{
		PieceShapeData shapeData;
		shapeData.initialize(desc);
		for (int index = 0; index < mShapes.size(); ++index)
		{
			PieceShape* shape = mShapes[index].get();
			int dir = shape->findSameShape(shapeData);
			if (dir != INDEX_NONE)
			{
				outDir = dir;
				return shape;
			}
		}
		return nullptr;
	}

	void Level::removePieceShape(PieceShape* shape)
	{
		RemovePred(mShapes, [shape](auto const& value) { return value.get() == shape; });
	}

	bool Level::tryLockPiece(Piece& piece)
	{
		assert(piece.isLocked() == false);

		if (piece.angle != piece.dir * Math::PI / 2)
		{
			piece.angle = piece.dir * Math::PI / 2;
			piece.updateTransform();
		}

		Vector2 posWorld = piece.getLTCornerPos();

		for (int indexMap = 0; indexMap < mMaps.size(); ++indexMap)
		{
			MarkMap& map = mMaps[indexMap];

			Vector2 pos = posWorld - map.mPos;
			Vec2i mapPos;
			mapPos.x = Math::RoundToInt(pos.x);
			mapPos.y = Math::RoundToInt(pos.y);
			if (!map.isInBound(mapPos))
				continue;
			if (!map.tryLock(mapPos, piece.shape->mDataMap[piece.dir]))
				continue;

			piece.indexMapLocked = indexMap;
			piece.mapPosLocked = mapPos;
			piece.pos += Vector2(mapPos) - pos;
			piece.updateTransform();

			return true;
		}

		return false;
	}

	void Level::unlockPiece(Piece& piece)
	{
		assert(piece.isLocked() == true);
		mMaps[piece.indexMapLocked].unlock(piece.mapPosLocked, piece.shape->mDataMap[piece.dir]);
		piece.indexMapLocked = INDEX_NONE;
	}

	void Level::unlockAllPiece()
	{
		for (int index = 0; index < mPieces.size(); ++index)
		{
			Piece* piece = mPieces[index].get();
			if (piece->isLocked())
			{
				unlockPiece(*piece);
			}
		}
	}

	Vector2 Level::calcPiecePos(Piece& piece, int indexMap, Vec2i const& mapPos, DirType dir)
	{
		return mMaps[indexMap].mPos + Vector2(mapPos) - piece.shape->getLTCornerOffset(dir);
	}

}//namespace SBlock