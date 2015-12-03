#ifndef TActor_h__
#define TActor_h__

#include "common.h"
#include "TPhyEntity.h"

#include <vector>

class TSkill;


class TEffectBase;
class TMovement;
class TEquipTable;
class TAbilityProp;
class TItemStorage;
class TAnimation;
class TSkillBook;
class TCDManager;

struct TActorModelData;
struct DamageInfo;
struct MoveResult;
struct PropModifyInfo;
struct TRoleData;

enum  ModelType;
enum  ActivityType;
enum  MoveType;
enum  ActorState;
enum  PropType;
enum  AnimationType;
enum  EquipSlot;

#define DEFULT_ROLE_ID     0


#endif // TActor_h__