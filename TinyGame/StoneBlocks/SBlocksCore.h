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
	using Math::Matrix2;
	using Math::Vector2;

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
		static std::vector< T > ConvertTo(std::vector< uint8 >& bitData, uint32 sizeX, T value = T(1))
		{
			std::vector< T > result;

			uint32 dataSizeX = GetDataSizeX(sizeX);
			uint32 sizeY = (bitData.size() + dataSizeX - 1) / sizeX;
			result.resize(dataSizeX * sizeY, 0);
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

	struct PieceShapeDesc
	{
		uint16 sizeX;
		std::vector< uint8 > data;

		bool	bUseCustomPivot;
		Vector2 customPivot;

		template< class OP >
		void serialize(OP& op);
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(PieceShapeDesc);

	using AABB = ::Math::TAABBox< Int16Point2D >;


	struct PieceShapeData
	{
		Vec2i boundSize;
		std::vector< Int16Point2D > blocks;

		void initialize(PieceShapeDesc const& desc);

		void sortBlocks();

		bool operator == (PieceShapeData const& rhs) const;
	};


	struct PieceShape
	{

		struct Line
		{
			Int16Point2D start;
			Int16Point2D end;
		};
		std::vector< Line > outlines;

		int findSameShape(PieceShapeData const& data);
		int getBlockCount() const
		{
			return mDataMap[0].blocks.size();
		}
		void generateOutline();

		Int16Point2D boundSize;
		PieceShapeData mDataMap[DirType::RestNumber];
		Vector2 pivot;

		int getDifferentShapeDirs(int outDirs[4]);

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

		int index = 0;
		EColor::Name color;
		RenderTransform2D xform;
		RenderTransform2D xFormRender;
		float angle = 0.0f;
		Vector2 pos;

		int clickFrame = 0;


		Vector2 getLTCornerOffset() const
		{
			return getLTCornerPos() - pos;
		}

		Vector2 getLTCornerPos() const
		{
			Vector2 lPos = shape->getCornerPos(dir);
			return xform.transformPosition(lPos);
		}

		static Matrix2 GetRotation(DirType dir)
		{
			switch (dir)
			{
			case 0: return Matrix2::Identity();
			case 1: return Matrix2(0, 1, -1, 0);
			case 2: return Matrix2(-1, 0, 0, -1);
			case 3: return Matrix2(0,-1, 1 ,0);
			}
			NEVER_REACH("GetRotation");
			return Matrix2::Identity();
		}

		void updateTransform()
		{
			xform.setIdentity();
			xform.translateWorld(-shape->pivot);
			xform.rotateWorld(GetRotation(dir));
			xform.translateWorld(pos + shape->pivot);

			xFormRender.setIdentity();
			xFormRender.translateWorld(-shape->pivot);
			xFormRender.rotateWorld(angle);
			xFormRender.translateWorld(pos + shape->pivot);
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
		void serialize(OP& op);
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

		int getValue(Vec2i const& pos)
		{
			return mData(pos.x, pos.y);
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

		bool tryLock(Vec2i const& pos, PieceShapeData const& shapeData);
		void unlock(Vec2i const& pos, PieceShapeData const& shapeData);
		bool tryLockAssumeInBound(Vec2i const& pos, PieceShapeData const& shapeData);
		bool canLockAssumeInBound(Vec2i const& pos, PieceShapeData const& shapeData);
		void lockChecked(Vec2i const& pos, PieceShapeData const& shapeData);
		bool canLock(Vec2i const& pos, PieceShapeData const& shapeData);

		bool isFinish() { return numTotalBlocks == numBlockLocked; }

		void improtDesc(MapDesc const& desc);
		void exportDesc(MapDesc& outDesc);
		void resize(int sizeX, int sizeY);

		void toggleDataType(Vec2i const& pos);

		void copy(MarkMap const& rhs, bool bInitState = false);
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
		void serialize(OP& op);
	};
	TYPE_SUPPORT_SERIALIZE_FUNC(PieceDesc);


	struct LevelDesc
	{
		MapDesc map;
		std::vector<PieceShapeDesc> shapes;
		std::vector<PieceDesc> pieces;

		template< class OP >
		void serialize(OP& op);
	};
	TYPE_SUPPORT_SERIALIZE_FUNC(LevelDesc);


	class Level
	{
	public:
		int findShapeID(PieceShape* shape);

		void importDesc(LevelDesc const& desc);
		void exportDesc(LevelDesc& outDesc);

		Piece* createPiece(PieceShape& shape, DirType dir);
		Piece* createPiece(PieceDesc const& desc);
		PieceShape* createPieceShape(PieceShapeDesc const& desc);
		PieceShape* findPieceShape(PieceShapeDesc const& desc , int& outDir);

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