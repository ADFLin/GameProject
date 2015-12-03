#ifndef TEX_INFO
#	define ENUM_DEF__
#	define TEX_INFO( id , name ) id ,
#else
#	undef TextureId_h__01483E10_E1CF_45E2_BFAE_0BA290C1A825
#endif

#ifndef TextureId_h__01483E10_E1CF_45E2_BFAE_0BA290C1A825
#define TextureId_h__01483E10_E1CF_45E2_BFAE_0BA290C1A825

#ifdef ENUM_DEF__
namespace TripleTown
{
	enum TestureId
	{
#endif
		TEX_INFO( TID_GAME_TILES , "GameTiles" )
		TEX_INFO( TID_UI_CURSOR  , "ui_cursor" )
		TEX_INFO( TID_ITEM_SHADOW_LARGE , "item_shadow_large" )
		TEX_INFO( TID_GRASS_OUTLINE , "item_grass_outline" )
		TEX_INFO( TID_BEAR_1 , "bear_3" )

#ifdef ENUM_DEF__
		TEX_ID_NUM ,
	};
}//namespace TripleTown
#endif

#endif // TextureId_h__01483E10_E1CF_45E2_BFAE_0BA290C1A825


#undef ENUM_DEF__
#undef TEX_INFO