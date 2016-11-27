#include "ProjectPCH.h"
#include "IScriptTable.h"

IScriptTable::IScriptTable()
	:mKeyTable(NULL)
	,mNextSlotIndex(0)
{
	mDefaultTable = NULL;
	mRefCount = 0;
}

IScriptTable::~IScriptTable()
{
	if ( mDefaultTable )
		mDefaultTable->release();
}

bool IScriptTable::getValueAny( char const* key , ScriptAnyValue& value , int& idxSlot )
{
	int slotIndex = getSlotIndex( key , idxSlot );
	if ( slotIndex == -1 )
	{
		if ( !mDefaultTable )
			return false;

		return mDefaultTable->getValueAny( key , value , idxSlot );
	}
	idxSlot = slotIndex;
	value = mSlotTable[ slotIndex ].value;
	return true;
}

bool IScriptTable::setValueAny( char const* key , ScriptAnyValue const& value , int& idxSlot )
{
	int slotIndex = getSlotIndex( key , idxSlot );
	if ( slotIndex == -1 )
		return false;
	idxSlot = slotIndex;
	return assignSlotValue( mSlotTable[ slotIndex ] , value );
}

bool IScriptTable::getValueAny( char const* key , ScriptAnyValue& value  )
{
	int slotIndex = getSlotIndex( key , -1 );
	if ( slotIndex == -1 )
	{
		if ( !mDefaultTable )
			return false;
		int slot;
		return mDefaultTable->getValueAny( key , value , slot );
	}
	value = mSlotTable[ slotIndex ].value;
	return true;
}

bool IScriptTable::setValueAny( char const* key , ScriptAnyValue const& value )
{
	int slotIndex = getSlotIndex( key , -1 );
	if ( slotIndex == -1 )
		return false;
	return assignSlotValue( mSlotTable[ slotIndex ] , value );
}

void IScriptTable::pushValueAny( char const* key , ScriptAnyValue const& value )
{
	ValueSlot slot;
	slot.keyID = getKeyTable().addKey( key );
	mSlotTable.push_back( slot );

	assignSlotValue( mSlotTable.back() , value );
	mSlotMap[ slot.keyID ] = mSlotTable.size() - 1;
}

int IScriptTable::getSlotIndex( char const* key , int idxSlot )
{
	unsigned keyID;
	if ( !getKeyTable().getKeyID( key , keyID ) ) 
		return -1;

	if ( idxSlot != -1 && idxSlot < (int)mSlotTable.size() )
	{
		if ( mSlotTable[ idxSlot ].keyID == keyID )
			return idxSlot;
	}

	ValueSlotMap::iterator iter = mSlotMap.find( keyID );
	if ( iter == mSlotMap.end() )
		return -1;

	return (int)iter->second;
}

bool IScriptTable::assignSlotValue( ValueSlot& slot , ScriptAnyValue const& value )
{
	if ( slot.value.type == SVT_NIL )
	{
		slot.value = value;
		if ( slot.value.type == SVT_STR )
			slot.value.strValue = NULL;
		else 
			return true;
	}

	if ( slot.value.type == SVT_STR )
	{
		if ( value.type != SVT_STR )
			return false;

		if ( slot.value.strValue )
			::free( (void*)slot.value.strValue );

		int len = strlen( value.strValue ) + 1;
		char* allocStr = (char*)::malloc( len );
		strcpy( allocStr , value.strValue );
		slot.value.strValue = allocStr;
		return true;
	}

	if ( slot.value.type != value.type )
		return false;

	slot.value = value;
	return true;
}

void IScriptTable::beginGetSetChain()
{
	mNextSlotIndex = 0;
}

void IScriptTable::endGetSetChain()
{

}

void IScriptTable::release()
{
	--mRefCount; 
	if ( mRefCount == 0 )
		delete this;
}

void IScriptTable::setDefalutTable( IScriptTable* table )
{
	if ( mDefaultTable )
		mDefaultTable->release();

	mDefaultTable = table;  

	if ( mDefaultTable )
		mDefaultTable->addRef();
}

IScriptTable* ScriptSystem::createScriptTable()
{
	IScriptTable* table = new IScriptTable;
	table->mKeyTable = &mKeyTable;
	table->addRef();
	return table;
}
