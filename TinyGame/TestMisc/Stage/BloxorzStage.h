#ifndef BloxorzStage_h__
#define BloxorzStage_h__

#include "StageBase.h"

#include "Math/TVector3.h"
#include "Math/TVector2.h"
#include "Core/IntegerType.h"

#include "WinGLPlatform.h"
#include "DataStructure/Grid2D.h"
#include "Tween.h"
#include "GameRenderSetup.h"
#include "RHI/MeshUtility.h"
#include "RHI/SimpleRenderState.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"

namespace Bloxorz
{
	using namespace Render;

	typedef Render::IntVector2 Vec2i;
	typedef Render::IntVector3 Vec3i;

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
			Vector3 ka;
			Vector3 kd;
			Vector3 ks;

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
		
		std::vector< Vector3 > vertices;
		std::vector< Vector3 > normals;
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

	struct GPU_ALIGN ObjectData
	{
		DECLARE_BUFFER_STRUCT(ObjectDataBlock, Objects);

		Matrix4 worldToLocal;
		Vector4 typeParams;
		int     Type;
		int     MatID;
		int     dummy[2];
	};
	struct GPU_ALIGN MaterialData
	{
		DECLARE_BUFFER_STRUCT(MaterialDataBlock, Materials);

		Color3f color;
		float   shininess;
	};

	struct GPU_ALIGN MapTileData
	{
		DECLARE_BUFFER_STRUCT(MapTileDataBlock, MapTiles);

		Vector4 posAndSize;
	};
	class TestStage : public StageBase
		            , public IGameRenderSetup
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

		bool onKey(KeyMsg const& msg);

		void drawObjectBody( Vector3 const& color );

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}
		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;
		virtual bool setupRenderSystem(ERenderSystem systemName) override;
		virtual void preShutdownRenderSystem(bool bReInit = false) override;
		
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
		
		Object   mObject;
		Vector3  mLookPos;
		Vector3  mCamRefPos;
		TGrid2D< int > mMap;
		bool   mIsGoal;
		bool   mbEditMode;

		bool bUseRayTrace = true;
		bool bUseSceneBuitin = true;
		bool bUseDeferredRending = false;
		bool bFreeView = true;
		bool bMoveCamera = true;

		bool canInput()
		{
			if (bFreeView)
			{
				return !bMoveCamera;
			}
			return true;
		}
		SimpleCamera mCamera;
		TransformStack mStack;
		
		int mNumMapTile;
		std::vector< ObjectData > mObjectList;

		class RayTraceProgram* mProgRayTrace;
		class RayTraceProgram* mProgRayTraceBuiltin;
		class RayTraceProgram* mProgRayTraceDeferred;
		class RayTraceLightingProgram* mProgRayTraceLighting;
		TStructuredBuffer< ObjectData >   mObjectBuffer;
		TStructuredBuffer< MaterialData > mMaterialBuffer;
		TStructuredBuffer< MapTileData >  mMapTileBuffer;
		
		RHITexture2DRef   mRenderBuffers[2];
		RHIFrameBufferRef mFrameBuffer;
		ViewInfo mView;
		Mesh     mCube;

		float    mRotateAngle;
		Vector3  mRotateAxis;
		Vector3  mRotatePos;
		float    mSpot;

		typedef std::vector< Object* > ObjectVec;
		ObjectVec mObjects;
		typedef Tween::GroupTweener< float > Tweener;
		Tweener   mTweener;

	};



}//namespace Bloxorz

#endif // BloxorzStage_h__
