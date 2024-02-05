#ifndef LightObject_h__
#define LightObject_h__

#include "Object.h"
#include "Light.h"


class LightObject : public LevelObject
	              , public Light
{
	DECLARE_OBJECT_CLASS( LightObject , LevelObject )
public:

	LightObject();
	~LightObject();

	void init();
	virtual void onSpawn( unsigned flag );
	virtual void onDestroy( unsigned flag );
	virtual void tick();
	virtual void setupDefault();


	REFLECT_STRUCT_BEGIN(LightObject)
		REF_BASE_CLASS(LevelObject)
		REF_PROPERTY(radius, "Radius")
		REF_PROPERTY(color, "Color")
		REF_PROPERTY(intensity, "Intensity")
		REF_PROPERTY(drawShadow, "DrawShadow")
	REFLECT_STRUCT_END()
};





#endif // LightObject_h__