#include "ProjectPCH.h"
#include "TItemSystem.h"

#include "serialization.h"

#include "CFObject.h"

//#include "UtilityGlobal.h"
#include "TResManager.h"

BOOST_CLASS_VERSION( ItemDataInfo , 5 )
static int const g_ItemDataVersion = 5;

template < class Archive >
void registerItemType( Archive & ar )
{
	ar.register_type< TWeapon >();
	ar.register_type< TArmour >();
}


template < class Archive >
void ItemDataInfo::serialize( Archive & ar , const unsigned int version )
{
	ar & type & id & name & maxNumInSolt & buyCost & sellCost;

	ar & cdTime & helpStr & iconID;

	ar & PMVec;

	if ( type == ITEM_WEAPON || type == ITEM_ARMOUR )
	{
		ar & slot;
		ar & equiVal;
		if ( version >= 2)
			ar & modelName;

		if ( version >= 3 )
		{
			ar & scale & pos & rotation;
		}
	}

	if ( type ==  ITEM_WEAPON )
	{
		if ( version >= 5)
			ar & weaponType;
	}

	char c = '\n';
	if ( version >= 4 )
		ar & c;
}

template < class Archive >
void TItemBase::serialize( Archive & ar , const unsigned int file_version )
{
	ar & type & id & name & maxNumInSolt & buyCost & sellCost;
	ar & cdTime & helpStr & iconID;
	ar & PMVec;
}

template < class Archive >
void TEquipment::serialize( Archive & ar , const unsigned int file_version )
{
	ar & boost::serialization::base_object<TItemBase>(*this);
	ar & slot & equiVal & modelName;
	ar & scale & pos & rotation;
}

template < class Archive >
void TWeapon::serialize( Archive & ar , const unsigned int  file_version )
{
	ar & boost::serialization::base_object<TEquipment>(*this);
	ar & weaponType;
}

template < class Archive >
void TArmour::serialize( Archive & ar , const unsigned int  file_version )
{
	ar & boost::serialization::base_object<TEquipment>(*this);
	if ( slot == EQS_SHOULDER )
	{
		ar & posLeft & rotationLeft;
	}
	else if ( slot == EQS_HAND )
	{
		ar & DTP;
	}
}


ItemDataInfo::ItemDataInfo()
:scale( 1.0f )
,pos( 0,0,0 )
,rotation( 0,0,0,1)
{

	cdTime  = 0.0;
	helpStr = TSTR("");
	iconID   = 0;
	modelName = TSTR("weapon01");

	weaponType = WT_SWORD;
}


TItemManager::TItemManager()
{
	maxItemID = -1;
	m_ItemTable.resize( 1024 , NULL );
}

unsigned TItemManager::getItemID( char const* name )
{
	if ( name )
	{
		ItemIDMap::iterator iter = m_itemIDMap.find( name );
		if ( iter != m_itemIDMap.end() )
			return iter->second;
	}
	return ITEM_ERROR_ID;
}

bool TItemManager::saveItemData( char const* name )
{
	try
	{
		//std::ofstream fs(name);
		//boost::archive::binary_oarchive ar(fs);


		//int size = queryItemNum();
		//ar & size;

		//for( int i = 0; i <= maxItemID ; ++i )
		//{
		//	if ( ! m_ItemTable[i] )
		//		continue;

		//	ItemDataInfo info;
		//	m_ItemTable[i]->getDataInfo( info );
		//	ar & info;
		//}

		//ar & m_IconNameVec;

		std::ofstream fs(name);
		boost::archive::text_oarchive ar(fs);

		registerItemType( ar );

		ar & m_IconNameVec;

		int size = queryItemNum();
		ar & size;

		for( int i = 0; i <= maxItemID ; ++i )
		{
			TItemBase* item = m_ItemTable[i];
			if ( ! item )
				continue;
			ar & item;
		}


	}
	catch ( ... )
	{
		return false;
	}

	return true;
}

bool TItemManager::loadItemData( char const* name )
{
	try
	{
		//std::ifstream fs(name);
		//boost::archive::binary_iarchive ar(fs);

		//int size;
		//ar & size;

		//for( int i = 0; i < size ; ++i )
		//{
		//	ItemDataInfo info;
		//	ar & info;
		//	createItem( &info , info.id );
		//}

		//ar & m_IconNameVec;

		std::ifstream fs(name);
		boost::archive::text_iarchive ar(fs);

		registerItemType( ar );

		ar & m_IconNameVec;

		int size;
		ar & size;

		for( int i = 0; i < size ; ++i )
		{
			TItemBase* item;

			ar & item;

			m_ItemTable[ item->getID() ] = item;
			updateMaxItemID( item->getID() );
			registerItem( item );
		}

	}
	catch ( ... )
	{
		return false;
	}

	return true;
}
unsigned TItemManager::queryItemNum()
{
	unsigned result = 0;
	for( int i = 0 ; i < m_ItemTable.size(); ++i )
	{
		if ( m_ItemTable[i] )
			++result;
	}
	return result;
}

unsigned TItemManager::getEmptyItemID()
{
	for( int i = 0 ; i < m_ItemTable.size(); ++i )
	{
		if ( !m_ItemTable[i] )
			return i;
	}

	return ITEM_ERROR_ID;
}

TItemBase* TItemManager::getItem( unsigned id )
{
	if ( id == ITEM_ERROR_ID )
		return NULL;
	return m_ItemTable[id];
}

TItemBase* TItemManager::createItem( ItemDataInfo* info , unsigned startID /*= ITEM_ERROR_ID */ )
{
	if ( startID == ITEM_ERROR_ID )
		startID = getEmptyItemID();

	assert( startID != ITEM_ERROR_ID );

	TItemBase* item = NULL;

	info->name = getUnusedName( info->name.c_str() );

	info->id = startID;
	switch( info->type )
	{
	case ITEM_BASE:   item = new TItemBase(startID);  break;
	case ITEM_WEAPON: item = new TWeapon(startID);  break;
	case ITEM_ARMOUR: item = new TArmour(startID); break;
	}

	item->modify( *info );

	m_ItemTable[ startID ] = item;

	updateMaxItemID( startID );
	registerItem( item );

	return item;
}


void TItemManager::modifyItem( unsigned id , ItemDataInfo& info )
{
	TItemBase* item = getItem( id );
	assert( item );

	ItemIDMap::iterator iter = m_itemIDMap.find( item->getName() );

	if ( iter != m_itemIDMap.end() )
	{
		m_itemIDMap.erase( iter );
	}

	info.name = getUnusedName( info.name.c_str() );
	item->modify( info );

	registerItem( item );
}

void TItemManager::registerItem( TItemBase* item )
{
	m_itemIDMap.insert( std::make_pair( item->getName() , item->getID() ) );
}

unsigned TItemManager::changeItemID( unsigned oldID , unsigned newID )
{
	TItemBase* item = getItem( oldID );
	while ( getItem( newID ) != NULL )
	{
		++newID;
		if( newID == ITEM_ERROR_ID )
			newID = 0;
	}

	item->id = newID;
	m_itemIDMap[ item->getName() ] = newID;
	m_ItemTable[ oldID ] = NULL;
	m_ItemTable[ newID ] = item;

	updateMaxItemID( oldID );
	updateMaxItemID( newID );

	return newID;
}

void TItemManager::updateMaxItemID( int modifyID )
{
	if ( modifyID > maxItemID )
	{
		maxItemID = modifyID;
		return;
	}
	else if ( modifyID == maxItemID )
	{
		for ( int id = maxItemID ; id != 0 ; --id )
		{
			if ( getItem( id ) )
			{
				maxItemID = id;
				return;
			}
		}
	}
}

void TItemManager::deleteItem( unsigned id )
{
	TItemBase* item = getItem( id );
	if ( !item )
		return;

	m_itemIDMap.erase( m_itemIDMap.find( item->getName() ) );
	m_ItemTable[id] = NULL;

	updateMaxItemID( id );
}

void TItemManager::clear()
{
	for(std::vector< TItemBase* >::iterator iter = m_ItemTable.begin();
		iter != m_ItemTable.end() ; ++iter )
	{
		delete *iter;
	}
	m_ItemTable.clear();
	m_itemIDMap.clear();
}

String TItemManager::getUnusedName( char const* str )
{
	ItemIDMap::iterator iter = m_itemIDMap.find( str );


	if ( iter !=  m_itemIDMap.end() )
	{
		char name[64];
		int i = 1;
		while ( iter != m_itemIDMap.end() )
		{
			sprintf( name , "%s_%d" , str , i );
			iter = m_itemIDMap.find( name );
			++i;
		}
		return name;

	}
	return str;
}

TItemStorage::TItemStorage( int numSolt )
{
	m_ItemSoltTable.resize( numSolt , m_ISStorage.end() );
}

TItemStorage::ItemSlot* TItemStorage::getItemSlot( int pos )
{
	if ( m_ItemSoltTable[pos] != m_ISStorage.end() )
		return &( *m_ItemSoltTable[pos] );

	return NULL;
}

void TItemStorage::removeItemSlot( int pos )
{
	m_ISStorage.erase( m_ItemSoltTable[pos] );
	m_ItemSoltTable[pos] = m_ISStorage.end();
}

int TItemStorage::removeItemInSlotPos( int soltPos , int maxNum /*= 1 */ )
{
	ItemSlot* solt = getItemSlot( soltPos );

	if ( solt == NULL )
		return 0;

	int num = TMin( maxNum , solt->num );

	solt->num -= num;

	if ( solt->num <= 0  )
	{
		removeItemSlot( soltPos );
	}
	return num;
}

bool TItemStorage::addItem( unsigned itemID )
{
	TItemBase* item = TItemManager::getInstance().getItem( itemID );
	int maxNum = item->getMaxNumInSlot();
	for( ItemSoltIter iter = m_ISStorage.begin();
		iter != m_ISStorage.end(); ++iter )
	{
		if ( iter->itemID == itemID && 
			iter->num < maxNum )
		{
			++iter->num;
			return true;
		}
	}

	int soltPos = getEmptySoltPos();
	if ( soltPos >= 0 )
	{
		ItemSlot solt;
		solt.itemID  = itemID;
		solt.num     = 1;
		solt.soltPos = soltPos;

		m_ISStorage.push_front(solt);
		m_ItemSoltTable[ soltPos ] = m_ISStorage.begin();

		return true;
	}

	return false;
}

int TItemStorage::addItem( unsigned itemID , int maxNum )
{
	TItemBase* item = TItemManager::getInstance().getItem( itemID );
	int maxItemNum = item->getMaxNumInSlot();

	ItemSoltIter iter = m_ISStorage.begin();
	while ( maxNum && iter != m_ISStorage.end() )
	{
		if ( iter->itemID == itemID )
		{
			int addNum = TMin( maxNum , maxItemNum - iter->num );

			maxNum    -= addNum;
			iter->num += addNum;
		}
		++iter;
	}


	int slotPos;
	while ( maxNum && ( slotPos = getEmptySoltPos() ) >= 0  )
	{
		int addNum = TMin( maxNum , maxItemNum );

		maxNum    -= addNum;

		ItemSlot slot;
		slot.itemID  = itemID;
		slot.num     = addNum;
		slot.soltPos = slotPos;

		m_ISStorage.push_front(slot);
		m_ItemSoltTable[ slotPos ] = m_ISStorage.begin();
	}

	return maxNum;
}

int TItemStorage::getEmptySoltPos()
{
	for( int i = 0 ; i < m_ItemSoltTable.size() ; ++i )
	{
		if ( m_ItemSoltTable[i] == m_ISStorage.end() )
			return i;
	}
	return -1;
}

int TItemStorage::queryItemNum( unsigned itemID )
{
	int result = 0;
	for( ItemSoltIter iter = m_ISStorage.begin();
		iter != m_ISStorage.end(); ++iter )
	{
		ItemSlot& solt = *iter;

		if ( solt.itemID == itemID )
		{
			result += solt.num;
		}
	}
	return result;
}

TItemStorage::ItemSoltIter TItemStorage::findItem( unsigned itemID , ItemSoltIter startIter )
{
	for( ItemSoltIter iter = startIter;
		iter != m_ISStorage.end(); ++iter )
	{
		if ( iter->itemID == itemID )
			return iter;
	}
	return m_ISStorage.end();
}

int TItemStorage::removeItem( unsigned itemID , int MaxNum )
{
	ItemSoltIter iter = m_ISStorage.begin();
	int removeNum = 0;

	while ( MaxNum > 0 && iter != m_ISStorage.end() )
	{
		iter = findItem( itemID , iter );

		int useNum = TMin( iter->num , MaxNum );

		int pos = iter->soltPos;
		++iter; //removeSoltPosItem maybe remove cur node
		removeItemInSlotPos( pos , useNum );
		MaxNum -= useNum;
		removeNum += useNum;
	}

	return removeNum;
}

void TItemStorage::swapItemSlot( int slot1 , int slot2 )
{
	std::swap( m_ItemSoltTable[slot1] ,  m_ItemSoltTable[slot2] );
}

TItemBase* TItemStorage::getItem( int slotPos )
{
	if ( ItemSlot* slot = getItemSlot( slotPos ) )
	{
		return TItemManager::getInstance().getItem( slot->itemID );
	}
	return NULL;
}

int TItemStorage::getItemNum( int slotPos )
{
	ItemSlot* slot = getItemSlot( slotPos );
	if ( slot )
		return slot->num;

	return 0;
}

bool TItemStorage::setSlotSize( int size )
{
	if ( size < m_ItemSoltTable.size() )
	{
		return false;
	}
	else
	{
		m_ItemSoltTable.insert( m_ItemSoltTable.end() ,
			size - m_ItemSoltTable.size() , m_ISStorage.end() );
	}
	return true;
}

bool TItemStorage::haveEmptySolt( int num )
{
	return m_ItemSoltTable.size() - m_ISStorage.size() >=  num;
}

int TItemStorage::findItemSoltPos( unsigned itemID )
{
	ItemSoltIter iter = findItem( itemID , m_ISStorage.begin() );
	if ( iter != m_ISStorage.end() )
	{
		return iter->soltPos;
	}
	return -1;
}
TItemBase::TItemBase( unsigned itemID )
{
	type = ITEM_BASE;
	id = itemID;
}

bool TItemBase::use( CActor* actor )
{
	return true;
}

TEquipment::TEquipment( unsigned itemID ) :TItemBase(itemID)
{

}
TEquipment* TEquipment::downCast( TItemBase* item )
{
	if ( item && item->isEquipment() )
		return (TEquipment*)item;
	return NULL;
}


TWeapon::TWeapon(unsigned itemID)
:TEquipment( itemID )
{
	type = ITEM_WEAPON;
}


TWeapon* TWeapon::downCast( TItemBase* item )
{
	if ( item && item->isWeapon() )
		return (TWeapon*)item;
	return NULL;
}


TArmour::TArmour( unsigned itemID ) 
:TEquipment( itemID )
,posLeft( 0,0,0 )
,rotationLeft( 0,0,0,1)
,DTP(10)
{
	type = ITEM_ARMOUR;
}


TArmour* TArmour::downCast( TItemBase* item )
{
	if ( item && item->isWeapon() )
		return (TArmour*)item;
	return NULL;
}


TEquipTable::TEquipTable()
{
	for( int i = 0 ; i < EQS_EQUIP_TABLE_SLOT_NUM ; ++i )
	{
		m_EquipSlots[i] = ITEM_ERROR_ID;
	}
}

TEquipment* TEquipTable::getEquipment( EquipSlot slot )
{
	return  static_cast< TEquipment* >( TItemManager::getInstance().getItem( m_EquipSlots[ slot ] ) );
}

int TEquipTable::queryRemoveEquip( unsigned itemID , EquipSlot *removeSlot)
{
	int numRemoveItem = 0;
	TEquipment* equipment = TEquipment::downCast( 
		TItemManager::getInstance().getItem( itemID ) );

	assert( equipment );

	EquipSlot slot = equipment->getEquipSolt();
	TEquipment* rEquip = getEquipment( EQS_HAND_R );

	if ( slot == EQS_MAIN_HAND )
	{
		if ( rEquip )
		{
			if ( rEquip->getEquipSolt() == EQS_SUB_HAND )
			{
				if ( m_EquipSlots[ EQS_HAND_L] )
				{
					removeSlot[ numRemoveItem++ ] = EQS_HAND_L;
				}
			}
			else
			{
				removeSlot[ numRemoveItem++ ] = EQS_HAND_R;
			}
		}
	}
	else if ( slot == EQS_SUB_HAND )
	{
		if ( rEquip && rEquip->getEquipSolt() == EQS_TWO_HAND )
		{
			removeSlot[ numRemoveItem++ ] = EQS_HAND_R;
		}
		else if ( m_EquipSlots[EQS_HAND_L] != ITEM_ERROR_ID  && 
			m_EquipSlots[EQS_HAND_R] != ITEM_ERROR_ID  )
		{
			removeSlot[ numRemoveItem++ ] = EQS_HAND_R;
		}
	}
	else if ( slot == EQS_TWO_HAND )
	{
		if ( m_EquipSlots[EQS_HAND_L] != ITEM_ERROR_ID )
		{
			removeSlot[ numRemoveItem++ ] = EQS_HAND_L;
		}

		if ( m_EquipSlots[EQS_HAND_R] != ITEM_ERROR_ID )
		{
			removeSlot[ numRemoveItem++ ] = EQS_HAND_R;
		}
	}
	else if ( slot == EQS_HAND )
	{
		if ( haveEquipment( EQS_HAND_L ) &&
			haveEquipment( EQS_HAND_R ) )
		{
			removeSlot[ numRemoveItem++ ] = EQS_HAND_R;
		}
	}
	else
	{
		if ( m_EquipSlots[slot] != ITEM_ERROR_ID )
		{
			removeSlot[ numRemoveItem++ ] = slot;
		}
	}

	return numRemoveItem;
}


EquipSlot TEquipTable::equip( unsigned itemID )
{
	int numRemoveItem = 0;
	EquipSlot resultSlot =  EQS_NO_SLOT;

	TEquipment* equipment = TEquipment::downCast( 
		TItemManager::getInstance().getItem(itemID) );

	if ( !equipment )
		return resultSlot;

	EquipSlot slot = equipment->getEquipSolt();

	if ( slot == EQS_MAIN_HAND )
	{
		if ( TEquipment* rEqui = getEquipment( EQS_HAND_R ) )
		{
			if ( rEqui->getEquipSolt() == EQS_SUB_HAND )
			{
				m_EquipSlots[ EQS_HAND_L ] = m_EquipSlots[ EQS_HAND_R ];
			}
		}

		m_EquipSlots[ EQS_HAND_R ] = itemID;
		resultSlot = EQS_HAND_R;
	}
	else if ( slot == EQS_SUB_HAND )
	{
		if ( m_EquipSlots[EQS_HAND_L] != ITEM_ERROR_ID )
		{
			if (  m_EquipSlots[EQS_HAND_R] != ITEM_ERROR_ID  )
			{
				m_EquipSlots[EQS_HAND_R] = m_EquipSlots[EQS_HAND_L];
				m_EquipSlots[EQS_HAND_L] = itemID;
				resultSlot = EQS_HAND_L;
			}
			else
			{
				m_EquipSlots[EQS_HAND_R] = itemID;
				resultSlot = EQS_HAND_R;
			}
		}
		else
		{
			m_EquipSlots[EQS_HAND_L] = itemID;
			resultSlot = EQS_HAND_L;
		}
	}
	else if ( slot == EQS_TWO_HAND )
	{
		m_EquipSlots[EQS_HAND_R] = itemID;
		resultSlot = EQS_HAND_R;
	}
	else if ( slot == EQS_HAND )
	{
		if ( m_EquipSlots[EQS_HAND_L] != ITEM_ERROR_ID )
		{
			m_EquipSlots[EQS_HAND_R] = m_EquipSlots[EQS_HAND_L];
			m_EquipSlots[EQS_HAND_L] = itemID;
			resultSlot = EQS_HAND_L;
		}
		else
		{
			m_EquipSlots[EQS_HAND_L] = itemID;
			resultSlot = EQS_HAND_L;
		}

	}
	else
	{
		m_EquipSlots[slot] = itemID;
		resultSlot = slot;
	}
	return resultSlot;
}

unsigned TEquipTable::removeEquip( EquipSlot slot )
{
	unsigned itemID = ITEM_ERROR_ID;

	itemID = m_EquipSlots[slot];
	m_EquipSlots[slot] = ITEM_ERROR_ID;

	return itemID;
}

TItemBase* UG_GetItem( unsigned itemID )
{
	return TItemManager::getInstance().getItem( itemID );
}

TItemBase* UG_GetItem( char const* itemName )
{
	unsigned id = TItemManager::getInstance().getItemID( itemName );
	if ( id == ITEM_ERROR_ID )
		return nullptr;
	return TItemManager::getInstance().getItem( id );
}

unsigned   UG_GetItemID( char const* itemName )
{
	return TItemManager::getInstance().getItemID( itemName );
}



void TItemBase::getDataInfo( ItemDataInfo& info )
{
#define ASSIGN_INFO( var ) info.##var = var

	info.id   = id;
	info.type = type;
	info.name = name;
	info.maxNumInSolt = maxNumInSolt;
	info.buyCost = buyCost;
	info.sellCost = sellCost;
	ASSIGN_INFO( cdTime );
	ASSIGN_INFO( helpStr );
	ASSIGN_INFO( iconID );
	info.PMVec = PMVec;

#undef ASSIGN_INFO
}

void TItemBase::modify( ItemDataInfo& info )
{
#define ASSIGN_INFO( var ) var = info.##var

	name = info.name;
	maxNumInSolt = info.maxNumInSolt;
	buyCost = info.buyCost;
	sellCost = info.sellCost;
	ASSIGN_INFO( cdTime );
	ASSIGN_INFO( helpStr );
	ASSIGN_INFO( iconID );
	PMVec = info.PMVec;

#undef ASSIGN_INFO
}

void TEquipment::getDataInfo( ItemDataInfo& info )
{
#define ASSIGN_INFO( var ) info.##var = var
	TItemBase::getDataInfo( info );
	info.slot = slot;
	info.equiVal = equiVal;
	ASSIGN_INFO( modelName );
	ASSIGN_INFO( scale );
	ASSIGN_INFO( pos );
	ASSIGN_INFO( rotation );

#undef ASSIGN_INFO
}

void TEquipment::modify( ItemDataInfo& info )
{
#define ASSIGN_INFO( var ) var = info.##var
	TItemBase::modify( info );
	slot    = info.slot;
	equiVal = info.equiVal;
	ASSIGN_INFO( modelName );
	ASSIGN_INFO( scale );
	ASSIGN_INFO( pos );
	ASSIGN_INFO( rotation );

#undef ASSIGN_INFO
}




void TWeapon::getDataInfo( ItemDataInfo& info )
{
#define ASSIGN_INFO( var )  info.##var = var
	TEquipment::getDataInfo( info );
	ASSIGN_INFO( weaponType );
#undef ASSIGN_INFO

}

void TWeapon::modify( ItemDataInfo& info )
{
#define ASSIGN_INFO( var ) var = info.##var
	TEquipment::modify( info );
	ASSIGN_INFO( weaponType );
#undef ASSIGN_INFO

}

void TArmour::getDataInfo( ItemDataInfo& info )
{
	TEquipment::getDataInfo( info );
}

void TArmour::modify( ItemDataInfo& info )
{
	TEquipment::modify( info );
}

size_t TArmour::createModel( CFObject* objList[] )
{
	size_t num = TEquipment::createModel( objList );

	if ( slot != EQS_SHOULDER )
		return num;

	EquipModelRes* res = static_cast< EquipModelRes* >( 
		TResManager::getInstance().getResData( RES_EQUIP_MODEL , getModelName() ) );

	CFObject* obj = res->model->instance( true );

	obj->scale( Vec3D(scale , scale , scale) , CFly::CFTO_LOCAL );
	obj->xform();
	Quat r = rotation;
	r.w = -r.w;
	obj->rotate( r , CFly::CFTO_LOCAL );
	obj->rotate( CFly::CF_AXIS_X , Math::Deg2Rad( 90 ) , CFly::CFTO_LOCAL );

	//obj->translate( Vec3D( pos.x , -pos.y , pos.z ) , CFly::CFTO_GLOBAL );

	objList[num++] = obj;
	return num;
}

size_t TEquipment::createModel( CFObject* objList[] )
{
	if ( modelName.empty() )
		return 0;

	EquipModelRes* res = static_cast< EquipModelRes* >( 
		TResManager::getInstance().getResData( RES_EQUIP_MODEL , getModelName() ) );

	if ( !res )
		return 0;

	CFObject* obj = res->model->instance( true );

	obj->scale( Vec3D(scale , scale , scale) , CFly::CFTO_LOCAL );
	obj->xform();

	Quat r = rotation;
	r.w = -r.w;

	obj->translate( pos , CFly::CFTO_GLOBAL );
	obj->rotate( r , CFly::CFTO_LOCAL );
	
	objList[0] = obj;

	return 1;
}

bool TEquipment::isShield()
{
	return type == ITEM_ARMOUR && slot == EQS_HAND;
}
