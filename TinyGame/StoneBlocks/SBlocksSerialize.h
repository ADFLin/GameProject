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
			//-----------------------
			LastVersionPlusOne,
			LastVersion = LastVersionPlusOne - 1,
		};
	}

	template< class OP >
	void PieceShapeDesc::serialize(OP& op)
	{
		op & sizeX & data;
		if (OP::IsLoading && op.version() < ELevelSaveVersion::UseBitGird)
		{
			data = std::move(FBitGird::ConvertForm(data, sizeX));
		}

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
		op & sizeX & data;
		if (OP::IsLoading && op.version() < ELevelSaveVersion::UseBitGird)
		{
			data = std::move(FBitGird::ConvertForm(data, sizeX));
		}

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
		op & id & pos & dir;
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
		op & shapes & pieces;
	}

}//namespace SBlocks

#endif // SBlocksSerialize_h__