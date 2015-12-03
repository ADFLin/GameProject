#ifndef CFSkeleton_h__
#define CFSkeleton_h__

#include "CFBase.h"
#include "CFSceneNode.h"
#include "CFAnimatable.h"

#define CF_BASE_BONE_NAME "__Base__"

namespace CFly
{
	class Skeleton;

	class   IBoneController;
	typedef uint8 BoneId;

	struct AnimSequence
	{
		int idxStart;
		int idxEnd;
	};

	class BoneNode : public SubNode
	{
	public:
		BoneNode();

		BoneId          id;
		BoneId          parentId;
		Matrix4         invLocalTrans;


		char const*      getName() const { return mName; }

		
		MotionKeyFrame*  keyFrames;
		KeyFrameFormat   mKeyFormat;
		
		void calcFrameTransform( Matrix4& result , int frame );
		void calcFrameTransform( Matrix4& result , int frame0 , int frame1 , float t );
		void calcFrameTransform( Matrix4& result , float* frames , float* weights , int num );

		void generateBoneLine( std::vector< Vector3 >& lineVec , Matrix4 const& trans , int frame );
		void interpolationMotion( Vector3& pos , Quaternion& rotate , int frame0 , float t );

	private:
		BoneNode( BoneNode const& rhs );
		char const*  mName;
		friend class Skeleton;
	};


	class IBoneController
	{
	public:
		enum Type
		{
			BCT_X ,
			BCT_Y ,
			BCT_Z ,
			BCT_RX ,
			BCT_RY ,
			BCT_RZ ,
		};

		virtual void install( Skeleton& sk ){}
		virtual void uninstall( Skeleton& sk ){}
		virtual void setParamValue( int idxParam , float* value ) = 0;
	};

	class Skeleton : public RefObjectT< Skeleton >
	{
	public:

		Skeleton();

		~Skeleton();

		Skeleton* clone();

		void      useInvLocalTransform( bool beUse ){ mUseInvLocalTrans = beUse; }

		BoneNode* getBone( char const* name );
		BoneNode* getBone( int id ){  return mBoneVec[id];  }

		BoneNode* createBone( char const* name , char const* parentName );
		BoneNode* createBone( char const* name , BoneId parentId );

		void      createMotionData( int numFrame );
		int       getTotalPoseFrame(){ return mTotalKeyFrame; }
		int       getBoneNum(){ return mBoneVec.size(); }

		IBoneController* getController(){ return mController; }
		void             setController( IBoneController* manager );

		void      setupBoneWorldTransform( RenderSystem* renderSys , Matrix4 const& baseWorld , Matrix4* boneTrans );
		void      generateInvLoacalTransform();
		void      generateBoneLine( std::vector< Vector3 >& lineVec , int frame );
		void      generateBoneTransform( Matrix4* matVec , Matrix4 const& baseTrans , int frame , float fract );
		void      generateBoneTransform( Matrix4* matVec , Matrix4 const& baseTrans , float* frames , float* weights , int num );



	private:

		Skeleton( Skeleton const& rhs );

		typedef std::map< String , short > BoneMap;
		BoneMap    mNameMap;
		bool       mUseInvLocalTrans;
		int        mTotalKeyFrame;
		BoneNode*  mBaseBone;
		IBoneController*           mController;
		std::vector< BoneNode* >   mBoneVec;
	};






}//namespace CFly

#endif // CFSkeleton_h__
