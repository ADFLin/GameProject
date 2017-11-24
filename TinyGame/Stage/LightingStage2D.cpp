#include "TinyGamePCH.h"
#include "LightingStage2D.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"
#include "RenderUtility.h"
#include "GLGraphics2D.h"

#include "RenderGL/ShaderCompiler.h"

namespace Lighting2D
{
	bool TestStage::onInit()
	{
		if ( !Global::getDrawEngine()->startOpenGL() )
			return false;

		GameWindow& window = Global::getDrawEngine()->getWindow();

		char const* entryNames[] = { SHADER_ENTRY(LightingPS) };
		if( !ShaderManager::getInstance().loadFile(
			mProgram, "Shader/Game/lighting2D", 
			BIT(RHIShader::ePixel), entryNames ) )
			return false;

		glClearColor( 0 , 0 , 0 , 0 );

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, window.getWidth() , window.getHeight() , 0 , 1, -1);
		glMatrixMode(GL_MODELVIEW);

		::Global::GUI().cleanupWidget();
		WidgetUtility::CreateDevFrame();
		restart();
		return true;
	}

	void TestStage::onEnd()
	{
		Global::getDrawEngine()->stopOpenGL();
	}

	void TestStage::restart()
	{
		lights.clear();
		blocks.clear();
	}

	void TestStage::onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	char* TestStage::readFile( char const* file )
	{
		FILE* f= fopen(file,"rt");

		if ( !f )
			return NULL;

		fseek(f,0,SEEK_END);
		int broj=ftell(f);
		rewind(f);

		char* output=(char*)malloc(sizeof(char)*(broj+1));
		broj=fread(output,sizeof(char),broj,f);
		output[broj]='\0';

		fclose(f);

		return output;
	}

	void TestStage::onRender( float dFrame )
	{
		GameWindow& window = Global::getDrawEngine()->getWindow();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, window.getWidth(), 0 , window.getHeight(), 1, -1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		glEnable( GL_STENCIL_TEST );


		int w = window.getWidth();
		int h = window.getHeight();

	
		for ( LightList::iterator iter = lights.begin(), itEnd = lights.end();
			  iter != itEnd ; ++iter )
		{
			Light& light = *iter;

#if 1
			glColorMask(false, false, false, false);
			glStencilFunc(GL_ALWAYS, 1, 1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

			for( BlockList::iterator iter = blocks.begin(), itEnd = blocks.end();
				 iter != itEnd ; ++iter )
			{
				Block& block = *iter;
				renderPolyShadow( light , block.pos , block.getVertices() , block.getVertexNum() );
			}
			
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glStencilFunc(GL_EQUAL, 0, 1);
			glColorMask(true, true, true, true);
#endif

			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			{
				GL_BIND_LOCK_OBJECT(mProgram);

				mProgram.setParameters(light.pos, light.color);
				glBegin(GL_QUADS);
				glVertex2i(0, 0);
				glVertex2i(w, 0);
				glVertex2i(w, h);
				glVertex2i(0, h);
				glEnd();
			}
			glDisable(GL_BLEND);
			glClear(GL_STENCIL_BUFFER_BIT);
		}

		glDisable( GL_STENCIL_TEST );


		GLGraphics2D& g = ::Global::getDrawEngine()->getGLGraphics();

		g.beginRender();

		RenderUtility::SetFont( g , FONT_S8 );
		FixString< 256 > str;
		Vec2i pos = Vec2i( 10 , 10 );
		g.drawText( pos , str.format( "Lights Num = %u" , lights.size() ) );

		g.endRender();
	}

	void TestStage::renderPolyShadow( Light const& light , Vector2 const& pos , Vector2 const* vertices , int numVertex  )
	{
		int idxPrev = numVertex - 1;
		for( int idxCur = 0 ; idxCur < numVertex ; idxPrev = idxCur , ++idxCur )
		{
			Vector2 const& cur  = pos + vertices[ idxCur ];
			Vector2 const& prev = pos + vertices[ idxPrev ];
			Vector2 edge = cur - prev;

			Vector2 dirCur = cur - light.pos;
			Vector2 dirPrev = prev - light.pos;

			if ( dirCur.x * edge.y - dirCur.y * edge.x < 0 )
			{
				Vector2 v1 = prev + 1000 * dirPrev;
				Vector2 v2 = cur + 1000 * dirCur;

				glBegin( GL_QUADS );
				glVertex2f( prev.x , prev.y );
				glVertex2f( v1.x , v1.y );
				glVertex2f( v2.x , v2.y  );
				glVertex2f( cur.x , cur.y );
				glEnd();
			}
		}
	}

	bool TestStage::onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

		GameWindow& window = Global::getDrawEngine()->getWindow();

		Vector2 worldPos = Vector2(msg.getPos().x, window.getHeight() - msg.getPos().y);
		if ( msg.onLeftDown() )
		{
			Light light;
			light.pos = worldPos;
			light.color = Color( float( ::Global::Random() ) / RAND_MAX  , 
				                 float( ::Global::Random() ) / RAND_MAX  , 
				                 float( ::Global::Random() ) / RAND_MAX  );
			lights.push_back( light );
		}
		else if ( msg.onRightDown() )
		{
			Block block;
			//#TODO
			block.setBox(worldPos, Vector2( 50 , 50 ) );
			blocks.push_back( block );
		}
		else if ( msg.onMoving() )
		{
			if ( !lights.empty() )
			{
				lights.back().pos = worldPos;
			}
		}

		return true;
	}

}//namespace Lighting