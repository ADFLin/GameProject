#ifndef CommonFwd_H
#define CommonFwd_H

#include <cmath>
#include <cstdlib>
#include "TVector2.h"

namespace Shoot2D
{
	class Object;
	class Action;
	class Weapon;
	class BulletGen;
	class RenderEngine;
	class ObjectManger;
	class ObjectCreator;
	struct ObjModel;

	enum ModelId;
	enum ObjStats;
	enum ObjTeam;
	enum GeomType;

	void spawnObject(Object* obj);

	typedef TVector2<float> Vec2D;

	struct Rect_t
	{
		Vec2D Max,Min;
		bool isInRange(Vec2D const& pos)
		{
			return Min.x <= pos.x && pos.x <= Max.x &&
				Min.y <= pos.y && pos.y <= Max.y;
		}
	};

}//namespace Shoot2D

#endif CommonFwd_H