#ifndef DataCacheInterface_h__
#define DataCacheInterface_h__

#include "Template/ArrayView.h"
#include "Core/IntegerType.h"

#include "Serialize/StreamBuffer.h"
#include "Serialize/DataStream.h"

#include "Core/StringConv.h"
#include "InlineString.h"
#include "DataStructure/Array.h"

#include <functional>


using SerializeDelegate = std::function< bool (IStreamSerializer&) >;
struct DataCacheArg
{
	DataCacheArg();

	template< class ...Args >
	DataCacheArg(Args&& ...args)
	{
		add(std::forward<Args>(args)...);
	}

	void add(char const* str) { addString(str); }

	template< class T >
	void add(T&& t)
	{
		addString(FStringConv::From(std::forward<T>(t)));
	}

	template< class T , class ...Args >
	void add( T&& t , Args&& ...args)
	{
		add(std::forward<T>(t));
		addString("-");
		add(std::forward<Args>(args)...);
	}


	template< class ...Args >
	void addFormat(char const* format, Args&& ...args)
	{
		InlineString< 512 > str;
		str.format(format, std::forward<Args>(args)...);
		addString(str);
	}
	void addString(char const* str);

	void addFileAttribute(char const* filePath);
	uint64 value;
};

struct DataCacheKey
{
	char const* typeName;
	char const* version;
	DataCacheArg keySuffix;
};

using DataCacheHandle = uint32;

#define ERROR_DATA_CACHE_HANDLE DataCacheHandle(-1)

class DataCacheInterface
{
public:
	static DataCacheInterface* Create( char const* dir );

	virtual ~DataCacheInterface() {}
	virtual bool save(DataCacheKey const& key, TArrayView< uint8 > saveData) = 0;
	virtual bool saveDelegate(DataCacheKey const& key, SerializeDelegate inDelegate) = 0;
	virtual bool load(DataCacheKey const& key, TArray< uint8 >& outBuffer) = 0;
	virtual bool loadDelegate(DataCacheKey const& key, SerializeDelegate inDelegate) = 0;
	virtual void ignoreDataType(char const* typeName) = 0;

	virtual DataCacheHandle find(DataCacheKey const& key) = 0;
	virtual void release() = 0;

	template< class T >
	bool saveT(DataCacheKey const& key, T const& data)
	{
		return saveDelegate(key, [&data](IStreamSerializer& serializer)
		{
			serializer << data;
			return true;
		});
	}

	template< class T >
	bool loadT(DataCacheKey const& key, T& data)
	{
		return loadDelegate(key, [&data](IStreamSerializer& serializer)
		{
			serializer >> data;
			return true;
		});
	}

};

#endif // DataCacheInterface_h__