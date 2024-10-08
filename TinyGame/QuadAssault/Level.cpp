#include "Level.h"

#include "GameInterface.h"
#include "RenderSystem.h"
#include "SoundManager.h"

#include "ObjectFactory.h"
#include "ObjectRenderer.h"

#include "Block.h"

#include "LightObject.h"
#include "ItemPickup.h"
#include "Bullet.h"
#include "Mob.h"
#include "Player.h"
#include "Particle.h"
#include "Message.h"
#include "Explosion.h"

#include "LaserMob.h"
#include "MinigunMob.h"
#include "PlasmaMob.h"

#include "ProfileSystem.h"


Level::Level() 
{
	mTopMessage = NULL;
	mSpwanDestroyFlag = SDF_CAST_EFFECT;
}


void Level::init( ObjectCreator& creator )
{
	mColManager.setTerrain( mTerrain );
	mObjectCreator = &creator;
}

void Level::cleanup()
{
	destroyAllObjectImpl();

	mPlayers.clear();
	mObjects.clear();

	mLights.clear();
	mBullets.clear();
	mMobs.clear();	
	mItems.clear();
	mParticles.clear();

	mTopMessage = NULL;
	for(int i=0; i<mMsgQueue.size(); i++)
		delete mMsgQueue[i];
	mMsgQueue.clear();

}

void Level::tick()
{
	for(LevelObject* obj : mObjects)
	{
		obj->tick();
	}

	{
		PROFILE_ENTRY("Collision Update");
		mColManager.update();
	}
	for( ObjectList::iterator iter = mObjects.begin() , itEnd = mObjects.end(); 
		  iter != itEnd; )
	{
		LevelObject* obj = *iter;
		++iter;

		if( obj->mNeedDestroy )
		{
			obj->onDestroy( mSpwanDestroyFlag );
			delete obj;
		}
		else
		{
			obj->postTick();
		}
	}

	if ( mTopMessage == NULL )
	{
		if ( !mMsgQueue.empty() )
		{
			mTopMessage = mMsgQueue.front();
			mTopMessage->nodifyShow();
		}
	}
	if ( mTopMessage )
	{
		mTopMessage->tick();
		if ( mTopMessage->needDestroy )
		{
			delete mTopMessage;
			mMsgQueue.erase(mMsgQueue.begin());
			mTopMessage = NULL;
		}
	}
}

void Level::setupTerrain( int w , int h )
{
	mColManager.setup( w * BLOCK_SIZE ,  h * BLOCK_SIZE ,  5 * BLOCK_SIZE );

	mTerrain.resize( w ,  h );
	for(int i=0; i< w ; i++)
	{
		for(int j=0; j< h; j++)
		{		
			Tile& tile = mTerrain.getData( i , j );
			tile.pos  = Vec2f( BLOCK_SIZE * i , BLOCK_SIZE * j );
			tile.id = BID_FLAT;
			tile.meta = 0;
			if(i==0 || j==0 || i== w-1 || j== h-1)
				tile.id = BID_WALL;
		}	
	}
}


void Level::updateRender( float dt )
{
	for(LevelObject* obj : mObjects)
	{
		obj->updateRender( dt );
	}

	RenderLightList& lights = getRenderLights();
	for(Light* light : lights )
	{		
		light->cachedPos = light->offset;
		if (light->host)
		{
			light->cachedPos += light->host->getPos();
		}
	}

	if ( mTopMessage )
		mTopMessage->updateRender( dt );
}

void Level::addObjectInternal( LevelObject* obj )
{
	assert( obj && obj->mLevel == NULL );
	mObjects.push_front( obj );

	obj->mLevel = this;
	if (mSpwanDestroyFlag & (SDF_LOAD_LEVEL | SDF_EDIT))
	{
		obj->mbTransient = false;
	}
	obj->onSpawn( mSpwanDestroyFlag );
}

Player* Level::createPlayer()
{
	Player* player = new Player;
	player->init();
	player->mPlayerId = mPlayers.size();
	mPlayers.push_back( player );
	addObjectInternal( player );

	return player;
}

Explosion* Level::createExplosion( Vec2f const& pos , float raidus )
{
	Explosion* e = new Explosion( pos , raidus );
	e->init();
	addObjectInternal( e );
	return e;
}

LightObject* Level::createLight( Vec2f const& pos ,float radius )
{
	LightObject* light = new LightObject();
	light->init();
	light->setPos( pos );
	light->radius = radius;
	mLights.push_back( light );
	addObjectInternal( light );
	return light;
}

Bullet* Level::addBullet( Bullet* bullet )
{
	mBullets.push_back( bullet );
	addObjectInternal( bullet );
	return bullet;
}

ItemPickup* Level::addItem( ItemPickup* item )
{
	mItems.push_back( item );
	addObjectInternal( item );
	return item;
}

Mob* Level::addMob( Mob* mob )
{
	mMobs.push_back( mob );
	addObjectInternal( mob );
	return mob;
}

Particle* Level::addParticle( Particle* particle )
{
	mParticles.push_back( particle );
	addObjectInternal( particle );
	return particle;
}

int Level::random( int i1, int i2 )
{
	return ::rand()%i2+i1;
}

void Level::renderObjects(PrimitiveDrawer& drawer, RenderPass pass)
{
	for( LevelObject* obj : mItems )
	{
		IObjectRenderer* renderer =  obj->getRenderer();
		renderer->render(drawer, pass , obj);
	}


	for( LevelObject* obj : mMobs )
	{
		IObjectRenderer* renderer =  obj->getRenderer();
		renderer->render(drawer, pass, obj);
	}

	for( Player* player : mPlayers )
	{
		IObjectRenderer* renderer = player->getRenderer();
		renderer->render(drawer, pass, player);
	}

	for( LevelObject* obj : mBullets )
	{
		IObjectRenderer* renderer =  obj->getRenderer();
		renderer->render(drawer, pass, obj);
	}

	for( LevelObject* obj : mParticles )
	{
		IObjectRenderer* renderer =  obj->getRenderer();
		renderer->render(drawer, pass, obj);
	}

}

Sound* Level::playSound( char const* name , bool canRepeat /*= false */ )
{
	SoundPtr sound = getGame()->getSoundMgr()->addSound( name , canRepeat );
	if ( sound )
		sound->play();

	return sound;
}

void Level::destroyObject( LevelObject* object )
{
	object->onDestroy( mSpwanDestroyFlag );
	delete object;
}

void Level::changeState( State state )
{
	if ( mState = state )
		return;

	mState = state;

	LevelEvent event;
	event.id     = LevelEvent::eCHANGE_STATE;
	event.intVal = state;
	sendEvent( event );
}

Message* Level::addMessage( Message* msg )
{
	mMsgQueue.push_back( msg );
	return mMsgQueue.back();
}

Message* Level::addMessage(String const& sender, String const& content, float duration, String const& soundName)
{
	Message* msg = new Message();
	msg->init(sender, content, duration, soundName);
	return addMessage(msg);
}

void Level::sendEvent( LevelEvent const& event )
{
	for( auto lister : mListeners)
	{
		lister->onLevelEvent( event );
	}
}

void Level::addListerner( EventListener& listener )
{
	mListeners.push_back( &listener );
}

Tile* Level::getTile( Vec2f const& pos )
{
	int tx = int( pos.x / BLOCK_SIZE );
	int ty = int( pos.y / BLOCK_SIZE );

	if ( !mTerrain.checkRange( tx , ty ) )
		return NULL;
	return &mTerrain.getData( tx , ty );
}

void Level::addLight( Light& light )
{
	mRenderLights.push_front( &light );
}

void Level::destroyAllObjectImpl()
{
	for( ObjectList::iterator iter = mObjects.begin() , itEnd = mObjects.end();
		iter != itEnd;  )
	{
		LevelObject* obj = *iter;
		++iter;
		delete obj;
	}
}

void Level::destroyAllObject( bool skipPlayer )
{
	if ( skipPlayer )
	{
		for(Player* player : mPlayers)
		{
			player->baseHook.unlink();
		}
	}

	destroyAllObjectImpl();

	if ( skipPlayer )
	{
		for (Player* player : mPlayers)
		{
			mObjects.push_back( player );
		}
	}
	else
	{
		mPlayers.clear();
	}
}

void Level::addObject( LevelObject* object )
{
	assert( object );
	switch( object->getType() )
	{
	case OT_MOB:    mMobs.push_back( object->castChecked< Mob >() ); break;
	case OT_BULLET: mBullets.push_back( object->castChecked< Bullet >() ); break;
	case OT_LIGHT:  mLights.push_back( object->castChecked< LightObject >() ); break;
	case OT_PARTICLE:  mParticles.push_back( object->castChecked< Particle >() ); break;
	case OT_ITEM:   mItems.push_back( object->castChecked< ItemPickup >() ); break;
	case OT_PLAYER: 
		{
			return;
		}
		break;
	}
	addObjectInternal( object );
	
}

LevelObject* Level::spawnObjectByName( char const* name , Vec2f const& pos  )
{
	LevelObject* obj = mObjectCreator->createObject( name );
	if ( !obj )
		return NULL;

	obj->init();
	obj->setPos( pos );
	if ( mSpwanDestroyFlag & SDF_SETUP_DEFAULT )
		obj->setupDefault();
	addObject( obj );
	return obj;
}

LevelObject* Level::hitObjectTest( Vec2f const& pos )
{
	for(LevelObject* obj : mObjects)
	{
		BoundBox bBox;
		obj->calcBoundBox( bBox );

		if ( bBox.isInside( pos ) )
			return obj;
	}

	return NULL;
}

void Level::renderDev(RHIGraphics2D& g, DevDrawMode mode)
{
	for(LevelObject* obj : mObjects)
	{
		obj->renderDev(g, mode);
	}
}

unsigned Level::setSpwanDestroyFlag( unsigned flag )
{
	unsigned prevFlag = mSpwanDestroyFlag;
	mSpwanDestroyFlag = flag;
	return prevFlag;
}
