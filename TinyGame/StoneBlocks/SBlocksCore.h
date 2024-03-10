#pragma once
#ifndef SBlockCore_H_CEF300EC_5B0F_406A_942F_AC4CBE559D18
#define SBlockCore_H_CEF300EC_5B0F_406A_942F_AC4CBE559D18

#include "CycleValue.h"
#include "Renderer/RenderTransform2D.h"
#include "DataStructure/Grid2D.h"
#include "Math/Vector2.h"
#include "Serialize/SerializeFwd.h"

#include "ColorName.h"
#include "RenderUtility.h"
#include "Math/GeometryPrimitive.h"
#include "DataStructure/Array.h"

#include <memory>
#include <algorithm>

namespace SBlocks
{
	using DirType = TCycleValue< 4, uint8 >;
	using Int16Point2D = TVector2< int16 >;
	using RenderTransform2D = Render::RenderTransform2D;
	using Math::Matrix2;
	using Math::Vector2;


	const Int16Point2D GConsOffsets[8] = 
	{
		Int16Point2D(1,0) , Int16Point2D(0,1) , Int16Point2D(-1,0) , Int16Point2D(0,-1),
		Int16Point2D(1,1) , Int16Point2D(-1,1) , Int16Point2D(-1,-1) , Int16Point2D(1,-1)
	};


	FORCEINLINE Matrix2 GetRotation(DirType dir)
	{
		switch (dir)
		{
		case 0: return Matrix2::Identity();
		case 1: return Matrix2(0, 1, -1, 0);
		case 2: return Matrix2(-1, 0, 0, -1);
		case 3: return Matrix2(0, -1, 1, 0);
		}
		NEVER_REACH("GetRotation");
		return Matrix2::Identity();
	}


	class FBitGird
	{
	public:
		static uint32 ToIndex(uint32 dataSizeX, int x, int y)
		{
			return x / 8 + y * dataSizeX;
		}
		static uint32 GetDataSizeX(uint8 sizeX)
		{
			return (sizeX + 7) / 8;
		}
		static bool Read(TArray< uint8 > const& bitData, uint32 dataSizeX, int x, int y)
		{
			uint32 index = ToIndex(dataSizeX, x, y);
			return !!(bitData[index] & BIT(x % 8));
		}
		static void Add(TArray< uint8 >& bitData, uint32 dataSizeX, int x, int y)
		{
			uint32 index = ToIndex(dataSizeX, x, y);
			bitData[index] |= BIT(x % 8);
		}
		static void Remove(TArray< uint8 >& bitData, uint32 dataSizeX, int x, int y)
		{
			uint32 index = ToIndex(dataSizeX, x, y);
			bitData[index] &= ~BIT(x % 8);
		}

		static void Toggle(TArray< uint8 >& bitData, uint32 dataSizeX, int x, int y)
		{
			uint32 index = ToIndex(dataSizeX, x, y);
			bitData[index] ^= BIT(x % 8);
		}

		template< class T >
		static TArray< uint8 > ConvertForm(TArray< T >& data, uint32 sizeX , uint32 mask = 0xff)
		{
			TArray< uint8 > result;
			uint32 sizeY = (data.size() + sizeX - 1) / sizeX;
			uint32 dataSizeX = GetDataSizeX(sizeX);
			result.resize(dataSizeX * sizeY, 0);
			for (uint32 index = 0; index < data.size(); ++index)
			{
				if (data[index] & mask)
				{
					uint32 x = index % sizeX;
					uint32 y = index / sizeX;
					Add(result, dataSizeX, x, y);
				}
			}
			return result;
		}
		template< class T >
		static TArray< T > ConvertTo(TArray< uint8 >& bitData, uint32 sizeX, T value = T(1))
		{
			TArray< T > result;

			uint32 dataSizeX = GetDataSizeX(sizeX);
			uint32 sizeY = (bitData.size() + dataSizeX - 1) / dataSizeX;
			result.resize(sizeX * sizeY, 0);
			for (uint32 index = 0; index < result.size(); ++index)
			{
				uint32 x = index % sizeX;
				uint32 y = index / sizeX;
				if (Read(bitData, dataSizeX, x, y))
				{
					result[index] = value;
				}
			}
			return result;
		}
	};

	FORCEINLINE bool IsInBound(Vec2i const& pos, Vec2i const& boundSize)
	{
		return 0 <= pos.x && pos.x < boundSize.x &&
			   0 <= pos.y && pos.y < boundSize.y;
	}

	struct PieceShapeDesc
	{
		uint16 sizeX;
		TArray< uint8 > data;
		bool	bUseCustomPivot;
		Vector2 customPivot;

		static constexpr uint8 BLOCK_BIT = 0x1;

		void toggleValue(Vec2i const& pos)
		{
			int index = pos.x + sizeX * pos.y;
			data[index] ^= BLOCK_BIT;
		}

		Vec2i getBoundSize() const
		{
			return Vec2i(sizeX, (data.size() + sizeX - 1) / sizeX);
		}

		Vector2 getPivot() const
		{
			if (bUseCustomPivot)
			{
				return customPivot;
			}
			else
			{
				return 0.5f * Vector2(getBoundSize());
			}
		}
		template< class OP >
		void serialize(OP& op);
	};

	using AABB = ::Math::TAABBox< Int16Point2D >;

	struct BlockCollection
	{
		struct Block
		{
			Block() = default;
			Block(Int16Point2D const& inPos, uint8 inType) :pos(inPos), type(inType) {}

			bool operator == (Block const& rhs) const
			{
				return pos == rhs.pos && type == rhs.type;
			}

			bool operator != (Block const& rhs) const
			{
				return !this->operator==(rhs);
			}

			Int16Point2D pos;
			uint8 type;
			operator Int16Point2D() const { return pos; }
		};

		TArray< Block > blocks;
	};

#define SBLOCK_SHPAEDATA_USE_BLOCK_HASH 0
	struct PieceShapeData : BlockCollection
	{
		Int16Point2D boundSize;
#if SBLOCK_SHPAEDATA_USE_BLOCK_HASH
		uint32 blockHash;
		uint32 blockHashNoType;
#endif
		void initialize(PieceShapeDesc const& desc);

		void standardizeBlocks();

		bool isSubset(PieceShapeData const& rhs) const;

		bool compareBlockPos(PieceShapeData const& rhs) const;
		bool operator == (PieceShapeData const& rhs) const;

		struct Line
		{
			Int16Point2D start;
			Int16Point2D end;
		};

		void generateOuterConPosList(TArray< Int16Point2D >& outPosList) const;
		void generateOutline(TArray<Line>& outlines) const;
	
	private:
		int getStartIndex(int y) const
		{
			return blocks.findIndexPred([y](Block const& block)
			{
				return block.pos.y == y;
			});
		}
	};

	namespace EMirrorOp
	{
		enum Type
		{
			None ,
			X  ,
			Y  ,

			COUNT,
		};
	};

	struct PieceShape
	{
		Int16Point2D boundSize;

		Vector2 pivot;

		TArray< TArray<PieceShapeData::Line> > outlines;

		int indexSolve;

		uint8 getDataIndex(DirType const& dir, EMirrorOp::Type op = EMirrorOp::None) const
		{
			return mDataIndexMap[op][dir];
		}
		PieceShapeData const& getData(DirType const& dir, EMirrorOp::Type op = EMirrorOp::None) const
		{
			uint8 index = mDataIndexMap[op][dir];
			return mDataStorage[index];
		}
		PieceShapeData const& getDataByIndex(int index) const
		{
			return mDataStorage[index];
		}

		struct OpState
		{
			uint8 dir;
			EMirrorOp::Type mirror;
		};

		OpState getOpState(int index) const;

		int findSameShape(PieceShapeData const& data) const;
		int findSameShapeIgnoreBlockType(PieceShapeData const& data) const;
		int findSubset(PieceShapeData const& data) const;
		int getBlockCount() const
		{
			return mDataStorage[0].blocks.size();
		}

		int getDifferentShapeNum() const;

		void exportDesc(PieceShapeDesc& outDesc);

		void importDesc(PieceShapeDesc const& desc, bool bAllowMirrorOp);

		Vec2i getBoundSize(DirType dir) const
		{
			if (dir % 2)
				return Vec2i(boundSize.y, boundSize.x);
			return boundSize;
		}

		Vec2i getCornerPos(DirType dir) const
		{
			switch (dir)
			{
			case 0: return Vec2i::Zero();
			case 1: return Vec2i(0, boundSize.y);
			case 2: return boundSize;
			case 3: return Vec2i(boundSize.x, 0);
			}
			NEVER_REACH("dir value always in 0-3");
			return Vec2i::Zero();
		}

		Vector2 getLTCornerOffset(DirType inDir) const
		{
			Vector2 lPos = getCornerPos(inDir);
			return pivot + GetRotation(inDir).leftMul(lPos - pivot);
		}

		void registerMirrorData();
	private:
		int addNewData(PieceShapeData& data, int dir, EMirrorOp::Type op = EMirrorOp::None);
		int registerData(PieceShapeData& data, int dir, EMirrorOp::Type op = EMirrorOp::None);
		TArray<PieceShapeData> mDataStorage;
		uint8 mDataIndexMap[EMirrorOp::COUNT][DirType::RestValue];
	};

	struct Piece
	{
		PieceShape* shape;
		DirType dir;
		EMirrorOp::Type mirror = EMirrorOp::None;
		bool bCanOperate;

		int index = 0;
		Int16Point2D mapPosLocked;

		Vector2 pos;
		float angle = 0.0f;
		RenderTransform2D renderXForm;
		int clickFrame = 0;

		int indexSolve;
		bool isLocked() const { return indexMapLocked != INDEX_NONE; }

		Vector2 getLTCornerPos() const
		{
			Vector2 offset = shape->getLTCornerOffset(dir);
			return pos + offset;
		}


		void move(Vector2 const& offset)
		{
			pos += offset;
			renderXForm.translateWorld(offset);
		}
		void updateTransform()
		{
			renderXForm = RenderTransform2D::TranslateThenRotate(-shape->pivot, angle);
			renderXForm.translateWorld(pos + shape->pivot);
		}

		PieceShapeData const& getShapeData() const
		{
			return shape->getData(dir, mirror);
		}

		bool hitTest(Vector2 const& pos, Vector2& outHitLocalPos)
		{
			Vector2 lPos = renderXForm.transformInvPositionAssumeNoScale(pos);

			auto const& shapeData = shape->getData(DirType::ValueChecked(0), mirror);
			for (auto const& block : shapeData.blocks)
			{
				if (block.pos.x < lPos.x && lPos.x < block.pos.x + 1 &&
					block.pos.y < lPos.y && lPos.y < block.pos.y + 1)
				{
					outHitLocalPos = lPos;
					return true;
				}
			}
			return false;
		}

	private:
		friend class Level;
		int indexMapLocked = INDEX_NONE;
	};

	struct MapDesc
	{
		uint16  sizeX;
		TArray< uint8 > data;
		Vector2 pos;

		template< class OP >
		void serialize(OP& op);
	};

	class MarkMap
	{
	public:
		enum EBlock
		{
			Normal = 0,
		};

		enum
		{
			LOCK_MASK  = BIT(0),
			BLOCK_MASK = BIT(1),
			MASK_BITS = 2,
			ALL_MASK = BIT(MASK_BITS) - 1,
#define MAKE_MAP_DATA(TYPE, v) (((TYPE) << MASK_BITS ) | v)

			MAP_BLOCK = MAKE_MAP_DATA(EBlock::Normal, BLOCK_MASK),
		};

		uint8 getValue(int x, int y) const
		{
			return mData(x, y);
		}

		uint8 getValue(Vec2i const& pos) const
		{
			return mData(pos.x, pos.y);
		}

		static bool  CanLock(uint8 value)   { return !(value & (LOCK_MASK | BLOCK_MASK)); }
		static bool  HaveBlock(uint8 value) { return !!(value & BLOCK_MASK); }
		static void  AddBlock(uint8& value) { value |= BLOCK_MASK; }
		static void  RemoveBlock(uint8& value) { value &= ~BLOCK_MASK; }
		static bool  IsLocked(uint8 value)  { return !!(value & LOCK_MASK); }
		static void  AddLock(uint8& value)  { value |= LOCK_MASK; }
		static void  RemoveLock(uint8& value) { value &= ~LOCK_MASK; }
		static uint8 GetType(uint8 value)   { return value >> MASK_BITS; }
		static void  SetType(uint8& value, uint8 type) { value = (type << MASK_BITS) | (value & ALL_MASK); }
		bool isInBound(Vec2i const& pos)
		{
			return IsInBound(pos, getBoundSize());
		}

		Vec2i getBoundSize() const
		{
			return Vec2i(mData.getSizeX(), mData.getSizeY());
		}

		bool tryLock(Vec2i const& pos, PieceShapeData const& shapeData);
		void unlock(Vec2i const& pos, PieceShapeData const& shapeData);
		bool tryLockAssumeInBound(Vec2i const& pos, PieceShapeData const& shapeData);
		bool canLockAssumeInBound(Vec2i const& pos, PieceShapeData const& shapeData);
		void lockChecked(Vec2i const& pos, PieceShapeData const& shapeData);
		bool canLock(Vec2i const& pos, PieceShapeData const& shapeData);

		bool checkBound(Vec2i const &pos, PieceShapeData const &shapeData) const;

		bool isFinish() const { return numTotalBlocks == numBlockLocked; }

		void importDesc(MapDesc const& desc);
		void exportDesc(MapDesc& outDesc);
		void resize(int sizeX, int sizeY);

		void toggleBlock(Vec2i const& pos);

		void copyFrom(MarkMap const& rhs, bool bInitState = false);

		bool isSymmetry() const;

		Vector2 mPos = Vector2::Zero();
		int numBlockLocked;
		int numTotalBlocks;
		TGrid2D<uint8> mData;
	};

	struct PieceDesc
	{
		uint16  shapeId;
		Vector2 pos;
		union
		{
			struct
			{
				uint8   dir : 2;
				uint8   bLockOperation : 1;
				uint8   mirror : 2;
				uint8   dummy : 3;
			};
			uint8 stateAndFlags;
		};

		template< class OP >
		void serialize(OP& op);
	};

	struct LevelDesc
	{
		TArray<MapDesc> maps;
		TArray<PieceShapeDesc> shapes;
		TArray<PieceDesc> pieces;

		bool bAllowMirrorOp = false;

		template< class OP >
		void serialize(OP& op);
	};

	class Level
	{
	public:
		int findShapeID(PieceShape* shape);

		bool isFinish() const;

		void importDesc(LevelDesc const& desc);
		void exportDesc(LevelDesc& outDesc);

		Piece* createPiece(PieceShape& shape, DirType dir);
		Piece* createPiece(PieceDesc const& desc);
		PieceShape* createPieceShape(PieceShapeDesc const& desc);
		PieceShape* findPieceShape(PieceShapeDesc const& desc , int& outDir);

		void removePieceShape(PieceShape* shape);

		bool tryLockPiece(Piece& piece);
		bool tryLockPiece(Piece& piece, int mapIndex, Vec2i const& mapPos);
		void unlockPiece(Piece& piece);

		void unlockAllPiece();

		Vector2 calcPiecePos(Piece const& piece, int indexMap, Vec2i const& mapPos , DirType dir );

		TArray< std::unique_ptr< Piece > > mPieces;
		TArray< std::unique_ptr< PieceShape > > mShapes;
		TArray< MarkMap > mMaps;

		bool bAllowMirrorOp = false;
	};

}//namespace SBlock

#endif // SBlockCore_H_CEF300EC_5B0F_406A_942F_AC4CBE559D18