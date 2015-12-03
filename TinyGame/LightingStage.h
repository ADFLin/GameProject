#ifndef LightGLStage_h__
#define LightGLStage_h__


#include "StageBase.h"
#include "TVector2.h"
#include "TVector3.h"
#include "GL/glew.h"
#include "WinGLPlatform.h"
#include "TGrid2D.h"
#include "CppVersion.h"
#include "GLCommon.h"

namespace Lighting
{

	typedef TVector2< float > Vec2f;
	typedef TVector3< float > Color;

	struct Light
	{
		Vec2f pos;
		Color color;
	};

	struct Block
	{
		Vec2f pos;
		std::vector< Vec2f > vertices;
		int          getVertexNum() const { return (int)vertices.size(); }
		Vec2f const& getVertex( int idx ) const { return vertices[ idx ]; }
		Vec2f const* getVertices() const { return &vertices[0]; }
		void setBox( Vec2f const& pos , Vec2f const& size )
		{
			this->pos = pos;
			float wHalf = size.x / 2;
			float hHalf = size.y / 2;
			vertices.push_back( Vec2f( wHalf , hHalf ) );
			vertices.push_back( Vec2f( -wHalf , hHalf ) );
			vertices.push_back( Vec2f( -wHalf , -hHalf ) );
			vertices.push_back( Vec2f( wHalf , -hHalf ) );
		}

	};


	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;

		typedef std::vector< Light > LightList;
		typedef std::vector< Block > BlockList;
	public:

		LightList lights;
		BlockList blocks;

		GLuint program;
		GLuint fragShader;
		GLuint loc_lightLocation;
		GLuint loc_lightColor;
		GLuint loc_lightAttenuation;


		TestStage(){}

		virtual bool onInit();
		virtual void onEnd();

		char* readFile(char const* file);
		virtual void onUpdate( long time );

		void onRender( float dFrame );

		void renderPolyShadow( Light const& light , Vec2f const& pos , Vec2f const* vertices , int numVertex );

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
			case 'R': restart(); break;
			}
			return false;
		}
	protected:

	};



}



#endif // LightGLStage_h__
