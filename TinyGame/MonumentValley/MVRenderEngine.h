#ifndef MVRenderEngine_h__
#define MVRenderEngine_h__

#include "MVCommon.h"
#include "MVLevel.h"
#include "MVPathFinder.h"
#include "MVSpaceControl.h"

#include "GL/glew.h"
#include "GLConfig.h"

#include "RHI/DrawUtility.h"
#include "RHI/ShaderProgram.h"

namespace MV
{
	class CRotator : public IRotator
	{
	public:
		CRotator()
		{
			rotateAngle = 0.0f;
		}
		virtual void prevRender()
		{
			if ( !isUse )
				return;

			Vec3f const& axis = FDir::OffsetF( mDir );
			glPushMatrix();
			glTranslatef( mPos.x , mPos.y , mPos.z );
			glRotatef( rotateAngle , axis.x , axis.y , axis.z );
			glTranslatef( -mPos.x , -mPos.y , -mPos.z );
		}
		virtual void postRender()
		{
			if ( !isUse )
				return;
			glPopMatrix();
		}

		virtual void updateValue( float factor )
		{
			rotateAngle = 90 * factor;
		}
		float rotateAngle;
	};

	class CParallaxRotator : public IGroupModifier
	{
	public:
		CParallaxRotator()
		{
			rotateAngle = 0.0f;
		}
		virtual void prevRender()
		{
			if ( !isUse )
				return;

			//Vec3i const& axis = getOffset( mDir );
			//glPushMatrix();
			//glTranslatef( mPos.x , mPos.y , mPos.z );
			//glRotatef( rotateAngle , axis.x , axis.y , axis.z );
			//glTranslatef( -mPos.x , -mPos.y , -mPos.z );
		}
		virtual void postRender()
		{
			if ( !isUse )
				return;
			glPopMatrix();
		}

		virtual void updateValue( float factor , float crtlFactor )
		{
			rotateAngle = 90 * factor * crtlFactor;
		}



		Vec3i pos;
		float rotateAngle;
	};


	class CModifyCreator : public IModifyCreator
	{
	public:
		CRotator* createRotator() override
		{
			return new CRotator;
		}
		//virtual ITranslator* createTranslator(); override
		//{
		//	return new CTranslator;
		//}

	};

	enum MeshId
	{
		MESH_SPHERE ,
		MESH_BOX , 
		MESH_STAIR ,
		MESH_TRIANGLE ,
		MESH_ROTATOR_C ,
		MESH_ROTATOR_NC ,
		MESH_CORNER1 ,
		MESH_LADDER ,
		MESH_PLANE  ,

		NUM_MESH ,
	};

	struct RenderParam
	{
		RenderParam()
		{
			bShowNavNode = true;
			bShowGroupColor = true;
			bShowNavPath = true;
		}
		World* world;
		bool   bShowNavNode;
		bool   bShowNavPath;
		bool   bShowGroupColor;
	};

	struct View
	{
		Mat4 worldToClip;
		Mat4 worldToView;
	};

	class RenderEngine : public CModifyCreator
	{
	public:
		RenderEngine();
		bool init();

		void renderScene( Mat4 const& matView );

		void beginRender( Mat4 const& matView )
		{
			Mat4 matViewInv;
			float det;
			matView.inverse( matViewInv , det );
			glLoadMatrixf( matView );

			RHISetShaderProgram(*mCommandList, mEffect.getRHIResource());
			mEffect.setParam( *mCommandList, SHADER_PARAM(Global.matViewInv) , matViewInv );
		}

		void endRender()
		{
			RHISetShaderProgram(*mCommandList, nullptr);
		}

		void renderBlock(Block& block , Vec3i const& pos );
		void renderPath( Path& path , PointVec& points );
		void renderMesh(int idMesh , Vec3f const& pos , Vec3f const& rotation );
		void renderMesh(int idMesh , Vec3f const& pos , AxisRoataion const& rotation );
		void renderGroup(ObjectGroup& group);
		void renderActor(Actor& actor);
		void renderNav(ObjectGroup &group);

		RenderParam mParam;

		Mat4 mViewToWorld;
		Render::RHICommandList* mCommandList;
		Render::ShaderProgram   mEffect;
		Render::ShaderParameter paramDirX;
		Render::ShaderParameter paramDirZ;
		Render::ShaderParameter paramLocalScale;

		Render::Mesh mMesh[ NUM_MESH ];

		std::vector< float > mCacheBuffer;
		float* useCacheBuffer( int len )
		{
			if ( len > mCacheBuffer.size() )
				mCacheBuffer.resize( len );
			return &mCacheBuffer[0];
		}

		void pushMatrix() { mWorldXFormStack.push_back(mWorldXFormStack.back()); }
		void popMatrix()  { mWorldXFormStack.pop_back(); }
		void loadMatrix(Mat4 const& m) { mWorldXFormStack.back() = m; }
		void multiMatrix(Mat4 const& m){ mWorldXFormStack.back() = m * mWorldXFormStack.back(); }

		std::vector< Mat4 > mWorldXFormStack;


	};

	RenderEngine& getRenderEngine();

}//namespace MV

#endif // MVRenderEngine_h__
