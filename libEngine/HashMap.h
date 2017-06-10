



template< class KT , class VT >
class MapValue
{
	KT key;
	VT value;
};

template< class KT , class VT >
class THashMap
{


	typedef MapValue<KT, VT> ValueType;

	ValueType** mHashTable;
	int mTableSize;

	void insert(KT const& key, VT const& value)
	{


	}

};