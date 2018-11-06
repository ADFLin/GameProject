#include "TinyGamePCH.h"
#include "PropertyKey.h"


#define GLOBAL_SECTION "__GLOBAL__"

float KeyValue::getFloat() const
{
	if( mCacheGetType != CacheFloat )
	{
		mCacheGetValue.floatValue = float(::atof(mValue.c_str()));
		mCacheGetType = CacheFloat;
	}
	return mCacheGetValue.floatValue;
}

int KeyValue::getInt() const
{
	if( mCacheGetType != CacheInt )
	{
		mCacheGetValue.intValue = ::atoi(mValue.c_str());
		mCacheGetType = CacheInt;
	}
	return mCacheGetValue.intValue;
}

bool KeyValue::getBool() const
{
	if( mCacheGetType != CacheInt )
	{
		if( FCString::CompareIgnoreCase(mValue.c_str(), "true") == 0 )
			mCacheGetValue.intValue = 1;
		else if( FCString::CompareIgnoreCase(mValue.c_str(), "true") == 0 )
			mCacheGetValue.intValue = 1;
		else if( FCString::CompareIgnoreCase(mValue.c_str(), "t") == 0 )
			mCacheGetValue.intValue = 1;
		else if( FCString::CompareIgnoreCase(mValue.c_str(), "f") == 0 )
			mCacheGetValue.intValue = 1;
		else 
			mCacheGetValue.intValue = ::atoi(mValue.c_str());
		mCacheGetType = CacheInt;
	}
	return mCacheGetValue.intValue;
}

void  KeyValue::setFloat( float value )
{ 
	std::stringstream ss;
	ss << value;
	mValue = ss.str();
	mCacheGetType = CacheFloat;
	mCacheGetValue.floatValue = value;
}

void KeyValue::setInt( int value )
{
	std::stringstream ss;
	ss << value;
	mValue = ss.str();
	mCacheGetType = CacheInt;
	mCacheGetValue.intValue = value;
}

void KeyValue::setChar(char value)
{
	mValue.clear();
	mValue.push_back(value);
	mCacheGetType = NoCache;
}

void KeyValue::setString(char const* value)
{
	mValue = value;
	mCacheGetType = NoCache;
}

KeyValue* KeySection::getKeyValue( char const* keyName )
{
	KeyValueMap::iterator iter = mKeyMap.find( keyName );
	if ( iter != mKeyMap.end() )
		return &iter->second;
	return NULL;
}

void KeySection::serializtion( std::ostream& os )
{
	for( KeyValueMap::iterator iter = mKeyMap.begin();
		iter != mKeyMap.end() ; ++iter )
	{
		os << iter->first << " = " << iter->second.getString() << '\n'; 
	}
}

KeyValue* PropertyKey::getKeyValue( char const* group, char const* keyName )
{
	if ( !group )
		group = GLOBAL_SECTION;

	KeySectionMap::iterator iter = mGourpMap.find( group );
	if ( iter == mGourpMap.end() )
		return NULL;

	return iter->second.getKeyValue( keyName );
}

template< class T > struct KeyOp {};
template<>  struct KeyOp< int >  { static int   Get( KeyValue const* value ){ return value->getInt(); }  }; 
template<>  struct KeyOp< float >{ static  float Get( KeyValue const* value ){ return value->getFloat(); }  };
template<>  struct KeyOp< char > { static  char  Get( KeyValue const* value ){ return value->getChar(); }  };
template<>  struct KeyOp< char const* >{ static char const* Get( KeyValue const* value ){ return value->getString(); } };

template< class T >
bool PropertyKey::tryGetValueT( char const* keyName , char const* group , T& value )
{
	KeyValue const* keyValue = getKeyValue( group, keyName );
	if ( !keyValue )
		return false;

	value = KeyOp<T>::Get( keyValue );
	return true;
}

template< class T >
T PropertyKey::getValueT( char const* keyName , char const* group , T defaultValue )
{
	if( !group )
		group = GLOBAL_SECTION;

	KeyValue const* keyValue = getKeyValue( group, keyName );
	if ( keyValue )
	{
		return KeyOp<T>::Get( keyValue );
	}
	setKeyValue( keyName , group , defaultValue );
	return defaultValue;
}

bool PropertyKey::tryGetCharValue( char const* keyName , char const* group , char& value )
{
	return tryGetValueT( keyName , group , value );
}

bool PropertyKey::tryGetIntValue( char const* keyName , char const* group , int& value )
{
	return tryGetValueT( keyName , group , value );
}

bool PropertyKey::tryGetFloatValue( char const* keyName , char const* group , float& value )
{
	return tryGetValueT( keyName , group , value );
}

bool PropertyKey::tryGetStringValue( char const* keyName , char const* group , char const*& value )
{
	return tryGetValueT( keyName , group , value );
}

char PropertyKey::getCharValue( char const* keyName , char const* group , char defaultValue )
{
	return getValueT( keyName , group , defaultValue );
}

int PropertyKey::getIntValue( char const* keyName , char const* group , int defaultValue )
{
	return getValueT( keyName , group , defaultValue );
}

float PropertyKey::getFloatValue( char const* keyName , char const* group , float defaultValue )
{
	return getValueT( keyName , group , defaultValue );
}

char const* PropertyKey::getStringValue( char const* keyName , char const* group , char const* defaultValue )
{
	return getValueT( keyName , group , defaultValue );
}

bool PropertyKey::saveFile( char const* path )
{
	std::ofstream fs( path );

	if ( !fs )
		return false;

	KeySectionMap::iterator globalIter = mGourpMap.find( GLOBAL_SECTION );

	if( globalIter != mGourpMap.end() )
	{
		globalIter->second.serializtion(fs);
		fs << "\n";
	}

	for( KeySectionMap::iterator iter = mGourpMap.begin();
		iter != mGourpMap.end() ; ++iter )
	{
		if ( iter == globalIter )
			continue;

		fs << "[" << iter->first << "]" << "\n";
		iter->second.serializtion( fs );
		fs << "\n";
	}
	return true;
}


static char* skipTo( char* str , char c )
{
	return strchr( str , c );
}

static char* skipTo( char* str , char* s )
{
	return strpbrk( str , s );
}


static char* skipSpace( char* str )
{
	char* p = str;
	while( ::isspace( *p ) ){  ++p; }
	return p;
}

static void cutBackSpace( char* str )
{
	char* p = str;
	do { --p; } while( ::isspace( *p ) );
	*( p + 1 ) = '\0';
}

enum
{
	PARSE_SUCCESS       = 0,
	PARSE_SECTION_ERROR = 1,
	PARSE_UNKONW_RULE    ,
	PARSE_KEYVALUE_ERROR ,
};

int PropertyKey::parseLine( char* buffer , KeySection** curSection )
{
	char* token;
	char* test;
	token = skipSpace( buffer );

	if ( token[0] == '\0' )
		return PARSE_SUCCESS;

	if ( token[0] == '/' )
	{
		if ( token[1] != '/' )
			return PARSE_UNKONW_RULE;
	}
	else if ( token[0] == '[' )
	{
		token = skipSpace( token + 1 );
		char* sectionName = token;

		token = skipTo( token , ']' );

		if ( token == NULL )
			return PARSE_SECTION_ERROR;

		cutBackSpace( token );

		token = skipSpace( token + 1 );
		if ( *token != '\0' )
			return PARSE_SECTION_ERROR;

		*curSection = &mGourpMap[ sectionName ];
	}
	else if ( test = skipTo( token , '=' ) )
	{
		switch( *test )
		{
		case '=':
			{
				char* keyName = token;
				token = test;

				cutBackSpace( token );

				token = skipSpace( token + 1 );
				char* keyValue = token;

				cutBackSpace( token + strlen( token ) );

				(*curSection)->addKeyValue( keyName , keyValue );
			}
			break;
		}
	}
	else
	{
		return PARSE_UNKONW_RULE;
	}

	return PARSE_SUCCESS;
}

bool PropertyKey::loadFile( char const* path )
{
	
	std::ifstream fs( path );
	if ( !fs.is_open() )
		return false;

	KeySection* curSection = &mGourpMap[ GLOBAL_SECTION ];

	fs.peek();
	while( fs.good() )
	{
		char buffer[4096];
		fs.getline( buffer , sizeof( buffer ) );

		switch ( parseLine( buffer , &curSection ) )
		{
		case PARSE_SECTION_ERROR:
			return false;			
		case PARSE_UNKONW_RULE:
			break;
		}
		fs.peek();
	}

	return true;
}
