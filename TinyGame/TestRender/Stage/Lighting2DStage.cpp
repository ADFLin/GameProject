#include "Lighting2DStage.h"

#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"
#include "RenderUtility.h"

#include "GLGraphics2D.h"

#include "RHI/ShaderManager.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHICommand.h"


#include "GL/wglew.h"

namespace Lighting2D
{
	bool TestStage::onInit()
	{
		if ( !Global::GetDrawEngine().startOpenGL() )
			return false;

		wglSwapIntervalEXT(0);

		GameWindow& window = Global::GetDrawEngine().getWindow();

		{
			ShaderEntryInfo entries[] =
			{
				{ Shader::ePixel , SHADER_ENTRY(LightingPS) } ,
			};
			if( !ShaderManager::Get().loadFile(
				mProgLighting, "Shader/Game/lighting2D", entries ) )
				return false;
		}

		{
			FixString< 128 > defineStr;
			ShaderCompileOption option;
			ShaderEntryInfo entries[] =
			{ 
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel , SHADER_ENTRY(MainPS) } ,
				{ Shader::eGeometry , SHADER_ENTRY(MainGS) },
			};
			if( !ShaderManager::Get().loadFile(
				mProgShadow, "Shader/Game/Lighting2DShadow", entries, option ) )
				return false;
		}

		::Global::GUI().cleanupWidget();
		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_RUN_BENCHMARK, "Run Benchmark");

		WidgetPropery::Bind(frame->addCheckBox(UI_ANY , "bUseGeometryShader"), bUseGeometryShader);

		restart();

		return true;
	}

	void TestStage::onInitFail()
	{
		Global::GetDrawEngine().stopOpenGL(true);
	}

	void TestStage::onEnd()
	{
		Global::GetDrawEngine().stopOpenGL(true);
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
				int blockNum = 5000;
				GameWindow& window = Global::GetDrawEngine().getWindow();

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
					block.setBox(pos, Vector2(10, 10));
					blocks.push_back(std::move(block));

				}
			}
			return false;


		}
		return BaseClass::onWidgetEvent(event, id, ui);
	}

	void TestStage::onRender(float dFrame)
	{
		GameWindow& window = Global::GetDrawEngine().getWindow();

		RHICommandList& commandList = RHICommandList::GetImmediateList();


		Matrix4 projectMatrix = OrthoMatrix(0, window.getWidth(), 0, window.getHeight(), 1, -1);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(AdjProjectionMatrixForRHI(projectMatrix));
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None > ::GetRHI());
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

		int w = window.getWidth();
		int h = window.getHeight();

		if (bUseGeometryShader)
		{
			mBuffers.clear();
			for (Block& block : blocks)
			{
				renderPolyShadow(Light(), block.pos, block.getVertices(), block.getVertexNum());
			}
		}

		for ( Light const& light : lights )
		{

			RHISetDepthStencilState(commandList,
				TStaticDepthStencilState<
					true , ECompareFun::Always ,
					true , ECompareFun::Always , 
					Stencil::eKeep , Stencil::eKeep , Stencil::eReplace ,0x1 
				>::GetRHI(), 0x1 );
			
			RHISetBlendState(commandList, TStaticBlendState< CWM_None >::GetRHI());

			{

				if (bUseGeometryShader)
				{
					RHISetShaderProgram(commandList, mProgShadow.getRHIResource());
					mProgShadow.setParameters(commandList, light.pos);
				}
				else
				{
					mBuffers.clear();
					for (Block& block : blocks)
					{
						renderPolyShadow(light, block.pos, block.getVertices(), block.getVertexNum());
					}
				}

				if (!mBuffers.empty())
				{

					if (bUseGeometryShader)
					{
						TRenderRT< RTVF_XY >::Draw(commandList, PrimitiveType::LineList, &mBuffers[0], mBuffers.size());
					}
					else
					{
						TRenderRT< RTVF_XY >::Draw(commandList, PrimitiveType::Quad, &mBuffers[0], mBuffers.size());
					}
					
				}
			}
			
			RHISetDepthStencilState(commandList,
				TStaticDepthStencilState<
					true, ECompareFun::Always,
					true, ECompareFun::Equal, 
					Stencil::eKeep, Stencil::eKeep, Stencil::eKeep,0x1 
				>::GetRHI(), 0x0);

			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
			{
				RHISetShaderProgram(commandList, mProgLighting.getRHIResource());
				mProgLighting.setParameters(commandList, light.pos, light.color);
				DrawUtility::Rect(commandList, w, h);
			}
			
			

			glClear(GL_STENCIL_BUFFER_BIT);
		}

		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

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

		GLGraphics2D& g = ::Global::GetDrawEngine().getRHIGraphics();

		g.beginRender();

		RenderUtility::SetFont( g , FONT_S8 );
		FixString< 256 > str;
		Vec2i pos = Vec2i( 10 , 10 );
		g.drawText( pos , str.format( "Lights Num = %u" , lights.size() ) );

		g.endRender();
	}

	void TestStage::renderPolyShadow( Light const& light , Vector2 const& pos , Vector2 const* vertices , int numVertex  )
	{

		if (bUseGeometryShader)
		{
			int idxPrev = numVertex - 1;
			for (int idxCur = 0; idxCur < numVertex; idxPrev = idxCur, ++idxCur)
			{
				Vector2 const& prev = pos + vertices[idxPrev];
				Vector2 const& cur = pos + vertices[idxCur];
				mBuffers.push_back(prev);
				mBuffers.push_back(cur);
			}
		}
		else
		{
			int idxPrev = numVertex - 1;
			for (int idxCur = 0; idxCur < numVertex; idxPrev = idxCur, ++idxCur)
			{
				Vector2 const& prev = pos + vertices[idxPrev];
				Vector2 const& cur = pos + vertices[idxCur];
				Vector2 edge = cur - prev;

				Vector2 dirCur = cur - light.pos;
				Vector2 dirPrev = prev - light.pos;

				if (dirCur.x * edge.y - dirCur.y * edge.x < 0)
				{
					Vector2 v1 = prev + 1000 * dirPrev;
					Vector2 v2 = cur + 1000 * dirCur;

					mBuffers.push_back(prev);
					mBuffers.push_back(v1);
					mBuffers.push_back(v2);
					mBuffers.push_back(cur);
				}
			}
		}

	}

	bool TestStage::onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

		GameWindow& window = Global::GetDrawEngine().getWindow();

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


REGISTER_STAGE("2D Lighting Test", Lighting2D::TestStage, EStageGroup::GraphicsTest);