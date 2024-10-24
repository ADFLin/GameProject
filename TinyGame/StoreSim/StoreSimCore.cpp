#include "StoreSimCore.h"

namespace StoreSim
{

	AABB EquipmentClass::calcAreaBound(XForm2D const& xForm)
	{
		auto areaList = getAreaList();
		CHECK(areaList.size() > 0);
#if 0
		AABB localBound = areaList[0].bound;
		for (int i = 1; i < areaList.size(); ++i)
		{
			localBound += areaList[i].bound;
		}

		Vector2 v[4];
		GetVertices(localBound, xForm, v);
		AABB result; result.invalidate();
		for (int i = 0; i < ARRAY_SIZE(v); ++i)
		{
			result.addPoint(v[i]);
		}
		return result;
#else
		AABB result; result.invalidate();
		for (int i = 0; i < areaList.size(); ++i)
		{
			Vector2 v[4];
			GetVertices(areaList[i].bound, xForm, v);
			for (int i = 0; i < ARRAY_SIZE(v); ++i)
			{
				result.addPoint(v[i]);
			}
		}
		return result;
#endif
	}

	void Equipment::updateNavArea()
	{
		navAreas.clear();

		auto areaList = mClass->getAreaList();
		for (auto const& area : areaList)
		{
			if (CanWalkable(area.flags))
				continue;

			Bsp2D::PolyArea polyArea;
			BuildArea(polyArea, area.bound, transform);
			navAreas.push_back(std::move(polyArea));
		}
	}

}//namespace StoreSim


