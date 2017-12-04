#ifndef BlockType_h__
#define BlockType_h__

#define BLOCK_TYPE_LIST( op )\
	op( BID_FLAT , "Flat" )\
	op( BID_WALL , "Wall" )\
	op( BID_GAP  , "Gap" )\
	op( BID_DOOR , "Door" )\
	op( BID_ROCK , "Rock" )

#define EnumOp( A ,...) A,

enum BlockTypeEnum
{
	BLOCK_TYPE_LIST( EnumOp )
	NUM_BLOCK_TYPE ,
};

#undef EnumOp

#endif // BlockType_h__