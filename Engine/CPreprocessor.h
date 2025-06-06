#pragma once
#ifndef CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
#define CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685

#include "StringParse.h"
#include "HashString.h"
#include "LogSystem.h"
#include "Template/ConstString.h"
#include "Template/ArrayView.h"
#include "Template/StringView.h"
#include "DataStructure/Array.h"

#include <string>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <functional>


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
		ConstString text;
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
		//void push(ConstString const& str) { push(str.c_str(), str.size()); }
		void push(StringView const& str) { push(str.data(), str.size()); }
		void push(char c)
		{
			mStream.write(&c, 1);
		}

		void push(char const* str, size_t num)
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
		static constexpr OperationInfo GetOperationInfo(EOperator::Type type);

		template< EOperatorPrecedence::Type Precedence >
		static int FindOperator(char const* code, EOperator::Type& outType);
	};

	class FCodeParse
	{
	public:
		static bool IsValidStartCharForIdentifier(char c)
		{
			if (c < 0)
				return false;
			return FCString::IsAlpha(c) || c == '_';
		}

		static bool IsValidCharForIdentifier(char c)
		{
			if (c < 0)
				return false;

			return FCString::IsAlphaNumeric(c) || c == '_';
		}

		static int SkipConcat(char const*& Code)
		{
			if (Code[0] == '\\')
			{
				if (Code[1] == '\n')
				{
					Code += 2;
					return 2;
				}
				else if (Code[1] == '\r' && Code[2] == '\n')
				{
					Code += 3;
					return 3;
				}
			}
			return 0;
		}

		static int Advance(char const*& code)
		{
			++code;
			return 1 + SkipConcat(code);
		}

		static int Backward(char const*& code)
		{
			--code;
			if (code[0] == '\n')
			{
				if (code[-1] == '\\')
				{
					code -= 2;
					return 2;
				}
				else if (code[-1] == '\r' && code[-2] == '\\')
				{
					code -= 3;
					return 3;
				}
			}
			return 0;
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
	};

	template< class T >
	class CodeTokenUtiliyT : public FCodeParse
	{
	public:
		T* _this() { return static_cast<T*>(this); }

		void advance();
		char const* getCur();
		void skipToLineEnd();

		char getChar() { return *_this()->getCur(); }

		bool tokenChar(char c)
		{
			if (_this()->getChar() != c)
				return false;

			_this()->advance();
			return true;
		}

		bool tokenIdentifier(StringView& outIdentifier)
		{
			if (!IsValidStartCharForIdentifier(_this()->getChar()))
				return false;
			
			char const* start = _this()->getCur();
			do
			{
				_this()->advance();
			} 
			while (IsValidCharForIdentifier(_this()->getChar()));

			outIdentifier = StringView(start, _this()->getCur() - start);
			return true;
		}

		bool tokenNumber(int& outValue)
		{
			char const* start = _this()->getCur();

			if (!FCString::IsDigit(_this()->getChar()))
				return false;

			for (;;)
			{
				_this()->advance();
				if (!FCString::IsDigit(_this()->getChar()))
					break;
			}

			outValue = FStringConv::To<int>(start, _this()->getCur() - start);
			return true;

		}

		bool tokenLineString(StringView& outString, bool bTrimEnd = true)
		{
			char const* start = _this()->getCur();
			_this()->skipToLineEnd();
			char const* end = _this()->getCur();

			if (end == start)
				return false;

			if ( bTrimEnd )
			{
				while (end != start)
				{
					Backward(end);

					if (!FCString::IsSpace(*end))
						break;
				}
			}

			outString = StringView(start, end - start + 1);
			return true;
		}

		void skipSpaceInLine()
		{
			while (char c = _this()->getChar())
			{
				if (c != ' ' &&  c != '\t' && c != '\r')
					break;

				_this()->advance();
			}
		}
	};


	class CodeLoc : public CodeTokenUtiliyT< CodeLoc >
	{
	public:
		bool isEoF() const { return *mCur == 0; }
		bool isEoL() const { return *mCur == '\n' || (mCur[0] == '\r' && mCur[1] == '\n'); }

		char const* getCur() { return mCur; }

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

		void advanceNoEoL(int offset)
		{
			CHECK(FStringParse::CountChar(mCur, mCur + offset, '\n') == 0);

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

		char operator [](int index) const { return mCur[index]; }

		StringView getDifference(CodeLoc const& start) const
		{
			return StringView(start.mCur, mCur - start.mCur);
		}

		char const* mCur;
		int mLineCount;
	};

	class CodeSource
	{
	public:

		virtual int getLineOffset() { return 0; }
		virtual char const* getCode() { return nullptr; }
		virtual HashString  getFilePath() { return HashString(); }
		virtual char const* getSourceName() { return ""; }
	};


	class CodeStringSource : public CodeSource
	{
	public:

		virtual int getLineOffset() { return 0; }
		virtual char const* getCode() { return mString.data(); }
		virtual HashString  getFilePath() { return HashString(); }
		virtual char const* getSourceName() { return ""; }

		StringView mString;
	};

	class CodeBufferSource : public CodeSource
	{
	public:
		bool loadFile(char const* path);

		void appendString(StringView const& str);
		bool appendFile(char const* path);

		TArrayView< uint8 const > getBuffer() { return mBuffer; }


		virtual int getLineOffset(){  return lineOffset;  }
		virtual char const* getCode() {  return (char const*)mBuffer.data();  }
		virtual HashString  getFilePath() { return filePath; }
		virtual char const* getSourceName() { return filePath.c_str(); }
		HashString    filePath;
		int lineOffset = 0;
	private:

		TArray< uint8 > mBuffer;

		friend class Preprocessor;
		friend class CodeInput;
	};


	class CodeInput : public CodeLoc
	{
	public:
		CodeSource* source;


		char const* getSourceName()
		{
			if (source)
				return source->getSourceName();
			return "";
		}

		int getLine() const
		{
#if _DEBUG
			int lineCountActual = countCharFormStart('\n');
			assert(lineCountActual == mLineCount);
#endif
			return mLineCount + source->getLineOffset() + 1;
		}

		void resetSeek()
		{
			assert(source);
			mCur = source->getCode();
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
			return FStringParse::CountChar(source->getCode(), mCur, c);
		}
		
	private:
		friend class Preprocessor;
	};

	class SyntaxError : public std::exception
	{
	public:
		STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT(SyntaxError)
	};

	class CodeSourceLibrary
	{
	public:

		~CodeSourceLibrary();

		CodeBufferSource* findOrLoadSource(HashString const& path);

		void cleanup();

		std::unordered_map< HashString, CodeBufferSource* > mSourceMap;
	};

	class Preprocessor
	{
	public:
		Preprocessor();
		~Preprocessor();

		void pushInput(CodeSource& sorce);
		void translate();
		void translate(CodeSource& sorce);
		void setOutput(CodeOutput& output);
		void setSourceLibrary(CodeSourceLibrary& sourceLibrary);
		void addSreachDir(char const* dir);
		void addDefine(char const* name, int value);
		void addInclude(char const* fileName, bool bSystemPath = false);

		void getUsedIncludeFiles(std::unordered_set< HashString >& outFiles);

		bool bSupportMarcoArg = true;
		bool bReplaceMarcoText = false;
		bool bAllowRedefineMarco = false;
		bool bCommentIncludeFileName = true;
		bool bAddLineMarco = true;
		enum LineFormat
		{
			LF_LineNumber,
			LF_LineNumberAndFilePath,
		};
		LineFormat lineFormat;
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

		bool parseConditionExpression(int& ret);

		bool parseExpression(int& ret);
		bool parseExpression(CodeLoc const& loc, int& ret)
		{
			InputLocScope scope(*this, loc);
			return parseExpression(ret);
		}

		bool tokenOp(EOperatorPrecedence::Type precedence, EOperator::Type& outType);
		bool parseExprOp(int& ret , EOperatorPrecedence::Type precedence = EOperatorPrecedence::Type(0));
		
		template< EOperatorPrecedence::Type Precedence >
		bool tokenOp(EOperator::Type& outType);
		template< EOperatorPrecedence::Type Precedence >
		bool parseExprOp(int& ret);


		bool parseExprFactor(int& ret);
		bool parseExprValue(int& ret);

		bool ensureInputValid();
		bool IsInputEnd();
		bool skipToNextLine();


		bool expandMarco(StringView const& lineText, std::string& outText );

		struct ExpandMarcoResult
		{
			int numRefMarco;
			int numRefMarcoWithArgs;

			ExpandMarcoResult()
			{
				numRefMarco = 0;
				numRefMarcoWithArgs = 0;
			}

			void append(ExpandMarcoResult const& other)
			{
				numRefMarco += other.numRefMarco;
				numRefMarcoWithArgs += other.numRefMarcoWithArgs;
			}

			bool isExpanded() const { return !!numRefMarco || !!numRefMarcoWithArgs; }
		};
		bool expandMarco(StringView const& lineText, std::string& outText, ExpandMarcoResult& outExpandResult);


		bool checkIdentifierToExpand(StringView const& id, class LineStringViewCode& code, std::string& outText, ExpandMarcoResult& outResult);
		bool expandMarcoInternal(class LineStringViewCode& code, std::string& outText, ExpandMarcoResult& outResult);

		bool tokenControl(StringView& outName);


		struct BlockState
		{
			bool bSkipText;
		};
		TArray< BlockState > mBlockStateStack;

		struct BlockScope
		{
			BlockScope(Preprocessor& preprocessor, BlockState const& state)
				:preprocessor(preprocessor)
			{
				preprocessor.mBlockStateStack.push_back(state);
			}

			~BlockScope()
			{
				preprocessor.mBlockStateStack.pop_back();
			}

			Preprocessor& preprocessor;
		};
		
		bool isSkipText()
		{
			return mBlockStateStack.back().bSkipText;
		}

		bool emitWarning(char const* msg)
		{
			LogWarning(0, msg);
			return true;
		}

		bool emitError(char const* msg)
		{
			throw SyntaxError(msg);
		}

		struct MarcoSymbol
		{
			//StringView  name;
			std::string expr;
			int numArgs;

			uint32 flags;
			enum EFlags
			{
				ConstEvalValue = BIT(0),
			};

			struct ArgEntry
			{
				int indexArg;
				int offset;
			};
			TArray< ArgEntry > argEntries;
			ArgEntry vaArgs;
			bool bVaEatComma;

			int cachedEvalValue;
			int evalFrame;
		};

		MarcoSymbol* findMarco(StringView const& name);

		MarcoSymbol* findMarcoInLineText(StringView& inoutText)
		{




		}


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
		TArray< InputEntry > mInputStack;


		bool findFile(std::string const& name, std::string& fullPath);

		CodeOutput* mOutput;

		std::unordered_set< HashString >  mPragmaOnceSet;
		TArray< std::string > mFileSreachDirs;
		std::unordered_set< HashString >  mUsedFiles;
		EOperator::Type mParsedCachedOP = EOperator::None;
		EOperatorPrecedence::Type mParesedCacheOPPrecedence;

		CodeSourceLibrary* mSourceLibrary = nullptr;
		bool mbSourceLibraryManaged = false;
		friend class ExpressionEvaluator;
	};


	class ExpressionEvaluator
	{
	public:
		ExpressionEvaluator(Preprocessor& preprocessor, CodeLoc const& loc)
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

BITWISE_RELLOCATABLE_FAIL(CPP::Preprocessor::InputEntry);

#endif // CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
