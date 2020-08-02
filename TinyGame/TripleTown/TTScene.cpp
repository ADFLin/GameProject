#include "TinyGamePCH.h"
#include "TTScene.h"

#include "RenderUtility.h"

#include "EasingFunction.h"

#include "Core/IntegerType.h"
#include <fstream>

#include "TextureId.h"
#include "CppVersion.h"

#include "RHI/RHIGlobalResource.h"
#include "RHI/DrawUtility.h"

#define USE_TEXTURE_ATLAS 1
#define USE_COMPACT_IMAGE 0

bool GUseBatchedRender = true;

namespace TripleTown
{
	float AnimTime = 0.3f;
	int const GameTilesTexLength = 512;
	int const TileImageLength = 112;
	int const ItemOutLineLength = 112;

	char const* gResourceDir = "TripleTown";

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

	Color3ub const MASK_KEY( 255 , 255 , 255 );
	struct ItemImageLoadInfo
	{
		int id;
		int addition;
		char const* fileName;	
	};
	int GetItemImageId(int id , EItemImage::Type type) { return TEX_ID_NUM + id * EItemImage::Count + type; }
	int GetTexImageId(int id){ return id; }

	
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


	struct TexImageData
	{
		uint32 width;
		uint32 height;
		std::vector< uint32 > data;

		bool load(char const* path)
		{
			using std::ios;
			std::ifstream fs(path, ios::binary);
			if( !fs.is_open() )
				return false;

			fs.seekg(0, ios::beg);
			ios::pos_type pos = fs.tellg();

			fs.seekg(0, ios::end);
			int sizeFile = fs.tellg() - pos;

			fs.seekg(0, ios::beg);

			TexHeader header;
			fs.read((char*)&header, sizeof(header));

			width = header.width;
			height = header.height;

			data.resize(width * height);
			switch( header.colorBits )
			{
			case 4:
				
				fs.read((char*)&data[0], 4 * data.size() );
				break;
			case 3:
				{
					auto* pData = (uint8*)&data[0];
					for( int i = 0; i < width * height; ++i )
					{
						fs.read((char*)pData, sizeof(uint8) * 3);
						pData[3] = 255;
						pData += 4;
					}

				}
				break;
			case 2:
				{
					auto* pData = (uint8*)&data[0];
					Color16 c;
					assert(sizeof(c) == 2);
					for( int i = 0; i < width * height; ++i )
					{
						fs.read((char*)&c, sizeof(c));

						pData[0] = uint8(unsigned(255 * c.r) / 16);
						pData[1] = uint8(unsigned(255 * c.g) / 16);
						pData[2] = uint8(unsigned(255 * c.b) / 16);
						pData[3] = uint8(unsigned(255 * c.a) / 16);

						pData += 4;
					}
				}
				break;
			case 1:
				{
					auto* pData = (uint8*)&data[0];
					uint8 c;
					for( int i = 0; i < width * height; ++i )
					{
						fs.read((char*)&c, sizeof(c));
						pData[0] =
						pData[1] = 
						pData[2] =
						pData[3] = c;
						pData += 4;
					}
				}
				break;
			default:
				LogWarning(0, "Can't load image %s", path);
				return false;
			}

			return true;
		}

		int getCompactOffset(int startPos, int offsetStride, int offsetSize, int checkStride, int checkSize)
		{
			uint32* pStart = data.data() + startPos;
			int result = 0;
			for( int i = 0; i < offsetSize; ++i )
			{
				uint32* pCur = pStart + i * offsetStride;
				for( int n = 0; n < checkSize; ++n )
				{
					if( *pCur & 0xff000000 )
						return result;

					pCur += checkStride;
				}
				++result;
			}
			return result;
		}
		void calcCompactSize(Vec2i& compactMin, Vec2i& compactMax)
		{
			compactMin.x = getCompactOffset(0, 1, width, width, height);
			compactMax.x = width - getCompactOffset(width - 1, -1, width - compactMin.x, width, height);
			compactMin.y = getCompactOffset(compactMin.x, width, height, 1, compactMax.x - compactMin.x);
			compactMax.y = height - getCompactOffset(compactMin.x + (height - 1) * width, -width, height - compactMin.y, 1, compactMax.x - compactMin.x);
		}
	};


	class BatchedDrawer
	{




	};

	ItemImageLoadInfo gItemImageList[] =
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
		mLevel = nullptr;
		mMapOffset = Vec2i( 0 , 0 );
		mNumPosRemove = 0;
		mPosPeek = Vec2i( -1 , -1 );
	}

	Scene::~Scene()
	{
		if( mLevel )
			mLevel->setListener(nullptr);

		releaseResource();
	}

	void Scene::releaseResource()
	{
		mFontTextures[0].release();
		mFontTextures[1].release();

		mTexAtlas.finalize();

#if !USE_TEXTURE_ATLAS
		for (int i = 0; i < NUM_OBJ; ++i)
		{
			for (int j = 0; j < EItemImage::Count; ++j)
			{
				mItemImageMap[i][j].texture.release();
			}
		}
#endif

		for (auto& texPtr : mTexMap)
		{
			texPtr.release();
		}
	}

	float value;
	bool Scene::init()
	{
		mItemScale = 0.8f;
		mMouseAnim = nullptr;
		mTweener.tweenValue< Easing::CIOQuad >(mItemScale, 0.65f, 0.8f, 1).cycle();
		return true;
	}

	bool Scene::loadResource()
	{
		TRACE_RESOURCE_TAG_SCOPE("TTScene");
		FixString< 256 > path;
		unsigned w, h;

		TRACE_RESOURCE_TAG("TTScene.TexAtlas");
		VERIFY_RETURN_FALSE(mTexAtlas.initialize(Texture::eRGBA8, 2048, 2048 , 1 ));

		for( int i = 0; i < TEX_ID_NUM; ++i )
		{
			path.format("%s/%s.tex", gResourceDir, gTextureName[i]);
			VERIFY_RETURN_FALSE(loadTextureImageResource(path, i));
		}

		for( int i = 0 ; i < ARRAY_SIZE( gItemImageList ) ; ++i )
		{
			ItemImageLoadInfo& info = gItemImageList[i];
			VERIFY_RETURN_FALSE(loadItemImageResource(info));
		}

		LogMsg("Atlas = %f", mTexAtlas.calcUsageAreaRatio());
		return true;
	}


	RHITexture2DRef LoadTexture(char const* path)
	{
		TexImageData imageData;
		if( !imageData.load(path) )
			return nullptr;

		return RHICreateTexture2D(Texture::eRGBA8, imageData.width, imageData.height, 0, 1, TCF_DefalutValue, imageData.data.data());
	}

	bool Scene::loadTextureImageResource( char const* path , int textureId )
	{
		TexImageData imageData;
		if( !imageData.load(path) )
			return false;

		mTexMap[textureId] = RHICreateTexture2D(Texture::eRGBA8, imageData.width, imageData.height, 0, 1, TCF_DefalutValue, imageData.data.data());

		if( !mTexAtlas.addImage(textureId , imageData.width, imageData.height, Texture::eRGBA8, imageData.data.data()) )
			return false;

		return true;
	}

	bool Scene::loadItemImageResource(ItemImageLoadInfo& info)
	{
		FixString< 256 > path;
		path.format("%s/item_%s.tex", gResourceDir, info.fileName);

		VERIFY_RETURN_FALSE(loadImageTexFile(path, info.id, EItemImage::eNormal));
		path.format("%s/item_%s_outline.tex", gResourceDir, info.fileName);
		VERIFY_RETURN_FALSE(loadImageTexFile(path, info.id, EItemImage::eOutline));

		if( info.addition )
		{
			path.format("%s/item_%s2.tex", gResourceDir, info.fileName);
			VERIFY_RETURN_FALSE(loadImageTexFile(path, info.id, EItemImage::eAddition));

			path.format("%s/item_%s2_outline.tex", gResourceDir, info.fileName);
			VERIFY_FAILCODE(loadImageTexFile(path, info.id, EItemImage::eAdditionOutline), LogMsg("Path = %s", path.c_str()); );
		}

		return true;

	}

	bool Scene::loadImageTexFile(char const* path, int itemId, EItemImage::Type imageType)
	{
		TexImageData imageData;
		if( !imageData.load(path) )
			return false;
		TRACE_RESOURCE_TAG_SCOPE("TTScene");
		auto& imageInfo = mItemImageMap[itemId][imageType];
#if !USE_TEXTURE_ATLAS
		imageInfo.texture = RHICreateTexture2D(Texture::eRGBA8, imageData.width, imageData.height, 0, 1, TCF_DefalutValue, imageData.data.data());
#endif

#if USE_COMPACT_IMAGE
		Vec2i compactMin, compactMax;
		imageData.calcCompactSize(compactMin, compactMax);
		Vec2i compactSize = compactMax - compactMin;
		if( !mTexAtlas.addImage(GetItemImageId(itemId, imageType), compactSize.x, compactSize.y, Texture::eRGBA8, imageData.data.data() + compactMin.x + compactMin.y * imageData.width, imageData.width) )
			return false;

		imageInfo.rect.offset = Vector2(compactMin) / Vector2(imageData.width, imageData.height);
		imageInfo.rect.size = Vector2(compactSize) / Vector2(imageData.width, imageData.height);
#else

		if( !mTexAtlas.addImage(GetItemImageId(itemId, imageType), imageData.width, imageData.height, Texture::eRGBA8, imageData.data.data()) )
			return false;
#endif

		return true;
	}


	void Scene::setupLevel( Level& level )
	{
		mLevel = &level;
		mLevel->setListener( this );

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
						RenderUtility::SetBrush( g , EColor::Red );
						RenderUtility::SetPen( g , EColor::Black );
						g.drawCircle( pos + TileSize / 2 , 10 );
						break;
					case OBJ_BEAR:
						RenderUtility::SetBrush( g , EColor::Yellow );
						RenderUtility::SetPen( g , EColor::Black );
						g.drawCircle( pos + TileSize / 2 , 10 );
						break;
					}
				}
				else if ( Level::GetInfo( tile.id ).typeClass->getType() == OT_BASIC )
				{
					FixString< 32 > str;
					str.format( "%d" , mLevel->getLinkNum( TilePos( i , j ) ) );
					g.setTextColor(Color3ub(100 , 0 , 255) );
					g.drawText( pos , str );
				}
			}
		}

		RenderUtility::SetPen( g , EColor::Red );
		RenderUtility::SetBrush( g , EColor::Null );
		for( int i = 0 ; i < mNumPosRemove ; ++i )
		{
			Vec2i pos = mMapOffset + TileLength * Vec2i( mRemovePos[i].x , mRemovePos[i].y );
			g.drawRect( pos , TileSize );
		}
	}

	void Scene::renderTile( Graphics2D& g , Vec2i const& pos , ObjectId id , int meta /*= 0 */ )
	{
		RenderUtility::SetPen( g , EColor::Null );

		switch( id )
		{
		case OBJ_NULL:
			RenderUtility::SetBrush( g , EColor::Yellow , COLOR_DEEP );
			g.drawRect( pos , TileSize );
			break;
		case OBJ_GRASS:
			{
				Vec2i const border = Vec2i( 4 , 4 );
				RenderUtility::SetBrush( g , EColor::Green , COLOR_LIGHT );
				g.drawRect( pos , TileSize );
				RenderUtility::SetBrush( g , EColor::Green );
				g.drawRect( pos + border , TileSize - 2 * border );
			}
			break;
		case OBJ_BUSH:
			{
				RenderUtility::SetBrush( g , EColor::Green , COLOR_LIGHT );
				g.drawRect( pos , TileSize );
				RenderUtility::SetBrush( g , EColor::Green );
				g.drawCircle( pos + TileSize / 2 , TileLength / 2 - 10 );
			}
			break;
		case OBJ_TREE:
			{
				Vec2i bSize( 10 , 20 );
				RenderUtility::SetBrush( g , EColor::Green , COLOR_LIGHT );
				g.drawRect( pos , TileSize );
				RenderUtility::SetBrush( g , EColor::Cyan );
				g.drawRect( pos + TileSize / 2 - bSize / 2 , bSize );
				RenderUtility::SetBrush( g , EColor::Green );
				g.drawCircle( pos + TileSize / 2 , TileLength / 2 - 12 );
			}
			break;
		case OBJ_STOREHOUSE:
			{
				RenderUtility::SetBrush( g , EColor::Red );
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
				RenderUtility::SetBrush( g , EColor::Red );
				g.drawRect( pos + TileSize / 2 - bSize / 2 , bSize );
				FixString< 32 > str;
				str.format( "%d" , id );
				g.drawText( pos , TileSize , str );
			}
		}
	}



	void Scene::click( Vec2i const& pos )
	{
		TilePos tPos = calcTilePos( pos );

		if ( !mLevel->isMapRange( tPos ) )
			return;
		mLevel->clickTile( tPos );
	}

	ObjectId Scene::getObjectId(Vec2i const& pos)
	{
		TilePos tPos = calcTilePos(pos);
		if( !mLevel->isMapRange(tPos) )
			return OBJ_NULL;
		return mLevel->getObjectId(mLevel->getTile(tPos));
	}
	void Scene::peekObject( Vec2i const& pos )
	{
		TilePos tPos = calcTilePos( pos );

		if ( mPosPeek == tPos )
			return;

		if ( !mLevel->isMapRange( tPos ) )
			return;

		if ( mMouseAnim )
		{
			mTweener.remove( mMouseAnim );
			mMouseAnim = nullptr;
			for( int i = 0 ; i < mNumPosRemove ; ++i )
			{
				TilePos& pos = mRemovePos[i];
				TileData& tile = mMap.getData( pos.x , pos.y );
				tile.pos = float( TileImageLength ) * Vector2( pos );
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
				Vector2 from = float( TileImageLength ) * Vector2( pos );
				Vector2 offset = Vector2( mPosPeek - pos );
				offset *= ( 0.15f * TileImageLength );
				Vector2 to = from + offset;
				tween.addValue< Easing::CLinear >( tile.pos , from , to );
			}
			mMouseAnim = &tween;
		}
	}

	void Scene::render()
	{
		TransformStack2D& xformStack = mRenderState.xformStack;

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		float scaleItem = 0.8f;

		Vec2i screenSize = ::Global::GetScreenSize();
		mRenderState.baseTransform = AdjProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0, -100, 100));
		mRenderState.sampler = &TStaticSamplerState< Sampler::eBilinear >::GetRHI();
		float scaleMap = float( TileLength ) / TileImageLength;
	
		TRANSFORM_STACK_SCOPE(xformStack, RenderTransform2D( Vector2(scaleMap , scaleMap ) , mMapOffset) );

		TileMap const& map = mLevel->getMap();


#if USE_TEXTURE_ATLAS
		mRenderState.texture = &mTexAtlas.getTexture();
#else
		mRenderState.texture = mTexMap[TID_GAME_TILES];
#endif

		for( int j = 0 ; j < map.getSizeY() ; ++j )
		{
			for( int i = 0 ; i < map.getSizeX() ; ++i )
			{
				Vec2i pos = mMapOffset + TileLength * Vec2i( i , j );
				Tile const& tile = map.getData( i , j );

				//depth offset = -1
				TRANSFORM_STACK_SCOPE(xformStack);

				xformStack.translate(Vector2(i * TileImageLength, j * TileImageLength));
				switch( tile.getTerrain() )
				{
				case TT_ROAD:
					for ( int dir = 0 ; dir < 4 ; ++dir )
						drawTilePartImpl(i, j, dir, TT_ROAD, gRoadTexCoord);
					break;
				case TT_WATER:
					for ( int dir = 0 ; dir < 4 ; ++dir )
						drawTilePartImpl(i, j, dir, TT_WATER, gLakeTexCoord);
					break;
				case TT_GRASS:
				default:
					{

						float tw = float(TileImageLength) / GameTilesTexLength;
						float th = -float(TileImageLength) / GameTilesTexLength;
						float tx = 0;
						float ty = -th;
#if USE_TEXTURE_ATLAS
						Vector2 uvMin, uvMax;
						mTexAtlas.getRectUV( GetTexImageId( TID_GAME_TILES ) , uvMin, uvMax);
						Vector2 size = uvMax - uvMin;
						tx = uvMin.x + size.x * tx;
						ty = uvMin.y + size.y * ty;
						tw *= size.x;
						th *= size.y;
#endif
						drawQuad(TileImageLength, TileImageLength, tx , ty , tw , th);
					}
				}
			}
		}

		submitRenderCommand(commandList);

		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA , Blend::eSrcAlpha , Blend::eOneMinusSrcAlpha >::GetRHI());

		TilePos posTileMouse = calcTilePos( mLastMousePos );
		bool renderQueue = true;

		for( int j = 0 ; j < map.getSizeY() ; ++j )
		{
			for( int i = 0 ; i < map.getSizeX() ; ++i )
			{
				Vec2i pos = mMapOffset + TileLength * Vec2i( i , j );

				Tile const& tile = map.getData( i , j );

				TRANSFORM_STACK_SCOPE(xformStack);

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
						//depth offset = j
						xformStack.translate(data.pos);
						{
							TRANSFORM_STACK_SCOPE(xformStack);
							xformStack.translate(Vector2((TileImageLength - ItemImageSize.x) / 2, TileImageLength - ItemImageSize.y));
							drawItemByTexId(TID_ITEM_SHADOW_LARGE);

							ActorData& e = mLevel->getActor(tile);
							drawItem(e.id, EItemImage::eNormal);
						}
					}
					else if ( tile.id != OBJ_NULL )
					{
						//depth offset = j
						xformStack.translate(data.pos);
						{
							TRANSFORM_STACK_SCOPE(xformStack);
							xformStack.translate(Vector2((TileImageLength - ItemImageSize.x) / 2, TileImageLength - ItemImageSize.y));
							drawItemByTexId(TID_ITEM_SHADOW_LARGE);
							drawItem(tile.id, (tile.bSpecial && gItemImageList[tile.id].addition) ? EItemImage::eAddition : EItemImage::eNormal);
						}


						if ( tile.id == OBJ_STOREHOUSE )
						{
							if ( tile.meta )
							{
								//depth offset = 0.1
								TRANSFORM_STACK_SCOPE(xformStack, RenderTransform2D( Vector2(scaleItem, scaleItem) , Vector2(TileImageLength / 2, TileImageLength / 2)) );
								drawItemCenter( tile.meta , EItemImage::eNormal );
							}
						}
					}
				}
			}
		}

		submitRenderCommand(commandList);

		if ( renderQueue )
		{
			if ( mLevel->isMapRange( posTileMouse ) )
			{
				//depth offset = 10
				TRANSFORM_STACK_SCOPE(xformStack, RenderTransform2D( Vector2(mItemScale * scaleMap, mItemScale * scaleMap) , Vector2(mLastMousePos) ) , false);

				drawItemCenter( mLevel->getQueueObject() , EItemImage::eNormal );
				//drawItemOutline( *mItemImageMap[ mLevel->getQueueObject() ][ 2 ].texture );
			}
		}
		else
		{

			//depth = 50
			TRANSFORM_STACK_SCOPE(xformStack, RenderTransform2D::Translate(posTileMouse.x * TileImageLength, posTileMouse.y * TileImageLength));
			drawImageByTextId(TID_UI_CURSOR, Vector2(TileImageLength, TileImageLength));
			xformStack.transform(RenderTransform2D(Vector2(mItemScale, mItemScale), 0.5 * Vector2(TileImageLength, TileImageLength)));

			drawItemCenter( mLevel->getQueueObject(), EItemImage::eNormal );
			//drawItemOutline( mTexItemMap[ mLevel->getQueueObject() ][ 2 ] );
		}
			
		submitRenderCommand(commandList);

		if( bShowTexAtlas )
		{
			TRANSFORM_STACK_SCOPE(xformStack, RenderTransform2D(Vector2(scaleMap, scaleMap), Vector2(0, 0)), false);
			drawImageInvTexY(mTexAtlas.getTexture(), 700, 700);
		}
		else if ( mPreviewTexture.isValid() && bShowPreviewTexture )
		{
			TRANSFORM_STACK_SCOPE(xformStack, RenderTransform2D(Vector2(scaleMap, scaleMap), Vector2(0, 0)), false);
			drawImageInvTexY(*mPreviewTexture, 700, 700);
		}

		submitRenderCommand(commandList);

		CHECK(xformStack.mStack.size() == 1);

	}

	void Scene::drawItemOutline( RHITexture2D& texture )
	{
		int x =  - ItemOutLineLength / 2;
		int y =  - ItemOutLineLength / 2;
		mRenderState.texture = &texture;
		drawQuad( x , y , ItemOutLineLength , ItemOutLineLength , 0 , 1 , 1 , -1 );
	}


	void Scene::drawItemByTexId(int texId)
	{
#if USE_TEXTURE_ATLAS
		Vector2 uvMin, uvMax;
		mTexAtlas.getRectUV(GetTexImageId(texId), uvMin, uvMax);
		drawQuad(ItemImageSize.x, ItemImageSize.y, uvMin.x, uvMax.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);
#else
		mRenderState.texture = mTexMap[texId];
		drawQuad(ItemImageSize.x, ItemImageSize.y, 0, 1, 1, -1);
#endif
	}

	void Scene::drawImageByTextId(int texId, Vector2 size)
	{
#if USE_TEXTURE_ATLAS
		Vector2 uvMin, uvMax;
		mTexAtlas.getRectUV(GetTexImageId(texId), uvMin, uvMax);
		drawQuad(size.x, size.y, uvMin.x, uvMin.y, uvMax.x - uvMin.x, uvMax.y - uvMin.y);
#else
		mRenderState.texture = mTexMap[texId];
		drawQuad(size.x, size.y, 0, 0, 1, 1);
#endif
	}

	void Scene::drawItem(int itemId, EItemImage::Type imageType)
	{
#if USE_TEXTURE_ATLAS
		Vector2 uvMin, uvMax;
		mTexAtlas.getRectUV(GetItemImageId(itemId, imageType), uvMin, uvMax);
	
#if USE_COMPACT_IMAGE
		CompactRect const& rect = mItemImageMap[itemId][imageType].rect;
		Vector2 offset = rect.offset * Vector2(ItemImageSize);
		Vector2 size = rect.size * Vector2(ItemImageSize);
		drawQuad(offset.x , ItemImageSize.y - offset.y - size.y , size.x , size.y , uvMin.x, uvMax.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);
#else
		drawQuad(ItemImageSize.x, ItemImageSize.y, uvMin.x, uvMax.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);
#endif

#else
		mRenderState.texture = mItemImageMap[itemId][imageType].texture;
		drawQuad(ItemImageSize.x, ItemImageSize.y, 0, 1, 1, -1);
#endif
	}

	void Scene::drawItemCenter(int itemId, EItemImage::Type imageType)
	{
		int x = -ItemImageSize.x / 2;
		int y = TileImageLength / 2 - ItemImageSize.y;
#if USE_TEXTURE_ATLAS
		Vector2 uvMin, uvMax;
		mTexAtlas.getRectUV(GetItemImageId(itemId, imageType), uvMin, uvMax);
#if USE_COMPACT_IMAGE
		CompactRect const& rect = mItemImageMap[itemId][imageType].rect;
		Vector2 offset = rect.offset * Vector2(ItemImageSize);
		Vector2 size = rect.size * Vector2(ItemImageSize);
		drawQuad(x + offset.x , y + ItemImageSize.y - offset.y - size.y , size.x, size.y, uvMin.x, uvMax.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);
#else
		drawQuad(x, y, ItemImageSize.x, ItemImageSize.y, uvMin.x, uvMax.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);
#endif

#else
		mRenderState.texture = mItemImageMap[itemId][imageType].texture;
		drawQuad(x, y, ItemImageSize.x, ItemImageSize.y, 0, 1, 1, -1);
#endif
	}

	void Scene::emitQuad(Vector2 const& p1, Vector2 const& p2, Vector2 const& uvMin, Vector2 const& uvMax)
	{
		Vector2 v0 = mRenderState.getTransform().transformPosition(p1);
		Vector2 v1 = mRenderState.getTransform().transformPosition(Vector2(p2.x, p1.y));
		Vector2 v2 = mRenderState.getTransform().transformPosition(p2);
		Vector2 v3 = mRenderState.getTransform().transformPosition(Vector2(p1.x, p2.y));

		mBatchedVertices.push_back({ v0, uvMin });
		mBatchedVertices.push_back({ v1, Vector2(uvMax.x, uvMin.y) });
		mBatchedVertices.push_back({ v2, uvMax });
		mBatchedVertices.push_back({ v3, Vector2(uvMin.x, uvMax.y) });

#if USE_TEXTURE_ATLAS == 0
		submitRenderCommand(RHICommandList::GetImmediateList());
#endif

	}

	void Scene::submitRenderCommand( RHICommandList& commandList )
	{
		if( !mBatchedVertices.empty() )
		{
			mRenderState.setupRenderState(commandList);
			TRenderRT< RTVF_XY_T2 >::Draw(commandList, EPrimitive::Quad, mBatchedVertices.data(), mBatchedVertices.size());
			mBatchedVertices.clear();
		}
	}

	template< class TFunc >
	class MoveFunc
	{
	public:
		Vector2 operator()(float t, Vector2 const& b, Vector2 const& c, float const& d)
		{
			float const height = 1500;
			float const dest = d / 2;
			if( t < dest )
				return TFunc().operator() < Vector2 > (t, b, Vector2(0, -height), dest);
			return TFunc().operator() < Vector2 > (t - dest, b + c - Vector2(0, height), Vector2(0, height), dest);
		}

	};
	void Scene::notifyActorMoved(ObjectId id, TilePos const& posFrom, TilePos const& posTo)
	{
		TileData& data = mMap.getData(posTo.x, posTo.y);

		Vector2 posRFrom = float(TileImageLength) * Vector2(posFrom);
		Vector2 posRTo = float(TileImageLength) * Vector2(posTo);
		switch( id )
		{
		case OBJ_BEAR:
			mTweener.tweenValue< Easing::IOQuad >(data.pos, posRFrom, posRTo, AnimTime, 0);
			break;
		case OBJ_NINJA:
			mTweener.tweenValue< MoveFunc< Easing::Linear > >(data.pos, posRFrom, posRTo, AnimTime, 0);
			break;
		}
	}

	void Scene::markTileDirty(TilePos const& pos)
	{
		mMap(pos.x, pos.y).bTexDirty = true;

		static const int offsetX[] = { 1,-1,0,0,1,1,-1,-1 };
		static const int offsetY[] = { 0,0,1,-1,1,-1,1,-1 };
		for( int i = 0; i < 8; ++i )
		{
			TilePos posN;
			posN.x = pos.x + offsetX[i];
			posN.y = pos.y + offsetY[i];
			if( mMap.checkRange(posN.x, posN.y) )
			{
				mMap(posN.x, posN.y).bTexDirty = true;
			}
		}
	}

	void Scene::postSetupLand()
	{

	}

	void Scene::postSetupMap()
	{
		mMap.resize(mLevel->getMap().getSizeX(), mLevel->getMap().getSizeY());

		Vec2i screenSize = ::Global::GetScreenSize();
		Vec2i mapSize = TileLength * Vec2i(mLevel->getMap().getSizeX(), mLevel->getMap().getSizeY());
		mMapOffset = (screenSize - mapSize) / 2;

		for( int i = 0; i < mMap.getSizeX(); ++i )
		{
			for( int j = 0; j < mMap.getSizeY(); ++j )
			{
				TileData& data = mMap.getData(i, j);
				data.pos.setValue(TileImageLength * i, TileImageLength * j);
				data.animOffset = Vector2::Zero();
				data.scale = 1.0f;
				data.shadowScale = 1.0f;
				data.bTexDirty = true;
			}
		}
	}

	void Scene::notifyObjectAdded(TilePos const& pos, ObjectId id)
	{
		markTileDirty(pos);
	}

	void Scene::notifyObjectRemoved(TilePos const& pos, ObjectId id)
	{
		markTileDirty(pos);
	}

	void Scene::drawQuadRotateTex90(float x, float y, float w, float h, float tx, float ty, float tw, float th)
	{
		float x2 = x + w;
		float y2 = y + h;
		float tx2 = tx + tw;
		float ty2 = ty + th;

	
		Vector2 v0 = mRenderState.getTransform().transformPosition(Vector2(x, y));
		Vector2 v1 = mRenderState.getTransform().transformPosition(Vector2(x2, y));
		Vector2 v2 = mRenderState.getTransform().transformPosition(Vector2(x2, y2));
		Vector2 v3 = mRenderState.getTransform().transformPosition(Vector2(x, y2));

		mBatchedVertices.push_back({ v0 , Vector2(tx2, ty) });
		mBatchedVertices.push_back({ v1 , Vector2(tx2, ty2) });
		mBatchedVertices.push_back({ v2 , Vector2(tx, ty2) });
		mBatchedVertices.push_back({ v3 , Vector2(tx, ty) });
#if USE_TEXTURE_ATLAS == 0
		submitRenderCommand(RHICommandList::GetImmediateList());
#endif

	}


	void Scene::drawQuad(float x, float y, float w, float h, float tx, float ty, float tw, float th)
	{
		float x2 = x + w;
		float y2 = y + h;
		float tx2 = tx + tw;
		float ty2 = ty + th;
		emitQuad(Vector2(x, y), Vector2(x2, y2), Vector2(tx, ty), Vector2(tx2, ty2));
	}

	void Scene::drawQuad(float w , float h , float tx , float ty , float tw , float th )
	{
		float tx2 = tx + tw;
		float ty2 = ty + th;
		emitQuad(Vector2(0, 0), Vector2(w, h), Vector2(tx, ty), Vector2(tx2, ty2));
	}

	void Scene::emitQuad(Vector2 const& size, Vector2 const& uvMin, Vector2 const& uvMax)
	{
		emitQuad(Vector2(0, 0), size, uvMin, uvMax);
	}

	void Scene::drawTilePartImpl( int x , int y , int dir , TerrainType tId , float const (*texCoord)[2] )
	{
		TileData& data = mMap.getData(x, y);

		if ( data.bTexDirty )
		{
			if( dir == 3 )
			{
				data.bTexDirty = false;
			}

			int const offsetX[] = { -1 , 1 , -1 , 1 };
			int const offsetY[] = { -1 , -1 , 1 , 1 };

			int xCorner = x + offsetX[dir];
			int yCorner = y + offsetY[dir];

			unsigned bitH = mLevel->getTerrain(TilePos(xCorner, y)) != tId ? 1 : 0;
			unsigned bitV = mLevel->getTerrain(TilePos(x, yCorner)) != tId ? 1 : 0;
			unsigned char con;
			con = (bitV << 1) | bitH;
			if( con == 0 )
			{
				unsigned bitC = mLevel->getTerrain(TilePos(xCorner, yCorner)) != tId ? 1 : 0;
				con |= (bitC << 2);
			}

			float const tLen = float(TileImageLength / 2) / GameTilesTexLength;
			int idx = 2 * (int)con + ((dir % 2) ? 1 : 0);

			float tx = texCoord[idx][0];
			float ty = texCoord[idx][1];
			float tw = tLen;
			float th = tLen;
			//  0   1
			//  2   3

			bool bSpecical = (con == 0x2 && tId == TT_WATER);
			if( con )
			{
				switch( dir )
				{
				case 0:
					tw = -tLen; th = -tLen;
					tx += tLen; ty += tLen;
					if( bSpecical )
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
					if( bSpecical )
					{
						tx += tw;
						tw = -tw;
					}
					break;
				}
				if( con == 0x2 )
				{
					ty += th;
					th = -th;
				}
			}
#if USE_TEXTURE_ATLAS
			Vector2 uvMin, uvMax;
			mTexAtlas.getRectUV(GetTexImageId(TID_GAME_TILES), uvMin, uvMax);
			Vector2 size = uvMax - uvMin;
			tx = uvMin.x + size.x * tx;
			ty = uvMin.y + size.y * ty;
			tw *= size.x;
			th *= size.y;
#endif

			data.texPos[dir].x = tx;
			data.texPos[dir].y = ty;
			data.texSize[dir].x = tw;
			data.texSize[dir].y = th;
			data.bSpecial[dir] = bSpecical;
		}

		int const offsetFactorX[] = { 0 , 1 , 0 , 1 };
		int const offsetFactorY[] = { 0 , 0 , 1 , 1 };
		int ox = offsetFactorX[dir] * TileImageLength / 2;
		int oy = offsetFactorY[dir] * TileImageLength / 2;

		Vector2 const& texPos = data.texPos[dir];
		Vector2 const& texSize = data.texSize[dir];
		if( data.bSpecial[dir] )
		{
			drawQuadRotateTex90(ox, oy, TileImageLength / 2, TileImageLength / 2, texPos.x, texPos.y, texSize.x, texSize.y );
		}
		else
		{
			drawQuad(ox, oy, TileImageLength / 2, TileImageLength / 2, texPos.x, texPos.y, texSize.x, texSize.y);
		}

	}

	TilePos Scene::calcTilePos( Vec2i const& pos )
	{
		TilePos tPos;
		tPos.x = ( pos.x - mMapOffset.x ) / TileLength;
		tPos.y = ( pos.y - mMapOffset.y ) / TileLength;
		return tPos;
	}

	void Scene::drawImageInvTexY( RHITexture2D& texture , int w , int h )
	{
		mRenderState.texture = &texture;
		drawQuad( w , h , 0 , 1 , 1 , -1 );
	}

	void Scene::updateFrame( int frame )
	{
		float delta = float( frame * gDefaultTickTime ) / 1000;
		mTweener.update( delta );
	}

	void Scene::loadPreviewTexture(char const* name)
	{
		FixString< 1024 > path;
		path.format("TripleTown/%s", name);
		mPreviewTexture = LoadTexture(path);
	}

}//namespace TripleTown