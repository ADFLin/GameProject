#include "MVRenderEngine.h"

#include "RenderGL/GLCommon.h"
#include "RenderGL/DrawUtility.h"
#include "RenderGL/ShaderCompiler.h"

#include "FixString.h"

#include <fstream>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE( ar ) ( sizeof(ar)/sizeof(ar[0]) )
#endif

namespace MV
{
	using namespace RenderGL;

	struct MeshInfo
	{
		MeshId id;
		char const* name;
	};

	MeshInfo gMeshInfo[] = 
	{
		{ MESH_STAIR  , "stair" } ,
		{ MESH_TRIANGLE , "tri" } ,
		{ MESH_ROTATOR_C , "rotator" } ,
		{ MESH_ROTATOR_NC , "rotatorF" } ,
		{ MESH_CORNER1 , "corner1" }  ,
		{ MESH_LADDER , "ladder" }  ,
	};

	float groupColor[][3] = 
	{
		{ 1 , 1 , 1 },
		{ 1 , 0.5 , 0.5 },
		{ 0.5 , 1 , 0.5 },
		{ 0.5 , 0.5 , 1 },
		{ 1 , 1 , 0.5 },
		{ 1 , 0.5 , 1 },
		{ 0.5 , 1 , 1 },
	};
	static RenderEngine* gRE = NULL;


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
	{
		gRE = this;
	}

	bool RenderEngine::init()
	{
		if ( !ShaderManager::Get().loadFileSimple(mEffect , "Shader/Game/MVPiece") )
			return false;

		locDirX = mEffect.getParamLoc( "dirX" );
		locDirZ = mEffect.getParamLoc( "dirZ" );
		locLocalScale = mEffect.getParamLoc( "localScale" );

		{
			if ( !MeshBuild::Cube( mMesh[ MESH_BOX ] , 0.5f ) ||
				!MeshBuild::UVSphere( mMesh[ MESH_SPHERE ] , 0.3 , 10 , 10 ) ||
				!MeshBuild::Plane( mMesh[ MESH_PLANE ] , Vector3(0.5,0,0) ,Vector3(1,0,0) , Vector3(0,1,0) , 0.5 , 1 ) )
				return false;

			for( int i = 0 ; i < ARRAY_SIZE( gMeshInfo ) ; ++i )
			{
				MeshInfo const& info = gMeshInfo[i];
				FixString< 256 > path = "Mesh/";
				path += info.name; 
				path += ".obj";
				if ( !MeshBuild::LoadObjectFile( mMesh[ info.id ] , path ) )
					return false;
			}
		}


		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glEnable( GL_CULL_FACE );
		glEnable( GL_DEPTH_TEST );


		return true;
	}



	void RenderEngine::renderScene(Mat4 const& matView)
	{
		Mat4 matViewInv;
		float det;
		matView.inverse( matViewInv , det );
		mEffect.bind();
		mEffect.setParam( SHADER_PARAM(View.viewToWorld) , matViewInv );

		glLoadMatrixf( matView );
		renderGroup( mParam.world->mRootGroup );

		mEffect.unbind();
	}

	void RenderEngine::renderBlock(Block& block , Vec3i const& pos)
	{
		if ( mParam.bShowGroupColor )
		{
			int idx = ( block.group == &mParam.world->mRootGroup ) ? 0 : ( block.group->idx );
			glColor3fv( groupColor[idx % 7 ] );
		}
		else
		{
			glColor3f( 1 , 1 , 1 );
		}

		glPushMatrix();
		glTranslatef( pos.x , pos.y , pos.z );
		mEffect.setParam( locDirX , (int)block.rotation[0] );
		mEffect.setParam( locDirZ , (int)block.rotation[2] );
		mMesh[ block.idMesh ].draw();

		for( int i = 0 ; i < 6 ; ++i )
		{
			BlockSurface& surface =  block.getLocalFace( Dir( i ) );
			if ( surface.fun == NFT_LADDER )
			{
				Vec3f offset = 0.5 * FDir::OffsetF( block.rotation.toWorld( Dir(i) ) );
				glTranslatef( offset.x , offset.y , offset.z );
				mMesh[ MESH_LADDER ].draw();
			}
		}
		glPopMatrix();
	}

	void RenderEngine::renderPath( Path& path , PointVec& points)
	{
		if ( !points.empty() )
			TRenderRT< RTVF_XYZ >::Draw( PrimitiveType::LineStrip , &points[0] , points.size() , sizeof( Vec3f ) );
	}

	void RenderEngine::renderMesh(int idMesh , Vec3f const& pos , Vec3f const& rotation)
	{
		glPushMatrix();
		glTranslatef( pos.x , pos.y , pos.z );
		mEffect.setParam( locDirX , (int)Dir::X );
		mEffect.setParam( locDirZ , (int)Dir::Z );
		Quat q; q.setEulerZYX( rotation.z , rotation.y , rotation.x );
		glMultMatrixf( Matrix4::Rotate( q ) );
		mMesh[ idMesh ].draw();
		glPopMatrix();
	}

	void RenderEngine::renderMesh(int idMesh , Vec3f const& pos , AxisRoataion const& rotation)
	{
		glPushMatrix();
		glTranslatef( pos.x , pos.y , pos.z );
		mEffect.setParam( locDirX , (int)rotation[0] );
		mEffect.setParam( locDirZ , (int)rotation[2] );
		mMesh[ idMesh ].draw();
		glPopMatrix();
	}

	void RenderEngine::renderGroup(ObjectGroup& group)
	{
		if ( group.node )
			group.node->prevRender();

		for( BlockList::iterator iter = group.blocks.begin() , itEnd = group.blocks.end();
			iter != itEnd ; ++iter )
		{
			Block* block = *iter;
			renderBlock( *block , block->pos );
		}

		if ( mParam.bShowNavNode )
		{
			renderNav( group );
		}
		for( ActorList::iterator iter = group.actors.begin() , itEnd = group.actors.end();
			iter != itEnd ; ++iter)
		{
			Actor* actor = *iter;
			renderActor( *actor );
		}

		for( MeshList::iterator iter = group.meshs.begin() , itEnd = group.meshs.end();
			iter != itEnd ; ++iter)
		{
			MeshObject* mesh = *iter;
			renderMesh( mesh->idMesh , mesh->pos , mesh->rotation );
		}

		if ( group.node )
		{
			if ( !group.node->bModifyChildren )
				group.node->postRender();

			for( GroupList::iterator iter = group.children.begin() , itEnd = group.children.end();
				iter != itEnd ; ++iter )
			{
				ObjectGroup* child = *iter;
				renderGroup( *child );
			}

			if ( group.node->bModifyChildren )
				group.node->postRender();
		}
		else
		{
			for( GroupList::iterator iter = group.children.begin() , itEnd = group.children.end();
				iter != itEnd ; ++iter )
			{
				ObjectGroup* child = *iter;
				renderGroup( *child );
			}

		}
	}

	void RenderEngine::renderActor(Actor& actor)
	{
		glPushMatrix();

		Vec3f pos = actor.renderPos + 0.5 * FDir::OffsetF( actor.actFaceDir );

		Vec3f frontOffset = FDir::OffsetF( actor.rotation[0] );
		Vec3f upOffset = FDir::OffsetF( actor.rotation[2] );

		switch( actor.actState )
		{
		case Actor::eActClimb: pos += 0.5 * upOffset; break;
		}
		glTranslatef( pos.x , pos.y , pos.z );

		mEffect.setParam( locDirX , (int)actor.rotation[0] );
		mEffect.setParam( locDirZ , (int)actor.rotation[2] );

		glColor3f( 0.5 , 0.5 , 0.5 );

		glPushMatrix();
		mEffect.setParam( locLocalScale , Vec3f( 0.4 , 0.6 , 1.0 ) );
		mMesh[ MESH_BOX ].draw();
		mEffect.setParam( locLocalScale , Vec3f( 1.0 , 1.0 , 1.0 ) );
		glPopMatrix();

		//Vector3 offset = actor.moveOffset * cast( frontOffset ) - 0.2 * cast( upOffset );
		glTranslatef( 0.9 * upOffset.x, 0.9 * upOffset.y, 0.9 * upOffset.z );

		mMesh[ MESH_SPHERE ].draw();

		mEffect.unbind();

		glBegin( GL_LINES );
		glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(frontOffset.x,frontOffset.y,frontOffset.z);
		glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(upOffset.x,upOffset.y,upOffset.z); 
		glEnd();
		mEffect.bind();
		glPopMatrix();
	}

	void RenderEngine::renderNav(ObjectGroup &group)
	{
		mEffect.unbind();

		float* buffer = useCacheBuffer( group.blocks.size() * 6 * 4 * 2 * 2 * 6 );

		float* v = buffer;
		int nV = 0;
		for( BlockList::iterator iter = group.blocks.begin() , itEnd = group.blocks.end();
			iter != itEnd ; ++iter )
		{
			Block* block = *iter;

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
		TRenderRT< RTVF_XYZ_C >::Draw( PrimitiveType::LineList, buffer , nV  );
		mEffect.bind();
	}

}//namespace MV