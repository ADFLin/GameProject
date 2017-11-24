#pragma once
#ifndef HashMap_H_6B89800E_5677_46BF_9058_F923AFFE8A74
#define HashMap_H_6B89800E_5677_46BF_9058_F923AFFE8A74

#include "Core/IntegerType.h"

typedef uint32 HashValueType;

template< class T >
HashValueType GetHashValue();

template< class KT , class VT >
class MapValue
{
	KT key;
	VT value;
};

template< class KT , class VT >
class THashMap
{
public:
	typedef MapValue<KT, VT> ValueType;

	ValueType** mHashTable;
	int mTableSize;

	void insert(KT const& key, VT const& value)
	{
		HashValueType hashValue = GetHashValue(key);

		HashValueType index = ( hashValue % mTableSize );

		if( mHashTable[index] == nullptr )
		{


		}

		while( mHashTable[index] )
		{
			if( equal(mHashTable[index]->key, key) )
			{

				return false;
			}
		}


	}

	bool equal(KT const& k1, KT const& k2)
	{

	}

};

#endif // HashMap_H_6B89800E_5677_46BF_9058_F923AFFE8A74