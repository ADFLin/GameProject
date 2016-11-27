#ifndef TextureId_h__01483E10_E1CF_45E2_BFAE_0BA290C1A825
#define TextureId_h__01483E10_E1CF_45E2_BFAE_0BA290C1A825

#define TEXTURE_ID_LIST( op )\
	op( TID_GAME_TILES , "GameTiles" )\
	op( TID_UI_CURSOR  , "ui_cursor" )\
	op( TID_ITEM_SHADOW_LARGE , "item_shadow_large" )\
	op( TID_GRASS_OUTLINE , "item_grass_outline" )\
	op( TID_BEAR_1 , "bear_3" )

#define EnumOp( A , ... ) A,
namespace TripleTown
{
	enum TestureId
	{
		TEXTURE_ID_LIST(EnumOp)
		TEX_ID_NUM ,
	};
}//namespace TripleTown

#undef EnumOp
#endif // TextureId_h__01483E10_E1CF_45E2_BFAE_0BA290C1A825


