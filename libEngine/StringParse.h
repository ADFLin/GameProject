#pragma once
#ifndef StringParse_H_B9D7C5CE_5A1A_4B11_94A1_4BC663FBA46F
#define StringParse_H_B9D7C5CE_5A1A_4B11_94A1_4BC663FBA46F

#include <string>
#include <cassert>
#include <cstring>
#include <cstdlib>

#include "Core/IntegerType.h"
#include "EnumCommon.h"
#include "MemorySecurity.h"
#include "Template/StringView.h"


class DelimsTable
{
public:
	DelimsTable();

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

class FStringParse
{
public:
	static char const* FindLastChar(char const* str, int num, char c);
	static char const* FindLastChar(char const* str, char c)
	{
		return FindLastChar(str, strlen(str), c);
	}
	static char const* FindChar(char const* str, char c);
	static char const* FindChar(char const* str, char c1, char c2);
	static char const* FindChar(char const* str, char c1, char c2, char c3);
	static char const* FindChar(char const* str, char c1, char c2, char c3, char c4);
	static char const* FindChar(char const* str, char const* findChars);


	static char const* SkipChar(char const* str, char const* skipChars);
	static char const* SkipSpace(char const* str);
	static char const* SkipToNextLine(char const* str);

	static char const* SkipToChar(char const* str, char c, bool bCheckComment, bool bCheckString);
	static char const* SkipToChar(char const* str, char c, char cPair, bool bCheckComment, bool bCheckString);
	static void ReplaceChar(char* str, char c, char replace);

	enum TokenType
	{
		eNoToken = 0,
		eStringType = 1,
		eDelimsType = 2,
	};
	static TokenType StringToken(char const*& inoutStr, DelimsTable const& table, StringView& outToken);

	static TokenType StringToken(char const*& inoutStr, char const* dropDelims, char const* stopDelims, StringView& outToken);
	static bool      StringToken(char const*& inoutStr, char const* dropDelims, StringView& outToken);
	static StringView StringTokenLine(char const*& inoutStr);

	enum
	{
		IntNumber,

		HexIntNumber,
		FlotNumber,
		DoubleNumber,
		ErrorNumber,
	};
	static int ParseNumber(char const* str, int& num);
	//
	static int ParseIntNumber(char const* str, int& num);

	static char const* TrySkipToStringSectionEnd(char const* str) { return TrySkipToSectionEnd(str, '\"'); }
	static char const* TrySkipToCharSectionEnd(char const* str) { return TrySkipToSectionEnd(str, '\''); }
	static char const* CheckAndSkipToCommentSectionEnd(char const* str);

private:
	static char const* TrySkipToSectionEnd(char const* str, char c);

};


class Tokenizer : public DelimsTable
{
public:
	Tokenizer(char const* str, char const* dropDelims, char const* stopDelims = "");

	typedef FStringParse::TokenType TokenType;

	void        reset(char const* str);
	char const* next() { return mPtr; }
	void        offset(int off) { mPtr += off; }
	void        skipDropDelims();


	TokenType  take(StringView& str);

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
	StringTokenizer() { mNextToken = NULL; }
	StringTokenizer(char* str) { begin(str); }

	void begin(char* str)
	{
		mStr = str;
		mNextToken = NULL;
	}

	char* nextToken()
	{
		assert(mStr || mNextToken);
		char* out = strtok_s(mStr, " \r\t\n", &mNextToken);
		mStr = NULL;
		return out;
	}

	char*   nextToken(char const* delim)
	{
		assert(mStr || mNextToken);
		char* out = strtok_s(mStr, delim, &mNextToken);
		mStr = NULL;
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
