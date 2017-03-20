#pragma once
#ifndef CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
#define CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685

#include "StringParse.h"

#include <vector>
#include <set>
#include <map>
#include <string>
#include <ostream>

namespace CPP
{
	class CodeOutput
	{
	public:
		CodeOutput(std::ostream& stream)
			:mStream(stream)
		{
		}

		void pushNewline() { push('\n'); }
		void pushSpace(int num = 1)
		{
			for( int i = 0; i < num; ++i )
				push(' ');
		}
		void push(TokenString const& str) { push(str.ptr, str.num); }
		void push(char const* str)
		{
			push(str, strlen(str));
		}

		void push(char c)
		{
			mStream.write(&c, 1);
		}

		void push(char const* str, int num)
		{
			mStream.write(str, num);
		}
		std::ostream& mStream;

	};


	class CodeInput
	{
	public:
		void appendString(char const* str)
		{
			if ( !mBuffer.empty() )
				mBuffer.pop_back();
			mBuffer.insert(mBuffer.end(), str, str + strlen(str));
			mBuffer.push_back(0);
		}
		bool loadFile(char const* path);
		void reset()
		{
			mCur = &mBuffer[0];
		}
		void skipLine()
		{
			mCur = ParseUtility::FindChar(mCur, '\n');
			if( *mCur != 0 )
				++mCur;
		}

		std::vector< char > mBuffer;
		std::string   mFileName;
		char const*   mCur;
	};

	enum TokenType
	{

		Token_String ,
		Token_Eof ,
	};

	struct TokenInfo
	{
		TokenInfo(){}
		TokenInfo(TokenString inStr)
		{
			type = Token_String;
			str = inStr;
		}

		TokenType type;

		union
		{
			char delimChar;
			TokenString str;
		};

	};

	class SyntalError : public std::exception
	{
	public:
		SyntalError( char const* str ):
			std::exception( str ){}
	};

	enum class Command
	{
		Include,
		Define,
		If,
		Ifdef,
		Ifndef,
		Elif,
		Else,
		Endif,
		Pragma,

		None,
	};

	struct DefineSymbol
	{
		TokenString name;
		std::string expr;
		std::vector< uint8 > paramEntry;
	};


	class Preprocessor
	{
	public:
		Preprocessor();
		void translate( CodeInput& input );
		void setOutput(CodeOutput& output);
		void addSreachDir(char const* dir);
		void define(char const* name, int value){}

		bool bSupportMarco = false;

	private:

		TokenInfo nextToken(CodeInput& input);



		void execInclude(CodeInput& input);


		void execIf(CodeInput& input);

		bool evalExpression( std::string const& expr , int& outValue )
		{
			//#TODO:support \ connect
			return true;
		}

		struct StrCmp
		{
			bool operator()(TokenString const& lhs, TokenString const& rhs) const
			{
				return lhs.compare(rhs) < 0;
			}
		};
		static std::map< TokenString, Command , StrCmp > sCommandMap;

		Command nextCommand(CodeInput& input, bool bOutString, char const*& comStart);

		std::string getExpression(CodeInput& input)
		{
			return std::string("");
		}


		bool evalNeedOutput()
		{
			return true;
		}

		bool findFile(std::string const& name, std::string& fullPath);

		int mScopeCount;
		DelimsTable   mDelimsTable;
		CodeOutput* mOutput;
		std::set< DefineSymbol > mDefineSymbolSet;

		std::set< std::string >  mParamOnceSet;

		struct State
		{
			bool bEval;
			bool bHaveElse;
		};

		std::vector< State >       mStateStack;
		std::vector< std::string > mFileSreachDirs;
	};
}


#endif // CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
