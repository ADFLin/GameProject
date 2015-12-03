#ifndef ObjModel_h__
#define ObjModel_h__

#include "CommonFwd.h"
#include "Object.h"
#include "Singleton.h"

#include <vector>

namespace Shoot2D
{
	enum ModelId
	{
		MD_NO_MODEL = -1,
		MD_PLAYER ,
		MD_BASE,
		MD_C,
		MD_BULLET_1,
		MD_BULLET_2,
		MD_BULLET_3,
		MD_LEVEL_UP,
		MD_SMALL_BOW,
		MD_BIG_BOW,
		MD_END,
	};


	struct ObjModel
	{
		GeomType geomType;
		union 
		{
			struct 
			{ 
				float x , y; 
			};
			float r;
		};
		Vec2D getCenterOffset() const;
		ModelId  id;
		ObjModel(){}

		ObjModel(Vec2D const& l , ModelId id = MD_NO_MODEL);
		ObjModel(float vr , ModelId id = MD_NO_MODEL );
	};



	class ObjModelManger : public SingletonT<ObjModelManger>
	{
	public:
		ObjModelManger();
		ObjModel const& getModel(unsigned id)
		{
			return m_models[id];
		}
		typedef std::vector< ObjModel > ModelVector;
		ModelVector m_models;
	};

}//namespace Shoot2D

#endif // ObjModel_h__