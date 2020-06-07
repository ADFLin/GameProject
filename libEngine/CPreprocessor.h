#pragma once
#ifndef CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
#define CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685

#include "StringParse.h"
#include "HashString.h"

#include "Template/StaticString.h"

#include <vector>
#include <string>
#include <ostream>
#include <unordered_map>
#include <functional>
#include <unordered_set>

namespace CPP
{
	struct EOperatorPrecedence
	{
		enum Type
		{
			LogicalOR,
			LogicalAND,
			BitwiseOR,
			BitwiseXOR,
			BitwiseAND,
			Equality,
			Comparsion,
			Shift,
			Addition,
			Multiplication,
			Preifx,
		};
	};

#define BINARY_OPERATOR_LIST(op)\
	op( LogicalOR  , || , EOperatorPrecedence::LogicalOR )\
	op( LogicalAND , && , EOperatorPrecedence::LogicalAND )\
	op( BitwiseOR  , | , EOperatorPrecedence::BitwiseOR )\
	op( BitwiseXOR , ^ , EOperatorPrecedence::BitwiseXOR )\
	op( BitwiseAND , & , EOperatorPrecedence::BitwiseAND )\
	op( EQ , == , EOperatorPrecedence::Equality )\
	op( NEQ, != , EOperatorPrecedence::Equality )\
	op(	CompGE, >= , EOperatorPrecedence::Comparsion )\
	op( CompG , > , EOperatorPrecedence::Comparsion )\
	op( CompLE, <= , EOperatorPrecedence::Comparsion )\
	op( CompL , < , EOperatorPrecedence::Comparsion )\
	op( LShift, <<  , EOperatorPrecedence::Shift )\
	op( RShift, >> , EOperatorPrecedence::Shift )\
	op( Add , + , EOperatorPrecedence::Addition )\
	op( Sub , - , EOperatorPrecedence::Addition )\
	op( Mul , * , EOperatorPrecedence::Multiplication)\
	op( Div , / , EOperatorPrecedence::Multiplication)\
	op( Rem , % , EOperatorPrecedence::Multiplication)\

#define UNARY_OPERATOR_LIST(op)\
	op( Plus , + , EOperatorPrecedence::Preifx)\
	op( Minus , - , EOperatorPrecedence::Preifx)\
	op( LogicalNOT , ! , EOperatorPrecedence::Preifx)\
	op( BitwiseNOT , ~ , EOperatorPrecedence::Preifx)\
	op( Inc , ++ , EOperatorPrecedence::Preifx)\
	op( Dec , -- , EOperatorPrecedence::Preifx)\

	namespace EOperator
	{
		enum Type
		{
			None = -1,
#define ENUM_OP( NAME , OP , P ) NAME ,
			BINARY_OPERATOR_LIST(ENUM_OP)
			UNARY_OPERATOR_LIST(ENUM_OP)
#undef ENUM_OP

			BINARY_OPERATOR_COUNT = Plus,
		};
	};

	struct OperationInfo
	{
		EOperator::Type type;
		StaticString    text;
		EOperatorPrecedence::Type precedence;
	};

	class CodeOutput
	{
	public:
		CodeOutput(std::ostream& stream)
			:mStream(stream)
		{
		}

		void pushEoL() { push("\r\n", 2); }
		void pushSpace(int num = 1)
		{
			for( int i = 0; i < num; ++i )
				push(' ');
		}
		void push(StringView const& str) { push(str.data(), str.size()); }
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

	class FExpressionUitlity
	{
	public:
		static int FindOperator(char const* code, EOperatorPrecedence::Type precedence, EOperator::Type& outType);
		static int FindOperatorToEnd(char const* code, EOperatorPrecedence::Type precedenceStart, EOperator::Type& outType);
		static OperationInfo GetOperationInfo(EOperator::Type type);
	};

	class CodeLoc
	{
	public:

		void skipToNextLine()
		{
			for(;;)
			{
				mCur = FStringParse::FindChar(mCur, '\\', '\n');
				switch (*mCur)
				{
				case 0: return;
				case '\n':
					++mLineCount;
					++mCur;
					return;
				default:
					if (SkipConcat(mCur))
					{
						++mLineCount;
					}
					else
					{
						++mCur;
					}
					break;
				}
			}
		}

		void skipToLineEnd()
		{
			for (;;)
			{
				mCur = FStringParse::FindChar(mCur, '\\', '\n');
				switch (*mCur)
				{
				case 0: return;
				case '\n': return;
				default:
					if (SkipConcat(mCur))
					{
						++mLineCount;
					}
					else
					{
						++mCur;
					}
					break;
				}
			}
		}

		void skipSpaceInLine()
		{
			while (*mCur)
			{
				if (*mCur != ' ' && *mCur != '\t' && *mCur != '\r')
					break;

				advance();
			}
		}

		void advanceNoConcat()
		{
			if (mCur[0] == '\n')
			{
				++mLineCount;
			}

			++mCur;

			if (SkipConcat(mCur))
			{
				++mLineCount;
			}
		}

		void advanceNoEoL(int offset)
		{
			assert(FStringParse::CountChar(mCur, mCur + offset, '\n') == 0);

			mCur += offset;
			if (SkipConcat(mCur))
			{
				++mLineCount;
			}
		}

		void advance()
		{
			if (mCur[0] == '\n')
			{
				++mLineCount;
			}
			
			++mCur;

			if (SkipConcat(mCur))
			{
				++mLineCount;
			}
		}

		bool isEoF() const { return *mCur == 0; }
		bool isEoL() const { return *mCur == '\n'; }

		static bool SkipConcat(char const*& Code)
		{
			if (Code[0] == '\\')
			{
				if (Code[1] == '\n')
				{
					Code += 2;
					return true;
				}
				else if (Code[1] == '\r' && Code[2] == '\n')
				{
					Code += 3;
					return true;
				}
			}
			return false;
		}

		static void Advance(char const*& code)
		{
			++code;
			SkipConcat(code);
		}
		static bool Backward(char const*& code)
		{
			--code;
			if (code[0] == '\n')
			{
				if (code[-1] == '\\')
				{
					code -= 2;
					return true;
				}
				else if (code[-1] == '\r' && code[-2] == '\\')
				{
					code -= 3;
					return true;
				}
			}
			return false;
		}
		static bool SkipBOM(char const*& code)
		{
			if (code[0] == '\xef' &&
				code[1] == '\xbb' &&
				code[2] == '\xbF')
			{
				code += 3;
				return true;
			}

			return false;
		}

		char const* mCur;
		int mLineCount;
	};

	class CodeSource
	{
	public:
		void appendString(StringView const& str);
		bool loadFile(char const* path);

		HashString    filePath;
		int lineOffset = 0;
	private:

		std::vector< char > mBuffer;

		friend class Preprocessor;
		friend class CodeInput;
	};


	class CodeInput : public CodeLoc
	{
	public:
		CodeSource* source;

		int getLine() const
		{
#if _DEBUG
			int lineCountActual = countCharFormStart('\n');
			assert(lineCountActual == mLineCount);
#endif
			return mLineCount + source->lineOffset + 1;
		}

		void resetSeek()
		{
			assert(source);
			mCur = &source->mBuffer[0];
			//skip BOM

			SkipBOM(mCur);
			mLineCount = 0;
		}
		void setLoc(CodeLoc const& loc)
		{
			mCur = loc.mCur;
			mLineCount = loc.mLineCount;
		}

		int countCharFormStart(char c) const
		{
			return FStringParse::CountChar(&source->mBuffer[0], mCur, c);
		}
	private:
		friend class Preprocessor;
	};

	class SyntaxError : public std::exception
	{
	public:
		STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT(SyntaxError)
	};

	class Preprocessor
	{
	public:
		Preprocessor();
		~Preprocessor();

		void translate(CodeSource& sorce);
		void setOutput(CodeOutput& output);
		void addSreachDir(char const* dir);
		void addDefine(char const* name, int value);

		void getIncludeFiles(std::vector< HashString >& outFiles);

		bool bSupportMarcoArg = false;
		bool bReplaceMarcoText = false;
		bool bCanRedefineMarco = false;
		bool bCommentIncludeFileName = true;
		bool bAddLineMarco = true;

	private:


		enum EControlPraseResult
		{
			OK,
			ExitBlock,
			RetainText,
		};

		bool parseCode(bool bSkip);
		bool parseControlLine(EControlPraseResult& result);
		bool parseInclude();
		bool parseDefine();
		bool parseIf();
		bool parseIfdef();
		bool parseIfndef();
		bool parseIfInternal(int exprRet);

		bool parsePragma();
		bool parseError();
		bool parseUndef();
		bool parseDefined(int& ret);

		bool parseExpression(int& ret);
		bool parseExpression(CodeLoc const& loc, int& ret)
		{
			InputLocScope scope(*this, loc);
			return parseExpression(ret);
		}

		bool tokenOp(EOperatorPrecedence::Type precedence, EOperator::Type& outType);
		bool parseExprOp(int& ret , EOperatorPrecedence::Type precedence = EOperatorPrecedence::Type(0));
		bool parseExprFactor(int& ret);
		bool parseExprValue(int& ret);


		bool ensureInputValid();
		bool haveMore();
		bool skipToNextLine();
		bool skipSpaceInLine();
		bool tokenChar(char c);

		void replaceMarco(StringView const& text , std::string& outText );

		static bool IsValidStartCharForIdentifier(char c)
		{
			return ::isalpha(c) || c == '_';
		}

		static bool IsValidCharForIdentifier(char c)
		{
			return ::isalnum(c) || c == '_';
		}


		bool tokenIdentifier(StringView& outIdentifier);
		bool tokenNumber(int& outValue);
		bool tokenString(StringView& outString);
		bool tokenControl(StringView& outName);


		struct ScopeState
		{
			bool bSkipText;
		};
		std::vector< ScopeState > mScopeStack;
		
		bool isSkipText()
		{
			return mScopeStack.back().bSkipText;
		}
		int numLine = 0;
		bool warning(char const*)
		{



			return true;
		}


		struct MarcoSymbol
		{
			StringView  name;
			std::string expr;

			struct ArgEntry
			{
				int indexArg;
				int pos;
			};
			std::vector< ArgEntry > argEntries;

			int cacheEvalValue;
			int evalFrame;
		};

		int mIfScopeDepth;
		std::unordered_map< HashString, MarcoSymbol > mMarcoSymbolMap;
		int mCurFrame = 0;
		bool bRequestSourceLine = false;
		
		void emitSourceLine(int lineOffset = 0);
		void emitCode(StringView const& code);

		struct InputLocScope
		{	
			InputLocScope(Preprocessor& p, CodeLoc const& loc)
				:mP(p)
			{
				mPrevLoc = p.mInput;
				p.mInput.setLoc(loc);
			}

			~InputLocScope()
			{
				mP.mInput.setLoc(mPrevLoc);
			}
			Preprocessor& mP;
			CodeLoc mPrevLoc;

		};
		CodeInput mInput;
		struct  InputEntry
		{
			CodeInput input;
			std::function< void() > onChildFinish;
		};
		std::vector< InputEntry > mInputStack;


		bool findFile(std::string const& name, std::string& fullPath);

		CodeOutput* mOutput;

		MarcoSymbol* findMarco(StringView token)
		{
			auto iter = mMarcoSymbolMap.find(token);
			if( iter == mMarcoSymbolMap.end() )
				return nullptr;
			return &iter->second;
		}

		std::unordered_set< HashString >  mParamOnceSet;
		std::vector< std::string > mFileSreachDirs;
		std::unordered_set< HashString >  mUsedFiles;


		EOperator::Type mParsedCachedOP = EOperator::None;
		EOperatorPrecedence::Type mParesedCacheOPPrecedence;

		std::unordered_map< HashString, CodeSource* > mLoadedSourceMap;

		friend class ExpressionEvaluator;
	};



	class ExpressionEvaluator
	{
	public:
		ExpressionEvaluator(Preprocessor& preprocessor , CodeLoc const& loc)
			:mProcesser(preprocessor)
			,mLoc(loc)
		{



		}

		bool evaluate(int& ret)
		{



		}

		CodeLoc mLoc;
		Preprocessor& mProcesser;
	};

}


#endif // CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
