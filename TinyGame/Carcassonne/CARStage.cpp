#include "CARStage.h"

#include "CARPlayer.h"
#include "CARExpansion.h"

#include "DrawEngine.h"
#include "RenderUtility.h"

#include "GameGUISystem.h"
#include "Math/Base.h"
#include "PropertyKey.h"

#include "GameWorker.h"
#include "NetGameStage.h"

#include "DataTransferImpl.h"


namespace CAR
{
	static Vec2f const SidePos[] = { Vec2f(0.5,0) , Vec2f(0,0.5) , Vec2f(-0.5,0) , Vec2f(0,-0.5) };
	static Vec2f const CornerPos[] = { Vec2f(0.5,0.5) , Vec2f(-0.5,0.5) , Vec2f(-0.5,-0.5) , Vec2f(0.5,-0.5) };
	float const farmOffset = 0.425;
	float const farmOffset2 = 0.25;

	static Vec2f const FarmPos[] = 
	{ 
		Vec2f( farmOffset ,-farmOffset2 ) , Vec2f( farmOffset , farmOffset2 ) , Vec2f( farmOffset2 , farmOffset ) , Vec2f( -farmOffset2 , farmOffset ) , 
		Vec2f( -farmOffset , farmOffset2 ) , Vec2f( -farmOffset , -farmOffset2 ) , Vec2f( -farmOffset2 , -farmOffset ) , Vec2f( farmOffset2 , -farmOffset ) , 
	};

	int ColorMap[] = { Color::eBlack , Color::eRed , Color::eYellow , Color::eGreen , Color::eBlue , Color::eGray };


	bool gDrawFarmLink = true;
	bool gDrawSideLink = true;
	bool gDrawFeatureInfo = true;

	char const* gFieldNames[] = 
	{
		"Meeple" ,
		"BigMeeple" ,
		"Mayor" ,
		"Wagon" ,
		"Barn"  ,
		"Shepherd",
		"Phantom" ,
		"Abbot" ,
		"Builder" ,
		"Pig" ,
		"Gain" ,
		"Wine" ,
		"Cloth" ,
		"Tower" ,
		"Abbey" ,
		"Bridge" ,
		"Castle" ,
		"AuctionedTile" ,
	};

	char const* gActorShortNames[] =
	{
		"M" , "BM" , "MY" , "WG" , "BN" , "SH" , "PH" , "AB" , "BR" , "PG" , "DG" , "FR" ,
	};

	bool LevelStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

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

			mb2DView = true;
			mRenderOffset = Vec2f(0,0);


			mViewport = mWorld->createViewport( 0 , 0 , window.getWidth() , window.getHeight() );


			D3DDevice* d3dDevice = mWorld->getD3DDevice();
			d3dDevice->CreateOffscreenPlainSurface( window.getWidth() , window.getHeight() , D3DFMT_X8R8G8B8 , D3DPOOL_SYSTEMMEM , &mSurfaceBufferTake , NULL );

			mWorld->setDir( DIR_TEXTURE , "CAR" );

			mScene = mWorld->createScene( 1 );
			mScene->setAmbientLight( Color4f(1,1,1) );

			mSceneUI = mWorld->createScene( 1 );

			{
				mCamera = mScene->createCamera();
				mCamera->setProjectType(CFPT_ORTHOGONAL);

				mCamera->setNear( -1000 );
				mCamera->setFar( 1000 );
				//mCamera->setAspect( float(  window.getWidth() ) / window.getHeight() );
				//mCamera->setNear(0.1f);
				//mCamera->setFar(1000.0f);
				setRenderScale( 60 );
				setRenderOffset( Vec2f(0,0));
			}

			{

				mTileShowObject = mScene->createObject();
				mTileIdShow = FAIL_TILE_ID;
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

		mMoudule.mDebug = ::Global::GameSetting().getIntValue( "Debug" , "CAR" , 0 ) != 0;
		mMoudule.mListener = this;
		
		mInput.onAction = std::bind( &LevelStage::onGameAction , this , std::placeholders::_1 , std::placeholders::_2 );
		mInput.onPrevAction = std::bind( &LevelStage::onGamePrevAction , this , std::placeholders::_1 , std::placeholders::_2 );

		return true;
	}

	void LevelStage::onEnd()
	{
		mInput.exitGame();
		if ( mSurfaceBufferTake )
			mSurfaceBufferTake->Release();
		CFly::cleanupSystem();
	}

	void LevelStage::onRestart( uint64 seed , bool bInit)
	{
		if ( !bInit )
		{
			mInput.exitGame();
			for( int i = 0 ; i < mRenderObjects.size() ; ++i )
			{
				mRenderObjects[i]->release();
			}
			mRenderObjects.clear();
		}

		mMoudule.restart( bInit );
		if ( ::Global::GameSetting().getIntValue( "LoadGame" , "CAR" , 1 )  )
		{
			char const* file = ::Global::GameSetting().getStringValue( "LoadGameName" , "CAR" , "car_record2" ) ;
			mInput.loadReplay(file);
		}
		
		mInput.run( mMoudule );
	}

	void LevelStage::onRender(float dFrame)
	{

		::Graphics2D& g = Global::getGraphics2D();
		if ( 1 )
		{
			mScene->render( mCamera , mViewport );
			mSceneUI->render2D( mViewport , CFly::CFRF_CLEAR_Z );

			IDirect3DSurface9* pBackBufferSurface;
			D3DDevice* d3dDevice = mWorld->getD3DDevice();
			d3dDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBufferSurface);
			HRESULT hr = d3dDevice->GetRenderTargetData( pBackBufferSurface ,mSurfaceBufferTake );
			pBackBufferSurface->Release();

			int w = ::Global::getDrawEngine()->getScreenWidth();
			int h = ::Global::getDrawEngine()->getScreenHeight();

			HDC hDC;
			mSurfaceBufferTake->GetDC(&hDC);
			::BitBlt(  g.getRenderDC() ,0 , 0, w , h , hDC , 0 , 0 , SRCCOPY );
			mSurfaceBufferTake->ReleaseDC(hDC);
		}

		WorldTileManager& level = mMoudule.getLevel();
		for ( WorldTileManager::WorldTileMap::iterator iter = level.mMap.begin() , itEnd = level.mMap.end();
			iter != itEnd ; ++iter )
		{
			RenderUtility::setBrush( g , Color::eNull );
			RenderUtility::setPen( g , Color::eGray );

			MapTile& mapData = iter->second;

			drawTileRect( g , mapData.pos );
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
			FixString< 512 > str;
			for( int i = 0 ; i < mSetting.getFieldNum() ; i += 2 )
			{
				str += gFieldNames[ mFiledTypeMap[i] ];
				str += "  ";
			}
			g.drawText( pos , str );
			pos.y += 12;

			str.clear();
			for( int i = 1 ; i < mSetting.getFieldNum() ; i += 2 )
			{
				str += gFieldNames[ mFiledTypeMap[i] ];
				str += "  ";
			}
			g.drawText( pos , str );
			pos.y += 12;
			
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
					for( MapTileSet::iterator iter = feature->mapTiles.begin() , itEnd = feature->mapTiles.end() ;
						iter != itEnd ; ++iter )
					{
						RenderUtility::setBrush( g , Color::eNull );
						RenderUtility::setPen( g , Color::eRed );
						drawTileRect( g , (*iter)->pos );
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
								g.drawRect( convertToScreenPos( Vec2f( node->getMapTile()->pos ) + SidePos[ node->index ] ) - size / 2 , size );
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
								g.drawRect( convertToScreenPos( Vec2f(  node->getMapTile()->pos ) + FarmPos[ node->index ] ) - size / 2 , size );
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
				case ActorPos::eTileCorner:
					mapPos += CornerPos[ actor->pos.meta ];
					break;
				}
			}

			RenderUtility::setBrush( g , Color::eWhite );
			RenderUtility::setPen( g , Color::eWhite );

			FixString< 128 > str;
			Vec2f screenPos = convertToScreenPos( mapPos );
			switch( actor->type )
			{
			case ActorType::eMeeple:
				g.drawCircle( screenPos , 10 );
				break;
			case ActorType::eBigMeeple:
				g.drawCircle( screenPos , 15 );
				break;

			case ActorType::eDragon:
				{
					Vec2f size( 20 , 20 );
					g.drawRect( screenPos - size / 2 ,  size );
					g.setTextColor( 255 , 0 , 0 );
					g.drawText( screenPos - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , "D" );
				}
				break;
			case ActorType::eFariy:
				{
					Vec2f size( 20 , 20 );
					g.drawRect( screenPos - size / 2 ,  size );
					g.setTextColor( 255 , 0 , 0 );
					g.drawText( screenPos - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , "F" );
				}
				break;
			case ActorType::eBuilder:
				{
					Vec2f size( 10 , 20 );
					g.drawRect( screenPos - size / 2 ,  size );
				}
				break;
			case ActorType::ePig:
				{
					Vec2f size( 20 , 20 );
					g.drawRect( screenPos - size / 2 ,  size );
				}
				break;
			case ActorType::eBarn:
				{
					g.drawCircle( screenPos , 15 );
				}
				break;
			case ActorType::eMayor:
				{
					g.drawCircle( screenPos , 15 );
				}
				break;
			case ActorType::eWagon:
				{
					g.drawCircle( screenPos , 15 );
				}
				break;
			}

			if ( actor->owner )
			{
				RenderUtility::setFontColor( g , ColorMap[actor->owner->getId()] , COLOR_DEEP );
				g.drawText( screenPos - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , gActorShortNames[ actor->type ] );
			}
		}

		if ( mInput.getReplyAction() == ACTION_PLACE_TILE )
		{
			RenderUtility::setBrush( g , Color::eNull );
			RenderUtility::setPen( g , Color::eWhite );
			drawTileRect( g , mCurMapPos );
			if ( level.findMapTile( mCurMapPos ) == nullptr )
			{
				drawTile( g , Vec2f(mCurMapPos) , level.getTile( mMoudule.mUseTileId ) , mRotation );
			}

			for( int i = 0  ; i < mMoudule.mPlaceTilePosList.size() ; ++i )
			{
				Vec2i pos = mMoudule.mPlaceTilePosList[i];
				RenderUtility::setBrush( g , Color::eNull );
				RenderUtility::setPen( g , Color::eGreen );
				drawTileRect( g , pos );
			}
		}

		char const* actionStr = "Unknown Action";
		if ( mInput.getReplyAction() != ACTION_NONE )
		{
			switch( mInput.getReplyAction() )
			{
			case ACTION_SELECT_ACTOR_INFO:
			case ACTION_SELECT_MAPTILE:
			case ACTION_SELECT_ACTOR:
				{
					GameSelectActionData* myData = static_cast< GameSelectActionData* >( mInput.getReplyData() );
#define CASE( ID , STR )\
			case ID: actionStr = STR; break;

					switch( myData->reason )
					{
					CASE(SAR_CONSTRUCT_TOWER , "Construct Tower")
					CASE(SAR_MOVE_DRAGON, "Move Dragon")
					CASE(SAR_MAGIC_PORTAL,"Magic Portal")
					CASE(SAR_WAGON_MOVE_TO_FEATURE, "Wagon Move To Feature")
					CASE(SAR_FAIRY_MOVE_NEXT_TO_FOLLOWER, "Move Fairy")
					CASE(SAR_TOWER_CAPTURE_FOLLOWER, "Tower Capture Follower")
					CASE(SAR_PRINCESS_REMOVE_KINGHT, "Princess Remove Kinght")
					CASE(SAR_EXCHANGE_PRISONERS, "Exchange Prisoners")
					}
#undef CASE
				}
				break;
			case ACTION_PLACE_TILE:
				actionStr = "Place Tile";
				break;
			case ACTION_DEPLOY_ACTOR:
				{
					actionStr = "Deploy Actor";
					GameDeployActorData* myData = static_cast< GameDeployActorData* >( mInput.getReplyData() );

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
							g.drawLine( button->getPos() + button->getSize() / 2 , convertToScreenPos( posNode ) );

						}
					}
				}
				break;
			case ACTION_AUCTION_TILE:
				actionStr = "Auction Tile";
				break;
			case ACTION_BUY_AUCTIONED_TILE:
				actionStr = "Buy Auction Tile";
				break;
			}

			g.setTextColor( 255 , 0 , 0 );
			g.drawText( 400 , 10  , actionStr );
		}
	}

	void LevelStage::drawMapData( Graphics2D& g , Vec2f const& mapPos , MapTile const& mapData)
	{

		if ( gDrawFarmLink )
		{
			unsigned maskAll = 0;
			
			for( int i = 0; i < TilePiece::NumFarm ; ++i )
			{
				int color = Color::eCyan;
				MapTile::FarmNode const& node = mapData.farmNodes[i];

				if ( node.outConnect )
				{
					RenderUtility::setPen( g , color , COLOR_DEEP );
					g.drawLine( convertToScreenPos( mapPos + FarmPos[i] + ( 0.5 - farmOffset ) * SidePos[ TilePiece::FarmSideDir( i ) ]  ) ,
						        convertToScreenPos( mapPos + FarmPos[i] )  );
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
						g.drawLine( convertToScreenPos( mapPos + FarmPos[indexPrev] ) , convertToScreenPos( mapPos + FarmPos[idx] ) );
						mask &= ~bit;
						indexPrev = idx;
					}

					RenderUtility::setPen( g , color , COLOR_DEEP );
					g.drawLine( convertToScreenPos( mapPos + FarmPos[indexPrev] ) , convertToScreenPos( mapPos + FarmPos[i] ) );
				}
			}
		}

		drawTile( g , mapPos , *mapData.mTile , mapData.rotation , &mapData );
	}

	void LevelStage::drawTile(Graphics2D& g , Vec2f const& mapPos , TilePiece const& tile , int rotation , MapTile const* mapTile )
	{

		if ( gDrawSideLink )
		{
			for( int i = 0; i < TilePiece::NumSide ; ++i )
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
				g.drawRect( convertToScreenPos( mapPos + SidePos[dir] ) - size / 2 , size );

				if ( mapTile )
				{
					unsigned mask = mapTile->getSideLinkMask( dir ) & ~( ( BIT( i+1 ) - 1 ) );
					while( mask )
					{
						unsigned bit = FBit::Extract( mask );
						int idx = FBit::ToIndex4( bit );
						RenderUtility::setPen( g , color , COLOR_LIGHT );
						g.drawLine( convertToScreenPos( mapPos + SidePos[dir] ) , convertToScreenPos( mapPos + SidePos[ idx ] ) );
						mask &= ~bit;
					}
				}
				else
				{
					unsigned mask = tile.getSideLinkMask( i ) & ~( ( BIT( i+1 ) - 1 ) );
					while( mask )
					{
						unsigned bit = FBit::Extract( mask );
						int idx = FBit::ToIndex4( bit );
						RenderUtility::setPen( g , color , COLOR_LIGHT );
						g.drawLine( convertToScreenPos( mapPos + SidePos[dir] ) , convertToScreenPos( mapPos + SidePos[ FDir::ToWorld( idx , rotation ) ] ) );
						mask &= ~bit;
					}
				}
			}

			if ( tile.contentFlag & TileContent::eCloister )
			{
				RenderUtility::setBrush( g , Color::ePurple , COLOR_NORMAL );
				RenderUtility::setPen( g , Color::ePurple , COLOR_NORMAL );
				Vec2i size( 4 ,4 );
				g.drawRect( convertToScreenPos( mapPos ) - size / 2 , size );
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
				MapTile const* mapTile = myFeature->nodes[0]->getMapTile();
				g.drawText( tempPos , str.format( " City: ( %d %d , %d ) NodeNum=%d OpenCount=%d HSCount=%d FarmNum=%d " ,
					mapTile->pos.x , mapTile->pos.y , myFeature->nodes[0]->index ,
					myFeature->nodes.size() , myFeature->openCount , myFeature->halfSepareteCount , myFeature->linkFarms.size() ) ) ;
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eRoad:
			{
				RoadFeature* myFeature = static_cast< RoadFeature* >( build );
				MapTile const* mapTile = myFeature->nodes[0]->getMapTile();
				g.drawText( tempPos , str.format( " Road: ( %d %d , %d ) NodeNum=%d OpenCount=%d" ,  
					mapTile->pos.x , mapTile->pos.y , myFeature->nodes[0]->index ,
					myFeature->nodes.size() , myFeature->openCount ) );
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eFarm:
			{
				FarmFeature* myFeature = static_cast< FarmFeature* >( build );
				MapTile const* mapTile = myFeature->nodes[0]->getMapTile();
				g.drawText( tempPos , str.format( " Farm: ( %d %d , %d ) NodeNum=%d CityNum=%d" , 
					mapTile->pos.x , mapTile->pos.y , myFeature->nodes[0]->index ,
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
		case Keyboard::eZ: mb2DView = !mb2DView; setRenderOffset( mRenderOffset ); break;
		case Keyboard::eR: getStageMode()->restart( false ); break;
		case Keyboard::eF: gDrawFarmLink = !gDrawFarmLink; break;
		case Keyboard::eS: gDrawSideLink = !gDrawSideLink; break;
		case Keyboard::eQ: 
			++mRotation; 
			if ( mRotation == FDir::TotalNum ) mRotation = 0; 
			break;
		case Keyboard::eE: 
			if ( canInput() )
			{
				TileId id = mMoudule.mUseTileId + 1;
				if ( id == mMoudule.mTileSetManager.getRegisterTileNum() ) 
					id = 0;

				mInput.changePlaceTile( id );
				updateShowTileObject( id );
			}
			break;
		case Keyboard::eW: 
			if ( canInput() )
			{
				TileId id = mMoudule.mUseTileId;
				if ( id == 0 ) 
					id = mMoudule.mTileSetManager.getRegisterTileNum() - 1;
				else
					id -= 1;

				mInput.changePlaceTile( id );
				updateShowTileObject( id );
			}
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

		WorldTileManager& level = mMoudule.getLevel();

		if ( msg.onLeftDown() )
		{
			if ( canInput() )
			{
				switch( mInput.getReplyAction() )
				{
				case ACTION_PLACE_TILE:
					mInput.replyPlaceTile( mCurMapPos , mRotation );
					break;
				}
			}
		}
		else if ( msg.onMoving() )
		{
			Vec2i pos = convertToMapTilePos( msg.getPos() );
			if ( pos != mCurMapPos )
			{
				mCurMapPos = pos;
				if ( canInput() )
				{
					switch( mInput.getReplyAction() )
					{
					case ACTION_PLACE_TILE:
						if ( level.isLinkTilePosible( pos , mMoudule.mUseTileId ) )
						{
							int dir = 0;
							for(  ; dir < FDir::TotalNum ; ++dir )
							{
								int rotation = ( mRotation + dir ) % FDir::TotalNum;
								PutTileParam param;
								param.checkRiverConnect = 1;
								param.usageBridge = 0;
								if ( mMoudule.getLevel().canPlaceTile( mMoudule.mUseTileId , mCurMapPos , rotation , param ) )
								{
									mRotation = rotation;
									break;
								}
							}
							mTileShowObject->setLocalOrientation( CFly::CF_AXIS_Z , Math::Deg2Rad( mRotation * 90 ) );
						}
						mTileShowObject->setLocalPosition( CFly::Vector3( pos.x , pos.y , 0 ) );
						mTileShowObject->show( level.findMapTile( pos ) == nullptr );
						break;
					}
				}
			}
		}
		else if ( msg.onRightDown() )
		{
			if ( canInput() )
			{
				switch( mInput.getReplyAction() )
				{
				case ACTION_PLACE_TILE:
					Vec2i pos = convertToMapTilePos( msg.getPos() );
					if ( level.isLinkTilePosible( pos , mMoudule.mUseTileId ) )
					{
						for( int dir = 0 ; dir < FDir::TotalNum ; ++dir )
						{
							mRotation = ( mRotation + 1 ) % FDir::TotalNum;
							PutTileParam param;
							param.checkRiverConnect = 1;
							param.usageBridge = 0;
							if ( mMoudule.getLevel().canPlaceTile( mMoudule.mUseTileId , mCurMapPos , mRotation , param ) )
								break;
						}
					}
					else
					{
						mRotation = ( mRotation + 1 ) % FDir::TotalNum;
					}
					mTileShowObject->setLocalOrientation( CFly::CF_AXIS_Z , Math::Deg2Rad( mRotation * 90 ) );
					break;
				}
			}
		}

		return true;
	}

	void LevelStage::setRenderScale(float scale)
	{
		mRenderScale = scale;
		mRenderTileSize = mRenderScale * Vec2f(1,1);

		int w = ::Global::getDrawEngine()->getScreenWidth();
		int h = ::Global::getDrawEngine()->getScreenHeight();
		float factor = float( w ) / h;
		float len = w / ( 2 * mRenderScale );
		mCamera->setScreenRange( -len , len , -len / factor , len / factor );
	}

	void LevelStage::setRenderOffset(Vec2f const& a_offset)
	{
		mRenderOffset = a_offset;

		Vector3 offset;
		offset.x = -mRenderOffset.x ;
		offset.y = -mRenderOffset.y ;
		offset.z = 0;

		if ( mb2DView )
			mCamera->setLookAt( offset + Vector3(0,0,10) , offset , Vector3(0,1,0) );
		else
			mCamera->setLookAt( offset + Vector3( 10 , 10 , 10 ) , offset , Vector3(0,0,1) );

		mMatVP = mCamera->getViewMatrix() * mCamera->getProjectionMartix();
		float det;
		mMatVP.inverse( mMatInvVP , det );

		for( int i = 0; i < mGameActionUI.size() ; ++i )
		{
			GWidget* widget = mGameActionUI[i];
			switch( widget->getID() )
			{
			case UI_ACTOR_POS_BUTTON:
				{
					ActorPosButton* button = widget->cast< ActorPosButton >();
					button->setPos( convertToScreenPos( button->mapPos) );
				}
				break;
			case UI_ACTOR_INFO_BUTTON:
			case UI_ACTOR_BUTTON:
			case UI_MAP_TILE_BUTTON:
				{
					SelectButton* button = widget->cast< SelectButton >();
					button->setPos( convertToScreenPos( button->mapPos) );
				}
				break;
			}
		}
	}

	Vec2f LevelStage::convertToMapPos(Vec2i const& sPos)
	{
		using namespace CFly;

		int h = ::Global::getDrawEngine()->getScreenHeight();
		int w = ::Global::getDrawEngine()->getScreenWidth();

		if ( mb2DView )
		{
			return ( Vec2f( sPos.x - w / 2 , h / 2 - sPos.y ) / mRenderScale - mRenderOffset );
		}

		float x = float( 2 * sPos.x ) / w - 1;
		float y = 1 - float( 2 * sPos.y ) / h;
		Vector3 rayStart = transform( Vector3( x , y , 0 ) , mMatInvVP );
		Vector3 rayEnd = transform( Vector3( x , y , 1 ) , mMatInvVP );
		Vector3 dir = rayEnd - rayStart;
		Vector3 temp = rayStart - ( rayStart.z / dir.z ) * ( dir );
		return Vec2f( temp.x , temp.y );
	}

	Vec2i LevelStage::convertToMapTilePos(Vec2i const& sPos)
	{
		using namespace CFly;
		Vec2f mapPos = convertToMapPos( sPos );
		return Vec2i( Math::Floor( mapPos.x + 0.5f ) , Math::Floor( mapPos.y + 0.5f ) );
	}

	Vec2f LevelStage::convertToScreenPos( Vec2f const& posMap )
	{
		using namespace CFly;

		int h = ::Global::getDrawEngine()->getScreenHeight();
		int w = ::Global::getDrawEngine()->getScreenWidth();
		if ( mb2DView )
		{
			Vec2f pos = mRenderScale * ( mRenderOffset + posMap );
			return Vec2f( w / 2 + pos.x , h / 2 - pos.y );
		}
		Vector3 pos;
		pos.x = posMap.x;
		pos.y = posMap.y;
		pos.z = 0;
		pos = transform( pos , mMatVP );

		return  0.5f * Vec2f( ( 1 + pos.x ) * w , ( 1 - pos.y ) * h );
	}

	void LevelStage::drawTileRect(Graphics2D& g , Vec2f const& mapPos)
	{
		if ( mb2DView )
		{
			g.drawRect( convertToScreenPos( mapPos ) - mRenderTileSize / 2 , mRenderTileSize );
		}
		else
		{
			Vec2i pos[4];
			for( int i = 0 ; i < 4 ; ++i )
			{
				pos[i] = convertToScreenPos( mapPos + CornerPos[i] );
			}
			g.drawPolygon( pos , 4 );
		}
	}

	bool LevelStage::onWidgetEvent(int event , int id , GWidget* ui)
	{

		switch( id )
		{
		case UI_ACTOR_POS_BUTTON:
			{
				int indexPos = ui->cast< ActorPosButton >()->indexPos;
				ActorType type = ui->cast< ActorPosButton >()->type;
				removeGameActionUI();
				mInput.replyDeployActor( indexPos , type );
			}
			return false;
		case UI_ACTOR_BUTTON:
		case UI_MAP_TILE_BUTTON:
			{
				int index = ui->cast< SelectButton >()->index;
				removeGameActionUI();
				mInput.replySelect( index );
			}
			return false;
		case UI_ACTION_SKIP:
			{
				removeGameActionUI();
				mInput.replySkip();
			}
			return false;
		case UI_ACTION_END_TURN:
			{
				ui->destroy();
				mInput.replyDoIt();
			}
			return false;
		case UI_BUY_AUCTION_TILE:
		case UI_BUILD_CASTLE:
			if ( event == EVT_BOX_YES )
			{
				mInput.replyDoIt();
			}
			else
			{
				mInput.replySkip();
			}
			return false;
		case UI_REPLAY_STOP:
			{
				ui->destroy();
				mInput.returnGame();
			}
			return false;
		case UI_AUCTION_TILE_PANEL:
			{
				ui->cast< AuctionPanel >()->fireInput( mInput );
				ui->destroy();
			}
			return false;
		}

		return true;
	}

	void LevelStage::onGamePrevAction( GameModule& moudule , CGameInput& input )
	{
		if ( input.isReplayMode() && input.getReplyAction() == ACTION_TRUN_OVER )
		{
			GButton* button = new GButton( UI_REPLAY_STOP , GUISystem::calcScreenCenterPos( Vec2i(100,30) ) + Vec2i(0,200) , Vec2i(100,30) , nullptr );
			button->setTitle( "End Turn");
			::Global::GUI().addWidget( button );
			input.waitReply();
		}
	}

	void LevelStage::onGameAction( GameModule& moudule , CGameInput& input )
	{
		PlayerAction action = input.getReplyAction();
		GameActionData* data = input.getReplyData();

		if ( !canInput() )
		{
			if ( action != ACTION_TRUN_OVER )
				input.waitReply();
			return;
		}

		PlayerBase* player = moudule.getTurnPlayer();

		int offset = 30;
		switch ( action )
		{
		case ACTION_PLACE_TILE:
			{
				GamePlaceTileData* myData = data->cast< GamePlaceTileData >();
				updateShowTileObject( myData->id );
				mTileShowObject->show( true );
			}
			break;
		case ACTION_DEPLOY_ACTOR:
			{
				GameDeployActorData* myData = data->cast< GameDeployActorData >();

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
					for ( int num = 0; FBit::MaskIterator< 32 >( mask , type ) ; ++num )
					{
						ActorPosButton* button = new ActorPosButton( UI_ACTOR_POS_BUTTON ,  convertToScreenPos( pos + offset * num  ) - size / 2 , size , nullptr );
						button->indexPos = i;
						button->type     = ActorType( type );
						button->info     = &info;
						button->mapPos   = pos + offset * num;
						addActionWidget( button );
					}
				}
			}
			break;
		case ACTION_SELECT_ACTOR:
			{
				GameSelectActorData* myData = data->cast< GameSelectActorData >();

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
					SelectButton* button = new SelectButton( UI_ACTOR_BUTTON , convertToScreenPos( pos ) , Vec2i(20,20) , nullptr );
					button->index = i;
					button->actor = actor;
					button->mapPos = pos;
					addActionWidget( button );
				}
			}
			break;
		case ACTION_SELECT_ACTOR_INFO:
			{
				GameSelectActorInfoData* myData = data->cast< GameSelectActorInfoData >();

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
				GameSelectMapTileData* myData = data->cast< GameSelectMapTileData >();

				if ( myData->canSkip() )
				{
					GButton* button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}

				if ( myData->reason == SAR_WAGON_MOVE_TO_FEATURE )
				{
					GameFeatureTileSelectData* selectData = data->cast< GameFeatureTileSelectData >();
					for( int n = 0 ; n < selectData->infos.size() ; ++n )
					{
						GameFeatureTileSelectData::Info& info = selectData->infos[n];
						for( int i = 0 ; i < info.num ; ++i )
						{
							int idx = info.index + i;
							MapTile* mapTile = selectData->mapTiles[ idx ];
							ActorPos actorPos;
							info.feature->getActorPos( *mapTile , actorPos );
							Vec2f pos = calcActorMapPos( actorPos , *mapTile );
							SelectButton* button = new SelectButton( UI_MAP_TILE_BUTTON , convertToScreenPos( pos ) , Vec2i(20,20) , nullptr );
							button->index   = idx;
							button->mapTile = mapTile;
							button->mapPos  = pos;
							addActionWidget( button );
						}
					}
				}
				else
				{
					for( int i = 0 ; i < myData->numSelection ; ++i )
					{
						MapTile* mapTile = myData->mapTiles[i];
						Vec2f pos = mapTile->pos;
						SelectButton* button = new SelectButton( UI_MAP_TILE_BUTTON , convertToScreenPos( pos ) , Vec2i(20,20) , nullptr );
						button->index = i;
						button->mapTile = mapTile;
						button->mapPos = pos;
						addActionWidget( button );
					}
				}
			}
			break;
		case ACTION_AUCTION_TILE:
			{
				AuctionPanel* panel = new AuctionPanel( UI_AUCTION_TILE_PANEL , Vec2i( 100,100) , nullptr );

				panel->mSprite = mSceneUI->createSprite();
				panel->init( *this , data->cast< GameAuctionTileData >() );
				::Global::GUI().addWidget( panel );
			}
			break;
		case ACTION_BUY_AUCTIONED_TILE:
			{
				GameAuctionTileData* myData = data->cast< GameAuctionTileData >();
				FixString< 512 > str;
				::Global::GUI().showMessageBox( UI_BUY_AUCTION_TILE , str.format( "Can You Buy Tile : Score = %d , Id = %d" , myData->maxScore , myData->pIdCallMaxScore ) );
			}
			break;
		case ACTION_BUILD_CASTLE:
			{
				::Global::GUI().showMessageBox( UI_BUILD_CASTLE , "Can You Build Castle" );
			}
			break;
		case  ACTION_TRUN_OVER:
			{
				//if ( getGameType() == GT_SINGLE_GAME )
					return;

				GButton* button = new GButton( UI_ACTION_END_TURN , GUISystem::calcScreenCenterPos( Vec2i(100,30) ) + Vec2i(0,200) , Vec2i(100,30) , nullptr );
				button->setTitle( "End Turn");
				::Global::GUI().addWidget( button );
			}
			break;
		}

		input.waitReply();
	}

	Vec2f LevelStage::calcActorMapPos(ActorPos const& pos , MapTile const& mapTile)
	{
		Vec2f result = mapTile.pos;
		switch( pos.type )
		{
		case ActorPos::eTileCorner:
			result += CornerPos[ pos.meta ];
			break;
		case ActorPos::eFarmNode:
			{
				Vec2f offset = Vec2f(0,0);
				unsigned mask = mapTile.getFarmLinkMask( pos.meta );
				int idx;
				int count = 0;
				while( FBit::MaskIterator< TilePiece::NumFarm >(mask,idx))
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
					while( FBit::MaskIterator< FDir::TotalNum >(mask,idx))
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

	Vec2f LevelStage::getActorPosMapOffset( ActorPos const& pos )
	{
		switch( pos.type )
		{
		case ActorPos::eSideNode:
			return SidePos[ pos.meta ];
		case ActorPos::eFarmNode:
			return FarmPos[ pos.meta ];
		case ActorPos::eTileCorner:
			return CornerPos[ pos.meta ];
		}
		return Vec2f(0,0);
	}

	void LevelStage::onPutTile( TileId id , MapTile* mapTiles[] , int numMapTile )
	{
		mTileShowObject->show( false );
		
		MapTile& mapTile = *mapTiles[0];
		using namespace CFly;
		Object* obj = mScene->createObject();
		float x = mapTile.pos.x;
		float y = mapTile.pos.y;
		obj->setLocalPosition( Vector3(x,y,0) );
		obj->setLocalOrientation( CF_AXIS_Z , Math::Deg2Rad(90*mapTile.rotation) );
		createTileMesh( obj , id );
		//obj->setRenderOption( CFRO_CULL_FACE , CF_CULL_NONE );
		setTileObjectTexture( obj, mapTile.getId() );
		mRenderObjects.push_back( obj );

	}



	void LevelStage::createTileMesh( CFly::Object* obj , TileId id )
	{
		using namespace CFly;

		TileSet const& tileSet = mMoudule.mTileSetManager.getTileSet( id );
		MeshInfo meshInfo;
		int indices[] = { 0,1,2,0,2,3 }; 
		float const vtxSingle[] = 
		{
			-0.5,-0.5,0,0,1,
			0.5,-0.5,0,1,1,
			0.5,0.5,0,1,0,
			-0.5,0.5,0,0,0
		};

		float const vtxDouble[] = 
		{
			-0.5,-0.5,0,0,1,
			1.5,-0.5,0,1,1,
			1.5,0.5,0,1,0,
			-0.5,0.5,0,0,0
		};

		
		meshInfo.isIntIndexType = true;
		meshInfo.numIndices = 6;
		meshInfo.pIndex = indices;

		meshInfo.numVertices = 4;
		meshInfo.pVertex = ( tileSet.type == TileType::eDouble ) ? (void*)vtxDouble : (void*)vtxSingle;
		meshInfo.vertexType = CFVT_XYZ | CFVF_TEX1(2);
		meshInfo.primitiveType = CFPT_TRIANGLELIST;

		Material* mat = mWorld->createMaterial( Color4f(1,1,1) );
		obj->createMesh( mat , meshInfo );

	}

	void LevelStage::setTileObjectTexture(CFly::Object* obj, TileId id)
	{
		CFly::Material* mat = obj->getElement(0)->getMaterial();

		FixString< 512 > texName;
		getTileTexturePath(id, texName);
		mat->addTexture(0,0,texName);
		mat->getTextureLayer(0).setFilterMode( CFly::CF_FILTER_POINT );
	}

	void LevelStage::updateTileMesh(CFly::Object* obj , TileId newId , TileId oldId )
	{
		if ( oldId != FAIL_TILE_ID )
		{
			TileSet const& tileSetOld = mMoudule.mTileSetManager.getTileSet( oldId );
			TileSet const& tileSetNew = mMoudule.mTileSetManager.getTileSet( newId );
			if ( tileSetNew.type != tileSetOld.type )
			{
				obj->removeAllElement();
				createTileMesh( obj , newId );
			}
		}
		else
		{
			createTileMesh( obj , newId );
		}
		setTileObjectTexture( obj , newId );
	}

	void LevelStage::updateShowTileObject(TileId id)
	{
		updateTileMesh( mTileShowObject , id , mTileIdShow );
		mTileIdShow = id;
	}

	void LevelStage::addActionWidget(GWidget* widget)
	{
		assert( widget );
		::Global::GUI().addWidget( widget );
		mGameActionUI.push_back( widget );
	}

	void LevelStage::setupLocalGame(LocalPlayerManager& playerManager)
	{
		int numPlayer = 3;
		mSetting.mExpansionMask = 
			//BIT( EXP_THE_RIVER ) | 
			//BIT( EXP_THE_RIVER_II ) | 
			//BIT( EXP_TRADERS_AND_BUILDERS ) | 
			//BIT( EXP_INNS_AND_CATHEDRALS ) |
			//BIT( EXP_THE_PRINCESS_AND_THE_DRAGON ) |
			//BIT( EXP_THE_TOWER ) |
			//BIT( EXP_ABBEY_AND_MAYOR ) |
			//BIT( EXP_KING_AND_ROBBER ) |
			//BIT( EXP_BRIDGES_CASTLES_AND_BAZAARS ) |
			BIT( EXP_HILLS_AND_SHEEP ) |
			BIT( EXP_CASTLES ) |
			0;

		for( int i = 0 ; i < numPlayer ; ++i )
		{
			GamePlayer* player = playerManager.createPlayer(i);
			player->getInfo().type = PT_PLAYER;
		}
		playerManager.setUserID(0);
	}

	void LevelStage::setupScene(IPlayerManager& playerManager)
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
					player->getInfo().actionPort = carPlayer->getId();
				}
				break;
			case PT_SPECTATORS:
			case PT_COMPUTER:
				break;
			}
		}

		mMoudule.mPlayerManager = &mPlayerManager;
		mMoudule.setupSetting( mSetting );

		for( int i = 0 ; i < FieldType::NUM ; ++i )
		{
			int idx = mSetting.getFieldIndex( FieldType::Enum(i) );
			if ( idx != -1 )
			{
				mFiledTypeMap[ idx ] = i;
			}
		}

		::Global::GUI().cleanupWidget();

		int userSlotId = playerManager.getUser()->getActionPort();
		if ( getModeType() == SMT_NET_GAME )
		{
			ComWorker* worker = static_cast< NetLevelStageMode* >( getStageMode() )->getWorker();
			mInput.setDataTransfer( new CWorkerDataTransfer( worker , userSlotId ) );
			if ( ::Global::GameNet().getNetWorker()->isServer() )
			{
				mServerDataTranfser = new CSVWorkerDataTransfer(::Global::GameNet().getNetWorker(), MaxPlayerNum );
				mServerDataTranfser->setRecvFun( RecvFun( this , &LevelStage::onRecvDataSV ) );
			}
		}
	}

	int LevelStage::getActionPlayerId()
	{
		if ( mInput.getReplyAction() != ACTION_NONE && mInput.getReplyData()->playerId != -1 )
			return mInput.getReplyData()->playerId;

		return mMoudule.getTurnPlayer()->getId();
	}
	void LevelStage::onRecvDataSV( int slot , int dataId , void* data , int dataSize )
	{
		if ( slot != getActionPlayerId() )
			return;
		mServerDataTranfser->sendData( SLOT_SERVER , dataId , data , dataSize );
	}

	bool LevelStage::isUserTrun()
	{
		return getStageMode()->getPlayerManager()->getUser()->getActionPort() == mMoudule.getTurnPlayer()->getId();
	}

	bool LevelStage::canInput()
	{
		if ( mMoudule.mIsStartGame == false )
			return false;

		if ( getModeType() == SMT_SINGLE_GAME )
			return true;

		if ( getModeType() == SMT_NET_GAME )
		{
			if ( getActionPlayerId() == getStageMode()->getPlayerManager()->getUser()->getActionPort() )
				return true;
		}

		return false;
	}

	void LevelStage::removeGameActionUI()
	{
		for( int i = 0 ; i < mGameActionUI.size() ; ++i )
		{
			mGameActionUI[i]->destroy();
		}
		mGameActionUI.clear();
	}

	void LevelStage::getTileTexturePath(TileId id, FixString< 512 > &texName)
	{
		TileSet const& tileSet = mMoudule.mTileSetManager.getTileSet( id );
		char const* dir = nullptr;
		switch( tileSet.expansions )
		{
		case EXP_BASIC: dir = "Basic"; break;
		case EXP_INNS_AND_CATHEDRALS: dir = "InnCathedral"; break;
		case EXP_TRADERS_AND_BUILDERS: dir = "TraderBuilder"; break;
		case EXP_THE_RIVER: dir = "River"; break;
		case EXP_THE_RIVER_II: dir = "River2" ; break;
		case EXP_THE_PRINCESS_AND_THE_DRAGON: dir = "PrincessDragon"; break;
		case EXP_THE_TOWER: dir = "Tower"; break;
		case EXP_ABBEY_AND_MAYOR: dir = "AbbiyMayor"; break;
		case EXP_BRIDGES_CASTLES_AND_BAZAARS: dir = "BridgeCastleBazaar"; break;
		case EXP_KING_AND_ROBBER: dir = "KingRobber"; break;
		case EXP_HILLS_AND_SHEEP: dir = "HillsSheep"; break;
		case EXP_CASTLES: dir = "Castle"; break;
		}

		if ( dir )
		{
			texName.format( "Tiles/%s/Tile_%02d_00_00" , dir , tileSet.idxDefine );
		}
		else
		{
			texName = "Tiles/Tile_NoTexture";
		}
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

		switch( info->pos.type )
		{
		case ActorPos::eSideNode:    RenderUtility::setBrush( g , Color::eYellow , COLOR_DEEP );  break;
		case ActorPos::eFarmNode:    RenderUtility::setBrush( g , Color::eGreen , COLOR_LIGHT ); break;
		case ActorPos::eTile:        RenderUtility::setBrush( g , Color::eOrange , COLOR_LIGHT ); break;
		case ActorPos::eTileCorner:  RenderUtility::setBrush( g , Color::eYellow  , COLOR_LIGHT ); break;
		default:
			RenderUtility::setBrush( g , Color::eGray );
		}
		g.drawRect( pos , size );

		g.setTextColor( 0 , 0 , 0 );
		FixString< 128 > str;
		g.drawText( pos , size , str.format( "%s" , gActorShortNames[ type ] ) , true );
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


	void AuctionPanel::init( LevelStage& stage , GameAuctionTileData* data)
	{
		mData = data;

		FixString< 512 > str;
		int const tileSize = 48;
		Vec2i pos = getWorldPos();
		pos.y -= ( tileSize + 2 );
		
		if ( mData->playerId == mData->pIdRound )
		{
			GChoice* choice = new GChoice( UI_TILE_ID_SELECT , Vec2i( 10 , 45 ) , Vec2i( 100 , 25 ) , this );
			for( int i = 0 ; i < mData->auctionTiles.size() ; ++i )
			{
				str.format( "%d" , i );
				choice->appendItem( str );

				stage.getTileTexturePath( mData->auctionTiles[i] , str );
				mSprite->createRectArea( pos.x + i * ( tileSize + 2 ) , pos.y , tileSize , tileSize , str );
			}
			choice->setSelection( 0 );
		}
		else
		{
			stage.getTileTexturePath( mData->tileIdRound , str );
			mSprite->createRectArea( pos.x , pos.y , tileSize , tileSize , str );
		}

		GSlider* slider = new GSlider( UI_SCORE , Vec2i( 10 , 80 ) , 100 , true , this );
		slider->showValue();
		slider->setRange( 0 , 20 );
		slider->setValue(0);


		GButton* button = new GButton( UI_OK , Vec2i( 120 , 80 ) , Vec2i( 60 , 25 ) , this );
		button->setTitle( "OK" );
	}

	void AuctionPanel::fireInput(CGameInput& input)
	{
		int idxSelect = -1;
		if ( mData->playerId == mData->pIdRound )
		{
			idxSelect = findChild( UI_TILE_ID_SELECT )->cast< GChoice >()->getSelection();
		}
		int riseScore = findChild( UI_SCORE )->cast< GSlider >()->getValue();
		input.replyAuctionTile( riseScore , idxSelect );
	}

	bool AuctionPanel::onChildEvent(int event , int id , GWidget* ui)
	{
		switch( id )
		{
		case UI_OK:
			sendEvent( EVT_BOX_OK );
			return false;
		}
		return true;
	}

	void AuctionPanel::onRender()
	{
		BaseClass::onRender();

		IGraphics2D& g = Global::getIGraphics2D();
		Vec2i pos  = getWorldPos();
		Vec2i size = getSize();

		FixString< 512 > str;
		g.setTextColor( 255 , 255 , 0 );
		RenderUtility::setFont( g , FONT_S12 );
		g.drawText( pos + Vec2i(10,10) , str.format( "Round = %d , PlayerId = %d" , mData->pIdRound , mData->playerId ) );
		if ( mData->playerId == mData->pIdRound )
		{

		}
		else
		{
			g.drawText( pos + Vec2i(10,30) , str.format( "Max Score = %d , Id = %d" , mData->maxScore , mData->pIdCallMaxScore  ) );
		}
	} 

}//namespace CAR