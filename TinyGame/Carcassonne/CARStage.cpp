#include "CARStage.h"

#include "CARPlayer.h"

#include "DrawEngine.h"
#include "RenderUtility.h"

#include "GameGUISystem.h"
#include "Math/Base.h"
#include "PropertyKey.h"

namespace CAR
{
	float const RenderScale = 60;

	static Vec2f const SidePos[] = { Vec2f(0.5,0) , Vec2f(0,0.5) , Vec2f(-0.5,0) , Vec2f(0,-0.5) };
	float const farmOffset = 0.425;
	float const farmOffset2 = 0.25;

	static Vec2f const FarmPos[] = 
	{ 
		Vec2f( farmOffset ,-farmOffset2 ) , Vec2f( farmOffset , farmOffset2 ) , Vec2f( farmOffset2 , farmOffset ) , Vec2f( -farmOffset2 , farmOffset ) , 
		Vec2f( -farmOffset , farmOffset2 ) , Vec2f( -farmOffset , -farmOffset2 ) , Vec2f( -farmOffset2 , -farmOffset ) , Vec2f( farmOffset2 , -farmOffset ) , 
	};

	Vec2i RenderTileSize = RenderScale * Vec2f( 1 , 1 );

	bool gDrawFarmLink = true;
	bool gDrawSideLink = true;
	bool gDrawFeatureInfo = true;

	extern TileDefine DataBasic[];
	bool LevelStage::onInit()
	{
		//if ( !::Global::getDrawEngine()->startOpenGL() )
		//	return false;

		{
			using namespace CFly;
			if ( !CFly::initSystem() )
				return false;

			GameWindow& window = ::Global::getDrawEngine()->getWindow();

			mWorld = CFly::createWorld( window.getHWnd() , window.getWidth() , window.getHeight() , 32 , false );
			if ( mWorld == nullptr )
			{
				return false;
			}

			mRenderOffset = Vec2f(0,0);

			mViewport = mWorld->createViewport( 0 , 0 , window.getWidth() , window.getHeight() );

			D3DDevice* d3dDevice = mWorld->getD3DDevice();

			d3dDevice->CreateOffscreenPlainSurface( window.getWidth() , window.getHeight() , D3DFMT_X8R8G8B8 , D3DPOOL_SYSTEMMEM , &mSurfaceBufferTake , NULL );
			

			mScene = mWorld->createScene( 1 );

			mWorld->setDir( DIR_TEXTURE , "CAR" );

			mScene->setAmbientLight( Color4f(1,1,1) );

			mCamera = mScene->createCamera();


			mCamera->setProjectType(CFPT_ORTHOGONAL);
			float factor = float(  window.getWidth() ) / window.getHeight();
			float len = window.getWidth() / RenderScale;
			mCamera->setScreenRange( 0 , len , len / factor , 0 );
			mCamera->setNear( -10000 );
			mCamera->setFar( 10000 );
			//mCamera->setAspect( float(  window.getWidth() ) / window.getHeight() );
			//mCamera->setNear(0.1f);
			//mCamera->setFar(100000.0f);


			setRenderOffset( Vec2f(5,5));

			//mCamera->setWorldPosition(  );

			{
				mTileShowObject = createTileObject();
				mTileShowObject->show( false );

			}
			{

				CFly::Object* obj = mScene->createObject( nullptr );
				CFly::Material* mat;

				float len = 10;
				Vector3 line[2];
				line[0] = Vector3(0,0,0);

				line[1] = Vector3( len ,0,0 );
				mat = mWorld->createMaterial( NULL , NULL , NULL , 0 , Color4f(1,0,0) );
				obj->createLines( mat ,CF_LINE_SEGMENTS , (float*) &line[0] , 2  );

				line[1] = Vector3(0,len,0);
				mat = mWorld->createMaterial( NULL , NULL , NULL , 0 , Color4f(0,1,0) );
				obj->createLines( mat ,CF_LINE_SEGMENTS , (float*) &line[0]  , 2 );

				line[1] = Vector3(0,0,len );
				mat = mWorld->createMaterial( NULL , NULL , NULL , 0 , Color4f(0,0,1) );
				obj->createLines( mat , CF_LINE_SEGMENTS , (float*) &line[0] , 2 );

				obj->setRenderOption( CFRO_LIGHTING , true );
			}
		}

		mRotation = 0;
		mCurMapPos = Vec2i(0,0);
		mIdxShowFeature = 0;

		mMoudule.mDebug = ::Global::getSetting().getIntValue( "Debug" , "CAR" , 0 ) != 0;
		mMoudule.mListener = this;
		
		mCoroutine.onAction = std::bind( &LevelStage::onGameAction , 
			this , std::placeholders::_1 , std::placeholders::_2 , std::placeholders::_3 );

		for( int i = 0 ; i < 2 ; ++i )
		{
			PlayerBase* player = new PlayerBase;
			player->mTeam = 0;
			mPlayerManager.addPlayer( player );
		}

		mSetting.mExpansionMask = 
			//BIT( EXP_THE_RIVER ) | 
			BIT( EXP_TRADEERS_AND_BUILDERS ) | 
			BIT( EXP_INNS_AND_CATHEDRALS ) |
			BIT( EXP_THE_PRINCESS_AND_THE_DRAGON ) |
			BIT( EXP_THE_TOWER );
		mSetting.calcUsageField( mPlayerManager.getPlayerNum() );

		mMoudule.mPlayerManager = &mPlayerManager;
		mMoudule.setupSetting( mSetting );

		::Global::getGUI().cleanupWidget();

		return true;
	}

	void LevelStage::onEnd()
	{
		mCoroutine.exitGame();
		CFly::cleanupSystem();
		//::Global::getDrawEngine()->enableSweepBuffer( true );
		//::Global::getDrawEngine()->stopOpenGL();
	}

	void LevelStage::onRestart( uint64 seed , bool bInit)
	{
		mMoudule.restart( bInit );
		
		if ( ::Global::getSetting().getIntValue( "LoadGame" , "CAR" , 1 )  )
		{
			char const* file = ::Global::getSetting().getStringValue( "LoadGameName" , "CAR" , "car_record2" ) ;
			mCoroutine.load(file);
		}
		
		std::function< void ( IGameCoroutine& ) > fun = std::bind( &GameModule::runLogic , &mMoudule , std::placeholders::_1 );
		mCoroutine.run( fun );
	}

	void LevelStage::onRender(float dFrame)
	{
		::Graphics2D& g = Global::getGraphics2D();
		{
			mScene->render( mCamera , mViewport );

			IDirect3DSurface9* pBackBufferSurface;
			mWorld->getD3DDevice()->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBufferSurface);
			HRESULT hr = mWorld->getD3DDevice()->GetRenderTargetData( pBackBufferSurface ,mSurfaceBufferTake );
			pBackBufferSurface->Release();

			int w = ::Global::getDrawEngine()->getScreenWidth();
			int h = ::Global::getDrawEngine()->getScreenHeight();

			HDC hDC;
			mSurfaceBufferTake->GetDC(&hDC);
			::BitBlt(  g.getRenderDC() ,0 , 0, w , h , hDC , 0 , 0 , SRCCOPY );
			mSurfaceBufferTake->ReleaseDC(hDC);
		}

		Level& level = mMoudule.getLevel();
		for ( Level::LevelTileMap::iterator iter = level.mMap.begin() , itEnd = level.mMap.end();
			iter != itEnd ; ++iter )
		{
			RenderUtility::setBrush( g , Color::eNull );
			RenderUtility::setPen( g , Color::eGray );

			MapTile& mapData = iter->second;

			g.drawRect( calcRenderPos( mapData.pos ) - RenderTileSize / 2   , RenderTileSize );
			drawMapData( g , mapData.pos , mapData );
		}

		if ( mMoudule.mIsStartGame )
		{
			FixString< 512 > str;
			g.drawText( 10 , 200 , str.format( "pID = %d tileId = %d , rotation = %d , pos = %d , %d " , mMoudule.getTurnPlayer()->getId() ,
				mMoudule.mUseTileId , mRotation , mCurMapPos.x , mCurMapPos.y ) );
		}

		{

			Vec2i pos = Vec2i( 10 , 500 );
			for( int i = 0 ; i < mPlayerManager.getPlayerNum() ; ++i )
			{
				pos = showPlayerInfo( g , pos , mPlayerManager.getPlayer(i) , 12 );
				pos.y += 2;
			}
		}
		if ( gDrawFeatureInfo )
		{
#if 0
			Vec2i pos = Vec2i( 300 , 20 );
			for(int i = 0 ; i < mMoudule.mFeatureMap.size() ; ++i )
			{
				FeatureBase* build = mMoudule.mFeatureMap[i];
				if ( build->group == -1 )
					continue;
				pos = showBuildInfo( g , pos , build , 12 );
				pos.y += 2;
			}
#endif

			if ( !mMoudule.mFeatureMap.empty() )
			{
				FeatureBase* feature = mMoudule.mFeatureMap[ mIdxShowFeature ];

				Vec2i pos = Vec2i( 300 , 20 );
				showFeatureInfo( g , pos , feature , 12 );

				if ( feature->group != -1 )
				{
					for( std::set< MapTile*>::iterator iter = feature->mapTiles.begin() , itEnd = feature->mapTiles.end() ;
						iter != itEnd ; ++iter )
					{
						RenderUtility::setBrush( g , Color::eNull );
						RenderUtility::setPen( g , Color::eRed );
						g.drawRect( calcRenderPos( (*iter)->pos ) - RenderTileSize / 2  , RenderTileSize );
					}

					switch( feature->type )
					{
					case FeatureType::eCity:
					case FeatureType::eRoad:
						{
							RenderUtility::setBrush( g , Color::eRed );
							RenderUtility::setPen( g , Color::eRed );
							SideFeature* sideFeature = static_cast< SideFeature* >( feature );
							for( std::vector< SideNode* >::iterator iter = sideFeature->nodes.begin() , itEnd = sideFeature->nodes.end();
								iter != itEnd ; ++iter )
							{
								SideNode* node = *iter;
								Vec2i size( 4 , 4 );
								g.drawRect( calcRenderPos( Vec2f( node->getMapTile()->pos ) + SidePos[ node->index ] ) - size / 2 , size );
							}
						}
						break;
					case FeatureType::eFarm:
						{
							RenderUtility::setBrush( g , Color::eRed );
							RenderUtility::setPen( g , Color::eRed );
							FarmFeature* farm = static_cast< FarmFeature* >( feature );
							for( std::vector< FarmNode* >::iterator iter = farm->nodes.begin() , itEnd = farm->nodes.end();
								iter != itEnd ; ++iter )
							{
								FarmNode* node = *iter;
								Vec2i size( 4 ,4 );
								g.drawRect( calcRenderPos( Vec2f(  node->getMapTile()->pos ) + FarmPos[ node->index ] ) - size / 2 , size );
							}
						}
						break;
					}
				}
			}

		}

		for( int i = 0 ; i < mMoudule.mActorList.size() ; ++i )
		{
			LevelActor* actor = mMoudule.mActorList[i];

			if ( actor->mapTile == nullptr )
				continue;

			Vec2f mapPos = Vec2f( actor->mapTile->pos );
			if ( actor->feature )
			{
				switch( actor->pos.type )
				{
				case ActorPos::eSideNode:
					mapPos += 0.7 * SidePos[ actor->pos.meta ];
					break;
				case ActorPos::eFarmNode:
					mapPos += FarmPos[ actor->pos.meta ];
					break;
				}
			}

			RenderUtility::setBrush( g , Color::eWhite );
			RenderUtility::setPen( g , Color::eWhite );

			switch( actor->type )
			{
			case ActorType::eMeeple:
				g.drawCircle( calcRenderPos( mapPos ) , 10 );
				break;
			case ActorType::eBigMeeple:
				g.drawCircle( calcRenderPos( mapPos ) , 15 );
				break;
			case ActorType::eBuilder:
				{
					Vec2f size( 10 , 20 );
					g.drawRect( calcRenderPos( mapPos ) - size / 2 ,  size );
				}
				break;
			case ActorType::eDragon:
				{
					Vec2f size( 20 , 20 );
					g.drawRect( calcRenderPos( mapPos ) - size / 2 ,  size );
					g.setTextColor( 255 , 0 , 0 );
					g.drawText( calcRenderPos( mapPos ) - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , "D" );
				}
				break;
			case ActorType::eFariy:
				{
					Vec2f size( 20 , 20 );
					g.drawRect( calcRenderPos( mapPos ) - size / 2 ,  size );
					g.setTextColor( 255 , 0 , 0 );
					g.drawText( calcRenderPos( mapPos ) - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , "F" );
				}
				break;
			}

			if ( actor->owner )
			{
				FixString< 128 > str;
				g.setTextColor( 125 , 125 , 0 );
				g.drawText( calcRenderPos( mapPos ) - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , str.format( "%d" , actor->owner->getId() ) );
			}
		}

		if ( mCoroutine.getAction() == ACTION_PLACE_TILE )
		{
			RenderUtility::setBrush( g , Color::eNull );
			RenderUtility::setPen( g , Color::eWhite );
			g.drawRect( calcRenderPos( mCurMapPos ) - RenderTileSize / 2  , RenderTileSize );
			if ( level.findMapTile( mCurMapPos ) == nullptr )
			{
				drawTile( g , Vec2f(mCurMapPos) , level.getTile( mMoudule.mUseTileId ) , mRotation );
			}

			for( int i = 0  ; i < mMoudule.mPlaceTilePosList.size() ; ++i )
			{
				Vec2i pos = mMoudule.mPlaceTilePosList[i];
				RenderUtility::setBrush( g , Color::eNull );
				RenderUtility::setPen( g , Color::eGreen );
				g.drawRect( calcRenderPos( pos ) - RenderTileSize / 2  , RenderTileSize );
			}
		}

		char const* actionStr = "Unknown Action";
		switch( mCoroutine.getAction() )
		{
		case ACTION_SELECT_ACTOR_INFO:
		case ACTION_SELECT_MAPTILE:
		case ACTION_SELECT_ACTOR:
			{
				GameSelectActionData* myData = static_cast< GameSelectActionData* >( mCoroutine.getActionData() );
#define CASE( ID , STR )\
		case ID: actionStr = STR; break;
				
				switch( myData->reason )
				{
				CASE(SAR_CONSTRUCT_TOWER , "Construct Tower")
				CASE(SAR_MOVE_DRAGON, "Move Dragon")
				CASE(SAR_WAGON_MOVE_TO_FEATURE, "Wagon Move To Feature")
				CASE(SAR_FAIRY_MOVE_NEXT_TO_FOLLOWER, "Move Fairy")
				CASE(SAR_TOWER_CAPTURE_FOLLOWER, "Tower Capture Follower")
				CASE(SAR_PRINCESS_REMOVE_KINGHT, "Princess Remove Kinght")
				CASE(SAR_EXCHANGE_PRISONERS, "Exchange Prisoners")
				CASE(SAR_MAGIC_PORTAL,"Magic Portal")
				}

				
			}
			break;
		case ACTION_PLACE_TILE:
			actionStr = "Place Tile";
			break;
		case ACTION_DEPLOY_ACTOR:
			{
				actionStr = "Deploy Actor";
				GameDeployActorData* myData = static_cast< GameDeployActorData* >( mCoroutine.getActionData() );

				RenderUtility::setBrush( g , Color::eNull );
				RenderUtility::setPen( g , Color::eWhite );
				Vec2f posBase  = Vec2f( myData->mapTile->pos );
				for( int i = 0 ; i < mGameActionUI.size() ; ++i )
				{
					GWidget* widget = mGameActionUI[i];

					if ( widget->getID() == UI_ACTOR_POS_BUTTON )
					{
						ActorPosButton* button = widget->cast< ActorPosButton >();
						ActorPosInfo& info = mMoudule.mActorDeployPosList[ button->indexPos ];
						Vec2f posNode = calcActorMapPos( info.pos , *myData->mapTile );
						//posBase + getActorPosMapOffset( info.pos );
						g.drawLine( button->getPos() + button->getSize() / 2 , calcRenderPos( posNode ) );

					}
				}
			}
			break;
		}

		g.drawText( 400 , 10  , actionStr );
	}

	Vec2f LevelStage::calcRenderPos(Vec2f const& posMap )
	{
		return RenderScale * ( mRenderOffset + posMap );
	}

	void LevelStage::drawMapData( Graphics2D& g , Vec2f const& mapPos , MapTile const& mapData)
	{

		if ( gDrawFarmLink )
		{
			unsigned maskAll = 0;
			
			for( int i = 0; i < Tile::NumFarm ; ++i )
			{
				int color = Color::eCyan;
				MapTile::FarmNode const& node = mapData.farmNodes[i];

				if ( node.outConnect )
				{
					RenderUtility::setPen( g , color , COLOR_DEEP );
					g.drawLine( calcRenderPos( mapPos + FarmPos[i] + ( 0.5 - farmOffset ) * SidePos[ Tile::FarmSideDir( i ) ]  ) ,
						        calcRenderPos( mapPos + FarmPos[i] )  );
				}

				if ( ( maskAll & BIT(i) ) == 0 )
				{
					unsigned mask = mapData.getFarmLinkMask( i );
					maskAll |= ( mask );

					int indexPrev = i;
					while( mask )
					{
						unsigned bit = FBit::Extract( mask );
						int idx = FBit::ToIndex8( bit );
						RenderUtility::setPen( g , color , COLOR_DEEP );
						g.drawLine( calcRenderPos( mapPos + FarmPos[indexPrev] ) , calcRenderPos( mapPos + FarmPos[idx] ) );
						mask &= ~bit;
						indexPrev = idx;
					}

					RenderUtility::setPen( g , color , COLOR_DEEP );
					g.drawLine( calcRenderPos( mapPos + FarmPos[indexPrev] ) , calcRenderPos( mapPos + FarmPos[i] ) );
				}
			}
		}

		drawTile( g , mapPos , *mapData.mTile , mapData.rotation );
	}

	void LevelStage::drawTile(Graphics2D& g , Vec2f const& mapPos , Tile const& tile , int rotation )
	{

		if ( gDrawSideLink )
		{
			for( int i = 0; i < Tile::NumSide ; ++i )
			{
				int dir = FDir::ToWorld( i , rotation );

				int color = Color::eWhite;
				switch( tile.getLinkType( i ) )
				{
				case SideType::eCity:   color = Color::eOrange; break;
				case SideType::eField:  color = Color::eGreen; break;
				case SideType::eRiver:  color = Color::eBlue; break;
				case SideType::eRoad:   color = Color::eYellow; break;
				}

				RenderUtility::setBrush( g , color , COLOR_NORMAL );
				RenderUtility::setPen( g , color , COLOR_NORMAL );
				Vec2i size( 6 ,6 );
				g.drawRect( calcRenderPos( mapPos + SidePos[dir] ) - size / 2 , size );

				unsigned mask = tile.getSideLinkMask( i ) & ~( ( BIT( i+1 ) - 1 ) );
				while( mask )
				{
					unsigned bit = FBit::Extract( mask );
					int idx = FBit::ToIndex4( bit );
					RenderUtility::setPen( g , color , COLOR_LIGHT );
					g.drawLine( calcRenderPos( mapPos + SidePos[dir] ) , calcRenderPos( mapPos + SidePos[ FDir::ToWorld( idx , rotation ) ] ) );
					mask &= ~bit;
				}
			}

			if ( tile.contentFlag & TileContent::eCloister )
			{
				RenderUtility::setBrush( g , Color::ePurple , COLOR_NORMAL );
				RenderUtility::setPen( g , Color::ePurple , COLOR_NORMAL );
				Vec2i size( 4 ,4 );
				g.drawRect( calcRenderPos( mapPos ) - size / 2 , size );
			}
		}
	}

	Vec2i LevelStage::showPlayerInfo( Graphics2D& g, Vec2i const& pos , PlayerBase* player , int offsetY )
	{
		FixString< 512 > str;
		Vec2i tempPos = pos;
		FixString< 512 > fieldStr;
		for( int i = 0 ; i < player->mFieldValues.size() ; ++i )
		{
			FixString< 32 > temp;
			fieldStr += temp.format( ( i == 0 ) ? "%d" : " %d" , player->mFieldValues[i] );
		}
		g.drawText( tempPos , str.format( "id=%d score=%d f=[ %s ]" , player->getId() , player->mScore , fieldStr.c_str() ) );
		tempPos.y += offsetY;

		return tempPos;
	}

	Vec2i LevelStage::showFeatureInfo( Graphics2D& g, Vec2i const& pos , FeatureBase* build , int offsetY )
	{
		FixString< 512 > str;
		Vec2i tempPos = pos;

		g.drawText( tempPos , str.format( "Group=%2d Type=%d TileNum=%d ActorNum=%d IsComplete=%d" ,
			build->group , build->type , build->mapTiles.size() , build->mActors.size() , build->checkComplete() ? 1 : 0 ) );
		tempPos.y += offsetY;
		switch( build->type )
		{
		case FeatureType::eCity:
			{
				CityFeature* myFeature = static_cast< CityFeature* >( build );
				g.drawText( tempPos , str.format( " City: ( %d %d , %d ) NodeNum=%d OpenCount=%d  FarmNum=%d " ,
					myFeature->nodes[0]->getMapTile()->pos.x , myFeature->nodes[0]->getMapTile()->pos.y , myFeature->nodes[0]->index ,
					myFeature->nodes.size() , myFeature->openCount , myFeature->linkFarms.size() ) ) ;
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eRoad:
			{
				RoadFeature* myFeature = static_cast< RoadFeature* >( build );
				g.drawText( tempPos , str.format( " Road: ( %d %d , %d ) NodeNum=%d OpenCount=%d" ,  
					myFeature->nodes[0]->getMapTile()->pos.x , myFeature->nodes[0]->getMapTile()->pos.y , myFeature->nodes[0]->index ,
					myFeature->nodes.size() , myFeature->openCount ) );
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eFarm:
			{
				FarmFeature* myFeature = static_cast< FarmFeature* >( build );
				g.drawText( tempPos , str.format( " Farm: ( %d %d , %d ) NodeNum=%d CityNum=%d" , 
					myFeature->nodes[0]->getMapTile()->pos.x , myFeature->nodes[0]->getMapTile()->pos.y , myFeature->nodes[0]->index ,
					myFeature->nodes.size() , myFeature->linkCities.size() ) );
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eCloister:
			{
				CloisterFeature* myFeature = static_cast< CloisterFeature* >( build );
				g.drawText( tempPos , str.format( " Cloister: LinkTile=%d" , myFeature->neighborTiles.size() ) );
				tempPos.y += offsetY;
			}
			break;
		}
		return tempPos;
	}


	bool LevelStage::onKey(unsigned key , bool isDown)
	{
		if ( !isDown )
			return false;

		float offset = 0.2f;
		switch( key )
		{
		case Keyboard::eR: getStage()->restart( false ); break;
		case Keyboard::eF: gDrawFarmLink = !gDrawFarmLink; break;
		case Keyboard::eS: gDrawSideLink = !gDrawSideLink; break;
		case Keyboard::eQ: 
			++mRotation; 
			if ( mRotation == FDir::TotalNum ) mRotation = 0; 
			break;
		case Keyboard::eE: 
			++mMoudule.mUseTileId; 
			if ( mMoudule.mUseTileId == mMoudule.mTileSetManager.getReigsterTileNum() ) 
				mMoudule.mUseTileId = 0;
			mMoudule.updatePosibleLinkPos();
			setTileObjectTexture( mTileShowObject , mMoudule.mUseTileId );
			break;
		case Keyboard::eW: 
			if ( mMoudule.mUseTileId == 0 ) 
				mMoudule.mUseTileId = mMoudule.mTileSetManager.getReigsterTileNum() - 1;
			else
				--mMoudule.mUseTileId;
			mMoudule.updatePosibleLinkPos();
			setTileObjectTexture( mTileShowObject , mMoudule.mUseTileId );
			break;
		case Keyboard::eO: 
			++mIdxShowFeature; 
			if ( mIdxShowFeature == mMoudule.mFeatureMap.size() ) 
				mIdxShowFeature = 0;
			break;
		case Keyboard::eP: 
			if ( mIdxShowFeature == 0 ) 
				mIdxShowFeature = mMoudule.mFeatureMap.size() - 1;
			else
				--mIdxShowFeature;
			break;
		case Keyboard::eUP:    setRenderOffset( mRenderOffset - Vec2f( 0,offset ) ); break;
		case Keyboard::eDOWN:  setRenderOffset( mRenderOffset + Vec2f( 0,offset ) ); break;
		case Keyboard::eLEFT:  setRenderOffset( mRenderOffset + Vec2f( offset , 0 ) ); break;
		case Keyboard::eRIGHT: setRenderOffset( mRenderOffset - Vec2f( offset , 0 ) ); break;
		}
		return false;
	}

	bool LevelStage::onMouse(MouseMsg const& msg)
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

		Level& level = mMoudule.getLevel();
		if ( msg.onMoving() )
		{
			Vec2i pos = convertToMapPos( msg.getPos() );
			if ( pos != mCurMapPos )
			{
				mCurMapPos = pos;
				switch( mCoroutine.getAction() )
				{
				case ACTION_PLACE_TILE:
					{
						if ( level.mEmptyLinkPosSet.find( pos ) != level.mEmptyLinkPosSet.end() )
						{
							int dir = 0;
							for(  ; dir < FDir::TotalNum ; ++dir )
							{
								int rotation = ( mRotation + dir ) % FDir::TotalNum;
								if ( mMoudule.getLevel().canPlaceTile( mMoudule.mUseTileId , mCurMapPos , rotation ) )
								{
									mRotation = rotation;
									break;
								}
							}
							mTileShowObject->setLocalOrientation( CFly::CF_AXIS_Z , Math::Deg2Rad( mRotation * 90 ) );
							
						}
						mTileShowObject->setLocalPosition( CFly::Vector3( pos.x , pos.y , 0 ) );
						mTileShowObject->show( level.findMapTile( pos ) == nullptr );
					}
					break;
				}
			}
		}
		else if ( msg.onLeftDown() )
		{

			switch( mCoroutine.getAction() )
			{
			case ACTION_PLACE_TILE:
				mCoroutine.resquestPlaceTile( mCurMapPos , mRotation );
				break;
			}
			//mLevel.putTile( mUseTileId , mCurMapPos , mRotation );
		}
		else if ( msg.onRightDown() )
		{
			switch( mCoroutine.getAction() )
			{
			case ACTION_PLACE_TILE:
				{
					Vec2i pos = convertToMapPos( msg.getPos() );
					if ( level.mEmptyLinkPosSet.find( pos ) != level.mEmptyLinkPosSet.end() )
					{
						for( int dir = 0 ; dir < FDir::TotalNum ; ++dir )
						{
							mRotation = ( mRotation + 1 ) % FDir::TotalNum;
							if ( mMoudule.getLevel().canPlaceTile( mMoudule.mUseTileId , mCurMapPos , mRotation ) )
								break;
						}
					}
					else
					{
						mRotation = ( mRotation + 1 ) % FDir::TotalNum;
					}
					mTileShowObject->setLocalOrientation( CFly::CF_AXIS_Z , Math::Deg2Rad( mRotation * 90 ) );
				}
				break;
			}
		}

		return true;
	}

	Vec2i LevelStage::convertToMapPos(Vec2i const& sPos)
	{
		Vec2f temp = ( Vec2f( sPos ) / RenderScale - mRenderOffset ) + Vec2f(0.5,0.5);
		return Vec2i( Math::Floor( temp.x ) , Math::Floor( temp.y ) );
	}

	bool LevelStage::onWidgetEvent(int event , int id , GWidget* ui)
	{
		switch( id )
		{
		case UI_ACTOR_POS_BUTTON:
			{
				int indexPos = ui->cast< ActorPosButton >()->indexPos;
				ActorType type = ui->cast< ActorPosButton >()->type;
				removeGamePlayUI();
				mCoroutine.resquestDeployActor( indexPos , type );
			}
			return false;
		case UI_ACTOR_BUTTON:
		case UI_MAP_TILE_BUTTON:
			{
				int index = ui->cast< SelectButton >()->index;
				removeGamePlayUI();
				mCoroutine.requestSelect( index );
			}
			return false;
		case UI_ACTION_SKIP:
			{
				removeGamePlayUI();
				mCoroutine.skipAction();
			}
			return false;
		case UI_ACTION_END_TURN:
			{
				ui->destroy();
				mCoroutine.returnGame();
			}
			return false;
		}


		return true;
	}

	void LevelStage::onGameAction(GameModule& moudule , PlayerAction action , GameActionData* data)
	{
		PlayerBase* player = moudule.getTurnPlayer();

		int offset = 30;
		switch ( action )
		{
		case ACTION_PLACE_TILE:
			{
				GamePlaceTileData* myData = static_cast< GamePlaceTileData* >( data );
				mTileShowObject->show( true );
				setTileObjectTexture( mTileShowObject , myData->id );
			}
			break;
		case ACTION_DEPLOY_ACTOR:
			{
				GameDeployActorData* myData = static_cast< GameDeployActorData* >( data );

				Vec2i size = Vec2i( 20 , 20 );
				Vec2f basePos = myData->mapTile->pos;
				int const offsetX = size.x + 5;
				int const offsetY = size.y + 5;
				std::vector< ActorPosInfo >& posList = moudule.mActorDeployPosList;
				{
					GButton* button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}
				for( int i = 0 ; i < posList.size() ; ++i )
				{
					ActorPosInfo& info = posList[i];

					Vec2f offset = getActorPosMapOffset( info.pos );
					Vec2f pos = basePos + 1.5 * offset;
					if ( info.pos.type == ActorPos::eTile )
					{
						offset = Vec2f(0,0.5);
					}
					unsigned mask = info.actorTypeMask;
					int type;
					for ( int num = 0; FBit::MaskIterator8( mask , type ) ; ++num )
					{
						ActorPosButton* button = new ActorPosButton( UI_ACTOR_POS_BUTTON ,  calcRenderPos( pos ) + RenderScale * num * offset - size / 2 , size , nullptr );
						button->indexPos = i;
						button->type     = ActorType( type );
						button->info     = &info;
						button->mapPos   = pos;
						addActionWidget( button );
					}
				}
			}
			break;
		case ACTION_SELECT_ACTOR:
			{
				GameSelectActorData* myData = static_cast< GameSelectActorData* >( data );

				if ( myData->canSkip() )
				{
					GButton* button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}
				
				for( int i = 0 ; i < myData->numSelection ; ++i )
				{
					LevelActor* actor = myData->actors[i];
					Vec2f pos = getActorPosMapOffset( actor->pos );
					if ( actor->mapTile )
						pos += actor->mapTile->pos;
					SelectButton* button = new SelectButton( UI_ACTOR_BUTTON , calcRenderPos( pos ) , Vec2i(20,20) , nullptr );
					button->index = i;
					button->actor = actor;
					button->mapPos = pos;
					addActionWidget( button );
				}
			}
			break;
		case ACTION_SELECT_ACTOR_INFO:
			{
				GameSelectActorInfoData* myData = static_cast< GameSelectActorInfoData* >( data );

				if ( myData->canSkip() )
				{
					GButton* button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}

				for( int i = 0 ; i < myData->numSelection ; ++i )
				{
					Vec2i pos = Vec2i(300,200);
					SelectButton* button = new SelectButton( UI_ACTOR_INFO_BUTTON , pos + Vec2i(30,0) , Vec2i(20,20) , nullptr );
					button->index = i;
					button->actorInfo = &myData->actorInfos[i];
					button->mapPos = pos;
					addActionWidget( button );
				}
			}
			break;
		case ACTION_SELECT_MAPTILE:
			{
				GameSelectMapTileData* myData = static_cast< GameSelectMapTileData* >( data );

				if ( myData->canSkip() )
				{
					GButton* button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}

				if ( myData->reason == SAR_WAGON_MOVE_TO_FEATURE )
				{
					for( int i = 0 ; i < myData->numSelection ; ++i )
					{
						MapTile* mapTile = myData->mapTiles[i];
						Vec2f pos = mapTile->pos;
						SelectButton* button = new SelectButton( UI_MAP_TILE_BUTTON , calcRenderPos( pos ) , Vec2i(20,20) , nullptr );
						button->index = i;
						button->mapTile = mapTile;
						button->mapPos = pos;
						addActionWidget( button );
					}
				}
				else
				{
					for( int i = 0 ; i < myData->numSelection ; ++i )
					{
						MapTile* mapTile = myData->mapTiles[i];
						Vec2f pos = mapTile->pos;
						SelectButton* button = new SelectButton( UI_MAP_TILE_BUTTON , calcRenderPos( pos ) , Vec2i(20,20) , nullptr );
						button->index = i;
						button->mapTile = mapTile;
						button->mapPos = pos;
						addActionWidget( button );
					}
				}
			}
			break;
		}
	}

	Vec2f LevelStage::getActorPosMapOffset( ActorPos const& pos )
	{
		switch( pos.type )
		{
		case ActorPos::eSideNode:
			return SidePos[ pos.meta ];
		case ActorPos::eFarmNode:
			return FarmPos[ pos.meta ];
		case ActorPos::eTileCorner:
			return SidePos[ pos.meta ] + SidePos[ ( pos.meta + 1 ) % FDir::TotalNum ];
		}
		return Vec2f(0,0);
	}

	void LevelStage::setRenderOffset(Vec2f const& a_offset)
	{
		mRenderOffset = a_offset;
		CFly::Vector3 offset;
		offset.x = -mRenderOffset.x ;
		offset.y = -mRenderOffset.y ;
		offset.z = 0;
		mCamera->setLookAt( offset + CFly::Vector3(0,0,10) , offset , CFly::Vector3(0,1,0) );
		for( int i = 0; i < mGameActionUI.size() ; ++i )
		{
			GWidget* widget = mGameActionUI[i];
			switch( widget->getID() )
			{
			case UI_ACTOR_POS_BUTTON:
				{
					ActorPosButton* button = widget->cast< ActorPosButton >();
					button->setPos( calcRenderPos( button->mapPos) );
				}
				break;
			case UI_ACTOR_INFO_BUTTON:
			case UI_ACTOR_BUTTON:
			case UI_MAP_TILE_BUTTON:
				{
					SelectButton* button = widget->cast< SelectButton >();
					button->setPos( calcRenderPos( button->mapPos) );
				}
				break;
			}
		}
	}

	void LevelStage::onPutTile(MapTile& mapTile)
	{
		mTileShowObject->show( false );

		using namespace CFly;
		Object* obj = createTileObject();
		float x = mapTile.pos.x;
		float y = mapTile.pos.y;
		obj->setLocalPosition( Vector3(x,y,0) );
		obj->setLocalOrientation( CF_AXIS_Z , Math::Deg2Rad(90*mapTile.rotation) );
		setTileObjectTexture( obj, mapTile.getId() );
		//mObject->setRenderOption( CFRO_CULL_FACE , CF_CULL_NONE );
	}

	CFly::Object* LevelStage::createTileObject()
	{
		using namespace CFly;
		Material* mat = mWorld->createMaterial( Color4f(1,1,1) );
		Object* obj = mScene->createObject();
		MeshInfo meshInfo;
		int indices[] = { 0,2,1,0,3,2 }; 

		float vtx[] = 
		{
			-0.5,-0.5,0,0,1,
			0.5,-0.5,0,1,1,
			0.5,0.5,0,1,0,
			-0.5,0.5,0,0,0
		};

		meshInfo.isIntIndexType = true;
		meshInfo.numIndices = 6;
		meshInfo.pIndex = indices;

		meshInfo.numVertices = 4;
		meshInfo.pVertex = vtx;
		meshInfo.vertexType = CFVT_XYZ | CFVF_TEX1(2);
		meshInfo.primitiveType = CFPT_TRIANGLELIST;
		obj->createMesh( mat , meshInfo );

		return obj;
	}

	void LevelStage::setTileObjectTexture(CFly::Object* obj, TileId id)
	{
		CFly::Material* mat = obj->getElement(0)->getMaterial();
		TileSet const& tileSet = mMoudule.mTileSetManager.getTileSet( id );


		char const* dir = nullptr;
		switch( tileSet.expansions )
		{
		case EXP_BASIC: dir = "Basic"; break;
		case EXP_INNS_AND_CATHEDRALS: dir = "InnCathedral"; break;
		case EXP_TRADEERS_AND_BUILDERS: dir = "TraderBuilder"; break;
		case EXP_THE_RIVER: dir = "River"; break;
		case EXP_THE_PRINCESS_AND_THE_DRAGON: dir = "PrincessDragon"; break;
		case EXP_THE_TOWER: dir = "Tower"; break;
		
		}

		FixString< 512 > texName;
		
		if ( dir )
		{
			texName.format( "Tiles/%s/Tile_%02d_00_00" , dir , tileSet.idxDefine );
		}
		else
		{
			texName = "Tiles/Tile_NoTexture";
		}

		mat->addTexture(0,0,texName);
		mat->getTextureLayer(0).setFilterMode( CFly::CF_FILTER_POINT );
		

	}

	void LevelStage::addActionWidget(GWidget* widget)
	{
		assert( widget );
		::Global::getGUI().addWidget( widget );
		mGameActionUI.push_back( widget );
	}

	Vec2f LevelStage::calcActorMapPos(ActorPos const& pos , MapTile const& mapTile)
	{
		Vec2f result = mapTile.pos;
		switch( pos.type )
		{
		case ActorPos::eFarmNode:
			{
				Vec2f offset = Vec2f(0,0);
				unsigned mask = mapTile.getFarmLinkMask( pos.meta );
				int idx;
				int count = 0;
				while( FBit::MaskIterator8(mask,idx))
				{
					offset += FarmPos[idx];
					++count;
				}
				result += offset / count;
			}
			break;
		case ActorPos::eSideNode:
			switch ( mapTile.getLinkType( pos.meta ) )
			{
			case SideType::eCity:
				{
					Vec2f offset = Vec2f(0,0);
					unsigned mask = mapTile.getSideLinkMask( pos.meta );
					int idx;
					int count = 0;
					while( FBit::MaskIterator4(mask,idx))
					{
						offset += SidePos[idx];
						++count;
					}
					result += offset / count;
				}
				break;
			case SideType::eRoad:
				result += SidePos[pos.meta] / 2;
				break;
			}
			break;
		}
		return result;
	}

	void LevelStage::setupLocalGame(LocalPlayerManager& playerManager)
	{
		for( IPlayerManager::Iterator iter = playerManager.getIterator();
			iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			switch( player->getType() )
			{
			case PT_PLAYER:
				{
					PlayerBase* carPlayer = new PlayerBase;
					carPlayer->mTeam = player->getSlot();
					mPlayerManager.addPlayer( carPlayer );
				}
				break;
			case PT_SPECTATORS:
			case PT_COMPUTER:
				break;
			}
		}
	}

	void LevelStage::setupScene(IPlayerManager& playerManager)
	{

	}

	void CGameCoroutine::waitTurnOver( GameModule& moudule , GameActionData& data )
	{
		mAction = ACTION_NONE;
		mActionData = &data;

		mOFS << '\n';
		if ( beRecoredMode && mYeild )
		{
			GButton* button = new GButton( LevelStage::UI_ACTION_END_TURN , GUISystem::calcScreenCenterPos( Vec2i(100,30) ) + Vec2i(0,200) , Vec2i(100,30) , nullptr );
			button->setTitle( "End Turn");
			::Global::getGUI().addWidget( button );
			(*mYeild )();
		}
	}

	void CGameCoroutine::waitActionImpl(GameModule& module , PlayerAction action , GameActionData& data)
	{
		mAction = action;
		mActionData = &data;


		if ( beRecoredMode )
		{
			unsigned stateBits = 0;
			if ( mIFS >> stateBits )
			{
				mActionData->resultSkipAction = ( stateBits & BIT(1) ) != 0;
			}
			else
			{
				beRecoredMode = false;
			}
		}

		if( beRecoredMode )
		{
			if ( mActionData->resultSkipAction == false && mActionData->resultExitGame == false )
			{
				switch( mAction )
				{
				case ACTION_PLACE_TILE:
					{
						GamePlaceTileData* myData = static_cast< GamePlaceTileData* >( mActionData );
						mIFS >> myData->resultPos.x >> myData->resultPos.y >> myData->resultRotation;
					}
					break;
				case ACTION_DEPLOY_ACTOR:
					{
						GameDeployActorData* myData = static_cast< GameDeployActorData* >( mActionData );
						int type;
						mIFS >> myData->resultIndex >> type;
						myData->resultType = ( ActorType )type;
					}
					break;
				case ACTION_SELECT_MAPTILE:
					{
						GameSelectMapTileData* myData = static_cast< GameSelectMapTileData* >( mActionData );
						mIFS >> myData->resultIndex ;
					}
					break;
				case ACTION_SELECT_ACTOR:
					{
						GameSelectActorData* myData = static_cast< GameSelectActorData* >( mActionData );
						mIFS >> myData->resultIndex ;
					}
					break;
				}
			}
			resquestInternal();
		}
		else
		{
			onAction( module , mAction , mActionData );
			(*mYeild )();
		}
	}

	void CGameCoroutine::resquestInternal()
	{
		unsigned stateBits = 0;
		if ( mActionData->resultSkipAction )
			stateBits |= BIT(1);
		mOFS << stateBits << ' ';
		if ( mActionData->resultSkipAction == false && mActionData->resultExitGame == false )
		{
			switch( mAction )
			{
			case ACTION_PLACE_TILE:
				{
					GamePlaceTileData* myData = static_cast< GamePlaceTileData* >( mActionData );
					mOFS << myData->resultPos.x << ' ' << myData->resultPos.y << ' ' << myData->resultRotation << ' ';
				}
				break;
			case ACTION_DEPLOY_ACTOR:
				{
					GameDeployActorData* myData = static_cast< GameDeployActorData* >( mActionData );
					mOFS << myData->resultIndex << ' ' << (int)myData->resultType << ' ';
				}
				break;
			case ACTION_SELECT_ACTOR_INFO:
			case ACTION_SELECT_ACTOR:
			case ACTION_SELECT_MAPTILE:
				{
					GameSelectActionData* myData = static_cast< GameSelectActionData* >( mActionData );
					mOFS << myData->resultIndex << ' ';
				}
				break;
			}
		}

		clearAction();
		if ( beRecoredMode == false && mYeild )
		{
			mImpl();
		}
	}

	bool CGameCoroutine::load(char const* file)
	{
		mIFS.open( file , std::ios::in | std::ios::binary );
		if ( !mIFS.is_open() )
			return false;
		//fs >> mFS.rdbuf();
		//mFS << fs.rdbuf();
		beRecoredMode = true;
		return true;
	}

	bool CGameCoroutine::save(char const* file)
	{
		std::ofstream fs( file , std::ios::binary );
		if ( !fs.is_open() )
			return false;
		std::ios::pos_type pos = mOFS.tellg();
		fs << mOFS.rdbuf();
		mOFS.seekg( pos );

		return true;
	}

	ActorPosButton::ActorPosButton(int id , Vec2i const& pos , Vec2i const& size , GWidget* parent) 
		:BaseClass( id , pos , size ,parent )
	{

	}

	void ActorPosButton::onRender()
	{
		IGraphics2D& g = Global::getIGraphics2D();

		Vec2i pos  = getWorldPos();
		Vec2i size = getSize();

		//assert( !useColorKey );

		if ( getButtonState() == BS_PRESS )
			RenderUtility::setPen( g , Color::eBlack );
		else
			RenderUtility::setPen( g , Color::eWhite );

		switch( type )
		{
		case ActorType::eMeeple:    RenderUtility::setBrush( g , Color::eYellow , COLOR_DEEP );  break;
		case ActorType::eBigMeeple: RenderUtility::setBrush( g , Color::eYellow , COLOR_LIGHT ); break;
		case ActorType::eBuilder:   RenderUtility::setBrush( g , Color::eOrange , COLOR_LIGHT ); break;
		case ActorType::ePig:       RenderUtility::setBrush( g , Color::eGreen  , COLOR_LIGHT ); break;
		default:
			RenderUtility::setBrush( g , Color::eGray );
		}
		g.drawRect( pos , size );

		g.setTextColor( 0 , 0 , 0 );
		FixString< 128 > str;
		g.drawText( pos , size , str.format( "%d %d" , (int)info->pos.type , info->pos.meta ) , true );
	}



	void SelectButton::onRender()
	{
		IGraphics2D& g = Global::getIGraphics2D();

		Vec2i pos  = getWorldPos();
		Vec2i size = getSize();

		//assert( !useColorKey );

		if ( getButtonState() == BS_PRESS )
			RenderUtility::setPen( g , Color::eBlack );
		else
			RenderUtility::setPen( g , Color::eWhite );

		RenderUtility::setBrush( g , Color::eYellow , COLOR_DEEP );
		g.drawRect( pos , size );

		//g.setTextColor( 0 , 0 , 0 );
		//FixString< 128 > str;
		//g.drawText( pos , size , str.format( "%d %d" , (int)info->pos.type , info->pos.meta ) , true );
	}

}//namespace CAR