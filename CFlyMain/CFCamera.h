#ifndef CFCamera_h__
#define CFCamera_h__


#include "CFBase.h"
#include "CFSceneNode.h"
#include "CFGeomUtility.h"

namespace CFly
{
	extern bool gUseFrustumClip;

	enum ProjectType
	{
		CFPT_PERSPECTIVE ,
		CFPT_ORTHOGONAL ,
	};

	//FIXME FIX RH to LH Problem In proj/view matrix

	class Camera : public SceneNode
	{
		typedef SceneNode BaseClass;
	public:
		enum
		{
			NODE_PROJ_MATRIX_DIRTY   = BaseClass::NEXT_NODE_FLAG ,
			NODE_VIEW_MATRIX_DIRTY   ,
			NODE_FRUSTUM_PLANE_DIRTY ,

			NEXT_NODE_FLAG ,
		};

		Camera( Scene* scene );

		void release();

		void setDefault();
		void setFov   ( float val ){ mFov = val; dirtyProjParams();  }
		void setNear  ( float val ){ mZNear = val; dirtyProjParams();  }
		void setFar   ( float val ){ mZFar = val; dirtyProjParams();  }
		void setAspect( float val ){ mAspect = val ; dirtyProjParams();  }
		void setProjectType( ProjectType type ){ mProjType = type;  dirtyProjParams(); }

		float getZFar()  const { return mZFar; }
		float getZNear() const { return mZNear; }

		Matrix4 const& getProjectionMartix() const;

		void updateProjectMatrix() const;

		Matrix4 const& getViewMatrix();

		void setScreenRange( float left , float right, float bottom , float top );

		Plane const& getViewVolumePlane( int idx );

		bool testVisible( Matrix4 const& trans , BoundSphere const& sphere );

		void createFrustumMesh( Object* obj );

		void setClipFrustum( Camera* cam ){ mClipCam = cam; }
		void setLookAt( Vector3 const& pos , Vector3 const& lookAt , Vector3 const& upDir );

	private:
		void dirtyProjParams(){ addFlagBits( BIT( NODE_PROJ_MATRIX_DIRTY ) | BIT( NODE_FRUSTUM_PLANE_DIRTY ) ); }
		void onModifyTransform( bool beLocal );

		bool doTestVisible( Matrix4 const& trans , BoundSphere const& sphere );

		void updateFrustumPlane();
		void calcOrthogonalMatrix( Matrix4& mat ) const;
		void calcPerspectiveMatrix( Matrix4& mat ) const;
		
		Camera* mClipCam;

		ProjectType   mProjType;
		mutable Matrix4 mMatProject;
		mutable Matrix4 mMatView;

		float mZFar;
		float mZNear;

		float mScreenRange[4];
		float mScaleFactor;

		float mFov;
		float mAspect;

		mutable bool  mFrustumPlaneDirty;
		mutable Plane mFrustumPlane[6];
	};

	DEFINE_ENTITY_TYPE( Camera , ET_CAMERA )


}//namespace CFly

#endif // CFCamera_h__