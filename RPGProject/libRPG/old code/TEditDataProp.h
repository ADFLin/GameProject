#ifndef TEditDataProp_h__
#define TEditDataProp_h__

#include "TEditData.h"
#include "TPorpLoader.h"
#include "TLogicObj.h"
#include "TActorTemplate.h"

BEGIN_PROP_CLASS_NOBASE( EmDataBase )
	PROP_MEMFUN( "Name" , TString , EmDataBase::setName , EmDataBase::getName )
	PROP_MEMFUN( "Show" , bool    , EmDataBase::show , EmDataBase::isShow )
END_PROP_CLASS()

BEGIN_ENUM_DATA( ObjEditFlag )
	ENUM_DATA( EDF_TERRAIN )
	ENUM_DATA( EDF_CREATE )
	ENUM_DATA( EDF_SAVE_XFORM )
	ENUM_DATA( EDF_LOAD_MODEL )
	ENUM_DATA( EDF_SAVE_COL_SHAPE )
END_ENUM_DATA()

BEGIN_PROP_CLASS( EmObjData , EmDataBase )
	PROP_MEMFUN( "Position" , Vec3D , EmObjData::setPosition , EmObjData::getPosition )
	PROP_MEMFUN( "Front Dir" , Vec3D , EmObjData::setFrontDir , EmObjData::getFrontDir )
	PROP_MEMFUN( "Up Dir" , Vec3D , EmObjData::setUpDir , EmObjData::getUpDir )
	PROP_ENUM_FLAG_MEMFUN( "Flag" , ObjEditFlag , EmObjData::setFlag , EmObjData::getFlag )
END_PROP_CLASS()


BEGIN_PROP_CLASS( EmSceneObj , EmObjData )
	PROP_MEMFUN( "Render Group" , RenderGroup , EmSceneObj::setRenderGroup , EmSceneObj::getRenderGroup)
END_PROP_CLASS()

BEGIN_PROP_CLASS( EmSpawnZone , EmDataBase )
	PROP_SUB_CLASS_PTR( TSpawnZone , spawnZone )
END_PROP_CLASS()

BEGIN_PROP_CLASS_NOBASE( TSpawnZone )
	PROP_MEMBER( "Position"   ,  TSpawnZone::pos )
	PROP_MEMFUN( "Spawn Role" ,  unsigned  , TSpawnZone::setSpawnRoleID , TSpawnZone::getSpawnRoleID  )
END_PROP_CLASS()

BEGIN_PROP_CLASS( EmLogicGroup, EmDataBase )
END_PROP_CLASS()

BEGIN_ENUM_DATA( RenderGroup )
	ENUM_DATA( RG_TERRAIN )
	ENUM_DATA( RG_SHADOW )
	ENUM_DATA( RG_VOLUME_LIGHT )
	ENUM_DATA( RG_DYNAMIC_OBJ )
	ENUM_DATA( RG_ACTOR )
	ENUM_DATA( RG_FX )
END_ENUM_DATA()


BEGIN_ENUM_DATA( ActorType )
	ENUM_DATA( AT_NPCBASE )
	ENUM_DATA( AT_VILLAGER )
END_ENUM_DATA()

BEGIN_PROP_CLASS( EmActor , EmObjData )
	PROP_MEMFUN( "Actor Type" , ActorType , EmActor::setActorType , EmActor::getActorType )
	PROP_MEMBER( "Prop" , EmActor::propStr )
	//PROP_MEMFUN( "Name" , TString , TEditData::setName , TEditData::getName )
END_PROP_CLASS()

BEGIN_PROP_CLASS( EmPlayerBoxTGR , EmObjData )
	PROP_MEMFUN( "Box size" , Vec3D , EmPlayerBoxTGR::setBoxSize , EmPlayerBoxTGR::getBoxSize )
//PROP_MEMFUN( "Name" , TString , TEditData::setName , TEditData::getName )
END_PROP_CLASS()

BEGIN_PROP_CLASS( EmPhyObj , EmObjData )
	PROP_MEMBER( "Model Name" , EmPhyObj::modelName )
END_PROP_CLASS()


BEGIN_PROP_CLASS( EmGroup , EmDataBase )
	PROP_MEMFUN( "Item Num" , unsigned , EmGroup::setChildrenNum , EmGroup::getChildrenNum )
END_PROP_CLASS()


BEGIN_PROP_CLASS( EmChangeLevel , EmDataBase )
	PROP_SUB_CLASS_PTR( TChangeLevel , logic )
END_PROP_CLASS()

BEGIN_PROP_CLASS_NOBASE( TChangeLevel )
	PROP_MEMBER( "Level Name" , TChangeLevel::levelName )
	PROP_MEMBER( "Player Position" , TChangeLevel::playerPos )
END_PROP_CLASS()

BEGIN_ENUM_DATA( TeamFlag )
    ENUM_DATA( TEAM_PLAYER   )
	ENUM_DATA( TEAM_VILLAGER )
	ENUM_DATA( TEAM_MONSTER  )
	ENUM_DATA( TEAM_ANIMAL   )
END_ENUM_DATA()

BEGIN_ENUM_DATA( RaceType )
	ENUM_DATA( RACE_HUMAN   )
	ENUM_DATA( RACE_UNKONOWN )
END_ENUM_DATA()

BEGIN_ENUM_DATA( CareerType )
	ENUM_DATA( CRT_UNKNOW  )
	ENUM_DATA( CRT_WARRIOR )
	ENUM_DATA( CRT_MAGE    )
END_ENUM_DATA()

BEGIN_PROP_CLASS_NOBASE( TRoleData )
	PROP_MEMBER( "Name"        , TRoleData::roleName )
	PROP_MEMBER( "Model"       , TRoleData::modelName )
	PROP_MEMBER( "Team"        , TRoleData::team )
	PROP_MEMBER( "Race"        , TRoleData::race )
	PROP_MEMBER( "Career"      , TRoleData::career )
	PROP_MEMBER( "Use Weapon"  , TRoleData::useWeapon )
	PROP_SUB_CLASS( TAbilityPropData  , propData )
END_PROP_CLASS()

BEGIN_PROP_CLASS_NOBASE( TAbilityPropData )
	PROP_MEMBER( "Level"           , TAbilityPropData::level )
	PROP_MEMBER( "Max HP"          , TAbilityPropData::MaxHP )
	PROP_MEMBER( "Max MP"          , TAbilityPropData::MaxMP )
	PROP_MEMBER( "STR"             , TAbilityPropData::STR )
	PROP_MEMBER( "INT"             , TAbilityPropData::INT )
	PROP_MEMBER( "DEX"             , TAbilityPropData::DEX )
	PROP_MEMBER( "END"             , TAbilityPropData::END )
	PROP_MEMBER( "Kill Exp"        , TAbilityPropData::KExp )
	PROP_MEMBER( "View Dist"       , TAbilityPropData::viewDist )
	PROP_MEMBER( "Attack Range"    , TAbilityPropData::ATRange )
	PROP_MEMBER( "Attack Speed"    , TAbilityPropData::ATSpeed )
	PROP_MEMBER( "Movement Speed"  , TAbilityPropData::MVSpeed )
	PROP_MEMBER( "Jump Height"     , TAbilityPropData::JumpHeight )
END_PROP_CLASS()


#endif // TEditDataProp_h__
