#include "BMPCH.h"

#include "BMLevel.h"
#include "BMRender.h"

#include "GameShare.h"

//FIXME
#include "GameGlobal.h"

#include <algorithm>

//TODO: fix ignore damage when player hurted

namespace BomberMan
{

	float const gKickBombSpeed = 0.15f;
	float const gPlayerMoveSpeed = 0.08f;
	float const gFastRunnerMoveSpeed = 0.2f;
	float const gSlowFootSpeed = 0.02f;

	int   const gPowerGloveThrowDist = 4;

	float const gTileLength = 1.0f;
	float const gTileHalfLength = 0.5f * gTileLength;
	Vec2f const gTileCenterOffset = Vec2f( gTileHalfLength , gTileHalfLength );
	Vec2f const gTileSize = Vec2f( gTileLength , gTileLength );

	unsigned gTypeMask[ NumObjectType ];


	static float const gDirFactor[] = { 1.0f ,1.0f, -1.0f, -1.0f };
	static Dir   const gInvDir[] = { DIR_WEST , DIR_NORTH , DIR_EAST , DIR_SOUTH }; 

	Dir getInvDir( Dir dir )
	{
		return gInvDir[ dir ];
	}


	static bool testIntersect( Vec2f const& aMin , Vec2f const& aMax , Vec2f const& bMin , Vec2f const& bMax )
	{
		if ( aMax.x <= bMin.x || bMax.x <= aMin.x )
			return false;
		if ( aMax.y <= bMin.y || bMax.y <= aMin.y )
			return false;
		return true;
	}

	static Dir calcIntersectInfo( Vec2f const& aMin , Vec2f const& aMax , Vec2f const& bMin , Vec2f const& bMax , float& depth )
	{
		assert( testIntersect( aMin, aMax, bMin, bMax ) );

		Dir   dir = Dir(-1);
		float test[4];
		depth = 10000000.0;
		test[DIR_EAST]  = aMax.x - bMin.x;
		test[DIR_WEST]  = bMax.x - aMin.x;
		test[DIR_SOUTH] = aMax.y - bMin.y;
		test[DIR_NORTH] = bMax.y - aMin.y;

		for( int i = 0 ; i < 4 ; ++i )
		{
			if ( test[i] > 0 && test[i] < depth )
			{
				dir   = Dir(i);
				depth = test[i];
			}
		}
		assert( dir != -1 );
		return dir;
	}


	Vec2f TileToPos( Vec2i const& pos )
	{
		return Vec2f( gTileLength * pos.x , gTileLength * pos.y );
	}

	Vec2i PosToTile( Vec2f const& pos )
	{
		return Vec2i( int( pos.x / gTileLength ) , int ( pos.y / gTileLength ) );
	}

	World::World()
	{
		gTypeMask[ OT_BOMB ]    = CMB_SOILD;
		gTypeMask[ OT_MONSTER ] = CMB_SOILD;
		gTypeMask[ OT_BOMB ]    = CMB_SOILD;
		gTypeMask[ OT_PLAYER ]  = CMB_SOILD;
		gTypeMask[ OT_BULLET ]  = CMB_BULLET;
		gTypeMask[ OT_TRIGGER ]  = CMB_TRIGGER;
		gTypeMask[ OT_MINE_CAR ] = CMB_SOILD;

		std::fill_n( mPlayerMap , gMaxPlayerNum , (Player*)NULL );
		mPlayerNum = 0;
		mUsingIndex = -1;
		mFreeIndex  = -1;
		mListener = NULL;
		mIsEntitiesDirty = true;
	}

	void World::setupMap( Vec2i const& size , uint16 const* id )
	{
		if ( mMap.getSizeX() != size.x || mMap.getSizeY() != size.y )
			mMap.resize( size.x , size.y );

		int num = size.x * size.y;
		if ( id )
		{
			for( int j = 0 ; j < size.y ; ++j )
			for( int i = 0 ; i < size.x ; ++i )
			{
				TileData& data = mMap.getData( i , j );
				data.setValue( *(id++) );
				data.meta = 0;
			}
		}
		else
		{

			for( int j = 0 ; j < size.y ; ++j )
			for( int i = 0 ; i < size.x ; ++i )
			{
				TileData& data = mMap.getData( i , j );
				data.terrain = MT_DIRT;
				data.obj     = MO_NULL;
				data.meta    = 0;
			}
		}
	}

	void World::restart()
	{
		cleanupEntity();

		mColObjList.clear();
		for( int i = 0 ; i < getPlayerNum() ; ++i )
		{
			Player* player = getPlayer( i );
			player->reset();
			addEntity( player , false );
		}

		mUsingIndex = -1;
		mFreeIndex  = -1;
		mBombList.clear();

	}

	void World::tick()
	{
		for( EntityList::iterator iter = mEnities.begin();
			 iter != mEnities.end() ; ++iter )
		{
			EntityData& data = *iter;
			Entity* entity = data.entity;

			entity->mPrevPos = entity->getPos();
			entity->update();
		}


		CollisionInfo info;
		info.type = COL_OBJECT;

		for( ColObjectList::iterator iter1 = mColObjList.begin();
			iter1 != mColObjList.end() ; ++iter1 )
		{
			ColObjectData& data = *iter1;
			ColObject* obj1 = iter1->obj;
			Entity*    client1 = obj1->getClient();
			data.bMin = client1->getPos() - obj1->getHalfBoundSize();
			data.bMax = client1->getPos() + obj1->getHalfBoundSize();
		}

		for( ColObjectList::iterator iter1 = mColObjList.begin();
			iter1 != mColObjList.end() ; ++iter1 )
		{
			ColObjectData& data = *iter1;
			ColObject* obj1 = iter1->obj;
			Entity*    client1 = obj1->getClient();

			ColObjectList::iterator iter2 = iter1;
			++iter2;

			for ( ; iter2 != mColObjList.end() ; ++iter2 )
			{
				ColObject* obj2 = iter2->obj;
				Entity*    client2 = obj2->getClient();

				bool b1 = ( obj1->getColMask() & gTypeMask[ client2->getType() ] ) != 0;
				bool b2 = ( obj2->getColMask() & gTypeMask[ client1->getType() ] ) != 0;
				if ( b1 && b2 )
				{
					if ( testIntersect( iter1->bMin , iter1->bMax , iter2->bMin , iter2->bMax ) )
					{
						Msg( "Object Collision ( %d , %d )" , obj1->getClient()->getType() , obj2->getClient()->getType() );
						info.obj = obj2;
						obj1->notifyCollision( info );
						info.obj = obj1;
						obj2->notifyCollision( info );
					}
				}
			}
		}

		int  index = mUsingIndex;
		int* pLinkIndex = &mUsingIndex;
		while( index != -1 )
		{
			Bomb& bomb = mBombList[ index ];
			if ( !updateBomb( bomb ) )
			{
				int next = bomb.next;
				bomb.next = mFreeIndex;
				mFreeIndex = index;
				*pLinkIndex = next;
				index = next;
			}
			else
			{
				index = bomb.next;
				pLinkIndex = &bomb.next;
			}
		}


		for( EntityList::iterator iter = mEnities.begin();
			iter != mEnities.end() ; ++iter )
		{
			iter->entity->postUpdate();
		}

		for( EntityList::iterator iter = mEnities.begin();
			iter != mEnities.end() ;  )
		{
			Entity* entity = iter->entity;
			if ( entity->mNeedRemove )
			{
				if ( iter->beManaged )
					delete entity;
				iter = mEnities.erase( iter );
				mIsEntitiesDirty = true;
			}
			else
			{
				++iter;
			}
		}

	}

	int World::addBomb( Player& player , Vec2i const& pos , int power , int time , unsigned flag )
	{
		int index;
		if ( mFreeIndex != -1 )
		{
			index = mFreeIndex;
			mFreeIndex = mBombList[ index ].next;
		}
		else
		{
			index = mBombList.size();
			mBombList.push_back( Bomb() );
			mBombList[ index ].next = -1;
		}

		Bomb& bomb = mBombList[ index ];
		bomb.index = index;
		bomb.pos   = pos;
		bomb.power = power;
		bomb.flag  = flag;

		bomb.owner  = &player;
		bomb.obj    = NULL;
		bomb.timer  = time;

		bomb.next   = mUsingIndex;
		mUsingIndex = index;

		if ( mMap.checkRange( pos.x , pos.y ))
		{
			bomb.state = Bomb::eONTILE;

			TileData& tile = getTile( pos );
			tile.obj   = MO_BOMB;
			tile.meta  = index;
		}
		else
		{
			bomb.state = Bomb::eTAKE;
		}

		return bomb.index;
	}



	void World::breakTileObj( Vec2i const& pos )
	{
		TileData& tile = getTile( pos );

		assert( tile.obj != MO_NULL );

		char  obj  = tile.obj;
		short meta = tile.meta;

		tile.obj  = MO_NULL;
		tile.meta = -1;

		mListener->onBreakTileObject( pos , obj , meta );
	}
	bool World::fireTile( Vec2i const& pos , Bomb& bomb , int dir )
	{
		TileData& tile = getTile( pos );

		switch( tile.terrain )
		{
		case MT_ARROW_CHANGE + 0:case MT_ARROW_CHANGE + 1:
		case MT_ARROW_CHANGE + 2:case MT_ARROW_CHANGE + 3:
			tile.terrain += 1;
			if ( tile.terrain == MT_ARROW_CHANGE_END )
				tile.terrain = MT_ARROW_CHANGE;
			break;
		}

		switch( tile.obj )
		{
		case MO_BLOCK:
			return true;
		case MO_WALL:
			breakTileObj( pos );
			getAnimManager()->setTileAnim( pos , ANIM_WALL_BURNING );
			return true;
		case MO_ITEM:
			breakTileObj( pos );
			return true;
		case MO_BOMB:
			if ( getBomb( tile.meta ).state == Bomb::eONTILE )
			{
				fireBomb( getBomb( tile.meta ) );
				return true;
			}
			return false;
		}
		return false;
	}

	bool World::updateBomb( Bomb& bomb )
	{
		if ( bomb.state >= 0 )
		{
			if ( bomb.flag & BF_DANGEROUS )
			{
				if ( bomb.state == bomb.power )
				{
					++bomb.timer;
					if ( bomb.timer >= gFireHoldFrame )
					{
						TileData& tile = getTile( bomb.pos );
						tile.obj  = MO_NULL;
						tile.meta = 0;
						return false;
					}
				}
				else
				{
					bomb.timer += gFireMoveSpeed;
					while ( bomb.timer >= gFireMoveTime )
					{
						bomb.state += 1;
						bomb.timer -= gFireMoveTime;
						{
							Vec2i posTop;  posTop.y  = bomb.pos.y - bomb.state;
							Vec2i posDown; posDown.y = bomb.pos.y + bomb.state;
							for( int i = -bomb.state ; i <= bomb.state ; ++i )
							{
								posTop.x = bomb.pos.x + i;
								if ( mMap.checkRange( posTop.x , posTop.y ) )
									fireTile( posTop , bomb , DIR_NORTH );
								posDown.x = bomb.pos.x + i;
								if ( mMap.checkRange( posDown.x , posDown.y ) )
									fireTile( posDown , bomb , DIR_SOUTH );
							}
						}
						{
							Vec2i posLeft;  posLeft.x  = bomb.pos.x - bomb.state;
							Vec2i posRight; posRight.x = bomb.pos.x + bomb.state;
							for( int j = bomb.state - 1 ; j <= bomb.state - 1 ; ++j )
							{
								posLeft.y = bomb.pos.y + j;
								if ( mMap.checkRange( posLeft.x , posLeft.y ) )
									fireTile( posLeft , bomb , DIR_WEST );
								posRight.y = bomb.pos.y + j;
								if ( mMap.checkRange( posRight.x , posRight.y ) )
									fireTile( posRight , bomb , DIR_EAST );
							}
						}

						if ( bomb.state == bomb.power )
						{
							bomb.timer = 0;
							break;
						}
					}
				}
			}
			else
			{
				if ( bomb.state == bomb.power )
				{
					++bomb.timer;
					if ( bomb.timer >= gFireHoldFrame )
					{
						TileData& tile = getTile( bomb.pos );
						tile.obj  = MO_NULL;
						tile.meta = 0;
						return false;
					}
				}
				else
				{
					bomb.timer += gFireMoveSpeed;
					while ( bomb.timer >= gFireMoveTime )
					{
						bomb.state += 1;
						bomb.timer -= gFireMoveTime;

						for( int dir = 0 ; dir < 4 ; ++dir )
						{
							if ( bomb.fireLength[dir] != -1 )
								continue;

							Vec2i pos = bomb.pos + bomb.state * getDirOffset( dir );
							if ( !mMap.checkRange( pos.x , pos.y ) )
								continue;

							if ( fireTile( pos , bomb , dir ) )
							{
								if ( ( bomb.flag & BF_PIERCE ) == 0 )
								{
									bomb.fireLength[dir] = bomb.state - 1;
								}
							}
						}

						if ( bomb.state == bomb.power )
						{
							bomb.timer = 0;
							break;
						}
					}
				}
			}

			testFireCollision( bomb );
		}
		else // bomb.fireState < 0 
		{
			switch ( bomb.state )
			{
			case Bomb::eDESTROY:
				{
					if ( mMap.checkRange( bomb.pos.x , bomb.pos.y ) )
					{
						TileData& tile = getTile( bomb.pos );
						if ( tile.obj == MO_BOMB && tile.meta == bomb.index )
						{
							tile.obj  = MO_NULL;
							tile.meta = -1;
						}
					}
					if ( bomb.obj )
					{
						removeEntity( bomb.obj );
						bomb.obj = NULL;
					}

					bomb.owner->onBombEvent( bomb , Player::eDESTROY );
					return false;	
				}
				break;
			case Bomb::eONTILE:
				{
					bomb.timer -= 1;
					checkFireBomb(bomb);
				}
				break;
			case Bomb::eTAKE:
				break;
			case Bomb::eJUMP:
			case Bomb::eMOVE:
				{
					if ( bomb.state == Bomb::eMOVE )
						bomb.timer -= 1;

					if ( bomb.obj->isStopMotion() )
					{
						bomb.pos = PosToTile( bomb.obj->getPos() );

						warpTilePos( bomb.pos );

						TileData& tile = getTile( bomb.pos );

						if ( tile.obj != MO_NULL )
						{
							Msg( "warming:" );
						}

						tile.obj  = MO_BOMB;
						tile.meta = bomb.index; 

						bomb.state = Bomb::eONTILE;
						removeEntity( bomb.obj );
						bomb.obj = NULL;

						checkFireBomb( bomb );
					}
				}
				break;
			}
		}
		return true;
	}


	void World::fireBomb( Bomb& bomb )
	{
		bomb.state = 0;
		bomb.timer = 0;

		if ( bomb.obj )
		{
			removeEntity( bomb.obj );
			bomb.obj = NULL;
		}

		bomb.fireLength[0] = bomb.fireLength[1] = -1;
		bomb.fireLength[2] = bomb.fireLength[3] = -1;

		bomb.owner->onBombEvent( bomb , Player::eFIRE );
	}

	Player* World::addPlayer( Vec2i const& pos )
	{
		if ( mPlayerNum == gMaxPlayerNum )
			return NULL;

		Player* player = new Player( (unsigned)mPlayerNum );
		player->reset();
		player->setPos( TileToPos( pos ) + gTileCenterOffset );

		mPlayerMap[ mPlayerNum ] = player;
		++mPlayerNum;

		addEntity( player , false );
		return player;
	}

	void World::testFireCollision( Bomb& bomb )
	{
		assert( bomb.state >= 0 );

		if ( bomb.state == 0 )
			return;

		int fireLen[4];
		for( int dir = 0 ; dir < 4 ; ++dir )
			fireLen[dir] = bomb.getFireLength( Dir(dir) );

		Vec2f bombPos   = TileToPos( bomb.pos );
		float const fireGap  = 0.1f;

		Vec2f minFire[4];
		Vec2f maxFire[4];

		minFire[DIR_EAST ] = Vec2f( 0 , fireGap );
		maxFire[DIR_EAST ] = Vec2f( fireLen[DIR_EAST] + 1.0f , 1.0f - fireGap );
		minFire[DIR_SOUTH] = Vec2f( fireGap , 0 );
		maxFire[DIR_SOUTH] = Vec2f( 1.0f - fireGap , fireLen[DIR_SOUTH] + 1.0f );
		minFire[DIR_WEST ] = Vec2f( -fireLen[DIR_WEST] , fireGap );
		maxFire[DIR_WEST ] = Vec2f( 1.0f , 1.0f - fireGap );
		minFire[DIR_NORTH] = Vec2f( fireGap , -fireLen[ DIR_NORTH ] );
		maxFire[DIR_NORTH] = Vec2f( 1.0f -fireGap , 1.0f );

		CollisionInfo info;
		info.type = COL_FIRE;
		info.bomb = &bomb;

		for( ColObjectList::iterator iter = mColObjList.begin();
				iter != mColObjList.end() ; ++iter )
		{
			ColObjectData& data = *iter;

			if ( data.obj->getColMask() & CMB_FIRE )
			{
				Vec2f minObj = data.bMin - bombPos; 
				Vec2f maxObj = data.bMax - bombPos;

				for( int i = 0 ; i < 4 ; ++i )
				{
					if ( testIntersect( minFire[i] , maxFire[i] , minObj , maxObj ) )
					{
						data.obj->notifyCollision( info );
						bomb.owner->onFireCollision( bomb , *data.obj );
						break;
					}
				}
			}
		}
	}


	void World::resolvePlayerCollision( ColObjectData& data1 , ColObjectData& data2 )
	{
		assert ( data1.obj->getClient()->getType() == OT_PLAYER &&
			     data2.obj->getClient()->getType() == OT_PLAYER );

		Player* playerA = static_cast< Player* >( data1.obj );
		Player* playerB = static_cast< Player* >( data2.obj );

		return;
		
		float depth;
		Dir dir = calcIntersectInfo( data1.bMin , data1.bMax , data2.bMin , data2.bMax , depth );

		Vec2f posA = playerA->getPos();
		Vec2f posB = playerB->getPos();

		int idx = dir % 2;
		if ( playerA->getFaceDir() == dir )
		{
			if ( playerB->getFaceDir() == getInvDir( dir ) )
			{
				if ( playerA->checkActionKey( Player::ACT_MOVE ) )
				{
					if ( playerB->checkActionKey( Player::ACT_MOVE ) )
					{
						posA[ idx ] -= gDirFactor[ dir ] * depth * gTileHalfLength;
						posB[ idx ] += gDirFactor[ dir ] * depth * gTileHalfLength;
					}
					else
					{
						posA[ idx ] -= gDirFactor[ dir ] * depth;
					}
				}
				else
				{
					posB[ idx ] += gDirFactor[ dir ] * depth;
				}

			}
			else
			{
				posA[ idx ] -= gDirFactor[ dir ] * depth;
			}
		}
		else if ( playerB->getFaceDir() == getInvDir( dir ) )
		{
			posB[ idx ] += gDirFactor[ dir ] * depth;
		}
		else
		{
			posA = playerA->getPrevPos();
			posB = playerB->getPrevPos();
		}

		playerA->setPos( posA );
		playerB->setPos( posB );
	}

	void World::addColObject( ColObject& obj )
	{
		ColObjectData data;
		data.obj       = &obj;
		mColObjList.push_back( data );
	}

	void World::removeColObject( ColObject& obj )
	{
		for( ColObjectList::iterator iter = mColObjList.begin();
			 iter != mColObjList.end() ; ++iter )
		{
			if ( iter->obj == &obj )
			{
				mColObjList.erase( iter );
				return;
			}
		}
	}

	void World::moveBomb( int idx , Dir dir , float speed , bool beForce )
	{
		Bomb& bomb = getBomb( idx );

		if ( !beForce && bomb.flag & BF_UNMOVABLE )
			return;

		if ( bomb.state == Bomb::eONTILE )
		{
			TileData& tile = getTile( bomb.pos );
			assert( tile.obj == MO_BOMB && tile.meta == getBombIndex( bomb ) );
			tile.obj  = MO_NULL;
			tile.meta = -1;
		}

		MoveBomb* bombObj = new MoveBomb( idx , bomb.pos , dir );
		bombObj->setMoveSpeed( speed );
		bomb.obj   = bombObj;
		bomb.state = Bomb::eMOVE;
		
		addEntity( bombObj , true );
	}

	void World::throwBomb( int idx , Vec2i const& pos , Dir dir , int dist )
	{
		Bomb& bomb = getBomb( idx );

		if ( bomb.state == Bomb::eONTILE )
		{
			TileData& tile = getTile( bomb.pos );
			assert( tile.obj == MO_BOMB && tile.meta == getBombIndex( bomb ) );
			tile.obj  = MO_NULL;
			tile.meta = -1;
		}

		bomb.obj = new JumpBomb( idx , pos , dir , dist );
		bomb.state = Bomb::eJUMP;
		addEntity( bomb.obj , true );
	}

	void World::checkFireBomb( Bomb &bomb )
	{
		bool fire = false;
		if ( bomb.flag & BF_CONTROLLABLE )
		{
			if ( bomb.owner->checkActionKey( Player::ACT_FUN ) )
				fire = true;
		}
		else
		{
			if ( bomb.timer <= 0 )
				fire = true;
		}

		if ( fire )
		{
			fireBomb(bomb);
		}
	}

	void World::clearAction()
	{
		for( int i = 0 ; i < mPlayerNum ; ++i )
		{
			getPlayer( i )->clearActionKey();
		}
	}

	void World::warpTilePos( Vec2i& pos )
	{
		pos.x = pos.x % mMap.getSizeX();
		if ( pos.x < 0 )
			pos.x += mMap.getSizeX();

		pos.y = pos.y % mMap.getSizeY();
		if ( pos.y < 0 )
			pos.y += mMap.getSizeY();
	}

	TileData& World::getTileWarp( Vec2i const& pos )
	{
		Vec2i warpPos = pos;
		warpTilePos( warpPos );
		return getTile( warpPos );
	}

	TileData* World::getTileWithPos( Vec2f const& pos , Vec2i& tPos )
	{
		if ( 0 > pos.x || pos.x > (float) gTileLength * mMap.getSizeX() )
			return NULL;
		if ( 0 > pos.y || pos.y > (float) gTileLength * mMap.getSizeY() )
			return NULL;
		tPos = PosToTile( pos );
		return &getTile( tPos );
	}

	int World::getBombIndex( Bomb& bomb )
	{
		return int( (&bomb) - &mBombList[0] );
	}

	class EmptyAnimManager : public IAnimManager
	{	
	public:
		virtual void setupLevel( LevelTheme theme ){}
		virtual void restartLevel( bool beInit ){}
		virtual void setPlayerAnim( Player& player , AnimAction action ){}
		virtual void setTileAnim( Vec2i const& pos , AnimAction action ){}
	} gEmptyAimManager;

	void World::setAnimManager( IAnimManager* manager )
	{
		mAimManager = manager;
		if ( mAimManager == NULL )
			mAimManager = &gEmptyAimManager;
	}

	int World::getRandom( int value )
	{
		return Global::RandomNet() % value;
	}

	SkullType World::getAvailableDisease()
	{
		return SkullType( getRandom( NumSkullType - 1 ) + 1 );
	}

	void World::addEntity( Entity* entity , bool beManaged )
	{
		assert( findEntity( entity ) == mEnities.end() );
		assert( entity );
		EntityData data;
		data.entity   = entity;
		data.beManaged = beManaged;

		mEnities.push_back( data );
		mIsEntitiesDirty = true;

		entity->mWorld = this;
		entity->onSpawn();
	}


	World::EntityList::iterator World::findEntity( Entity* entity )
	{
		EntityList::iterator iter = mEnities.begin();
		for ( ; iter != mEnities.end() ; ++iter )
		{
			if ( iter->entity == entity )
				break;
		}
		return iter;
	}

	void World::removeEntity( Entity* entity )
	{
		assert( entity );
		
		if ( entity->mNeedRemove )
			return;

		entity->onDestroy();
		entity->mNeedRemove = true;
	}

	void World::cleanupEntity()
	{
		for( EntityList::iterator iter = mEnities.begin();
			iter != mEnities.end() ; ++iter )
		{
			Entity* entity = iter->entity;
			if ( !entity->mNeedRemove )
				entity->onDestroy();
		}

		for( EntityList::iterator iter = mEnities.begin();
			iter != mEnities.end() ; ++iter )
		{
			if ( iter->beManaged )
				delete iter->entity;
		}

		mEnities.clear();
		mIsEntitiesDirty = true;
	}

	bool World::testCollision( ColObject& obj1 , ColObject& obj2 )
	{

		return false;
	}

	RooeyType World::getAvailableRooey()
	{
		return RT_NONE;
	}

	Actor::Actor( ObjectType type ) 
		:ColEntity( type )
	{
		setColMask( CMB_ALL );
	}


	bool Actor::moveOnTile( Dir moveDir , float offset , Vec2f& goalPos , TileData* colTile[] , float margin )
	{
		assert( offset > 0 );
		assert( getHalfBoundSize().x + margin  < gTileHalfLength && getHalfBoundSize().y + margin < gTileHalfLength );

		float const MaxDiff = 0.40f;

		Vec2f const& pos = getPrevPos();

		int idxMove = moveDir % 2;
		int idxFix  = ( idxMove + 1 ) % 2;

		Vec2f cp = pos;
		float testLen = getHalfBoundSize()[ idxMove ];
		cp[ idxMove ] += ( testLen + offset + margin ) * gDirFactor[ moveDir ];

		Vec2f p1 = cp; p1[ idxFix ] -= testLen;
		Vec2f p2 = cp; p2[ idxFix ] += testLen;

		colTile[0] = testBlocked( p1 );
		colTile[1] = testBlocked( p2 );

		if ( colTile[0] == NULL )
		{
			if ( colTile[1] == NULL )
			{
				goalPos[ idxMove ] = pos[ idxMove ] + offset * gDirFactor[ moveDir ];
				goalPos[ idxFix  ] = pos[ idxFix ];
				return false;
			}
			else
			{
				int fixPos = (int)p1[ idxFix ];
				float diff = p1[ idxFix ] - float( fixPos );
				if ( diff < MaxDiff )
				{
					float offFix = std::min( offset , diff );
					float offMove  = ( offset - offFix );
					if ( offMove > 0.0f )
						goalPos[ idxMove ] = pos[ idxMove ] + offMove * gDirFactor[ moveDir ];
					else
						goalPos[ idxMove ] = pos[ idxMove ];

					goalPos[ idxFix ] = pos[ idxFix ] - offFix;
					return true;
				}
			}
		}
		else if ( colTile[1] == NULL )
		{
			int fixPos = (int)p2[ idxFix ];
			float diff = float( fixPos ) + 1.0f - p2[ idxFix ];
			if ( diff < MaxDiff )
			{
				float offFix = std::min( offset , diff );
				float offMove  = ( offset - offFix );
				if ( offMove > 0.0f )
					goalPos[ idxMove ] = pos[ idxMove ] + offMove * gDirFactor[ moveDir ];
				else
					goalPos[ idxMove ] = pos[ idxMove ];

				goalPos[ idxFix ] = pos[ idxFix ] + offFix;
				return true;
			}
		}

		float factor2[] = { 1.0f , 1.0f , 0.0f , 0.0f };

		goalPos[ idxMove ] = floor( pos[ idxMove ] ) + factor2[ moveDir ] - gDirFactor[ moveDir ] * ( getHalfBoundSize()[ idxMove ] + margin );
		goalPos[ idxFix  ] = pos[idxFix];
		return true;
	}


	TileData* Actor::testBlocked( Vec2f const& pos )
	{
		Vec2i tPos;
		TileData* tile = getWorld()->getTileWithPos( pos , tPos );

		if ( tile == NULL )
			return NULL;

		if ( !tile->isBlocked() )
			return NULL;

		return tile;
	}

	Player::Player( unsigned id ) 
		:BaseClass( OT_PLAYER )
		,mId( id )
	{
		setHalfBoundSize( gTileLength * Vec2f( 0.475f , 0.475f ) );
	}

	void Player::reset()
	{
		setColMask( CMB_ALL );
		changeState( STATE_NORMAL );

		mFaceDir = DIR_SOUTH;
		mDisease = SKULL_NULL;

		mIdxBombTake = -1;

		changeAction( AS_IDLE );

		mActionFlag.clear();

		mPrevPlayerColBit = 0;
		mPlayerColBit     = 0;

		getStatus().health     = 1;
		getStatus().power      = 1;
		getStatus().numUseBomb = 0;
		getStatus().maxNumBomb = 1;
		mToolFlag   = 0;


		std::fill_n( tools , (int)NUM_ITEM_TYPE , 0 );
	}

	void Player::takeDamage()
	{
		--health;
		if ( health <= 0 )
		{
			changeState( STATE_DIE );
			setColMask( 0 );
			getWorld()->getListener()->onPlayerEvent( EVT_ACTOR_DIE , *this );
			getWorld()->getAnimManager()->setPlayerAnim( *this , ANIM_DIE  );
		}
		else
		{



		}
	}

	void Player::update()
	{
		Vec2f nextPos = getPos(); 

		mTimeState += 1;

		mPrevPlayerColBit = mPlayerColBit;
		mPlayerColBit = 0;

		switch( getState() )
		{
		case STATE_INVICIBLE:
		case STATE_NORMAL:
			{
				TileData* tiles[4] = { NULL };

				Vec2f const& bSize = getHalfBoundSize();
				int numTile = 0;
				numTile = checkTouchTileInternal( getPos() + Vec2f( bSize.x ,  bSize.y ) , tiles , numTile );
				numTile = checkTouchTileInternal( getPos() + Vec2f( -bSize.x , -bSize.y ) , tiles , numTile );
				numTile = checkTouchTileInternal( getPos() + Vec2f( bSize.x , -bSize.y ) , tiles , numTile );
				numTile = checkTouchTileInternal( getPos() + Vec2f( -bSize.x ,  bSize.y ) , tiles , numTile );

				updateAction( nextPos );
			}
			break;
		case STATE_DIE:
			if ( mTimeState > 100 && getWorld()->getRule().haveGhost )
			{
				changeState( STATE_GHOST );
				nextPos =  Vec2f( gTileHalfLength , 1.5 * gTileLength );
				setFaceDir( DIR_EAST );
			}
			break;
		case STATE_GHOST:
			{
				int const MoveDirMap[]    = { DIR_NORTH , DIR_EAST , DIR_SOUTH , DIR_WEST };
				int const InvMoveDirMap[] = { DIR_SOUTH , DIR_WEST , DIR_NORTH , DIR_EAST };
				
				float gGhostMoveSpeed = 0.1;

				float xMax = gTileLength * getWorld()->getMapSize().x - gTileHalfLength;
				float yMax = gTileLength * getWorld()->getMapSize().y - gTileHalfLength;

				if ( checkActionKey( Actor::ACT_MOVE ) )
				{
					nextPos = getPos() + gGhostMoveSpeed * gTileLength * Vec2f( getDirOffset( MoveDirMap[ getFaceDir() ] ) );
					
					switch( getFaceDir() )
					{
					case DIR_SOUTH:
						if ( nextPos.x > xMax )
						{
							nextPos.x = gTileLength * getWorld()->getMapSize().x - gTileHalfLength;
							setFaceDir( DIR_WEST );
						}
						break;
					case DIR_WEST:
						if ( nextPos.y > yMax )
						{
							nextPos.y = gTileLength * getWorld()->getMapSize().y - gTileHalfLength;
							setFaceDir( DIR_NORTH );
						}
						break;
					case DIR_NORTH:
						if ( nextPos.x < gTileHalfLength )
						{
							nextPos.x = gTileHalfLength;
							setFaceDir( DIR_EAST );
						}
						break;
					case DIR_EAST:
						if ( nextPos.y < gTileHalfLength )
						{
							nextPos.y = gTileHalfLength;
							setFaceDir( DIR_SOUTH );
						}
						break;
					}
				}
				else if ( checkActionKey( Actor::ACT_MOVE_INV ))
				{
					 nextPos = getPos() + gGhostMoveSpeed * gTileLength * Vec2f( getDirOffset( InvMoveDirMap[ getFaceDir() ] ) );

					switch( getFaceDir() )
					{
					case DIR_WEST:	
						if ( nextPos.y < gTileHalfLength )
						{
							nextPos.y = gTileHalfLength;
							setFaceDir( DIR_SOUTH );
						}
						break;
					case DIR_SOUTH:
						if ( nextPos.x < gTileHalfLength )
						{
							nextPos.x = gTileHalfLength;
							setFaceDir( DIR_EAST );
						}
						break;
					case DIR_EAST:
						if ( nextPos.y > yMax )
						{
							nextPos.x = gTileLength * getWorld()->getMapSize().x - gTileHalfLength;
							setFaceDir( DIR_NORTH );
						}
						break;
					case DIR_NORTH:
						if ( nextPos.x > xMax )
						{
							nextPos.x = xMax;
							setFaceDir( DIR_WEST );
						}
						break;

					}
				}
				if ( checkActionKey( Actor::ACT_BOMB ) )
				{
					int const gGhostThrowDist = 3;

					if ( numUseBomb == 0 )
					{
						Vec2i pos = PosToTile( getPos() );
						bool beFire = false;
						switch( getFaceDir() )
						{
						case DIR_EAST:
						case DIR_WEST:
							if ( 0 < pos.y && pos.y < getWorld()->getMapSize().y - 1 )
								beFire = true;
							break;
						case DIR_SOUTH:
						case DIR_NORTH:
							if ( 0 < pos.x && pos.x < getWorld()->getMapSize().x - 1 )
								beFire = true;
							break;
						}
						if ( beFire )
						{
							++numUseBomb;
							int bIdx = getWorld()->addBomb( *this , Vec2i(-1,-1) , power , gBombFireFrame , 0 );
							getWorld()->throwBomb( bIdx  , pos , getFaceDir() , gGhostThrowDist );
						}
					}
				}
			}
			break;
		}


		setPos( nextPos );
	}

	void Player::postUpdate()
	{
		BaseClass::postUpdate();
	}

	int Player::checkTouchTileInternal( Vec2f const& pos , TileData* tiles[] , int numTile )
	{
		Vec2i tPos;
		TileData* tile = getWorld()->getTileWithPos( pos , tPos );
		if ( tile == NULL )
			return numTile;

		for( int i = 0 ; i < numTile ; ++i )
		{
			if ( tile == tiles[i] )
				return numTile;
		}
		tiles[ numTile ] = tile;
		++numTile;
		onTouchTile( *tile );

		return numTile;
	}

	void Player::onBombEvent( Bomb& bomb , BombEvent event )
	{
		--numUseBomb;
	}

	void Player::onTouchTile( TileData& tile )
	{
		switch( tile.obj )
		{
		case MO_ITEM:
			if ( tile.meta != ITEM_ROOEY || getOwnRooey() == RT_NONE )
			{
				addItem( Item( tile.meta ) );
				tile.obj   = MO_NULL;
				tile.meta  = 0;
			}
			break;
		case MO_BOMB:
			break;
		}
	}

	void Player::onCollision( ColObject* objPart , CollisionInfo const& info )
	{
		switch( info.type )
		{
		case COL_FIRE:
			if ( getState() == STATE_NORMAL )
				takeDamage();
			break;
		case COL_OBJECT:
			{
				switch( info.obj->getClient()->getType() )
				{
				case OT_BOMB:
					break;
				case OT_PLAYER:
					{
						Player* player = static_cast< Player* >( info.obj );
						if ( ( mPrevPlayerColBit & BIT( player->getId() ) ) == 0 &&
							 ( player->mPrevPlayerColBit & BIT( getId() ) ) == 0 )
						{
							SkullType otherDisease = player->getDisease();
							player->catchDisease( mDisease );
							catchDisease( otherDisease );
						}	
						mPlayerColBit |= BIT( player->getId() );
					}
					break;
				}
			}
		}
	}

	void Player::addItem( Item tool )
	{
		tools[ tool ] += 1;

		if ( tools[ ITEM_SKULL ] )
		{
			Vec2i size = getWorld()->getMapSize();
			Vec2i tPos;

			tPos.x = getWorld()->getRandom( size.x );
			tPos.y = getWorld()->getRandom( size.y );

			while( 1 )
			{
				TileData& tile = getWorld()->getTile( tPos );

				if ( tile.obj != MO_NULL )
				{
					tPos.x += 1;
					if ( tPos.x >= size.x )
					{
						tPos.x = 0;
						tPos.y += 1;
						if ( tPos.y >= size.y )
							tPos.y = 0;
					}
					continue;
				}

				MoveItem* itemObj = new MoveItem( ITEM_SKULL , getPos() , TileToPos( tPos ) );
				getWorld()->addEntity( itemObj , true );
				tools[ ITEM_SKULL ] -= 1;
				break;
			}
		}

		switch( tool )
		{
		case ITEM_SKULL:
			catchDisease( getWorld()->getAvailableDisease() );
			break;
		case ITEM_ROOEY:
			mRooeyOwned = getWorld()->getAvailableRooey();
			break;
		case ITEM_REMOTE_CTRL:
		case ITEM_BOMB_KICK:
		case ITEM_POWER_GLOVE:
		case ITEM_BOMB_PASS:
		case ITEM_LINE_BOMB:
		case ITEM_WALL_PASS:
		case ITEM_RUBBER_BOMB:
		case ITEM_PIERCE_BOMB:
			mToolFlag |= BIT( tool );
			break;
		case ITEM_POWER:
			power += 1;
			break;
		case ITEM_FULL_POWER:
			power += 5;
			break;
		case ITEM_BOMB_UP:
			maxNumBomb += 1;
			break;
		case ITEM_HEART:
			health += 1;
			break;
		}
	}

	TileData* Player::testBlocked( Vec2f const& pos )
	{
		TileData* tile = BaseClass::testBlocked( pos );

		if ( tile )
		{
			switch( tile->obj )
			{
			case MO_BOMB:
				if ( haveItem( ITEM_BOMB_PASS ) )
					return NULL;
				break;
			case MO_WALL:
				if ( haveItem( ITEM_WALL_PASS ) )
					return NULL;
				break;
			}
		}
		return tile;
	}

	bool Player::isAlive()
	{
		return mState != STATE_DIE && mState != STATE_GHOST;
	}

	void Player::spawn( bool beInvicible )
	{
		changeState( beInvicible ? STATE_INVICIBLE : STATE_NORMAL );
		getWorld()->getAnimManager()->setPlayerAnim( *this , ANIM_STAND );
	}

	void Player::onFireCollision( Bomb& bomb , ColObject& obj )
	{

	}

	void Player::move( Dir dir )
	{
		if ( getDisease() == SKULL_REVERSE )
			dir = getInvDir( dir );
		setFaceDir( dir );
		addActionKey( Actor::ACT_MOVE );
	}

	void Player::catchDisease( SkullType disease )
	{
		mDisease = disease;

		if ( mDisease == SKULL_NULL )
			return;

		switch( mDisease )
		{
		case SKULL_HICCUP:
			mDiseaseTimer = 0;
			break;
		}
	}

	bool Player::isVisible()
	{
		return getDisease() != SKULL_INVISIBILITY || 
			   ( getState() == STATE_DIE && mTimeState > 2000 );
	}

	void Player::onTravelEnd( MineCar& car )
	{

	}

	void Player::onTravelStart( MineCar& car )
	{

	}

	void Player::updateAction( Vec2f& nextPos )
	{
		mTimeAction += 1;

		if ( getAction() == AS_IDLE )
		{
			AnimAction anim = ANIM_STAND;
			Vec2i tPlayerPos = PosToTile( getPos() );

			if ( checkActionKey( ACT_MOVE ) || getDisease() == SKULL_NO_STOP )
			{
				float speed = gPlayerMoveSpeed;

				if ( getDisease() == SKULL_FAST_RUNNER )
					speed = gFastRunnerMoveSpeed;
				else if ( getDisease() == SKULL_SLOW_FOOT )
					speed = gSlowFootSpeed;
				else
				{
					if ( haveItem( ITEM_SPEED ) )
						speed *= 1.25f;
				}

				TileData* colTile[2];
				if ( moveOnTile( mFaceDir , speed , nextPos , colTile , 0.01f ) )
				{
					if ( haveItem( ITEM_BOMB_KICK ) )
					{
						if ( colTile[0] == colTile[1] && colTile[0]->obj == MO_BOMB &&
							tPlayerPos != getWorld()->getBomb( colTile[0]->meta ).pos  )
						{
							getWorld()->moveBomb( colTile[0]->meta , getFaceDir() , gKickBombSpeed , false );
						}
					}
				}

				anim = ANIM_WALK;
			}

			bool needPutBomb = ( getDisease() == SKULL_DIARRHEA );
			if ( checkActionKey( ACT_BOMB ) )
			{
				if ( mIdxBombTake == -1 )
				{
					if ( haveItem( ITEM_POWER_GLOVE ) )
					{
						Vec2i tPos;
						TileData* tile = getWorld()->getTileWithPos( getPos() , tPos );
						if ( tile && tile->obj == MO_BOMB )
						{
							Bomb& bomb = getWorld()->getBomb( tile->meta );

							if ( !( bomb.flag & BF_UNMOVABLE ) ) 
							{
								mIdxBombTake = tile->meta;

								bomb.state = Bomb::eTAKE;

								tile->obj  = MO_NULL;
								tile->meta = 0;
							}
						}
					}

					if ( mIdxBombTake == -1 && getDisease() != SKULL_INABILITY )
					{
						needPutBomb = true;
					}
				}
				else
				{

					getWorld()->throwBomb( mIdxBombTake , tPlayerPos , getFaceDir() , gPowerGloveThrowDist );
					mIdxBombTake = -1;
				}
			}

			if ( needPutBomb )
			{
				if ( numUseBomb < maxNumBomb && getWorld()->getTile( tPlayerPos ).obj == MO_NULL )
				{
					unsigned flag = 0;
					if ( haveItem( ITEM_REMOTE_CTRL ) )
						flag |= BF_CONTROLLABLE;
					if ( haveItem( ITEM_LIQUID_BOMB ) )
						flag |= BF_UNMOVABLE;
					if ( haveItem( ITEM_RUBBER_BOMB ) )
						flag |= BF_BOUNCING;

					int timerFame = gBombFireFrame;
					int curPower = getStatus().power;

					switch( getDisease() )
					{
					case SKULL_LOW_POWER: curPower = 1; break;
					case SKULL_HASTY:     timerFame /= 2; break;
					case SKULL_LEISURE:   timerFame *= 2; break;
					}

					getWorld()->addBomb( *this , tPlayerPos , curPower , timerFame , flag );
					++numUseBomb;

					if ( haveItem( ITEM_LINE_BOMB ) )
					{
						int num = getStatus().maxNumBomb - getStatus().numUseBomb;
						for( int i = 0 ; i < num ; ++i )
						{
							Vec2i tPos = tPlayerPos + getDirOffset( getFaceDir() );

							TileData& tile = getWorld()->getTile( tPos );

							if ( tile.obj != MO_NULL )
								break;

							getWorld()->addBomb( *this , tPos , curPower , timerFame , flag );
							++numUseBomb;
						}
					}
				}
			}

			if ( checkActionKey( ACT_FUN ) )
			{
				switch ( getOwnRooey() )
				{
				case RT_NONE:
					break;
				case RT_GYAROOEY:
					{
						float gRange = 0.3 * gTileLength;
						Vec2f testPos = getPos() + gRange * Vec2f( getDirOffset( getFaceDir()) );
						Vec2i tPos = PosToTile( testPos );

						if ( tPos != PosToTile( getPos() ) )
						{
							TileData& tile = getWorld()->getTile( tPos );
							if ( tile.obj == MO_WALL )
							{

							}
						}
					}
					break;

				}
				if ( haveItem( ITEM_BOXING_GLOVE ) )
				{




				}
			}
			getWorld()->getAnimManager()->setPlayerAnim( *this , anim );
		}
	}

	void Player::changeAction( Action act )
	{
		if ( mAction == act )
			return;
		mAction     = act;
		mTimeAction = 0;
	}

	MoveBomb::MoveBomb( int idx , Vec2i const& pos , Dir dir  ) 
		:BaseClass( idx )
		,mMoveDir( dir )
	{
		mStopReady = false;
		setColMask( CMB_SOILD | CMB_FIRE );

		Vec2f bSize( gTileHalfLength , gTileHalfLength );
		mTrigger.setHalfBoundSize( bSize );
		mTrigger.setColMask( CMB_SOILD );
		mTrigger.collisionFun = Trigger::CollisionFun( this , &MoveBomb::onTiggerCollision );
		mTrigger.updateFun    = Trigger::UpdateFun( this , &MoveBomb::onTiggerUpdate );

		setHalfBoundSize( bSize );
		setPos( TileToPos( pos ) + gTileCenterOffset );
	}

	void MoveBomb::update()
	{
		Bomb& bomb = getData();

		Vec2f offset = mMoveSpeed * gTileLength * Vec2f( getDirOffset( mMoveDir ) );

		if ( ( bomb.flag & BF_CONTROLLABLE ) &&
			   bomb.owner->checkActionKey( Player::ACT_FUN ) )
			mStopReady = true;

		if ( bomb.timer <= 0 )
			mStopReady = true;

		if ( mStopReady )
		{
			float goalDiff;
			switch( mMoveDir )
			{
			case DIR_EAST:   
				goalDiff =  getPos().x - floor(getPos().x); 
				if ( goalDiff > gTileHalfLength ) 
					goalDiff = 1.5f * gTileLength - goalDiff;
				else
					goalDiff = gTileHalfLength - goalDiff;
				break;
			case DIR_SOUTH:  				
				goalDiff =  getPos().y - floor(getPos().y); 
				if ( goalDiff > gTileHalfLength ) 
					goalDiff = 1.5f * gTileLength - goalDiff;
				else
					goalDiff = gTileHalfLength - goalDiff;
				break;
			case DIR_NORTH:				
				goalDiff =  getPos().y - floor(getPos().y); 
				if ( goalDiff > gTileHalfLength ) 
					goalDiff = goalDiff - gTileHalfLength;
				else
					goalDiff = goalDiff + gTileHalfLength;
				break;

			case DIR_WEST:
				goalDiff =  getPos().x - floor(getPos().x); 
				if ( goalDiff > gTileHalfLength ) 
					goalDiff = goalDiff - gTileHalfLength;
				else
					goalDiff = goalDiff + gTileHalfLength;
				break;
			}

			if ( goalDiff < mMoveSpeed )
			{
				mbStopMotion = true;
				return;
			}
		}

		Vec2f testPos = getPos() + offset + gTileHalfLength * Vec2f( getDirOffset( mMoveDir ) );
	
		Vec2i tTestPos;
		TileData* tile = getWorld()->getTileWithPos( testPos , tTestPos );
		if ( tile )
		{
			if ( tile->isBlocked() )
			{
				if ( bomb.flag & BF_BOUNCING )
				{
					if ( tile->obj == MO_WALL || tile->obj == MO_BLOCK )
					{
						mMoveDir = getInvDir( mMoveDir );
						setPos( TileToPos( tTestPos + getDirOffset( mMoveDir ) ) + gTileCenterOffset );
						mTrigger.setPos( getPos() + gTileLength * Vec2f( getDirOffset( mMoveDir ) ) );
						return;
					}
				}

				mbStopMotion = true;
				return;

			}
			if ( tile->obj == MO_ITEM )
			{
				getWorld()->breakTileObj( PosToTile( testPos ) );
			}
		}

		setPos( getPrevPos() + offset );
		mTrigger.setPos( getPos() + gTileLength * Vec2f( getDirOffset( mMoveDir ) ) );
	}

	void MoveBomb::postUpdate()
	{
		BaseClass::postUpdate();

		Vec2i tPos = PosToTile( getPos() );
		TileData& tile = getWorld()->getTile( tPos );

		if ( ( MT_ARROW <= tile.terrain && tile.terrain < MT_ARROW_END ) ||
			 ( MT_ARROW_CHANGE <= tile.terrain && tile.terrain < MT_ARROW_CHANGE_END ) )
		{
			float movePos = getPos()[ mMoveDir % 2 ];
			float diff = gDirFactor[ mMoveDir ] * ( floor( movePos ) + gTileHalfLength - movePos );

			if ( 0 < diff && diff <= mMoveSpeed )
			{
				if ( MT_ARROW <= tile.terrain && tile.terrain < MT_ARROW_END ) 
					mMoveDir = Dir( tile.terrain - MT_ARROW );
				else
					mMoveDir = Dir( tile.terrain - MT_ARROW_CHANGE );
			}
		}
	}

	void MoveBomb::onCollision( ColObject* objPart , CollisionInfo const& info )
	{
		switch( info.type )
		{
		case COL_FIRE:
			mbStopMotion = true;
			getData().timer = 0;
			break;
		case COL_OBJECT:
			{
				if ( info.obj->getClient()->getType() == OT_BOMB )
				{
					Bomb& bomb = getData();
					Bomb& otherBomb = static_cast< BombObject* >( info.obj )->getData();
					if ( bomb.state == Bomb::eMOVE && otherBomb.state == Bomb::eMOVE )
					{
						Vec2f offset = getPos() - info.obj->getClient()->getPos();
						float const gBombMergeDist = 0.3;
						if (  offset.length2() < gBombMergeDist * gBombMergeDist )
						{
							if ( bomb.flag & BF_DANGEROUS )
							{
								bomb.power += 1;
							}
							else
							{
								bomb.flag |= BF_DANGEROUS;
								bomb.power = 2;
							}
	
							bomb.timer = gBombFireFrame;
							mbStopMotion = true;
							otherBomb.state = Bomb::eDESTROY;
						}	
					}
				}
			}
			break;
		}
	}

	void MoveBomb::onTiggerCollision( ColObject* objPart , CollisionInfo const& info )
	{
		switch( info.type )
		{
		case COL_OBJECT:
			{
				if ( info.obj->getClient()->getType() != OT_BOMB && 
					 info.obj->getClient()->getType() != OT_MINE_CAR )
				{
					mStopReady = true;
				}
			}
			break;
		}
	}

	void MoveBomb::onSpawn()
	{
		BaseClass::onSpawn();
		Bomb& bomb = getData();
		getWorld()->addEntity( &mTrigger , false );
	}

	void MoveBomb::onDestroy()
	{
		getWorld()->removeEntity( &mTrigger );
		BaseClass::onDestroy();
	}

	void MoveBomb::onTiggerUpdate()
	{

	}

	JumpBomb::JumpBomb( int idx , Vec2i const& pos , Dir dir , int jumpDist ) 
		:BaseClass( idx )
	{
		setPos( TileToPos(pos) + gTileCenterOffset );
		setHalfBoundSize( Vec2f( gTileHalfLength , gTileHalfLength ) );
		mCurTilePos = pos;
		mMoveDir    = dir;
		mJumpDist   = jumpDist;
		setColMask( CMB_SOILD );
		mDistance   = 0;
		mTimer      = 0;
	}

	void JumpBomb::update()
	{
		int const AdvanceTime = 10;
		mTimer += 1;
		if ( mTimer >= AdvanceTime )
		{
			mTimer = 0;
			++mDistance;
			mCurTilePos += getDirOffset( mMoveDir );
			getWorld()->warpTilePos( mCurTilePos );

			mHaveBlocked = false;
		}

		setPos( TileToPos( mCurTilePos ) + ( float( mTimer ) / float(AdvanceTime) ) * gTileLength * Vec2f( getDirOffset( mMoveDir ) ) + gTileCenterOffset );
	}

	void JumpBomb::postUpdate()
	{
		BaseClass::postUpdate();

		if ( mDistance >= mJumpDist && mTimer == 0 && !mHaveBlocked )
		{
			if ( mDistance == mJumpDist )
			{
				if ( getData().flag & BF_BOUNCING )
					mMoveDir = Dir( getWorld()->getRandom( 4 ) );
			}

			TileData& tile = getWorld()->getTile( mCurTilePos );
			if ( !tile.isBlocked() )
			{
				if ( tile.obj == MO_ITEM )
				{
					getWorld()->breakTileObj( mCurTilePos );
				}
				mbStopMotion = true;
			}
		}
	}

	void JumpBomb::onCollision( ColObject* objPart , CollisionInfo const& info )
	{
		switch( info.type )
		{
		case COL_OBJECT:
			if ( mDistance >= mJumpDist && mTimer == 0 )
			{
				mHaveBlocked = true;
				if ( info.obj->getClient()->getType() == OT_PLAYER )
				{
					Player* player = static_cast< Player* >( info.obj->getClient() );
					if ( player->getAction() == Player::AS_IDLE )
						player->changeAction( Player::AS_FAINT );
				}
			}
			break;
		}
	}

	BombObject::BombObject( int idx ) 
		:BaseClass( OT_BOMB )
		,mIndex( idx )
		,mbStopMotion( false )
	{

	}

	Bomb& BombObject::getData()
	{
		return getWorld()->getBomb( mIndex );
	}


	void AIQuery::setup( World& world )
	{
		mWorld = &world;
		mFireCount.resize( world.getMapSize().x , world.getMapSize().y );
	}

	void AIQuery::updateMapInfo()
	{
		mFireCount.fillValue( 0 );
		mDangerCount.fillValue( 0 );
		for( World::BombIterator iter = mWorld->getBombs();
			iter.haveMore() ; iter.goNext() )
		{
			Bomb& bomb = iter.getBomb();
			if ( bomb.state >= 0 )
			{

			}
		}
	}


	ColEntity::ColEntity( ObjectType type ) 
		:BaseClass( type )
		,ColObject( this )
	{

	}

	void ColEntity::onSpawn()
	{
		getWorld()->addColObject( *this );
	}

	void ColEntity::onDestroy()
	{
		getWorld()->removeColObject( *this );
	}

	Entity::Entity( ObjectType type ) 
		:mType( type )
	{
		mWorld = NULL;
		mNeedRemove = false;
	}

	void Entity::setPosWithTile( Vec2i const& tPos )
	{
		mPos = TileToPos( tPos ) + gTileCenterOffset;
	}


	MoveItem::MoveItem( Item item , Vec2f const& from , Vec2f const& to ) 
		:BaseClass( OT_ITEM )
		,mItem( item )
		,mDest( to )
	{
		setPos( from );
	}

	void MoveItem::update()
	{
		float const moveSpeed = 0.01;

		Vec2f offset = mDest - getPrevPos();
		float len2 = offset.length2();

		if ( len2 < moveSpeed * moveSpeed )
		{
			TileData& tile = getWorld()->getTile( PosToTile( mDest ) );
			if ( tile.obj == MO_NULL )
			{
				tile.obj  = MO_ITEM;
				tile.meta = mItem;
				getWorld()->removeEntity( this );
				return;
			}
		}
		setPos( getPrevPos() + (moveSpeed / sqrt( len2 ) ) * offset );
	}


	MineCar::MineCar( Vec2i const& tPos )
		:BaseClass( OT_MINE_CAR )
	{
		setColMask( CMB_SOILD | CMB_TRIGGER );
		setHalfBoundSize( gTileCenterOffset );
		setPosWithTile( tPos );
		mRider = NULL;
	}

	void MineCar::onCollision( ColObject* objPart , CollisionInfo const& info )
	{
		switch ( info.type )
		{
		case COL_OBJECT:
			if ( info.obj->getClient()->getType() == OT_PLAYER )
			{
				Player* player = static_cast< Player* >( info.obj );
				if ( isIdle() && player->getAction() == Player::AS_IDLE )
				{
					mRider = player;
					mRider->changeAction( Player::AS_ENTER_CAR );
					mState = eWAIT_IN;
					int idx = mRider->getFaceDir() % 2;
					mOffset = fabs( getPos()[ idx ] - mRider->getPos()[ idx ] );
				}
				else if ( player != mRider )
				{
					player->takeDamage();
				}
			}
			break;
		}
	}


	void MineCar::update()
	{
		if ( !mRider )
			return;

		int gWaitTime = 20;
		switch ( mState )
		{
		case eWAIT_IN:
			mRider->setPos( getPos() - mOffset * ( 1.0 - float( mRider->getActionTime() ) / gWaitTime ) * Vec2f( getDirOffset( mRider->getFaceDir() ) )  );
			if ( mRider->getActionTime() >= gWaitTime )
			{
				Vec2i tPos;
				int dir = 0;
				int nextDir = 0;
				Vec2i tPosCur = PosToTile( getPos() );
				for ( ; dir < 4 ; ++dir )
				{
					tPos = tPosCur + getDirOffset( dir );
					TileData& tile = getWorld()->getTile( tPos );
					if ( MT_RAIL <= tile.terrain && tile.terrain < MT_RAIL_END )
					{
						if ( isRailConnect( tile.terrain , getInvDir( Dir(dir) ) , nextDir ) )
							break;
					}
				}
				if ( dir == 4 )
				{
					mState = eWAIT_OUT;
				}

				mGoalTilePos = tPos;
				mMoveDir     = Dir( dir );
				mNextDir     = Dir( nextDir );
				mOffset      = gTileLength;

				mRider->changeAction( Player::AS_RIDING );
				mRider->setFaceDir( mMoveDir );
				mState = eMOVE;
			}
			break;
		case eMOVE:
			{
				float const gMineCarMoveSpeed = 0.12;
				mOffset -= gMineCarMoveSpeed;

				Vec2f nextPos;

				if ( mOffset <= 0 )
				{
					Vec2i tPos = mGoalTilePos + getDirOffset( mNextDir );
					TileData& tile = getWorld()->getTile( tPos );

					int nextDir;
					if ( MT_RAIL <= tile.terrain && tile.terrain < MT_RAIL_END &&
						isRailConnect( tile.terrain , getInvDir( mNextDir ) , nextDir ) )
					{
						mMoveDir = mNextDir;
						mNextDir = Dir( nextDir );
						mGoalTilePos = tPos;
						mOffset += gTileLength;

						mRider->setFaceDir( mMoveDir );
						nextPos = TileToPos( mGoalTilePos ) + gTileCenterOffset + mOffset * Vec2f( getDirOffset( getInvDir( mMoveDir ) ) );
					}
					else
					{
						mRider->changeAction( Player::AS_LEAVE_CAR );
						mState = eWAIT_OUT;
						nextPos = TileToPos( mGoalTilePos ) + gTileCenterOffset;

						for( int i = 0 ; i < 4 ; ++i )
						{
							mMoveDir = Dir( ( mMoveDir + i ) % 4 );
							Vec2i pos =  mGoalTilePos + getDirOffset( mMoveDir );
							TileData& tile = getWorld()->getTile( pos );
							if ( !tile.isBlocked() )
							{
								mGoalTilePos = pos;
								break;
							}
						}
					}
				}
				else
				{
					nextPos = TileToPos( mGoalTilePos ) + gTileCenterOffset + mOffset * Vec2f( getDirOffset( getInvDir( mMoveDir ) ) );
				}

				setPos( nextPos );
				mRider->setPos( nextPos );
			}
			break;
		case eWAIT_OUT:
			if ( mRider->getActionTime() >= gWaitTime )
			{
				mRider->changeAction( Player::AS_IDLE );
				mRider->setPosWithTile( mGoalTilePos );

				mRider = NULL;
			}
			else
			{
				mRider->setPos( getPos() + gTileLength * ( float( mRider->getActionTime() ) / gWaitTime ) * Vec2f( getDirOffset( mMoveDir ) ) );
			}
			break;
		}
	}

	void MineCar::postUpdate()
	{
		BaseClass::postUpdate();
	}

}//namespace BomberMan