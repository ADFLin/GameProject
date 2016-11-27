#include "TinyGamePCH.h"
#include "TTScene.h"

#include "DrawEngine.h"
#include "RenderUtility.h"


#include "EasingFun.h"

#include "lodepng/lodepng.h"
#include "IntegerType.h"
#include <fstream>

#include "TextureId.h"
#include "CppVersion.h"

namespace TripleTown
{
	float AnimTime = 0.3f;
	int const GameTilesTexLength = 512;
	int const TileImageLength = 112;
	int const ItemOutLineLength = 112;

	uint32 nextPowOf2( uint32 n )
	{
		n--;
		n |= n >> 1;   // Divide by 2^k for consecutive doublings of k up to 32,
		n |= n >> 2;   // and then or the results.
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n++;
		return n;
	}

#define TILE_POS_X( x , y ) ( (x) * ( TileImageLength / 2 + 3 ) + ( (y) == 0  ? 182 : 0 ) )
#define TILE_POS_Y( x , y ) ( (y) * ( TileImageLength / 2 + 3 ) + ( (y) == 0  ? 0 : ( 112 - TileImageLength / 2 ) ) )
#define TILE_TEX_POS( y , x )  { float( TILE_POS_X( x , y ) ) / GameTilesTexLength  , float( TILE_POS_Y( x , y ) ) / GameTilesTexLength  } ,
	// CVH
	float const gRoadTexCoord[][2] =
	{
		TILE_TEX_POS( 2 , 6 ) TILE_TEX_POS( 6 , 2 ) //E
		TILE_TEX_POS( 1 , 2 ) TILE_TEX_POS( 3 , 3 ) //H
		TILE_TEX_POS( 2 , 5 ) TILE_TEX_POS( 4 , 1 ) //V
		TILE_TEX_POS( 0 , 2 ) TILE_TEX_POS( 0 , 4 ) //C
		TILE_TEX_POS( 0 , 0 ) TILE_TEX_POS( 1 , 3 ) //CC
	};
	float const gLakeTexCoord[][2] = 
	{
		TILE_TEX_POS( 2 , 3 ) TILE_TEX_POS( 2 , 7 ) //E
		TILE_TEX_POS( 1 , 6 ) TILE_TEX_POS( 2 , 4 ) //H
		TILE_TEX_POS( 1 , 6 ) TILE_TEX_POS( 2 , 4 ) //H
		//TILE_TEX_POS( 0 , 3 ) TILE_TEX_POS( 5 , 2 ) //V
		TILE_TEX_POS( 1 , 7 ) TILE_TEX_POS( 4 , 2 ) //C
		TILE_TEX_POS( 2 , 1 ) TILE_TEX_POS( 6 , 1 ) //CC
	};


#define StringOp(A,B,...) B,
	char const* gTextureName[] = 
	{
		TEXTURE_ID_LIST( StringOp )
	};
#undef StringOp

	Vec2i const ItemImageSize = Vec2i( 114 , 224 );
	Vec2i const ItemImageSizeFix = Vec2i( 128 , 256 );

	int const TileLength = 80;
	Vec2i const TileSize = Vec2i( TileLength , TileLength );

	ColorKey3 const MASK_KEY( 255 , 255 , 255 );
	struct ImageInfo
	{
		int id;
		int addition;
		char const* fileName;
	};

	ImageInfo gImageInfo[] =
	{
		{ OBJ_GRASS , 0 , "grass" } ,
		{ OBJ_BUSH , 1 , "bush" } ,
		{ OBJ_TREE , 1 , "tree" } ,
		{ OBJ_HUT , 1 , "hut" } ,
		{ OBJ_HOUSE , 1 , "house" } ,
		{ OBJ_MANSION , 1 , "mansion" } ,
		{ OBJ_CASTLE , 1 , "castle" } ,
		{ OBJ_FLOATING_CASTLE , 0 , "floatingcastle" } ,
		{ OBJ_TRIPLE_CASTLE , 0 , "triplecastle" } ,
		{ OBJ_TOMBSTONE , 0 , "tombstone" } ,
		{ OBJ_CHURCH , 0 , "church"} ,
		{ OBJ_CATHEDRAL , 0 , "cathedral" } ,
		{ OBJ_CHEST , 0 , "treasure" } ,
		{ OBJ_LARGE_CHEST , 0 , "largetreasure" } ,
		{ OBJ_ROCK     , 0 , "rock" } ,
		{ OBJ_MOUNTAIN , 0 , "mountain" } ,

		{ OBJ_STOREHOUSE , 0 , "stash" } ,
		{ OBJ_BEAR , 0 , "bear" } ,
		{ OBJ_NINJA , 0 , "ninja" } ,

		{ OBJ_CRYSTAL , 0 , "crystal" } ,
		{ OBJ_ROBOT , 0  , "bomb" } ,
	};


	Scene::Scene()
	{
		mLevel = NULL;
		mMapOffset = Vec2i( 0 , 0 );
		mNumPosRemove = 0;
		mPosPeek = Vec2i( -1 , -1 );
	}

	float value;
	bool Scene::init()
	{
		//glEnable( GL_DEPTH_TEST );

		glClearColor( 100 / 255.f , 100 / 255.f , 100 / 255.f  , 1 );
		
		if ( !loadResource() )
			return false;

		mItemScale = 0.8f;
		mMouseAnim = nullptr;
		mTweener.tweenValue< Easing::CLinear >( mItemScale , 0.65f , 0.8f , 1 ).cycle();

		return true;
	}

	void Scene::setupLevel( Level& level )
	{
		mLevel = &level;
		mLevel->setAnimManager( this );

	}

	void Scene::render( Graphics2D& g )
	{
		TileMap const& map = mLevel->getMap();

		for( int i = 0 ; i < map.getSizeX() ; ++i )
		{
			for( int j = 0 ; j < map.getSizeY() ; ++j )
			{
				Vec2i pos = mMapOffset + TileLength * Vec2i( i , j );
				Tile const& tile = map.getData( i , j );
				renderTile( g , pos , tile.id , tile.meta );

				if ( tile.haveActor() )
				{
					ActorData& e = mLevel->getActor( tile );
					switch( e.id )
					{
					case OBJ_NINJA:
						RenderUtility::setBrush( g , Color::eRed );
						RenderUtility::setPen( g , Color::eBlack );
						g.drawCircle( pos + TileSize / 2 , 10 );
						break;
					case OBJ_BEAR:
						RenderUtility::setBrush( g , Color::eYellow );
						RenderUtility::setPen( g , Color::eBlack );
						g.drawCircle( pos + TileSize / 2 , 10 );
						break;
					}
				}
				else if ( Level::getInfo( tile.id ).typeClass->getType() == OT_BASIC )
				{
					FixString< 32 > str;
					str.format( "%d" , mLevel->getLinkNum( TilePos( i , j ) ) );
					g.setTextColor( 100 , 0 , 255 );
					g.drawText( pos , str );
				}
			}
		}

		RenderUtility::setPen( g , Color::eRed );
		RenderUtility::setBrush( g , Color::eNull );
		for( int i = 0 ; i < mNumPosRemove ; ++i )
		{
			Vec2i pos = mMapOffset + TileLength * Vec2i( mRemovePos[i].x , mRemovePos[i].y );
			g.drawRect( pos , TileSize );
		}
	}

	void Scene::renderTile( Graphics2D& g , Vec2i const& pos , ObjectId id , int meta /*= 0 */ )
	{
		RenderUtility::setPen( g , Color::eNull );

		switch( id )
		{
		case OBJ_NULL:
			RenderUtility::setBrush( g , Color::eYellow , COLOR_DEEP );
			g.drawRect( pos , TileSize );
			break;
		case OBJ_GRASS:
			{
				Vec2i const border = Vec2i( 4 , 4 );
				RenderUtility::setBrush( g , Color::eGreen , COLOR_LIGHT );
				g.drawRect( pos , TileSize );
				RenderUtility::setBrush( g , Color::eGreen );
				g.drawRect( pos + border , TileSize - 2 * border );
			}
			break;
		case OBJ_BUSH:
			{
				RenderUtility::setBrush( g , Color::eGreen , COLOR_LIGHT );
				g.drawRect( pos , TileSize );
				RenderUtility::setBrush( g , Color::eGreen );
				g.drawCircle( pos + TileSize / 2 , TileLength / 2 - 10 );
			}
			break;
		case OBJ_TREE:
			{
				Vec2i bSize( 10 , 20 );
				RenderUtility::setBrush( g , Color::eGreen , COLOR_LIGHT );
				g.drawRect( pos , TileSize );
				RenderUtility::setBrush( g , Color::eCyan );
				g.drawRect( pos + TileSize / 2 - bSize / 2 , bSize );
				RenderUtility::setBrush( g , Color::eGreen );
				g.drawCircle( pos + TileSize / 2 , TileLength / 2 - 12 );
			}
			break;
		case OBJ_STOREHOUSE:
			{
				RenderUtility::setBrush( g , Color::eRed );
				g.drawRect( pos , TileSize );

				if ( meta )
				{
					renderTile( g , pos , ObjectId( meta ) );
				}
			}
			break;
		default:
			{
				Vec2i bSize( TileLength - 16  , TileLength - 16 );
				RenderUtility::setBrush( g , Color::eRed );
				g.drawRect( pos + TileSize / 2 - bSize / 2 , bSize );
				FixString< 32 > str;
				str.format( "%d" , id );
				g.drawText( pos , TileSize , str );
			}
		}
	}

	void Scene::postCreateLand()
	{
		mMap.resize( mLevel->getMap().getSizeX() , mLevel->getMap().getSizeY() );

		Vec2i screenSize = ::Global::getDrawEngine()->getScreenSize();
		Vec2i mapSize = TileLength * Vec2i( mLevel->getMap().getSizeX() , mLevel->getMap().getSizeY() );
		mMapOffset = ( screenSize - mapSize ) / 2;

		for( int i = 0 ; i < mMap.getSizeX() ; ++i )
		{
			for( int j = 0 ; j < mMap.getSizeY() ; ++j )
			{
				TileData& data = mMap.getData( i , j );
				data.pos.setValue( TileImageLength * i , TileImageLength * j );
				data.scale = 1.0f;
			}
		}
	}

	void Scene::click( Vec2i const& pos )
	{
		TilePos tPos = calcTilePos( pos );

		if ( !mLevel->isVaildMapRange( tPos ) )
			return;
		mLevel->clickTile( tPos );
	}

	void Scene::peekObject( Vec2i const& pos )
	{
		TilePos tPos = calcTilePos( pos );

		if ( mPosPeek == tPos )
			return;

		if ( !mLevel->isVaildMapRange( tPos ) )
			return;

		if ( mMouseAnim )
		{
			mTweener.remove( mMouseAnim );
			mMouseAnim = nullptr;
			for( int i = 0 ; i < mNumPosRemove ; ++i )
			{
				TilePos& pos = mRemovePos[i];
				TileData& tile = mMap.getData( pos.x , pos.y );
				tile.pos = float( TileImageLength ) * Vec2f( pos );
			}
		}


		mNumPosRemove = mLevel->peekObject( tPos , mLevel->getQueueObject() , mRemovePos );
		mPosPeek = tPos;

		if ( mNumPosRemove )
		{
			Tweener::CMultiTween& tween = mTweener.tweenMulti( 1 ).cycle();
			for( int i = 0 ; i < mNumPosRemove ; ++i )
			{
				TilePos& pos = mRemovePos[i];
				TileData& tile = mMap.getData( pos.x , pos.y );
				Vec2f from = float( TileImageLength ) * Vec2f( pos );
				Vec2f offset = Vec2f( mPosPeek - pos );
				offset *= ( 0.15f * TileImageLength );
				Vec2f to = from + offset;
				tween.addValue< Easing::CLinear >( tile.pos , from , to );
			}
			mMouseAnim = &tween;
		}
	}

	bool Scene::loadFile( char const* name , GLuint& tex , unsigned& w , unsigned& h )
	{
		unsigned char* dataLoad;
		unsigned error = lodepng_decode_file( &dataLoad , &w , &h , name , LCT_RGBA , 8 );
		if ( error )
			return false;

		glGenTextures( 1 , &tex );
		glBindTexture( GL_TEXTURE_2D , tex );
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D , 0, GL_RGBA, w , h , 0, GL_RGBA, GL_UNSIGNED_BYTE , dataLoad );
		return true;
	}


	void Scene::render()
	{
		glDisable( GL_CULL_FACE );
		glDisable( GL_DEPTH_TEST );
		glColor3f(1,1,1);

		float scaleItem = 0.8f;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		int wScreen = ::Global::getDrawEngine()->getScreenWidth();
		int hScreen = ::Global::getDrawEngine()->getScreenHeight();

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glOrtho( 0 , wScreen ,  hScreen , 0  , -100 , 100 );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		float scaleMap = float( TileLength ) / TileImageLength;
	
		glPushMatrix();
		glTranslatef( mMapOffset.x , mMapOffset.y , 0 );
		glScalef( scaleMap , scaleMap , 1 );
#if 0
		for( int i = 0 ; i < ARRAY_SIZE( gImageInfo ) ; ++i )
		{
			glLoadIdentity();
			ImageInfo& info = gImageInfo[i];
			int ox = ( info.w - 96 ) / 2;
			int oy = ( info.h - 96 ) / 2;
			glTranslated( 100 * ( i % 5 ) - ox + 100 , 100 * ( i / 5 ) - oy + 100 , 0 );
			drawItemImage( mTexItemMap[ info.id ][0] );
			
		}
#endif
		TileMap const& map = mLevel->getMap();

		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D , mTexMap[ TID_GAME_TILES ] );

		for( int j = 0 ; j < map.getSizeY() ; ++j )
		{
			for( int i = 0 ; i < map.getSizeX() ; ++i )
			{
				Vec2i pos = mMapOffset + TileLength * Vec2i( i , j );
				Tile const& tile = map.getData( i , j );

				glPushMatrix();
				glTranslated( i * TileImageLength , j * TileImageLength , -1 );

				switch( tile.getTerrain() )
				{
				case TT_ROAD:
					for ( int dir = 0 ; dir < 4 ; ++dir )
						drawRoadTile( i , j , dir );
					break;
				case TT_WATER:
					for ( int dir = 0 ; dir < 4 ; ++dir )
						drawLakeTile( i , j , dir );
					break;
				case TT_GRASS:
				default:
					float tw = float( TileImageLength ) / GameTilesTexLength;
					float th = float( TileImageLength ) / GameTilesTexLength;
					drawQuad( TileImageLength , TileImageLength , 0 , th , tw , -th );
				}
				glPopMatrix();
			}
		}

		glDisable( GL_TEXTURE_2D );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );

		TilePos posTileMouse = calcTilePos( mLastMousePos );
		bool renderQueue = true;

		for( int j = 0 ; j < map.getSizeY() ; ++j )
		{
			for( int i = 0 ; i < map.getSizeX() ; ++i )
			{
				Vec2i pos = mMapOffset + TileLength * Vec2i( i , j );

				Tile const& tile = map.getData( i , j );

				glPushMatrix();
				if ( tile.isEmpty() )
				{
					if ( posTileMouse.x == i && posTileMouse.y == j )
					{
						renderQueue = false;
					}
				}
				else
				{
					TileData const& data = mMap.getData( i , j );
					if ( tile.haveActor() )
					{
						glTranslatef( data.pos.x , data.pos.y , j );
						glPushMatrix();
						glTranslatef( ( TileImageLength - ItemImageSize.x ) / 2 , TileImageLength - ItemImageSize.y , 0 );
						drawItem( mTexMap[ TID_ITEM_SHADOW_LARGE ] );

						ActorData& e = mLevel->getActor( tile );
						drawItem( mTexItemMap[ e.id ][ 0 ] );
						glPopMatrix();
					}
					else if ( tile.id != OBJ_NULL )
					{
						glTranslatef( data.pos.x , data.pos.y , j );
						glPushMatrix();
						glTranslatef( ( TileImageLength - ItemImageSize.x ) / 2 , TileImageLength - ItemImageSize.y , 0 );
						drawItem( mTexMap[ TID_ITEM_SHADOW_LARGE ] );

						drawItem( mTexItemMap[ tile.id ][ 0 ] );
						glPopMatrix();

						if ( tile.id == OBJ_STOREHOUSE )
						{
							if ( tile.meta )
							{
								glPushMatrix();
								glTranslatef( TileImageLength / 2 , TileImageLength / 2 , 0.1 );
								glScalef( scaleItem , scaleItem , 1 );
								drawItemCenter( mTexItemMap[ tile.meta ][ 0 ] );
								glPopMatrix();
							}
						}
					}
				}
				glPopMatrix();
			}
		}

		
		if ( renderQueue )
		{
			if ( mLevel->isVaildMapRange( posTileMouse ) )
			{
				glPushMatrix();
				glLoadIdentity();
				glTranslatef( mLastMousePos.x , mLastMousePos.y  , 10 );
				glScalef( mItemScale * scaleMap , mItemScale * scaleMap , 1 );
				drawItemCenter( mTexItemMap[ mLevel->getQueueObject() ][ 0 ] );
				//drawItemOutline( mTexItemMap[ mLevel->getQueueObject() ][ 2 ] );
				glPopMatrix();
			}
		}
		else
		{
			glPushMatrix();
			glTranslatef( posTileMouse.x * TileImageLength , posTileMouse.y * TileImageLength , 50 );
			drawImage( mTexMap[ TID_UI_CURSOR ] , TileImageLength , TileImageLength );
			glTranslatef(  TileImageLength / 2 , TileImageLength / 2 , 0 );
			glScalef( mItemScale , mItemScale , 1 );
			drawItemCenter( mTexItemMap[ mLevel->getQueueObject() ][ 0 ] );
			//drawItemOutline( mTexItemMap[ mLevel->getQueueObject() ][ 2 ] );
			glPopMatrix();
		}
		
		

#if 1
		glPushMatrix();
		glLoadIdentity();
		glTranslatef( 0 , 0 , 20 );
		glScalef( scaleMap , scaleMap , 1 );
		drawImageInvTexY( mTexMap[ TID_GRASS_OUTLINE ] , TileImageLength , TileImageLength );
		glPopMatrix();
#endif
		glDisable( GL_BLEND );

		glPopMatrix();
	}

	void Scene::drawItemOutline( GLuint tex )
	{
		int x =  - ItemOutLineLength / 2;
		int y =  - ItemOutLineLength / 2;
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D , tex );
		drawQuad( x , y , ItemOutLineLength , ItemOutLineLength , 0 , 1 , 1 , -1 );
		glDisable( GL_TEXTURE_2D );
	}

	void Scene::drawItem( GLuint tex )
	{
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D , tex );
		drawQuad( ItemImageSize.x , ItemImageSize.y , 0 , 1 , 1 , -1 );
		glDisable( GL_TEXTURE_2D );
	}

	void Scene::drawItemCenter( GLuint tex )
	{
		int x =  - ItemImageSize.x / 2;
		int y = TileImageLength / 2 - ItemImageSize.y;
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D , tex );
		drawQuad( x , y , ItemImageSize.x , ItemImageSize.y , 0 , 1 , 1 , -1 );
		glDisable( GL_TEXTURE_2D );
	}

	void Scene::drawItem( Texture& tex  )
	{
		glEnable( GL_TEXTURE_2D );
		tex.bind();
		drawQuad( ItemImageSize.x , ItemImageSize.y , 0 , 1 * tex.mScaleT , 1 * tex.mScaleS , -1 * tex.mScaleT );
		glDisable( GL_TEXTURE_2D );
	}

	void Scene::drawItemCenter( Texture& tex )
	{
		int x =  - ItemImageSize.x / 2;
		int y = TileImageLength / 2 - ItemImageSize.y;
		glEnable( GL_TEXTURE_2D );
		tex.bind();
		drawQuad( x , y , ItemImageSize.x , ItemImageSize.y , 0 , 1 * tex.mScaleT , 1 * tex.mScaleS , -1 * tex.mScaleT );
		glDisable( GL_TEXTURE_2D );
	}


	void Scene::drawQuadRotateTex90( int x , int y , int w , int h , float tx , float ty , float tw , float th )
	{
		int x2 = x + w;
		int y2 = y + h;
		float tx2 = tx + tw;
		float ty2 = ty + th;
		glBegin( GL_QUADS );
		glTexCoord2f( tx2 , ty  );glVertex2i( x  , y  ); 
		glTexCoord2f( tx2 , ty2 );glVertex2i( x2 , y  );
		glTexCoord2f( tx  , ty2 );glVertex2i( x2 , y2 );
		glTexCoord2f( tx  , ty  );glVertex2i( x  , y2 );
		glEnd();
	}


	void Scene::drawQuad( int x , int y , int w , int h , float tx , float ty , float tw , float th )
	{
		int x2 = x + w;
		int y2 = y + h;
		float tx2 = tx + tw;
		float ty2 = ty + th;
		glBegin( GL_QUADS );
		glTexCoord2f( tx  , ty  );glVertex2i( x  , y  ); 
		glTexCoord2f( tx2 , ty  );glVertex2i( x2 , y  );
		glTexCoord2f( tx2 , ty2 );glVertex2i( x2 , y2 );
		glTexCoord2f( tx  , ty2 );glVertex2i( x  , y2 );
		glEnd();
	}

	void Scene::drawQuad( int w , int h , float tx , float ty , float tw , float th )
	{
		float tx2 = tx + tw;
		float ty2 = ty + th;
		glBegin( GL_QUADS );
		glTexCoord2f( tx  , ty  );glVertex2i( 0 , 0 ); 
		glTexCoord2f( tx2 , ty  );glVertex2i( w , 0 );
		glTexCoord2f( tx2 , ty2 );glVertex2i( w , h );
		glTexCoord2f( tx  , ty2 );glVertex2i( 0 , h );
		glEnd();
	}

	void Scene::drawRoadTile( int x , int y , int dir )
	{
		drawTileImpl( x , y , dir , TT_ROAD , gRoadTexCoord );
	}


	void Scene::drawLakeTile( int x , int y , int dir )
	{
		drawTileImpl( x , y , dir , TT_WATER , gLakeTexCoord );
	}

	void Scene::drawTileImpl( int x , int y , int dir , TerrainType tId , float const (*texCoord)[2] )
	{
		int const offsetX[] = { -1 , 1 , -1 , 1 };
		int const offsetY[] = { -1 , -1 , 1 , 1 };

		int const offsetFactorX[] = { 0 , 1 , 0 , 1 };
		int const offsetFactorY[] = { 0 , 0 , 1 , 1 };

		int xCorner = x + offsetX[ dir ];
		int yCorner = y + offsetY[ dir ];

		unsigned bitH = mLevel->getTerrain( TilePos( xCorner , y ) ) != tId ? 1 : 0 ;
		unsigned bitV = mLevel->getTerrain( TilePos( x , yCorner ) ) != tId ? 1 : 0 ;
		unsigned char con;
		con = ( bitV << 1 ) | bitH;
		if ( con == 0 )
		{
			unsigned bitC = mLevel->getTerrain( TilePos( xCorner , yCorner ) ) != tId ? 1 : 0 ;
			con |= ( bitC << 2 );
		}

		TileData& data = mMap.getData( x , y );
		data.con[dir] = con;
		
		float const tLen = float( TileImageLength / 2 ) / GameTilesTexLength;
		int idx = 2 * (int)con + (( dir % 2 ) ? 1 : 0 );

		float tx = texCoord[ idx ][ 0 ];
		float ty = texCoord[ idx ][ 1 ];
		float tw = tLen; 
		float th = tLen;
		//  0   1
		//  2   3

		bool checkSpecial = ( con == 0x2 && tId == TT_WATER );
		if ( con )
		{
			switch( dir )
			{
			case 0:
				tw = -tLen; th = -tLen;
				tx += tLen; ty += tLen;
				if ( checkSpecial )
				{
					tx += tw;
					tw = -tw;
				}
				break;	
			case 1:
				tw = tLen; th = -tLen;
				ty += tLen;
				break;
			case 2:
				tw = -tLen; th = tLen;
				tx += tLen;
				break;
			case 3:
				tw = tLen; th = tLen;
				if ( checkSpecial )
				{
					tx += tw;
					tw = -tw;
				}
				break;
			}
			if ( con == 0x2 )
			{
				ty += th;
				th = -th;
			}
		}
		int ox = offsetFactorX[ dir ] * TileImageLength / 2;
		int oy = offsetFactorY[ dir ] * TileImageLength / 2;
		if ( checkSpecial )
			drawQuadRotateTex90( ox , oy , TileImageLength / 2 , TileImageLength / 2 , tx , ty , tw , th );
		else
			drawQuad( ox , oy , TileImageLength / 2 , TileImageLength / 2 , tx , ty , tw , th );

	}

	void Scene::drawTile( int x , int y , Tile const& tile )
	{

	}

	bool Scene::loadResource()
	{
		char const* dir = "TripleTown";
		FixString< 128 > path;
		unsigned w , h;

		uint32 data[] = 
		{ 
			0x00 , 0x00 , 0x00 , 0x00 , 
		};

		GLuint tex;
		glGenTextures( 1 , &tex );
		glBindTexture( GL_TEXTURE_2D , tex );	
		glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D , 0 , GL_RGBA , 2 , 2 , 0, GL_RGBA , GL_UNSIGNED_BYTE , &data[0] );

		GLenum error = glGetError();
		if ( error != GL_NO_ERROR )
		{
			int i = 1;
		}

		std::fill_n( mTexMap , (int)TEX_ID_NUM , 0 );

		for( int i = 0 ; i < ARRAY_SIZE( gImageInfo ) ; ++i )
		{
			ImageInfo& info = gImageInfo[i];

			path.format( "%s/item_%s.tex" , dir , info.fileName );
			
			if ( !loadTexFile( path , mTexItemMap[ info.id ][ 0 ] , w , h ) )
				return false;

			//if ( !mTexItemMap[ info.id ][ 0 ].loadFile( path ) )
				//return false;

			if ( info.addition )
			{
				path.format( "%s/item_%s2.tex" , dir , info.fileName );
				if ( !loadTexFile( path , mTexItemMap[ info.id ][ 1 ] , w , h ) )
					return false;

				//if ( !mTexItemMap[ info.id ][ 1 ].loadFile( path ) )
					//return false;
			}

			path.format( "%s/item_%s_outline.tex" , dir , info.fileName );
			if ( !loadTexFile( path , mTexItemMap[ info.id ][ 2 ] , w , h ) )
				return false;
		}

		for( int i = 0 ; i < TEX_ID_NUM ; ++i )
		{
			path.format( "%s/%s.tex" , dir , gTextureName[i] );
			if ( !loadTexFile( path , mTexMap[ i ] , w , h ) )
				return false;
		}
		return true;
	}


	struct TexHeader
	{
		uint32 width;
		uint32 height;
		uint32 size;
		uint32 colorBits;
		uint32 unkonw[9];
		uint32 dataSize;
	};

	struct Color16
	{
		uint16 a : 4;
		uint16 b : 4;
		uint16 g : 4;
		uint16 r : 4;
	};


	bool Scene::loadTexFile( char const* path , GLuint& tex , unsigned& w , unsigned& h )
	{
		using std::ios;
		std::ifstream fs( path , ios::binary );
		if ( !fs.is_open() )
			return false;

		fs.seekg( 0 , ios::beg );
		ios::pos_type pos = fs.tellg();

		fs.seekg( 0 , ios::end );
		int sizeFile = fs.tellg() - pos;

		fs.seekg( 0 , ios::beg );

		TexHeader header;
		fs.read( (char*)&header , sizeof( header ) );

		w = header.width;
		h = header.height;
		std::vector< unsigned char > data;
		switch ( header.colorBits )
		{
		case 4:
			data.resize( header.dataSize );
			fs.read( (char*)&data[0] , header.dataSize );
			break;
		case 2:
			{
				data.resize( header.dataSize * 2 );
				unsigned char* pData = &data[0];
				Color16 c;
				assert( sizeof(c) == 2 );
				for ( int i = 0 ; i < w * h ; ++i )
				{
					fs.read( (char*)&c , sizeof(c) );

					pData[0] = unsigned char( unsigned( 255 * c.r ) / 16 );
					pData[1] = unsigned char( unsigned( 255 * c.g ) / 16 );
					pData[2] = unsigned char( unsigned( 255 * c.b ) / 16 );
					pData[3] = unsigned char( unsigned( 255 * c.a ) / 16 );

					pData  += 4;
				}
			}
			break;
		}
		glGenTextures( 1 , &tex );
		glBindTexture( GL_TEXTURE_2D , tex );	
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D , 0 , GL_RGBA , w , h , 0, GL_RGBA , GL_UNSIGNED_BYTE , &data[0] );

		GLenum error = glGetError();
		if ( error != GL_NO_ERROR )
		{
			::Msg( "Load texture Fail error code = %d" , (int)error );
			//return false;
		}
		return true;
	}

	TilePos Scene::calcTilePos( Vec2i const& pos )
	{
		TilePos tPos;
		tPos.x = ( pos.x - mMapOffset.x ) / TileLength;
		tPos.y = ( pos.y - mMapOffset.y ) / TileLength;
		return tPos;
	}

	void Scene::drawImage( GLuint tex , int w , int h )
	{
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D , tex );
		drawQuad( w , h , 0 , 0 , 1 , 1 );
		glDisable( GL_TEXTURE_2D );
	}

	void Scene::drawImageInvTexY( GLuint tex , int w , int h )
	{
		glEnable( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D , tex );
		drawQuad( w , h , 0 , 1 , 1 , -1 );
		glDisable( GL_TEXTURE_2D );
	}



	template< class Fun >
	class MoveFun
	{
	public:
		Vec2f operator()(float t, Vec2f const& b, Vec2f const& c, float const& d ) 
		{
			float const height = 1500;
			float const dest = d / 2;
			if ( t < dest )
				return Fun()( t , b , Vec2f( 0, -height ) , dest );
			return Fun()( t - dest , b + c - Vec2f( 0, height ) , Vec2f( 0, height ) , dest );
		}
		
	};
	void Scene::moveActor( ObjectId id , TilePos const& posFrom , TilePos const& posTo )
	{
		TileData& data = mMap.getData( posTo.x , posTo.y );

		Vec2f posRFrom = TileImageLength * Vec2f( posFrom );
		Vec2f posRTo = TileImageLength * Vec2f( posTo );
		switch( id )
		{
		case OBJ_BEAR:
			mTweener.tweenValue< Easing::IOQuad >( data.pos , posRFrom , posRTo , AnimTime , 0 );
			break;
		case OBJ_NINJA:
			mTweener.tweenValue< MoveFun< Easing::Linear > >( data.pos , posRFrom , posRTo , AnimTime , 0 );
			break;
		}
	}

	void Scene::updateFrame( int frame )
	{
		float delta = float( frame * gDefaultTickTime ) / 1000;
		mTweener.update( delta );
	}

	Scene::~Scene()
	{
		if ( mLevel )
			mLevel->setAnimManager( NULL );

		releaseResource();
	}

	void Scene::releaseResource()
	{

	}


	bool Texture::loadFile( char const* path )
	{
		using std::ios;
		std::ifstream fs( path , ios::binary );
		if ( !fs.is_open() )
			return false;

		TexHeader header;
		fs.read( (char*)&header , sizeof( header ) );

		uint32 fixW = nextPowOf2( header.width );
		uint32 fixH = nextPowOf2( header.height );
	
		mScaleS = float( header.width ) / fixW;
		mScaleT = float( header.height ) / fixH;

		std::vector< unsigned char > data;

		if ( fixW == header.width && fixH == header.height )
		{
			switch ( header.colorBits )
			{
			case 4:
				data.resize( header.dataSize );
				fs.read( (char*)&data[0] , header.dataSize );
				break;
			case 2:
				{
					data.resize( header.dataSize * 2 );
					unsigned char* pData = &data[0];
					Color16 c;
					assert( sizeof(c) == 2 );
					for ( int i = 0 ; i < header.width * header.height ; ++i )
					{
						fs.read( (char*)&c , sizeof(c) );

						pData[0] = unsigned char( unsigned( 255 * c.r ) / 16 );
						pData[1] = unsigned char( unsigned( 255 * c.g ) / 16 );
						pData[2] = unsigned char( unsigned( 255 * c.b ) / 16 );
						pData[3] = unsigned char( unsigned( 255 * c.a ) / 16 );

						pData += 4;
					}
				}
				break;
			}
		}
		else
		{
			switch ( header.colorBits )
			{
			case 4:
				{
					data.resize( 4 * fixW * fixH , 0 );
					unsigned offset = 4 * fixW;
					unsigned char* pData = &data[0];
					for ( int i = 0 ; i < header.height ; ++i )
					{
						fs.read( (char*)pData , header.width * 4 );
						pData += offset;
					}
				}
				break;
			case 2:
				{
					data.resize( header.dataSize * 2 , 0 );
					unsigned char* pData = &data[0];
					Color16 c;
					assert( sizeof(c) == 2 );
					for ( int i = 0 ; i < header.width * header.height ; ++i )
					{
						fs.read( (char*)&c , sizeof(c) );

						pData[0] = unsigned char( unsigned( 255 * c.r ) / 16 );
						pData[1] = unsigned char( unsigned( 255 * c.g ) / 16 );
						pData[2] = unsigned char( unsigned( 255 * c.b ) / 16 );
						pData[3] = unsigned char( unsigned( 255 * c.a ) / 16 );

						pData  += 4;
					}
				}
				break;
			}
		}
		glGenTextures( 1 , &mTex );
		glBindTexture( GL_TEXTURE_2D , mTex );	
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );
		glTexImage2D( GL_TEXTURE_2D , 0 , GL_RGBA , fixW , fixH , 0, GL_RGBA , GL_UNSIGNED_BYTE , &data[0] );

		GLenum error = glGetError();
		if ( error != GL_NO_ERROR )
		{
			::Msg( "Load texture Fail error code = %d" , (int)error );
			//return false;
		}
		return true;

	}

}//namespace TripleTown