#ifndef LightGLStage_h__
#define LightGLStage_h__


#include "StageBase.h"
#include "GL/glew.h"
#include "WinGLPlatform.h"
#include "DataStructure/Grid2D.h"
#include "CppVersion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"


#include "RHI/ShaderProgram.h"

#define SHADOW_USE_GEOMETRY_SHADER 1


namespace Lighting2D
{
	using namespace Render;

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
			mParamLightLocation.bind(parameterMap, SHADER_PARAM(LightLocation));
			mParamLightColor.bind(parameterMap, SHADER_PARAM(LightColor));
			mParamLightAttenuation.bind(parameterMap, SHADER_PARAM(LightAttenuation));
		}

		void setParameters(RHICommandList& commandList, Vector2 const& lightPos , Color const& lightColor )
		{
			setParam(commandList, mParamLightLocation, lightPos);
			setParam(commandList, mParamLightColor, lightColor);
			setParam(commandList, mParamLightAttenuation, Vector3( 0.0, 1 / 5.0, 0.0 ));
		}
		ShaderParameter mParamLightLocation;
		ShaderParameter mParamLightColor;
		ShaderParameter mParamLightAttenuation;
	};


	class LightingShadowProgram : public ShaderProgram
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamLightLocation.bind(parameterMap, SHADER_PARAM(LightLocation));
		}

		void setParameters(RHICommandList& commandList, Vector2 const& lightPos)
		{
			setParam(commandList, mParamLightLocation, lightPos);
		}

		ShaderParameter mParamLightLocation;
	};


	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;

		typedef std::vector< Light > LightList;
		typedef std::vector< Block > BlockList;
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
		TestStage(){}

		std::vector< Vector2 > mBuffers;

		virtual bool onInit();
		virtual void onInitFail() override;
		virtual void onEnd();
		virtual void onUpdate( long time );

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override;
		void onRender( float dFrame );


		void renderPolyShadow( Light const& light , Vector2 const& pos , Vector2 const* vertices , int numVertex );

		void restart();


		void tick()
		{

		}

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg );

		bool onKey(KeyMsg const& msg)
		{
			if ( !msg.isDown() )
				return false;

			switch(msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
			return false;
		}



	protected:

	};



}



#endif // LightGLStage_h__
