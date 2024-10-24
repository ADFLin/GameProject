#pragma once
#ifndef StoreSimSerialize_H_FC694783_8C34_4ED1_B737_EF80FF885FBC
#define StoreSimSerialize_H_FC694783_8C34_4ED1_B737_EF80FF885FBC

#include "StoreSimCore.h"
#include "Serialize/DataStream.h"

namespace StoreSim
{
	namespace ELevelSaveVersion
	{
		enum
		{
			InitVersion = 0,

			//-----------------------
			LastVersionPlusOne,
			LastVersion = LastVersionPlusOne - 1,
		};
	}

	template< typename OP >
	void Serialize(XForm2D& xform, OP& op)
	{
		if (OP::IsLoading)
		{
			Vector2 pos;
			float angle;
			op & pos & angle;
			xform.setTranslation(pos);
			xform.setRoatation(angle);
		}
		else
		{
			Vector2 pos = xform.getPos();
			float angle = xform.getRotateAngle();
			op & pos & angle;
		}
	}
}
#endif // StoreSimSerialize_H_FC694783_8C34_4ED1_B737_EF80FF885FBC
