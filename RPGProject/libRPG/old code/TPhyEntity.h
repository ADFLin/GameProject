#ifndef TPhyEntity_h__
#define TPhyEntity_h__

#include "common.h"
#include "TEntityBase.h"
#include "shareDef.h"

struct DamageInfo;
struct TObjModelData;
class  TObjMotionState;
class  TPhyBodyInfo;
class  TLevel;

enum PhyBodyType
{
	PHY_RIGID_BODY ,
	PHY_SENSOR_BODY,
};

class TPhyEntity : public TLevelEntity 
{
public:
	DESCRIBLE_ENTITY_CLASS( TPhyEntity , TLevelEntity );

	TPhyEntity( PhyBodyType type , TPhyBodyInfo const& info );
	TPhyEntity( PhyBodyType type , unsigned modelID );
	~TPhyEntity();

	virtual void OnTakeDamage( DamageInfo const& info );

	virtual void OnCollision( 
		TPhyEntity* colObj , Float depth ,
		Vec3D const& selfPos , Vec3D const& colObjPos , 
		Vec3D const& normalOnColObj );

	virtual  void OnCollisionTerrain(){}

	PhyShape*     getCollisionShape(){ return m_phyBody->getCollisionShape(); }
	void          setPosition( Vec3D const& pos );
	void          setTransform( XForm const& trans );
	void          setRotation( Quat const& q );
	void          updateTransformFromFlyData();
	PhyRigidBody* getPhyBody(){ return m_phyBody; }
	XForm const&  getTransform() const { return m_phyBody->getWorldTransform(); }
	Vec3D const&  getPosition() const  { return m_phyBody->getCenterOfMassPosition(); }
	virtual Float getMoveSpeed();
	// for call when handle "use key"
	virtual void  interact(){}

	static PhyRigidBody*   createPhyBody( PhyBodyType type , TPhyBodyInfo const& info );
	static TPhyBodyInfo    createPhyBodyInfo( TObjModelData& data );
	static TPhyBodyInfo    createPhyBodyInfo( Float mass , btCollisionShape* shape );
	static TObjModelData&  getModelData( unsigned modelID );

	unsigned         getModelID() const { return m_ModelID; }
	TObjMotionState* getMotionState(){ return m_motionState; }

	void  testCollision( bool beT ){ m_testCol = beT; }
	bool  needTestCollision(){ return m_testCol; }


protected:
	void             setMotionState( TObjMotionState* state );

	bool             m_testCol;
	TObjMotionState* m_motionState;
	PhyRigidBody*    m_phyBody;
	unsigned         m_ModelID;
};


class TObjMotionState : public btMotionState
{
public:
	TObjMotionState( XForm const& trans , OBJECTid id  ,Vec3D& offset = Vec3D(0,0,0) );

	//fly system to phy system
	void getWorldTransform(btTransform& phyTrans ) const;
	//phy system to fly system
	void setWorldTransform(const btTransform& phyTrans );
	void setFlyTransform(const btTransform& phyTrans );
	void setOffset( Vec3D const& offset );

	void setFyObj( OBJECTid objID ){ m_obj.Object( objID ); }

	XForm  m_f2pTrans;
	XForm  m_p2fTrans;
	mutable FnObject m_obj;
};

#endif // TPhyEntity_h__