#ifndef IScriptTable_h__
#define IScriptTable_h__

#include "common.h"
enum ScriptValueType
{
	SVT_NIL    ,
	SVT_NUMBER ,
	SVT_FLOAT ,
	SVT_VEC3D ,
	SVT_STR   ,
	SVT_TABLE ,
};

struct ScriptAnyValue
{
	ScriptValueType type;
	union
	{
		double        number;
		char const*   strValue;
		struct { float x , y , z; } vec3;
		//IScriptTable* table;
	};

	ScriptAnyValue():type( SVT_NIL ){}
	ScriptAnyValue( int value ):type( SVT_NUMBER ),number( value ){}
	ScriptAnyValue( unsigned value ):type( SVT_NUMBER ),number( value ){}
	ScriptAnyValue( float value ):type( SVT_FLOAT ),number( value ){}
	ScriptAnyValue( char const* value ):type( SVT_STR ),strValue( value ){}

	ScriptAnyValue( int value , int ):type( SVT_NUMBER){}
	ScriptAnyValue( unsigned value , int ):type( SVT_NUMBER ){}
	ScriptAnyValue( float value , int):type( SVT_FLOAT ){}
	ScriptAnyValue( char const* value , int):type( SVT_STR ){}
	

	bool copyTo( int& value )        { if ( type == SVT_NUMBER ){ value = (int)number; return true; } return false; }
	bool copyTo( unsigned& value )   { if ( type == SVT_NUMBER ){ value = (unsigned)number; return true; } return false; }
	bool copyTo( char const*& value ){ if ( type == SVT_STR )   { value = strValue; return true; } return false; }
	bool copyTo( Vec3D& value )      { if ( type == SVT_VEC3D ) { value.setValue( vec3.x , vec3.y , vec3.z );  return true; } return false; }

	ScriptAnyValue& operator = ( ScriptAnyValue const& rhs )
	{
		type = rhs.type;
		switch( type )
		{
		case SVT_NUMBER: number   = rhs.number; break;
		case SVT_STR :   strValue = rhs.strValue; break;
		case SVT_VEC3D:  vec3.x   = rhs.vec3.x; vec3.y = rhs.vec3.y; vec3.z = rhs.vec3.z; break;
		}
		return *this;
	}
};

class ScriptKeyTable
{
public:
	unsigned addKey( char const* key )
	{
		KeyMap::iterator iter = mKeyMap.find( key );
		if ( iter != mKeyMap.end() )
			return iter->second;
		unsigned result = mNextID;
		mKeyMap.insert( std::make_pair( String( key ) , mNextID ) );
		return mNextID++;
	}
	bool     getKeyID( char const* key , unsigned& id )
	{
		KeyMap::iterator iter = mKeyMap.find( key );
		if ( iter != mKeyMap.end() )
		{
			id = iter->second;
			return true;
		}
		return false;
	}

private:
	typedef std::map< String , unsigned > KeyMap;

	KeyMap   mKeyMap;
	unsigned mNextID;
};

class IScriptTable
{
public:
	IScriptTable();
	~IScriptTable();

	void addRef(){ ++mRefCount; }
	void release();
	template< class T >
	bool setValue( char const* key , T const& value ){ return setValueAny( key , value );  }
	template< class T >
	bool getValue( char const* key , T& value )
	{ 
		ScriptAnyValue any( value , 0 );
		return getValueAny( key , any ) && any.copyTo( value );  
	}

	void beginGetSetChain();
	void endGetSetChain();
	template< class T >
	bool setValueChain( char const* key , T const& value , int& idxSlot )
	{ 
		return setValueAny( key , value , idxSlot );  
	}
	template< class T >
	bool getValueChain( char const* key , T& value , int& idxSlot ) const
	{ 
		ScriptAnyValue any( value , 0 );
		return getValueAny( key , any , idxSlot ) && any.copyTo( value );  
	}

	bool getValueAny( char const* key , ScriptAnyValue& value , int& idxSlot );
	bool setValueAny( char const* key , ScriptAnyValue const& value , int& idxSlot );
	bool getValueAny( char const* key , ScriptAnyValue& value );
	bool setValueAny( char const* key , ScriptAnyValue const& value );


	template< class T >
	void pushValue( char const* key , T const& value )
	{
		pushValueAny( key , value );
	}

	void pushValueAny( char const* key , ScriptAnyValue const& value );
	void setDefalutTable( IScriptTable* table );
	//size_t getValueCount();
	//void   setIndexValueAny( int index )
private:

	int getSlotIndex( char const* key , int idxSlot );

	ScriptKeyTable* mKeyTable;
	struct ValueSlot
	{
		unsigned keyID;
		ScriptAnyValue value;
	};

	typedef std::vector< ValueSlot > ValueSlotTable;
	typedef std::map< unsigned , unsigned >  ValueSlotMap;

	ScriptKeyTable& getKeyTable(){ return *mKeyTable; }
	struct SlotCmp
	{
		SlotCmp(ValueSlotTable& table):table( table ){}
		bool operator()( unsigned idx1 , unsigned idx2 ){ return table[idx1].keyID < table[idx2].keyID;  }
		ValueSlotTable& table;
	};

	bool assignSlotValue( ValueSlot& slot , ScriptAnyValue const& value );
	ValueSlotTable mSlotTable;
	ValueSlotMap   mSlotMap;
	unsigned       mNextSlotIndex;
	IScriptTable*  mDefaultTable;
	int            mRefCount;
	friend class ScriptSystem;
};

class ScriptSystem
{
public:
	IScriptTable* createScriptTable();
private:
	ScriptKeyTable mKeyTable;
};

#endif // IScriptTable_h__

