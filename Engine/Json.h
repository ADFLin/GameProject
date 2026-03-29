#pragma once
#ifndef Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769
#define Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769

#include "DataStructure/Array.h"
#include <string>
#include <type_traits>

#include "ReflectionCollect.h"
#include "Core/StringConv.h"

template<typename T, typename = void>
struct TIsArrayLike : std::false_type {};

template<typename T>
struct TIsArrayLike<T, std::void_t<typename T::value_type, decltype(std::declval<T>().size()), decltype(std::declval<T>().resize(0))>> 
	: std::integral_constant<bool, !std::is_same_v<T, std::string>> {};

template<typename T, typename = void>
struct TIsMapLike : std::false_type {};

template<typename T>
struct TIsMapLike<T, std::void_t<typename T::key_type, typename T::mapped_type, decltype(std::declval<T>().size())>> 
	: std::true_type {};

enum class EJsonType : uint8
{
	Null,
	Object,
	Array,
	String,
	Bool,
	NumberInt,
	NumberUInt,
	NumberFloat,
};

class JsonObject;
class JsonArray
{
public:
	bool isVaild() const { return !!mPtr;}
	size_t size() const;
	void resize(size_t sz);

	bool tryGet(size_t index, std::string& outValue) const;
	bool tryGet(size_t index, int& outValue) const;
	bool tryGet(size_t index, float& outValue) const;
	bool tryGet(size_t index, bool& outValue) const;

	void set(size_t index, std::string const& value);
	void set(size_t index, int value);
	void set(size_t index, float value);
	void set(size_t index, bool value);
	void set(size_t index, char const* value) { set(index, std::string(value)); }

	JsonObject getObject(size_t index);
	JsonObject getOrAddObject(size_t index);

	void* mPtr = nullptr;
};

class JsonValue
{
public:
	bool isObject() const { return type == EJsonType::Object; }
	JsonObject asObject() const;


	EJsonType type = EJsonType::Null;
	void* ptr = nullptr;
};

class JsonObject
{
public:

	bool isVaild() const { return !!mPtr;}

	bool tryGet(char const* key, std::string& outValue) const;
	bool tryGet(char const* key, int& outValue) const;
	bool tryGet(char const* key, float& outValue) const;
	bool tryGet(char const* key, bool& outValue) const;

	void set(char const* key, std::string const& value);
	void set(char const* key, int value);
	void set(char const* key, float value);
	void set(char const* key, bool value);
	void set(char const* key, char const* value) { set(key, std::string(value)); }

	std::string getString(char const* key, std::string const& defaultValue) const { return getValue(key, defaultValue); }
	int         getInt(char const* key, int defaultValue) const { return getValue(key, defaultValue); }
	float       getFloat(char const* key, float defaultValue) const { return getValue(key, defaultValue); }
	bool        getBool(char const* key, bool defaultValue) const { return getValue(key, defaultValue); }

	JsonObject getObject(char const* key);
	TArray<JsonValue> getArray(char const* key);

	JsonObject getOrAddObject(char const* key);
	JsonObject getOrAddObject(std::string const& key) { return getOrAddObject(key.c_str()); }

	JsonArray getJsonArray(char const* key) const;
	JsonArray getOrAddArray(char const* key);

	// Dictionary access support for Maps
	template<typename T>
	bool tryGetDict(char const* key, T& outValue) const { return tryGet(key, outValue); }
	template<typename T>
	bool tryGetDict(std::string const& key, T& outValue) const { return tryGet(key.c_str(), outValue); }

	template<typename T>
	void setDict(char const* key, T const& value) { set(key, value); }
	template<typename T>
	void setDict(std::string const& key, T const& value) { set(key.c_str(), value); }
	
	// Map iteration Support 
	std::vector<std::string> getKeys() const;

	template< typename T>
	T getValue(char const* key, T defaultValue) const
	{
		T outValue;
		if (tryGet(key, outValue))
			return outValue;

		return defaultValue;
	}
	void* mPtr = nullptr;

};

class JsonFile
{
public:
	virtual bool isObject() = 0;
	virtual bool isArray() = 0;
	virtual JsonObject getObject() = 0;
	virtual TArray<JsonValue> getArray() = 0;

	virtual std::string toString() = 0;

	virtual void release() = 0;
	static JsonFile* Load(char const* path);
	static JsonFile* Create();
};

class JsonSerializer;

template< typename T >
class TJsonSerializeCollector;

namespace JsonDetail
{
	template<typename KeyType>
	inline KeyType FromString(std::string const& str)
	{
		if constexpr (std::is_same_v<KeyType, std::string>)
		{
			return str;
		}
		else if constexpr (std::is_enum_v<KeyType>)
		{
			using IntType = std::underlying_type_t<KeyType>;
			return static_cast<KeyType>(FStringConv::To<IntType>(str.c_str(), (int)str.length()));
		}
		else
		{
			return FStringConv::To<KeyType>(str.c_str(), (int)str.length());
		}
	}

	template<typename KeyType>
	inline std::string ToString(KeyType const& key)
	{
		if constexpr (std::is_same_v<KeyType, std::string>)
		{
			return key;
		}
		else if constexpr (std::is_enum_v<KeyType>)
		{
			using IntType = std::underlying_type_t<KeyType>;
			return FStringConv::From(static_cast<IntType>(key));
		}
		else
		{
			return FStringConv::From(key);
		}
	}
}

class JsonSerializer
{
public:
	JsonSerializer(JsonObject object, bool bLoading)
		: mObject(object), mbLoading(bLoading)
	{
	}

	bool isReading() const { return mbLoading; }
	bool isWriting() const { return !mbLoading; }


	template< typename T>
	void serialize(char const* key, T& value)
	{
		if constexpr (TIsMapLike<T>::value)
		{
			readWriteMap(key, value);
		}
		else if constexpr (TIsArrayLike<T>::value)
		{
			readWriteArray(key, value);
		}
		else if constexpr (IsRefectableObject<T> || (std::is_class_v<T> && !std::is_same_v<T, std::string>))
		{
			readWriteObject(key, value);
		}
		else if constexpr (std::is_enum_v<T>)
		{
			using IntType = std::underlying_type_t<T>;
			readWrite(key, *reinterpret_cast<IntType*>(&value));
		}
		else
		{
			readWrite(key, value);
		}
	}

	template< typename T >
	void readWrite(char const* key, T& value)
	{
		if (mbLoading)
		{
			mObject.tryGet(key, value);
		}
		else
		{
			mObject.set(key, value);
		}	
	}

	template< typename T >
	void readWriteObject(char const* key, T& value)
	{
		if (mbLoading)
		{
			JsonObject childObj = mObject.getObject(key);
			if (childObj.isVaild())
			{
				JsonSerializer childSerializer(childObj, true);
				if constexpr (IsRefectableObject<T>)
				{
					TJsonSerializeCollector<T> collector(childSerializer, value);
					T::CollectReflection(collector);
				}
				else
				{
					value.serialize(childSerializer);
				}
			}
		}
		else
		{
			JsonObject childObj = mObject.getOrAddObject(key);
			if (childObj.isVaild())
			{
				JsonSerializer childSerializer(childObj, false);
				if constexpr (IsRefectableObject<T>)
				{
					TJsonSerializeCollector<T> collector(childSerializer, value);
					T::CollectReflection(collector);
				}
				else
				{
					value.serialize(childSerializer);
				}
			}
		}
	}

	template< typename TArrayLike >
	void readWriteArray(char const* key, TArrayLike& arr)
	{
		using ElementType = typename TArrayLike::value_type;

		if (mbLoading)
		{
			JsonArray childArr = mObject.getJsonArray(key);
			if (childArr.isVaild())
			{
				size_t count = childArr.size();
				arr.resize(count);
				for (size_t i = 0; i < count; ++i)
				{
					if constexpr (IsRefectableObject<ElementType>)
					{
						JsonObject childObj = childArr.getObject(i);
						if (childObj.isVaild())
						{
							JsonSerializer childSerializer(childObj, true);
							TJsonSerializeCollector<ElementType> collector(childSerializer, arr[i]);
							ElementType::CollectReflection(collector);
						}
					}
					else if constexpr (std::is_class_v<ElementType> && !std::is_same_v<ElementType, std::string>)
					{
						JsonObject childObj = childArr.getObject(i);
						if (childObj.isVaild())
						{
							JsonSerializer childSerializer(childObj, true);
							arr[i].serialize(childSerializer);
						}
					}
					else
					{
						childArr.tryGet(i, arr[i]);
					}
				}
			}
		}
		else
		{
			JsonArray childArr = mObject.getOrAddArray(key);
			if (childArr.isVaild())
			{
				size_t count = arr.size();
				childArr.resize(count);
				for (size_t i = 0; i < count; ++i)
				{
					if constexpr (IsRefectableObject<ElementType>)
					{
						JsonObject childObj = childArr.getOrAddObject(i);
						if (childObj.isVaild())
						{
							JsonSerializer childSerializer(childObj, false);
							TJsonSerializeCollector<ElementType> collector(childSerializer, arr[i]);
							ElementType::CollectReflection(collector);
						}
					}
					else if constexpr (std::is_class_v<ElementType> && !std::is_same_v<ElementType, std::string>)
					{
						JsonObject childObj = childArr.getOrAddObject(i);
						if (childObj.isVaild())
						{
							JsonSerializer childSerializer(childObj, false);
							arr[i].serialize(childSerializer);
						}
					}
					else
					{
						childArr.set(i, arr[i]);
					}
				}
			}
		}
	}

	template< typename TMapLike >
	void readWriteMap(char const* key, TMapLike& map)
	{
		using KeyType = typename TMapLike::key_type;
		using ValueType = typename TMapLike::mapped_type;

		if (mbLoading)
		{
			JsonObject childObj = mObject.getObject(key);
			if (childObj.isVaild())
			{
				map.clear();
				for (auto const& k_str : childObj.getKeys())
				{
					KeyType k = JsonDetail::FromString<KeyType>(k_str);
					ValueType& val = map[k];
					if constexpr (IsRefectableObject<ValueType>)
					{
						JsonObject nestedObj = childObj.getObject(k_str.c_str());
						if (nestedObj.isVaild())
						{
							JsonSerializer childSerializer(nestedObj, true);
							TJsonSerializeCollector<ValueType> collector(childSerializer, val);
							ValueType::CollectReflection(collector);
						}
					}
					else if constexpr (std::is_class_v<ValueType> && !std::is_same_v<ValueType, std::string>)
					{
						JsonObject nestedObj = childObj.getObject(k_str.c_str());
						if (nestedObj.isVaild())
						{
							JsonSerializer childSerializer(nestedObj, true);
							val.serialize(childSerializer);
						}
					}
					else
					{
						childObj.tryGetDict(k_str, val);
					}
				}
			}
		}
		else
		{
			JsonObject childObj = mObject.getOrAddObject(key);
			if (childObj.isVaild())
			{
				for (auto& pair : map)
				{
					std::string k_str = JsonDetail::ToString(pair.first);
					auto& val = pair.second;
					
					if constexpr (IsRefectableObject<ValueType>)
					{
						JsonObject nestedObj = childObj.getOrAddObject(k_str);
						if (nestedObj.isVaild())
						{
							JsonSerializer childSerializer(nestedObj, false);
							TJsonSerializeCollector<ValueType> collector(childSerializer, val);
							ValueType::CollectReflection(collector);
						}
					}
					else if constexpr (std::is_class_v<ValueType> && !std::is_same_v<ValueType, std::string>)
					{
						JsonObject nestedObj = childObj.getOrAddObject(k_str);
						if (nestedObj.isVaild())
						{
							JsonSerializer childSerializer(nestedObj, false);
							val.serialize(childSerializer);
						}
					}
					else
					{
						childObj.setDict(k_str, val);
					}
				}
			}
		}
	}

private:
	JsonObject mObject;
	bool mbLoading;
};


template< typename T >
class TJsonSerializeCollector : public ReflectionCollector
{
public:
	TJsonSerializeCollector(JsonSerializer& serializer, T& instance)
		: mSerializer(serializer), mInstance(instance)
	{
	}

	template< typename ClassType, typename TBase >
	void addBaseClass()
	{
		REF_COLLECT_TYPE(TBase, *this);
	}

	template< typename ClassType, typename P >
	void addProperty(P(ClassType::*memberPtr), char const* name)
	{
		mSerializer.serialize(name, mInstance.*memberPtr);
	}

	template< typename ClassType, typename P, typename ...TMeta >
	void addProperty(P(ClassType::*memberPtr), char const* name, TMeta&& ...meta)
	{
		mSerializer.serialize(name, mInstance.*memberPtr);
	}

private:
	JsonSerializer& mSerializer;
	T& mInstance;
};

#endif // Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769