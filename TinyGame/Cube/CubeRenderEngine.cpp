#include "CubePCH.h"
#include "CubeRenderEngine.h"

#include "CubeBlockRenderer.h"

#include "CubeWorld.h"

#include "WindowsHeader.h"
#include <gl\gl.h>		// Header File For The OpenGL32 Library
#include <gl\glu.h>		// Header File For The GLu32 Library

namespace Cube
{

	RenderEngine::RenderEngine( int w , int h )
	{
		mClientWorld = NULL;
		mBlockRenderer = new BlockRenderer;
		mAspect        = float( w ) / h ;

		mBlockRenderer->mMesh = &mMesh;

		mRenderWidth  = 0;
		mRenderHeight = ChunkBlockMaxHeight;
		mRenderDepth  = 0;

		glDisable( GL_TEXTURE );
		//glDisable( GL_CULL_FACE );


		glEnable( GL_CULL_FACE );
		glCullFace( GL_BACK );
	}

	void RenderEngine::setupWorld( World& world )
	{
		if ( mClientWorld )
			mClientWorld->removeListener( *this );

		mClientWorld = &world;
		mClientWorld->addListener( *this );

		mBlockRenderer->mBlockAccess = &world;
	}

	void RenderEngine::renderWorld( ICamera& camera )
	{
		World& world = *mClientWorld;

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		gluPerspective( 45.0f , mAspect , 0.01 , 1000.0 );

		Vec3f camPos  = camera.getPos();
		Vec3f viewPos = camPos + camera.getViewDir();
		Vec3f upDir   = camera.getUpDir();

		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		gluLookAt( camPos.x , camPos.y , camPos.z , 
			       viewPos.x , viewPos.y , viewPos.z , 0 , 0 , 1.0 );


		{
			
			glPushMatrix();

			glTranslatef( 0 , 10 , 0 );

			float len = 10;
			drawCroodAxis(len);


			glPopMatrix();
		}

#if 0
		glColor3f (1.0, 1.0, 0.0);//
		glBegin ( GL_TRIANGLES );//
		glVertex3f( 0 , 0 , 0  );// 
		glVertex3f(0 ,0.5 , 0 );
		glVertex3f(0 ,0.5 , 0.5  );
		glEnd ();

		mMesh.clearBuffer();
		mMesh.setVertexOffset( Vec3f(0,0,0) );
		mMesh.setColor( 0 , 255 , 0 );
		mMesh.addVertex( 0 , 0 , 0  );
		mMesh.addVertex( 0 ,0.5 , 0 );
		mMesh.addVertex( 0 ,0.5 , 0.5  );
		mMesh.addTriangle( 0 , 1 , 2 );
		mMesh.render();

		for( int i = 0 ; i < 1 ; ++i )
		{
			//glTranslatef( 0 , 1 , 0 );
			mBlockRenderer->drawUnknown( 0xffffffff );
		}
		mMesh.render();
#endif


		int bx = Math::floor( camPos.x );
		int by = Math::floor( camPos.y );

		ChunkPos cPos;
		cPos.setBlockPos( bx , by );
		int len = 1;

		glPolygonMode(GL_FRONT, GL_LINE);
		for( int i = -len ; i <= len ; ++i )
		{
			for( int j = -len ; j <= len ; ++j )
			{
				ChunkPos curPos;
				curPos.x = cPos.x + i;
				curPos.y = cPos.y + j;

				WDMap::iterator iter = mWDMap.find( curPos.hash_value() );
				WorldData* data = NULL;
				if ( iter == mWDMap.end() )
				{
					Chunk* chunk = world.getChunk( curPos );

					if ( !chunk )
						continue;

					data = new WorldData;
					data->needUpdate = true;
					data->drawList[0] = glGenLists( WorldData::NumPass );
					for( int i = 1 ; i < WorldData::NumPass ; ++i )
					{
						data->drawList[i] = data->drawList[0] + i;
					}
					mWDMap.insert( std::make_pair( curPos.hash_value() , data ) );
				}
				else
				{
					data = iter->second;
				}

				if ( data->needUpdate )
				{
					Chunk* chunk = world.getChunk( curPos );

					assert( chunk );

					glPushMatrix();
					glLoadIdentity();

					for( int i = 0 ; i < WorldData::NumPass ; ++i )
					{
						mMesh.clearBuffer();
						chunk->render( *mBlockRenderer );

						glNewList( data->drawList[i] , GL_COMPILE );
						mMesh.render();
						glEndList();
					}
					glPopMatrix();
					data->needUpdate = false;
				}
				else
				{
					glPushMatrix();
					int bx = ChunkSize * curPos.x;
					int by = ChunkSize * curPos.y;
					glTranslatef( bx , by , 0 );
					glCallLists( WorldData::NumPass , GL_UNSIGNED_INT , data->drawList );
					glPopMatrix();
				}
			}
		}

		BlockPosInfo info;
		BlockId id = world.rayBlockTest( camPos , camera.getViewDir() , 100 , &info );
		if ( id )
		{
			glPushMatrix();
			glTranslatef( info.x  , info.y , info.z );
			glColor3f ( 1 , 1 , 1 );
			glBegin( GL_LINE_LOOP );

			switch( info.face )
			{
			case FACE_X:
				glVertex3f( 1 , 0 , 0 );
				glVertex3f( 1 , 1 , 0 );
				glVertex3f( 1 , 1 , 1 );
				glVertex3f( 1 , 0 , 1 );
				break;
			case FACE_NX:
				glVertex3f( 0 , 0 , 0 );
				glVertex3f( 0 , 1 , 0 );
				glVertex3f( 0 , 1 , 1 );
				glVertex3f( 0 , 0 , 1 );
				break;
			case FACE_Y:
				glVertex3f( 0 , 1 , 0 );
				glVertex3f( 1 , 1 , 0 );
				glVertex3f( 1 , 1 , 1 );
				glVertex3f( 0 , 1 , 1 );
				break;
			case FACE_NY:
				glVertex3f( 0 , 0 , 0 );
				glVertex3f( 1 , 0 , 0 );
				glVertex3f( 1 , 0 , 1 );
				glVertex3f( 0 , 0 , 1 );
				break;
			case FACE_Z:
				glVertex3f( 0 , 0 , 1 );
				glVertex3f( 1 , 0 , 1 );
				glVertex3f( 1 , 1 , 1 );
				glVertex3f( 0 , 1 , 1 );
				break;
			case FACE_NZ:
				glVertex3f( 0 , 0 , 0 );
				glVertex3f( 1 , 0 , 0 );
				glVertex3f( 1 , 1 , 0 );
				glVertex3f( 0 , 1 , 0 );
				break;
			}

			glEnd();
			glPopMatrix();
		}

	}

	void RenderEngine::beginRender()
	{
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	}

	void RenderEngine::endRender()
	{
		glFlush();
	}

	void RenderEngine::onModifyBlock( int bx , int by , int bz )
	{
		ChunkPos cPos; 
		{
			cPos.setBlockPos( bx + 1 , by );
			WDMap::iterator iter = mWDMap.find( cPos.hash_value() );
			if ( iter != mWDMap.end() )
				iter->second->needUpdate = true;
		}
		{
			cPos.setBlockPos( bx - 1 , by );
			WDMap::iterator iter = mWDMap.find( cPos.hash_value() );
			if ( iter != mWDMap.end() )
				iter->second->needUpdate = true;
		}
		{
			cPos.setBlockPos( bx , by + 1 );
			WDMap::iterator iter = mWDMap.find( cPos.hash_value() );
			if ( iter != mWDMap.end() )
				iter->second->needUpdate = true;
		}
		{
			cPos.setBlockPos( bx , by - 1 );
			WDMap::iterator iter = mWDMap.find( cPos.hash_value() );
			if ( iter != mWDMap.end() )
				iter->second->needUpdate = true;
		}
	}

	void RenderEngine::drawCroodAxis( float len )
	{
		glBegin( GL_LINES );
		glColor3f ( 1 , 0,  0  );
		glVertex3f( 0 , 0 , 0  );
		glVertex3f( len , 0 , 0 );
		glEnd();
		glBegin( GL_LINES );
		glColor3f ( 0 , 1,  0  );
		glVertex3f( 0 , 0 , 0  );
		glVertex3f( 0 , len , 0 );
		glEnd();
		glBegin( GL_LINES );
		glColor3f ( 0 , 0,  1  );
		glVertex3f( 0 , 0 , 0  );
		glVertex3f( 0 , 0 , len );
		glEnd();
	}

}//namespace Cube