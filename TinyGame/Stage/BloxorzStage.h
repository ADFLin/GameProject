#ifndef BloxorzStage_h__
#define BloxorzStage_h__

#include "StageBase.h"

#include "TVector3.h"
#include "TVector2.h"
#include "Core/IntegerType.h"

#include "WinGLPlatform.h"
#include "DataStructure/Grid2D.h"
#include "Tween.h"

namespace Bloxorz
{

	typedef TVector2< int > Vec2i;
	typedef TVector2< float > Vector2;
	typedef TVector3< int > Vec3i;
	typedef TVector3< float > Vec3f;

	enum Dir
	{
		DIR_X  = 0 ,
		DIR_Y  = 1 ,
		DIR_NX = 2 ,
		DIR_NY = 3 ,
		DIR_NONE = 0xff,
	};
	inline bool isNegative( Dir dir ){ return dir >= DIR_NX; }
	inline Dir  inverseDir( Dir dir ){ return Dir( (dir + 2 ) % 4 );}

	class OBJFile
	{
	public:
		bool load( char const* path );

		struct MatInfo
		{
			std::string texName;
			Vec3f ka;
			Vec3f kd;
			Vec3f ks;

			float d;
			float s;
		};

		enum VertexFormat
		{
			eXYZ_N ,
			eXYZ  ,
			eXYZ_N_T2 ,
			eXYZ_T2 ,
		};

		struct GroupInfo
		{
			std::string  name;
			std::string  matName;
			int          faceVertexCount;
			VertexFormat format;
		};
		
		std::vector< Vec3f > vertices;
		std::vector< Vec3f > normals;
		std::vector< Vector2 > UVs;
		std::vector< int >   indices;
		std::vector< GroupInfo* > groups;
		std::vector< MatInfo > materials;
	};


	class Object
	{
	public:

		Object();
		Object( Object const& rhs );

		void addBody( Vec3i const& pos );
		void move( Dir dir );
		int  getMaxLocalPosX() const;
		int  getMaxLocalPosY() const;
		int  getMaxLocalPosZ() const;
		int  getBodyNum() const { return mNumBodies; }
		Vec3i const& getBodyLocalPos(int idx) const { return mBodiesPos[idx]; }

		Vec3i const& getPos() const { return mPos; }
		void         setPos( Vec3i const& val ) { mPos = val; }
		
		static int const MaxBodyNum = 10;


		Vec3i mPos;

		int   mNumBodies;
		Vec3i mBodiesPos[ MaxBodyNum ];

	};

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit();
		virtual void onEnd();
		virtual void onUpdate( long time );
		void onRender( float dFrame );

		void restart();
		void tick();
		
		void requestMove( Dir dir );
		void moveObject();

		void updateFrame( int frame );

		bool onMouse( MouseMsg const& msg );

		bool onKey( unsigned key , bool isDown );


		void drawObjectBody( Vec3f const& color );
		void drawCube();
		void drawCubeImpl();
	protected:
		enum State
		{
			eGoal,
			eFall,
			eMove,
			eBlock ,
		};
		State checkObjectState( Object const& object , uint8& holeDirMask );

		Dir    mMoveCur;
		Dir    mMoveStack;
		
		Object mObject;
		Vec3f  mLookPos;
		Vec3f  mCamRefPos;
		TGrid2D< int > mMap;
		bool   mIsGoal;
		bool   mbEditMode;

		float  mRotateAngle;
		Vec3f  mRotateAxis;
		Vec3f  mRotatePos;
		GLuint mCubeList;
		float  mSpot;

		typedef std::vector< Object* > ObjectVec;
		ObjectVec mObjects;
		typedef Tween::GroupTweener< float > Tweener;
		Tweener mTweener;

	};



}//namespace Bloxorz

#endif // BloxorzStage_h__
