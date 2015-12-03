#ifndef GameObjectComponent_h__
#define GameObjectComponent_h__

#include "GameObject.h"

#include "SpatialComponent.h"
#include "common.h"
#include "CEntity.h"
#include "CHandle.h"
#include "Thinkable.h"

#include "CFTransformUtility.h"

struct DamageInfo;

class ISpatialComponent;

enum LevelObjectType
{
	LOT_UNKNOWN = 0,
	LOT_ACTOR   = BIT( 0 ),
	LOT_OBJECT  = BIT( 1 ),
	LOT_PLAYER  = BIT( 2 ),
	LOT_NPC     = BIT( 3 ),
};


class  CSceneLevel;

struct GameObjectInitHelper
{
	CFWorld*           cfWorld;
	CSceneLevel*       sceneLevel;
};

enum LevelEvent
{
	
};

class ILevelObject : public IGameObjectExtension
	               , public TRefObj
				   , public Thinkable
{
	DESCRIBLE_CLASS( ILevelObject , IGameObjectExtension )
public:
	ILevelObject();

	//virtual
	bool init( GameObject* gameObj , GameObjectInitHelper const& helper );
	void postInit();
	void release();
	void update( long time );

	//
	virtual void  takeDamage( DamageInfo const& info ){}
	virtual void  onMakeDamage( DamageInfo const& info ){}
	//
	virtual void  onSpawn(){}
	virtual void  onDestroy(){}
	virtual void  onInteract( ILevelObject* logicComp ){}
	Vec3D   getPosition(){ return mSpatialCompoent->getPosition();  }


	ISpatialComponent* getSpatialComponent() const;

	unsigned     getEntityType() const { return mObjectType; }
	bool         isKindOf( unsigned type ){  return ( mObjectType & type ) != 0; }

	CSceneLevel*  getSceneLevel(){ return mSceneLevel; }


	void setAlignDirectionAxis( CFly::AxisEnum frontAxis , CFly::AxisEnum upAxis );
	void setFrontUpDirection( Vec3D const& front , Vec3D const& up );
	void getFrontUpDirection( Vec3D* front , Vec3D* up ) const;

	

	CFly::AxisEnum mFrontAxis;
	CFly::AxisEnum mUpAxis;

protected:

	ISpatialComponent* mSpatialCompoent;
	unsigned           mEntityFlag;
	unsigned           mObjectType;

private:
	friend class  CSceneLevel;
	CSceneLevel*  mSceneLevel;
};


#define DESCRIBLE_GAME_OBJECT_CLASS( thisClass , baseClass )\
	DESCRIBLE_CLASS( thisClass , baseClass )\
	public:\
		static thisClass * downCast( ILevelObject* entity );

#define DEFINE_DOWNCAST( type , CAST_ID )\
	type* type::downCast( ILevelObject* entity )\
	{\
		if ( entity && entity->isKindOf( CAST_ID ) )\
			return static_cast< type* >( entity );\
		return NULL;\
	}\

class ILevelObject;
typedef CHandle< ILevelObject > EHandle;
class CActor;
typedef CHandle< CActor >  AHandle;

#endif // GameObjectComponent_h__
