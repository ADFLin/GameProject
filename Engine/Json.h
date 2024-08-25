#pragma once
#ifndef Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769
#define Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769

#include "DataStructure/Array.h"
#include <string>


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

	std::string getString(char const* key, std::string const& defaultValue) const { return getValue(key, defaultValue); }
	int         getInt(char const* key, int defaultValue) const { return getValue(key, defaultValue); }
	float       getFloat(char const* key, float defaultValue) const { return getValue(key, defaultValue); }
	bool        getBool(char const* key, bool defaultValue) const { return getValue(key, defaultValue); }

	JsonObject getObject(char const* key);
	TArray<JsonValue> getArray(char const* key);


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

	virtual void release() = 0;
	static JsonFile* Load(char const* path);
};


#endif // Json_H_C6F5228B_5BA8_421D_81FD_2BAD0BCEA769