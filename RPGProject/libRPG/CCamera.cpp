#include "ProjectPCH.h"
#include "CCamera.h"

#include "UtilityMath.h"

#include "LinearMath/btQuaternion.h"
void CameraView::setViewDir( const Vec3D& dir )
{
	float yaw , pitch;
	UM_VecToYawPitch( dir , yaw , pitch );
	mRotation.setEulerZYX( yaw , pitch , 0 );	
}

Vec3D CameraView::calcScreenRayDir( int xPos ,int yPos , int screenWidth , int screenHeight )
{
	float aspect = float( screenHeight ) / screenWidth;

	float dx =( float(xPos)/screenWidth - float(0.5) ) * aspect;
	float dy =  float(yPos)/screenHeight - float(0.5);

	Vec3D rayTo = getViewDir() - dx * transLocalDir( Vec3D(0,1,0) ) - dy * transLocalDir( Vec3D(0,0,1) );
	rayTo.normalize();
	return rayTo;
}

FirstViewCamControl::FirstViewCamControl( CameraView* cam ) 
:CamControl( cam )
{
	m_MoveSpeed = 20;
	m_TurnSpeed = 0.005f;
}

void FirstViewCamControl::rotateByMouse( int dx , int dy )
{
	Vec3D dir = mCamera->getViewDir();
	float yaw , pitch;
	UM_VecToYawPitch( dir , yaw , pitch );

	yaw   -= m_TurnSpeed * float( dx );
	pitch += m_TurnSpeed * float( dy );
	//if ( pitch < 0 )
	//	pitch = 0;
	//else if ( pitch > Math::DegToRad(180) )
	//	pitch = Math::DegToRad(180);

	Quat q;
	q.setEulerZYX( yaw , pitch , 0 );
	mCamera->setOrientation( q );
}


void FirstViewCamControl::trunRight( float angle )
{
	Vec3D dir = mCamera->getViewDir();
	float yaw , pitch;
	UM_VecToYawPitch( dir , yaw , pitch );

	yaw   -= angle;

	Quat q;
	q.setEulerZYX( yaw , pitch , 0 );
	mCamera->setOrientation( q );

}


void FirstViewCamControl::moveForward()
{
	mCamera->move( m_MoveSpeed* mCamera->getViewDir() );
}
void FirstViewCamControl::moveBack()
{
	mCamera->move( (-m_MoveSpeed )* mCamera->getViewDir() );
}
void FirstViewCamControl::moveLeft()
{
	mCamera->moveLocal(  m_MoveSpeed * Vec3D(0,1,0) );
}
void FirstViewCamControl::moveRight()
{
	mCamera->moveLocal( -m_MoveSpeed * Vec3D(0,1,0) );
}

FollowCamControl::FollowCamControl( CameraView* cam )
	:CamControl( cam )
	,m_camToObjDist( 600 )
	,m_objLookDist( 600 )
	,m_localObjFront(0,-1,0)
	,m_camHeight( 100 )
	,m_lookHeight( 0 )
	,m_moveAnglurSpeed( Math::DegToRad( 8 ) )
	,m_moveRadialSpeed( 500 )
	,m_turnSpeed( Math::DegToRad(50))
{

}

void FollowCamControl::move( Vec3D const& entityPos , Vec3D const& goalCamPos , Vec3D const& ObjLookDir , bool chViewDir )
{
	Vec3D goalLookPos = entityPos + ObjLookDir *  m_objLookDist + m_lookHeight * Vec3D(0,0,1);
	Vec3D camPos;

	if ( DistanceSqure( goalCamPos ,mCamera->getPosition() ) < 5 * 5 )
	{
		camPos = goalCamPos;
	}
	else
	{
		Vec3D sDir = mCamera->getPosition() - goalLookPos;
		Vec3D gDir = goalCamPos - goalLookPos;

		float sLen = sDir.length();
		float gLen = gDir.length();

		float yaw , pitch;
		UM_VecToYawPitch( sDir , yaw , pitch );
		Quat curQuat; 
		curQuat.setEulerZYX( yaw , pitch , 0 );

		UM_VecToYawPitch( gDir , yaw , pitch );
		Quat goalQuat; 
		goalQuat.setEulerZYX( yaw , pitch , 0 );

		float dotVal = ( sDir.dot( gDir ) / sLen ) / gLen;
		float angle = acos( TMin( 1.0f , dotVal ) );
		float rotateAngle = ( m_moveAnglurSpeed * g_GlobalVal.frameTime );

		if ( rotateAngle > angle )
		{
			curQuat = goalQuat;
		}
		else
		{
			curQuat = UM_Slerp( curQuat , goalQuat , rotateAngle / angle );
		}


		float dL = gLen - sLen;
		if ( dL > 0 )
			sLen += TMin( m_moveRadialSpeed * g_GlobalVal.frameTime , dL );
		else
			sLen -= TMin( m_moveRadialSpeed * g_GlobalVal.frameTime , -dL );

		camPos = curQuat.rotate( sLen * Vec3D(1,0,0) ) + goalLookPos;
	}

	Vec3D lookDir = goalLookPos - camPos;
	lookDir[2] = 0;
	float camToObjDist = lookDir.length();
	Vec3D lookPos = entityPos + lookDir * ( m_objLookDist / camToObjDist ) + m_lookHeight * Vec3D(0,0,1);

	mCamera->setPosition( camPos );

	if ( chViewDir )
	{
		Vec3D goalLookDir = goalLookPos - goalCamPos;
		Vec3D curLookDir  = mCamera->getViewDir();


		float yaw , pitch;
		UM_VecToYawPitch( curLookDir  , yaw , pitch );
		Quat curQuat; 
		curQuat.setEulerZYX( yaw , pitch , 0 );

		UM_VecToYawPitch( goalLookDir , yaw , pitch );
		Quat goalQuat; 
		goalQuat.setEulerZYX( yaw , pitch , 0 );


		float angle = UM_Angle( curLookDir , goalLookDir );
		float rotateAngle = ( m_turnSpeed * g_GlobalVal.frameTime );

		if ( rotateAngle > angle )
		{
			curQuat = goalQuat;
		}
		else
		{
			curQuat = UM_Slerp( curQuat , goalQuat , rotateAngle / angle );
		}

		mCamera->setViewDir( curQuat.rotate( Vec3D(1,0,0) ) );
	}
}

Vec3D FollowCamControl::computeLookPos( Vec3D const& entityPos )
{
	Vec3D dir = entityPos - mCamera->getPosition();
	dir[2] = 0;
	dir.normalize();

	Vec3D lookPos = mCamera->getPosition();
	lookPos += dir * ( m_objLookDist + m_camToObjDist ); 
	lookPos += ( m_lookHeight - m_camHeight ) * Vec3D(0,0,1);

	return lookPos;
}

void FollowCamControl::trunViewDir( Vec3D const& goalViewDir , float maxRotateAngle )
{
	Vec3D sDir = mCamera->getViewDir();
	Vec3D gDir = goalViewDir;

	float sLen = sDir.length();
	float gLen = gDir.length();

	float yaw , pitch;
	UM_VecToYawPitch( sDir , yaw , pitch );
	Quat curQuat = mCamera->getOrientation();

	UM_VecToYawPitch( gDir , yaw , pitch );
	Quat goalQuat; 
	goalQuat.setEulerZYX( yaw , pitch , 0 );

	float dotVal = ( sDir.dot( gDir ) / sLen ) / gLen;
	float angle = Math::ACos( TMin( 1.0f , dotVal ) );

	if ( maxRotateAngle > angle )
	{
		curQuat = goalQuat;
	}
	else
	{
		curQuat = UM_Slerp( curQuat , goalQuat , maxRotateAngle / angle );
	}

	mCamera->setOrientation( curQuat );
}

void FollowCamControl::setObject( ILevelObject* obj )
{
	//m_prevObjPos = obj->getPosition();
	m_obj = obj;
}
