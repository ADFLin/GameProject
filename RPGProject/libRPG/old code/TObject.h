#ifndef TObject_h__
#define TObject_h__

#include "common.h"
#include "TPhyEntity.h"

enum  ModelType;
struct TActorModelData;
struct DamageInfo;
class TEffectBase;
class TLevel;
enum  ActivityType;
enum  ActorState;

struct TObjModelData;
class  TPhyEntity;


class TObject : public TPhyEntity
{
	DESCRIBLE_ENTITY_CLASS( TObject , TPhyEntity );

public:
	TObject( OBJECTid id , TPhyBodyInfo const& info );
	TObject( unsigned modelID , XForm const& trans = XForm( Quat(0,0,0,1) ) );

	~TObject();

	void OnSpawn();
	static TObjModelData& getModelData( unsigned modelID );

	void setKimematic( bool beK );
	void updateFrame()
	{

	}
	void OnCollision( TPhyEntity* colObj , Float depth , 
		              Vec3D const& selfPos , Vec3D const& colObjPos , 
					  Vec3D const& normalOnColObj )
	{

	}

	FnObject& getFlyObj(){ return m_obj; }

	

	FnObject m_obj;
};





#endif // TObject_h__