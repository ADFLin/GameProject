#include "CARStage.h"

#include "CARPlayer.h"
#include "CARExpansion.h"

#include "DrawEngine.h"
#include "RenderUtility.h"

#include "GameGUISystem.h"
#include "Math/Base.h"
#include "PropertySet.h"

#include "GameWorker.h"
#include "NetGameMode.h"

#include "DataTransferImpl.h"


namespace CAR
{
#if CAR_USE_INPUT_COMMAND
	class IInputCommand
	{
	public:
		virtual ~IInputCommand() {}
		virtual void execute(CGameInput&) = 0;
	};
	template < class TFunc >
	class TFuncInputCommand : public IInputCommand
	{
	public:
		TFuncInputCommand(TFunc func) :mFunc(func) {}
		void execute(CGameInput& input) override { mFunc(input); }
		TFunc mFunc;
	};
#define CAR_INPUT_COMMAND( CODE , ...)\
	{\
		auto func = [__VA_ARGS__](CGameInput& mInput) CODE; \
		addInputCommand( new TFuncInputCommand< decltype ( func ) >( func ) );\
	}
#else
#define CAR_INPUT_COMMAND( CODE , ...) CODE
#endif


	static Vector2 const SidePos[] = { Vector2(0.5,0) , Vector2(0,0.5) , Vector2(-0.5,0) , Vector2(0,-0.5) };
	static Vector2 const CornerPos[] = { Vector2(0.5,0.5) , Vector2(-0.5,0.5) , Vector2(-0.5,-0.5) , Vector2(0.5,-0.5) };
	float const farmOffset = 0.425;
	float const farmOffset2 = 0.25;

	static Vector2 const FarmPos[] = 
	{ 
		Vector2( farmOffset ,-farmOffset2 ) , Vector2( farmOffset , farmOffset2 ) , Vector2( farmOffset2 , farmOffset ) , Vector2( -farmOffset2 , farmOffset ) , 
		Vector2( -farmOffset , farmOffset2 ) , Vector2( -farmOffset , -farmOffset2 ) , Vector2( -farmOffset2 , -farmOffset ) , Vector2( farmOffset2 , -farmOffset ) , 
	};

	int ColorMap[] = { EColor::Black , EColor::Red , EColor::Yellow , EColor::Green , EColor::Blue , EColor::Gray };


	bool gDrawFarmLink = true;
	bool gDrawSideLink = true;
	bool gDrawFeatureInfo = true;

	struct FieldNameDesc
	{
		char const* name;
		char const* shortName;
	};

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
		"GermanCastleTiles" ,
		"Gold",
		"HalflingTiles" ,
		"LaPorxadaFinishScoring",
		"WomenScore",
		"RobberScorePos",
		"TowerBuilding",
		"HouseBuilding",
		"ShedBuilding",
	};

	char const* gActorShortNames[] =
	{
		"M" , "BM" , "MY" , "WG" , "BN" , "SH" , "PH" , "AB" , "BR" , "PG" , 
		"DG" , "FR" , "CO" , "MA" , "WI" , "BPP" , "TE" , "FA" ,
	};

	bool LevelStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		{
			using namespace CFly;
			if ( !CFly::initSystem() )
				return false;

			GameWindow& window = ::Global::GetDrawEngine().getWindow();

			mWorld = CFly::createWorld( window.getHWnd() , window.getWidth() , window.getHeight() , 32 , false );
			if ( mWorld == nullptr )
			{
				return false;
			}

			mb2DView = true;
			mRenderOffset = Vector2(0,0);


			mViewport = mWorld->createViewport( 0 , 0 , window.getWidth() , window.getHeight() );


			D3DDevice* d3dDevice = mWorld->getD3DDevice();
			d3dDevice->CreateOffscreenPlainSurface( window.getWidth() , window.getHeight() , D3DFMT_X8R8G8B8 , D3DPOOL_SYSTEMMEM , &mSurfaceBufferTake , nullptr );

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
				setRenderOffset( Vector2(0,0));
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

		mGameLogic.mListener = this;

		if (::Global::GameConfig().getIntValue("AutoSaveGame", "CAR", 1))
		{
			mInput.mAutoSavePath = ::Global::GameConfig().getStringValue("AutoSaveGameName", "CAR", "car_record_temp");
		}
		
		mInput.onAction = std::bind( &LevelStage::onGameAction , this , std::placeholders::_1 , std::placeholders::_2 );
		mInput.onPrevAction = std::bind( &LevelStage::onGamePrevAction , this , std::placeholders::_1 , std::placeholders::_2 );

		return true;
	}

	void LevelStage::onEnd()
	{
		cleanupGameData( true );
		if ( mSurfaceBufferTake )
			mSurfaceBufferTake->Release();
		CFly::cleanupSystem();

		BaseClass::onEnd();
	}

	void LevelStage::cleanupGameData(bool bEndStage)
{
		mInput.exitGame();
		if ( !bEndStage )
		{
			for(auto object : mRenderObjects)
			{
				object->release();
			}
			mRenderObjects.clear();
		}
#if CAR_USE_INPUT_COMMAND
		std::for_each(mInputCommands.begin(), mInputCommands.end(), [](auto com) { delete com; });
		mInputCommands.clear();
#endif
	}

	void LevelStage::onRestart(bool bInit)
	{
		if ( !bInit )
		{
			cleanupGameData(false);
		}

		mGameLogic.restart( bInit );
		if ( ::Global::GameConfig().getIntValue( "LoadGame" , "CAR" , 1 )  )
		{
			char const* file = ::Global::GameConfig().getStringValue( "LoadGameName" , "CAR" , "car_record2" ) ;
			mInput.loadReplay(file);
		}
		
		CAR_INPUT_COMMAND({ mInput.runLogic(mGameLogic); }, this);
	}

	void LevelStage::tick()
	{
#if CAR_USE_INPUT_COMMAND
		while( !mInputCommands.empty() )
		{
			IInputCommand* com = mInputCommands.front();
			com->execute(mInput);
			delete com;
			mInputCommands.erase(mInputCommands.begin());
		}
#endif
	}

	void LevelStage::onRender(float dFrame)
	{

		::Graphics2D& g = Global::GetGraphics2D();
		if ( true )
		{
			mScene->render( mCamera , mViewport );
			mSceneUI->render2D( mViewport , CFly::CFRF_CLEAR_Z );

			IDirect3DSurface9* pBackBufferSurface;
			D3DDevice* d3dDevice = mWorld->getD3DDevice();
			d3dDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBufferSurface);
			HRESULT hr = d3dDevice->GetRenderTargetData( pBackBufferSurface ,mSurfaceBufferTake );
			pBackBufferSurface->Release();

			int w = ::Global::GetScreenSize().x;
			int h = ::Global::GetScreenSize().y;

			HDC hDC;
			mSurfaceBufferTake->GetDC(&hDC);
			::BitBlt(  g.getRenderDC() ,0 , 0, w , h , hDC , 0 , 0 , SRCCOPY );
			mSurfaceBufferTake->ReleaseDC(hDC);
		}

		WorldTileManager& world = mGameLogic.getWorld();
		for ( auto iter = world.createWorldTileIterator() ; iter ; ++iter )
		{
			RenderUtility::SetBrush( g , EColor::Null );
			RenderUtility::SetPen( g , EColor::Gray );

			MapTile const& mapData = iter->second;

			drawTileRect( g , mapData.pos );
			drawMapData( g , mapData.pos , mapData );
		}

		if ( mGameLogic.mIsStartGame )
		{
			FixString< 512 > str;
			str.format("pID = %d tileId = %d , rotation = %d , pos = %d , %d ", mGameLogic.getTurnPlayer()->getId(),
				mGameLogic.mUseTileId, mRotation, mCurMapPos.x, mCurMapPos.y);
			g.drawText( 10 , 200 , str );
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
			for(int i = 0 ; i < mGameLogic.mFeatureMap.size() ; ++i )
			{
				FeatureBase* build = mGameLogic.mFeatureMap[i];
				if ( build->group == INDEX_NONE )
					continue;
				pos = showBuildInfo( g , pos , build , 12 );
				pos.y += 2;
			}
#endif

			if ( !mGameLogic.mFeatureMap.empty() )
			{
				FeatureBase* feature = mGameLogic.mFeatureMap[ mIdxShowFeature ];

				Vec2i pos = Vec2i( 300 , 20 );
				showFeatureInfo( g , pos , feature , 12 );

				if ( feature->group != INDEX_NONE )
				{
					for(auto mapTile : feature->mapTiles)
					{
						RenderUtility::SetBrush( g , EColor::Null );
						RenderUtility::SetPen( g , EColor::Red );
						drawTileRect( g , mapTile->pos );
					}

					switch( feature->type )
					{
					case FeatureType::eCity:
					case FeatureType::eRoad:
						{
							RenderUtility::SetBrush( g , EColor::Red );
							RenderUtility::SetPen( g , EColor::Red );
							auto sideFeature = static_cast< SideFeature* >( feature );
							for(auto node : sideFeature->nodes)
							{
								Vec2i size( 4 , 4 );
								g.drawRect( convertToScreenPos( Vector2( node->getMapTile()->pos ) + SidePos[ node->index ] ) - size / 2 , size );
							}
						}
						break;
					case FeatureType::eFarm:
						{
							RenderUtility::SetBrush( g , EColor::Red );
							RenderUtility::SetPen( g , EColor::Red );
							auto* farm = static_cast< FarmFeature* >( feature );
							for(auto node : farm->nodes)
							{
								Vec2i size( 4 ,4 );
								g.drawRect( convertToScreenPos( Vector2(  node->getMapTile()->pos ) + FarmPos[ node->index ] ) - size / 2 , size );
							}
						}
						break;
					}
				}
			}

		}

		for(LevelActor* actor : mGameLogic.mActors)
		{
			if ( actor->mapTile == nullptr )
				continue;

			Vector2 mapPos = Vector2( actor->mapTile->pos );
			if ( actor->feature )
			{
				switch( actor->pos.type )
				{
				case ActorPos::eSideNode:
					mapPos += 0.7f * SidePos[ actor->pos.meta ];
					break;
				case ActorPos::eFarmNode:
					mapPos += FarmPos[ actor->pos.meta ];
					break;
				case ActorPos::eTileCorner:
					mapPos += CornerPos[ actor->pos.meta ];
					break;
				}
			}

			RenderUtility::SetBrush( g , EColor::White );
			RenderUtility::SetPen( g , EColor::White );

			FixString< 128 > str;
			Vector2 screenPos = convertToScreenPos( mapPos );
			switch( actor->type )
			{
			case EActor::Meeple:
				g.drawCircle( screenPos , 10 );
				break;
			case EActor::BigMeeple:
				g.drawCircle( screenPos , 15 );
				break;

			case EActor::Dragon:
				{
					Vector2 size( 20 , 20 );
					g.drawRect( screenPos - size / 2 ,  size );
					g.setTextColor(Color3ub(255 , 0 , 0) );
					g.drawText( screenPos - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , "D" );
				}
				break;
			case EActor::Fariy:
				{
					Vector2 size( 20 , 20 );
					g.drawRect( screenPos - size / 2 ,  size );
					g.setTextColor(Color3ub(255 , 0 , 0) );
					g.drawText( screenPos - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , "F" );
				}
				break;
			case EActor::Builder:
				{
					Vector2 size( 10 , 20 );
					g.drawRect( screenPos - size / 2 ,  size );
				}
				break;
			case EActor::Pig:
				{
					Vector2 size( 20 , 20 );
					g.drawRect( screenPos - size / 2 ,  size );
				}
				break;
			case EActor::Barn:
				{
					g.drawCircle( screenPos , 15 );
				}
				break;
			case EActor::Mayor:
				{
					g.drawCircle( screenPos , 15 );
				}
				break;
			case EActor::Wagon:
				{
					g.drawCircle( screenPos , 15 );
				}
				break;
			}

			if ( actor->ownerId != CAR_ERROR_PLAYER_ID  )
			{
				RenderUtility::SetFontColor( g , ColorMap[actor->ownerId] , COLOR_DEEP );
				g.drawText( screenPos - Vec2i( 20 , 20 ) / 2 , Vec2i( 20 , 20 ) , gActorShortNames[ actor->type ] );
			}
		}

		if ( mInput.getReplyAction() == ACTION_PLACE_TILE )
		{
			RenderUtility::SetBrush( g , EColor::Null );
			RenderUtility::SetPen( g , EColor::White );
			drawTileRect( g , mCurMapPos );
			if ( world.findMapTile( mCurMapPos ) == nullptr )
			{
				drawTile( g , Vector2(mCurMapPos) , world.getTile( mGameLogic.mUseTileId ) , mRotation );
			}

			for(Vec2i const& pos : mGameLogic.mPlaceTilePosList)
			{
				RenderUtility::SetBrush( g , EColor::Null );
				RenderUtility::SetPen( g , EColor::Green );
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
					CASE(SAR_PLACE_ABBEY_TILE , "Place Abbey Tile")
					CASE(SAR_LA_PORXADA_SELF_FOLLOWER, "Select Self Follower")
					CASE(SAR_LA_PORXADA_OTHER_PLAYER_FOLLOWER, "Select Self Follower")
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

					RenderUtility::SetBrush( g , EColor::Null );
					RenderUtility::SetPen( g , EColor::White );
					for(GWidget* widget : mGameActionUI)
					{
						if ( widget->getID() == UI_ACTOR_POS_BUTTON )
						{
							auto button = widget->cast< ActorPosButton >();
							ActorPosInfo& info = mGameLogic.mActorDeployPosList[ button->indexPos ];
							Vector2 posNode = calcActorMapPos( info.pos , *info.mapTile );
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

			g.setTextColor(Color3ub(255 , 0 , 0) );
			g.drawText( 400 , 10  , actionStr );
		}
	}

	void LevelStage::drawMapData( Graphics2D& g , Vector2 const& mapPos , MapTile const& mapData)
	{

		if ( gDrawFarmLink )
		{
			unsigned maskAll = 0;
			
			for( int i = 0; i < TilePiece::NumFarm ; ++i )
			{
				int color = EColor::Cyan;
				MapTile::FarmNode const& node = mapData.farmNodes[i];

				if ( node.outConnect )
				{
					RenderUtility::SetPen( g , color , COLOR_DEEP );
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
						unsigned bit = FBitUtility::ExtractTrailingBit( mask );
						int idx = FBitUtility::ToIndex8( bit );
						RenderUtility::SetPen( g , color , COLOR_DEEP );
						g.drawLine( convertToScreenPos( mapPos + FarmPos[indexPrev] ) , convertToScreenPos( mapPos + FarmPos[idx] ) );
						mask &= ~bit;
						indexPrev = idx;
					}

					RenderUtility::SetPen( g , color , COLOR_DEEP );
					g.drawLine( convertToScreenPos( mapPos + FarmPos[indexPrev] ) , convertToScreenPos( mapPos + FarmPos[i] ) );
				}
			}
		}

		drawTile( g , mapPos , *mapData.mTile , mapData.rotation , &mapData );
	}

	void LevelStage::drawTile(Graphics2D& g , Vector2 const& mapPos , TilePiece const& tile , int rotation , MapTile const* mapTile )
	{

		if ( gDrawSideLink )
		{
			if( tile.isHalflingType() )
			{
				//#TODO:Halfling
			}
			else
			{
				for( int i = 0; i < TilePiece::NumSide; ++i )
				{
					int dir = FDir::ToWorld(i, rotation);

					int color = EColor::White;
					switch( tile.getLinkType(i) )
					{
					case ESide::Type::City:   color = EColor::Orange; break;
					case ESide::Type::Field:  color = EColor::Green; break;
					case ESide::Type::River:  color = EColor::Blue; break;
					case ESide::Type::Road:   color = EColor::Yellow; break;
					}

					RenderUtility::SetBrush(g, color, COLOR_NORMAL);
					RenderUtility::SetPen(g, color, COLOR_NORMAL);
					Vec2i size(6, 6);
					g.drawRect(convertToScreenPos(mapPos + SidePos[dir]) - size / 2, size);

					if( mapTile )
					{
						unsigned mask = mapTile->getSideLinkMask(dir) & ~((BIT(i + 1) - 1));
						while( mask )
						{
							unsigned bit = FBitUtility::ExtractTrailingBit(mask);
							int idx = FBitUtility::ToIndex4(bit);
							RenderUtility::SetPen(g, color, COLOR_LIGHT);
							g.drawLine(convertToScreenPos(mapPos + SidePos[dir]),
									   convertToScreenPos(mapPos + SidePos[idx]));
							mask &= ~bit;
						}
					}
					else
					{
						unsigned mask = tile.getSideLinkMask(i) & ~((BIT(i + 1) - 1));
						while( mask )
						{
							unsigned bit = FBitUtility::ExtractTrailingBit(mask);
							int idx = FBitUtility::ToIndex4(bit);
							RenderUtility::SetPen(g, color, COLOR_LIGHT);
							g.drawLine(convertToScreenPos(mapPos + SidePos[dir]),
									   convertToScreenPos(mapPos + SidePos[FDir::ToWorld(idx, rotation)]));
							mask &= ~bit;
						}
					}
				}
			}

			if ( tile.contentFlag & TileContent::eCloister )
			{
				RenderUtility::SetBrush( g , EColor::Purple , COLOR_NORMAL );
				RenderUtility::SetPen( g , EColor::Purple , COLOR_NORMAL );
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
			temp.format((i == 0) ? "%d" : " %d", player->mFieldValues[i]);
			fieldStr += temp;
		}
		str.format("id=%d score=%d f=[ %s ]", player->getId(), player->mScore, fieldStr.c_str());
		g.drawText( tempPos , str);
		tempPos.y += offsetY;

		return tempPos;
	}

	Vec2i LevelStage::showFeatureInfo( Graphics2D& g, Vec2i const& pos , FeatureBase* build , int offsetY )
	{
		FixString< 512 > str;
		Vec2i tempPos = pos;
		str.format("Group=%2d Type=%d TileNum=%d ActorNum=%d IsComplete=%d",
			build->group, build->type, build->mapTiles.size(), build->mActors.size(), build->checkComplete() ? 1 : 0);
		g.drawText( tempPos , str);
		tempPos.y += offsetY;
		switch( build->type )
		{
		case FeatureType::eCity:
			{
				CityFeature* myFeature = static_cast< CityFeature* >( build );
				MapTile const* mapTile = myFeature->nodes[0]->getMapTile();
				str.format(" City: ( %d %d , %d ) NodeNum=%d OpenCount=%d HSCount=%d FarmNum=%d ",
					mapTile->pos.x, mapTile->pos.y, myFeature->nodes[0]->index,
					myFeature->nodes.size(), myFeature->openCount, myFeature->halfSepareteCount, myFeature->linkedFarms.size());
				g.drawText( tempPos , str ) ;
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eRoad:
			{
				RoadFeature* myFeature = static_cast< RoadFeature* >( build );
				MapTile const* mapTile = myFeature->nodes[0]->getMapTile();
				str.format(" Road: ( %d %d , %d ) NodeNum=%d OpenCount=%d",
					mapTile->pos.x, mapTile->pos.y, myFeature->nodes[0]->index,
					myFeature->nodes.size(), myFeature->openCount);
				g.drawText( tempPos , str );
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eFarm:
			{
				FarmFeature* myFeature = static_cast< FarmFeature* >( build );
				MapTile const* mapTile = myFeature->nodes[0]->getMapTile();
				str.format(" Farm: ( %d %d , %d ) NodeNum=%d CityNum=%d",
					mapTile->pos.x, mapTile->pos.y, myFeature->nodes[0]->index,
					myFeature->nodes.size(), myFeature->linkedCities.size());
				g.drawText( tempPos , str );
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eCloister:
			{
				auto myFeature = static_cast< CloisterFeature* >( build );
				str.format(" Cloister: LinkTile=%d", myFeature->neighborTiles.size());
				g.drawText( tempPos , str );
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eGarden:
			{
				auto myFeature = static_cast<GardenFeature*>(build);
				str.format(" Garden: LinkTile=%d", myFeature->neighborTiles.size());
				g.drawText(tempPos, str);
				tempPos.y += offsetY;
			}
			break;
		case FeatureType::eChrine:
		case FeatureType::eGermanCastle:
		default:
			break;
		}
		return tempPos;
	}


	bool LevelStage::onKey(KeyMsg const& msg)
	{
		if ( msg.isDown() )
		{
			float offset = 0.2f;
			switch (msg.getCode())
			{
			case EKeyCode::Z: mb2DView = !mb2DView; setRenderOffset(mRenderOffset); break;
			case EKeyCode::R: getStageMode()->restart(false); break;
			case EKeyCode::F: gDrawFarmLink = !gDrawFarmLink; break;
			case EKeyCode::S: gDrawSideLink = !gDrawSideLink; break;
			case EKeyCode::Q:
				++mRotation;
				if (mRotation == FDir::TotalNum) mRotation = 0;
				break;
			case EKeyCode::E:
				if (canInput())
				{
					TileId id = mGameLogic.mUseTileId + 1;
					if (id == mGameLogic.mTileSetManager.getRegisterTileNum())
						id = 0;

					mInput.changePlaceTile(id);
					updateShowTileObject(id);
				}
				break;
			case EKeyCode::W:
				if (canInput())
				{
					TileId id = mGameLogic.mUseTileId;
					if (id == 0)
						id = mGameLogic.mTileSetManager.getRegisterTileNum() - 1;
					else
						id -= 1;

					mInput.changePlaceTile(id);
					updateShowTileObject(id);
				}
				break;
			case EKeyCode::O:
				++mIdxShowFeature;
				if (mIdxShowFeature == mGameLogic.mFeatureMap.size())
					mIdxShowFeature = 0;
				break;
			case EKeyCode::P:
				if (mIdxShowFeature == 0)
					mIdxShowFeature = mGameLogic.mFeatureMap.size() - 1;
				else
					--mIdxShowFeature;
				break;
			case EKeyCode::Up:    setRenderOffset(mRenderOffset - Vector2(0, offset)); break;
			case EKeyCode::Down:  setRenderOffset(mRenderOffset + Vector2(0, offset)); break;
			case EKeyCode::Left:  setRenderOffset(mRenderOffset + Vector2(offset, 0)); break;
			case EKeyCode::Right: setRenderOffset(mRenderOffset - Vector2(offset, 0)); break;
			}
		}
		return false;
	}

	bool LevelStage::onMouse(MouseMsg const& msg)
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;

		WorldTileManager& level = mGameLogic.getWorld();

		if ( msg.onLeftDown() )
		{
			if ( canInput() )
			{
				switch( mInput.getReplyAction() )
				{
				case ACTION_PLACE_TILE:
					CAR_INPUT_COMMAND( { mInput.replyPlaceTile(mCurMapPos , mRotation); } , this );
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
						if ( level.isLinkTilePosible( pos , mGameLogic.mUseTileId ) )
						{
							int dir = 0;
							for(  ; dir < FDir::TotalNum ; ++dir )
							{
								int rotation = ( mRotation + dir ) % FDir::TotalNum;
								PlaceTileParam param;
								param.checkRiverConnect = 1;
								param.canUseBridge = 0;
								if ( mGameLogic.getWorld().canPlaceTile( mGameLogic.mUseTileId , mCurMapPos , rotation , param ) )
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
					if ( level.isLinkTilePosible( pos , mGameLogic.mUseTileId ) )
					{
						for( int dir = 0 ; dir < FDir::TotalNum ; ++dir )
						{
							mRotation = ( mRotation + 1 ) % FDir::TotalNum;
							PlaceTileParam param;
							param.checkRiverConnect = 1;
							param.canUseBridge = 0;
							if ( mGameLogic.getWorld().canPlaceTile( mGameLogic.mUseTileId , mCurMapPos , mRotation , param ) )
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
		mRenderTileSize = mRenderScale * Vector2(1,1);

		int w = ::Global::GetScreenSize().x;
		int h = ::Global::GetScreenSize().y;
		float factor = float( w ) / h;
		float len = w / ( 2 * mRenderScale );
		mCamera->setScreenRange( -len , len , -len / factor , len / factor );
	}

	void LevelStage::setRenderOffset(Vector2 const& a_offset)
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

		mWorldToClip = mCamera->getViewMatrix() * mCamera->getProjectionMartix();
		float det;
		mWorldToClip.inverse( mClipToWorld , det );

		for(GWidget* widget : mGameActionUI)
		{
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

	Vector2 LevelStage::convertToMapPos(Vec2i const& sPos)
	{
		using namespace CFly;

		int h = ::Global::GetScreenSize().y;
		int w = ::Global::GetScreenSize().x;

		if ( mb2DView )
		{
			return ( Vector2( sPos.x - w / 2 , h / 2 - sPos.y ) / mRenderScale - mRenderOffset );
		}

		float x = float( 2 * sPos.x ) / w - 1;
		float y = 1 - float( 2 * sPos.y ) / h;
		Vector3 rayStart = TransformPosition( Vector3( x , y , 0 ) , mClipToWorld );
		Vector3 rayEnd = TransformPosition( Vector3( x , y , 1 ) , mClipToWorld );
		Vector3 dir = rayEnd - rayStart;
		Vector3 temp = rayStart - ( rayStart.z / dir.z ) * ( dir );
		return Vector2( temp.x , temp.y );
	}

	Vec2i LevelStage::convertToMapTilePos(Vec2i const& sPos)
	{
		using namespace CFly;
		Vector2 mapPos = convertToMapPos( sPos );
		return Vec2i( Math::Floor( mapPos.x + 0.5f ) , Math::Floor( mapPos.y + 0.5f ) );
	}

	Vector2 LevelStage::convertToScreenPos( Vector2 const& posMap )
	{

		return (mb2DView) ? convertToScreenPos_2D(posMap) : convertToScreenPos_3D(posMap);
	}

	Vector2 LevelStage::convertToScreenPos_2D(Vector2 const& posMap)
	{
		using namespace CFly;
		assert(mb2DView);
		int h = ::Global::GetScreenSize().y;
		int w = ::Global::GetScreenSize().x;
		Vector2 pos = mRenderScale * (mRenderOffset + posMap);
		return Vector2(w / 2 + pos.x, h / 2 - pos.y);
	}

	Vector2 LevelStage::convertToScreenPos_3D(Vector2 const& posMap)
	{
		using namespace CFly;
		assert(!mb2DView);
		int h = ::Global::GetScreenSize().y;
		int w = ::Global::GetScreenSize().x;
		Vector3 pos;
		pos.x = posMap.x;
		pos.y = posMap.y;
		pos.z = 0;
		pos = TransformPosition(pos, mWorldToClip);
		return  0.5f * Vector2((1 + pos.x) * w, (1 - pos.y) * h);
	}

	void LevelStage::drawTileRect(Graphics2D& g, Vector2 const& mapPos)
	{
		if ( mb2DView )
		{
			g.drawRect( convertToScreenPos_2D( mapPos ) - mRenderTileSize / 2 , mRenderTileSize );
		}
		else
		{
			Vec2i pos[4];
			for( int i = 0 ; i < 4 ; ++i )
			{
				pos[i] = convertToScreenPos_3D( mapPos + CornerPos[i] );
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
				EActor::Type type = ui->cast< ActorPosButton >()->type;
				removeGameActionUI();
				CAR_INPUT_COMMAND( { mInput.replyDeployActor(indexPos, type); } , indexPos , type );
			}
			return false;
		case UI_ACTOR_BUTTON:
		case UI_MAP_TILE_BUTTON:
			{
				int index = ui->cast< SelectButton >()->index;
				removeGameActionUI();
				CAR_INPUT_COMMAND({ mInput.replySelection(index); } , index );
			}
			return false;
		case UI_ACTION_SKIP:
			{
				removeGameActionUI();
				CAR_INPUT_COMMAND({ mInput.replySkip(); });
			}
			return false;
		case UI_ACTION_END_TURN:
			{
				ui->destroy();
				CAR_INPUT_COMMAND({ mInput.replyDoIt(); });
			}
			return false;
		case UI_BUY_AUCTION_TILE:
		case UI_BUILD_CASTLE:
			if ( event == EVT_BOX_YES )
			{
				CAR_INPUT_COMMAND({ mInput.replyDoIt(); });
			}
			else
			{
				CAR_INPUT_COMMAND({ mInput.replySkip(); });
			}
			return false;
		case UI_REPLAY_STOP:
			{
				ui->destroy();
				CAR_INPUT_COMMAND({ mInput.executeGameLogic(); });
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

	void LevelStage::onGamePrevAction( GameLogic& gameLogic , CGameInput& input )
	{
		if ( input.isReplayMode() && input.getReplyAction() == ACTION_TRUN_OVER )
		{
			GButton* button = new GButton( UI_REPLAY_STOP , GUISystem::calcScreenCenterPos( Vec2i(100,30) ) + Vec2i(0,200) , Vec2i(100,30) , nullptr );
			button->setTitle( "End Turn");
			::Global::GUI().addWidget( button );
			input.waitReply();
		}
	}

	void LevelStage::onGameAction( GameLogic& gameLogic, CGameInput& input )
	{
		PlayerAction action = input.getReplyAction();
		GameActionData* data = input.getReplyData();

		if ( !canInput() )
		{
			if ( action != ACTION_TRUN_OVER )
				input.waitReply();
			return;
		}

		PlayerBase* player = gameLogic.getTurnPlayer();

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
				
				int const offsetX = size.x + 5;
				int const offsetY = size.y + 5;
				std::vector< ActorPosInfo >& posList = gameLogic.mActorDeployPosList;
				{
					GButton* button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}
				for( int i = 0 ; i < posList.size() ; ++i )
				{
					ActorPosInfo& info = posList[i];

					Vector2 offset = getActorPosMapOffset( info.pos );
					Vector2 basePos = info.mapTile->pos;
					Vector2 pos = basePos + 1.5 * offset;
					if ( info.pos.type == ActorPos::eTile )
					{
						offset = Vector2(0,0.5);
					}

					int num = 0;
					for( auto iter = TBitMaskIterator< 32 >(info.actorTypeMask); iter; ++iter )
					{
						auto* button = new ActorPosButton( UI_ACTOR_POS_BUTTON ,  convertToScreenPos( pos + offset * num  ) - size / 2 , size , nullptr );
						button->indexPos = i;
						button->type     = EActor::Type( iter.index );
						button->info     = &info;
						button->mapPos   = pos + offset * num;
						addActionWidget( button );
						++num;
					}
				}
			}
			break;
		case ACTION_SELECT_ACTOR:
			{
				auto* myData = data->cast< GameSelectActorData >();

				if ( myData->bCanSkip )
				{
					GButton* button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}
				
				for( int i = 0 ; i < myData->numSelection ; ++i )
				{
					LevelActor* actor = myData->actors[i];
					Vector2 pos = getActorPosMapOffset( actor->pos );
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
				auto myData = data->cast< GameSelectActorInfoData >();

				if ( myData->bCanSkip )
				{
					auto button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}

				for( int i = 0 ; i < myData->numSelection ; ++i )
				{
					Vec2i pos = Vec2i(300,200);
					auto button = new SelectButton( UI_ACTOR_INFO_BUTTON , pos + Vec2i(30,0) , Vec2i(20,20) , nullptr );
					button->index = i;
					button->actorInfo = &myData->actorInfos[i];
					button->mapPos = pos;
					addActionWidget( button );
				}
			}
			break;
		case ACTION_SELECT_MAPTILE:
			{
				auto myData = data->cast< GameSelectMapTileData >();

				if ( myData->bCanSkip )
				{
					auto button = new GButton( UI_ACTION_SKIP , Vec2i( 20 , 270 ) , Vec2i( 50 , 20 ) , nullptr );
					button->setTitle( "Skip" );
					addActionWidget( button );
				}

				if ( myData->reason == SAR_WAGON_MOVE_TO_FEATURE )
				{
					auto selectData = data->cast< GameFeatureTileSelectData >();
					for( int n = 0 ; n < selectData->infos.size() ; ++n )
					{
						GameFeatureTileSelectData::Info& info = selectData->infos[n];
						for( int i = 0 ; i < info.num ; ++i )
						{
							int idx = info.index + i;
							MapTile* mapTile = selectData->mapTiles[ idx ];
							ActorPos actorPos;
							info.feature->getActorPos( *mapTile , actorPos );
							Vector2 pos = calcActorMapPos( actorPos , *mapTile );
							auto button = new SelectButton( UI_MAP_TILE_BUTTON , convertToScreenPos( pos ) , Vec2i(20,20) , nullptr );
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
						Vector2 pos = mapTile->pos;
						auto button = new SelectButton( UI_MAP_TILE_BUTTON , convertToScreenPos( pos ) , Vec2i(20,20) , nullptr );
						button->index = i;
						button->mapTile = mapTile;
						button->mapPos = pos;
						addActionWidget( button );
					}
				}
			}
			break;
		case ACTION_SELECT_ACTION_OPTION:
			{
				auto* myData = data->cast< GameSelectActionOptionData >();


			}
			break;
		case ACTION_AUCTION_TILE:
			{
				auto panel = new AuctionPanel( UI_AUCTION_TILE_PANEL , Vec2i( 100,100) , nullptr );

				panel->mSprite = mSceneUI->createSprite();
				panel->init( *this , data->cast< GameAuctionTileData >() );
				::Global::GUI().addWidget( panel );
			}
			break;
		case ACTION_BUY_AUCTIONED_TILE:
			{
				auto* myData = data->cast< GameAuctionTileData >();
				FixString< 512 > str;
				str.format("Can You Buy Tile : Score = %d , Id = %d", myData->maxScore, myData->pIdCallMaxScore);
				::Global::GUI().showMessageBox( UI_BUY_AUCTION_TILE , str );
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

	Vector2 LevelStage::calcActorMapPos(ActorPos const& pos , MapTile const& mapTile)
	{
		Vector2 result = mapTile.pos;
		switch( pos.type )
		{
		case ActorPos::eTileCorner:
			result += CornerPos[ pos.meta ];
			break;
		case ActorPos::eFarmNode:
			{
				Vector2 offset = Vector2(0,0);
				unsigned mask = mapTile.getFarmLinkMask( pos.meta );
				int count = 0;
				for( auto iter = TBitMaskIterator< TilePiece::NumFarm >(mask); iter; ++iter )
				{
					offset += FarmPos[iter.index];
					++count;
				}
				result += offset / count;
			}
			break;
		case ActorPos::eSideNode:
			switch ( mapTile.getLinkType( pos.meta ) )
			{
			case ESide::Type::City:
				{
					Vector2 offset = Vector2(0,0);
					unsigned mask = mapTile.getSideLinkMask( pos.meta );
					int count = 0;
					for( auto iter = TBitMaskIterator< FDir::TotalNum >(mask); iter; ++iter )
					{
						offset += SidePos[iter.index];
						++count;
					}
					result += offset / count;
				}
				break;
			case ESide::Type::Road:
				result += SidePos[pos.meta] / 2;
				break;
			}
			break;
		}
		return result;
	}

	Vector2 LevelStage::getActorPosMapOffset( ActorPos const& pos )
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
		return Vector2(0,0);
	}

	void LevelStage::notifyPlaceTiles( TileId id , MapTile* mapTiles[] , int numMapTile )
	{
		mTileShowObject->show( false );
		
		for( int i = 0 ; i < 1 ; ++i )
		{
			MapTile& mapTile = *mapTiles[i];

			using namespace CFly;
			Object* obj = mScene->createObject();
			float x = mapTile.pos.x;
			float y = mapTile.pos.y;
			obj->setLocalPosition(Vector3(x, y, 0));
			obj->setLocalOrientation(CF_AXIS_Z, Math::Deg2Rad(90 * mapTile.rotation));
			createTileMesh(obj, id);
			//obj->setRenderOption( CFRO_CULL_FACE , CF_CULL_NONE );

			if( mapTile.getId() == TEMP_TILE_ID )
			{
				setTileObjectTexture(obj, mapTile.mergedTileId[0], 0);
				setTileObjectTexture(obj, mapTile.mergedTileId[1], 1);
			}
			else
			{
				setTileObjectTexture(obj, mapTile.getId());
			}

			mRenderObjects.push_back(obj);
		}

	}

	void LevelStage::createTileMesh( CFly::Object* obj , TileId id )
	{
		using namespace CFly;

		MeshInfo meshInfo;
		int indicesQuad[] = { 0,1,2,0,2,3 }; 
		
		float const vtxSingle[] = 
		{
			-0.5,-0.5,0, 0,1,
			0.5,-0.5,0,  1,1,
			0.5,0.5,0,   1,0,
			-0.5,0.5,0,  0,0
		};

		float const vtxDouble[] = 
		{
			-0.5,-0.5,0, 0,1,
			1.5,-0.5,0,  1,1,
			1.5,0.5,0,   1,0,
			-0.5,0.5,0,  0,0
		};

		
		meshInfo.isIntIndexType = true;
		meshInfo.vertexType = CFVT_XYZ | CFVF_TEX1(2);
		meshInfo.primitiveType = CFPT_TRIANGLELIST;

		
		if( id == TEMP_TILE_ID )
		{
			float const vtxTri1[] =
			{
				-0.5,-0.5,0, 0,1,
				0.5,-0.5,0,  1,1,
				0.5,0.5,0,   1,0,
				-0.5,0.5,0,  0,0
			};

			float const vtxTri2[] =
			{
				-0.5,-0.5,0, 0,1,
				0.5,-0.5,0,  1,1,
				0.5,0.5,0,   1,0,
				-0.5,0.5,0,  0,0
			};
			int indicesTri[] = { 0,1,2 };
			meshInfo.pIndex = indicesTri;
			meshInfo.numIndices = 3;
			meshInfo.numVertices = 3;

			meshInfo.pVertex = (void*)vtxTri1;
			Material* mat1 = mWorld->createMaterial(Color4f(1, 1, 1));
			obj->createMesh(mat1, meshInfo);

			meshInfo.pVertex = (void*)vtxTri2;
			Material* mat2 = mWorld->createMaterial(Color4f(1, 1, 1));
			obj->createMesh(mat2, meshInfo);
		}
		else
		{
			meshInfo.pIndex = indicesQuad;
			meshInfo.numIndices = 6;
			TileSet const& tileSet = mGameLogic.mTileSetManager.getTileSet(id);
			meshInfo.pVertex = (tileSet.type == ETileClass::Double) ? (void*)vtxDouble : (void*)vtxSingle;
			meshInfo.numVertices = 4;

			Material* mat = mWorld->createMaterial(Color4f(1, 1, 1));
			obj->createMesh(mat, meshInfo);
		}
	}

	void LevelStage::setTileObjectTexture(CFly::Object* obj, TileId id , int idMesh )
	{
		assert(id != TEMP_TILE_ID);
		CFly::Material* mat = obj->getElement(0)->getMaterial();
		if ( mat )
		{
			FixString< 512 > texName;
			getTileTexturePath(id, texName);
			mat->addTexture(0, 0, texName);
			mat->getTextureLayer(0).setFilterMode(CFly::CF_FILTER_POINT);
		}
	}

	void LevelStage::updateTileMesh(CFly::Object* obj , TileId newId , TileId oldId )
	{
		assert(newId != TEMP_TILE_ID && oldId != TEMP_TILE_ID);
		if ( oldId != FAIL_TILE_ID )
		{
			TileSet const& tileSetOld = mGameLogic.mTileSetManager.getTileSet( oldId );
			TileSet const& tileSetNew = mGameLogic.mTileSetManager.getTileSet( newId );
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
		Expansion const usageExpansions[] =
		{
			//EXP_THE_RIVER, 
			//EXP_THE_RIVER_II,
			//EXP_TRADERS_AND_BUILDERS, 
			//EXP_INNS_AND_CATHEDRALS,
			//EXP_THE_PRINCESS_AND_THE_DRAGON,
			//EXP_THE_TOWER,
			//EXP_ABBEY_AND_MAYOR,
			//EXP_KING_AND_ROBBER,
			//EXP_BRIDGES_CASTLES_AND_BAZAARS,
			//EXP_HILLS_AND_SHEEP,
			//EXP_CASTLES,
			EXP_THE_WIND_ROSES ,
		};
		for( auto exp : usageExpansions )
		{
			mSetting.addExpansion(exp);
		}
		mGameLogic.mDebugMode = ::Global::GameConfig().getIntValue("DebugMode", "CAR", 0);
		mGameLogic.mDebugMode |= EDebugModeMask::ShowAllTitles;
		for( int i = 0 ; i < numPlayer ; ++i )
		{
			GamePlayer* player = playerManager.createPlayer(i);
			player->getInfo().type = PT_PLAYER;
		}
		playerManager.setUserID(0);
	}

	void LevelStage::setupScene(IPlayerManager& playerManager)
	{
		for( auto iter = playerManager.createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			switch( player->getType() )
			{
			case PT_PLAYER:
				{
					PlayerBase* carPlayer = new PlayerBase;
					carPlayer->mTeam = player->getSlot();
					mPlayerManager.addPlayer( carPlayer );
					player->setActionPort( carPlayer->getId() );
				}
				break;
			case PT_SPECTATORS:
			case PT_COMPUTER:
				break;
			}
		}

		mGameLogic.setup( mSetting , mRandom , mPlayerManager );

		for( int i = 0 ; i < EField::COUNT ; ++i )
		{
			int idx = mSetting.getFieldIndex( EField::Type(i) );
			if ( idx != -1 )
			{
				mFiledTypeMap[ idx ] = i;
			}
		}

		::Global::GUI().cleanupWidget();

		int userSlotId = playerManager.getUser()->getActionPort();
		if ( getModeType() == EGameStageMode::Net )
		{
			ComWorker* worker = static_cast< NetLevelStageMode* >( getStageMode() )->getWorker();
			mInput.setRemoteSender( new CWorkerDataTransfer( worker , userSlotId ) );
			if ( ::Global::GameNet().getNetWorker()->isServer() )
			{
				mServerDataTranfser = new CSVWorkerDataTransfer(::Global::GameNet().getNetWorker(), MaxPlayerNum );
				mServerDataTranfser->setRecvFunc( RecvFunc( this , &LevelStage::onRecvDataSV ) );
			}
		}
	}

	int LevelStage::getActionPlayerId()
	{
		if ( mInput.getReplyAction() != ACTION_NONE && mInput.getReplyData()->playerId != -1 )
			return mInput.getReplyData()->playerId;

		return mGameLogic.getTurnPlayer()->getId();
	}

	void LevelStage::onRecvDataSV( int slot , int dataId , void* data , int dataSize )
	{
		if ( slot != getActionPlayerId() )
			return;
		mServerDataTranfser->sendData( SLOT_SERVER , dataId , data , dataSize );
	}

	bool LevelStage::isUserTrun()
	{
		return getStageMode()->getPlayerManager()->getUser()->getActionPort() == mGameLogic.getTurnPlayer()->getId();
	}

	bool LevelStage::canInput()
	{
		if ( mGameLogic.mIsStartGame )
		{
			if( getModeType() == EGameStageMode::Single )
				return true;

			if( getModeType() == EGameStageMode::Net )
			{
				if( getActionPlayerId() == getStageMode()->getPlayerManager()->getUser()->getActionPort() )
					return true;
			}
		}

		return false;
	}

	void LevelStage::removeGameActionUI()
	{
		for(auto widget : mGameActionUI)
		{
			widget->destroy();
		}
		mGameActionUI.clear();
	}

	void LevelStage::getTileTexturePath(TileId id, FixString< 512 > &texName)
	{
		TileSet const& tileSet = mGameLogic.mTileSetManager.getTileSet( id );
		char const* dir = nullptr;
		switch( tileSet.expansion )
		{
		case EXP_BASIC: dir = "Basic"; break;
		case EXP_INNS_AND_CATHEDRALS: dir = "InnCathedral"; break;
		case EXP_TRADERS_AND_BUILDERS: dir = "TraderBuilder"; break;
		case EXP_THE_RIVER_I: dir = "River"; break;
		case EXP_THE_RIVER_II: dir = "River2" ; break;
		case EXP_THE_PRINCESS_AND_THE_DRAGON: dir = "PrincessDragon"; break;
		case EXP_THE_TOWER: dir = "Tower"; break;
		case EXP_ABBEY_AND_MAYOR: dir = "AbbiyMayor"; break;
		case EXP_BRIDGES_CASTLES_AND_BAZAARS: dir = "BridgeCastleBazaar"; break;
		case EXP_KING_AND_ROBBER: dir = "KingRobber"; break;
		case EXP_HILLS_AND_SHEEP: dir = "HillsSheep"; break;
		case EXP_CASTLES_IN_GERMANY: dir = "Castle"; break;
		case EXP_HALFLINGS_I: dir = "Halfling1"; break;
		case EXP_HALFLINGS_II: dir = "Halfling2"; break;
		case EXP_THE_WIND_ROSES: dir = "WindRose"; break;
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
		IGraphics2D& g = Global::GetIGraphics2D();

		Vec2i pos  = getWorldPos();
		Vec2i size = getSize();

		//assert( !useColorKey );

		if ( getButtonState() == BS_PRESS )
			RenderUtility::SetPen( g , EColor::Black );
		else
			RenderUtility::SetPen( g , EColor::White );

		switch( info->pos.type )
		{
		case ActorPos::eSideNode:    RenderUtility::SetBrush( g , EColor::Yellow , COLOR_DEEP );  break;
		case ActorPos::eFarmNode:    RenderUtility::SetBrush( g , EColor::Green , COLOR_LIGHT ); break;
		case ActorPos::eTile:        RenderUtility::SetBrush( g , EColor::Orange , COLOR_LIGHT ); break;
		case ActorPos::eTileCorner:  RenderUtility::SetBrush( g , EColor::Yellow  , COLOR_LIGHT ); break;
		default:
			RenderUtility::SetBrush( g , EColor::Gray );
		}
		g.drawRect( pos , size );

		g.setTextColor(Color3ub(0 , 0 , 0) );
		FixString< 128 > str;
		str.format("%s", gActorShortNames[type]);
		g.drawText( pos , size , str , true );
	}

	void SelectButton::onRender()
	{
		IGraphics2D& g = Global::GetIGraphics2D();

		Vec2i pos  = getWorldPos();
		Vec2i size = getSize();

		//assert( !useColorKey );

		if ( getButtonState() == BS_PRESS )
			RenderUtility::SetPen( g , EColor::Black );
		else
			RenderUtility::SetPen( g , EColor::White );

		RenderUtility::SetBrush( g , EColor::Yellow , COLOR_DEEP );
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
				choice->addItem( str );

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

		auto slider = new GSlider( UI_SCORE , Vec2i( 10 , 80 ) , 100 , true , this );
		slider->showValue();
		slider->setRange( 0 , 20 );
		slider->setValue(0);


		auto button = new GButton( UI_OK , Vec2i( 120 , 80 ) , Vec2i( 60 , 25 ) , this );
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

		IGraphics2D& g = Global::GetIGraphics2D();
		Vec2i pos  = getWorldPos();
		Vec2i size = getSize();

		FixString< 512 > str;
		g.setTextColor(Color3ub(255 , 255 , 0) );
		RenderUtility::SetFont( g , FONT_S12 );
		str.format("Round = %d , PlayerId = %d", mData->pIdRound, mData->playerId);
		g.drawText( pos + Vec2i(10,10) , str);
		if ( mData->playerId == mData->pIdRound )
		{

		}
		else
		{
			str.format("Max Score = %d , Id = %d", mData->maxScore, mData->pIdCallMaxScore);
			g.drawText( pos + Vec2i(10,30) , str);
		}
	} 

}//namespace CAR