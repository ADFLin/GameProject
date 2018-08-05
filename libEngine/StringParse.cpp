#include "StringParse.h"

#include "MarcoCommon.h"

static int CountCharReverse(char const* str , char const* last , char c )
{
	int result = 0;
	while( *str == c )
	{
		++result;
		if( str == last )
			break;
		--str;
	}
	return result;
}

Tokenizer::Tokenizer(char const* str, char const* dropDelims, char const* stopDelims /*= ""*/) 
	:mPtr(str)
{

	addDelims(dropDelims, DropMask);
	addDelims(stopDelims, StopMask);
}

void Tokenizer::skipDropDelims()
{
	while( *mPtr != 0 && isDropDelims(*mPtr) )
	{
		++mPtr;
	}
}

Tokenizer::TokenType Tokenizer::take(StringView& str)
{
	return FStringParse::StringToken(mPtr, *this, str);
}

void Tokenizer::reset(char const* str)
{
	mPtr = str;
}

int FStringParse::ParseNumber(char const* str, int& num)
{
	//#TODO : hexInt support
	char const* cur = str;
	int result = IntNumber;
	//Int
	while( *cur != 0 )
	{
		if( '0' > *cur || *cur > '9' )
			break;
		++cur;
	}
	if( *cur == '.' )
	{
		result = DoubleNumber;
		++cur;
		while( *cur != 0 )
		{
			if( '0' > *cur || *cur > '9' )
				break;
			++cur;
		}
	}
	if( *cur == 'e' || *cur == 'E' )
	{
		result = DoubleNumber;
		++cur;
		if( *cur == '-' )
			++cur;
		while( *cur != 0 )
		{
			if( '0' > *cur || *cur > '9' )
				break;
			++cur;
		}
	}
	if( result == DoubleNumber && *cur == 'f' )
	{
		result = FlotNumber;
		++cur;
	}
	if( *cur != 0 || !isspace(*cur) )
	{
		if( isalpha(*cur) )
			return ErrorNumber;
	}

	num = cur - str;
	return result;
}

int FStringParse::ParseIntNumber(char const* str, int& num)
{
	char const* cur = str;
	int base = 10;
	if( cur[0] == '0' && cur[1] == 'x' )
	{
		cur += 2;
		base = 16;
	}

	int result = 0;
	while( *cur != 0 )
	{
		if( '0' > *cur || *cur > '9' )
			break;

		result *= base;
		result += int(*cur - '0');
		++cur;
	}
	return result;
}

char const* FStringParse::FindLastChar(char const* str, int num, char c)
{
	char const* ptr = str + (num - 1);
	while( num )
	{
		if( *ptr == c )
			return ptr;
		--ptr;
		--num;
	}
	return nullptr;
}

char const* FStringParse::FindChar(char const* str, char c)
{
	while( *str != 0 )
	{
		if( *str == c )
			break;
		++str;
	}
	return str;
}

char const* FStringParse::FindChar(char const* str, char c1, char c2)
{
	while( *str != 0 )
	{
		if( *str == c1 || *str == c2 )
			break;
		++str;
	}
	return str;
}

char const* FStringParse::FindChar(char const* str, char c1, char c2, char c3)
{
	while( *str != 0 )
	{
		if( *str == c1 || *str == c2 || *str == c3 )
			break;
		++str;
	}
	return str;
}

char const* FStringParse::FindChar(char const* str, char c1, char c2, char c3, char c4)
{
	while( *str != 0 )
	{
		if( *str == c1 || *str == c2 || *str == c3 || *str == c4 )
			break;
		++str;
	}
	return str;
}

char const* FStringParse::FindChar(char const* str, char const* findChars)
{
	assert(str && findChars);
	while( *str != 0 )
	{
		if( *FindChar( findChars , *str ) != 0 )
			break;
		++str;
	}
	return str;
}


char const* FStringParse::TrySkipToSectionEnd(char const* str, char c)
{
	assert(str && *str == c);

	char const* ptr = str;
	for( ;;)
	{
		ptr = FindChar(ptr + 1, c , '\n');
		if( *ptr == 0 )
			return ptr;

		if( *ptr == '\n' )
			return str;

		//*ptr = '\"';
		if( *(ptr - 1) == '\\' )
		{
			if( !(CountCharReverse(ptr - 2, str, '\\') & 0x1) )
			{
				continue;
			}
		}

		break;
	}
	return ptr;
}

char const* FStringParse::CheckAndSkipToCommentSectionEnd(char const* str)
{
	assert(str && *str == '/');

	if( str[1] == '/' )
	{
		str = FindChar(str + 2, '\n');
	}
	else if( str[1] == '*' )
	{
		str += 2;
		for(;;)
		{
			str = FindChar(str, '*');
			if( *str == 0 )
				break;
			if( str[1] == '/' )
			{
				str += 1;
				break;
			}
			str += 1;
		}
	}

	return str;
}

char const* FStringParse::SkipChar(char const* str, char const* skipChars)
{
	assert(str && skipChars);
	while( *str != 0 )
	{
		if( *FindChar(skipChars, *str) == 0 )
			break;
		++str;
	}
	return str;
}

char const* FStringParse::SkipSpace(char const* str)
{
	char const* p = str;
	while( *p ) 
	{
		if ( !::isspace(*p) )
			break;
		++p; 
	}
	return p;
}

char const* FStringParse::SkipToNextLine(char const* str)
{
	char const* nextLine = FindChar(str, '\n');
	if( *nextLine != 0 )
		++nextLine;
	return nextLine;
}

char const* FStringParse::SkipToChar(char const* str, char c, char cPair, bool bCheckComment, bool bCheckString)
{
	int countPair = 0;
	if( bCheckString )
	{
		if( bCheckComment )
		{
			for( ;;)
			{
				str = FindChar(str, c, cPair, '/', '\"');
				if( *str == 0 )
					return str;

				if( *str == cPair )
				{
					++countPair;
				}
				else if( *str == c )
				{
					if( countPair == 0 )
						break;
					--countPair;
				}
				else if( *str == '\"' )
				{
					str = TrySkipToStringSectionEnd(str);
					if( *str == 0 )
						return str;
				}
				else //SkipComment 
				{
					str = CheckAndSkipToCommentSectionEnd(str);
					if( *str == 0 )
						return str;
				}

				str += 1;
			}
		}
		else
		{
			for( ;;)
			{
				str = FindChar(str, c, cPair, '\"');
				if( *str == 0 )
					return str;

				if( *str == cPair )
				{
					++countPair;
				}
				else if( *str == c )
				{
					if( countPair == 0 )
						break;
					--countPair;
				}
				else if( *str == '\"' )
				{
					str = TrySkipToStringSectionEnd(str);
					if( *str == 0 )
						return str;
				}

				str += 1;
			}
		}
	}
	else if( bCheckComment )
	{
		for( ;;)
		{
			str = FindChar(str, c, cPair, '/');
			if( *str == 0 )
				return str;

			if( *str == cPair )
			{
				++countPair;
			}
			else if( *str == c )
			{
				if( countPair == 0 )
					break;
				--countPair;
			}
			else //SkipComment
			{
				str = CheckAndSkipToCommentSectionEnd(str);
				if( *str == 0 )
					return str;
			}
			str += 1;
		}
	}
	else
	{
		for( ;;)
		{
			str = FindChar(str, c, cPair);
			if( *str == 0 )
				return str;
			if( *str == cPair )
			{
				++countPair;
			}
			else
			{
				if( countPair == 0 )
					break;
				--countPair;
			}
			str += 1;
		}
	}

	return str;
}

char const* FStringParse::SkipToChar(char const* str, char c, bool bCheckComment, bool bCheckString)
{
	if( bCheckString )
	{
		if( bCheckComment )
		{
			for( ;;)
			{
				str = FindChar(str, c, '/', '\"');
				if( *str == 0 )
					break;

				if( *str == c )
				{
					break;
				}
				else if( *str == '\"' )
				{
					str = TrySkipToStringSectionEnd(str);
					if( *str == 0 )
						return str;
				}
				else
				{
					str = CheckAndSkipToCommentSectionEnd(str);
					if( *str == 0 )
						return str;
				}

				str += 1;
			}
		}
		else
		{
			for( ;;)
			{
				str = FindChar(str, c, '\"');
				if( *str == 0 )
					break;

				if( *str == '\"' )
				{
					str = TrySkipToStringSectionEnd(str);
					if( *str == 0 )
						return str;
				}

				str += 1;
			}
		}

	}
	else if( bCheckComment )
	{
		for( ;;)
		{
			str = FindChar(str, c, '/');
			if( *str == 0 )
				break;
			//SkipComment
			if( *str == c )
			{
				break;
			}
			else
			{
				str = CheckAndSkipToCommentSectionEnd(str);
				if( *str == 0 )
					return str;
			}

			str += 1;
		}
	}
	else
	{
		str = FindChar(str, c);
	}

	return str;
}

void FStringParse::ReplaceChar(char* str, char c, char replace)
{
	char* ptr = str;
	while( *ptr != 0 )
	{
		if( *ptr == c )
			*ptr = replace;
		++ptr;
	}
}

FStringParse::TokenType FStringParse::StringToken(char const*& inoutStr, DelimsTable const& table, StringView& outToken)
{
	char cur = *(inoutStr++);
	for( ;; )
	{
		if( cur == '\0' )
		{
			return FStringParse::eNoToken;
		}
		if( !table.isDropDelims(cur) )
			break;

		cur = *(inoutStr++);
	}

	char const* ptr = inoutStr - 1;
	if( table.isStopDelims(cur) )
	{
		outToken = StringView(ptr, 1);
		return FStringParse::eDelimsType;
	}

	for( ;;)
	{
		cur = *inoutStr;
		if( cur == '\0' || table.isDelims(cur) )
			break;
		++inoutStr;
	}
	outToken = StringView(ptr, inoutStr - ptr);
	return FStringParse::eStringType;
}

FStringParse::TokenType FStringParse::StringToken(char const*& inoutStr, char const* dropDelims, char const* stopDelims, StringView& outToken)
{
	char cur = *(inoutStr++);
	for( ;; )
	{
		if( cur == '\0' )
		{
			return FStringParse::eNoToken;
		}
		if( *FindChar( dropDelims , cur ) )
			break;

		cur = *(inoutStr++);
	}

	char const* ptr = inoutStr - 1;
	if( *FindChar(stopDelims, cur) != 0 )
	{
		outToken = StringView(ptr, 1);
		return FStringParse::eDelimsType;
	}

	for( ;;)
	{
		cur = *inoutStr;
		if( cur == '\0' || ( *FindChar(stopDelims, cur) ) || (*FindChar(stopDelims, cur) ) )
			break;
		++inoutStr;
	}

	outToken = StringView(ptr, inoutStr - ptr);
	return FStringParse::eStringType;
}

bool FStringParse::StringToken(char const*& inoutStr, char const* dropDelims, StringView& outToken)
{
	inoutStr = SkipChar(inoutStr, dropDelims);
	if( *inoutStr == 0 )
	{
		return false;
	}

	char const* ptr = inoutStr;
	inoutStr = FindChar(inoutStr, dropDelims);
	outToken = StringView(ptr, inoutStr - ptr);
	return true;
}

StringView FStringParse::StringTokenLine(char const*& inoutStr)
{
	char const* start = inoutStr;
	inoutStr = FindChar(inoutStr, '\n');
	char const* end = inoutStr;
	if( *end != 0 )
	{
		++inoutStr;
		if( *end == '\n' )
		{
			if( end != start && *(end - 1) == '\r' )
			{
				--end;
			}
		}
	}
	return StringView( start , end - start );
}

DelimsTable::DelimsTable()
{
	std::fill_n(mDelimsMap, ARRAY_SIZE(mDelimsMap), 0);
}

void DelimsTable::addDelims(char const* delims, uint8 mask)
{
	assert(delims);
	for( ;;)
	{
		char cur = *delims;
		if( cur == 0 )
			return;
		mDelimsMap[uint8(cur)] |= mask;
		++delims;
	}
}

bool DelimsTable::isDelims(char c) const
{
	return !!mDelimsMap[uint8(c)];
}

bool DelimsTable::isStopDelims(char c) const
{
	return !!(mDelimsMap[uint8(c)] & StopMask);
}

bool DelimsTable::isDropDelims(char c) const
{
	return !!(mDelimsMap[uint8(c)] & DropMask);
}
