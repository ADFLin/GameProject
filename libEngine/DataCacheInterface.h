#ifndef DataCacheInterface_h__
#define DataCacheInterface_h__

#include "Template/ArrayView.h"
#include "Core/IntegerType.h"

#include "Serialize/StreamBuffer.h"
#include "Serialize/DataStream.h"

#include "Core/StringConv.h"

#include <vector>
#include <functional>

typedef std::function< bool (IStreamSerializer&) > SerializeDelegate;
struct DataCacheArg
{
	DataCacheArg();
	void add(char const* str);

	template< class T , class ...Args >
	void addArgs( T t , Args&& ...args)
	{
		add(FStringConv::From(t));
		add("-");
		addArgs(std::forward<Args>(args)...);
	}

	template< class T >
	void addArgs(T t)
	{
		add(FStringConv::From(t));
	}

	template< class ...Args >
	void addFormat(char const* format, Args&& ...args)
	{
		FixString< 512 > str;
		str.format(format, std::forward<Args>(args)...);
		add(str);
	}

	uint64 value;
};

struct DataCacheKey
{
	char const* typeName;
	char const* version;
	DataCacheArg keySuffix;
};

typedef uint32 DataCacheHandle;

#define ERROR_DATA_CACHE_HANDLE DataCacheHandle(-1)

class DataCacheInterface
{
public:
	static DataCacheInterface* Create( char const* dir );

	virtual ~DataCacheInterface() {}
	virtual bool save(DataCacheKey const& key, TArrayView< uint8 > saveData) = 0;
	virtual bool saveDelegate(DataCacheKey const& key, SerializeDelegate inDelegate) = 0;
	virtual bool load(DataCacheKey const& key, std::vector< uint8 >& outBuffer) = 0;
	virtual bool loadDelegate(DataCacheKey const& key, SerializeDelegate inDelegate) = 0;

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