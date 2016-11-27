#include "TinyGamePCH.h"
#include "PropertyKey.h"


#define GLOBAL_SECTION "__GLOBAL__"

void  KeyValue::setFloat( float value )
{ 
	std::stringstream ss;
	ss << value;
	mValue = ss.str();
}

void KeyValue::setInt( int value )
{
	std::stringstream ss;
	ss << value;
	mValue = ss.str();
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
template<>  struct KeyOp< int >  { int   get( KeyValue* value ){ return value->getInt(); }  }; 
template<>  struct KeyOp< float >{ float get( KeyValue* value ){ return value->getFloat(); }  }; 
template<>  struct KeyOp< char > { char  get( KeyValue* value ){ return value->getChar(); }  }; 
template<>  struct KeyOp< char const* >{ char const* get( KeyValue* value ){ return value->getString(); } };

template< class T >
bool PropertyKey::tryGetValueT( char const* keyName , char const* group , T& value )
{
	KeyValue* keyValue = getKeyValue( group, keyName );
	if ( !keyValue )
		return false;

	KeyOp< T > op;
	value = op.get( keyValue );
	return true;
}

template< class T >
T PropertyKey::getValueT( char const* keyName , char const* group , T defaultValue )
{
	if( !group )
		group = GLOBAL_SECTION;

	KeyValue* keyValue = getKeyValue( group, keyName );
	if ( keyValue )
	{
		KeyOp< T > op;
		return op.get( keyValue );
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

	if ( globalIter != mGourpMap.end() )
		globalIter->second.serializtion( fs );

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
