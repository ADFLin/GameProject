#pragma once
#ifndef PropertySet_H_F7B8B66E_A07B_44DF_BA67_8895369C3C8E
#define PropertySet_H_F7B8B66E_A07B_44DF_BA67_8895369C3C8E

#include "Template/StringView.h"
#include "DataStructure/Array.h"

#include <string>
#include <unordered_map>


class  KeyValue
{
public:
	KeyValue(){}
	KeyValue( char const* value ):mValue( value ){}
	KeyValue( int value ){ setInt( value ); }
	KeyValue( float value ){ setFloat( value ); }
	KeyValue( char  value ){ setChar( value ); }

	char        getChar() const { return mValue[0]; }
	float       getFloat() const;
	int         getInt() const ;
	char const* getString() const { return mValue.c_str(); }
	StringView  getStringView() const { return StringView(mValue.data(), mValue.size()); }
	bool        getBool() const;
	void        setFloat( float value );
	void        setInt( int value );
	void        setChar(char value);
	void        setString(char const* value);

private:
	enum 
	{
		NoCache ,
		CacheInt ,
		CacheFloat ,
	};

	mutable int mCacheGetType = NoCache;
	union SavedValue
	{
		int   intValue;
		float floatValue;
	};

	mutable SavedValue mCacheGetValue;
	friend class KeySection;
	friend class PropertySet;
	int  mSequenceOrder = INDEX_NONE;
	std::string mValue;
};


class  KeySection
{
public:
	KeyValue* getKeyValue( char const* keyName );


	TArray<KeyValue*> getKeyValues(char const* keyName);

	template< class T >
	KeyValue* setKeyValue(char const* keyName, T&& value)
	{
		mKeyMap.erase(keyName);
		return &mKeyMap.emplace(keyName, std::forward<T>(value))->second;
	}


	template< class T >
	KeyValue* addKeyValue( char const* keyName , T&& value )
	{
		return &mKeyMap.emplace(keyName, std::forward<T>(value))->second;
	}

	bool removeKeyValue(char const* keyName, char const* value)
	{
		auto iterPair = mKeyMap.equal_range(keyName);
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			if (FCString::Compare(iter->second.getString(), value) == 0)
			{
				mKeyMap.erase(iter);
				return true;
			}
		}
		return false;
	}

	void removeKey(char const* keyName)
	{
		mKeyMap.erase(keyName);
	}

protected:

	struct StrCmp
	{
		bool operator()( char const* s1 , char const* s2 ) const
		{
			return ::strcmp( s1 , s2 ) < 0;
		}
	};
	using KeyValueMap = std::unordered_multimap< std::string , KeyValue >;

	friend class PropertySet;
	int  mSequenceOrder = INDEX_NONE;
	KeyValueMap mKeyMap;
};


class PropertyFile
{
public:
	enum class EValueOp
	{
		Set ,
		Add ,
		Remove,
		RemoveAll,
	};

	struct Element
	{
		EValueOp    op;
		std::string key;
		std::string value;
	};

	struct Section
	{
		std::string key;
		TArray< Element > elements;
	};

	void clear()
	{
		mGlobalElements.clear();
		mSections.clear();
	}

	TArray< Element > mGlobalElements;
	TArray< Section > mSections;

	bool save(char const* path);
	bool load(char const* path);

	int parseLine(char* buffer, TArray< Element >*& sectionElementsPtr);


	struct FindResult
	{
		TArray< Element >* firstElements = nullptr;
		Element* element = nullptr;
	};

	enum class EVisitOp
	{
		Keep,
		Stop,
		Remove,
		RemoveStop,
	};
	template< typename TFunc >
	void visit(char const* sectionName, char const* keyName, TFunc&& func);

	void setKeyValue(char const* section, char const* keyName, char const* value);
};

class  PropertySet
{
public:
	PropertySet();
	KeyValue*   getKeyValue( char const* keyName , char const* section );

	char        getCharValue  ( char const* keyName , char const* section , char defaultValue );
	int         getIntValue   ( char const* keyName , char const* section , int defaultValue );
	float       getFloatValue ( char const* keyName , char const* section , float defaultValue );
	char const* getStringValue( char const* keyName , char const* section , char const* defaultValue );
	bool        getBoolValue(char const* keyName, char const* section, bool defaultValue);

	bool        tryGetCharValue  ( char const* keyName , char const* section , char& value );
	bool        tryGetIntValue   ( char const* keyName , char const* section , int& value );
	bool        tryGetFloatValue ( char const* keyName , char const* section , float& value );
	bool        tryGetStringValue( char const* keyName , char const* section , char const*& value );
	bool        tryGetBoolValue(char const* keyName, char const* section, bool& value);

	bool        saveFile( char const* path );
	bool        loadFile( char const* path );

	void        getStringValues(char const* keyName, char const* section, TArray< char const* >& outValue)
	{


	}
	void        setStringValues(char const* keyName, char const* section, TArray< char const* > const&& values)
	{

	}

	template< class T >
	void setKeyValue( char const* keyName , char const* section , T&& value )
	{
		auto& sectionData = mSectionMap[section];
		sectionData.setKeyValue( keyName , std::forward<T>(value) );
		mFile.setKeyValue(section, keyName, FStringConv::From(value));
	}

	void setKeyValue(char const* keyName, char const* section, char const* value)
	{
		auto& sectionData = mSectionMap[section];
		sectionData.setKeyValue(keyName, value);
		mFile.setKeyValue(section, keyName, value);
	}

	template< class TFunc >
	void visitSection( char const* section , TFunc&& func )
	{
		auto iter = mSectionMap.find(section);
		if (iter != mSectionMap.end())
		{
			for (auto& pair : iter->second.mKeyMap)
			{
				func(pair.first.c_str(), pair.second);
			}
		}
	}

	void append(PropertyFile& file);
private:
	template< class T >
	T     getValueT( char const* keyName , char const* section , T defaultValue );
	template< class T >
	bool  tryGetValueT( char const* keyName , char const* section , T& value );


	using KeySectionMap = std::unordered_map< std::string , KeySection >;
	KeySectionMap mSectionMap;
	PropertyFile mFile;

	int mNextSectionSeqOrder;
	int mNextValueSeqOrder;
};

#endif // PropertySet_H_F7B8B66E_A07B_44DF_BA67_8895369C3C8E
