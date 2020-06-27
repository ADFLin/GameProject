#include "TinyGamePCH.h"
#include "PropertySet.h"
#include "Core/StringConv.h"

#include <fstream>
#include "StringParse.h"

#define GLOBAL_SECTION "__GLOBAL__"

float KeyValue::getFloat() const
{
	if( mCacheGetType != CacheFloat )
	{
		if (!FStringConv::ToCheck(mValue.data(), mValue.size(), mCacheGetValue.floatValue))
		{
			mCacheGetValue.floatValue = 0;
		}
		mCacheGetType = CacheFloat;
	}
	return mCacheGetValue.floatValue;
}

int KeyValue::getInt() const
{
	if( mCacheGetType != CacheInt )
	{
		if (!FStringConv::ToCheck(mValue.data(), mValue.size(), mCacheGetValue.intValue))
		{
			mCacheGetValue.intValue = 0;
		}
		mCacheGetType = CacheInt;
	}
	return mCacheGetValue.intValue;
}

bool KeyValue::getBool() const
{
	if( mCacheGetType != CacheInt )
	{
		bool temp;
		if (FStringConv::ToCheck(mValue.data(), mValue.size(), temp))
		{
			mCacheGetValue.intValue = temp ? 1 : 0;
		}
		else
		{
			mCacheGetValue.intValue = 0;
		}
		mCacheGetType = CacheInt;
	}
	return mCacheGetValue.intValue;
}

void  KeyValue::setFloat( float value )
{
	mValue = FStringConv::From( value );
	mCacheGetType = CacheFloat;
	mCacheGetValue.floatValue = value;
}

void KeyValue::setInt( int value )
{
	mValue = FStringConv::From(value);
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
	auto iter = mKeyMap.find( keyName );
	if ( iter != mKeyMap.end() )
		return &iter->second;
	return nullptr;
}

void KeySection::serializtion( std::ostream& os )
{
	std::vector< KeyValueMap::iterator > sortedValueIters;

	for( auto iter = mKeyMap.begin();
		iter != mKeyMap.end() ; ++iter )
	{
		sortedValueIters.push_back(iter);	
	}

	std::sort(sortedValueIters.begin(), sortedValueIters.end(), [](auto lhs, auto rhs)
	{
		return lhs->second.mSequenceOrder < rhs->second.mSequenceOrder;
	});

	for( auto iter : sortedValueIters )
	{
		os << iter->first << " = " << iter->second.getString() << '\n';
	}
}

PropertySet::PropertySet()
{
	mNextSectionSeqOrder = 0;
	mNextValueSeqOrder = 0;
}

KeyValue* PropertySet::getKeyValue( char const* group, char const* keyName )
{
	if ( !group )
		group = GLOBAL_SECTION;

	auto iter = mSectionMap.find( group );
	if ( iter == mSectionMap.end() )
		return nullptr;

	return iter->second.getKeyValue( keyName );
}

template< class T > struct KeyOp {};
template<>  struct KeyOp< int >  { static int   Get( KeyValue const* value ){ return value->getInt(); }  }; 
template<>  struct KeyOp< float >{ static  float Get( KeyValue const* value ){ return value->getFloat(); }  };
template<>  struct KeyOp< char > { static  char  Get( KeyValue const* value ){ return value->getChar(); }  };
template<>  struct KeyOp< char const* >{ static char const* Get( KeyValue const* value ){ return value->getString(); } };

template< class T >
bool PropertySet::tryGetValueT( char const* keyName , char const* group , T& value )
{
	KeyValue const* keyValue = getKeyValue( group, keyName );
	if ( !keyValue )
		return false;

	value = KeyOp<T>::Get( keyValue );
	return true;
}

template< class T >
T PropertySet::getValueT( char const* keyName , char const* group , T defaultValue )
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

bool PropertySet::tryGetCharValue( char const* keyName , char const* group , char& value )
{
	return tryGetValueT( keyName , group , value );
}

bool PropertySet::tryGetIntValue( char const* keyName , char const* group , int& value )
{
	return tryGetValueT( keyName , group , value );
}

bool PropertySet::tryGetFloatValue( char const* keyName , char const* group , float& value )
{
	return tryGetValueT( keyName , group , value );
}

bool PropertySet::tryGetStringValue( char const* keyName , char const* group , char const*& value )
{
	return tryGetValueT( keyName , group , value );
}

char PropertySet::getCharValue( char const* keyName , char const* group , char defaultValue )
{
	return getValueT( keyName , group , defaultValue );
}

int PropertySet::getIntValue( char const* keyName , char const* group , int defaultValue )
{
	return getValueT( keyName , group , defaultValue );
}

float PropertySet::getFloatValue( char const* keyName , char const* group , float defaultValue )
{
	return getValueT( keyName , group , defaultValue );
}

char const* PropertySet::getStringValue( char const* keyName , char const* group , char const* defaultValue )
{
	return getValueT( keyName , group , defaultValue );
}

bool PropertySet::saveFile( char const* path )
{
	std::ofstream fs( path );

	if ( !fs )
		return false;

	auto globalIter = mSectionMap.find( GLOBAL_SECTION );

	if( globalIter != mSectionMap.end() )
	{
		globalIter->second.serializtion(fs);
		fs << "\n";
	}

	std::vector < KeySectionMap::iterator > sortedSectionIters;

	for( auto iter = mSectionMap.begin();
		iter != mSectionMap.end() ; ++iter )
	{
		if ( iter == globalIter )
			continue;

		sortedSectionIters.push_back(iter);
	}

	std::sort( sortedSectionIters.begin() , sortedSectionIters.end() , 
		[](auto lhs, auto rhs)
		{
			return lhs->second.mSequenceOrder < rhs->second.mSequenceOrder;
		}
	);

	for( auto iter : sortedSectionIters )
	{
		fs << "[" << iter->first << "]" << "\n";
		iter->second.serializtion(fs);
		fs << "\n";
	}

	return true;
}


static char* SkipTo( char* str , char c )
{
	return strchr( str , c );
}

static char* SkipTo( char* str , char* s )
{
	return strpbrk( str , s );
}

static void CutBackSpace( char* str )
{
	char* p = str;
	do { --p; } while( FCString::IsSpace( *p ) );
	*( p + 1 ) = '\0';
}

enum
{
	PARSE_SUCCESS       = 0,
	PARSE_SECTION_ERROR = 1,
	PARSE_UNKONW_RULE    ,
	PARSE_KEYVALUE_ERROR ,
};

int PropertySet::parseLine( char* buffer , KeySection** curSection )
{
	char* token;
	char* test;
	token = const_cast< char* >( FStringParse::SkipSpace( buffer ) );
	
	if ( token[0] == '\0' )
		return PARSE_SUCCESS;

	if ( token[0] == '/' )
	{
		if ( token[1] != '/' )
			return PARSE_UNKONW_RULE;
	}
	else if ( token[0] == '[' )
	{
		token = const_cast<char*>( FStringParse::SkipSpace( token + 1 ) );
		char* sectionName = token;

		token = SkipTo( token , ']' );

		if ( token == nullptr )
			return PARSE_SECTION_ERROR;

		CutBackSpace( token );

		token = const_cast<char*>( FStringParse::SkipSpace( token + 1 ) );
		if ( *token != '\0' )
			return PARSE_SECTION_ERROR;

		*curSection = &mSectionMap[ sectionName ];

		(*curSection)->mSequenceOrder = mNextSectionSeqOrder;
		++mNextSectionSeqOrder;
	}
	else if ( test = SkipTo( token , '=' ) )
	{
		switch( *test )
		{
		case '=':
			{
				char* keyName = token;
				token = test;

				CutBackSpace( token );

				token = const_cast<char*>( FStringParse::SkipSpace( token + 1 ));
				char* keyValueString = token;

				CutBackSpace( token + strlen( token ) );

				auto pKeyValue = (*curSection)->addKeyValue( keyName , keyValueString );

				if( pKeyValue )
				{
					pKeyValue->mSequenceOrder = mNextValueSeqOrder;
					++mNextValueSeqOrder;
				}
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

bool PropertySet::loadFile( char const* path )
{
	std::ifstream fs( path );
	if ( !fs.is_open() )
		return false;

	KeySection* curSection = &mSectionMap[ GLOBAL_SECTION ];

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
