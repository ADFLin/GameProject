#include "LevelStage.h"

#include "GameInterface.h"
#include "GameInput.h"
#include "SoundManager.h"
#include "GUISystem.h"
#include "RenderSystem.h"
#include "TextureManager.h"
#include "Texture.h"

#include "MenuStage.h"
#include "LevelEditStage.h"

#include "SurvivalMode.h"

#include "GlobalVariable.h"
#include "DataPath.h"
#include "RenderUtility.h"

#include "ObjectRenderer.h"
#include "Block.h"

#include "Player.h"
#include "Explosion.h"
#include "Mob.h"
#include "LightObject.h"
#include "Trigger.h"
#include "Message.h"

#include "KeyPickup.h"
#include "WeaponPickup.h"


#include "LaserMob.h"
#include "PlasmaMob.h"
#include "MinigunMob.h"

#include "Laser.h"
#include "Plasma.h"
#include "Minigun.h"

#include "EasingFunction.h"
#include "InlineString.h"


#include "GameInterface.h"
#include "SoundManager.h"

//
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"

#include <fstream>
#include <sstream>





void WorldData::build()
{

	Block::Initialize();
	IRenderer::Initialize();

	mObjectCreator = new ObjectCreator;
	mActionCreator = new ActionCreator;

	reigsterObject();

	mLevel = new Level;
	mLevel->init( *mObjectCreator );

	mCamera = new Object();
	mCamera->setPos(Vec2f(0,0));
	mWorldScaleFactor = 1.0f;
}

void WorldData::cleanup()
{
	IRenderer::cleanup();
	Block::Cleanup();
}

void WorldData::reigsterObject()
{
	mObjectCreator->registerClass< LaserMob >();
	mObjectCreator->registerClass< MinigunMob >();
	mObjectCreator->registerClass< PlasmaMob >();
	mObjectCreator->registerClass< KeyPickup >();
	mObjectCreator->registerClass< WeaponPickup >();
	mObjectCreator->registerClass< LightObject >();
	mObjectCreator->registerClass< AreaTrigger >();
	mObjectCreator->registerClass< PlayerStart >();

#define REG_ACTION( ACTION )\
	mActionCreator->registerClass< ACTION >( ACTION::StaticName() )

	REG_ACTION( GoalAct );
	REG_ACTION( PlaySoundAct );
	REG_ACTION( MessageAct );
	REG_ACTION( SpawnAct );

#undef REG_ACTION

}

LevelStageBase::~LevelStageBase()
{

}

bool LevelStageBase::onInit()
{
	mPause    = false;
	mTexCursor = getRenderSystem()->getTextureMgr()->getTexture("cursor.tga");

	mDevMsg.reset( IText::Create( getGame()->getFont( 0 ) , 18 , Color4ub(50,255,50) ) );
	return true;
}

void LevelStageBase::onExit()
{
	mDevMsg.clear();
}

void LevelStageBase::onWidgetEvent( int event , int id , QWidget* sender )
{

}


MsgReply LevelStageBase::onMouse( MouseMsg const& msg )
{
	return BaseClass::onMouse(msg);
}

MsgReply LevelStageBase::onKey(KeyMsg const& msg)
{
	if (msg.isDown())
	{
		switch (msg.getCode())
		{
		case EKeyCode::F2:
			break;
		case EKeyCode::Escape:
			GUISystem::Get().findTopWidget(UI_MENU_PANEL)->show(true);
			mPause = true;
			break;
		}
	}
	return BaseClass::onKey(msg);
}

bool LevelStage::onInit()
{
	renderLoading();

	if ( !BaseClass::onInit() )
		return false;

	int screenWidth   = getGame()->getScreenSize().x;
	int screenHeight  = getGame()->getScreenSize().y;

	getRenderSystem()->getTextureMgr()->loadTexture("backgroundUniverse.tga");

	GUISystem::Get().cleanupWidget();

	{
		Vec2i panelSize( 150 , 200 );

		Vec2i panelPos;
		panelPos.x = ( screenWidth - panelSize.x ) / 2;
		panelPos.y = ( screenHeight - panelSize.y ) / 2;
		Vec2i buttonSize( 128 , 32 );
		QPanel* panel = new QPanel( UI_MENU_PANEL , panelPos , panelSize , NULL );
		panel->show( false );

		GUISystem::Get().addWidget( panel );

		IFont* font = getGame()->getFont( 0 );

		Vec2i pos;
		pos.x = ( panelSize.x - buttonSize.x ) / 2;
		pos.y = 10;
		QTextButton* button;
		button = new QTextButton( UI_BACK_GAME , pos , buttonSize , panel );
		button->text->setFont( font );
		button->text->setCharSize( 24 );
		button->text->setString( "Back" );

		pos.y += 40;
		button = new QTextButton( UI_EXIT_GAME , pos , buttonSize , panel );
		button->text->setFont( font );
		button->text->setCharSize( 24 );
		button->text->setString( "Exit Game" );

		pos.y += 40;
		button = new QTextButton( UI_GO_MENU , pos , buttonSize , panel );
		button->text->setFont( font );
		button->text->setCharSize( 24 );
		button->text->setString( "Menu" );

	}

	WorldData::build();
	mLevel->addListerner( *this );

	unsigned flag = getLevel()->setSpwanDestroyFlag( SDF_LOAD_LEVEL );
	loadLevel();
	getLevel()->setSpwanDestroyFlag( flag );


	Player* player = mLevel->getPlayer();
	//player->addWeapon(new Plasma());
	player->addWeapon(new Laser());
	player->addWeapon(new Laser());
	//player->addWeapon(new Plasma());
	//player->addWeapon(new Minigun());
	//player->addWeapon(new Minigun());

#if 0
	for (int i = 0; i < 20; ++i)
	{
		mLevel->spawnObjectByName("Mob.Laser", Vec2f(300 + i * 100, 1000));
		mLevel->spawnObjectByName("Mob.Minigun", Vec2f(300 + i * 100, 1200));
	}
#endif

#if 0
	Message* gameOverMsg = new Message();
	gameOverMsg->init("Base", "Level Load", 4, "blip.wav");
	getLevel()->addMessage(gameOverMsg);
#endif


	mScreenFade.setColor( 0 );
	mScreenFade.fadeIn();

	mGameOverTimer = 3;
	mTickTimer     = 0.0f;

	if (mMode)
	{
		mMode->beginPlay();
	}


	return true;
}


void LevelStage::onExit()
{
	if (mMode)
	{
		mMode->endPlay();
		mMode.release();
	}

	mLevel->cleanup();

	if (mMusic)
	{
		mMusic->stop();
		mMusic.release();
	}
	delete mLevel;
	delete mCamera;

}


void LevelStage::onUpdate(float deltaT)
{	
	mTickTimer += deltaT;
	int numFrame = mTickTimer / TICK_TIME;

	if ( numFrame )
	{
		mTickTimer -= numFrame * TICK_TIME;
		int nFrame = 0;
		for ( ; nFrame < numFrame ; ++nFrame )
		{
			if ( mPause )
				break;

			tick();
		}
		updateRender( nFrame * TICK_TIME );
	}
	else
	{

	}

	mScreenFade.update( deltaT );
}

void LevelStage::changeMenuStage()
{
	MenuStage* stage;
	if(  mLevel->getState() == Level::eFINISH || mLevel->getPlayer()->isDead() )
		stage = new MenuStage( MenuStage::MS_SELECT_LEVEL );
	else
		stage = new MenuStage();

	getGame()->addStage( stage , true );
}

void LevelStage::tick()
{
	if (mMode)
	{
		mMode->tick(TICK_TIME);
	}

	Vec2f wPosMouse = convertToWorldPos( getGame()->getMousePos() );

	Player* player = mLevel->getPlayer();

	float rotateSpeed = Math::DegToRad( 150 );
	float moveAcc = 1;
	if(Input::isKeyPressed(EKeyCode::Left) || Input::isKeyPressed(EKeyCode::A))
		player->rotate(-rotateSpeed*TICK_TIME);
	if(Input::isKeyPressed(EKeyCode::Right) || Input::isKeyPressed(EKeyCode::D))
		player->rotate( rotateSpeed*TICK_TIME);
	if(Input::isKeyPressed(EKeyCode::Up) || Input::isKeyPressed(EKeyCode::W))
		player->addMoment( moveAcc);
	if(Input::isKeyPressed(EKeyCode::Down) || Input::isKeyPressed(EKeyCode::S))
		player->addMoment(-moveAcc);
	if(Platform::IsButtonPressed( EMouseKey::Left ) )
		player->shoot( wPosMouse );

	player->update( wPosMouse );

	mLevel->tick();

	if( mLevel->getState() == Level::eFINISH || player->isDead() )
		mGameOverTimer -= TICK_TIME;

	if( mGameOverTimer <= 0.0 )
		mScreenFade.fadeOut( std::bind( &LevelStage::changeMenuStage , this ) );

}


void LevelStage::updateRender( float dt )
{
	mLevel->updateRender( dt );
	mTweener.update( dt );
	mCamera->setPos( mLevel->getPlayer()->getPos() - ( 0.5f * mWorldScaleFactor ) * Vec2f( getGame()->getScreenSize() ) );
}


void RenderBar(RHIGraphics2D& g, float len , float h , float frac , float alpha , Color3f const& colorAT , Color3f const& colorAD , Color3f const& colorBT , Color3f const& colorBD )
{
	struct Vertex
	{
		Vec2f   pos;
		Color4f color;
	};

	if ( frac > 0 )
	{
		g.beginBlend(alpha, ESimpleBlendMode::Translucent);
		float temp = len * frac;
		g.drawGradientRect(Vec2f(0, 0), colorAT, Vec2f(temp, h), colorAD, false);
		g.endBlend();
	}

	g.beginBlend(alpha, ESimpleBlendMode::Add);
	g.drawCustomFunc([=](RHICommandList& commandList, Matrix4 const& baseTransform, RenderBatchedElement& element, Render::RenderTransform2D const& transform)
	{
		Vertex v[] =
		{
			{Vec2f(0,0), Color4f(colorBT,alpha) },
			{Vec2f(len , 0.0), Color4f(colorBT,alpha) },
			{Vec2f(len , h), Color4f(colorBD,alpha) },
			{Vec2f(0.0 , h) , Color4f(colorBD,alpha) },
		};
		for (int i = 0; i < ARRAY_SIZE(v); ++i)
		{
			v[i].pos = transform.transformPosition(v[i].pos);
		}
		TRenderRT< RTVF_XY_CA >::Draw(commandList, EPrimitive::LineLoop, v, ARRAY_SIZE(v));

	});
	g.endBlend();

}



void LevelStage::onRender()
{

	RenderEngine* renderEngine = getGame()->getRenderEenine();

	using namespace Render;

	RHICommandList& commandList = RHICommandList::GetImmediateList();

	RHISetFrameBuffer(commandList, nullptr);
	RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth , &LinearColor(0, 0, 0, 1) , 1 );

	//mWorldScaleFactor = 0.5f;

	mRenderConfig.camera      = mCamera;
	mRenderConfig.level       = mLevel;
	mRenderConfig.scaleFactor = mWorldScaleFactor;
	mRenderConfig.mode        = RM_ALL;

	renderEngine->renderScene( mRenderConfig );

	Player* player = mLevel->getPlayer();

	RHIGraphics2D& g = IGame::Get().getGraphics2D();
	g.beginRender();

	g.drawRect(Vector2(0, 0), Vector2(0, 0));
	Message* msg = mLevel->getTopMessage();
	if ( msg )
		msg->render(g);



	//Render Energy Bar
	g.pushXForm();
	g.translateXForm(getGame()->getScreenSize().x - 232, getGame()->getScreenSize().y - 32);
	{
		float colorAT[] = { 0.2, 0.2, 0.5 };
		float colorAD[] = { 0.05, 0.05, 0.1 };
		float colorBT[] = { 0.2, 0.3, 0.75 };
		float colorBD[] = { 0.1, 0.1, 0.25 };
		float frac = player->getEnergy() / player->getMaxEnergy();
		RenderBar(g, 200, 16, frac, 0.75, colorAT, colorAD, colorBT, colorBD);
	}
	g.popXForm();

	//Render HP Bar
	g.pushXForm();
	g.translateXForm(32, getGame()->getScreenSize().y - 32);
	{
		float colorAT[] = { 0.0, 0.5, 0.0 };
		float colorAD[] = { 0.0, 0.1, 0.0 };
		float colorBT[] = { 0.0, 0.75, 0.0 };
		float colorBD[] = { 0.0, 0.25, 0.0 };
		float frac = player->getHP() / player->getMaxHP();
		RenderBar(g, 200, 16, frac, 0.75, colorAT, colorAD, colorBT, colorBD);
	}
	g.popXForm();
	if ( mPause )
	{
		g.beginBlend(0.8, ESimpleBlendMode::Translucent);
		g.setBrush(Color3f(0, 0, 0));
		g.enablePen(false);
		g.drawRect(Vec2f(0, 0), Vec2f(getGame()->getScreenSize()));
		g.enablePen(true);
		g.endBlend();
	}

	GUISystem::Get().render();

	mScreenFade.render(g);
	g.beginBlend(1, ESimpleBlendMode::Add);
	{
		Vec2f size = Vec2f(32, 32);
		g.setBrush(Color3f(1, 1, 1));
		g.drawTexture(*mTexCursor->resource, Vec2f(getGame()->getMousePos()) - size / 2, size);
	}
	g.endBlend();

	InlineString< 256 > str;
	str.format( "x = %f , y = %f " , player->getPos().x , player->getPos().y );
	mDevMsg->setString( str );
	getRenderSystem()->drawText( mDevMsg , Vec2i( 10 , 10 ) , TEXT_SIDE_LEFT | TEXT_SIDE_RIGHT );

	g.endRender();
}

MsgReply LevelStage::onMouse( MouseMsg const& msg )
{
	return BaseClass::onMouse(msg);
}

extern bool gUseGroupRender;
#include <iostream>

MsgReply LevelStage::onKey(KeyMsg const& msg)
{
	if ( msg.isDown() )
	{
		switch (msg.getCode())
		{
		case EKeyCode::F4:
			{
				LevelEditStage* stage = new LevelEditStage;
				//FIXME
				stage->mLevel = mLevel;
				stage->mCamera = mCamera;
				stage->mWorldScaleFactor = mWorldScaleFactor;
				stage->mObjectCreator = mObjectCreator;
				stage->mActionCreator = mActionCreator;
				getGame()->addStage(stage, false);
			}
			break;
		case EKeyCode::F2:
			{
				gUseGroupRender = !gUseGroupRender;
				std::cout << "gUseGroupRender =" << gUseGroupRender << std::endl;
			}

			break;
		case EKeyCode::Q:
			mTweener.tweenValue< Easing::OQuad >(mWorldScaleFactor, mWorldScaleFactor, std::max(mWorldScaleFactor + 0.25f, 0.25f), 1.0f);
			break;
		case EKeyCode::E:
			mTweener.tweenValue< Easing::OQuad >(mWorldScaleFactor, mWorldScaleFactor, mWorldScaleFactor - 0.25f, 1.0f);
			break;
		}
	}

	return BaseClass::onKey( msg );
}


void LevelStage::onWidgetEvent( int event , int id , QWidget* sender )
{
	switch( id )
	{
	case UI_BACK_GAME:
		GUISystem::Get().findTopWidget( UI_MENU_PANEL )->show( false );
		mPause = false;
		break;
	case UI_GO_MENU:
		getGame()->addStage( new MenuStage , true );
		break;
	case UI_EXIT_GAME:
		mScreenFade.fadeOut( std::bind( &IGame::stopPlay , getGame() ) );
		break;
	}
	BaseClass::onWidgetEvent( event , id , sender );
}


void LevelStage::onLevelEvent( LevelEvent const& event )
{
	switch( event.id )
	{
	case LevelEvent::ePLAYER_DEAD:
		break;
	case LevelEvent::eCHANGE_STATE:
		if ( event.intVal == Level::eFINISH )
		{
			auto& levelInfo = IGame::Get().getPlayingLevel();

			levelInfo.levelStates[levelInfo.levelIndex] = PlayLevelInfo::ePlayed;
			if (levelInfo.levelIndex < MAX_LEVEL_NUM - 1)
			{
				levelInfo.levelStates[levelInfo.levelIndex + 1] = PlayLevelInfo::eOpen;
			}
			std::ofstream of( LEVEL_DIR LEVEL_LOCK_FILE );	
			for(int i=0; i<MAX_LEVEL_NUM; i++)
			{
				of << levelInfo.levelStates[i] << " ";
			}
			of.close();
		}
		break;
	}
}

void LevelStage::loadLevel()
{
	int mapWidth  = 128;
	int mapHeight = 128;

	auto& levelInfo = IGame::Get().getPlayingLevel();
	String mapPath = LEVEL_DIR;
	mapPath += levelInfo.mapFile;

	std::ifstream mapFS( mapPath.c_str() , std::ios::in );

	if ( mapFS.is_open() )
	{
		mapFS >> mapWidth;
		mapFS >> mapHeight;
	}

	mLevel->setupTerrain( mapWidth , mapHeight );

	TileMap& terrain = mLevel->getTerrain();

	if ( mapFS.good() )
	{
		std::string line;
		while(getline(mapFS,line))
		{
			std::istringstream lstring(line,std::ios::in);
			std::string token;
			while( getline(lstring,token,' ') )
			{
				if(token=="block")
				{
					getline(lstring,token,' ');
					int x = atoi(token.c_str());
					getline(lstring,token,' ');
					int y = atoi(token.c_str());
					getline(lstring,token,' ');
					int type = atoi(token.c_str());
					getline(lstring,token,' ');
					int meta = atoi(token.c_str());

					if (Block::IsVaild(type) && terrain.checkRange( x , y ) )
					{
						Tile& tile = terrain.getData( x , y ); 
						tile.id = type;
						tile.meta = meta;
					}
				}
				else if (token == "object")
				{
					getline(lstring, token, ' ');
					LevelObject* obj = mLevel->spawnObjectByName(token.c_str(), Vec2f::Zero());
					if (obj)
					{
						std::istreambuf_iterator<char> it(lstring) , end;
						std::string propertyStr = { it , end };
						TextPropEditor editor;
						editor.setupPorp(*obj);
						editor.importString(propertyStr.c_str());
					}
				}
			}
		}
		mapFS.close();
	}


	Player* player = mLevel->createPlayer();

	Vec2f posPlayer = Vec2f(0,0);

	String levelPath = LEVEL_DIR;
	levelPath += levelInfo.levelFile;

#if 1
	std::ifstream levelFS( levelPath.c_str() ,std::ios::in);
	std::string line;
	while(getline(levelFS,line))
	{
		std::istringstream lstring(line,std::ios::in);
		std::string token;
		while(getline(lstring,token,' '))
		{			
			if(token=="player")
			{
				getline(lstring,token,' ');
				float x=atof(token.c_str());
				getline(lstring,token,' ');
				float y=atof(token.c_str());
				posPlayer=Vec2f(x,y);
				player->setPos( posPlayer );
			}
			else if (token == "Mode")
			{
				getline(lstring, token, ' ');
				if (token == "Survival")
				{
					mMode.reset(new SurvivalMode);
				}
			}
			else if(token=="weapon")
			{
				Vec2f pos;
				getline(lstring,token,' ');
				pos.x=atof(token.c_str());
				getline(lstring,token,' ');
				pos.y=atof(token.c_str());
				getline(lstring,token,' ');
				int id=atoi(token.c_str());

				WeaponPickup* item = new WeaponPickup( pos , id );
				item->init();
				mLevel->addItem( item );
			}
			else if(token=="key")
			{
				Vec2f pos;
				getline(lstring,token,' ');
				pos.x=atof(token.c_str());
				getline(lstring,token,' ');
				pos.y=atof(token.c_str());
				getline(lstring,token,' ');
				int id=atoi(token.c_str());

				KeyPickup* item = new KeyPickup( pos , id );
				item->init();
				mLevel->addItem( item );
			}
			else if(token=="preload_sound")
			{
				getline(lstring,token,' ');
				getGame()->getSoundMgr()->loadSound(token.c_str());
			}
			else if(token=="music")
			{
				getline(lstring,token,' ');
				mMusic = getGame()->getSoundMgr()->addSound(token.c_str(), true);
				if (mMusic)
				{
					mMusic->play(0.15 , true);
				}
			}
			else if(token=="mob" || token == "object")
			{
				Vec2f pos;
				getline(lstring,token,' ');
				pos.x=atof(token.c_str());
				getline(lstring,token,' ');
				pos.y=atof(token.c_str());
				getline(lstring,token,' ');
				mLevel->spawnObjectByName( token.c_str() , pos );
			}
			else if(token=="mob_trigger")
			{
				Vec2f v1 , v2;
				getline(lstring,token,' ');
				v1.x=atof(token.c_str());
				getline(lstring,token,' ');
				v1.y=atof(token.c_str());
				getline(lstring,token,' ');
				v2.x=atof(token.c_str());
				getline(lstring,token,' ');
				v2.y=atof(token.c_str());
				getline(lstring,token,' ');
				
				AreaTrigger* trigger = new AreaTrigger( v1 , v2 );
				trigger->init();

				SpawnAct* act = new SpawnAct;
				act->spawnPos.x =atof(token.c_str());
				getline(lstring,token,' ');
				act->spawnPos.y =atof(token.c_str());
				getline(lstring,token,' ');	
				act->className.name = token;
				trigger->addAction( act );

				mLevel->addObject( trigger );
			}
			else if(token=="goal_trigger")
			{
				Vec2f v1 , v2;
				getline(lstring,token,' ');
				v1.x=atof(token.c_str());
				getline(lstring,token,' ');
				v1.y=atof(token.c_str());
				getline(lstring,token,' ');
				v2.x=atof(token.c_str());
				getline(lstring,token,' ');
				v2.y=atof(token.c_str());
				getline(lstring,token,' ');
				AreaTrigger* trigger = new AreaTrigger( v1 , v2 );
				trigger->init();

				trigger->addAction( new GoalAct );

				mLevel->addObject( trigger );
			}
			else if(token=="msg_trigger")
			{				
				Vec2f v1 , v2;
				getline(lstring,token,' ');
				v1.x=atof(token.c_str());
				getline(lstring,token,' ');
				v1.y=atof(token.c_str());
				getline(lstring,token,' ');
				v2.x=atof(token.c_str());
				getline(lstring,token,' ');
				v2.y=atof(token.c_str());
				getline(lstring,token,' ');
				AreaTrigger* trigger = new AreaTrigger( v1 , v2 );
				trigger->init();

				MessageAct* act = new MessageAct;
				getline(lstring,token,';');
				act->sender = token;
				getline(lstring,token,';');
				act->content = token;
				getline(lstring,token,' ');
				act->duration = atof(token.c_str());	
				getline(lstring,token,' ');
				act->soundName = token;
				trigger->addAction( act );

				mLevel->addObject( trigger );
			}
		}
	}
	levelFS.close();
#endif

}

void LevelStage::renderLoading()
{
	Texture* texBG2 = getRenderSystem()->getTextureMgr()->getTexture("MenuLoading1.tga");		

	RHIBeginRender();
	getRenderSystem()->prevRender();

	RHIGraphics2D& g = IGame::Get().getGraphics2D();
	g.beginRender();

	g.setBrush(Color3f(1, 1, 1));
	g.drawTexture(*texBG2->resource, Vec2f(0.0, 0.0), Vec2f(getGame()->getScreenSize().x, getGame()->getScreenSize().y));

	Vec2i pos = getGame()->getScreenSize() / 2;
	getRenderSystem()->drawText(getGame()->getFont(0), 35, "Loading Data..." , pos, Color4ub(50, 255, 50));
	g.endRender();

	getRenderSystem()->postRender();

	RHIEndRender(true);
}
