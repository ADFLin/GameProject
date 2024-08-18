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

	struct RenderParam
	{
		RenderParam()
		{
			bShowNavNode = true;
			bShowGroupColor = true;
			bShowNavPath = true;
			bDrawAxis = true;
		}

		bool   bShowNavNode;
		bool   bShowNavPath;
		bool   bShowGroupColor;
		bool   bDrawAxis;
	};


	struct RenderContext : public SimpleRenderState
	{
		template< class TShader>
		void setupShader(TShader& shader)
		{		
			RHICommandList& commandList = getCommandList();
			view->setupShader(commandList, shader);
			SET_SHADER_PARAM(commandList, shader, LocalToWorld, stack.get());
			SET_SHADER_PARAM(commandList, shader, LightDir, Vector3(0.4, 0.5, 0.8));
		}

		template< class TShader>
		void setupPrimitiveParams(TShader& shader)
		{
			RHICommandList& commandList = getCommandList();
			SET_SHADER_PARAM(commandList, shader, LocalToWorld, stack.get());
		}

		void setSimpleShader()
		{
			using namespace Render;
			RHICommandList& commandList = getCommandList();
			RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(stack.get() * view->worldToClip), mColor);
		}

		void setSimpleShader(float depthBias)
		{
			RHICommandList& commandList = getCommandList();
			using namespace Render;
			Matrix4 depthOffset =
			{
				1, 0 ,0 ,0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, depthBias, 1,
			};
			RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(stack.get() * view->worldToView * depthOffset * view->viewToClip), mColor);
		}

		RHICommandList& getCommandList() { return *mCommandList; }

		RHICommandList* mCommandList;
		RenderParam param;
		World*      world = nullptr;
		ViewInfo*   view = nullptr;
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
			if ( !bUsing )
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
			if ( !bUsing )
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
			if ( !bUsing )
				return;

			//Vec3f const& axis = FDir::OffsetF(mDir);

			//context.stack.push();
			//context.stack.translate(pos);
			//context.stack.rotate(Quat::Rotate(axis, Math::DegToRad(rotateAngle)));
			//context.stack.translate(-pos);
		}
		void postRender(RenderContext& context) override
		{
			if ( !bUsing )
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



	class RenderEngine : public CModifyCreator
	{
	public:
		RenderEngine();
		bool init();

		void renderScene(RenderContext& context);

		void beginRender(RenderContext& context);
		void endRender(RenderContext& context);

		void renderBlock(RenderContext& context, Block& block , Vec3i const& pos );
		void renderPath(RenderContext& context, Path& path , PointVec& points );
		void renderMesh(RenderContext& context, int idMesh , Vec3f const& pos , Vec3f const& rotation );
		void renderMesh(RenderContext& context, int idMesh , Vec3f const& pos , AxisRoataion const& rotation );
		void renderGroup(RenderContext& context, ObjectGroup& group);
		void renderActor(RenderContext& context, Actor& actor);
		void renderNav(RenderContext& context, ObjectGroup &group);


		Render::RHICommandList* mCommandList;

		class BaseRenderProgram* mProgBaseRender;

		Render::Mesh mMesh[ NUM_MESH ];

		FrameAllocator mAllocator;
	};

	RenderEngine& getRenderEngine();

}//namespace MV

#endif // MVRenderEngine_h__
