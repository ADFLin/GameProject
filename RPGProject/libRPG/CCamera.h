#ifndef CCamera_H
#define CCamera_H

#include "common.h"

Vec3D const gLocalCamViewDir( 0,0,-1 );
Vec3D const gLocalCamRightDir( 0 , 1 , 0 );
class CameraView
{
public:
	CameraView(Vec3D const& pos = Vec3D(0,0,0) , Quat const& rotation = Quat(0,0,0,1))
		:mPos(pos),mRotation(rotation){ }

	CameraView(Vec3D const& camPos , Vec3D const& lookPos);

	Quat const&  getOrientation() const { return mRotation; }
	Vec3D const& getPosition() const { return mPos; }
	void         setOrientation( Quat const& q ){  mRotation = q; }
	void         setPosition( Vec3D const& Pos ){ mPos = Pos; }

	Vec3D getLookPos() const { return getPosition() + getViewDir(); }
	Vec3D getViewDir() const {	return mRotation.rotate( Vec3D(1,0,0) ); }

	void  setViewDir( Vec3D const& dir);
	void  setLookPos( Vec3D const& lookPos )           {  setViewDir( lookPos - getPosition() );  }
	void  move( Vec3D const& offset )                  {  mPos += offset;  }
	void  moveLocal( Vec3D const& offset)              {  mPos += mRotation.rotate( offset );  }
	void  rotateLocal( Vec3D const& axis , float angle ){  mRotation = mRotation * Quat::Rotate( axis , angle );  }
	
	//Mat3x3 getRotationMatrix(){	return Mat3x3( m_rotation ); }
	Vec3D  calcRayDirection( int xPos, int yPos , float fov , float aspect )
	{
		//FIXME
		return Vec3D(0, 0, 1);
	}

	Vec3D  transLocalDir( Vec3D const& lDir ){  return mRotation.rotate( lDir );  }
	Vec3D  calcScreenRayDir( int xPos ,int yPos , int screenWidth , int screenHeight );
private:
	Quat  mRotation;
	Vec3D mPos;
};

class CamControl
{
public:
	CamControl( CameraView* cam )
		:mCamera( cam ){}

	void changeCamera( CameraView* cam ){ mCamera = cam; }
	//virtual void updateCamera( Vec3D const& playerPos , Vec3D const& playerDir , Vec3D const& goalPos ){}
protected:
	CameraView* mCamera;
};

class FristViewCamControl : public CamControl
{
public:
	FristViewCamControl( CameraView* cam );

	void  moveForward();
	void  moveBack();
	void  moveLeft();
	void  moveRight();

	void  setTurnSpeed(float TurnSpeed) { m_TurnSpeed = TurnSpeed; }
	void  setMoveSpeed(float MoveSpeed) { m_MoveSpeed = MoveSpeed; }

	float getMoveSpeed() const { return m_MoveSpeed; }
	float getTurnSpeed() const { return m_TurnSpeed; }



	void  trunRight( float angle );
	void  rotateByMouse( int dx , int dy );
protected:

	float m_MoveSpeed;
	float m_TurnSpeed;
};


#include "GameObjectComponent.h"

class FollowCamControl : public CamControl
{
public:
	FollowCamControl( CameraView* cam );

	void     setObject( ILevelObject* obj );
	void     setLocalObjFront(Vec3D const& val) { m_localObjFront = val; }

	float    getCamHeight() const { return m_camHeight; }

	float    getObjLookDist() const { return m_objLookDist; }
	float    getCamToObjDist() const { return m_camToObjDist; }
	float    getLookHeight() const { return m_lookHeight; }
	void     setCamHeight(float val) { m_camHeight = val; }
	void     setObjLookDist( float objLookDist) { m_objLookDist = objLookDist; }
	void     setCamToObjDist(float val) { m_camToObjDist = val; }
	void     setMoveAnglurSpeed(float val) { m_moveAnglurSpeed = val; }
	void     setMoveRadialSpeed(float val) { m_moveRadialSpeed = val; }
	void     setTurnSpeed(float val) { m_turnSpeed = val; }

	Vec3D    computeLookPos( Vec3D const& entityPos );

	void     trunViewDir( Vec3D const& goalViewDir , float maxRotateAngle );
	void     move( Vec3D const& entityPos , Vec3D const& goalCamPos , 
		           Vec3D const& ObjLookDir , bool chViewDir = true );

protected:

	Vec3D    m_prevObjPos;


	float    mAnglurAcc;
	float    mMaxAnglurSSpeed;
	float    m_moveRadialSpeed;
	float    m_moveAnglurSpeed;
	float    m_turnSpeed;

	Vec3D    m_goalCamPos;

	Vec3D    m_orgCamPos;

	float    m_objLookDist;
	float    m_camHeight;
	float    m_lookHeight;
	float    m_camToObjDist;



	Vec3D    m_localObjFront;
	EHandle  m_obj;
};



#endif //CCamera_H
