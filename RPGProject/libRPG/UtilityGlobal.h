#ifndef UtilityGlobal_h__
#define UtilityGlobal_h__

Viewport*  UG_GetScreenViewport();

struct DamageInfo;
void       UG_InputDamage( DamageInfo& info );

struct TEvent;
class  EvtCallBack;
enum   EventType;
class  HandledObject;
void       UG_SendEvent( TEvent& event );
void       UG_ProcessEvent( TEvent& event );
void       UG_ConnectEvent( EventType type , int id , EvtCallBack const& callback , HandledObject* holder = nullptr );
void       UG_DisconnectEvent( EventType type , EvtCallBack const& callback , HandledObject* holder = nullptr );

class TEffectBase;
class TEntityBase;
void       UG_AddEffect( TEntityBase* entity , TEffectBase* effect );
bool       UG_RemoveEffect( TEntityBase* entity , TEffectBase* effect );

class TItemBase;
TItemBase* UG_GetItem( unsigned itemID );
TItemBase* UG_GetItem( char const* itemName );
unsigned   UG_GetItemID( char const* itemName );

class  TSkill;
struct SkillInfo;
SkillInfo const* UG_QuerySkill( char const* name , int level );


//AUDIOid UG_PlaySound( char const* name , float volume , DWORD mode = ONCE );
//OBJECTid UG_PlaySound3D( OBJECTid objID , char const* name , float volume , DWORD mode = ONCE );
//OBJECTid UG_PlaySound3D( Vec3D const& pos , char const* name , float volume , DWORD mode = ONCE );




#endif // UtilityGlobal_h__