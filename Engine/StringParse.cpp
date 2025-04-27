#include "StringParse.h"

#include "MarcoCommon.h"

template< typename CharT >
static int CountCharReverse(CharT const* str , CharT const* last , CharT c )
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

int Tokenizer::skipToChar(char c)
{
	char const* prev = mPtr;
	mPtr = FStringParse::FindChar(mPtr, c);
	return mPtr - prev;
}

Tokenizer::TokenType Tokenizer::take(StringView& token)
{
	return FStringParse::StringToken(mPtr, *this, token);
}

Tokenizer::TokenType Tokenizer::take(DelimsTable const& table, StringView& token)
{
	return FStringParse::StringToken(mPtr, table, token);
}

bool Tokenizer::takeUntil(char stopChar, StringView& token)
{
	char const* content = mPtr;
	char const* contentEnd = FStringParse::FindChar(mPtr, ']');
	if (*contentEnd == 0)
	{
		return false;
	}
	token = StringView(content, contentEnd - content);
	mPtr = contentEnd + 1;
	return true;
}

bool Tokenizer::takeChar(char c)
{
	FStringParse::SkipDelims(mPtr, *this);
	if (*mPtr == c)
	{
		++mPtr;
		return true;
	}
	return false;
}

void Tokenizer::reset(char const* str)
{
	mPtr = str;
}

DelimsTable::DelimsTable()
{
	std::fill_n(mDelimsMap, ARRAY_SIZE(mDelimsMap), 0);
}

DelimsTable::DelimsTable(char const* dropDelims, char const* stopDelims)
{
	std::fill_n(mDelimsMap, ARRAY_SIZE(mDelimsMap), 0);
	addDelims(dropDelims, DropMask);
	addDelims(stopDelims, StopMask);
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

#define CharL(v) STRING_LITERAL(CharT , v)

template< typename CharT >
int TStringParse< CharT >::ParseNumber(CharT const* str, int& num)
{
	//#TODO : hexInt support
	CharT const* cur = str;
	int result = IntNumber;
	//Int
	while( *cur != 0 )
	{
		if(!FCString::IsDigit(*cur))
			break;
		++cur;
	}
	if( *cur == CharL('.') )
	{
		result = DoubleNumber;
		++cur;
		while( *cur != 0 )
		{
			if (!FCString::IsDigit(*cur))
				break;
			++cur;
		}
	}
	if( *cur == CharL('e') || *cur == CharL('E') )
	{
		result = DoubleNumber;
		++cur;
		if( *cur == CharL('-') )
			++cur;
		while( *cur != 0 )
		{
			if (!FCString::IsDigit(*cur))
				break;
			++cur;
		}
	}
	if( result == DoubleNumber && *cur == CharL('f') )
	{
		result = FloatNumber;
		++cur;
	}
	if( *cur != 0 || !FCString::IsSpace(*cur) )
	{
		if( FCString::IsAlpha(*cur) )
			return ErrorNumber;
	}

	num = cur - str;
	return result;
}

template< typename CharT >
int TStringParse< CharT >::ParseIntNumber(CharT const* str, int& num)
{
	CharT const* cur = str;
	int base = 10;
	if( cur[0] == CharL('0') && cur[1] == CharL('x') )
	{
		cur += 2;
		base = 16;
	}

	int result = 0;
	while( *cur != 0 )
	{
		if( !FCString::IsDigit(*cur) )
			break;

		result *= base;
		result += int(*cur - CharL('0'));
		++cur;
	}
	return result;
}

template< typename CharT >
CharT const* TStringParse< CharT >::FindLastChar(CharT const* str, int num, CharT c)
{
	CharT const* ptr = str + (num - 1);
	while( num )
	{
		if( *ptr == c )
			return ptr;
		--ptr;
		--num;
	}
	return nullptr;
}

template< typename CharT>
FORCEINLINE bool CheckChars(CharT c)
{
	return false;
}

template< typename CharT, typename ...Chars >
FORCEINLINE bool CheckChars(CharT c, CharT c1, Chars ...chars)
{
	if (c == c1)
		return true;

	return CheckChars(c, chars...);
}

template< typename CharT >
CharT const* TStringParse< CharT >::FindChar(CharT const* str, CharT c)
{
	while( *str != 0 )
	{
		if (CheckChars(*str, c))
			break;
		++str;
	}
	return str;
}
 
template< typename CharT >
CharT const* TStringParse< CharT >::FindChar(CharT const* str, CharT c1, CharT c2)
{
	while( *str != 0 )
	{
		if (CheckChars(*str, c1, c2))
			break;
		++str;
	}
	return str;
}

template< typename CharT >
CharT const* TStringParse< CharT >::FindChar(CharT const* str, CharT c1, CharT c2, CharT c3)
{
	while( *str != 0 )
	{
		if (CheckChars(*str, c1, c2, c3))
			break;
		++str;
	}
	return str;
}

template< typename CharT >
CharT const* TStringParse< CharT >::FindChar(CharT const* str, CharT c1, CharT c2, CharT c3, CharT c4)
{
	while( *str != 0 )
	{
		if (CheckChars(*str, c1, c2, c3, c4))
			break;
		++str;
	}
	return str;
}

template< typename CharT >
CharT const* TStringParse< CharT >::FindChar(CharT const* str, CharT const* findChars)
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

template< typename CharT >
CharT const* TStringParse< CharT >::FindCharN(CharT const* str, int num , CharT c)
{
	while (*str != 0 && num )
	{
		if (CheckChars(*str, c))
			break;
		++str;
		--num;
	}
	return str;
}

template< typename CharT >
CharT const* TStringParse< CharT >::FindCharN(CharT const* str, int num, CharT c1, CharT c2)
{
	while (*str != 0 && num)
	{
		if (CheckChars(*str, c1, c2))
			break;
		++str;
		--num;
	}
	return str;
}

template< typename CharT >
CharT const* TStringParse< CharT >::FindCharN(CharT const* str, int num, CharT c1, CharT c2, CharT c3)
{
	while (*str != 0 && num)
	{
		if (CheckChars(*str, c1, c2, c3))
			break;
		++str;
		--num;
	}
	return str;
}

template< typename CharT >
CharT const* TStringParse< CharT >::FindCharN(CharT const* str, int num, CharT c1, CharT c2, CharT c3, CharT c4)
{
	while (*str != 0 && num)
	{
		if (CheckChars(*str, c1, c2, c3, c4))
			break;
		++str;
		--num;
	}
	return str;
}


template< typename CharT >
CharT const* TStringParse< CharT >::TrySkipToSectionEnd(CharT const* str, CharT c)
{
	assert(str && *str == c);

	CharT const* ptr = str;
	for( ;;)
	{
		ptr = FindChar(ptr + 1, c , CharL('\n'));
		if( *ptr == 0 )
			return ptr;

		if( *ptr == CharL('\n') )
			return str;

		//*ptr = '\"';
		if( *(ptr - 1) == CharL('\\') )
		{
			if( !(CountCharReverse(ptr - 2, str, CharL('\\')) & 0x1) )
			{
				continue;
			}
		}

		break;
	}
	return ptr;
}

template< typename CharT >
CharT const* TStringParse< CharT >::CheckAndSkipToCommentSectionEnd(CharT const* str)
{
	assert(str && *str == CharL('/'));

	if( str[1] == CharL('/') )
	{
		str = FindChar(str + 2, CharL('\n'));
	}
	else if( str[1] == CharL('*') )
	{
		str += 2;
		for(;;)
		{
			str = FindChar(str, CharL('*'));
			if( *str == 0 )
				break;
			if( str[1] == CharL('/') )
			{
				str += 1;
				break;
			}
			str += 1;
		}
	}

	return str;
}

template< typename CharT >
CharT const* TStringParse< CharT >::SkipChar(CharT const* str, CharT const* skipChars)
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

template< typename CharT >
CharT const* TStringParse< CharT >::SkipChar(CharT const* str, CharT skipChar)
{
	while( *str != 0 )
	{
		if( *str != skipChar )
			break;
		++str;
	}
	return str;
}

template< typename CharT >
CharT const* TStringParse< CharT >::SkipSpace(CharT const* str)
{
	CharT const* p = str;
	while( *p ) 
	{
		if ( !FCString::IsSpace(*p) )
			break;
		++p; 
	}
	return p;
}

template< typename CharT >
CharT const* TStringParse< CharT >::SkipToNextLine(CharT const* str)
{
	CharT const* nextLine = FindChar(str, CharL('\n'));
	if( *nextLine != 0 )
		++nextLine;
	return nextLine;
}

template< typename CharT >
CharT const* TStringParse< CharT >::SkipToChar(CharT const* str, CharT c, CharT cPair, bool bCheckComment, bool bCheckString)
{
	int countPair = 0;
	if( bCheckString )
	{
		if( bCheckComment )
		{
			for( ;;)
			{
				str = FindChar(str, c, cPair, CharL('/'), CharL('\"'));
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
				else if( *str == CharL('\"') )
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
				str = FindChar(str, c, cPair, CharL('\"'));
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
				else if( *str == CharL('\"') )
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
			str = FindChar(str, c, cPair, CharL('/'));
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

template< typename CharT >
CharT const* TStringParse< CharT >::SkipToChar(CharT const* str, CharT c, bool bCheckComment, bool bCheckString)
{
	if( bCheckString )
	{
		if( bCheckComment )
		{
			for( ;;)
			{
				str = FindChar(str, c, CharL('/'), CharL('\"'));
				if( *str == 0 )
					break;

				if( *str == c )
				{
					break;
				}
				else if( *str == CharL('\"') )
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
				str = FindChar(str, c, CharL('\"'));
				if( *str == 0 )
					break;

				if( *str == CharL('\"') )
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
			str = FindChar(str, c, CharL('/'));
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

template< typename CharT >
void TStringParse< CharT >::ReplaceChar(CharT* str, CharT c, CharT replace)
{
	CharT* ptr = str;
	while( *ptr != 0 )
	{
		if( *ptr == c )
			*ptr = replace;
		++ptr;
	}
}

template< typename CharT >
EStringToken TStringParse< CharT >::StringToken(CharT const*& inoutStr, DelimsTable const& table, TStringView<CharT>& outToken)
{
	CharT cur = *(inoutStr++);
	for( ;; )
	{
		if( cur == CharL('\0') )
		{
			return EStringToken::None;
		}
		if( !table.isDropDelims(cur) )
			break;

		cur = *(inoutStr++);
	}

	CharT const* ptr = inoutStr - 1;
	if( table.isStopDelims(cur) )
	{
		outToken = TStringView<CharT>(ptr, 1);
		return EStringToken::Delim;
	}

	for( ;;)
	{
		cur = *inoutStr;
		if( cur == CharL('\0') || table.isDelims(cur) )
			break;
		++inoutStr;
	}
	outToken = TStringView<CharT>(ptr, inoutStr - ptr);
	return EStringToken::Content;
}

template< typename CharT >
void TStringParse< CharT >::SkipDelims(CharT const*& inoutStr, DelimsTable const& table)
{
	for (;;)
	{
		CharT cur = *(inoutStr+1);
		if (cur == CharL('\0'))
		{
			return;
		}
		if (!table.isDropDelims(cur))
			break;

		++inoutStr;
	}
}

template< typename CharT >
EStringToken TStringParse< CharT >::StringToken(CharT const*& inoutStr, CharT const* dropDelims, CharT const* stopDelims, TStringView<CharT>& outToken)
{
	CharT cur = *(inoutStr++);
	for( ;; )
	{
		if( cur == CharL('\0') )
		{
			return EStringToken::None;
		}
		if (*FindChar(dropDelims, cur) == 0)
		{
			break;
		}

		cur = *(inoutStr++);
	}

	CharT const* ptr = inoutStr - 1;
	if( *FindChar(stopDelims, cur) != 0 )
	{
		outToken = TStringView<CharT>(ptr, 1);
		return EStringToken::Delim;
	}

	for( ;;)
	{
		cur = *inoutStr;
		if( cur == CharL('\0') || ( *FindChar(dropDelims, cur) ) || (*FindChar(stopDelims, cur) ) )
			break;
		++inoutStr;
	}

	outToken = TStringView<CharT>(ptr, inoutStr - ptr);
	return EStringToken::Content;
}

template< typename CharT >
bool TStringParse< CharT >::StringToken(CharT const*& inoutStr, CharT const* dropDelims, TStringView<CharT>& outToken)
{
	inoutStr = SkipChar(inoutStr, dropDelims);
	if( *inoutStr == 0 )
	{
		return false;
	}

	CharT const* ptr = inoutStr;
	inoutStr = FindChar(inoutStr, dropDelims);
	outToken = TStringView<CharT>(ptr, inoutStr - ptr);
	return true;
}


template< typename CharT >
TStringView<CharT> TStringParse< CharT >::StringTokenLine(CharT const*& inoutStr)
{
	CharT const* start = inoutStr;
	inoutStr = FindChar(inoutStr, CharL('\n'));
	CharT const* end = inoutStr;
	if( *end != 0 )
	{
		++inoutStr;
		if( *end == CharL('\n') )
		{
			if( end != start && *(end - 1) == CharL('\r') )
			{
				--end;
			}
		}
	}
	return TStringView<CharT>( start , end - start );
}

#undef CharL

template class TStringParse<char>;
template class TStringParse<wchar_t>;