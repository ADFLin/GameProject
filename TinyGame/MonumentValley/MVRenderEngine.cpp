#include "MVRenderEngine.h"

#include "tinyobjloader/tiny_obj_loader.h"

#include "GLCommon.h"
#include "GLUtility.h"

#include <fstream>

namespace MV
{
	using namespace GL;

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


	bool MeshLoader::load( GL::Mesh& mesh , char const* path )
	{
		std::ifstream objStream( path );
		if ( !objStream.is_open() )
			return false;

		class MaterialStringStreamReader : public tinyobj::MaterialReader
		{
		public:
			virtual std::string operator() (
				const std::string& matId,
				std::vector< tinyobj::material_t >& materials,
				std::map<std::string, int>& matMap)
			{
				return "";
			}
		};  

		MaterialStringStreamReader matSSReader;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		std::string err = tinyobj::LoadObj(shapes, materials, objStream, matSSReader);

		if ( shapes.empty() )
			return true;

		tinyobj::mesh_t& objMesh = shapes[0].mesh;
		int numVertex = objMesh.positions.size() / 3;
		std::vector< float > vertices( 3 * numVertex * 2 );
		float* v = &vertices[0];
		float* p = &objMesh.positions[0];
		float* n = &objMesh.normals[0];
		for( int i = 0 ; i < numVertex ; ++i )
		{
			v[0] = p[0];v[1] = p[1];v[2] = p[2];
			v[3] = n[0];v[4] = n[1];v[5] = n[2];
			p +=3;
			n +=3;
			v +=6;
		}

		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eNormal   , Vertex::eFloat3 );
		if ( !mesh.createBuffer( &vertices[0] , numVertex , &objMesh.indices[0] , objMesh.indices.size() , true ) )
			return false;
		return true;
	}

	bool MeshLoader::load( GL::Mesh& mesh , Data& data )
	{
		int maxNumVertex = data.numPosition * data.numNormal;
		std::vector< int > idxMap( maxNumVertex , -1 );
		std::vector< float > vertices;
		std::vector< int > indices( data.numIndex );
		vertices.reserve( maxNumVertex );

		int numVertex = 0;
		for( int i = 0 ; i < data.numIndex; ++i )
		{
			int idxPos = data.indices[2*i] - 1;
			int idxNormal = data.indices[2*i + 1] - 1;
			int idxToVertex = idxPos + idxNormal * data.numPosition;
			int idx = idxMap[idxToVertex];
			if ( idx == -1 )
			{
				idx = numVertex;
				++numVertex;
				idxMap[idxToVertex] = idx;

				float* p = data.position + 3 * idxPos;
				vertices.push_back( p[0] );
				vertices.push_back( p[1] );
				vertices.push_back( p[2] );
				float* n = data.normal+ 3 * idxNormal;
				vertices.push_back( n[0] );
				vertices.push_back( n[1] );
				vertices.push_back( n[2] );
			}
			indices[i] = idx;
		}

		mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
		mesh.mDecl.addElement( Vertex::eNormal   , Vertex::eFloat3 );
		if ( !mesh.createBuffer( &vertices[0] , numVertex , &indices[0] , data.numIndex , true ) )
			return false;
		return true;
	}


	static RenderEngine* gRE = NULL;

	RenderEngine& getRenderEngine(){ return *gRE; }

	RenderEngine::RenderEngine()
	{
		gRE = this;
	}

	bool RenderEngine::init()
	{
		if ( !mEffect.loadFromSingleFile( "Shader/MVPiece") )
			return false;

		locDirX = mEffect.getParamLoc( "dirX" );
		locDirZ = mEffect.getParamLoc( "dirZ" );
		locLocalScale = mEffect.getParamLoc( "localScale" );

		{
			MeshLoader loader;

			if ( !MeshUtility::createCube( mMesh[ MESH_BOX ] , 0.5f ) ||
				!MeshUtility::createUVSphere( mMesh[ MESH_SPHERE ] , 0.3 , 10 , 10 ) ||
				!MeshUtility::createPlane( mMesh[ MESH_PLANE ] , Vector3(0.5,0,0) ,Vector3(1,0,0) , Vector3(0,1,0) , 0.5 , 1 ) )
				return false;

			for( int i = 0 ; i < ARRAY_SIZE( gMeshInfo ) ; ++i )
			{
				MeshInfo const& info = gMeshInfo[i];
				FixString< 256 > path = "Mesh/";
				path += info.name; path += ".obj";
				if ( !loader.load( mMesh[ info.id ] , path ) )
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
		mEffect.setParam( "gParam.matViewInv" , matViewInv );

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
			RenderRT::draw< RenderRT::eXYZ >( GL_LINE_STRIP , &points[0] , points.size() , sizeof( Vec3f ) );
	}

	void RenderEngine::renderMesh(int idMesh , Vec3f const& pos , Vec3f const& rotation)
	{
		glPushMatrix();
		glTranslatef( pos.x , pos.y , pos.z );
		mEffect.setParam( locDirX , (int)eDirX );
		mEffect.setParam( locDirZ , (int)eDirZ );
		Quat q; q.setEulerZYX( rotation.z , rotation.y , rotation.x );
		glMultMatrixf( Matrix4::Rotate( q ) );
		mMesh[ idMesh ].draw();
		glPopMatrix();
	}

	void RenderEngine::renderMesh(int idMesh , Vec3f const& pos , Roataion const& rotation)
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

				for( int idx = 0 ; idx < NUM_FACE_NAV_LINK ; ++idx )
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
				for( int idx = 0 ; idx < NUM_FACE_NAV_LINK ; ++idx )
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
		RenderRT::draw< RenderRT::eXYZ_C >( GL_LINES , buffer , nV  );
		mEffect.bind();
	}

}//namespace MV