#pragma once
#ifndef SBlockCore_H_CEF300EC_5B0F_406A_942F_AC4CBE559D18
#define SBlockCore_H_CEF300EC_5B0F_406A_942F_AC4CBE559D18

#include "TCycleNumber.h"
#include "Renderer/RenderTransform2D.h"
#include "DataStructure/Grid2D.h"
#include "Math/Vector2.h"
#include "Serialize/SerializeFwd.h"

#include "ColorName.h"
#include "RenderUtility.h"

#include <memory>
#include <algorithm>
#include "Math/GeometryPrimitive.h"

namespace SBlocks
{
	using DirType = TCycleNumber< 4, uint8 >;
	using Int16Point2D = TVector2< int16 >;
	using RenderTransform2D = Render::RenderTransform2D;
	using Math::Vector2;

	namespace ELevelSaveVersion
	{
		enum
		{
			InitVersion = 0,
			UseBitGird ,
			LastVersionPlusOne,
			LastVersion = LastVersionPlusOne - 1,
		};
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
		static bool Read(std::vector< uint8 > const& bitData, uint32 dataSizeX, int x, int y)
		{
			uint32 index = ToIndex(dataSizeX, x, y);
			return !!(bitData[index] & BIT(x % 8));
		}
		static void Add(std::vector< uint8 >& bitData, uint32 dataSizeX, int x, int y)
		{
			uint32 index = ToIndex(dataSizeX, x, y);
			bitData[index] |= BIT(x % 8);
		}
		static void Remove(std::vector< uint8 >& bitData, uint32 dataSizeX, int x, int y)
		{
			uint32 index = ToIndex(dataSizeX, x, y);
			bitData[index] &= ~BIT(x % 8);
		}

		template< class T >
		static std::vector< uint8 > ConvertForm(std::vector< T >& data, uint32 sizeX)
		{
			std::vector< uint8 > result;
			uint32 sizeY = (data.size() + sizeX - 1) / sizeX;
			uint32 dataSizeX = GetDataSizeX(sizeX);
			result.resize(dataSizeX * sizeY, 0);
			for (uint32 index = 0; index < data.size(); ++index)
			{
				if (data[index])
				{
					uint32 x = index % sizeX;
					uint32 y = index / sizeX;
					Add(result, dataSizeX, x, y);
				}
			}
			return result;
		}
		template< class T >
		static std::vector< T > ConvertTo(std::vector< uint8 >& bitData, uint32 sizeX , T value = T(1) )
		{
			std::vector< T > result;

			uint32 dataSizeX = GetDataSizeX(sizeX);
			uint32 sizeY = (bitData.size() + dataSizeX - 1) / sizeX;
			result.resize(dataSizeX * sizeY, 0);
			for (uint32 index = 0; index < result.size(); ++index)
			{
				uint32 x = index % sizeX;
				uint32 y = index / sizeX;
				if (Read(bitData, dataSizeX, x , y ))
				{
					result[index] = value;
				}
			}
			return result;
		}
	};

	struct PieceShapeDesc
	{
		uint16 sizeX;
		std::vector< uint8 > data;
		Vector2 pivot;

		template< class OP >
		void serialize(OP& op)
		{
			op & sizeX & data & pivot;
			if (OP::IsLoading && op.version() < ELevelSaveVersion::UseBitGird)
			{
				data = std::move(FBitGird::ConvertForm(data, sizeX));
			}
		}
	};
	TYPE_SUPPORT_SERIALIZE_FUNC(PieceShapeDesc);

	using AABB = ::Math::TAABBox< Int16Point2D >;

	struct PieceShape
	{
		struct Data
		{
			Vec2i boundSize;
			std::vector< Int16Point2D > blocks;

			void sortBlocks()
			{
				std::sort(blocks.begin(), blocks.end(), [](Int16Point2D const& lhs, Int16Point2D const& rhs)
				{
					if (lhs.x < rhs.x)
						return true;
					if (lhs.x == rhs.x && lhs.y < rhs.y)
						return true;

					return false;
				});
			}

			bool operator == (Data const& rhs) const
			{
				if (blocks.size() != rhs.blocks.size())
					return false;

				for (int i = 0; i < blocks.size(); ++i)
				{
					if (blocks[i] != rhs.blocks[i])
						return false;
				}

				return true;
			}
		};

		struct Line
		{
			Int16Point2D start;
			Int16Point2D end;
		};
		std::vector< Line > outlines;

		int findSameShape(Data const& data);

		void generateOutline();

		Int16Point2D boundSize;
		Data mDataMap[DirType::RestNumber];
		Vector2 pivot;

		void exportDesc(PieceShapeDesc& outDesc);

		void importDesc(PieceShapeDesc const& desc);

		Vec2i getBoundSize(DirType dir)
		{
			if (dir % 2)
				return Vec2i(boundSize.y, boundSize.x);
			return boundSize;
		}

		Vec2i getCornerPos(DirType dir)
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
	};

	struct Piece
	{
		PieceShape* shape;
		DirType dir;
		bool bLocked;

		EColor::Name color;
		RenderTransform2D xform;
		float angle = 0.0f;
		Vector2 pos;

		int clickFrame = 0;

		Vector2 getLTCornerPos()
		{
			Vector2 lPos = shape->getCornerPos(dir);
			return xform.transformPosition(lPos);
		}

		void updateTransform()
		{
			xform.setIdentity();
			xform.translateWorld(-shape->pivot);
			xform.rotateWorld(angle);
			xform.translateWorld(pos + shape->pivot);
		}

		bool hitTest(Vector2 const& pos, Vector2& outHitLocalPos)
		{
			Vector2 lPos = xform.inverse().transformPosition(pos);

			auto const& shapeData = shape->mDataMap[0];
			for (auto const& block : shapeData.blocks)
			{
				if (block.x < lPos.x && lPos.x < block.x + 1 &&
					block.y < lPos.y && lPos.y < block.y + 1)
				{
					outHitLocalPos = lPos;
					return true;
				}
			}
			return false;
		}

	};

	struct MapDesc
	{
		int sizeX;
		std::vector< uint8 > data;

		template< class OP >
		void serialize(OP& op)
		{
			op & sizeX & data;
			if (OP::IsLoading && op.version() < ELevelSaveVersion::UseBitGird)
			{
				data = std::move( FBitGird::ConvertForm(data, sizeX) );
			}
		}
	};
	TYPE_SUPPORT_SERIALIZE_FUNC(MapDesc);

	class MarkMap
	{
	public:
		enum
		{
			MAP_BLOCK = 1,
			PIECE_BLOCK = 2,
		};

		int getValue(int x, int y)
		{
			return mData(x, y);
		}

		bool isInBound(Vec2i const& pos)
		{
			return 0 <= pos.x && pos.x < mData.getSizeX() &&
				0 <= pos.y && pos.y < mData.getSizeY();
		}


		Vec2i getBoundSize() const
		{
			return Vec2i(mData.getSizeX(), mData.getSizeY());
		}

		bool tryLock(Vec2i const& pos, PieceShape::Data const& shapeData);
		void unlock(Vec2i const& pos, PieceShape::Data const& shapeData);
		bool canLock(Vec2i const& pos, PieceShape::Data const& shapeData);

		bool isFinish() { return numTotalBlocks == numBlockLocked; }

		void improtDesc(MapDesc const& desc);
		void exportDesc(MapDesc& outDesc);
		void resize(int sizeX, int sizeY);

		void toggleDataType(Vec2i const& pos);

		void copy(MarkMap const& rhs)
		{
			numBlockLocked = 0;
			numTotalBlocks = rhs.numTotalBlocks;
			mData.resize(rhs.mData.getSizeX(), rhs.mData.getSizeY());
			mData.fillValue(0);
			for (int i = 0; i < mData.getRawDataSize(); ++i)
			{
				if ( rhs.mData[i] == PIECE_BLOCK )
				{
					mData[i] = PIECE_BLOCK;
				}
			}
		}
		int numBlockLocked;
		int numTotalBlocks;
		TGrid2D<uint8> mData;
	};

	struct PieceDesc
	{
		uint16  id;
		Vector2 pos;
		uint8   dir;

		template< class OP >
		void serialize(OP& op)
		{
			op & id & pos & dir;
		}
	};
	TYPE_SUPPORT_SERIALIZE_FUNC(PieceDesc);


	struct LevelDesc
	{
		MapDesc map;
		std::vector<PieceShapeDesc> shapes;
		std::vector<PieceDesc> pieces;

		template< class OP >
		void serialize(OP& op)
		{
			op & map & shapes & pieces;
		}
	};
	TYPE_SUPPORT_SERIALIZE_FUNC(LevelDesc);


	class Level
	{
	public:
		int findShapeID(PieceShape* shape);

		void importDesc(LevelDesc const& desc);
		void exportDesc(LevelDesc& outDesc);

		Piece* createPiece(PieceShape& shape);
		Piece* createPiece(PieceDesc const& desc);
		PieceShape* createPieceShape(PieceShapeDesc const& desc);

		static Vec2i GetLockMapPos(Piece& piece);

		bool tryLockPiece(Piece& piece);
		void unlockPiece(Piece& piece);

		void unlockAllPiece();

		std::vector< std::unique_ptr< Piece > > mPieces;
		std::vector< std::unique_ptr< PieceShape > > mShapes;

		MarkMap mMap;
	};

}//namespace SBlock

#endif // SBlockCore_H_CEF300EC_5B0F_406A_942F_AC4CBE559D18