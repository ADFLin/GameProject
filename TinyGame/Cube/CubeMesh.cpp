#include "CubePCH.h"
#include "CubeMesh.h"

#include "Win32Header.h"
#include <gl/GL.h>
#include <gl/GLU.h>

namespace Cube
{

	Mesh::Mesh()
	{
		mVtxBuffer.reserve( 4 * 1000 );
		mIndexBuffer.reserve( 3 * 2 * 1000 );
		mIndexBase = 0;
		mVertexOffset.setValue( 0 , 0 , 0 );
	}

	void Mesh::addVertex( Vec3f const& pos )
	{
		addVertex( pos.x , pos.y , pos.z );
	}

	void Mesh::addVertex( float x , float y , float z )
	{
		mCurVertex.pos[0] = mVertexOffset.x + x;
		mCurVertex.pos[1] = mVertexOffset.y + y;
		mCurVertex.pos[2] = mVertexOffset.z + z;
		mVtxBuffer.push_back( mCurVertex );

	}

	void Mesh::setIndexBase( int index )
	{
		mIndexBase = index;
	}

	void Mesh::addTriangle( int o1 , int o2 , int o3 )
	{
		mIndexBuffer.push_back( mIndexBase + o1 );
		mIndexBuffer.push_back( mIndexBase + o2 );
		mIndexBuffer.push_back( mIndexBase + o3 );
	}

	void Mesh::addQuad( int o1 , int o2 , int o3 , int o4 )
	{
		addTriangle( o1 , o2 , o3 );
		addTriangle( o1 , o3 , o4 );
	}

	void Mesh::setColor( uint8 r , uint8 g , uint8 b )
	{
		mCurVertex.color[0] = r;
		mCurVertex.color[1] = g;
		mCurVertex.color[2] = b;
		mCurVertex.color[3] = 255;
	}

	void Mesh::render()
	{

		glEnableClientState( GL_VERTEX_ARRAY );
		glEnableClientState( GL_COLOR_ARRAY );
		//glEnableClientState( GL_INDEX_ARRAY );

		glVertexPointer( 3 , GL_FLOAT , sizeof( Vertex ) , &mVtxBuffer[0].pos );
		glColorPointer( 4 , GL_UNSIGNED_BYTE , sizeof( Vertex ) , &mVtxBuffer[0].color );
		//glIndexPointer( GL_INT , sizeof( int32 ) , &mIndexBuffer[0] );
		glDrawElements( GL_TRIANGLES ,  (int)mIndexBuffer.size() , GL_UNSIGNED_INT , &mIndexBuffer[0] );

		glDisableClientState( GL_VERTEX_ARRAY );
		glDisableClientState( GL_COLOR_ARRAY );
		//glDisableClientState( GL_INDEX_ARRAY );
	}

}//namespace Cube
