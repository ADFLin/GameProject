#include "Lighting2DStage.h"

#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"
#include "RenderUtility.h"

#include "RHI/RHIGraphics2D.h"
#include "RHI/ShaderManager.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHICommand.h"

namespace Render
{
	REGISTER_STAGE("2D Lighting Test", Lighting2DTestStage, EStageGroup::GraphicsTest);

	bool Lighting2DTestStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		::Global::GUI().cleanupWidget();
		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_RUN_BENCHMARK, "Run Benchmark");

		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "bShowShadowRender"), bShowShadowRender);
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY , "bUseGeometryShader"), bUseGeometryShader);

		restart();

		return true;
	}

	bool Lighting2DTestStage::setupRenderSystem(ERenderSystem systemName)
	{
		VERIFY_RETURN_FALSE(BaseClass::setupRenderSystem(systemName));
		{
			ShaderEntryInfo entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) } ,
				{ EShader::Pixel , SHADER_ENTRY(LightingPS) } ,
			};
			VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(
				mProgLighting, "Shader/Game/lighting2D", entries));
		}

		{
			FixString< 128 > defineStr;
			ShaderCompileOption option;
			ShaderEntryInfo entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Geometry , SHADER_ENTRY(MainGS) },
				{ EShader::Pixel , SHADER_ENTRY(MainPS) } ,
			};
			VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(
				mProgShadow, "Shader/Game/Lighting2DShadow", entries, option))
		}

		return true;
	}

	void Lighting2DTestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		mProgLighting.releaseRHI();
		mProgShadow.releaseRHI();
		BaseClass::preShutdownRenderSystem(bReInit);
	}


	void Lighting2DTestStage::restart()
	{
		lights.clear();
		blocks.clear();
	}

	void Lighting2DTestStage::onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	bool Lighting2DTestStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch( id )
		{
		case UI_RUN_BENCHMARK:
			{
				int lightNum = 100;
				int blockNum = 5000;

				Vec2i screenSize = ::Global::GetScreenSize();
				int w = screenSize.x;
				int h = screenSize.y;

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

	void Lighting2DTestStage::onRender(float dFrame)
	{

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		Vec2i screenSize = ::Global::GetScreenSize();

		Matrix4 projectMatrix = OrthoMatrix(0, screenSize.x, 0, screenSize.y, 1, -1);

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1);
	
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None > ::GetRHI());
		RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(projectMatrix));

		int w = screenSize.x;
		int h = screenSize.y;

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
			if (bShowShadowRender)
			{
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA , EBlend::One , EBlend::One , EBlend::Add >::GetRHI());
			}
			else
			{
				RHISetDepthStencilState(commandList,
					TStaticDepthStencilState<
					true, ECompareFunc::Always,
					true, ECompareFunc::Always,
					EStencil::Keep, EStencil::Keep, EStencil::Replace, 0x1 >::GetRHI(), 0x1);

				RHISetBlendState(commandList, TStaticBlendState< CWM_None >::GetRHI());
			}

			{

				if (bUseGeometryShader)
				{
					RHISetShaderProgram(commandList, mProgShadow.getRHIResource());
					mProgShadow.setParameters(commandList, light.pos, ::Global::GetScreenSize());

					if (!mBuffers.empty())
					{
						TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &mBuffers[0], mBuffers.size());
					}
				}
				else
				{				
					mBuffers.clear();
					for (Block& block : blocks)
					{
						renderPolyShadow(light, block.pos, block.getVertices(), block.getVertexNum());
					}

					RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(projectMatrix));
					if (!mBuffers.empty())
					{
						TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::Quad, &mBuffers[0], mBuffers.size());
					}
				}
			}
			
			if ( !bShowShadowRender )
			{
#if 1
				RHISetDepthStencilState(commandList,
					TStaticDepthStencilState<
					true, ECompareFunc::Always,
					true, ECompareFunc::Equal,
					EStencil::Keep, EStencil::Keep, EStencil::Keep, 0x1
					>::GetRHI(), 0x0);
#else

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
#endif
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None > ::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
				{
					RHISetShaderProgram(commandList, mProgLighting.getRHIResource());
					mProgLighting.setParameters(commandList, light.pos, light.color);
					DrawUtility::ScreenRect(commandList, w, h);
				}

				RHIClearRenderTargets(commandList, EClearBits::Stencil, nullptr, 0, 1.0, 0);
			}
		}

		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

#if 1
		RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(projectMatrix));
		Vector2 vertices[] =
		{
			Vector2(100 , 100),
			Vector2(200 , 100),
			Vector2(200 , 200),
			Vector2(100 , 200),
		};
		uint32 indices[] =
		{
			0,1,2,0,2,3,
		};
		TRenderRT<RTVF_XY>::DrawIndexed(commandList, EPrimitive::TriangleList, vertices, 4, indices, 6 , LinearColor(1,0,0,1));
#endif

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

		g.beginRender();

		RenderUtility::SetFont( g , FONT_S8 );
		FixString< 256 > str;
		Vec2i pos = Vec2i( 10 , 10 );
		str.format("Lights Num = %u", lights.size());
		g.drawText( pos , str );

		g.endRender();
	}

	void Lighting2DTestStage::renderPolyShadow( Light const& light , Vector2 const& pos , Vector2 const* vertices , int numVertex  )
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

	bool Lighting2DTestStage::onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

		Vec2i screenSize = ::Global::GetScreenSize();
		Vector2 worldPos = Vector2(msg.getPos().x, screenSize.y - msg.getPos().y);
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


