#pragma once
#ifndef PropertySet_H_F7B8B66E_A07B_44DF_BA67_8895369C3C8E
#define PropertySet_H_F7B8B66E_A07B_44DF_BA67_8895369C3C8E

#include "Template/StringView.h"

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
	int  mSequenceOrder = -1;
	std::string mValue;
};


class  KeySection
{
public:
	KeyValue* getKeyValue( char const* keyName );

	template< class T >
	KeyValue* addKeyValue( char const* keyName , T value )
	{
		auto& keyValue = mKeyMap[keyName];
		keyValue = KeyValue(value);
		return &keyValue;
	}
	void serializtion( std::ostream& os );
protected:

	struct StrCmp
	{
		bool operator()( char const* s1 , char const* s2 ) const
		{
			return ::strcmp( s1 , s2 ) < 0;
		}
	};
	using KeyValueMap = std::unordered_map< std::string , KeyValue >;

	friend class PropertySet;
	int  mSequenceOrder = INDEX_NONE;
	KeyValueMap mKeyMap;
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

	template< class T >
	void setKeyValue( char const* keyName , char const* section , T value )
	{
		auto& sectionData = mSectionMap[ section ];
		if(sectionData.mSequenceOrder == INDEX_NONE )
		{
			sectionData.mSequenceOrder = mNextSectionSeqOrder;
			++mNextSectionSeqOrder;
		}
		KeyValue* keyValue = sectionData.addKeyValue( keyName , value );
		if( keyValue && keyValue->mSequenceOrder == INDEX_NONE )
		{
			keyValue->mSequenceOrder = mNextValueSeqOrder;
			++mNextValueSeqOrder;
		}
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
private:
	template< class T >
	T     getValueT( char const* keyName , char const* section , T defaultValue );
	template< class T >
	bool  tryGetValueT( char const* keyName , char const* section , T& value );

	int   parseLine( char* buffer , KeySection** curSection );

	using KeySectionMap = std::unordered_map< std::string , KeySection >;
	KeySectionMap mSectionMap;

	int mNextSectionSeqOrder;
	int mNextValueSeqOrder;
};

#endif // PropertySet_H_F7B8B66E_A07B_44DF_BA67_8895369C3C8E
