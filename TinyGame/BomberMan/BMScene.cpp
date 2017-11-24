#include "BMPCH.h"
#include "BMScene.h"

#include "RenderUtility.h"

#include "GIFLoader.h"
#include "BitmapDC.h"
#include "MarcoCommon.h"

#include "BMRender.h"

#include <algorithm>

//#TODO : fix animation of dangerous bomb

namespace BomberMan
{
	class Texture : public GdiTexture
	{
	public:
		Color3ub getTransColor(){ return mTransColor; }
		void      setTransColor( Color3ub color ){ mTransColor = color; }

		bool load( HDC hDC , char const* path )
		{
			return createFromFile( hDC , path );
		}

		class MyLoader : public GIFLoader::Listener
		{
		public:
			bool loadImage( GIFImageInfo const& info )
			{
				uint8* data;
				tex->create( hDC , info.imgWidth , info.imgHeight , (void**)&data );

				switch( info.bpp )
				{
				case 8:
					for ( int j = info.imgHeight - 1 ; j >= 0 ; --j )
					{
						uint8* pIndex = (uint8*)info.raster +  j * info.bytesPerRow;
						for ( int i = 0 ; i < info.imgWidth ; ++i )
						{
							COLOR& color = info.palette[ *(pIndex++) ];
							*(data++) = color.b;
							*(data++) = color.g;
							*(data++) = color.r;
							*(data++) = 0;
						}
					}
					if ( info.transparent != -1 )
					{
						COLOR& color = info.palette[ info.transparent ];
						transColor.r = color.r;
						transColor.g = color.g;
						transColor.b = color.b;
					}
					break;
				case 24:
					for ( int j = info.imgHeight - 1 ; j >= 0 ; --j )
					{
						uint8* pIndex = (uint8*)info.raster +  j * info.bytesPerRow;
						for ( int i = 0 ; i < info.imgWidth ; ++i )
						{
							*(data++) = *(pIndex++);
							*(data++) = *(pIndex++);
							*(data++) = *(pIndex++);
							*(data++) = 0;
						}
					}
					break;
				}

				return false;
			}

			Color3ub   transColor;
			HDC         hDC;
			GdiTexture* tex;
		};

		bool load( HDC hDC , GIFLoader& gifLoader , char const* path )
		{
			MyLoader texLoader;
			texLoader.tex = this;
			texLoader.hDC = hDC;
			if ( !gifLoader.load( path  , texLoader ) )
				return false;
			mTransColor = texLoader.transColor;
			return true;
		}

		Color3ub mTransColor;
	};


	enum AnimFlag
	{
		AF_ONCE    = 1,
		AF_RETRACE = 2,
		AF_CYCLE   = 3,
		AF_FRAME_MASK  = ( 1 << 2 ) - 1 ,
	
		AF_HIDE        = 1 << 4,
		AF_IMG_OFFET_Y = 1 << 5,		
	};


	class SpriteAnim
	{
	public:
		SpriteAnim()
		{
			mAnimMap.resize( 8 , -1 );
		}
		void addAnim( int animId , int numFrame , Vec2i const& frameSize , Vec2i const& pos  , unsigned flag = AF_CYCLE );
		void addAnimGroup( int animId  , int numFrame , Vec2i const& frameSize , Vec2i const pos[] , int numAnim  , unsigned flag = AF_CYCLE );

		void addAnimDesc( int animId , Vec2i const& frameSize, int numFrame , unsigned flag );

		bool haveAnim( int animId ) const { return animId <= mAnimMap.size() && mAnimMap[ animId ] != -1; }
		int  getAnimFrameLength( int animId );

		void setTexture( Texture& tex ){ mTex = &tex; }
		void render( Graphics2D& g , Vec2i const& pos , int animId , int frame , int idx = 0 );
		
	private:
		int  evalFrame( unsigned flag , int numFrame , int frame );
		typedef int (*FrameFun)( unsigned , int , int );
		struct AnimKey
		{
			Vec2i    pos;
		};

		Color3ub  mTransColor;
		Texture*   mTex;
		std::vector< AnimKey > mAnimKeys;
		struct AnimDesc
		{
			int      keyPos;
			int      numFrame;
			Vec2i    frameSize;
			unsigned flag;
			FrameFun fun;
		};
		std::vector< AnimDesc > mAnimDesc;
		std::vector< int >      mAnimMap;

	};

	void SpriteAnim::render( Graphics2D& g , Vec2i const& pos , int animId , int frame , int idx /*= 0 */ )
	{
		assert( haveAnim( animId ) );

		AnimDesc& desc = mAnimDesc[ mAnimMap[ animId ] ];
		AnimKey&  key  = mAnimKeys[ desc.keyPos + idx ];

		frame = (*desc.fun)( desc.flag , desc.numFrame , frame );
		//frame = evalFrame( desc.flag , desc.numFrame , frame );
		if ( frame == -1 )
			return;
		
		if ( desc.flag & AF_IMG_OFFET_Y )
		{
			g.drawTexture( *mTex , pos , key.pos + Vec2i( 0 , desc.frameSize.y ) , desc.frameSize , mTex->getTransColor() );
		}
		else
		{
			g.drawTexture( *mTex , pos , key.pos + Vec2i( frame * desc.frameSize.x , 0 ) , desc.frameSize , mTex->getTransColor() );
		}
	}

	int SpriteAnim::getAnimFrameLength( int animId )
	{
		assert( haveAnim( animId ) );
		AnimDesc& desc = mAnimDesc[ mAnimMap[ animId ] ];
		if ( desc.flag & AF_RETRACE )
			return  2 * desc.numFrame - 1;
		return desc.numFrame;
	}

	static int FrameFunCycle( unsigned flag , int numFrame , int frame )
	{
		assert( (flag & AF_FRAME_MASK ) == AF_CYCLE );
		frame = frame % numFrame;
		if ( frame < 0 )
			frame += numFrame;
		return frame;
	}
	static int FrameFunOnce( unsigned flag , int numFrame , int frame )
	{
		assert( (flag & AF_FRAME_MASK ) == AF_ONCE );
		if ( frame >= numFrame )
		{
			if ( flag & AF_HIDE )
				return -1;
			return numFrame -1;
		}
		return frame;
	}

	static int FrameFunRetrace( unsigned flag , int numFrame , int frame )
	{
		assert( (flag & AF_FRAME_MASK ) == AF_RETRACE );

		if ( numFrame > 1 )
		{
			int idx  = frame % ( numFrame - 1 );
			int cycle = frame / ( numFrame - 1 );
			if ( (flag & AF_HIDE) && cycle > 1 )
			{
				return -1;
			}
			else
			{
				if ( cycle % 2 )
					return numFrame - idx - 1;
				else 
					return idx;
			}
		}
		return 0;
	}


	int SpriteAnim::evalFrame( unsigned flag , int numFrame , int frame )
	{
		switch( flag & AF_FRAME_MASK )
		{
		case AF_CYCLE:
			frame = frame % numFrame;
			if ( frame < 0 )
				frame += numFrame;
			return frame;
		case AF_ONCE:
			if ( frame >= numFrame )
			{
				if ( flag & AF_HIDE )
					return -1;
				return numFrame -1;
			}
			return frame;
		case AF_RETRACE:
			if ( numFrame > 1 )
			{
				int idx  = frame % ( numFrame - 1 );
				int cycle = frame / ( numFrame - 1 );
				if ( (flag & AF_HIDE) && cycle > 1 )
				{
					return -1;
				}
				else
				{
					if ( cycle % 2 )
						return numFrame - idx - 1;
					else 
						return idx;
				}
			}
			return 0;
		}

		return -1;
	}

	void SpriteAnim::addAnimGroup( int animId , int numFrame , Vec2i const& frameSize , Vec2i const pos[] , int numAnim , unsigned flag /*= AF_CYCLE */ )
	{
		addAnimDesc( animId , frameSize, numFrame, flag );

		int idx = mAnimKeys.size();
		mAnimKeys.resize( mAnimKeys.size() + numAnim );

		for( int i = 0 ; i < numAnim ; ++ i )
		{
			AnimKey& key = mAnimKeys[ idx + i ];
			key.pos = pos[i];
		}
	}

	void SpriteAnim::addAnim( int animId , int numFrame , Vec2i const& frameSize , Vec2i const& pos , unsigned flag /*= AF_CYCLE */ )
	{
		addAnimDesc( animId , frameSize , numFrame , flag );
		
		int idx = mAnimKeys.size();
		mAnimKeys.resize( mAnimKeys.size() + 1 );

		AnimKey& key = mAnimKeys[ idx ];
		key.pos = pos;
	}

	void SpriteAnim::addAnimDesc( int animId , Vec2i const& frameSize, int numFrame , unsigned flag )
	{
		if ( mAnimMap.size() <= animId )
		{
			mAnimMap.resize( animId + 1 , -1 );
		}
		assert( mAnimMap[ animId ] == -1 );

		mAnimMap[ animId ] = (int)mAnimDesc.size();
		mAnimDesc.resize( mAnimDesc.size() + 1 );

		AnimDesc& desc = mAnimDesc[ mAnimMap[ animId ] ];

		desc.frameSize = frameSize;
		desc.numFrame  = numFrame;
		desc.flag      = flag;
		desc.keyPos    = mAnimKeys.size();

		switch( flag & AF_FRAME_MASK )
		{
		case AF_CYCLE:    desc.fun = &FrameFunCycle; break;
		case AF_ONCE:     desc.fun = &FrameFunOnce; break;
		case AF_RETRACE:  desc.fun = &FrameFunRetrace; break;
		default:
			assert(0);
		}
	}

	enum BombAnim
	{
		ANIM_BOMB_HOLD ,
		ANIM_BOMB_FIRE ,
		ANIM_BOMB_FIRE_C ,
		ANIM_BOMB_FIRE_H ,
		ANIM_BOMB_FIRE_V ,
		ANIM_BOMB_FIRE_SIDE ,
	};

	enum TileAnim
	{
		ANIM_TILE_BLOCK_STAND ,
		ANIM_TILE_WALL_STAND ,
		ANIM_TILE_WALL_BURNING ,
		ANIM_TILE_DIRT ,
		ANIM_TILE_RAIL ,
		ANIM_TILE_ARROW ,
		ANIM_TILE_ARROW_CHANGE ,

	};


	enum ItemAnim
	{
		ANIM_ITEM_STAND ,
		ANIM_ITEM_BLINK ,
	};

	int gItemAnimMap[ NUM_ITEM_TYPE ] = { -1 };


	class ResManager
	{
	public:
		void init( HDC hDC )
		{
			GIFLoader gifLoader;

			baseTex.load( hDC , "Bomberman/playerBase.bmp" );
			baseTex.setTransColor( Color3ub( 3 , 227 , 19 ) );

			itemTex.load( hDC , "Bomberman/item.bmp");
			itemTex.setTransColor( Color3ub( 0 , 255 , 0 ) );

			levelTex.load( hDC , "Bomberman/lvClassical.bmp" );
			levelTex.setTransColor( Color3ub( 0 , 255 , 0 ) );


			{
				SpriteAnim& sprAnim = playerSpr;
				Vec2i frameSize( 20 , 30 );
				int offset = 3 * frameSize.x;
				Vec2i walkPos[] = { 
					Vec2i( 3 * offset , 0 ),Vec2i( 1 * offset , 0 ), 
					Vec2i( 0 * offset , 0 ),Vec2i( 2 * offset , 0 ) 
				};
				sprAnim.setTexture( baseTex );
				sprAnim.addAnimGroup( ANIM_STAND , 1 , frameSize , walkPos , 4 );
				sprAnim.addAnimGroup( ANIM_WALK  , 3 , frameSize , walkPos , 4 , AF_RETRACE );
				sprAnim.addAnim     ( ANIM_DIE   , 7 , frameSize , Vec2i( 0 , 1 * frameSize.y ) , AF_ONCE | AF_HIDE );
			}
			{
				Vec2i frameSize( 20 , 20 );
				Vec2i posOffset( 0 , 60 );
				int offset = 4 * frameSize.x;
				Vec2i cPos [] = 
				{
					posOffset + Vec2i( 1 * offset ,1 * frameSize.y ),
					posOffset + Vec2i( 0 * offset ,1 * frameSize.y ),
					posOffset + Vec2i( 1 * offset ,0 * frameSize.y ),
					posOffset + Vec2i( 0 * offset ,0 * frameSize.y ),
				};

				bombSpr.setTexture( baseTex );
				bombSpr.addAnim   ( ANIM_BOMB_FIRE_C , 4 , frameSize , posOffset + Vec2i( 0 * offset ,2 * frameSize.y ) , AF_RETRACE | AF_HIDE );
				bombSpr.addAnim   ( ANIM_BOMB_FIRE_H , 4 , frameSize , posOffset + Vec2i( 2 * offset ,0 * frameSize.y )  , AF_RETRACE | AF_HIDE );
				bombSpr.addAnim   ( ANIM_BOMB_FIRE_V , 4 , frameSize , posOffset + Vec2i( 2 * offset ,1 * frameSize.y ) , AF_RETRACE | AF_HIDE );
				bombSpr.addAnim   ( ANIM_BOMB_HOLD   , 3 , frameSize , posOffset + Vec2i( 1 * offset ,2 * frameSize.y ) , AF_RETRACE );
				bombSpr.addAnim   ( ANIM_BOMB_FIRE   , 4 , frameSize , posOffset + Vec2i( 1 * offset ,2 * frameSize.y ) , AF_ONCE );
				bombSpr.addAnimGroup( ANIM_BOMB_FIRE_SIDE , 4 , frameSize , cPos , 4 , AF_RETRACE | AF_HIDE );
			}
			{

				Vec2i frameSize( 16 , 16 );

				Vec2i const standPos[] =
				{
					Vec2i( 0 * frameSize.x ,0 * frameSize.y ) ,
					Vec2i( 1 * frameSize.x ,0 * frameSize.y ) ,
					Vec2i( 2 * frameSize.x ,0 * frameSize.y ) ,
					Vec2i( 3 * frameSize.x ,0 * frameSize.y ) ,
					Vec2i( 4 * frameSize.x ,0 * frameSize.y ) ,
					Vec2i( 5 * frameSize.x ,0 * frameSize.y ) ,
					Vec2i( 0 * frameSize.x ,1 * frameSize.y ) ,
					Vec2i( 1 * frameSize.x ,1 * frameSize.y ) ,
					Vec2i( 2 * frameSize.x ,1 * frameSize.y ) ,
					Vec2i( 3 * frameSize.x ,1 * frameSize.y ) ,
					Vec2i( 4 * frameSize.x ,1 * frameSize.y ) ,
					Vec2i( 5 * frameSize.x ,1 * frameSize.y )  ,
				};

				itemSpr.setTexture( itemTex );

				int idx = 0;
				gItemAnimMap[ ITEM_BOMB_PASS ] = idx++;
				gItemAnimMap[ ITEM_HEART ] = idx++;
				gItemAnimMap[ ITEM_SPEED ] = idx++;
				gItemAnimMap[ ITEM_REMOTE_CTRL ] = idx++;
				gItemAnimMap[ ITEM_BOMB_UP ] = idx++;
				gItemAnimMap[ ITEM_POWER_GLOVE ] = idx++;
				gItemAnimMap[ ITEM_WALL_PASS ] = idx++;
				gItemAnimMap[ ITEM_BOMB_KICK ] = idx++;
				gItemAnimMap[ ITEM_BOMB_PASS ] = idx++;
				gItemAnimMap[ ITEM_SKULL   ] = idx++;
				gItemAnimMap[ ITEM_BOXING_GLOVE ] = idx++;
				gItemAnimMap[ ITEM_POWER ] = idx++;
				gItemAnimMap[ ITEM_FULL_POWER ] = idx++;

				itemSpr.addAnimGroup( ANIM_ITEM_STAND , 1 , frameSize , standPos , ARRAY_SIZE( standPos ),  AF_CYCLE );
				itemSpr.addAnim     ( ANIM_ITEM_BLINK , 2 , frameSize , Vec2i( 0 * frameSize.x ,2 * frameSize.y ) , AF_CYCLE );
			}

			{

				Vec2i frameSize( 16 , 16 );
				levelSpr.setTexture( levelTex );
				levelSpr.addAnim( ANIM_TILE_DIRT         , 1 , frameSize , Vec2i( 0 * frameSize.x ,0 * frameSize.y ) , AF_CYCLE );
				levelSpr.addAnim( ANIM_TILE_WALL_STAND   , 1 , frameSize , Vec2i( 1 * frameSize.x ,0 * frameSize.y ) , AF_CYCLE );
				levelSpr.addAnim( ANIM_TILE_BLOCK_STAND  , 1 , frameSize , Vec2i( 2 * frameSize.x ,0 * frameSize.y ) , AF_CYCLE );
				levelSpr.addAnim( ANIM_TILE_WALL_BURNING , 6 , frameSize , Vec2i( 0 * frameSize.x ,1 * frameSize.y ) , AF_ONCE | AF_HIDE );
				Vec2i cPos[] = 
				{
					Vec2i( 0 * frameSize.x ,2 * frameSize.y ),
					Vec2i( 1 * frameSize.x ,2 * frameSize.y ),
					Vec2i( 2 * frameSize.x ,2 * frameSize.y ),
					Vec2i( 3 * frameSize.x ,2 * frameSize.y ),
					Vec2i( 4 * frameSize.x ,2 * frameSize.y ),
					Vec2i( 5 * frameSize.x ,2 * frameSize.y ),
				};
				levelSpr.addAnimGroup( ANIM_TILE_RAIL , 1 , frameSize  , cPos , 6 , AF_CYCLE );

				Vec2i cPos2[] = 
				{
					Vec2i( 0 * frameSize.x ,3 * frameSize.y ),
					Vec2i( 1 * frameSize.x ,3 * frameSize.y ),
					Vec2i( 2 * frameSize.x ,3 * frameSize.y ),
					Vec2i( 3 * frameSize.x ,3 * frameSize.y ),
				};
				levelSpr.addAnimGroup( ANIM_TILE_ARROW , 1 , frameSize  , cPos2 , 4 , AF_CYCLE );

				Vec2i cPos3[] = 
				{
					Vec2i( 4 * frameSize.x ,3 * frameSize.y ),
					Vec2i( 5 * frameSize.x ,3 * frameSize.y ),
					Vec2i( 6 * frameSize.x ,3 * frameSize.y ),
					Vec2i( 7 * frameSize.x ,3 * frameSize.y ),
				};
				levelSpr.addAnimGroup( ANIM_TILE_ARROW_CHANGE , 1 , frameSize  , cPos3 , 4 , AF_CYCLE );
			}
		}

		SpriteAnim& getPlayerSpriteAnim( int type ){ return playerSpr; }

		int        refCount;

		Texture    baseTex;
		Texture    itemTex;
		Texture    levelTex;
		SpriteAnim playerSpr;
		SpriteAnim bombSpr;
		SpriteAnim itemSpr;
		SpriteAnim levelSpr;
	};

	ResManager* gResManager = NULL;



	Scene::Scene()
	{
		//mDrawDebug = false;

		if ( gResManager == NULL )
		{
			gResManager = new ResManager;
			gResManager->init(  NULL );
			gResManager->refCount += 1;
		}
		mCurFrame = 0;
		mViewPos.setValue( 0 ,0 );

		for( int i = 0 ; i < 8 ; ++i )
		{
			RenderData& data = mPlayerData[i];
			data.action = ANIM_STAND;
			data.frame  = 0;
		}

		getWorld().setAnimManager( this );
	}

	void Scene::render( Graphics2D& g )
	{
		Vec2i size = getWorld().getMapSize();

		Vec2i tilePos;
		for( tilePos.x = 0 ; tilePos.x < size.x ; ++tilePos.x )
		{
			for( tilePos.y = 0 ; tilePos.y < size.y ; ++tilePos.y )
			{
				TileData& tile = getWorld().getTile( tilePos );
				Vec2i pos = tilePos * TileLength - mViewPos;
				drawTile( g , pos, tilePos , tile );
			}
		}

		for( World::BombIterator iter = getWorld().getBombs() ; 
			 iter.haveMore() ; iter.goNext() )
		{
			Bomb& bomb = iter.getBomb();

			Vec2i renderPos = bomb.pos * TileLength + TileSize / 2 - mViewPos;
			if ( bomb.state >= 0 )
			{
				drawBlast( g , renderPos , bomb );
			}
			else if ( bomb.state == Bomb::eONTILE )
			{
				drawBomb( g , renderPos , bomb );
			}
		}


		struct RenderFun
		{
			RenderFun( Scene& s , Graphics2D& g ):scene(s),g(g){}
			Scene&      scene;
			Graphics2D& g;
			void operator()( Entity* obj )
			{
				scene.drawEntity( g , *obj );
			}
		};
		std::for_each( mSortedEntites.begin() , mSortedEntites.end() , RenderFun( *this , g ) );

		static int frame = 0;
		++frame;
		gResManager->levelSpr.render( g , Vec2i(0,0) , ANIM_TILE_WALL_BURNING , frame / 32 );


	}

	void Scene::drawTile( Graphics2D& g , Vec2i const& pos , Vec2i const& tPos , TileData const& tile )
	{

		SpriteAnim& lvSpr = gResManager->levelSpr;
		switch( tile.terrain )
		{
		case MT_DIRT:
			lvSpr.render( g , pos , ANIM_TILE_DIRT , mCurFrame );
			break;
		case MT_RAIL + 0:  case MT_RAIL + 1: case MT_RAIL + 2: 
		case MT_RAIL + 3:  case MT_RAIL + 4: case MT_RAIL + 5:
			lvSpr.render( g , pos , ANIM_TILE_DIRT , mCurFrame );
			lvSpr.render( g , pos ,  ANIM_TILE_RAIL , mCurFrame , tile.terrain - MT_RAIL );
			break;
		case MT_ARROW + 0:case MT_ARROW + 1:case MT_ARROW + 2:case MT_ARROW + 3:
			lvSpr.render( g , pos , ANIM_TILE_DIRT , mCurFrame );
			lvSpr.render( g , pos , ANIM_TILE_ARROW , mCurFrame , tile.terrain - MT_ARROW );
			break;
		case MT_ARROW_CHANGE + 0:case MT_ARROW_CHANGE + 1:case MT_ARROW_CHANGE + 2:case MT_ARROW_CHANGE + 3:
			lvSpr.render( g , pos , ANIM_TILE_DIRT , mCurFrame );
			lvSpr.render( g , pos , ANIM_TILE_ARROW_CHANGE , mCurFrame , tile.terrain - MT_ARROW_CHANGE );
			break;
		}

		TileAnimData& data = mTileAnim.getData( tPos.x , tPos.y );

		if  ( data.animId != -1 )
		{
			int frame = mCurFrame - data.frame;
			switch( data.animId )
			{
			case ANIM_WALL_BURNING:
				frame /= 8; 
				if ( frame >= lvSpr.getAnimFrameLength( ANIM_TILE_WALL_BURNING ) )
				{
					data.animId = -1;
					data.frame  = 0;
				}
				else
				{
					lvSpr.render( g , pos , ANIM_TILE_WALL_BURNING , frame );
				}
				break;
			}
		}

		switch( tile.obj )
		{
		case MO_WALL:
			if ( mDrawDebug )
			{
				RenderUtility::SetPen( g , Color::eBlack );
				RenderUtility::SetBrush( g , Color::eYellow );
				g.drawRect( pos , TileSize );
			}
			lvSpr.render( g , pos , ANIM_TILE_WALL_STAND , mCurFrame );
			break;
		case MO_BOMB:
#if 0
			{
				Bomb const& bomb = getLevel().getBomb( tile.meta );
				if ( bomb.state >= 0 )
				{
					drawFire( g , pos + TileSize / 2, bomb );

				}
				else
				{
					drawBomb( g , pos + TileSize / 2 , bomb );
				}
			}
#endif
			break;
		case MO_BLOCK:
			if ( mDrawDebug )
			{
				RenderUtility::SetBrush( g , Color::eRed );
				g.drawRoundRect( pos , TileSize , Vec2i( 3 , 3 ) );
			}

			lvSpr.render( g , pos , ANIM_TILE_BLOCK_STAND , mCurFrame );

			break;
		case MO_ITEM:
			if ( data.animId == -1 )
			{
				if ( mDrawDebug )
				{
					int gap = 1;
					int color = Color::eBlue;
					char const* text = "";
					switch( tile.meta )
					{
					case ITEM_REMOTE_CTRL:   text = "C"; color = Color::eYellow; break;
					case ITEM_LIQUID_BOMB:   text = "L"; color = Color::eCyan; break;
					case ITEM_BOMB_KICK:     text = "K"; color = Color::eOrange; break;
					case ITEM_POWER_GLOVE:   text = "G"; color = Color::eBlue; break;
					case ITEM_SPEED:         text = "S"; color = Color::eGreen; break;
					case ITEM_POWER:         text = "P"; color = Color::ePurple; break;
					case ITEM_BOMB_UP:       text = "B"; color = Color::eRed; break;

					default:
						text = "?"; 
					}
					RenderUtility::SetPen( g , Color::eBlack );
					RenderUtility::SetBrush( g , color );
					g.drawRoundRect( pos + Vec2i( gap , gap ) , TileSize - 2 * Vec2i( gap , gap ) , Vec2i( 5 , 5 ) );

					g.setTextColor( 0 , 0 , 0 );
					g.drawText( pos , TileSize , text );
				}

				SpriteAnim& sprAnim = gResManager->itemSpr;
				if ( sprAnim.haveAnim( tile.meta ) )
				{
					Vec2i renderPos = pos;
					sprAnim.render( g , renderPos , ANIM_ITEM_BLINK , mCurFrame / 8 );
					sprAnim.render( g , renderPos , ANIM_ITEM_STAND , 0 , gItemAnimMap[ tile.meta ] );
				}
			}
			break;
		}
	}

	void Scene::drawEntity( Graphics2D& g , Entity& entity )
	{
		Vec2i renderPos = Vec2i( TileLength * entity.getPos() ) - mViewPos;

		switch( entity.getType() )
		{
		case OT_BOMB:
			{
				BombObject& bombObj = static_cast< BombObject& >( entity );
				drawBomb( g , renderPos , bombObj.getData() );
			}
			break;
		case OT_PLAYER:
			{
				Player& player = static_cast< Player& >( entity );
				drawPlayer( g , renderPos , player );
			}
			break;
		case OT_MONSTER:

			break;
		case OT_MINE_CAR:
			if ( mDrawDebug )
			{
				MineCar& car = static_cast< MineCar& >( entity );
				RenderUtility::SetBrush( g , Color::eYellow );
				g.drawRoundRect( renderPos - TileSize / 2 , TileSize , Vec2i( 3 , 3 ) );
			}
			break;
		}
	}


	void Scene::drawBomb( Graphics2D& g , Vec2i const& pos , Bomb const& bomb )
	{
		if ( mDrawDebug )
		{
			RenderUtility::SetBrush( g , Color::eCyan );
			g.drawCircle( pos , TileLength / 2 - 2 );
		}
		SpriteAnim& spr = gResManager->bombSpr;

		Vec2i offset( -10 , -10 );
		spr.render( g , pos + offset , ANIM_BOMB_HOLD , 16 * ( 1.0f - float( bomb.timer ) / gBombFireFrame )  );
	}


	void Scene::drawPlayer( Graphics2D& g , Vec2i const& pos , Player& player )
	{

		if ( mDrawDebug )
		{
			bool bTakeBomb = player.getTakeBombIndex() != -1 ;
			RenderUtility::SetBrush( g , Color::eBlue );
			g.drawRect( pos - TileSize / 2 , TileSize );
			RenderUtility::SetPen( g , Color::eYellow );
			g.drawLine( pos , pos + ( TileLength / 2 ) * getDirOffset( player.getFaceDir() ) );
			RenderUtility::SetPen( g , Color::eBlack );
			if (  bTakeBomb )
			{
				drawBomb( g , pos - Vec2i( 0 , 5 ) , getWorld().getBomb( player.getTakeBombIndex() ));
			}

			if ( player.getDisease() != SKULL_NULL )
			{





			}
		}

		if ( !player.isVisible() )
			return;

		SpriteAnim& sprAnim = gResManager->getPlayerSpriteAnim( 0 );
		RenderData& renderData = mPlayerData[ player.getId() ];

		Vec2i offset( -10 , -23 );
		switch( renderData.action )
		{
		case ANIM_STAND:
			sprAnim.render( g , pos + offset  , ANIM_WALK , 1 , player.getFaceDir() );
			break;
		case ANIM_DIE:
			sprAnim.render( g , pos + offset , renderData.action , ( mCurFrame - renderData.frame ) / 8 );
			break;
		default:
			sprAnim.render( g , pos + offset , renderData.action , ( mCurFrame - renderData.frame ) / 8 , player.getFaceDir() );
			break;
		}
		
	}

	void Scene::drawBlast( Graphics2D& g , Vec2i const& pos , Bomb const& bomb )
	{
		Vec2i blastOffset( -10 , -10 );

		if ( bomb.flag & BF_DANGEROUS )
		{
			if ( mDrawDebug )
			{

			}
			
			if ( bomb.state > 0 )
			{
				SpriteAnim& spr = gResManager->bombSpr;


				int timeA     = gFireMoveTime * ( bomb.power - 1 );
				int time;
				if ( bomb.state == bomb.power )
					time = timeA + bomb.timer * gFireMoveSpeed;
				else
					time = ( bomb.state - 1 ) * gFireMoveTime + bomb.timer;
				int totalTime = timeA + gFireHoldFrame * gFireMoveSpeed;
				int frame = time * spr.getAnimFrameLength( ANIM_BOMB_FIRE_C ) / ( totalTime );

				Vec2i renderPos = pos + blastOffset;
				
				for( int i = -bomb.state ; i <= bomb.state ; ++i )
				for( int j = -bomb.state ; j <= bomb.state ; ++j )
				{
					Vec2i tPos = bomb.pos + Vec2i( i , j );

					if ( !mTileAnim.checkRange( tPos.x , tPos.y ) )
						continue;

					TileAnimData& animData = mTileAnim.getData( tPos.x , tPos.y );

					if ( animData.animId != -1 )
						continue;

					if ( getWorld().getTile( tPos ).obj )
						continue;

					spr.render( g , renderPos + TileLength * Vec2i( i , j ) , ANIM_BOMB_FIRE_C , frame );
				}
			}

		}
		else
		{
			int const fireCap = 4;

			if ( mDrawDebug )
			{
				Vec2i renderPos = pos - TileSize / 2;
				RenderUtility::SetBrush( g , Color::eRed );
				RenderUtility::SetPen( g , Color::eNull );

				g.drawRect( renderPos + Vec2i( 0 , fireCap ) , TileSize - Vec2i( 0 , 2 * fireCap ) );
				g.drawRect( renderPos + Vec2i( fireCap , 0 ) , TileSize - Vec2i( 2 * fireCap , 0 ) );

				if ( bomb.state > 0 )
				{
					for( int dir = 0 ; dir < 4 ; ++dir )
					{
						int   fireLen = bomb.getFireLength( Dir(dir) );
						Vec2i offset = TileLength * getDirOffset( dir );
						Vec2i firePos = renderPos;
						for( int i = 0 ; i < fireLen ; ++i )
						{

							firePos += offset;

							switch( dir )
							{
							case DIR_EAST:case DIR_WEST:
								g.drawRect( firePos + Vec2i( 0 , fireCap ) , TileSize - Vec2i( 0 , 2 * fireCap ) );
								break;
							case DIR_SOUTH:case DIR_NORTH:
								g.drawRect( firePos + Vec2i( fireCap , 0 ) , TileSize - Vec2i( 2 * fireCap , 0 ) );
								break;
							}
						}
					}
				}

				RenderUtility::SetPen( g , Color::eBlack );
			}
			SpriteAnim& spr = gResManager->bombSpr;
			

			if ( bomb.state > 0 )
			{
				int timeA     = gFireMoveTime * ( bomb.power - 1 );
				int time;
				if ( bomb.state == bomb.power )
					time = timeA + bomb.timer * gFireMoveSpeed;
				else
					time = ( bomb.state - 1 ) * gFireMoveTime + bomb.timer;
				int totalTime = timeA + gFireHoldFrame * gFireMoveSpeed;
				int frame = time * spr.getAnimFrameLength( ANIM_BOMB_FIRE_C ) / ( totalTime );

				spr.render( g , pos + blastOffset , ANIM_BOMB_FIRE_C , frame );

				for( int dir = 0 ; dir < 4 ; ++dir )
				{
					int   fireLen = bomb.getFireLength( Dir(dir) );
					Vec2i fireOffset = TileLength * getDirOffset( dir );
					Vec2i firePos = pos + blastOffset + fireOffset;
					for( int i = 0 ; i < fireLen - 1; ++i )
					{
						spr.render( g, firePos , isVertical( Dir(dir) ) ? ANIM_BOMB_FIRE_V : ANIM_BOMB_FIRE_H , frame );
						firePos += fireOffset;
					}
					if ( fireLen > 0 )
					{
						if ( bomb.fireLength[ dir ] == -1 )
							spr.render( g , firePos , ANIM_BOMB_FIRE_SIDE , frame , dir );
						else
							spr.render( g, firePos , isVertical( Dir(dir) ) ? ANIM_BOMB_FIRE_V : ANIM_BOMB_FIRE_H , frame );
					}

				}
			}
			else
			{
				int frame = bomb.timer * spr.getAnimFrameLength( ANIM_BOMB_FIRE ) / ( gFireMoveSpeed * gFireMoveTime );
				spr.render( g , pos + blastOffset , ANIM_BOMB_FIRE , frame );
			}
		}
	}

	void Scene::updateFrame( int frame )
	{
		mCurFrame += frame;

		struct VisitFun
		{
			VisitFun( EntityList& el ):entities( el ){}
			void operator()( Entity* e )
			{
				entities.push_back( e );
			}
			EntityList& entities;
		};

		if ( getWorld().checkEntitiesModified() )
		{
			mSortedEntites.clear();
			getWorld().visitEntities( VisitFun( mSortedEntites ) );
		}

		struct SortFun
		{
			bool operator()( Entity* e1 , Entity* e2 )
			{
				return e1->getPos().y < e2->getPos().y;
			}
		};
		std::sort( mSortedEntites.begin() , mSortedEntites.end() , SortFun() );
	}

	void Scene::setPlayerAnim( Player& player , AnimAction action )
	{
		RenderData& data = mPlayerData[ player.getId() ];
		if ( data.action != action )
		{
			data.action = action;
			if ( data.action == ANIM_VICTORY )
				data.action = ANIM_STAND;
			data.frame  = mCurFrame;
		}
	}

	void Scene::restartLevel( bool beInit )
	{
		if ( beInit )
		{
			Vec2i mapSize = getWorld().getMapSize();
			mTileAnim.resize( mapSize.x , mapSize.y );
		}
		mTileAnim.fillValue( TileAnimData() );
	}

	void Scene::setTileAnim( Vec2i const& pos , AnimAction action )
	{
		TileAnimData& data = mTileAnim.getData( pos.x , pos.y );
		data.animId = action;
		data.frame  = mCurFrame;
	}

}//namespace BomberMan