#ifndef PropertyKey_h__
#define PropertyKey_h__


#include <string>
#include <sstream>
#include <fstream>
#include <map>

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
	char const* getString() const { return mValue.c_str();  }
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
	union SaveValue
	{
		int   intValue;
		float floatValue;
	};
	mutable SaveValue mCacheGetValue;
	std::string mValue;
};


class  KeySection
{
public:
	KeyValue* getKeyValue( char const* keyName );

	template< class T >
	bool     addKeyValue( char const* keyName , T value )
	{
		mKeyMap[ keyName ] = KeyValue( value );
		return true;
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
	typedef std::map< std::string , KeyValue > KeyValueMap;
	KeyValueMap mKeyMap;
};


class  PropertyKey
{
public:
	KeyValue*   getKeyValue( char const* keyName , char const* group );

	char        getCharValue  ( char const* keyName , char const* group , char defaultValue );
	int         getIntValue   ( char const* keyName , char const* group , int defaultValue );
	float       getFloatValue ( char const* keyName , char const* group , float defaultValue );
	char const* getStringValue( char const* keyName , char const* group , char const* defaultValue );

	bool        tryGetCharValue  ( char const* keyName , char const* group , char& value );
	bool        tryGetIntValue   ( char const* keyName , char const* group , int& value );
	bool        tryGetFloatValue ( char const* keyName , char const* group , float& value );
	bool        tryGetStringValue( char const* keyName , char const* group , char const*& value );

	bool        saveFile( char const* path );
	bool        loadFile( char const* path );

	template< class T >
	void setKeyValue( char const* keyName , char const* group , T value )
	{
		mGourpMap[ group ].addKeyValue( keyName , value );
	}

private:
	template< class T >
	T     getValueT( char const* keyName , char const* group , T defaultValue );
	template< class T >
	bool  tryGetValueT( char const* keyName , char const* group , T& value );

	int   parseLine( char* buffer , KeySection** curSection );
	typedef std::map< std::string , KeySection > KeySectionMap;
	KeySectionMap mGourpMap;


};


#endif // PropertyKey_h__