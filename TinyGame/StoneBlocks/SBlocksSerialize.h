#ifndef SBlocksSerialize_h__
#define SBlocksSerialize_h__

#include "SBlocksCore.h"

namespace SBlocks
{
	namespace ELevelSaveVersion
	{
		enum
		{
			InitVersion = 0,
			UseBitGird,
			ShapeCustomPivotOption,
			MultiMarkMap ,
			AutoConvertBitGird,
			MapSizeXIntToUint16,
			MirrorOpSupported,
			PieceOwnMirror,

			//-----------------------
			LastVersionPlusOne,
			LastVersion = LastVersionPlusOne - 1,
		};
	}

	template < typename OP , typename SizeType>
	void SerializeGridData(OP& op, TArray<uint8>& data , SizeType& sizeX)
	{
		if (OP::IsLoading)
		{
			int32 version = op.version();

			bool bBitData = false;
			if (version < ELevelSaveVersion::UseBitGird)
			{
				bBitData = false;
			}
			else if (version < ELevelSaveVersion::AutoConvertBitGird)
			{
				bBitData = true;
			}
			else
			{
				op & bBitData;
			}

			if (bBitData)
			{
				TArray<uint8> temp;
				op & temp;
				data = FBitGird::ConvertTo<uint8>(temp, sizeX);
			}
			else
			{
				op & data;
			}
		}
		else
		{
			bool bBitData = std::find_if(data.begin(), data.end(), [](uint8 value) { return (value & ~BIT(0)) != 0; }) == data.end();
			op & bBitData;
			if (bBitData)
			{
				TArray<uint8> temp = FBitGird::ConvertForm(data, sizeX);
				op & temp;
			}
			else
			{
				op & data;
			}
		}
	}


	template< class OP >
	void PieceShapeDesc::serialize(OP& op)
	{
		op & sizeX;
		SerializeGridData(op, data, sizeX);

		if (OP::IsLoading && op.version() < ELevelSaveVersion::ShapeCustomPivotOption)
		{
			bUseCustomPivot = true;
			op & customPivot;
		}
		else
		{
			op & bUseCustomPivot;
			if (bUseCustomPivot)
			{
				op & customPivot;
			}
		}
	}

	template< class OP >
	void MapDesc::serialize(OP& op)
	{
		if (OP::IsLoading && op.version() < ELevelSaveVersion::MapSizeXIntToUint16)
		{
			int32 temp;
			op & temp;
			sizeX = temp;
		}
		else
		{
			op & sizeX;
		}

		SerializeGridData(op, data, sizeX);
	
		if (OP::IsLoading && op.version() < ELevelSaveVersion::MultiMarkMap)
		{
			pos = Vector2::Zero();
		}
		else
		{
			op & pos;
		}
	}

	template< class OP >
	void PieceDesc::serialize(OP& op)
	{
		op & shapeId & pos & stateAndFlags;
		if (OP::IsLoading && op.version() < ELevelSaveVersion::PieceOwnMirror)
		{
			mirror = EMirrorOp::None;
		}
	}

	template< class OP >
	void LevelDesc::serialize(OP& op)
	{
		if (OP::IsLoading && op.version() < ELevelSaveVersion::MultiMarkMap)
		{
			maps.resize(1);
			op & maps[0];
		}
		else
		{
			op & maps;
		}

		if (OP::IsLoading && op.version() < ELevelSaveVersion::MirrorOpSupported)
		{
			bAllowMirrorOp = false;
		}
		else
		{
			op & bAllowMirrorOp;
		}
		op & shapes & pieces;
	}

}//namespace SBlocks

#endif // SBlocksSerialize_h__