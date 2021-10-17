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
	}

	template< class OP >
	void PieceDesc::serialize(OP& op)
	{
		op & id & pos & dir;
	}

	template< class OP >
	void LevelDesc::serialize(OP& op)
	{
		op & map & shapes & pieces;
	}

}//namespace SBlocks

#endif // SBlocksSerialize_h__