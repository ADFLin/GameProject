#include "StoreSimSerialize.h"


namespace StreamSerializer
{
	using namespace  StoreSim;
	template< typename OP >
	int Serialize(XForm2D& xform, OP& op)
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
		return 0;
	}
}


namespace StoreSim
{
	void Equipment::serialize(IArchive& ar)
	{
		ar & transform;
	}

}//namespace StoreSim

