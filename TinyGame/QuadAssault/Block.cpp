#include "Block.h"

#include "Level.h"
#include "RenderSystem.h"
#include "TextureManager.h"

#include "RenderUtility.h"
#include "Bullet.h"
#include "Explosion.h"

static Block* gBlockMap[ 256 ] = { 0 };

struct BlockInfo;
typedef Block* (*CreateBlockFunc)(BlockId type);


struct BlockInfo
{
	BlockId   type;
	CreateBlockFunc createFunc;
	unsigned    colMask;
	unsigned    flags;
	char const* texDiffuse;
	char const* texNormal;
	char const* texGlow;
};

class BlockRenderer
{
public:
	virtual bool BlockInfo(Block& block) = 0;
	virtual void render(PrimitiveDrawer& drawer) = 0;
};


class DefaultBlockRenderer
{
public:

	virtual bool init(BlockInfo& block)
	{

		return true;
	}
	virtual void render(PrimitiveDrawer& drawer)
	{


	}

	Texture* baseTexture;
	Texture* normalTexture;

};

static Color3f gDoorColor[ NUM_DOOR_TYPE ] =
{
	Color3f( 1.0, 0.1, 0.1 ) ,
	Color3f( 0.1, 0.25, 1.0 ) ,
	Color3f( 0.1, 1.0, 0.1 ) ,
};

Color3f const& GetDoorColor( int type )
{
	return gDoorColor[ type ];
}

void Block::renderBasePass(PrimitiveDrawer& drawer, Tile const& tile)
{
	PrimitiveMat mat;
	mat.color = Color3f(1, 1, 1);
	mat.baseTex = mTex[TG_DIFFUSE];
	mat.normalTex = mTex[TG_NORMAL];
	drawer.setMaterial(mat);
	drawer.drawRect(tile.pos, gSimpleBlockSize);
}


void Block::renderGlow(PrimitiveDrawer& drawer, Tile const& tile)
{

}

void Block::renderNoTexture(PrimitiveDrawer& drawer, Tile const& tile)
{
	drawer.drawRect(tile.pos, gSimpleBlockSize);
}

void Block::onCollision( Tile& tile , Bullet* bullet )
{
	bullet->destroy();
}



class RockBlock : public Block
{
public:
	virtual void  onCollision( Tile& tile , Bullet* bullet );
	void renderBasePass(PrimitiveDrawer& drawer, Tile const& tile)
	{
		PrimitiveMat mat;
		mat.color = Color3f(1, 0, 0);
		mat.baseTex = mTex[TG_DIFFUSE];
		mat.normalTex = mTex[TG_NORMAL];
		drawer.setMaterial(mat);
		drawer.drawRect(tile.pos, gSimpleBlockSize);
	}
};

class DoorBlock : public Block
{
public:
	virtual void renderGlow(PrimitiveDrawer& drawer, Tile const& tile);

};

void RockBlock::onCollision( Tile& tile , Bullet* bullet )
{
	tile.meta -= 100 * bullet->getDamage();
	if ( tile.meta < 0 )
	{
		tile.id = BID_FLAT;
		tile.meta = 0;

		Explosion* e = bullet->getLevel()->createExplosion( tile.pos + 0.5 * gSimpleBlockSize , 128 );
		e->setParam(128,3000,200);

		bullet->getLevel()->playSound("explosion1.wav");		
	}
	bullet->destroy();
}


void DoorBlock::renderGlow(PrimitiveDrawer& drawer, Tile const& tile )
{
	Color3f color;
	switch ( tile.meta )
	{
	case DOOR_RED:   color = Color3f(1.0f, 0.2f, 0.2f); break;
	case DOOR_BLUE:  color = Color3f(0.2f, 0.2f, 1.0f); break;
	case DOOR_GREEN: color = Color3f(0.2f, 1.0f, 0.2f); break;
	default:
		color = Color3f(1,1,1);
	}
	drawer.setGlow(mTex[TG_GLOW], color);
	drawer.drawRect( tile.pos, gSimpleBlockSize);
}


struct TexInfo
{
	char const* texName[3];
};


TexInfo FlatTexInfo[] =
{
	{ "pod1Diffuse.tga" , "prazninaNormal2.tga" , NULL } ,
	{ "Bathroom.tga" , "BathroomN.tga" , NULL } ,
	{ "Tile.tga" , "TileN.tga" , NULL } ,
	{ "Metal1.tga" , "Metal1N.tga" , NULL } ,
	{ "Weave.tga" , "WeaveN.tga" , NULL } ,
	{ "Hex.tga" , "HexN.tga" , NULL } ,
};

template< class T >
Block* CreateBlockT(BlockId type)
{
	T* block = new T;
	block->init(type);
	return block;
}

#define BLOCK_CALSS(T) &CreateBlockT<T>
static BlockInfo const gInfo[] =
{
	{ BID_FLAT , BLOCK_CALSS(Block) , 0 , 0 , "pod1Diffuse.tga" , "prazninaNormal2.tga" , NULL } ,
	{ BID_WALL , BLOCK_CALSS(Block) ,COL_OBJECT | COL_VIEW , BF_CAST_SHADOW , "Block.tga" , "zid1Normal.tga" , NULL } ,
	//{ BID_WALL ,BLOCK_CALSS(Block), COL_OBJECT | COL_VIEW , BF_CAST_SHADOW , "Block.tga" , "SqureN.tga" , NULL } ,
	{ BID_GAP  , BLOCK_CALSS(Block) ,COL_SOILD | COL_TRIGGER | COL_VIEW , 0 , "prazninaDiffuse.tga" , "prazninaNormal.tga" , NULL } ,
	{ BID_DOOR , BLOCK_CALSS(DoorBlock) ,COL_OBJECT | COL_VIEW , BF_CAST_SHADOW , "vrataDiffuse.tga" , "vrataNormal.tga" , "vrataGlow.tga" } ,
	{ BID_ROCK , BLOCK_CALSS(RockBlock) ,COL_OBJECT | COL_VIEW , BF_CAST_SHADOW , "vrataDiffuse.tga" , "vrataNormal.tga" , "vrataGlow.tga" } ,
};

void Block::init(BlockId type)
{
	BlockInfo const& info = gInfo[type];

	assert(info.type == type);

	mId = info.type;
	mFlags = info.flags;
	mColMask = info.colMask;

	TextureManager* texMgr = getRenderSystem()->getTextureMgr();

	mTex[TG_DIFFUSE] = (info.texDiffuse) ? texMgr->getTexture(info.texDiffuse) : NULL;
	mTex[TG_NORMAL] = (info.texNormal) ? texMgr->getTexture(info.texNormal) : NULL;
	mTex[TG_GLOW] = (info.texGlow) ? texMgr->getTexture(info.texGlow) : NULL;
}

Block* Block::Get(BlockId id)
{
	return gBlockMap[id];
}

void Block::Initialize()
{
	std::fill_n(gBlockMap, ARRAY_SIZE(gBlockMap), nullptr);
	for( int i = 0; i < ARRAY_SIZE(gInfo); ++i )
	{
		auto const& info = gInfo[i];
		gBlockMap[info.type] = (*info.createFunc)(info.type);
	}
}

void Block::Cleanup()
{
	for( int i = 0; i < NUM_BLOCK_TYPE; i++ )
	{
		delete gBlockMap[i];
		gBlockMap[i] = NULL;
	}

}