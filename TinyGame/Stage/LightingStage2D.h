#ifndef LightGLStage_h__
#define LightGLStage_h__


#include "StageBase.h"
#include "GL/glew.h"
#include "WinGLPlatform.h"
#include "TGrid2D.h"
#include "CppVersion.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "RenderGL/GLCommon.h"

namespace Lighting2D
{
	using namespace RenderGL;

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
		void bindParameters()
		{
			mParamLightLocation.bind(*this, SHADER_PARAM(LightLocation));
			mParamLightColor.bind(*this, SHADER_PARAM(LightColor));
			mParamLightAttenuation.bind(*this, SHADER_PARAM(LightAttenuation));
		}

		void setParameters(Vector2 const& lightPos , Color const& lightColor )
		{
			setParam(mParamLightLocation, lightPos);
			setParam(mParamLightColor, lightColor);
			setParam(mParamLightAttenuation, 0, 1 / 5.0, 0);
		}
		ShaderParameter mParamLightLocation;
		ShaderParameter mParamLightColor;
		ShaderParameter mParamLightAttenuation;
	};

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;

		typedef std::vector< Light > LightList;
		typedef std::vector< Block > BlockList;
	public:

		LightList lights;
		BlockList blocks;

		LightingProgram mProgram;

		TestStage(){}

		virtual bool onInit();
		virtual void onEnd();

		virtual void onUpdate( long time );

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

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}
	protected:

	};



}



#endif // LightGLStage_h__
