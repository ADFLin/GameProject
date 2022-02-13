#pragma once
#ifndef StringParse_H_B9D7C5CE_5A1A_4B11_94A1_4BC663FBA46F
#define StringParse_H_B9D7C5CE_5A1A_4B11_94A1_4BC663FBA46F

#include "Core/IntegerType.h"
#include "EnumCommon.h"
#include "MemorySecurity.h"
#include "Template/StringView.h"
#include "CompilerConfig.h"

#include <string>
#include <cstring>
#include <cassert>
#include <cstdlib>

class DelimsTable
{
public:
	DelimsTable();
	DelimsTable(char const* dropDelims , char const* stopDelims );
	enum
	{
		StopMask = 1 << 0,
		DropMask = 1 << 1,
	};
	void addDelims(char const* delims, uint8 mask);
	bool isDelims(char c) const;
	bool isStopDelims(char c) const;
	bool isDropDelims(char c) const;
private:
	//TODO:
	uint8 mDelimsMap[256];
};

class FStringParseBase
{
public:
	enum TokenType
	{
		eNoToken = 0,
		eStringType = 1,
		eDelimsType = 2,
	};
	enum
	{
		IntNumber,

		HexIntNumber,
		FloatNumber,
		DoubleNumber,
		ErrorNumber,
	};
};

template<class CharT>
class TStringParse : public FStringParseBase
{
public:
	static CharT const* FindLastChar(CharT const* str, int num, CharT c);
	static CharT const* FindLastChar(CharT const* str, CharT c)
	{
		return FindLastChar(str, FCString::Strlen(str), c);
	}
	static CharT const* FindChar(CharT const* str, CharT c);
	static CharT const* FindChar(CharT const* str, CharT c1, CharT c2);
	static CharT const* FindChar(CharT const* str, CharT c1, CharT c2, CharT c3);
	static CharT const* FindChar(CharT const* str, CharT c1, CharT c2, CharT c3, CharT c4);


	static CharT const* FindCharN(CharT const* str, int num, CharT c);
	static CharT const* FindCharN(CharT const* str, int num, CharT c1, CharT c2);
	static CharT const* FindCharN(CharT const* str, int num, CharT c1, CharT c2, CharT c3);
	static CharT const* FindCharN(CharT const* str, int num, CharT c1, CharT c2, CharT c3, CharT c4);

	static CharT const* FindChar(CharT const* str, CharT const* findChars);


	static CharT const* SkipChar(CharT const* str, CharT const* skipChars);
	static CharT const* SkipChar(CharT const* str, CharT skipChar);
	static CharT const* SkipSpace(CharT const* str);
	static CharT const* SkipToNextLine(CharT const* str);

	static CharT const* SkipToChar(CharT const* str, CharT c, bool bCheckComment, bool bCheckString);
	static CharT const* SkipToChar(CharT const* str, CharT c, CharT cPair, bool bCheckComment, bool bCheckString);
	static void ReplaceChar(CharT* str, CharT c, CharT replace);


	static TokenType StringToken(CharT const*& inoutStr, DelimsTable const& table, TStringView<CharT>& outToken);
	static void      SkipDelims(CharT const*& inoutStr, DelimsTable const& table);

	static TokenType StringToken(CharT const*& inoutStr, CharT const* dropDelims, CharT const* stopDelims, TStringView<CharT>& outToken);
	static bool      StringToken(CharT const*& inoutStr, CharT const* dropDelims, TStringView<CharT>& outToken);
	static TStringView<CharT> StringTokenLine(CharT const*& inoutStr);


	static int ParseNumber(CharT const* str, int& num);
	//
	static int ParseIntNumber(CharT const* str, int& num);

	static CharT const* TrySkipToStringSectionEnd(CharT const* str) { return TrySkipToSectionEnd(str, STRING_LITERAL(CharT, '\"')); }
	static CharT const* TrySkipToCharSectionEnd(CharT const* str) { return TrySkipToSectionEnd(str, STRING_LITERAL(CharT, '\'')); }
	static CharT const* CheckAndSkipToCommentSectionEnd(CharT const* str);

	static int CountChar(CharT const* start, CharT const* end, CharT c)
	{
		CharT const* p = start;
		int result = 0;
		while (p < end)
		{
			if (*p == c)
				++result;

			++p;
		}
		return result;
	}

private:
	static CharT const* TrySkipToSectionEnd(CharT const* str, CharT c);


};


class FStringParse : public TStringParse< char >
	               , public TStringParse< wchar_t >
{
public:
#define FUNCTION_LIST(op)\
	op(FindLastChar)\
	op(FindChar)\
	op(FindCharN)\
	op(SkipChar)\
	op(SkipSpace)\
	op(SkipToNextLine)\
	op(SkipToChar)\
	op(ReplaceChar)\
	op(StringToken)\
	op(SkipDelims)\
	op(StringTokenLine)\
	op(ParseNumber)\
	op(ParseIntNumber)\
	op(TrySkipToStringSectionEnd)\
	op(TrySkipToCharSectionEnd)\
	op(CheckAndSkipToCommentSectionEnd)\
	op(CountChar)\

#define ACCESS_OP( FUNC )\
	 using TStringParse< char >::FUNC;\
	 using TStringParse< wchar_t >::FUNC;

	FUNCTION_LIST(ACCESS_OP)

#undef ACCESS_OP
#undef FUNCTION_LIST
};


class Tokenizer : public DelimsTable
{
public:
	Tokenizer(char const* str, char const* dropDelims, char const* stopDelims = "");

	using TokenType = FStringParse::TokenType;

	void        reset(char const* str);
	char        nextChar() { return *mPtr; }
	void        offset(int off) { mPtr += off; }
	void        skipDropDelims();
	int         skipToChar(char c);

	bool        takeUntil( char stopChar, StringView& token);
	bool        takeChar(char c);

	TokenType  take(DelimsTable const& table , StringView& token);
	TokenType  take(StringView& token);

	int        calcOffset(char const* ptr)
	{
		return mPtr - ptr;
	}

	char const* mPtr;
};


class TokenException : public std::exception
{
public:
	STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( TokenException )
};


class StringTokenizer
{
public:
	StringTokenizer() { mNextToken = nullptr; }
	StringTokenizer(char* str) { begin(str); }

	void begin(char* str)
	{
		mStr = str;
		mNextToken = nullptr;
	}

	char* nextToken()
	{
		assert(mStr || mNextToken);
		char* out = strtok_s(mStr, " \r\t\n", &mNextToken);
		mStr = nullptr;
		return out;
	}

	char*   nextToken(char const* delim)
	{
		assert(mStr || mNextToken);
		char* out = strtok_s(mStr, delim, &mNextToken);
		mStr = nullptr;
		return out;
	}

	double  nextFloat()
	{
		char* token = nextToken();
		if( !token )
			throw TokenException("");
		return atof(token);
	}

	int     nextInt()
	{
		char* token = nextToken();
		if( !token )
			throw TokenException("");
		return atoi(token);
	}

private:
	char const* tokenInternal(char const* delim, char& charEnd)
	{




	}

	char*  mStr;
	char*  mNextToken;
};

#endif // StringParse_H_B9D7C5CE_5A1A_4B11_94A1_4BC663FBA46F
