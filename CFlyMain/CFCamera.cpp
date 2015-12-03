#include "CFlyPCH.h"
#include "CFCamera.h"
#include "CFScene.h"
#include "CFMeshBuilder.h"

namespace CFly
{

	bool gUseFrustumClip = true;

	Camera::Camera( Scene* scene ) 
		:SceneNode( scene )
		,mClipCam( nullptr )
	{
		addFlagBits( BIT( NODE_FRUSTUM_PLANE_DIRTY ) | 
			         BIT( NODE_PROJ_MATRIX_DIRTY )   | 
					 BIT( NODE_VIEW_MATRIX_DIRTY )   );
		setDefault();
	}


	void Camera::setDefault()
	{
		mProjType = CFPT_PERSPECTIVE;
		mFov    = Math::PI * 0.5 * 2 / 3;
		mAspect = 1.0f;
		mZFar   = 10000.0f;
		mZNear  = 0.1f;

	}


	void Camera::updateProjectMatrix() const
	{
		if ( !checkFlag( NODE_PROJ_MATRIX_DIRTY ) )
			return;

		switch( mProjType )
		{
		case CFPT_ORTHOGONAL:
			calcOrthogonalMatrix( mMatProject );
			break;
		case CFPT_PERSPECTIVE:
			calcPerspectiveMatrix( mMatProject );

		}

		removeFlag( NODE_PROJ_MATRIX_DIRTY );
		addFlag( NODE_FRUSTUM_PLANE_DIRTY );

	}

	void Camera::calcPerspectiveMatrix( Matrix4& mat ) const
	{
		// xScale     0          0               0
		//	0        yScale      0               0
		//	0          0      zf/( zn - zf )    -1
		//	0          0      zn*zf/(zn- zf )     0
		//
		//yScale = cot(fovY/2)
		//xScale = yScale / aspect ratio

		float yScale = 1.0f / Math::Tan( 0.5f * mFov );
		float xScale = yScale / mAspect;
		float zFactor = mZFar / ( mZNear - mZFar );

		mat.setValue(
			xScale , 0 , 0 , 0 ,
			0 , yScale , 0 , 0 ,
			0 , 0 , zFactor , -1,
			0, 0, mZNear * zFactor , 0 );
	}


	void Camera::calcOrthogonalMatrix( Matrix4& mat ) const
	{

		//2/(r-l)      0            0           0
		//0            2/(t-b)      0           0
		//0            0            1/(zn-zf)   0
		//(l+r)/(l-r)  (t+b)/(b-t)  zn/(zn-zf)  1

		float w = mScreenRange[1] - mScreenRange[0];
		float h = mScreenRange[3] - mScreenRange[2];
		float zFactor = 1.0f / ( mZNear - mZFar );

		float x = -( mScreenRange[1] + mScreenRange[0] )/ w;
		float y = -( mScreenRange[3] + mScreenRange[2] )/ h;

		mat.setValue(
			2.0f / w , 0 , 0 , 0 ,
			0 , 2.0f / h , 0 , 0 ,
			0 , 0 , zFactor ,  0,
			x , y , mZNear * zFactor , 1 );
	}

	Matrix4 const&  Camera::getViewMatrix()
	{
		if ( checkFlag( NODE_VIEW_MATRIX_DIRTY ) )
		{
			Matrix4 const& mat = getWorldTransform();
			Vector3 pos   = getWorldPosition();
			Vector3 zAxis =  Vector3( mat[8] , mat[9] , mat[10] );
			Vector3 up    =  Vector3( mat[4] , mat[5] , mat[6]  );

			Vector3 xAxis = up.cross( zAxis );
			xAxis.normalize();
			Vector3 yAxis = zAxis.cross( xAxis );

			mMatView.setValue(
				          xAxis.x ,           yAxis.x ,         -zAxis.x , 0 ,
				          xAxis.y ,           yAxis.y ,         -zAxis.y , 0 ,
				          xAxis.z ,           yAxis.z ,         -zAxis.z , 0 ,
				-xAxis.dot( pos ) , -yAxis.dot( pos ) , zAxis.dot( pos ) , 1 );

			removeFlag( NODE_VIEW_MATRIX_DIRTY );

#if 0
			float det;
			Matrix4 invView; 
			mMatView.inverseAffine( invView , det );

			Matrix4 invTrans; 
			getWorldTransform().inverseAffine( invTrans , det );

			Vector3 at = Vector3( 0 , 0 , -1 ) * mat;
			D3DXMATRIX out;
			D3DXMatrixLookAtRH( &out , (D3DXVECTOR3*)&pos , (D3DXVECTOR3*)&at , (D3DXVECTOR3*)&up );

			int i = 1;
#endif
		}
		return mMatView;
	}

	void Camera::setLookAt( Vector3 const& pos , Vector3 const& lookAt , Vector3 const& upDir )
	{
		Vector3 zAxis = pos - lookAt;
		zAxis.normalize();
		Vector3 xAxis = upDir.cross( zAxis );
		xAxis.normalize();
		Vector3 yAxis = zAxis.cross( xAxis );

		Matrix4 trans;

		trans.setValue(
			xAxis.x , xAxis.y , xAxis.z , 0 ,
			yAxis.x , yAxis.y , yAxis.z , 0 ,
			zAxis.x , zAxis.y , zAxis.z , 0 ,
			pos.x   , pos.y   , pos.z   , 1 );

		setWorldTransform( trans );
	}

	Matrix4 const& Camera::getProjectionMartix() const
	{
		updateProjectMatrix();
		return mMatProject;
	}

	void Camera::setScreenRange( float left , float right, float bottom , float top )
	{
		mScreenRange[0] = left;
		mScreenRange[1] = right;

		mScreenRange[2] = bottom;
		mScreenRange[3] = top;

		addFlagBits( BIT( NODE_PROJ_MATRIX_DIRTY ) | BIT( NODE_FRUSTUM_PLANE_DIRTY ) );
	}

	void Camera::updateFrustumPlane()
	{
		Matrix4 const& matWorld = getWorldTransform();
		Vector3 dir( matWorld[8] , matWorld[9] , matWorld[10] );
		Vector3 orgPos = getWorldPosition();
		Vector3 upDir( matWorld[4] , matWorld[5] , matWorld[6] );

		float zy = mZFar / tan( 0.5f * mFov );
		float zx = zy * mAspect;
		Vector3 offsetX = upDir.cross( dir ) * zx;
		Vector3 offsetY = upDir * zy;
		Vector3 centerPos = orgPos +  mZFar * dir;

		Vector3 temp1 = offsetX + offsetY;
		Vector3 temp2 = offsetX - offsetY;
		Vector3 posRT = centerPos + temp1;
		Vector3 posLB = centerPos - temp1;
		Vector3 posRB = centerPos + temp2;
		Vector3 posLT = centerPos - temp2;

		mFrustumPlane[0] = Plane( -dir , centerPos ); //ZFar;
		mFrustumPlane[1] = Plane(  dir , orgPos + mZNear * dir ); //ZNear
		mFrustumPlane[2] = Plane( orgPos , posRT , posLT ); //top
		mFrustumPlane[3] = Plane( orgPos , posLB , posRB ); //bottom
		mFrustumPlane[4] = Plane( orgPos , posLT , posLB ); //left
		mFrustumPlane[5] = Plane( orgPos , posRB , posRT ); //right
	}

	void Camera::createFrustumMesh( Object* obj )
	{
		MeshBuilder builder( CFVT_XYZ );

		Vector3 dir = Vector3(0,0,1);
		Vector3 orgPos( 0,0,0 );
		Vector3 upDir = Vector3( 0,1,0 );

		float zy = mZFar / tan( 0.5f * mFov );
		float zx = zy * mAspect;
		Vector3 offsetX = upDir.cross( dir ) * zx;
		Vector3 offsetY = upDir * zy;
		Vector3 centerPos = orgPos +  mZFar * dir;

		builder.setPosition( centerPos - offsetX + offsetY );
		builder.addVertex();
		builder.setPosition( centerPos - offsetX - offsetY );
		builder.addVertex();
		builder.setPosition( centerPos + offsetX + offsetY );
		builder.addVertex();
		builder.setPosition( centerPos + offsetX - offsetY );
		builder.addVertex();

		zy = mZNear / tan( 0.5f * mFov );
		zx = zy * mAspect;
		offsetX = upDir.cross( dir ) * zx;
		offsetY = upDir * zy;
		centerPos = orgPos +  mZNear * dir;

		builder.setPosition( centerPos - offsetX + offsetY );
		builder.addVertex();
		builder.setPosition( centerPos - offsetX - offsetY );
		builder.addVertex();
		builder.setPosition( centerPos + offsetX + offsetY );
		builder.addVertex();
		builder.setPosition( centerPos + offsetX - offsetY );
		builder.addVertex();

		builder.addQuad( 0 , 2 , 6 , 4 );
		builder.addQuad( 3 , 1 , 5 , 7 );

		builder.createIndexTrangle( obj , nullptr );
	}

	bool Camera::doTestVisible( Matrix4 const& trans , BoundSphere const& sphere )
	{
		if ( checkFlag( NODE_FRUSTUM_PLANE_DIRTY ) )
		{
			updateFrustumPlane();
			removeFlag( NODE_FRUSTUM_PLANE_DIRTY );
		}

		Vector3 pos = sphere.getCenter() * trans;
		for( int i = 0 ; i < 6 ; ++i )
		{
			float dist = mFrustumPlane[i].calcDistance( pos );
			if ( dist < 0 && sphere.getRadius() < -dist )
				return false;
		}
		return true;
	}

	bool Camera::testVisible( Matrix4 const& trans , BoundSphere const& sphere )
	{
		if ( !gUseFrustumClip )
			return true;

		if ( mClipCam )
			return mClipCam->doTestVisible( trans , sphere );

		return doTestVisible( trans , sphere );
	}

	Plane const& Camera::getViewVolumePlane( int idx )
	{
		if ( checkFlag( NODE_FRUSTUM_PLANE_DIRTY ) )
		{
			updateFrustumPlane();
			removeFlag( NODE_FRUSTUM_PLANE_DIRTY );
		}
		return mFrustumPlane[idx];
	}

	void Camera::release()
	{
		getScene()->_destroyCamera( this );
	}

	void Camera::onModifyTransform( bool beLocal )
	{
		SceneNode::onModifyTransform( beLocal );
		addFlagBits( BIT( NODE_VIEW_MATRIX_DIRTY ) | BIT( NODE_FRUSTUM_PLANE_DIRTY ) );
	}

}//namespace CFly