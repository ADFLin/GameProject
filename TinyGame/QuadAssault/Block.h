#ifndef BLOK_H
#define BLOK_H

#include "Base.h"
#include "DataStructure/Grid2D.h"
#include "BlockId.h"

Vec2f const gSimpleBlockSize = Vec2f( BLOCK_SIZE , BLOCK_SIZE );

enum DoorType
{
	DOOR_RED   = 0 ,
	DOOR_GREEN = 1 ,
	DOOR_BLUE  = 2 ,

	NUM_DOOR_TYPE ,
};

Vec3f const& getDoorColor( int type );

enum BlockFlag
{
	BF_CAST_SHADOW   = BIT( 0 ),
	BF_NONSIMPLE     = BIT( 1 ),
};

typedef unsigned char BlockId;
struct Tile
{
	BlockId   id;
	int       meta;
	Vec2f     pos;
};

typedef TGrid2D< Tile , FastMapping > TileMap;

class Light;
class Texture;

class Block
{
public:
	static void   Initialize();
	static void   Cleanup();
	static Block* Get(BlockId id);

public:
	virtual ~Block(){}
	bool     checkFlag( unsigned checkBits ){ return ( mFlag & checkBits) != 0; }
	unsigned getColMask(){ return mColMask; }

	virtual void  init( BlockId type );
	virtual void  onCollision( Tile& tile , Bullet* bullet );

	virtual void  render( Tile const& tile );
	virtual void  renderNormal( Tile const& tile );
	virtual void  renderGlow( Tile const& tile );
	virtual void  renderNoTexture( Tile const& tile );

	//call when block is not simple
	virtual void  renderShadow( Tile const& tile , Vec2f const& lightPos , Light& light ){}
	virtual bool  rayTest( Tile const& tile , Vec2f const& from , Vec2f const& to ){ return false; }
	virtual bool  testIntersect( Tile const& tile , Rect const& bBox ){ return false; }


	Texture* getTexture( int idx ){ return mTex[ idx ]; }

protected:
	BlockId   mId;
	unsigned  mFlag;
	unsigned  mColMask;
	Texture*  mTex[ NUM_RENDER_PASS ];
};

#endif