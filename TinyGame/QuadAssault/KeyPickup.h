#ifndef KeyPickup_h__
#define KeyPickup_h__

#include "ItemPickup.h"

#include "Light.h"


class KeyPickup : public ItemPickup
{
	DECLARE_OBJECT_CLASS( KeyPickup , ItemPickup )

public:
	KeyPickup();
	KeyPickup( Vec2f const& pos , int id );

	virtual void init();
	virtual void tick();
	virtual void onDestroy( unsigned flag );
	virtual void onPick( Player* player );
	virtual void onSpawn( unsigned flag );
	virtual IObjectRenderer* getRenderer();

	virtual void setupDefault();

	virtual void updateEdit();

	
protected:
	int       mId;
	float     mRotation;
	Light     mLight;
	
	friend class KeyPickupRenderer;

	REFLECT_STRUCT_BEGIN(KeyPickup)
		REF_BASE_CLASS(ItemPickup)
		REF_PROPERTY(mId, "DoorId")
	REFLECT_STRUCT_END()

};

#endif // KeyPickup_h__