#include "DevStage.h"

#include "GameInterface.h"
#include "TextureManager.h"

#include "EditorWidget.h"

#include "GBuffer.h"
#include "Texture.h"
#include "Renderer.h"
#include "RenderUtility.h"

#include "MenuStage.h"

#include "RHI/ShaderCore.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"

#include "RHI/OpenGLCommon.h"

class WidgetTest : public TestBase
{

	virtual bool onInit()
	{
		PropFrame* frame = new PropFrame( UI_ANY , Vec2i( 10 , 10 ) , NULL );
		val = 1;
		val2 = 2;

		frame->addProp( "a1" , val );
		frame->addProp( "a2" , val2 );
		GUISystem::Get().addWidget( frame );

		QTextButton* button;
		button = new QTextButton( UI_ANY , Vec2i( 100 , 100 ) , Vec2i(128, 64) , NULL );
		button->text->setFont( getGame()->getFont(0) );
		button->text->setString( "Start" );
		GUISystem::Get().addWidget( button );

		QChoice* chioce = new QChoice( UI_ANY , Vec2i( 200 , 200 ) , Vec2i( 200 , 30 ) , NULL );
		chioce->addItem( "Hello" );
		chioce->addItem( "Good" );
		chioce->addItem( "Test" );
		GUISystem::Get().addWidget( chioce );

		QListCtrl* listCtrl = new QListCtrl( UI_ANY , Vec2i( 500 , 200 ) , Vec2i( 100 , 200 ) , NULL );
		listCtrl->addItem( "Hello" );
		listCtrl->addItem( "Good" );
		listCtrl->addItem( "Test" );
		GUISystem::Get().addWidget( listCtrl );

		frame->inputData();
		return true;
	}

	int       val;
	float     val2;
};

class RenderTest : public TestBase
{
public:

	struct Material
	{
		float ka;
		float kd;
		float ks;
		float power;

		Material(){}
		Material( float inKa , float inKd , float inKs , float inPower )
			:ka( inKa ) , kd( inKd ) , ks( inKs ) , power( inPower ){}
	};

	struct Light
	{
		Vec2f pos;
		Vec3f color;
		float ambIntensity;
		float difIntensity;
		float speIntensity;
		float radius;
	};

	class LightingShaderProgram : public Render::ShaderProgram
	{
	public:

		void bindParameters(Render::ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(paramTexMaterial, SHADER_PARAM(texMaterial));
			parameterMap.bind(paramTexBaseColor, SHADER_PARAM(texBaseColor));
			parameterMap.bind(paramTexNormal, SHADER_PARAM(texNormal));
			parameterMap.bind(paramPos, SHADER_PARAM(gLight.pos));
			parameterMap.bind(paramColor, SHADER_PARAM(gLight.color));
			parameterMap.bind(paramRadius, SHADER_PARAM(gLight.radius));
			parameterMap.bind(paramAmbIntensity, SHADER_PARAM(gLight.ambIntensity));
			parameterMap.bind(paramDifIntensity, SHADER_PARAM(gLight.difIntensity));
			parameterMap.bind(paramSpeIntensity, SHADER_PARAM(gLight.speIntensity));
		}

		void setTextureParameters(Render::RHICommandList& commandList, GBuffer* buffer , Render::RHITexture1D& texMat )
		{
			setTexture(commandList, paramTexMaterial , texMat );
			setTexture(commandList, paramTexBaseColor , *buffer->getTexture( GBuffer::eBASE_COLOR ) );
			setTexture(commandList, paramTexNormal , *buffer->getTexture( GBuffer::eNORMAL ) );
		}

		void setLightParameters(Render::RHICommandList& commandList, Light& light )
		{
			setParam(commandList, paramPos , light.pos );
			setParam(commandList, paramColor , light.color );
			setParam(commandList, paramRadius , light.radius );
			setParam(commandList, paramAmbIntensity , light.ambIntensity );
			setParam(commandList, paramDifIntensity , light.difIntensity );
			setParam(commandList, paramSpeIntensity , light.speIntensity );
		}


		Render::ShaderParameter paramTexMaterial;
		Render::ShaderParameter paramTexBaseColor;
		Render::ShaderParameter paramTexNormal;
		Render::ShaderParameter paramPos;
		Render::ShaderParameter paramColor;
		Render::ShaderParameter paramRadius;
		Render::ShaderParameter paramAmbIntensity;
		Render::ShaderParameter paramDifIntensity;
		Render::ShaderParameter paramSpeIntensity;
	};

	class GeomShaderProgram : public Render::ShaderProgram
	{
	public:
		void bindParameters(Render::ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(paramTexDiffuse , SHADER_PARAM(texDiffuse) );
			parameterMap.bind(paramTexNormal , SHADER_PARAM(texNormal) );
			parameterMap.bind(paramTexGlow ,SHADER_PARAM(texGlow) );
			parameterMap.bind(paramMatId , SHADER_PARAM(matId) );
		}
		void setTextureParameters(Render::RHICommandList& commandList, Render::RHITexture2D* texDif , Render::RHITexture2D* texN , Render::RHITexture2D* texG )
		{
			setTexture(commandList, paramTexDiffuse , texDif ? *texDif : *Render::GBlackTexture2D );
			setTexture(commandList, paramTexNormal, texN ? *texN : *Render::GBlackTexture2D);
			setTexture(commandList, paramTexGlow, texG ? *texG : *Render::GBlackTexture2D);
		}
		void setMaterial(Render::RHICommandList& commandList, int id ){ setParam(commandList, paramMatId , MatId2TexCoord( id ) ); }

		Render::ShaderParameter paramTexDiffuse;
		Render::ShaderParameter paramTexNormal;
		Render::ShaderParameter paramTexGlow;
		Render::ShaderParameter paramMatId;
	};

	static int const MaxRegMaterialNum = 512;
	static float MatId2TexCoord( int id ){ return float( id ) / MaxRegMaterialNum; }

	void registerDefaultMaterial()
	{
		float c = 1.0;
		registerMaterial( Material( c / 2 , c / 2 , c , 5 ) );
		registerMaterial( Material( c , 0 , 0 , 0 ) );
		registerMaterial( Material( 0 , c , 0 , 0 ) );
		registerMaterial( Material( 0 , 0 , c , 5 ) );
	}
	int registerMaterial( Material const& mat )
	{
		assert( mRegMatList.size() < MaxRegMaterialNum );
		mRegMatList.push_back( mat );
		return mRegMatList.size() - 1;
	}

	inline uint32 pow2roundup ( uint32 x )
	{
		if ( x == 0 )
			return 2;

		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return x+1;
	}

	Render::RHITexture1DRef createMaterialTexture()
	{
		std::vector< float > buf(4 * MaxRegMaterialNum, 0.0f);

		float* ptr = &buf[0];
		for( MaterialVec::iterator iter = mRegMatList.begin(), itEnd = mRegMatList.end();
			iter != itEnd; ++iter )
		{
			Material& mat = *iter;
			ptr[0] = mat.ka;
			ptr[1] = mat.kd;
			ptr[2] = mat.ks;
			ptr[3] = mat.power;
			ptr += 4;
		}

		Render::RHITexture1DRef result = Render::RHICreateTexture1D(Render::Texture::eFloatRGBA, MaxRegMaterialNum, 0, Render::TCF_DefalutValue, &buf[0]);
		Render::OpenGLCast::To(result)->bind();
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S , GL_CLAMP_TO_EDGE );
		Render::OpenGLCast::To(result)->unbind();
		return result;
	}
	virtual bool onInit()
	{
		RenderSystem* renderSys = getRenderSystem();

		VERIFY_RETURN_FALSE(Render::ShaderManager::Get().loadSimple(mProgLightingGlow, "LightingGlowVS", "LightingGlowFS") );
		VERIFY_RETURN_FALSE(Render::ShaderManager::Get().loadSimple(mProgGeom, "GeomDefaultVS", "GeomDefaultFS"));
		VERIFY_RETURN_FALSE(Render::ShaderManager::Get().loadSimple(mProgLighting, "LightingPointVS", "LightingPointFS"));

		TextureManager* texMgr = getRenderSystem()->getTextureMgr();

		mTexBlock[0] = texMgr->getTexture( "Block.tga" );
		mTexBlock[1] = texMgr->getTexture( "CircleN.tga" );

		mTexBlock[0] = texMgr->getTexture( "Block.tga" );
		mTexBlock[1] = texMgr->getTexture( "SqureN.tga" );

		mTexObject[ RP_DIFFUSE ] = texMgr->getTexture("mob1Diffuse.tga");
		mTexObject[ RP_NORMAL  ] = texMgr->getTexture("mob1Normal.tga");
		//mTexObject[ RP_NORMAL  ] = texMgr->getEmptyTexture();
		mTexObject[ RP_GLOW    ] = texMgr->getTexture("mob1Glow.tga");

		Vec2i screenSize = getGame()->getScreenSize();
		mGBuffer.reset( new GBuffer );
		if ( !mGBuffer->create( screenSize.x , screenSize.y ) )
			return false;

		mLights[0].pos = Vec2f( 30 , 30 );
		mLights[0].color = Vec3f( 1 , 1 , 1 );
		mLights[0].ambIntensity = 100;
		mLights[0].difIntensity = 25;
		mLights[0].speIntensity = 50;
		mLights[0].radius = 512;

		mLights[1].pos = Vec2f( 130 , 30 );
		mLights[1].color = Vec3f( 0 ,1 , 0 );
		mLights[1].ambIntensity = 25;
		mLights[1].difIntensity = 25;
		mLights[1].speIntensity = 50;
		mLights[1].radius = 256;

		mLights[2].pos = Vec2f( 30 , 130 );
		mLights[2].color = Vec3f( 0 , 0 , 1 );
		mLights[2].ambIntensity = 30;
		mLights[2].difIntensity = 20;
		mLights[2].speIntensity = 150;
		mLights[2].radius = 256;


		registerDefaultMaterial();
		mTexMaterial = createMaterialTexture();

		return true;
	}

	virtual void setDevMsg( FString& str )
	{
		str.format( "%f %f" , mLights[0].pos.x , mLights[0].pos.y );

	}

	bool onMouse(MouseMsg const& msg)
	{
		mLights[0].pos.x = msg.getPos().x;
		mLights[0].pos.y = msg.getPos().y;
		return true; 
	}

	virtual void onUpdate( float dt )
	{
		static float theta = 0.0f;
		theta += 2 * dt;
		Vec2f dir = Vec2f( Math::Cos( theta ) , Math::Sin( theta ) );
		mLights[2].pos = Vec2f( 330 , 130 ) + 100 * dir;
		mLights[1].pos = Vec2f( 230 , 230 ) + 200 * dir;


	}

	virtual void onRender()
	{
		
		mGBuffer->bind();

		Render::RHICommandList& commandList = Render::RHICommandList::GetImmediateList();

		//glDepthMask(GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

		RHISetShaderProgram(commandList, mProgGeom.getRHIResource());
		mProgGeom.setTextureParameters(commandList, mTexBlock[RP_DIFFUSE]->resource , mTexBlock[RP_NORMAL]->resource, nullptr );

		for( int j = 0 ; j < 10 ; ++j )
		{
			for( int i = 0 ; i < 20 ; ++i )
			{
				mProgGeom.setMaterial(commandList, ( i ) % 4 );
				Vec2f pos( 10 + BLOCK_SIZE * i , 10 + BLOCK_SIZE * j );
				Vec2f size( BLOCK_SIZE , BLOCK_SIZE );
				glBegin(GL_QUADS);
				glTexCoord2f(0.0, 0.0); glVertex2f(pos.x, pos.y);
				glTexCoord2f(1.0, 0.0); glVertex2f(pos.x+size.x, pos.y);
				glTexCoord2f(1.0, 1.0); glVertex2f(pos.x+size.x, pos.y+size.y);
				glTexCoord2f(0.0, 1.0); glVertex2f(pos.x, pos.y+size.y);
				glEnd();
			}
		}
		mProgGeom.setTextureParameters(commandList, mTexObject[RP_DIFFUSE]->resource, mTexObject[RP_NORMAL]->resource, mTexObject[ RP_GLOW ]->resource);
		mProgGeom.setMaterial(commandList, ( 0 ) % 4 );

		drawSprite( Vec2f( 200 + 64 * 1 , 100 ) , Vec2f( 64 , 64 ) , 0.0f );	
		drawSprite( Vec2f( 300 + 64 * 2 , 210 ) , Vec2f( 64 , 64 ) , 0.0f );

		RHISetShaderProgram(commandList, nullptr);
		mGBuffer->unbind();


		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

		glEnable( GL_BLEND );
		glBlendFunc( GL_ONE , GL_ONE );

		Vec2i size = getGame()->getScreenSize();

		RHISetShaderProgram(commandList, mProgLightingGlow.getRHIResource());
		mProgLightingGlow.setTexture(commandList, SHADER_PARAM( texBaseColor ) , *mGBuffer->getTexture( GBuffer::eBASE_COLOR ) );
		mProgLightingGlow.setTexture(commandList, SHADER_PARAM( texGlow ) , *mGBuffer->getTexture( GBuffer::eLIGHTING ) );

		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 1.0); glVertex2f(0, 0);
		glTexCoord2f(1.0, 1.0); glVertex2f(size.x, 0);
		glTexCoord2f(1.0, 0.0); glVertex2f(size.x, size.y);
		glTexCoord2f(0.0, 0.0); glVertex2f(0, size.y);
		glEnd();

		RHISetShaderProgram(commandList, mProgLighting.getRHIResource());
		mProgLighting.setTextureParameters(commandList, mGBuffer , *mTexMaterial );

		for( int i = 0 ; i < ARRAY_SIZE( mLights ) ; ++i )
		{
			Light& light = mLights[i];

			mProgLighting.setLightParameters(commandList, light );

			Vec2f halfRange =  Vec2f( light.radius , light.radius ); 

			Vec2f minRender = light.pos - halfRange;
			Vec2f maxRender = light.pos + halfRange;

			Vec2f minTex , maxTex;
			minTex.x = minRender.x / size.x;
			maxTex.x = maxRender.x / size.x;
			minTex.y = 1 - minRender.y / size.y;
			maxTex.y = 1 - maxRender.y / size.y;

			glColor3f(1,1,1);

			glBegin(GL_QUADS);
			glTexCoord2f(minTex.x,minTex.y); glVertex2f( minRender.x , minRender.y );
			glTexCoord2f(maxTex.x,minTex.y); glVertex2f( maxRender.x , minRender.y );
			glTexCoord2f(maxTex.x,maxTex.y); glVertex2f( maxRender.x , maxRender.y );
			glTexCoord2f(minTex.x,maxTex.y); glVertex2f( minRender.x , maxRender.y );
			glEnd();	

		}
		RHISetShaderProgram(commandList, nullptr);

		glDisable( GL_BLEND );


#if 0
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable( GL_TEXTURE_2D );

		{
			Vec2i pos( 50 , 50 );
			Vec2i size( 200 , 100 );
			glBindTexture( GL_TEXTURE_2D ,  Render::OpenGLCast::GetHandle( mGBuffer->getTexture( GBuffer::eBASE_COLOR) ) );
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); glVertex2f(pos.x, pos.y);
			glTexCoord2f(1.0, 0.0); glVertex2f(pos.x+size.x, pos.y);
			glTexCoord2f(1.0, 1.0); glVertex2f(pos.x+size.x, pos.y+size.y);
			glTexCoord2f(0.0, 1.0); glVertex2f(pos.x, pos.y+size.y);
			glEnd();
		}


		{
			Vec2i pos( 50 + 200 , 50 );
			Vec2i size( 200 , 100 );
			glEnable( GL_TEXTURE_2D );
			glBindTexture( GL_TEXTURE_2D , Render::OpenGLCast::GetHandle( mGBuffer->getTexture( GBuffer::eNORMAL ) ) );
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0); glVertex2f(pos.x, pos.y);
			glTexCoord2f(1.0, 0.0); glVertex2f(pos.x+size.x, pos.y);
			glTexCoord2f(1.0, 1.0); glVertex2f(pos.x+size.x, pos.y+size.y);
			glTexCoord2f(0.0, 1.0); glVertex2f(pos.x, pos.y+size.y);
			glEnd();
		}
#endif



	}


	typedef std::vector< Material > MaterialVec;
	MaterialVec mRegMatList;

	Texture*  mTexBlock[3];
	Texture*  mTexObject[3];
	Render::RHITexture1DRef mTexMaterial;
	Light     mLights[ 3 ];

	FPtr< GBuffer >  mGBuffer;
	Render::ShaderProgram  mProgLightingGlow;
	GeomShaderProgram      mProgGeom;
	LightingShaderProgram  mProgLighting;
};

DevStage::DevStage()
{
	mTest.reset( new RenderTest );
}

bool DevStage::onInit()
{
	mTexCursor = getRenderSystem()->getTextureMgr()->getTexture("cursor.tga");
	mDevMsg.reset( IText::create( getGame()->getFont( 0 ) , 18 , Color4ub(50,255,50) ) );

	GUISystem::Get().cleanupWidget();

	if ( !mTest->onInit() )
		return false;

	return true;
}

void DevStage::onExit()
{
	mDevMsg.clear();
}

void DevStage::onUpdate(float deltaT)
{
	getGame()->procSystemEvent();
	mTest->onUpdate( deltaT );
}

void DevStage::onRender()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	mTest->onRender();

	GUISystem::Get().render();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	drawSprite( Vec2f( getGame()->getMousePos() ) - Vec2f(16,16) ,Vec2f(32,32), mTexCursor );
	glDisable(GL_BLEND);

	FString str;
	mTest->setDevMsg( str );
	mDevMsg->setString( str );
	getRenderSystem()->drawText( mDevMsg , Vec2f( 5 ,5 ) , TEXT_SIDE_LEFT | TEXT_SIDE_TOP );
}

bool DevStage::onKey(unsigned key , bool isDown)
{
	if ( !isDown )
		return false;

	switch( key )
	{
	case Keyboard::eESCAPE:
		getGame()->addStage( new MenuStage , true );
		break;
	}
	return false;
}

bool DevStage::onMouse(MouseMsg const& msg)
{
	return mTest->onMouse( msg );
}

DevStage::~DevStage()
{

}
