#include "Stage/TestStageHeader.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "StringParse.h"
#include <sstream>

#if 1 \
 
#endif

#define MO_AA(a) a

int a = MO_AA (1);


char const* TestCode = CODE_STRING(

#define AA 1
#define BB 1

#if AA || BB
	int x = 1;
#elif BB
	int x = 2;
#endif

);

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

class FPreprocessParse
{
public:

	static char const* GetSymbolEndPos(char const* inStr)
	{
		for(;; )
		{	
			if( !isalpha(*inStr) && *inStr != '_' )
				break;

			++inStr;
		}
		return inStr;
	}

	static StringView GetSymbol(char const* inStr)
	{
		char const* posEnd = GetSymbolEndPos(inStr);
		return StringView(inStr, posEnd - inStr);
	}

	static char const* NextCommond(char const* inStr)
	{
		auto cur = inStr;
		while( *cur )
		{
			cur = FStringParse::FindChar(inStr, '#', '\"');
			if( *cur == '#' )
				break;
			if( *cur == '\"' )
			{
				cur = FStringParse::TrySkipToStringSectionEnd(cur);
			}
		}
		return cur;
	}

	static Command GetCommand(char const* inStr , StringView& commandText )
	{
		commandText = GetSymbol(inStr);

		static std::map< StringView , Command >  sCommandMap = 
		{
			{ "include", Command::Include  },
			{ "pragma", Command::Pragma  } ,
			{ "define", Command::Define },
			{ "if", Command::If   },
			{ "ifdef", Command::Ifdef  },
			{ "ifndef",Command::Ifndef  },
			{ "elif", Command::Elif   },
			{ "else" ,Command::Else  },
			{ "endif" ,Command::Endif   },
		};

		auto iter = sCommandMap.find(commandText);
		if( iter == sCommandMap.end() )
			return Command::None;

		return iter->second;

	}
};


class PreprocessorParser
{
public:

	void parse(char const* pCode)
	{
		mPosition.ptr = pCode;
		mPosition.column = 0;

	}
	struct CodePosition
	{

		void operator ++ ()
		{
			assert(*ptr != 0);
			if( *ptr == '\n' )
			{
				++column;
			}
			++ptr;
		}

		char operator [] (int idx) const { return *(ptr + idx); }
		char const* ptr;
		int column;
	};

	bool translate(char const* pCode)
	{
		mPosition.ptr = pCode;
		mPosition.column = 0;
	}

	CodePosition mPosition;

	struct MarcoSymbol
	{
		StringView name;
		std::string expr;
		std::vector< uint8 > paramEntry;
		int cacheEvalValue;
	};

	bool isCommandPreifx()
	{
		return (mPosition[0] == '#');
	}
	bool isCommand(char const* name)
	{
		assert(isCommandPreifx());

		StringView symbol = FPreprocessParse::GetSymbol(mPosition.ptr + 1);
		if( !symbol.compare(name) == 0 )
			return false;

		return true;
	}
	bool parseCommandMatch(char const* name)
	{
		assert(isCommandPreifx());

		StringView symbol = FPreprocessParse::GetSymbol(mPosition.ptr + 1);
		if( !symbol.compare(name) == 0 )
			return false;

		mPosition.ptr += 1 + symbol.size();
		return true;
	}


	bool parseExpression(int& outReturnValue)
	{


	}

	bool skipToNextCommand()
	{



	}

	bool skipLine()
	{
		mPosition.ptr = FStringParse::SkipToNextLine(mPosition.ptr);
		return true;
	}

	bool skipSpace()
	{
		mPosition.ptr = FStringParse::SkipSpace(mPosition.ptr);
		return true;
	}

	bool parseIfStatement()
	{
		if( !parseCommandMatch("if") )
			return false;
		
		skipSpace();
		
		int returnValue;
		if( !parseExpression(returnValue) )
		{
			throw std::exception();
		}

		if( returnValue )
		{
			parseBlock();
		}

		if( parseCommandMatch("elif") )
		{
			if( returnValue )
			{

			}
		}

		if( parseCommandMatch("else") )
		{


		}

		if( !parseCommandMatch("endif") )
		{
			throw std::exception();
		}
		return true;
	}



	bool parsePragma()
	{
		if( !parseCommandMatch("pargma") )
			return false;

		skipLine();
		return true;
	}

	bool parseDefine()
	{
		if( !parseCommandMatch("define") )
			return false;

		skipSpace();
		StringView symbol = FPreprocessParse::GetSymbol(mPosition.ptr);

		if( symbol.size() == 0 )
		{
			throw std::exception();
		}

		mPosition.ptr += symbol.size();

		if( mPosition[0] == '(' )
		{
			//TODO
			mPosition.ptr = FStringParse::FindChar(mPosition.ptr, ')');
			if( mPosition[0] != 0 )
				++mPosition;
		}
		else
		{
			int value;
		}
	}


	bool evalExpression(StringView const& expression , int& outValue)
	{
		static DelimsTable table{" \t", "+-*/\n" };


	}

	bool isEOF()
	{
		return mPosition[0] == 0;
	}

	bool skipExpression()
	{
		//#TODO : parse connect '\'
		return skipLine();
	}

	bool skipCommand()
	{
		assert(mPosition[0] == '#');
	}

	bool skipIfBlock()
	{
		int stack = 0;

		skipSpace();

		while( !isEOF() )
		{
			if( !isCommandPreifx() )
			{
				skipToNextCommand();
			}
			
			if( stack == 0 )
			{
				if( isCommand("elif") || isCommand("else") || isCommand("endif") )
					return true;
			}
			else
			{
				if( parseCommandMatch("endif") )
				{
					--stack;
				}
				else
				{
					skipCommand();
				}
			}
		}
		return true;

	}
	bool parseBlock()
	{
		while( !isEOF() )
		{
			skipSpace();
			if( isCommandPreifx() )
			{
				if( parseIfStatement() || parsePragma() || parsePragma() )
				{

				}
				else
				{
					return true;
				}
			}
			else
			{
				parseCodeContent();
			}

		}
		return true;
	}
	bool parseCodeContent()
	{
		if( *mPosition.ptr == 0 )
			return false;

		char const* nextCommandPos = FPreprocessParse::NextCommond(mPosition.ptr);

		if( nextCommandPos == nextCommandPos )
			return false;

		StringView codeContent{ mPosition.ptr , size_t(nextCommandPos - mPosition.ptr) };




		mPosition.ptr = nextCommandPos;
		return true;
	}
};

void PreprocessTest()
{

}

REGISTER_MISC_TEST_INNER("PreprocessTest", PreprocessTest);