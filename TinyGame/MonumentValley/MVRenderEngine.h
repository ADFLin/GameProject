#ifndef MVRenderEngine_h__
#define MVRenderEngine_h__

#include "MVCommon.h"
#include "MVLevel.h"
#include "MVPathFinder.h"
#include "MVSpaceControl.h"

#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/ShaderProgram.h"
#include "RHI/SimpleRenderState.h"
#include "RHI/RHICommand.h"

#include "Renderer/Mesh.h"

#include "Memory/FrameAllocator.h"

namespace MV
{
	using Render::SimpleRenderState;
	using Render::ViewInfo;
	using Render::RHICommandList;

	using RenderDepthStencilState = Render::TStaticDepthStencilState<true, Render::ECompareFunc::Less >;


#define USE_RENDER_CONTEXT 1

	struct RenderContext : public SimpleRenderState
	{
		template< class TShader>
		void setupShader(RHICommandList& commandList, TShader& shader)
		{		
			mView->setupShader(commandList, shader);
			SET_SHADER_PARAM(commandList, shader, LocalToWorld, stack.get());
		}

		template< class TShader>
		void setupPrimitiveParams(RHICommandList& commandList, TShader& shader)
		{
			SET_SHADER_PARAM(commandList, shader, LocalToWorld, stack.get());
		}

		void setupSimplePipeline(RHICommandList& commandList)
		{
			RHISetFixedShaderPipelineState(commandList, stack.get() * mView->worldToClip, mColor);
		}

		ViewInfo* mView;
	};
	class CRotator : public IRotator
	{
	public:
		CRotator()
		{
			rotateAngle = 0.0f;
		}
		void prevRender(RenderContext& context) override
		{
			if ( !isUse )
				return;

			Vec3f const& axis = FDir::OffsetF( mDir );

			Vec3f pos = Vec3f(mPos.x, mPos.y, mPos.z);
			context.stack.push();
			context.stack.translate(pos);
			context.stack.rotate(Quat::Rotate(axis, Math::DegToRad(rotateAngle)));
			context.stack.translate(-pos);

		}
		void postRender(RenderContext& context) override
		{
			if ( !isUse )
				return;
		
			context.stack.pop();
		}

		void updateValue( float factor ) override
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
		void prevRender(RenderContext& context) override
		{
			if ( !isUse )
				return;

			//Vec3f const& axis = FDir::OffsetF(mDir);

			//context.stack.push();
			//context.stack.translate(pos);
			//context.stack.rotate(Quat::Rotate(axis, Math::DegToRad(rotateAngle)));
			//context.stack.translate(-pos);
		}
		void postRender(RenderContext& context) override
		{
			if ( !isUse )
				return;
			
			context.stack.pop();
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


	class RenderEngine : public CModifyCreator
	{
	public:
		RenderEngine();
		bool init();

		void renderScene(RenderContext& context);

		void beginRender();

		void endRender();

		void renderBlock(RenderContext& context, Block& block , Vec3i const& pos );
		void renderPath(RenderContext& context, Path& path , PointVec& points );
		void renderMesh(RenderContext& context, int idMesh , Vec3f const& pos , Vec3f const& rotation );
		void renderMesh(RenderContext& context, int idMesh , Vec3f const& pos , AxisRoataion const& rotation );
		void renderGroup(RenderContext& context, ObjectGroup& group);
		void renderActor(RenderContext& context, Actor& actor);
		void renderNav(RenderContext& context, ObjectGroup &group);

		RenderParam mParam;

		Mat4 mViewToWorld;
		Render::RHICommandList* mCommandList;

		class BaseRenderProgram* mProgBaseRender;

		Render::Mesh mMesh[ NUM_MESH ];

		std::vector< float > mCacheBuffer;
		float* useCacheBuffer( int len )
		{
			if ( len > mCacheBuffer.size() )
				mCacheBuffer.resize( len );
			return &mCacheBuffer[0];
		}

		FrameAllocator mAllocator;

		ViewInfo mView;

	};

	RenderEngine& getRenderEngine();

}//namespace MV

#endif // MVRenderEngine_h__
