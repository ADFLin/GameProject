#include "TinyGamePCH.h"
#include "LightingStage2D.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"
#include "RenderUtility.h"

#include "GLGraphics2D.h"

#include "RenderGL/ShaderCompiler.h"
#include "RenderGL/DrawUtility.h"


#include "GL/wglew.h"

namespace Lighting2D
{
	bool TestStage::onInit()
	{
		if ( !Global::getDrawEngine()->startOpenGL() )
			return false;

		wglSwapIntervalEXT(0);

		GameWindow& window = Global::getDrawEngine()->getWindow();

		{
			ShaderEntryInfo entries[] =
			{
				{ Shader::ePixel , SHADER_ENTRY(LightingPS) } ,
				{ Shader::eEmpty , nullptr },
			};
			if( !ShaderManager::Get().loadFile(
				mProgLighting, "Shader/Game/lighting2D", entries ) )
				return false;
		}

		{
			FixString< 128 > defineStr;
			ShaderCompileOption option;
			option.version = 430;
			ShaderEntryInfo entries[] =
			{ 
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) } ,
				{ Shader::eGeometry , SHADER_ENTRY(MainGS) },
				{ Shader::eEmpty , nullptr },
			};
			if( !ShaderManager::Get().loadFile(
				mProgShadow, "Shader/Game/Lighting2DShadow", entries, option ) )
				return false;
		}

		::Global::GUI().cleanupWidget();
		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_RUN_BENCHMARK, "Run Benchmark");
		restart();

		return true;
	}

	void TestStage::onInitFail()
	{
		Global::getDrawEngine()->stopOpenGL(true);
	}

	void TestStage::onEnd()
	{
		Global::getDrawEngine()->stopOpenGL(true);
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

	bool TestStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch( id )
		{
		case UI_RUN_BENCHMARK:
			{
				int lightNum = 100;
				int blockNum = 500;
				GameWindow& window = Global::getDrawEngine()->getWindow();

				int w = window.getWidth();
				int h = window.getHeight();

				for( int i = 0; i < lightNum; ++i )
				{
					Light light;
					light.pos.x = ::Global::Random() % w;
					light.pos.y = ::Global::Random() % h;
					light.color = Color(float(::Global::Random()) / RAND_MAX,
										float(::Global::Random()) / RAND_MAX,
										float(::Global::Random()) / RAND_MAX);
					lights.push_back(light);
				}
				for( int i = 0; i < blockNum; ++i )
				{
					Block block;
					Vector2 pos;
					pos.x = ::Global::Random() % w;
					pos.y = ::Global::Random() % h;
					block.setBox(pos, Vector2(25, 25));
					blocks.push_back(std::move(block));

				}
			}
			return false;


		}
		return BaseClass::onWidgetEvent(event, id, ui);
	}

	void TestStage::onRender(float dFrame)
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

#if SHADOW_USE_GEOMETRY_SHADER
		mBuffers.clear();
		for( Block& block : blocks )
		{
			renderPolyShadow(Light(), block.pos, block.getVertices(), block.getVertexNum());
		}
#endif
		for ( Light& light : lights )
		{
			glColorMask(false, false, false, false);
			glStencilFunc(GL_ALWAYS, 1, 1);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

			{

#if SHADOW_USE_GEOMETRY_SHADER
				GL_BIND_LOCK_OBJECT(mProgShadow);
				mProgShadow.setParameters(light.pos);
#endif

#if !SHADOW_USE_GEOMETRY_SHADER
				mBuffers.clear();
				for( Block& block : blocks )
				{
					renderPolyShadow(light, block.pos, block.getVertices(), block.getVertexNum());
				}
#endif

				if( !mBuffers.empty() )
				{
#if SHADOW_USE_GEOMETRY_SHADER
					TRenderRT< RTVF_XY >::DrawShader(PrimitiveType::eLineList, &mBuffers[0], mBuffers.size());
#else
					TRenderRT< RTVF_XY >::DrawShader(PrimitiveType::eQuad, &mBuffers[0], mBuffers.size());
#endif
				}
			}
			
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glStencilFunc(GL_EQUAL, 0, 1);
			glColorMask(true, true, true, true);

			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			{
				GL_BIND_LOCK_OBJECT(mProgLighting);
				mProgLighting.setParameters(light.pos, light.color);
				DrawUtility::RectShader(w, h);
			}
			glDisable(GL_BLEND);
			glClear(GL_STENCIL_BUFFER_BIT);
		}

		glDisable( GL_STENCIL_TEST );


#if 0
		glColor3f(1, 0, 0);
		Vector2 vertices[] =
		{
			Vector2(100 , 100),
			Vector2(200 , 100),
			Vector2(200 , 200),
			Vector2(100 , 200),
		};
		int indices[] =
		{
			0,1,2,0,2,3,
		};
		TRenderRT<RTVF_XY>::DrawIndexed(PrimitiveType::eTriangleList, vertices, 4, indices, 6);
#endif

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
#if SHADOW_USE_GEOMETRY_SHADER

		int idxPrev = numVertex - 1;
		for( int idxCur = 0; idxCur < numVertex; idxPrev = idxCur, ++idxCur )
		{
			Vector2 const& prev = pos + vertices[idxPrev];
			Vector2 const& cur = pos + vertices[idxCur];
			mBuffers.push_back(prev);
			mBuffers.push_back(cur);
		}

#else
		int idxPrev = numVertex - 1;
		for( int idxCur = 0 ; idxCur < numVertex ; idxPrev = idxCur , ++idxCur )
		{
			Vector2 const& prev = pos + vertices[ idxPrev ];
			Vector2 const& cur = pos + vertices[idxCur];
			Vector2 edge = cur - prev;

			Vector2 dirCur = cur - light.pos;
			Vector2 dirPrev = prev - light.pos;

			if ( dirCur.x * edge.y - dirCur.y * edge.x < 0 )
			{
				Vector2 v1 = prev + 1000 * dirPrev;
				Vector2 v2 = cur + 1000 * dirCur;

				mBuffers.push_back(prev);
				mBuffers.push_back(v1);
				mBuffers.push_back(v2);
				mBuffers.push_back(cur);
			}
		}

#endif
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