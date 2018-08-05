#include "FlightGame.h"

#include "Player.h"
#include "Weapon.h"
#include "Common.h"
#include "Object.h"
#include "Action.h"
#include "ObjModel.h"
#include "RenderEngine.h"
#include "AxisSweepDetector.h"
#include "ResourcesLoader.h"

#include "GameGUISystem.h"

#include "SystemPlatform.h"

namespace Shoot2D
{

	RenderEngine* TestStage::de = NULL;
	AxisSweepDetector* TestStage::objManger = NULL;

	void spawnObject(Object* obj)
	{
		TestStage::objManger->addObj( obj );
	}

	bool TestStage::onInit()
	{

		::Global::GUI().cleanupWidget();

		srand( SystemPlatform::GetTickCount() );

		mFrameCount = 0;
		mFPS   = 0;
		GameWindow& window = ::Global::GetDrawEngine()->getWindow();

		ResourcesLoader::Get().setDC( window.getHDC() );
		ResourcesLoader::Get().load();

		de = new RenderEngine( ::Global::GetGraphics2D().getRenderDC() );

		Rect_t rect;
		rect.Min = Vec2D(0,0);
		rect.Max = Vec2D(400,600);
		//objManger = new AxisSweepDetector(rect);
		objManger = new AxisSweepDetector( rect );

		m_player = new PlayerFlight( Vec2D(100,100) );
		m_player->setAction( new PlayerAction(key) );
		spawnObject( m_player );


		Vehicle* fly =  new Vehicle( MD_BASE , Vec2D(350,300) );
		fly->setTeam( TEAM_EMPTY );

		BulletGen* gen = new  SimpleGen;

		Weapon* weapon = new Weapon(gen);
		weapon->bullet      = MD_BULLET_1;
		weapon->bulletSpeed = 100;
		weapon->cdtime      = 1500;
		weapon->deleyTime   = 0;
		weapon->destObj     = m_player;
		fly->setWeapon( weapon );


		Vec2D dr( -100 , 0 );
		CircleMove* move = new CircleMove( 
			getCenterPos( *fly ) + dr,  sqrtf( dr.length2() ) , 1600 , true );
		fly->setAction( new SampleAI(move) );


		spawnObject( fly );

		return true;
	}


	void TestStage::onEnd()
	{
		delete de;
		delete objManger;
	}


	void TestStage::onUpdate( long time )
	{

		//m_ob->update();
		if ( rand() % 1000 <= 20 )
			produceFlight();
		if ( rand() % 2000 <= 1 )
		{
			Object* fly =  new Object( MD_LEVEL_UP );
			fly->setPos( Vec2D(rand()%400 , rand()%400) );
			fly->setTeam( TEAM_FRIEND );
			fly->setColFlag( COL_FRIEND_VEHICLE );
			spawnObject( fly );
		}

		objManger->update(time);
		objManger->testCollision();
	}

	void TestStage::onRender( float dFrame )
	{
		struct DrawFun
		{
			DrawFun(RenderEngine& d)
				: de(d){}
			void operator()(Object* obj)
			{
				de.draw( obj );
			}
			RenderEngine& de;
		};

		de->beginRender();

		objManger->visit( DrawFun(*de) );


		FixString< 32 > buf;
		buf.format( "ObjNum = %d   fps = %.1f" , objManger->getObjNum() , mFPS );

		de->drawText( 10 , 10 , buf );
		de->endRender();

		++mFrameCount;
		int64 dt = SystemPlatform::GetTickCount() - mTempTime;
		if ( mFrameCount > 200 || dt > 2000 )
		{
			mFPS = float( mFrameCount )  /  dt * 1000;
			mFrameCount = 0;
			mTempTime = SystemPlatform::GetTickCount();
		}
	}

	void TestStage::produceFlight()
	{
		Vehicle* fly =  new Vehicle( MD_BASE , Vec2D(200,200) );
		fly->setPos( Vec2D(rand()%400 , rand()%400) );
		fly->setTeam( TEAM_EMPTY );

		BulletGen* gen = new  SimpleGen;

		Weapon* weapon = new Weapon(gen);
		weapon->bullet = MD_BULLET_1;
		weapon->bulletSpeed = 5000;
		weapon->cdtime = 1500;
		weapon->deleyTime = 0;
		weapon->destObj = m_player;
		weapon->bulletType = Weapon::BT_MISSILE;
		fly->setWeapon( weapon );

		if ( rand() % 1 == 0 )
		{
			Vec2D dr( Random(80,160) , Random(80,160) );
			if ( rand() % 2 == 1 )
				dr = Vec2D(0,0) - dr;

			CircleMove* path = new CircleMove( 
				getCenterPos( *fly ) + dr,  sqrtf( dr.length2() ) , 1600 , ( rand() % 2 ) == 0 );
			fly->setAction( new SampleAI(path) );
		}
		else
		{
			PathMove* path = new PathMove;
			path->addPos( fly->getPos() );
			path->addPos( Vec2D( Random(10,400) , Random(10,550) ) );
			path->addPos( Vec2D( Random(10,400) , Random(10,550) ) );
			path->flag = rand() % 3;
			fly->setAction( new SampleAI(path) );
		}

		spawnObject( fly );
	}

}//namespace Shoot2D
