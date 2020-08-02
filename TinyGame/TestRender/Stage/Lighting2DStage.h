#ifndef LightGLStage_h__
#define LightGLStage_h__

#include "Stage/TestRenderStageBase.h"
#include "StageBase.h"

#include "DataStructure/Grid2D.h"
#include "CppVersion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"


#include "RHI/ShaderProgram.h"

#define SHADOW_USE_GEOMETRY_SHADER 1


namespace Render
{
	typedef Math::Vector2 Vector2;
	typedef Math::Vector3 Color;

	struct Light
	{
		Vector2 pos;
		Color color;
	};

	struct Block
	{
		Vector2 pos;
		std::vector< Vector2 > vertices;
		int          getVertexNum() const { return (int)vertices.size(); }
		Vector2 const& getVertex( int idx ) const { return vertices[ idx ]; }
		Vector2 const* getVertices() const { return &vertices[0]; }
		void setBox( Vector2 const& pos , Vector2 const& size )
		{
			this->pos = pos;
			float wHalf = size.x / 2;
			float hHalf = size.y / 2;
			vertices.push_back( Vector2( wHalf , hHalf ) );
			vertices.push_back( Vector2( -wHalf , hHalf ) );
			vertices.push_back( Vector2( -wHalf , -hHalf ) );
			vertices.push_back( Vector2( wHalf , -hHalf ) );
		}

	};

	class LightingProgram : public ShaderProgram
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LightLocation);
			BIND_SHADER_PARAM(parameterMap, LightColor);
			BIND_SHADER_PARAM(parameterMap, LightAttenuation);
		}

		void setParameters(RHICommandList& commandList, Vector2 const& lightPos , Color const& lightColor )
		{
			SET_SHADER_PARAM(commandList, *this, LightLocation, lightPos);
			SET_SHADER_PARAM(commandList, *this, LightColor, lightColor);
			SET_SHADER_PARAM(commandList, *this, LightAttenuation, Vector3(0.0, 1 / 5.0, 0.0));
		}

		DEFINE_SHADER_PARAM(LightLocation);
		DEFINE_SHADER_PARAM(LightColor);
		DEFINE_SHADER_PARAM(LightAttenuation);
	};


	class LightingShadowProgram : public ShaderProgram
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LightLocation);
			BIND_SHADER_PARAM(parameterMap, ScreenSize);
		}

		void setParameters(RHICommandList& commandList, Vector2 const& lightPos, Vector2 const& screenSize)
		{
			SET_SHADER_PARAM(commandList, *this, LightLocation, lightPos);
			SET_SHADER_PARAM(commandList, *this, ScreenSize, screenSize);
		}

		DEFINE_SHADER_PARAM(LightLocation);
		DEFINE_SHADER_PARAM(ScreenSize);
	};


	class Lighting2DTestStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;

		using LightList = std::vector< Light >;
		using BlockList = std::vector< Block >;
	public:

		enum
		{
			UI_RUN_BENCHMARK = BaseClass::NEXT_UI_ID ,

			NEXT_UI_ID,
		};
		LightList lights;
		BlockList blocks;

		LightingProgram mProgLighting;
		LightingShadowProgram mProgShadow;
		Lighting2DTestStage(){}

		std::vector< Vector2 > mBuffers;

		bool bShowShadowRender = false;
		bool bUseGeometryShader = true;

		virtual ERenderSystem getDefaultRenderSystem() { return ERenderSystem::OpenGL; }


		bool onInit() override;
		bool initializeRHIResource() override;
		void releaseRHIResource(bool bReInit = false) override;

		void onUpdate( long time ) override;
		bool onWidgetEvent(int event, int id, GWidget* ui) override;
		void onRender( float dFrame ) override;


		void renderPolyShadow( Light const& light , Vector2 const& pos , Vector2 const* vertices , int numVertex );

		void restart();


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg ) override;

		bool onKey(KeyMsg const& msg) override
		{
			if ( msg.isDown() )
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}

			return BaseClass::onKey(msg);
		}



	protected:

	};



}



#endif // LightGLStage_h__
