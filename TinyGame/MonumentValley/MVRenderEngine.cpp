#include "MVRenderEngine.h"


#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"

#include "Renderer/MeshBuild.h"

#include "InlineString.h"

#include <fstream>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE( ar ) ( sizeof(ar)/sizeof(ar[0]) )
#endif

namespace MV
{
	using namespace Render;

	struct MeshInfo
	{
		MeshId id;
		char const* name;
	};

	MeshInfo GMeshInfo[] = 
	{
		{ MESH_STAIR  , "stair" } ,
		{ MESH_TRIANGLE , "tri" } ,
		{ MESH_ROTATOR_C , "rotator" } ,
		{ MESH_ROTATOR_NC , "rotatorF" } ,
		{ MESH_CORNER1 , "corner1" }  ,
		{ MESH_LADDER , "ladder" }  ,
	};

	Vec3f const GGroupColor[] =
	{
		{ 1 , 1 , 1 },
		{ 1 , 0.5 , 0.5 },
		{ 0.5 , 1 , 0.5 },
		{ 0.5 , 0.5 , 1 },
		{ 1 , 1 , 0.5 },
		{ 1 , 0.5 , 1 },
		{ 0.5 , 1 , 1 },
	};
	static RenderEngine* gRE = nullptr;


	Matrix4 ToMatrix(AxisRoataion const& rotation)
	{
		Vec3f xAxis = FDir::OffsetF(rotation[0]);
		Vec3f yAxis = FDir::OffsetF(rotation[1]);
		Vec3f zAxis = FDir::OffsetF(rotation[2]);
		return Matrix4::Basis(xAxis, yAxis, zAxis);
	}
	Matrix4 ToMatrix(AxisRoataion const& rotation , Vec3f scale)
	{
		Vec3f xAxis = scale.x * FDir::OffsetF(rotation[0]);
		Vec3f yAxis = scale.y * FDir::OffsetF(rotation[1]);
		Vec3f zAxis = scale.z * FDir::OffsetF(rotation[2]);
		return Matrix4::Basis(xAxis, yAxis, zAxis);
	}

	RenderEngine& getRenderEngine(){ return *gRE; }

	RenderEngine::RenderEngine()
		:mAllocator(4096)
	{
		gRE = this;
	}


	class BaseRenderProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(BaseRenderProgram, Global)
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/MVRender";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(BaseRenderVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BaseRenderPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Rotation);
			BIND_SHADER_PARAM(parameterMap, BaseColor);
			BIND_SHADER_PARAM(parameterMap, LocalScale);
			BIND_SHADER_PARAM(parameterMap, LocalToWorld);

		}

		DEFINE_SHADER_PARAM(Rotation);
		DEFINE_SHADER_PARAM(BaseColor);
		DEFINE_SHADER_PARAM(LocalScale);
		DEFINE_SHADER_PARAM(LocalToWorld);
	};

	IMPLEMENT_SHADER_PROGRAM(BaseRenderProgram);

	bool RenderEngine::init()
	{

		VERIFY_RETURN_FALSE( mProgBaseRender = ShaderManager::Get().getGlobalShaderT< BaseRenderProgram >() );

		mCommandList = &RHICommandList::GetImmediateList();

		{
			if ( !FMeshBuild::Cube( mMesh[ MESH_BOX ] , 0.5f ) ||
				 !FMeshBuild::UVSphere( mMesh[ MESH_SPHERE ] , 0.3 , 10 , 10 ) ||
				 !FMeshBuild::Plane( mMesh[ MESH_PLANE ] , Vector3(0.5,0,0) ,Vector3(1,0,0) , Vector3(0,1,0) , Vector2( 0.5 , 0.5) , 1 ) )
				return false;

			for( int i = 0 ; i < ARRAY_SIZE( GMeshInfo ) ; ++i )
			{
				MeshInfo const& info = GMeshInfo[i];
				InlineString< 256 > path = "Mesh/";
				path += info.name; 
				path += ".obj";
				if ( !FMeshBuild::LoadObjectFile( mMesh[ info.id ] , path ) )
					return false;
			}
		}

		return true;
	}

	void RenderEngine::renderScene(RenderContext& context)
	{
		RHICommandList& commandList = *mCommandList;
		beginRender();
		renderGroup(context, mParam.world->mRootGroup );
		endRender();
	}

	void RenderEngine::beginRender()
	{
		RHISetShaderProgram(*mCommandList, mProgBaseRender->getRHI());
		RHISetRasterizerState(*mCommandList, TStaticRasterizerState<>::GetRHI());
		RHISetDepthStencilState(*mCommandList, RenderDepthStencilState::GetRHI());
	}

	void RenderEngine::endRender()
	{
		RHISetShaderProgram(*mCommandList, nullptr);
	}

	void RenderEngine::renderBlock(RenderContext& context, Block& block, Vec3i const& pos)
	{
		RHICommandList& commandList = *mCommandList;

		context.stack.push();
		context.stack.translate( Vector3( pos.x , pos.y , pos.z ) );
		if (mParam.bShowGroupColor)
		{
			int idx = (block.group == &mParam.world->mRootGroup) ? 0 : (block.group->idx);
			SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, GGroupColor[idx % ARRAY_SIZE(GGroupColor)]);
		}
		else
		{
			SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, Vec3f(1,1,1));
		}
		
		SET_SHADER_PARAM(commandList, *mProgBaseRender, Rotation, Vec2i((int)block.rotation[0], (int)block.rotation[2]));
		context.setupShader(commandList, *mProgBaseRender);
		mMesh[ block.idMesh ].draw(commandList);

		for( int i = 0 ; i < 6 ; ++i )
		{
			BlockSurface& surface =  block.getLocalFace( Dir( i ) );
			if ( surface.func == NFT_LADDER )
			{
				Vec3f offset = 0.5 * FDir::OffsetF( block.rotation.toWorld( Dir(i) ) );
				context.stack.translate(offset);
				context.setupPrimitiveParams(commandList, *mProgBaseRender);
				mMesh[ MESH_LADDER ].draw(commandList);
			}
		}
		context.stack.pop();
	}

	void RenderEngine::renderPath(RenderContext& context, Path& path , PointVec& points)
	{
		return;

		RHICommandList& commandList = *mCommandList;
		if ( !points.empty() )
			TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::LineStrip , &points[0] , points.size() , sizeof( Vec3f ) );
	}

	void RenderEngine::renderMesh(RenderContext& context, int idMesh , Vec3f const& pos , Vec3f const& rotation)
	{
		RHICommandList& commandList = *mCommandList;
		context.stack.push();
		context.stack.translate(pos);

		SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, Vec3f(1, 1, 1));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, Rotation, Vec2i((int)Dir::X, (int)Dir::Z));
		context.setupShader(commandList, *mProgBaseRender);
		Quat q; q.setEulerZYX( rotation.z , rotation.y , rotation.x );

		context.stack.rotate(q);
		mMesh[ idMesh ].draw(commandList);


		context.stack.pop();
		
	}

	void RenderEngine::renderMesh(RenderContext& context, int idMesh , Vec3f const& pos , AxisRoataion const& rotation)
	{
		RHICommandList& commandList = *mCommandList;
		context.stack.push();
		context.stack.translate(pos);
	
		SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, Vec3f(1, 1, 1));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, Rotation, Vec2i((int)rotation[0], (int)rotation[2]));
		context.setupShader(commandList, *mProgBaseRender);
		mMesh[ idMesh ].draw(commandList);

		context.stack.pop();

	}

	void RenderEngine::renderGroup(RenderContext& context, ObjectGroup& group)
	{
		if ( group.node )
			group.node->prevRender(context);

		for(Block* block  : group.blocks)
		{
			renderBlock(context, *block , block->pos );
		}

		if ( mParam.bShowNavNode )
		{
			renderNav(context, group );
		}

		for(Actor* actor : group.actors)
		{
			renderActor(context, *actor );
		}

		for(MeshObject* mesh : group.meshs)
		{
			renderMesh(context, mesh->idMesh , mesh->pos , mesh->rotation );
		}

		if ( group.node )
		{
			if ( !group.node->bModifyChildren )
				group.node->postRender(context);

			for(ObjectGroup* child : group.children)
			{
				renderGroup(context, *child );
			}

			if ( group.node->bModifyChildren )
				group.node->postRender(context);
		}
		else
		{
			for(ObjectGroup* child : group.children)
			{
				renderGroup(context, *child );
			}

		}
	}


	struct Vertex_XYZ_C
	{
		Vec3f pos;
		Vec3f color;
	};

	void RenderEngine::renderActor(RenderContext& context, Actor& actor)
	{
		RHICommandList& commandList = *mCommandList;


		Vec3f pos = actor.renderPos + 0.5 * FDir::OffsetF( actor.actFaceDir );

		Vec3f frontOffset = FDir::OffsetF( actor.rotation[0] );
		Vec3f upOffset = FDir::OffsetF( actor.rotation[2] );

		switch( actor.actState )
		{
		case Actor::eActClimb: pos += 0.5 * upOffset; break;
		}
		context.stack.push();
		context.stack.translate(pos);

		SET_SHADER_PARAM( commandList , *mProgBaseRender, Rotation , Vec2i((int)actor.rotation[0], (int)actor.rotation[2]));
		context.setupShader(commandList, *mProgBaseRender);
		SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, Vec3f(0.5, 0.5, 0.5));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, LocalScale, Vec3f(0.4, 0.6, 1.0));
		mMesh[ MESH_BOX ].draw(commandList);
		SET_SHADER_PARAM(commandList, *mProgBaseRender, LocalScale, Vec3f(1.0, 1.0, 1.0));

		//Vector3 offset = actor.moveOffset * cast( frontOffset ) - 0.2 * cast( upOffset );
		context.stack.translate(0.9 * upOffset);
		context.setupPrimitiveParams(commandList, *mProgBaseRender);
		mMesh[ MESH_SPHERE ].draw(*mCommandList);



		Vertex_XYZ_C vertices[]
		{
			{ Vec3f(0,0,0) , Vec3f(1,0,0) },{ frontOffset, Vec3f(1,0,0) },
			{ Vec3f(0,0,0) , Vec3f(0,0,1) },{ upOffset, Vec3f(0,0,1) },
		};

		RHISetShaderProgram(commandList, nullptr);
		context.setupSimplePipeline(commandList);
		TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::LineList, vertices, ARRAY_SIZE(vertices));
		RHISetShaderProgram(commandList, mProgBaseRender->getRHI());

		context.stack.pop();
		
	}

	void RenderEngine::renderNav(RenderContext& context, ObjectGroup &group)
	{
		RHICommandList& commandList = *mCommandList;

#if 1
		StackMaker marker(mAllocator);
		float* buffer = (float*)mAllocator.alloc(sizeof(float) * group.getBlockNum() * 6 * 4 * 2 * 2 * 6);
#else
		float* buffer = useCacheBuffer(group.getBlockNum() * 6 * 4 * 2 * 2 * 6);
#endif

		float* v = buffer;
		int nV = 0;
		for(Block* block : group.blocks)
		{
			for( int i = 0 ; i < 6 ; ++i )
			{
				Dir faceDirL = Dir(i);
				Dir faceDir =block->rotation.toWorld(faceDirL);

				BlockSurface& surface = block->getLocalFace( faceDirL );

				Vec3f p1 = Vec3f( block->pos ) + 0.55 * FDir::OffsetF( faceDir );

				for( int idx = 0 ; idx < FACE_NAV_LINK_NUM ; ++idx )
				{
					NavNode& node = surface.nodes[ NODE_DIRECT ][idx];
					if ( !node.link )
						continue;
					Dir linkDir = block->rotation.toWorld( FDir::Neighbor( faceDirL , idx ) );
					Vec3f p2 = p1 + 0.5 * FDir::OffsetF( linkDir );

					v[0]=p1.x;v[1]=p1.y;v[2]=p1.z;v[3]=1;v[4]=1;v[5]=0; v += 6;
					v[0]=p2.x;v[1]=p2.y;v[2]=p2.z;v[3]=1;v[4]=1;v[5]=0; v += 6;
					nV += 2;
				}

				p1 += 0.02 * FDir::OffsetF( faceDir );
				for( int idx = 0 ; idx < FACE_NAV_LINK_NUM ; ++idx )
				{
					NavNode& node = surface.nodes[ NODE_PARALLAX ][idx];
					if ( !node.link )
						continue;
					Dir linkDir = block->rotation.toWorld( FDir::Neighbor( faceDirL , idx ) );
					Vec3f p2 = p1 + 0.5 * FDir::OffsetF( linkDir );

					v[0]=p1.x;v[1]=p1.y;v[2]=p1.z;v[3]=1;v[4]=0;v[5]=1; v += 6;
					v[0]=p2.x;v[1]=p2.y;v[2]=p2.z;v[3]=1;v[4]=0;v[5]=1; v += 6;
					nV += 2;
				}
			}
		}

		context.setupSimplePipeline(commandList);
		TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::LineList, buffer , nV  );
		RHISetShaderProgram(commandList, mProgBaseRender->getRHI());
	}

}//namespace MV