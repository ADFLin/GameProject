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
		void push(StringView const& str) { push(str.data(), str.size()); }
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
		void resetSeek()
		{
			mCur = &mBuffer[0];
		}
		void skipLine()
		{
			mCur = FStringParse::FindChar(mCur, '\n');
			if( *mCur != 0 )
				++mCur;
		}
		void skipSpace()
		{
			mCur = FStringParse::SkipSpace(mCur);
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
		TokenInfo(StringView inStr)
		{
			type = Token_String;
			str = inStr;
		}

		TokenType type;

		union
		{
			char delimChar;
			StringView str;
		};

	};

	class SyntaxError : public std::exception
	{
	public:
		STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( SyntaxError )
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

	struct MarcoSymbol
	{
		StringView name;
		std::string expr;
		std::vector< uint8 > paramEntry;
		int cacheEvalValue;
		int EvalFrame;
	};

	class Preprocessor
	{
	public:
		Preprocessor();
		~Preprocessor();

		void translate( CodeInput& input );
		void setOutput( CodeOutput& output);
		void addSreachDir(char const* dir);
		void define(char const* name, int value){}

		bool bSupportMarcoArg = false;
		bool bCanRedefineMarco = false;
		bool bCommentIncludeFileName = true;

	private:

		TokenInfo nextToken(CodeInput& input);



		bool execInclude(CodeInput& input);
		bool execIf(CodeInput& input);
		bool execDefine(CodeInput& input);

		bool execDefined(CodeInput& input)
		{

		}


		bool execEvalCommad(CodeInput& input, Command& outCommand)
		{


			return false;
		}

		bool execCommand(CodeInput& input , char const* str)
		{
			return false;
		}

		bool execStatement(CodeInput& input)
		{
			Command com;
			bool bExecIf = false;
			bool retVal = false;
			if( execCommand(input, "if") )
			{
				bExecIf = true;
				int evalValue;
				if( !execExpression(input, evalValue) )
				{

				}
				retVal = evalValue != 0;
			}
			else if( execCommand(input, "ifdef") )
			{
				retVal = execDefined(input);
			}

			if( bExecIf )
			{
				while( retVal == false)
				for (;;)
				{
					if( retVal )
					{
						if( !execStatement(input) )
						{

						}
						break;
					}

					if( execCommand(input, "elif") )
					{

					
					}
					else
					{
						
					}

					int evalValue;
					if( !execExpression(input, evalValue) )
					{

					}
					retVal = evalValue != 0;
				}
			}
			
			retVal = execBlock(input);

			if ( !execCommand(input, "endif") )
			{


			}

			return true;
		}

		bool execExpression(CodeInput& input, int& evalValue)
		{
			return false;
		}
		bool isCommand(CodeInput& input , Command& com )
		{

			return false;
		}
		bool execBlock(CodeInput& input)
		{
			Command com;
			if( isCommand( input , com ) )
			{
				switch( com )
				{
				case Command::Include:
					if( !execInclude(input) )
						return false;
					break;
				case Command::Define:
					break;
				case Command::If:
					break;
				case Command::Ifdef:
					{
						if( execDefined(input) )
						{


						}



					}
					break;
				case Command::Endif:
					break;
				case Command::Pragma:
					break;
				}
			}
			return true;
		}

		bool execCommand(CodeInput& input, Command com)
		{



		}

		bool execExpression( std::string const& expr , int& outValue )
		{
			//#TODO:support \ connect
			return true;
		}

		struct StrCmp
		{
			bool operator()(StringView const& lhs, StringView const& rhs) const
			{
				return lhs.compare(rhs) < 0;
			}
		};
		static std::map< StringView, Command , StrCmp > sCommandMap;

		Command nextCommand(CodeInput& input, bool bOutString, char const*& comStart);

		bool evalNeedOutput()
		{
			return true;
		}

		bool findFile(std::string const& name, std::string& fullPath);

		int mScopeCount;
		DelimsTable mDelimsTable;
		DelimsTable mExprDelimsTable;
		CodeOutput* mOutput;

		std::map< StringView , MarcoSymbol , StrCmp > mMarcoSymbolMap;

		MarcoSymbol* findMarco(StringView token)
		{
			auto iter = mMarcoSymbolMap.find(token);
			if( iter == mMarcoSymbolMap.end() )
				return nullptr;
			return &iter->second;
		}
		std::set< std::string >  mParamOnceSet;

		struct State
		{
			bool bEval;
			bool bHaveElse;
		};

		std::vector< State >       mStateStack;
		std::vector< std::string > mFileSreachDirs;
		std::vector< CodeInput* >  mLoadedInput;




		//expr eval
		struct ExprToken
		{
			enum Type
			{
				eOP,
				eString,
				eNumber,
				eEof,
			} type;

			union
			{
				StringView string;
				int value;
			};

			ExprToken(Type inType,int inValue = 0):type(inType),value(inValue){}
			ExprToken(StringView inString):type(eString),string(inString){}


		};

		enum OpType
		{
			OP_EQ = 1,
			OP_BEQ ,
			OP_SEQ ,
			OP_NEQ ,
			OP_LOFFSET,
			OP_ROFFSET,
		};

		ExprToken nextExprToken(CodeInput& input);

		struct EvalRet
		{
			int value;
		};

		
		struct OperatorInfo
		{
			char value;
			bool bRightAssoc;
			int  prodence;
		};
		bool eval_Compare(ExprToken token , CodeInput& input)
		{
			if( token.type == ExprToken::eOP )
			{
				int lvalue , rvalue;
				switch( token.value )
				{
				case '>': 
					lvalue = eval_Compare( nextExprToken(input) , input );
				case '<':
				case OP_BEQ:
				case OP_SEQ:
				case OP_EQ:
				case OP_NEQ:
					break;
				}
			}
			return 0;
		}
		bool eval_MulDiv(ExprToken token, CodeInput& input)
		{

			return 0;
		}
		bool eval_Atom(ExprToken token , CodeInput& input);
	};
}


#endif // CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
