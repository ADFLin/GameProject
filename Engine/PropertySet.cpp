#include "PropertySet.h"

#include "Core/StringConv.h"
#include "StringParse.h"
#include "DataStructure/Array.h"
#include "TemplateMisc.h"

#include <fstream>
#include <algorithm>
#include "Meta/Concept.h"
#include "Meta/FunctionCall.h"

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

TArray<KeyValue*> KeySection::getKeyValues(char const* keyName)
{
	TArray<KeyValue*> result;
	auto iterPair = mKeyMap.equal_range(keyName);
	for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
	{
		result.push_back(&iter->second);
	}
	return result;
}

BITWISE_RELLOCATABLE_FAIL(KeySection::KeyValueMap::iterator);

PropertySet::PropertySet()
{

}

KeyValue* PropertySet::getKeyValue( char const* section, char const* keyName )
{
	if ( !section )
		section = PROPERTYSET_GLOBAL_SECTION;

	auto iter = mSectionMap.find( section );
	if ( iter == mSectionMap.end() )
		return nullptr;

	return iter->second.getKeyValue( keyName );
}

template< class T > struct KeyOp {};
template<>  struct KeyOp< int >  { static int   Get( KeyValue const* value ){ return value->getInt(); }  }; 
template<>  struct KeyOp< float >{ static  float Get( KeyValue const* value ){ return value->getFloat(); }  };
template<>  struct KeyOp< char > { static  char  Get( KeyValue const* value ){ return value->getChar(); }  };
template<>  struct KeyOp< char const* >{ static char const* Get( KeyValue const* value ){ return value->getString(); } };
template<>  struct KeyOp< StringView > { static StringView Get( KeyValue const* value ) { return value->getStringView(); } };
template<>  struct KeyOp< bool > { static bool Get(KeyValue const* value) { return value->getBool(); } };

template< class T >
bool PropertySet::tryGetValueT( char const* keyName , char const* section , T& value )
{
	KeyValue const* keyValue = getKeyValue( section, keyName );
	if ( !keyValue )
		return false;

	value = KeyOp<T>::Get( keyValue );
	return true;
}

template< class T >
T PropertySet::getValueT( char const* keyName , char const* section , T defaultValue )
{
	KeyValue const* keyValue = getKeyValue( section, keyName );
	if ( keyValue )
	{
		return KeyOp<T>::Get( keyValue );
	}
	setKeyValue( keyName , section , defaultValue );
	return defaultValue;
}

bool PropertySet::tryGetCharValue( char const* keyName , char const* section , char& value )
{
	return tryGetValueT( keyName , section , value );
}

bool PropertySet::tryGetIntValue( char const* keyName , char const* section , int& value )
{
	return tryGetValueT( keyName , section , value );
}

bool PropertySet::tryGetFloatValue( char const* keyName , char const* section , float& value )
{
	return tryGetValueT( keyName , section , value );
}

bool PropertySet::tryGetStringValue( char const* keyName , char const* section , char const*& value )
{
	return tryGetValueT( keyName , section , value );
}

bool PropertySet::tryGetStringValue(char const* keyName, char const* section, StringView& value)
{
	return tryGetValueT(keyName, section, value);
}

bool PropertySet::tryGetBoolValue(char const* keyName, char const* section, bool& value)
{
	return tryGetValueT(keyName, section, value);
}

char PropertySet::getCharValue( char const* keyName , char const* section , char defaultValue )
{
	return getValueT( keyName , section , defaultValue );
}

int PropertySet::getIntValue( char const* keyName , char const* section , int defaultValue )
{
	return getValueT( keyName , section , defaultValue );
}

float PropertySet::getFloatValue( char const* keyName , char const* section , float defaultValue )
{
	return getValueT( keyName , section , defaultValue );
}

char const* PropertySet::getStringValue( char const* keyName , char const* section , char const* defaultValue )
{
	return getValueT( keyName , section , defaultValue );
}

bool PropertySet::getBoolValue(char const* keyName, char const* section, bool defaultValue)
{
	return getValueT(keyName, section, defaultValue);
}

BITWISE_RELLOCATABLE_FAIL(PropertySet::KeySectionMap::iterator);

bool PropertySet::saveFile( char const* path )
{
	return mFile.save(path);
}


static char* SkipTo( char* str , char c )
{
	return strchr( str , c );
}

static char* SkipTo( char* str , char* s )
{
	return strpbrk( str , s );
}

static void TrimEnd( char* str )
{
	char* p = str;
	do { --p; } while( FCString::IsSpace( *p ) );
	*( p + 1 ) = 0;
}

enum
{
	PARSE_SUCCESS       = 0,
	PARSE_SECTION_ERROR = 1,
	PARSE_UNKONW_RULE    ,
	PARSE_KEYVALUE_ERROR ,
};

bool PropertySet::loadFile( char const* path )
{
	mFile.clear();
	if (!mFile.load(path))
		return false;

	append(mFile);
	return true;
}

void PropertySet::getStringValues(char const* keyName, char const* section, TArray< char const* >& outValue)
{
	if (!section)
		section = PROPERTYSET_GLOBAL_SECTION;

	auto iter = mSectionMap.find(section);
	if (iter == mSectionMap.end())
		return;

	iter->second.visitKeys(keyName, [&outValue](KeyValue& value)
	{
		outValue.push_back(value.getString());
	});
}

void PropertySet::getStringValues(char const* keyName, char const* section, TArray< std::string >& outValue)
{
	if (!section)
		section = PROPERTYSET_GLOBAL_SECTION;

	auto iter = mSectionMap.find(section);
	if (iter == mSectionMap.end())
		return;

	iter->second.visitKeys(keyName, [&outValue](KeyValue& value)
	{
		outValue.push_back(value.getString());
	});
}

void PropertySet::setStringValues(char const* keyName, char const* section, TArray< char const* > const& values, bool bSorted)
{
	if (!section)
		section = PROPERTYSET_GLOBAL_SECTION;

	mFile.setKeyValues(section, keyName, values, bSorted);
	mSectionMap[section].setKeyValues(keyName, values);
}

void PropertySet::setStringValues(char const* keyName, char const* section, TArray< std::string > const& values, bool bSorted)
{
	if (!section)
		section = PROPERTYSET_GLOBAL_SECTION;

	mFile.setKeyValues(section, keyName, values, bSorted);
	mSectionMap[section].setKeyValues(keyName, values);
}

void PropertySet::append(PropertyFile& file)
{
	auto AppendSection = [this](KeySection& section, TArray< PropertyFile::Element > const& elements)
	{
		for( auto const& element : elements)
		{
			switch (element.op)
			{
			case PropertyFile::EValueOp::Set:
				section.setKeyValue(element.key.c_str(), element.value.c_str());
				break;
			case PropertyFile::EValueOp::Add:
				section.addKeyValue(element.key.c_str(), element.value.c_str());
				break;
			case PropertyFile::EValueOp::Remove:
				section.removeKeyValue(element.key.c_str(), element.value.c_str());
				break;
			case PropertyFile::EValueOp::RemoveAll:
				section.removeKey(element.key.c_str());
				break;
			}
		}
	};

	AppendSection(mSectionMap[PROPERTYSET_GLOBAL_SECTION], file.mGlobalElements);
	for (auto const& section : file.mSections)
	{
		AppendSection(mSectionMap[section.key], section.elements);
	}
}

bool PropertyFile::save(char const* path)
{
	std::ofstream fs(path);

	if (!fs)
		return false;

	auto SerializeValues = [&fs](TArray<Element> const& values)
	{
		for (auto const& value : values)
		{
			switch (value.op)
			{
			case EValueOp::Set:
				break;
			case EValueOp::Add:
				fs << '+';
				break;
			case EValueOp::Remove:
				fs << '-';
				break;
			case EValueOp::Comment:
				fs << value.value << std::endl;
				continue;
			}
			fs << value.key << " = " << value.value << std::endl;
		}
	};

	SerializeValues(mGlobalElements);

	for (auto const& section : mSections)
	{
		fs << std::endl;
		fs << '[' << section.key << ']' << std::endl;
		SerializeValues(section.elements);
	}

	return true;
}

bool PropertyFile::load(char const* path)
{
	std::ifstream fs(path);
	if (!fs.is_open())
		return false;

	TArray<Element>* sectionElementsPtr = &mGlobalElements;

	fs.peek();
	while (fs.good())
	{
		char buffer[4096];
		fs.getline(buffer, sizeof(buffer));

		switch (parseLine(buffer, sectionElementsPtr))
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

int PropertyFile::parseLine(char* buffer, TArray< Element >*& sectionElementsPtr)
{
	char* token;
	char* test;
	token = const_cast<char*>(FStringParse::SkipSpace(buffer));

	if (token[0] == '\0')
		return PARSE_SUCCESS;

	if (token[0] == '/')
	{
		if (token[1] != '/')
			return PARSE_UNKONW_RULE;

		Element value;
		value.op = EValueOp::Comment;
		value.value = token;
		sectionElementsPtr->push_back(std::move(value));
	}
	else if (token[0] == '[')
	{
		token = const_cast<char*>(FStringParse::SkipSpace(token + 1));
		char* sectionName = token;

		token = SkipTo(token, ']');

		if (token == nullptr)
			return PARSE_SECTION_ERROR;

		TrimEnd(token);

		token = const_cast<char*>(FStringParse::SkipSpace(token + 1));
		if (*token != 0)
			return PARSE_SECTION_ERROR;


		mSections.push_back(Section());

		mSections.back().key = sectionName;
		sectionElementsPtr = &mSections.back().elements;
	}
	else 
	{
		EValueOp op = EValueOp::Set;
		if (*token == '+')
		{
			op = EValueOp::Add;
			++token;

		}
		else if (*token == '-')
		{
			op = EValueOp::Remove;
			++token;
		}

		if (test = SkipTo(token, '='))
		{
			switch (*test)
			{
			case '=':
				{
					char* keyName = token;
					token = test;

					TrimEnd(token);

					token = const_cast<char*>(FStringParse::SkipSpace(token + 1));
					char* keyValueString = token;

					TrimEnd(token + FCString::Strlen(token));

					Element value;
					value.op = op;
					value.key = keyName;
					value.value = keyValueString;
					sectionElementsPtr->push_back(std::move(value));
				}
				break;
			}
		}
		else if ( op == EValueOp::Remove )
		{
			char* keyName = token;
			token = test;
			TrimEnd(token);
			return PARSE_UNKONW_RULE;
		}
		else
		{
			return PARSE_UNKONW_RULE;
		}
	}

	return PARSE_SUCCESS;
}


template< typename TFunc >
bool PropertyFile::visit(char const* sectionName, char const* keyName, TFunc&& func)
{
	auto VisitElement = [keyName, &func](TArray<Element>& elements) -> bool
	{
		if constexpr (TCheckConcept< CFunctionCallable, TFunc, TArray<Element> >::Value)
		{
			func(elements);
		}

		if constexpr (TCheckConcept< CFunctionCallable, TFunc, Element >::Value)
		{
			for (int i = 0; i < elements.size(); ++i)
			{
				auto& element = elements[i];
				if (element.op == EValueOp::Comment)
					continue;

				if (element.key != keyName)
					continue;

				switch (func(elements[i]))
				{
				case EVisitOp::Remove:
					elements.removeIndex(i);
					--i;
					break;
				case EVisitOp::RemoveStop:
					elements.removeIndex(i);
					--i;
				case EVisitOp::Stop:
					return true;
				}
			}
		}

		return false;
	};

	if (sectionName == nullptr || FCString::Compare(sectionName, PROPERTYSET_GLOBAL_SECTION) == 0)
	{
		return VisitElement(mGlobalElements);
	}

	for (Section& section : mSections)
	{
		if (section.key != sectionName)
			continue;

		if (VisitElement(section.elements))
			return true;
	}

	return false;
}

void PropertyFile::setKeyValue(char const* sectionName, char const* keyName, char const* value)
{
	TArray< Element >* firstElements = nullptr;
	bool bSetted = false;

	visit(sectionName, keyName, TOverloaded
		{
			[&](Element& element)
			{
				if (bSetted)
					return EVisitOp::Remove;

				if (element.value != value)
					return EVisitOp::Remove;
				
				if (element.op != EValueOp::Set)
					return EVisitOp::Remove;

				bSetted = true;
				return EVisitOp::Keep;
			},
			[&](TArray< Element >& elements)
			{
				if (firstElements == nullptr)
				{
					firstElements = &elements;
				}
			}
		}
	);

	if (bSetted)
		return;

	if (firstElements == nullptr)
	{
		Section section;
		section.key = sectionName;
		mSections.push_back(std::move(section));
		firstElements = &mSections.back().elements;
	}

	Element element;
	element.key = keyName;
	element.value = value;
	element.op = EValueOp::Set;
	firstElements->push_back(std::move(element));
}

void PropertyFile::setKeyValues(char const* sectionName, char const* keyName, TArray<char const*> const& values, bool bSorted)
{
	TArray< Element >* firstElements = nullptr;
	TArray< bool > indicesSetted( values.size(), false );
	int indexCheckSort = 0;

	visit(sectionName, keyName, TOverloaded
		{
			[&](Element& element)
			{
				int32 index = values.findIndexPred([&element](char const* value)
				{
					return element.value == value;
				});
				if (index == INDEX_NONE)
					return EVisitOp::Remove;

				if (indicesSetted[index])
					return EVisitOp::Remove;

				if (element.op != EValueOp::Add)
					return EVisitOp::Remove;

				if (bSorted)
				{
					if (index != indexCheckSort)
					{
						return EVisitOp::Remove;
					}
					else
					{
						++indexCheckSort;
					}
				}

				indicesSetted[index] = true;
				return EVisitOp::Keep;
			},
			[&](TArray< Element >& elements)
			{
				if (firstElements == nullptr)
				{
					firstElements = &elements;
				}
			}
		}
	);

	if (firstElements == nullptr)
	{
		Section section;
		section.key = sectionName;
		mSections.push_back(std::move(section));
		firstElements = &mSections.back().elements;
	}

	for (int index = 0; index < values.size(); ++index)
	{
		if (indicesSetted[index])
			continue;

		Element element;
		element.key = keyName;
		element.value = values[index];
		element.op = EValueOp::Add;
		firstElements->push_back(std::move(element));
	}
}


void PropertyFile::setKeyValues(char const* sectionName, char const* keyName, TArray<std::string> const& values, bool bSorted)
{
	TArray< Element >* firstElements = nullptr;
	TArray< bool > indicesSetted(values.size(), false);
	int indexCheckSort = 0;

	visit(sectionName, keyName, TOverloaded
		{
			[&](Element& element)
			{
				int32 index = values.findIndexPred([&element](std::string const& value)
				{
					return element.value == value;
				});
				if (index == INDEX_NONE)
					return EVisitOp::Remove;

				if (indicesSetted[index])
					return EVisitOp::Remove;

				if (element.op != EValueOp::Add)
					return EVisitOp::Remove;

				if (bSorted)
				{
					if (index != indexCheckSort)
					{
						return EVisitOp::Remove;
					}
					else
					{
						++indexCheckSort;
					}
				}

				indicesSetted[index] = true;
				return EVisitOp::Keep;
			},
			[&](TArray< Element >& elements)
			{
				if (firstElements == nullptr)
				{
					firstElements = &elements;
				}
			}
		}
	);

	if (firstElements == nullptr)
	{
		Section section;
		section.key = sectionName;
		mSections.push_back(std::move(section));
		firstElements = &mSections.back().elements;
	}

	for (int index = 0; index < values.size(); ++index)
	{
		if (indicesSetted[index])
			continue;

		Element element;
		element.key = keyName;
		element.value = values[index];
		element.op = EValueOp::Add;
		firstElements->push_back(std::move(element));
	}
}
