#include "SBlocksCore.h"

#include "LogSystem.h"
#include "Core/TypeHash.h"
#include "StdUtility.h"

#include <unordered_map>

namespace SBlocks
{
	class PieceShapeRegister
	{
	public:

		struct Hasher
		{
			uint32 operator()(BlockCollection* bc) const
			{
				uint32 blockHash = 0xcab129de1;
				for (auto const& block : bc->blocks)
				{
					blockHash = HashCombine(blockHash, block.pos.x);
					blockHash = HashCombine(blockHash, block.pos.y);
					blockHash = HashCombine(blockHash, block.type);
				}
				return blockHash;
			}
		};

		struct EquFunc
		{
			bool operator()(BlockCollection* lhs, BlockCollection* rhs) const
			{
				return *lhs == *rhs;
			}
		};


		std::unordered_map< BlockCollection*, PieceShapeData*, Hasher , EquFunc > mMap;

		PieceShapeData* findData(BlockCollection& data)
		{
			auto iter = mMap.find(&data);
			if (iter != mMap.end())
				return iter->second;

			return nullptr;
		}

		PieceShapeData* getOrRegisterData(BlockCollection& data)
		{
			auto iter = mMap.find(&data);
			if (iter != mMap.end())
				return iter->second;

			return registerData(data);
		}

		PieceShapeData* registerData(BlockCollection& data)
		{
			PieceShapeData* newData = new PieceShapeData(data);
			++mLastUsedId;
			newData->id = mLastUsedId;
			mMap.emplace(newData , newData);
			return newData;
		}

		int mLastUsedId = 0;
	};

	PieceShapeRegister GShapeRegister;

	void BlockCollection::standardize(Int16Point2D& outBoundSize)
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

		outBoundSize = bound.max - bound.min + Vec2i(1, 1);
		if (bound.min != Int16Point2D::Zero())
		{
			for (auto& block : blocks)
			{
				block.pos -= bound.min;
			}
		}

		std::sort(blocks.begin(), blocks.end(), [](Int16Point2D const& lhs, Int16Point2D const& rhs)
		{
			if (lhs.y != rhs.y)
				return lhs.y < rhs.y;

			return lhs.x < rhs.x;
		});
	}

	bool BlockCollection::operator==(BlockCollection const& rhs) const
	{
		if (blocks.size() != rhs.blocks.size())
			return false;

		for (int i = 0; i < blocks.size(); ++i)
		{
			if (blocks[i] != rhs.blocks[i])
			{
				return false;
			}
		}
		return true;
	}

	void PieceShapeData::initialize(PieceShapeDesc const& desc)
	{
		blocks.clear();
		blocks.reserve(desc.data.size());

		for (int index = 0; index < desc.data.size(); ++index)
		{
			uint8 data = desc.data[index];
			if ( data & PieceShapeDesc::BLOCK_BIT )
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
		BlockCollection::standardize(boundSize);

#if SBLOCK_SHPAEDATA_USE_BLOCK_HASH
		blockHash = 0xcab129de1;
		blockHashNoType = 0xcab129de1;
		for (auto const& block : blocks)
		{
			blockHash = HashCombine(blockHash, block.pos.x);
			blockHash = HashCombine(blockHash, block.pos.y);
			blockHash = HashCombine(blockHash, block.type);

			blockHashNoType = HashCombine(blockHashNoType, block.pos.x);
			blockHashNoType = HashCombine(blockHashNoType, block.pos.y);
		}
#endif
	}

	void PieceShapeData::generateOuterConPosList(TArray< Int16Point2D >& outPosList) const
	{
		for (auto const& block : blocks)
		{
			for (int i = 0; i < DirType::RestValue; ++i)
			{
				Int16Point2D pos = block.pos + GConsOffsets[i];
				if (!haveBlock(pos))
				{
					outPosList.addUnique(pos);
				}
			}
		}
	}

	bool PieceShapeData::isSubset(PieceShapeData const& rhs) const
	{
		if (blocks.size() < rhs.blocks.size())
			return false;

		if (boundSize.x < rhs.boundSize.x || boundSize.y < rhs.boundSize.y)
		{
			return false;
		}

		if (blocks.size() == rhs.blocks.size())
		{
			if (boundSize != rhs.boundSize)
				return false;
			
			return *this == rhs;
		}

		auto CheckBlocks = [this, &rhs](Int16Point2D const& offset)
		{
			int indexStart = 0;
			for (int i = 0; i < rhs.blocks.size(); ++i)
			{
				auto const& testBlock = rhs.blocks[i];
				auto testPos = testBlock.pos + offset;
				int j = indexStart;
				for (; j < blocks.size(); ++j)
				{
					auto const& block = blocks[j];
					if (block.type == testBlock.type && block.pos == testPos)
					{
						indexStart = j + 1;
						break;
					}
				}

				if (j == blocks.size())
					return false;
			}

			return true;
		};

		Int16Point2D delta = boundSize - rhs.boundSize;
		Int16Point2D offset;
		for (offset.y = 0; offset.y <= delta.y; ++offset.y)
		{
			for (offset.x = 0; offset.x <= delta.x; ++offset.x)
			{
				if (CheckBlocks(offset))
					return true;
			}
		}
		return false;
	}

	bool PieceShapeData::isNormal() const
	{
		for (auto const& block : blocks)
		{
			if (block.type != 0)
				return false;
		}
		return true;
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
		if (boundSize != rhs.boundSize)
			return false;

		if ( !BlockCollection::operator == (rhs) )
		{
#if SBLOCK_SHPAEDATA_USE_BLOCK_HASH
			LogWarning(0, "PieceShapeData of BlockHash work fail");
#endif
			return false;
		}

		return true;
	}


	void PieceShapeData::generateOutline(TArray<Line>& outlines) const
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

		TArray< Line > HLines;
		TArray< Line > VLines;
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

		auto MergeLine = [](TArray< Line >& lines)
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

	PieceShape::OpState PieceShape::getOpState(int index) const
	{
		CHECK(mDataStorage.isValidIndex(index));
		for (int mirrorOp = 0; mirrorOp < EMirrorOp::COUNT; ++mirrorOp)
		{
			for (uint8 dir = 0; dir < DirType::RestValue; ++dir)
			{
				if (mDataIndexMap[mirrorOp][dir] == index)
				{
					return { dir , EMirrorOp::Type(mirrorOp) };
				}
			}
		}
		NEVER_REACH("getOpState");
		return { (uint8)INDEX_NONE , EMirrorOp::None };
	}

	int PieceShape::findSameShape(PieceShapeData const& data) const
	{
		for (int i = 0; i < (int)mDataStorage.size(); ++i)
		{
			if (mDataStorage[i] == data)
				return i;
		}
		return INDEX_NONE;
	}

	int PieceShape::findSameShapeIgnoreBlockType(PieceShapeData const& data) const
	{
		for (int i = 0; i < (int)mDataStorage.size(); ++i)
		{
			if (mDataStorage[i].compareBlockPos(data))
				return i;
		}
		return INDEX_NONE;
	}

	int PieceShape::findSubset(PieceShapeData const& data) const
	{
		for (int i = 0; i < (int)mDataStorage.size(); ++i)
		{
			if (data.isSubset(mDataStorage[i]))
				return i;
		}
		return INDEX_NONE;
	}

	int PieceShape::getDifferentShapeNum() const
	{
		return (int)mDataStorage.size();
	}

	void PieceShape::exportDesc(PieceShapeDesc& outDesc)
	{
		outDesc.bUseCustomPivot = (2.0 * pivot != Vector2(boundSize));
		outDesc.customPivot = pivot;
		outDesc.sizeX = boundSize.x;

		outDesc.data.resize(boundSize.x * boundSize.y, 0);
		for (auto block : mDataStorage[0].blocks)
		{
			int index = boundSize.x * block.pos.y + block.pos.x;
			outDesc.data[index] = ( block.type << 1 ) | PieceShapeDesc::BLOCK_BIT;
		}
	}

	void PieceShape::importDesc(PieceShapeDesc const& desc, bool bAllowMirrorOp)
	{
		pivot = desc.getPivot();
		outlines.resize(bAllowMirrorOp ? 1 : EMirrorOp::COUNT);

		mDataStorage.clear();
		{
			PieceShapeData shapeData;
			shapeData.initialize(desc);
			boundSize = shapeData.boundSize;
			shapeData.generateOutline(outlines[EMirrorOp::None]);
			addNewData(shapeData, 0);
		}
		for (int dir = 1; dir < DirType::RestValue; ++dir)
		{
			auto const& shapeDataPrev = getData(DirType::ValueChecked(dir - 1));

			PieceShapeData shapeData;
			shapeData.blocks.reserve(shapeDataPrev.blocks.size());
			for (auto const& block : shapeDataPrev.blocks)
			{
				PieceShapeData::Block blockAdd;
				blockAdd.pos.x = -block.pos.y;
				blockAdd.pos.y = block.pos.x;
				blockAdd.type = block.type;
				shapeData.blocks.push_back(blockAdd);
			}
			registerData(shapeData, dir);
		}
		if (bAllowMirrorOp)
		{
			registerMirrorData();
		}
	}

	void PieceShape::registerMirrorData()
	{
		outlines.resize(EMirrorOp::COUNT);
		
		for (int op = 1; op < EMirrorOp::COUNT; ++op)
		{
			auto const& shapeDataOriginal = getData(DirType::ValueChecked(0));
			PieceShapeData shapeData;
			shapeData.blocks.reserve(shapeDataOriginal.blocks.size());
			switch (op)
			{
			case EMirrorOp::X:
				for (auto const& block : shapeDataOriginal.blocks)
				{
					PieceShapeData::Block blockAdd;
					blockAdd.pos.x = -block.pos.x;
					blockAdd.pos.y = block.pos.y;
					blockAdd.type = block.type;
					shapeData.blocks.push_back(blockAdd);
				}
				break;
			case EMirrorOp::Y:
				for (auto const& block : shapeDataOriginal.blocks)
				{
					PieceShapeData::Block blockAdd;
					blockAdd.pos.x = block.pos.x;
					blockAdd.pos.y = -block.pos.y;
					blockAdd.type = block.type;
					shapeData.blocks.push_back(blockAdd);
				}
				break;
			}
			int index = registerData(shapeData, 0, EMirrorOp::Type(op));
			getDataByIndex(index).generateOutline(outlines[op]);

			for (int dir = 1; dir < DirType::RestValue; ++dir)
			{
				auto const& shapeDataPrev = getData(DirType::ValueChecked(dir - 1), EMirrorOp::Type(op));

				PieceShapeData shapeData;
				shapeData.blocks.reserve(shapeDataPrev.blocks.size());
				for (auto const& block : shapeDataPrev.blocks)
				{
					PieceShapeData::Block blockAdd;
					blockAdd.pos.x = -block.pos.y;
					blockAdd.pos.y = block.pos.x;
					blockAdd.type = block.type;
					shapeData.blocks.push_back(blockAdd);
				}
				registerData(shapeData, dir, EMirrorOp::Type(op));
			}
		}

	}

	int PieceShape::addNewData(PieceShapeData& data, int dir, EMirrorOp::Type op)
	{
		int index = mDataStorage.size();
		mDataIndexMap[op][dir] = index;
		mDataStorage.push_back(std::move(data));
		return index;
	}

	int PieceShape::registerData(PieceShapeData& data, int dir, EMirrorOp::Type op)
	{
		data.standardizeBlocks();
		int index = findSameShape(data);
		if (index != INDEX_NONE)
		{
			mDataIndexMap[op][dir] = index;
			return index;
		}
		return addNewData(data, dir, op);
	}

	bool MarkMap::tryLock(Vec2i const& pos, PieceShapeData const& shapeData)
	{
		if (!checkBound(pos, shapeData))
			return false;

		return tryLockAssumeInBound(pos, shapeData);
	}

	void MarkMap::unlock(int posIndex, PieceShapeData const& shapeData)
	{
		for (auto const& block : shapeData.blocks)
		{
			int index = posIndex + mData.toIndex(block.pos.x, block.pos.y);
			uint8& data = mData[index];
			CHECK(IsLocked(data));
			RemoveLock(data);
		}
		numBlockLocked -= shapeData.blocks.size();
	}

	void MarkMap::unlock(Vec2i const& pos, PieceShapeData const& shapeData)
	{
#if 1
		int posIndex = mData.toIndex(pos.x, pos.y);
		unlock(posIndex, shapeData);
#else
		for (auto const& block : shapeData.blocks)
		{
			Vec2i mapPos = pos + block.pos;
			uint8& data = mData.getData(mapPos.x, mapPos.y);
			CHECK(IsLocked(data));
			RemoveLock(data);
		}
		numBlockLocked -= shapeData.blocks.size();
#endif
	}
	bool MarkMap::tryLockAssumeInBound(int posIndex, PieceShapeData const& shapeData)
	{
		if (!canLockAssumeInBound(posIndex, shapeData))
			return false;

		lockChecked(posIndex, shapeData);
		return true;
	}
	bool MarkMap::tryLockAssumeInBound(Vec2i const& pos, PieceShapeData const& shapeData)
	{
#if 1
		int posIndex = mData.toIndex(pos.x, pos.y);
		return tryLockAssumeInBound(posIndex, shapeData);
#else
		int index = 0;
		for (; index < shapeData.blocks.size(); ++index)
		{
			auto const& block = shapeData.blocks[index];

			Vec2i mapPos = pos + block.pos;
			CHECK(mData.checkRange(mapPos.x, mapPos.y));
			uint8& data = mData.getData(mapPos.x, mapPos.y);
			if (!CanLock(data) || GetType(data) != block.type)
			{
				for (int i = 0; i < index; ++i)
				{
					auto const& block = shapeData.blocks[i];
					Vec2i mapPos = pos + block.pos;
					uint8& data = mData.getData(mapPos.x, mapPos.y);
					RemoveLock(data);
				}
				return false;
			}
			AddLock(data);
		}

		numBlockLocked += shapeData.blocks.size();
		return true;
#endif
	}

	bool MarkMap::canLockAssumeInBound(int posIndex, PieceShapeData const& shapeData)
	{
		for (auto const& block : shapeData.blocks)
		{
			int index = posIndex + mData.toIndex(block.pos.x, block.pos.y);
			uint8 data = mData[index];
			if (!CanLock(data))
				return false;
			if (GetType(data) != block.type)
				return false;
		}
		return true;
	}

	bool MarkMap::canLockAssumeInBound(Vec2i const& pos, PieceShapeData const& shapeData)
	{
#if 1
		int posIndex = mData.toIndex(pos.x, pos.y);
		return canLockAssumeInBound(posIndex, shapeData);
#else
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
#endif
	}

	void MarkMap::lockChecked(int posIndex, PieceShapeData const& shapeData)
	{
		for (auto const& block : shapeData.blocks)
		{
			int index = posIndex + mData.toIndex(block.pos.x, block.pos.y);
			uint8& data = mData[index];
			CHECK(CanLock(data));
			AddLock(data);
		}
		numBlockLocked += shapeData.blocks.size();
	}
	void MarkMap::lockChecked(Vec2i const& pos, PieceShapeData const& shapeData)
	{
#if 1
		int posIndex = mData.toIndex(pos.x, pos.y);
		lockChecked(posIndex, shapeData);
#else
		for (auto const& block : shapeData.blocks)
		{
			Vec2i mapPos = pos + block.pos;
			uint8& data = mData.getData(mapPos.x, mapPos.y);
			CHECK(CanLock(data));
			AddLock(data);
		}
		numBlockLocked += shapeData.blocks.size();
#endif
	}

	bool MarkMap::canLock(Vec2i const& pos, PieceShapeData const& shapeData)
	{
		if (!checkBound(pos, shapeData))
			return false;

		return canLockAssumeInBound(pos, shapeData);
	}

	bool MarkMap::checkBound(Vec2i const &pos, PieceShapeData const &shapeData) const
	{
		if (pos.x < 0 || pos.y < 0)
			return false;
		
		Vec2i maxPos = pos + shapeData.boundSize;
		if (maxPos.x > mData.getSizeX() || maxPos.y > mData.getSizeY())
			return false;

		return true;
	}

	bool MarkMap::isAllNormalType() const
	{
		for (int i = 0; i < mData.getRawDataSize(); ++i)
		{
			uint8 data = mData[i];
			if (GetType(data) != EBlock::Normal)
				return false;
		}
		return true;
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
				mData[index] = MAKE_MAP_DATA(value >> 1, 0);
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
		TGrid2D<uint8> oldData = std::move(mData);
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
		mData = rhs.mData;

		if (bInitState)
		{
			for (int i = 0; i < mData.getRawDataSize(); ++i)
			{
				RemoveLock(mData[i]);
			}
			numBlockLocked = 0;
		}
		else
		{
			numBlockLocked = rhs.numBlockLocked;
		}
	}

	bool MarkMap::isSymmetry() const
	{
		if (mData.getSizeX() != mData.getSizeY())
			return false;

		Vector2 center = 0.5f * Vector2(mData.getSize());
		if (mData.getSizeX() % 2)
			center.x -= 0.5f;
		if (mData.getSizeY() % 2)
			center.y -= 0.5f;

		for (int y = 0; y < mData.getSizeY(); ++y)
		{
			for (int x = 0; x < mData.getSizeX(); ++x)
			{
				auto data = mData(x, y) & ~LOCK_MASK;
				if ( !HaveBlock(data) && GetType(data) == 0)
					continue;

				auto DoTest = [&](float tx, float ty)
				{
					int x2 = int(tx + center.x);
					int y2 = int(ty + center.y);
					auto data2 = mData(x2, y2) & ~LOCK_MASK;
					return data = data2;
				};

				float ox = float(x) - center.x;
				float oy = float(y) - center.y;

				if (!DoTest(-ox, oy))
					return false;

				if (!DoTest(ox, -oy))
					return false;

				float tx = ox;
				float ty = oy;
				for (int dir = 0; dir < 3; ++dir)
				{
					float temp = tx;
					tx = -ty;
					ty = temp;
					if (!DoTest(tx, ty))
						return false;
				}
			}
		}

		return true;
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

	bool Level::isFinish() const
	{
		for (MarkMap const& map : mMaps)
		{
			if (!map.isFinish())
				return false;
		}
		return true;
	}

	void Level::importDesc(LevelDesc const& desc)
	{
		bAllowMirrorOp = desc.bAllowMirrorOp;

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
		outDesc.bAllowMirrorOp = bAllowMirrorOp;

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
			desc.shapeId = findShapeID(piece->shape);
			desc.pos = piece->pos;
			desc.dir = piece->dir;
			desc.mirror = piece->mirror;
			desc.bLockOperation = piece->bCanOperate == false;
			outDesc.pieces.push_back(std::move(desc));
		}
	}

	Piece* Level::createPiece(PieceShape& shape, DirType dir)
	{
		std::unique_ptr< Piece > piece = std::make_unique< Piece >();

		piece->shape = &shape;
		piece->dir = dir;
		piece->angle = 0;
		piece->bCanOperate = true;
		piece->mapIndexLocked = INDEX_NONE;
		piece->pos = Vector2::Zero();
		piece->updateTransform();
		piece->index = mPieces.size();

		Piece* result = piece.get();
		mPieces.push_back(std::move(piece));
		return result;
	}

	Piece* Level::createPiece(PieceDesc const& desc)
	{
		if (mShapes.isValidIndex(desc.shapeId) == false)
			return nullptr;
		
		std::unique_ptr< Piece > piece = std::make_unique< Piece >();

		piece->shape = mShapes[desc.shapeId].get();
		piece->dir.setValue(desc.dir);
		piece->bCanOperate = desc.bLockOperation == 0;
		piece->angle = desc.dir * Math::PI / 2;
		piece->mapIndexLocked = INDEX_NONE;
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

		shape->importDesc(desc,bAllowMirrorOp);
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
			int indexData = shape->findSameShape(shapeData);
			if (indexData != INDEX_NONE)
			{
				auto opState = shape->getOpState(indexData);
				if (opState.mirror == EMirrorOp::None)
				{
					outDir = opState.dir;
					return shape;
				}
			}
		}
		return nullptr;
	}

	void Level::removePieceShape(PieceShape* shape)
	{
		mShapes.removePred([shape](auto const& value) { return value.get() == shape; });
	}

	bool Level::tryLockPiece(Piece& piece)
	{
		CHECK(piece.isLocked() == false);
		Vector2 posWorld = piece.getLTCornerPos();

		for (int indexMap = 0; indexMap < mMaps.size(); ++indexMap)
		{
			MarkMap& map = mMaps[indexMap];

			Vector2 pos = posWorld - map.mPos;
			Vec2i mapPos;
			mapPos.x = Math::RoundToInt(pos.x);
			mapPos.y = Math::RoundToInt(pos.y);

			if (tryLockPiece(piece, indexMap, mapPos))
				return true;
		}

		return false;
	}

	bool Level::tryLockPiece(Piece& piece, int mapIndex, Vec2i const& mapPos)
	{
		CHECK(mMaps.isValidIndex(mapIndex));
		CHECK(piece.isLocked() == false);
		MarkMap& map = mMaps[mapIndex];

		if (!map.isInBound(mapPos))
			return false;
		if (!map.tryLock(mapPos, piece.getShapeData()))
			return false;

		piece.mapIndexLocked = mapIndex;
		piece.mapPosLocked = mapPos;
		piece.pos   = calcPiecePos(piece, mapIndex, mapPos, piece.dir);
		piece.angle = piece.dir * Math::PI / 2;
		piece.updateTransform();
		return true;
	}

	void Level::unlockPiece(Piece& piece)
	{
		CHECK(piece.isLocked() == true);
		mMaps[piece.mapIndexLocked].unlock(piece.mapPosLocked, piece.getShapeData());
		piece.mapIndexLocked = INDEX_NONE;
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

	Vector2 Level::calcPiecePos(Piece const& piece, int indexMap, Vec2i const& mapPos, DirType dir)
	{
		return mMaps[indexMap].mPos + Vector2(mapPos) - piece.shape->getLTCornerOffset(dir);
	}

}//namespace SBlock