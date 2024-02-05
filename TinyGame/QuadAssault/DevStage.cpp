#include "DevStage.h"

#include "GameInterface.h"
#include "TextureManager.h"

#include "EditorWidget.h"

#include "Texture.h"
#include "Renderer.h"
#include "RenderUtility.h"

#include "MenuStage.h"

#include "RHI/ShaderCore.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"

#include "RHI/RHIGlobalResource.h"
#include "RHI/RHIGraphics2D.h"

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

		void bindParameters(Render::ShaderParameterMap const& parameterMap)
		{
			paramTexMaterial.bind(parameterMap, SHADER_PARAM(texMaterial));
			paramTexBaseColor.bind(parameterMap, SHADER_PARAM(texBaseColor));
			paramTexNormal.bind(parameterMap, SHADER_PARAM(texNormal));
			paramPos.bind(parameterMap, SHADER_PARAM(gLight.pos));
			paramColor.bind(parameterMap, SHADER_PARAM(gLight.color));
			paramRadius.bind(parameterMap, SHADER_PARAM(gLight.radius));
			paramAmbIntensity.bind(parameterMap, SHADER_PARAM(gLight.ambIntensity));
			paramDifIntensity.bind(parameterMap, SHADER_PARAM(gLight.difIntensity));
			paramSpeIntensity.bind(parameterMap, SHADER_PARAM(gLight.speIntensity));
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
		void bindParameters(Render::ShaderParameterMap const& parameterMap)
		{
			paramTexDiffuse.bind(parameterMap, SHADER_PARAM(texDiffuse) );
			paramTexNormal.bind(parameterMap, SHADER_PARAM(texNormal) );
			paramTexGlow.bind(parameterMap,SHADER_PARAM(texGlow) );
			paramMatId.bind(parameterMap, SHADER_PARAM(matId) );
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

		Render::RHITexture1DRef result = Render::RHICreateTexture1D(Render::ETexture::FloatRGBA, MaxRegMaterialNum, 0, 1, Render::TCF_DefalutValue, &buf[0]);
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

		mTexObject[ TG_DIFFUSE ] = texMgr->getTexture("mob1Diffuse.tga");
		mTexObject[ TG_NORMAL  ] = texMgr->getTexture("mob1Normal.tga");
		//mTexObject[ TG_NORMAL  ] = texMgr->getEmptyTexture();
		mTexObject[ TG_GLOW    ] = texMgr->getTexture("mob1Glow.tga");

		Vec2i screenSize = getGame()->getScreenSize();

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

	MsgReply onMouse(MouseMsg const& msg)
	{
		mLights[0].pos.x = msg.getPos().x;
		mLights[0].pos.y = msg.getPos().y;

		return MsgReply::Unhandled(); 
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
		

	}


	typedef std::vector< Material > MaterialVec;
	MaterialVec mRegMatList;

	Texture*  mTexBlock[3];
	Texture*  mTexObject[3];
	Render::RHITexture1DRef mTexMaterial;
	Light     mLights[ 3 ];

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
	mDevMsg.reset( IText::Create( getGame()->getFont( 0 ) , 18 , Color4ub(50,255,50) ) );

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
	mTest->onUpdate( deltaT );
}

void DevStage::onRender()
{
	using namespace Render;

	RHICommandList& commandList = RHICommandList::GetImmediateList();

	RHISetFrameBuffer(commandList, nullptr);
	RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0, 0, 0, 1), 1);

	RHIGraphics2D& g = IGame::Get().getGraphics2D();

	g.beginRender();
	mTest->onRender();

	GUISystem::Get().render();

	g.beginBlend(1, ESimpleBlendMode::Add);
	{
		Vec2f size = Vec2f(32, 32);
		g.setBrush(Color3f(1, 1, 1));
		g.drawTexture(*mTexCursor->resource, Vec2f(getGame()->getMousePos()) - size / 2, size);
	}
	g.endBlend();

	FString str;
	mTest->setDevMsg( str );
	mDevMsg->setString( str );
	getRenderSystem()->drawText( mDevMsg , Vec2f( 5 ,5 ) , TEXT_SIDE_LEFT | TEXT_SIDE_TOP );
	g.endRender();
}

MsgReply DevStage::onKey(KeyMsg const& msg)
{
	if (msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::Escape:
			getGame()->addStage(new MenuStage, true);
			break;
		}
	}
	return BaseClass::onKey(msg);
}

MsgReply DevStage::onMouse(MouseMsg const& msg)
{
	return mTest->onMouse( msg );
}

DevStage::~DevStage()
{

}
