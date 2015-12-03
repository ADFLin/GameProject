#include "ObjModel.h"

#include "Common.h"

namespace Shoot2D
{
	ObjModelManger::ObjModelManger()
	{
		m_models.resize( MD_END );
		m_models[ MD_PLAYER    ] = ObjModel( Vec2D(30,40)  , MD_BIG_BOW  );
		m_models[ MD_BASE      ] = ObjModel( Vec2D(25,25)  , MD_BIG_BOW  );
		m_models[ MD_BULLET_1  ] = ObjModel( 3 , MD_SMALL_BOW );
		m_models[ MD_LEVEL_UP  ] = ObjModel( Vec2D(20,20) );
		m_models[ MD_SMALL_BOW ] = ObjModel( 2 );
		m_models[ MD_BIG_BOW   ] = ObjModel( 5 );
	}

	ObjModel::ObjModel( Vec2D const& l , ModelId bow) 
		:geomType(GEOM_RECT)
		,id( bow )
	{
		x = l.x;
		y = l.y;
	}

	ObjModel::ObjModel( float vr ,ModelId bow )
		:geomType(GEOM_CIRCLE)
		,id( bow )
	{
		r = vr;
	}

	Vec2D ObjModel::getCenterOffset() const
	{
		switch ( geomType )
		{
		case GEOM_RECT:
			return Vec2D( x/2 , y/2 );
		case GEOM_CIRCLE:
			return Vec2D( 0 , 0 );
		}
		return Vec2D(0,0);
	}

}//namespace Shoot2D
