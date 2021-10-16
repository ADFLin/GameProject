#include "SBlocksCore.h"

#include "LogSystem.h"

namespace SBlocks
{
	int PieceShape::findSameShape(Data const& data)
	{
		for (int i = 0; i < DirType::RestNumber; ++i)
		{
			if (mDataMap[i] == data)
				return i;
		}
		return INDEX_NONE;
	}

	void PieceShape::generateOutline()
	{
		Data& data = mDataMap[0];

		auto FindBlock = [&](Int16Point2D const& inBlock) -> bool
		{
			for (auto const& block : data.blocks)
			{
				if (block == inBlock)
					return true;
			}
			return false;
		};

		std::vector< Line > HLines;
		std::vector< Line > VLines;
		for (auto const& block : data.blocks)
		{
			if (FindBlock(block + Int16Point2D(0, -1)) == false)
			{
				Line line;
				line.start = block;
				line.end = block + Int16Point2D(1, 0);
				HLines.push_back(line);
			}
			if (FindBlock(block + Int16Point2D(0, 1)) == false)
			{
				Line line;
				line.start = block + Int16Point2D(0, 1);
				line.end = block + Int16Point2D(1, 1);
				HLines.push_back(line);
			}
			if (FindBlock(block + Int16Point2D(-1, 0)) == false)
			{
				Line line;
				line.start = block;
				line.end = block + Int16Point2D(0, 1);
				VLines.push_back(line);
			}
			if (FindBlock(block + Int16Point2D(1, 0)) == false)
			{
				Line line;
				line.start = block + Int16Point2D(1, 0);
				line.end = block + Int16Point2D(1, 1);
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

	void PieceShape::exportDesc(PieceShapeDesc& outDesc)
	{
		outDesc.pivot = pivot;
		outDesc.sizeX = boundSize.x;

		uint8 dataSizeX = ( boundSize.x + 7 ) / 8;
		outDesc.data.resize(dataSizeX * boundSize.y, 0);
		for (auto block : mDataMap[0].blocks)
		{
			int index = boundSize.x * block.y + block.x;
			uint32 x = block.x;
			uint32 y = block.y;
			FBitGird::Add(outDesc.data, dataSizeX, x, y);
		}
	}

	void PieceShape::importDesc(PieceShapeDesc const& desc)
	{
		pivot = desc.pivot;

		{
			auto& shapeData = mDataMap[0];
			shapeData.blocks.clear();
			AABB bound;
			bound.invalidate();

			int32 dataSizeX = FBitGird::GetDataSizeX(desc.sizeX);
			int32 sizeY = (desc.data.size() + dataSizeX - 1 ) / dataSizeX;
			for (int32 y = 0; y < sizeY; ++y)
			{
				for (int32 x = 0; x < desc.sizeX; ++x)
				{
					if (FBitGird::Read(desc.data, dataSizeX, x, y))
					{
						Int16Point2D block;
						block.x = x;
						block.y = y;
						bound.addPoint(block);
						shapeData.blocks.push_back(block);
					}
				}
			}

			if (bound.isEmpty())
			{
				LogWarning(0, "Piece Shape no block");
			}

			boundSize = bound.max - bound.min + Vec2i(1, 1);
			if (bound.min != Vec2i(0, 0))
			{
				for (auto& block : shapeData.blocks)
				{
					block -= bound.min;
				}
			}
			shapeData.boundSize = boundSize;
			shapeData.sortBlocks();
		}


		for (int dir = 1; dir < DirType::RestNumber; ++dir)
		{
			auto const& shapeDataPrev = mDataMap[dir - 1];

			auto& shapeData = mDataMap[dir];
			shapeData.blocks.clear();
			AABB bound;
			bound.invalidate();
			for (auto& block : shapeDataPrev.blocks)
			{
				Int16Point2D blockAdd;
				blockAdd.x = -block.y;
				blockAdd.y = block.x;

				bound.addPoint(blockAdd);
				shapeData.blocks.push_back(blockAdd);
			}

			//shapeData.boundSize = bound.max - bound.min;
			if (bound.min != Vec2i(0, 0))
			{
				for (auto& block : shapeData.blocks)
				{
					block -= bound.min;
				}
			}

			shapeData.sortBlocks();
			shapeData.boundSize = getBoundSize(DirType::ValueChecked(dir));
		}

		generateOutline();
	}

	bool MarkMap::tryLock(Vec2i const& pos, PieceShape::Data const& shapeData)
	{
		if (!canLock(pos, shapeData))
			return false;

		for (auto const& block : shapeData.blocks)
		{
			Vec2i mapPos = pos + block;
			uint8& data = mData.getData(mapPos.x, mapPos.y);
			CHECK(data == 0);
			data = PIECE_BLOCK;
			++numBlockLocked;
		}
		return true;
	}

	void MarkMap::unlock(Vec2i const& pos, PieceShape::Data const& shapeData)
	{
		for (auto const& block : shapeData.blocks)
		{
			Vec2i mapPos = pos + block;
			uint8& data = mData.getData(mapPos.x, mapPos.y);
			CHECK(data == PIECE_BLOCK);
			data = 0;
			--numBlockLocked;
		}
	}

	bool MarkMap::canLock(Vec2i const& pos, PieceShape::Data const& shapeData)
	{
		{
			//check boundary
			if (!mData.checkRange(pos.x, pos.y))
				return false;
			Vec2i maxPos = pos + shapeData.boundSize - Vec2i(1, 1);
			if (!mData.checkRange(maxPos.x, maxPos.y))
				return false;
		}

		for (auto const& block : shapeData.blocks)
		{
			Vec2i mapPos = pos + block;
			CHECK(mData.checkRange(mapPos.x, mapPos.y));
			if (mData.getData(mapPos.x, mapPos.y) != 0)
				return false;
		}
		return true;
	}

	void MarkMap::improtDesc(MapDesc const& desc)
	{
		uint32 sizeX = desc.sizeX;
		uint32 dataSizeX = FBitGird::GetDataSizeX(desc.sizeX);
		uint32 sizeY = (desc.data.size() + dataSizeX - 1) / dataSizeX;
		mData.resize(sizeX, sizeY);
		numTotalBlocks = 0;
		for (uint32 y = 0; y < sizeY; ++y)
		{
			for (uint32 x = 0; x < desc.sizeX; ++x)
			{
				if (FBitGird::Read(desc.data, dataSizeX, x, y))
				{
					mData(x,y) = MAP_BLOCK;
				}
				else
				{
					mData(x,y) = 0;
					++numTotalBlocks;
				}
			}
		}
	}

	void MarkMap::exportDesc(MapDesc& outDesc)
	{
		outDesc.sizeX = mData.getSizeX();
		uint8 dataSizeX = FBitGird::GetDataSizeX(outDesc.sizeX);
		outDesc.data.resize(dataSizeX * mData.getSizeY(), 0);
		for (int index = 0; index < mData.getRawDataSize(); ++index)
		{
			if (mData[index] == MAP_BLOCK)
			{
				int x = index % mData.getSizeX();
				int y = index / mData.getSizeY();
				FBitGird::Add(outDesc.data, dataSizeX, x, y);
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

	void MarkMap::toggleDataType(Vec2i const& pos)
	{
		uint8& data = mData(pos.x, pos.y);
		if (data == 0)
		{
			data = MAP_BLOCK;
			--numTotalBlocks;
		}
		else
		{
			CHECK(data != PIECE_BLOCK);
			data = 0;
			++numTotalBlocks;
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

		mMap.improtDesc(desc.map);

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
		mMap.exportDesc(outDesc.map);

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

	Piece* Level::createPiece(PieceShape& shape)
	{
		std::unique_ptr< Piece > piece = std::make_unique< Piece >();

		piece->shape = &shape;
		piece->dir = DirType::ValueChecked(0);
		piece->angle = 0;
		piece->bLocked = false;
		piece->pos = Vector2::Zero();
		piece->updateTransform();

		Piece* result = piece.get();
		mPieces.push_back(std::move(piece));
		return result;
	}

	Piece* Level::createPiece(PieceDesc const& desc)
	{
		std::unique_ptr< Piece > piece = std::make_unique< Piece >();

		piece->shape = mShapes[desc.id].get();
		piece->dir.setValue(desc.dir);
		piece->angle = desc.dir * Math::PI / 2;
		piece->bLocked = false;
		piece->pos = desc.pos;
		piece->updateTransform();

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

	Vec2i Level::GetLockMapPos(Piece& piece)
	{
		Vector2 pos = piece.getLTCornerPos();
		Vec2i mapPos;
		mapPos.x = Math::RoundToInt(pos.x);
		mapPos.y = Math::RoundToInt(pos.y);
		return mapPos;
	}

	bool Level::tryLockPiece(Piece& piece)
	{
		assert(piece.bLocked == false);

		if (piece.angle != piece.dir * Math::PI / 2)
		{
			piece.angle = piece.dir * Math::PI / 2;
			piece.updateTransform();
		}

		Vec2i mapPos = GetLockMapPos(piece);
		if (!mMap.isInBound(mapPos))
			return false;

		if (!mMap.tryLock(mapPos, piece.shape->mDataMap[piece.dir]))
			return false;

		piece.bLocked = true;
		Vector2 pos = piece.xform.transformPosition(Vector2::Zero());

		Vector2 offset;
		offset.x = pos.x - Math::RoundToInt(pos.x);
		offset.y = pos.y - Math::RoundToInt(pos.y);
#if 1	
		piece.pos -= offset;
		piece.updateTransform();
#endif
		return true;
	}

	void Level::unlockPiece(Piece& piece)
	{
		assert(piece.bLocked == true);

		Vec2i mapPos = GetLockMapPos(piece);
		mMap.unlock(mapPos, piece.shape->mDataMap[piece.dir]);
		piece.bLocked = false;
	}

	void Level::unlockAllPiece()
	{
		for (int index = 0; index < mPieces.size(); ++index)
		{
			Piece* piece = mPieces[index].get();
			if (piece->bLocked)
			{
				unlockPiece(*piece);
			}
		}
	}

}//namespace SBlock