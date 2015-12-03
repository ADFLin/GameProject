#ifndef TItemSystem_h__
#define TItemSystem_h__

#include "common.h"

#include "Singleton.h"
#include "TAblilityProp.h"

#define ITEM_ERROR_ID   0xffffffff
#define ITEM_ICON_DIR   TSTR("Data/UI/items/")
#define ITEM_DATA_PATH  TSTR("data/item.dat")

enum ItemType
{
	ITEM_BASE ,
	ITEM_WEAPON ,
	ITEM_ARMOUR ,
	ITEM_FOOD   , // no impl
};

enum EquipSlot
{
	//TEquipTable and armour solt
	EQS_HEAD     , //繷场
	EQS_NECK     , //繴场
	EQS_CHEST    , //场
	EQS_WAIST    , //竬场
	EQS_LEG      , //籐场
	EQS_FOOT     , //竲场
	EQS_SHOULDER , //场
	EQS_WRIST    , //も得
	EQS_FINGER   , //も
	EQS_HAND     , //も
	EQS_HAND_L  = EQS_HAND ,
	EQS_TRINKET  , //杆耿珇
	EQS_HAND_R  ,
	/////////////////////////////
	EQS_EQUIP_TABLE_SLOT_NUM ,
	//weapon slot
	EQS_TWO_HAND ,
	EQS_MAIN_HAND,
	EQS_SUB_HAND ,

	EQS_NO_SLOT ,
	/////////////////////////////
};

enum WeaponType
{
	WT_SWORD ,
	WT_BOW   ,
	WT_AXE   ,
	WT_ANIFE ,
};


class TItemBase;
class CActor;
struct ItemDataInfo;
enum EquipSlot;


union TVarrint
{
	int   intType;
	float floatType;
};
struct ItemDataInfo
{
	ItemDataInfo();

	//////base////////////////
	ItemType  type;
	unsigned  id;

	String   name;
	String   helpStr;
	int       maxNumInSolt;
	int       buyCost;
	int       sellCost;
	float     cdTime;
	unsigned  iconID;

	std::vector< PropModifyInfo > PMVec;
	//////////////////////////////////
	//////Equipment
	EquipSlot slot;
	int       equiVal;
	//for model
	String   modelName;
	float     scale;
	Vec3D     pos;
	Quat      rotation;
	/////////////////////////////////
	//////Weapon
	WeaponType weaponType;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};


class TItemManager : public SingletonT< TItemManager >
{
public:
	TItemManager();

	unsigned   getItemID( char const* name );
	TItemBase* getItem( unsigned id );
	unsigned   queryItemNum();
	TItemBase* createItem( ItemDataInfo* info , unsigned startID = ITEM_ERROR_ID );
	unsigned   getEmptyItemID();

	void       deleteItem( unsigned id );
	void       updateMaxItemID( int addID );
	unsigned   changeItemID( unsigned oldID , unsigned newID );

	void       modifyItem( unsigned id , ItemDataInfo& info );

	bool       loadItemData( char const* name );
	bool       saveItemData( char const* name );
	void       registerItem(TItemBase* item );
	int        getMaxItemID() const { return maxItemID; }

	typedef std::vector< String >  StringVec;
	StringVec& getIconNameVec(){ return m_IconNameVec; }

	char const* getIconName( unsigned iconID )
	{
		return m_IconNameVec[iconID].c_str();
	}

	void  clear( );

	String getUnusedName( char const* str );

protected:

	int                       maxItemID;
	std::vector< TItemBase* > m_ItemTable;
	std::vector< String >     m_IconNameVec;

	struct StrCmp
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};

	typedef std::map< char const* , unsigned , StrCmp > ItemIDMap;
	ItemIDMap m_itemIDMap;

};

class TActor;

class TItemBase
{
public:
	TItemBase( unsigned itemID );
	virtual ~TItemBase(){}

	typedef std::vector<PropModifyInfo> PMInfoVec;
	
	ItemType     getType()       const { return type; }
	unsigned     getID()         const { return id; }
	char const*  getName()       const { return name.c_str(); }
	char const*  getHelpString() const { return helpStr.c_str(); }
	float        getCDTime()           { return cdTime; }
	unsigned     getIconID()           { return iconID; }
	
	bool         isEquipment()    {  return isWeapon() || isArmour() ;  }
	bool         isArmour()       { return type == ITEM_ARMOUR; }
	bool         isWeapon()       { return type == ITEM_WEAPON; }
	int          getMaxNumInSlot(){ return maxNumInSolt; }

	virtual bool use( CActor* actor);
	virtual void getDataInfo( ItemDataInfo& info );
	virtual void modify( ItemDataInfo& info );

	PMInfoVec&   getPropModifyVec(){ return PMVec; }
	int          getBuyCast(){ return buyCost; }
	int          getSellCast(){ return sellCost; }

protected:
	
	ItemType  type;
	int       maxNumInSolt;
	int       buyCost;
	int       sellCost;
	float     cdTime;
	unsigned  id;
	unsigned  iconID;
	String    name;
	String    helpStr;

	PMInfoVec PMVec;

	friend class TItemManager;

public:
	TItemBase(){}
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};

enum EquipSlot;

class TEquipment : public TItemBase
{
public:
	TEquipment(unsigned itemID);
	virtual void    equip(TActor* actor) = 0;
	EquipSlot       getEquipSolt(){ return slot; }
	int             getEquiVal(){ return equiVal; }
	char const*     getModelName(){ return modelName.c_str(); }
	virtual size_t  createModel( CFObject* objList[] );

	bool   isShield();

	//using in app
	virtual void    getDataInfo( ItemDataInfo& info );
	virtual void    modify( ItemDataInfo& info );

	static TEquipment* downCast( TItemBase* item );
protected:
	EquipSlot slot;
	int       equiVal;

	String    modelName;
	float     scale;
	Vec3D     pos;
	Quat      rotation;

	class TItemManager;

public:
	TEquipment(){}
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};

class TWeapon : public TEquipment
{
public:
	TWeapon(unsigned itemID);
	virtual void equip(TActor* actor){}
	virtual void getDataInfo( ItemDataInfo& info );
	virtual void modify( ItemDataInfo& info );
	TWeapon* downCast( TItemBase* item );
	WeaponType getWeaponType() const { return weaponType; }
protected:
	WeaponType weaponType;
	class TItemManager;


public:
	TWeapon(){}
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};

class TArmour : public TEquipment
{
public:
	TArmour(unsigned itemID);
	virtual void equip(TActor* actor){}
	virtual void getDataInfo( ItemDataInfo& info );
	virtual void modify( ItemDataInfo& info );
	virtual size_t createModel( CFObject* objList[] );
	TArmour* downCast( TItemBase* item );

protected:
	//shoulder
	Vec3D     posLeft;
	Quat      rotationLeft;
	//Shield
	float DTP;

public:
	TArmour(){}
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
	
};


class TEquipTable
{
public:
	TEquipTable();
	TEquipment* getEquipment( EquipSlot slot );
	unsigned    removeEquip( EquipSlot slot );
	EquipSlot   equip( unsigned itemID );
	bool        haveEquipment(EquipSlot slot ){	return m_EquipSlots[slot] != ITEM_ERROR_ID; }
	int         queryRemoveEquip( unsigned itemID , EquipSlot *removeSlot);
private:

	unsigned m_EquipSlots[EQS_EQUIP_TABLE_SLOT_NUM];
};


class TItemStorage
{
public:
	TItemStorage( int numSolt );

	struct ItemSlot
	{
		int      soltPos;
		unsigned itemID;
		int      num;
	};
	typedef std::list< ItemSlot > ItemSoltList;
	typedef ItemSoltList::iterator ItemSoltIter;

	size_t       getItemSlotNum(){ return m_ItemSoltTable.size(); }
	ItemSlot*    getItemSlot( int pos );
	TItemBase*   getItem( int slotPos );
	int          getItemNum( int slotPos );
	void         removeItemSlot( int pos );

	int          queryItemNum( unsigned itemID );
	int          removeItem( unsigned itemID , int MaxNum );
	int          findItemSoltPos( unsigned itemID );
	int          removeItemInSlotPos( int soltPos , int maxNum = 1 );
	bool         addItem( unsigned itemID );
	int          addItem( unsigned itemID , int maxNum );
	int          getEmptySoltPos();
	bool         haveEmptySolt( int num );

	void         swapItemSlot( int slot1 , int slot2);
	size_t       getUsedSlotNum(){ return m_ISStorage.size(); }

	bool         setSlotSize(int size );

	ItemSoltIter findItem( unsigned itemID , ItemSoltIter startIter );
private:
	ItemSoltList m_ISStorage;
	std::vector< ItemSoltIter > m_ItemSoltTable;
};


#endif // TItemSystem_h__