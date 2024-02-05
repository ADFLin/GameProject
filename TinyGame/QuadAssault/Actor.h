#ifndef Actor_h__
#define Actor_h__

#include "Object.h"

class Actor : public LevelObject
{

	DECLARE_OBJECT_CLASS( Actor , LevelObject )
public:

	Actor()
	{
		rotation = 0.0f;
	}

	float getRotation() const { return rotation; }
	void  setRotation( float theta )
	{ 
		rotation = theta;
		while(rotation > 2 * Math::PI )
			rotation -= 2 * Math::PI;
		while(rotation < 2 * Math::PI )
			rotation += 2 * Math::PI;
	}
	void  rotate(float theta){ setRotation( rotation + theta ); }

protected:

	float rotation;

	REFLECT_STRUCT_BEGIN(Actor)
		REF_BASE_CLASS(LevelObject)
		REF_PROPERTY(rotation, "Rotation")
	REFLECT_STRUCT_END()

};

#endif // Actor_h__
