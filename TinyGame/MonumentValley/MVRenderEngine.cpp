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
			BIND_SHADER_PARAM(parameterMap, LightDir);
		}

		DEFINE_SHADER_PARAM(Rotation);
		DEFINE_SHADER_PARAM(BaseColor);
		DEFINE_SHADER_PARAM(LocalScale);
		DEFINE_SHADER_PARAM(LocalToWorld);
		DEFINE_SHADER_PARAM(LightDir);
	};

	IMPLEMENT_SHADER_PROGRAM(BaseRenderProgram);

	bool RenderEngine::init()
	{

		VERIFY_RETURN_FALSE( mProgBaseRender = ShaderManager::Get().getGlobalShaderT< BaseRenderProgram >() );

		{
			VERIFY_RETURN_FALSE(FMeshBuild::Cube(mMesh[MESH_BOX], 0.5f));
			VERIFY_RETURN_FALSE(FMeshBuild::UVSphere(mMesh[MESH_SPHERE], 1, 20, 20));
			VERIFY_RETURN_FALSE(FMeshBuild::Plane(mMesh[MESH_PLANE], Vector3(0.5, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0), Vector2(0.5, 0.5), 1));

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
		RHICommandList& commandList = context.getCommandList();
		beginRender(context);

		if (context.param.bDrawAxis)
		{
			context.setColor(LinearColor(1, 1, 1));
			context.setSimpleShader();
			DrawUtility::AixsLine(commandList, 10);
			RHISetShaderProgram(commandList, mProgBaseRender->getRHI());
		}

		renderGroup(context, context.world->mRootGroup );
		endRender(context);
	}

	void RenderEngine::beginRender(RenderContext& context)
	{
		RHICommandList& commandList = context.getCommandList();
		RHISetShaderProgram(commandList, mProgBaseRender->getRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
	}

	void RenderEngine::endRender(RenderContext& context)
	{
		RHICommandList& commandList = context.getCommandList();
		RHISetShaderProgram(commandList, nullptr);
	}

	void RenderEngine::renderBlock(RenderContext& context, Block& block, Vec3i const& pos)
	{
		RHICommandList& commandList = context.getCommandList();

		context.stack.push();
		context.stack.translate( Vector3( pos.x , pos.y , pos.z ) );

		if (context.param.bShowGroupColor)
		{
			int idx = (block.group == &context.world->mRootGroup) ? 0 : (block.group->idx);
			SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, GGroupColor[idx % ARRAY_SIZE(GGroupColor)]);
		}
		else
		{
			SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, Vec3f(1,1,1));
		}
		SET_SHADER_PARAM(commandList, *mProgBaseRender, LocalScale, Vec3f(1.0, 1.0, 1.0));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, Rotation, Vec2i((int)block.rotation[0], (int)block.rotation[2]));
		context.setupShader(*mProgBaseRender);

		mMesh[ block.idMesh ].draw(commandList);

		for( int i = 0 ; i < 6 ; ++i )
		{
			BlockSurface& surface =  block.getLocalFace( Dir( i ) );
			if ( surface.func == NFT_LADDER )
			{
				Vec3f offset = 0.5 * FDir::OffsetF( block.rotation.toWorld( Dir(i) ) );
				context.stack.translate(offset);
				context.setupPrimitiveParams(*mProgBaseRender);
				mMesh[ MESH_LADDER ].draw(commandList);
			}
		}
		context.stack.pop();
	}

	void RenderEngine::renderMesh(RenderContext& context, int idMesh , Vec3f const& pos , Vec3f const& rotation)
	{
		RHICommandList& commandList = context.getCommandList();
		context.stack.push();
		context.stack.translate(pos);

		SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, Vec3f(1, 1, 1));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, Rotation, Vec2i((int)Dir::X, (int)Dir::Z));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, LocalScale, Vec3f(1.0, 1.0, 1.0));
		context.setupShader(*mProgBaseRender);
		Quat q; q.setEulerZYX( rotation.z , rotation.y , rotation.x );

		context.stack.rotate(q);
		mMesh[ idMesh ].draw(commandList);


		context.stack.pop();
		
	}

	void RenderEngine::renderMesh(RenderContext& context, int idMesh , Vec3f const& pos , AxisRoataion const& rotation)
	{
		RHICommandList& commandList = context.getCommandList();
		context.stack.push();
		context.stack.translate(pos);
	
		SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, Vec3f(1, 1, 1));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, Rotation, Vec2i((int)rotation[0], (int)rotation[2]));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, LocalScale, Vec3f(1.0, 1.0, 1.0));
		context.setupShader(*mProgBaseRender);
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

		if (context.param.bShowNavNode)
		{
			renderNav(context, group);
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
		RHICommandList& commandList = context.getCommandList();


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
		context.setupShader(*mProgBaseRender);
		SET_SHADER_PARAM(commandList, *mProgBaseRender, BaseColor, Vec3f(0.5, 0.5, 0.5));
		SET_SHADER_PARAM(commandList, *mProgBaseRender, LocalScale, Vec3f(0.4, 0.6, 1.0));
		mMesh[ MESH_BOX ].draw(commandList);
		//Vector3 offset = actor.moveOffset * cast( frontOffset ) - 0.2 * cast( upOffset );
	
		context.stack.translate(0.9 * upOffset);
		context.setupPrimitiveParams(*mProgBaseRender);
		SET_SHADER_PARAM(commandList, *mProgBaseRender, LocalScale, Vec3f(0.3, 0.3, 0.3));
		mMesh[ MESH_SPHERE ].draw(commandList);
		
		Vertex_XYZ_C vertices[]
		{
			{ Vec3f(0,0,0) , Vec3f(1,0,0) },{ frontOffset, Vec3f(1,0,0) },
			{ Vec3f(0,0,0) , Vec3f(0,0,1) },{ upOffset, Vec3f(0,0,1) },
		};

		RHISetShaderProgram(commandList, nullptr);
		context.setSimpleShader();
		TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::LineList, vertices, ARRAY_SIZE(vertices));
		RHISetShaderProgram(commandList, mProgBaseRender->getRHI());

		context.stack.pop();
		
	}

	void RenderEngine::renderNav(RenderContext& context, ObjectGroup &group)
	{
		RHICommandList& commandList = context.getCommandList();

		{
			StackMaker marker(mAllocator);

			Vertex_XYZ_C* buffer = (Vertex_XYZ_C*)mAllocator.alloc(sizeof(Vertex_XYZ_C) * group.getBlockNum() * Dir::COUNT * FACE_NAV_LINK_NUM * NUM_NODE_TYPE * 2);

			Vertex_XYZ_C* v = buffer;
			for (Block* block : group.blocks)
			{
				for (int i = 0; i < Dir::COUNT; ++i)
				{
					Dir faceDirL = Dir(i);
					Dir faceDir = block->rotation.toWorld(faceDirL);

					BlockSurface& surface = block->getLocalFace(faceDirL);

					float surfaceOffset = 0.55;
					Vec3f p1 = Vec3f(block->pos) + surfaceOffset * FDir::OffsetF(faceDir);

					for (int idx = 0; idx < FACE_NAV_LINK_NUM; ++idx)
					{
						NavNode& node = surface.nodes[NODE_DIRECT][idx];
						if (!node.link)
							continue;

						Dir linkDir = block->rotation.toWorld(FDir::Neighbor(faceDirL, idx));
						BlockSurface* linkSurf = node.link->getSurface();
						Dir linkSurfDir = Block::WorldDir(*linkSurf);

						Vec3f p2;
						if (linkDir == linkSurfDir)
						{
							p2 = p1 + surfaceOffset * FDir::OffsetF(linkDir);
						}
						else if (linkDir == FDir::Inverse(linkSurfDir))
						{
							p2 = p1 + ( 1 - surfaceOffset ) * FDir::OffsetF(linkDir);
						}
						else
						{
							p2 = p1 + 0.5 * FDir::OffsetF(linkDir);
						}

						v[0] = { p1 , Vec3f(1,1,0) };
						v[1] = { p2 , Vec3f(1,1,0) };
						v += 2;
					}

					surfaceOffset += 0.02;
					p1 += 0.02 * FDir::OffsetF(faceDir);
					for (int idx = 0; idx < FACE_NAV_LINK_NUM; ++idx)
					{
						NavNode& node = surface.nodes[NODE_PARALLAX][idx];
						if (!node.link)
							continue;

						Dir linkDir = block->rotation.toWorld(FDir::Neighbor(faceDirL, idx));
						BlockSurface* linkSurf = node.link->getSurface();
						Dir linkSurfDir = Block::WorldDir(*linkSurf);

						Vec3f p2;
						if (linkDir == linkSurfDir)
						{
							p2 = p1 + surfaceOffset * FDir::OffsetF(linkDir);
						}
						else if (linkDir == FDir::Inverse(linkSurfDir))
						{
							p2 = p1 + (1 - surfaceOffset) * FDir::OffsetF(linkDir);
						}
						else
						{
							p2 = p1 + 0.5 * FDir::OffsetF(linkDir);
						}

						v[0] = { p1 , Vec3f(1,0,1) };
						v[1] = { p2 , Vec3f(1,0,1) };
						v += 2;
					}
				}
			}

			context.setColor(LinearColor(1, 1, 1));
			context.setSimpleShader();
			TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::LineList, buffer, v - buffer);
		}

		if (context.view->isPerspectiveProjection())
		{
			StackMaker marker(mAllocator);

			Vertex_XYZ_C* buffer = (Vertex_XYZ_C*)mAllocator.alloc(sizeof(Vertex_XYZ_C) * group.getBlockNum() * Dir::COUNT * FACE_NAV_LINK_NUM * 2);

			Vertex_XYZ_C* v = buffer;
			for (Block* block : group.blocks)
			{
				for (int i = 0; i < Dir::COUNT; ++i)
				{
					Dir faceDirL = Dir(i);
					Dir faceDir = block->rotation.toWorld(faceDirL);

					BlockSurface& surface = block->getLocalFace(faceDirL);

					float surfaceOffset = 0.57;
					Vec3f p1 = Vec3f(block->pos) + surfaceOffset * FDir::OffsetF(faceDir);
					for (int idx = 0; idx < FACE_NAV_LINK_NUM; ++idx)
					{
						NavNode& node = surface.nodes[NODE_PARALLAX][idx];
						if (!node.link || &node < node.link)
							continue;

						Dir linkDir = block->rotation.toWorld(FDir::Neighbor(faceDirL, idx));
						Vec3f p2 = p1 + 0.5 * FDir::OffsetF(linkDir);

						BlockSurface* linkSurf = node.link->getSurface();
						Block* linkBlock = linkSurf->getBlock();
						Vec3f linkPos = Vec3f(linkBlock->pos) + surfaceOffset * (FDir::OffsetF(Block::WorldDir(*linkSurf))) + 0.5 * FDir::OffsetF(FDir::Inverse(linkDir));

						v[0] = { p2 , Vec3f(0,1,1) };
						v[1] = { linkPos , Vec3f(0,1,1) };
						v += 2;
					}
				}
			}

			context.setColor(LinearColor(1, 1, 1));
			context.setSimpleShader();
			TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::LineList, buffer, v - buffer);
		}

		for (auto node : context.world->mFixNodes)
		{
			BlockSurface* sufrace = node->getSurface();
			Block* block = sufrace->getBlock();
			
			float surfaceOffset = 0.5;
			Dir faceDir = Block::WorldDir(*node->getSurface());
			Vec3f p1 = Vec3f(block->pos) + surfaceOffset * FDir::OffsetF(faceDir);
			Dir linkDir = block->rotation.toWorld(FDir::Neighbor(Block::LocalDir(*sufrace) , node->idxDir));
			Vec3f p2 = p1 + 0.5 * FDir::OffsetF(linkDir);

			context.stack.push();
			context.stack.translate(p2);
			context.stack.scale(Vec3f(0.1, 0.1, 0.1));
			context.setColor(Color3f(0,0,1));
			context.setSimpleShader();
			mMesh[MESH_BOX].draw(commandList);
			context.stack.pop();

		}

		RHISetShaderProgram(commandList, mProgBaseRender->getRHI());


	}

	void RenderEngine::renderPath(RenderContext& context, Path& path , PointVec& points)
	{
		RHICommandList& commandList = context.getCommandList();
		context.setColor(LinearColor(0.2, 0.8, 0.8));

		int indexPoint = 0;
		int indexNode = 0;
		int numPointNode = 0;
		if (indexNode < path.getNodeNum())
		{
			numPointNode = path.getNode(indexNode).posCount;
		}

		for (auto const& point : points)
		{
			context.stack.push();
			context.stack.translate(point);
			context.stack.scale(Vec3f(0.1, 0.1, 0.1));
			context.setColor(GGroupColor[indexNode % ARRAY_SIZE(GGroupColor)]);
			context.setSimpleShader();
			mMesh[MESH_SPHERE].draw(commandList);
			context.stack.pop();

			++indexPoint;
			if (indexPoint == numPointNode)
			{
				auto& node = path.getNode(indexNode);

				indexPoint = 0;
				++indexNode;
				if (indexNode < path.getNodeNum())
				{
					numPointNode = path.getNode(indexNode).posCount;
					if (node.parallaxDir)
					{
						++numPointNode;
					}
				}
			}
		}

		context.setSimpleShader(-0.01);
		if ( !points.empty() )
			TRenderRT< RTVF_XYZ >::Draw(commandList, EPrimitive::LineStrip , &points[0] , points.size() , sizeof( Vec3f ) );
	}

}//namespace MV