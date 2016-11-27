#include "TinyGamePCH.h"
#include "LightingStage.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"
#include "RenderUtility.h"
#include "GLGraphics2D.h"



namespace Lighting
{
	bool TestStage::onInit()
	{
		if ( !Global::getDrawEngine()->startOpenGL() )
			return false;

		GameWindow& window = Global::getDrawEngine()->getWindow();

		program    = glCreateProgram();
		fragShader = glCreateShader( GL_FRAGMENT_SHADER );

		char const* strFrag = readFile( "Shader/light.glsl" );

		if ( strFrag == NULL )
			return false;

		glShaderSource( fragShader , 1 , &strFrag , 0);
		glCompileShader( fragShader );
		glAttachShader( program , fragShader );
		glLinkProgram( program );

		loc_lightLocation = glGetUniformLocation( program , "lightLocation" );
		loc_lightColor = glGetUniformLocation( program , "lightColor" );
		loc_lightAttenuation = glGetUniformLocation( program , "lightAttenuation" );


		free( (void*)strFrag );

		glClearColor( 0 , 0 , 0 , 0 );

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, window.getWidth() , window.getHeight() , 0 , 1, -1);
		glMatrixMode(GL_MODELVIEW);

		::Global::GUI().cleanupWidget();
		WidgetUtility::createDevFrame();
		restart();
		return true;
	}

	void TestStage::onEnd()
	{
		glDeleteProgram( program );
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
		glOrtho(0, window.getWidth(), window.getHeight(), 0, 1, -1);
		glMatrixMode(GL_MODELVIEW);

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

			glUseProgram(program);

			glUniform2f( loc_lightLocation , light.pos.x , light.pos.y );
			glUniform3f( loc_lightColor , light.color.x , light.color.y , light.color.z );
			glUniform3f( loc_lightAttenuation , 0 , 1 / 5.0 , 0 );

			glBegin( GL_QUADS );
			glVertex2i( 0 , 0 );
			glVertex2i( w , 0 );
			glVertex2i( w , h );
			glVertex2i( 0 , h );
			glEnd();

			glUseProgram(0);
			glDisable(GL_BLEND);
			glClear(GL_STENCIL_BUFFER_BIT);
		}

		glDisable( GL_STENCIL_TEST );


		GLGraphics2D& g = ::Global::getDrawEngine()->getGLGraphics();

		g.beginRender();

		RenderUtility::setFont( g , FONT_S8 );
		FixString< 256 > str;
		Vec2i pos = Vec2i( 10 , 10 );
		g.drawText( pos , str.format( "Lights Num = %u" , lights.size() ) );

		g.endRender();
	}

	void TestStage::renderPolyShadow( Light const& light , Vec2f const& pos , Vec2f const* vertices , int numVertex  )
	{
		int idxPrev = numVertex - 1;
		for( int idxCur = 0 ; idxCur < numVertex ; idxPrev = idxCur , ++idxCur )
		{
			Vec2f const& cur  = pos + vertices[ idxCur ];
			Vec2f const& prev = pos + vertices[ idxPrev ];
			Vec2f edge = cur - prev;

			Vec2f dirCur = cur - light.pos;
			Vec2f dirPrev = prev - light.pos;

			if ( dirCur.x * edge.y - dirCur.y * edge.x < 0 )
			{
				Vec2f v1 = prev + 1000 * dirPrev;
				Vec2f v2 = cur + 1000 * dirCur;

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

		if ( msg.onLeftDown() )
		{
			Light light;
			light.pos = Vec2f( msg.getPos().x , window.getHeight() - msg.getPos().y  );
			light.color = Color( float( ::Global::Random() ) / RAND_MAX  , 
				                 float( ::Global::Random() ) / RAND_MAX  , 
				                 float( ::Global::Random() ) / RAND_MAX  );
			lights.push_back( light );
		}
		else if ( msg.onRightDown() )
		{
			Block block;
			//TODO
			block.setBox( Vec2f( msg.getPos().x , msg.getPos().y  ) , Vec2f( 50 , 50 ) );
			blocks.push_back( block );
		}
		else if ( msg.onMoving() )
		{
			if ( !lights.empty() )
			{
				lights.back().pos = Vec2f( msg.getPos().x , window.getHeight() - msg.getPos().y  );
			}
		}

		return true;
	}

}//namespace Lighting