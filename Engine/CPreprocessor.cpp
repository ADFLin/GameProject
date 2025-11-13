#include "CPreprocessor.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "InlineString.h"
#include "Holder.h"
#include "Core/StringConv.h"
#include "Core/FNV1a.h"


static InlineString<512> GMsg;
#define PARSE_ERROR( MSG , ... ) \
	{\
		GMsg.format( "%s (%d) - " MSG , mInput.getSourceName() , mInput.getLine() , ##__VA_ARGS__ );\
		emitError( GMsg.c_str() );\
	}
#define PARSE_WARNING( MSG , ... )\
	{\
		GMsg.format( "%s (%d) - " MSG , mInput.getSourceName() , mInput.getLine() , ##__VA_ARGS__ );\
		emitWarning( GMsg.c_str() );\
	}

#define FUNCTION_CHECK( func ) func

#define PARSE_DEFINE_IF 1
#define USE_TEMPLATE_PARSE_OP 1
#define USE_OPERATION_CACHE 0 && (!USE_TEMPLATE_PARSE_OP)

namespace CPP
{
	static constexpr uint32 HashValue(StringView str)
	{
		return FNV1a::MakeStringHash<uint32>(str.data(), str.length()) & 0xffff;
	}

	Preprocessor::Preprocessor()
	{
		mInput.source = nullptr;
	}

	Preprocessor::~Preprocessor()
	{
		if (mbSourceLibraryManaged)
		{
			delete mSourceLibrary;
		}
	}

	void Preprocessor::pushInput(CodeSource& sorce)
	{
		if (mInput.source)
		{
			InputEntry entry;
			entry.input = mInput;
#if 0
			entry.onChildFinish = [this, savedPath = path.toStdString()]()
			{
				mOutput->pushEoL();
				if (bCommentIncludeFileName)
				{
					mOutput->push("//~Include ");
					mOutput->push(savedPath);
					mOutput->pushEoL();
				}

				bRequestSourceLine = true;
			};
#endif

			mInputStack.push_back(entry);
		}

		mInput.source = &sorce;
		mInput.resetSeek();
	}

	void Preprocessor::translate()
	{
		mBlockStateStack.clear();
		mIfScopeDepth = 0;
		parseCode(false);
	}

	void Preprocessor::translate(CodeSource& sorce)
	{
		mInputStack.clear();
		mInput.source = &sorce;
		mInput.resetSeek();

		mBlockStateStack.clear();
		mIfScopeDepth = 0;

		parseCode(false);
	}

	bool Preprocessor::parseCode(bool bSkip)
	{
		if (!mBlockStateStack.empty())
		{
			bSkip |= mBlockStateStack.back().bSkipText;

			if (!bSkip)
			{
				bRequestSourceLine = true;
			}
		}

		BlockState state;
		state.bSkipText = bSkip;
		BlockScope blockScope(*this, state);

		while (!IsInputEnd())
		{
			EControlPraseResult result = EControlPraseResult::OK;
			CodeLoc start = mInput;
			if (parseControlLine(result))
			{
				if (result == EControlPraseResult::ExitBlock)
				{
					break;
				}
				else if (result == EControlPraseResult::RetainText && !bSkip)
				{
					mInput.setLoc(start);
					mInput.skipToNextLine();
					emitCode(mInput.getDifference(start));
				}
			}
			else if (!bSkip)
			{
				mInput.setLoc(start);
				mInput.skipToNextLine();

				if (bReplaceMarcoText)
				{
					StringView lineText = mInput.getDifference(start);
					std::string text;
					expandMarco(lineText, text);
					emitCode(StringView(text));
				}
				else
				{
					emitCode(mInput.getDifference(start));
				}
			}
			else
			{
				mInput.skipToNextLine();
			}
		}

		return true;
	}

	bool Preprocessor::parseControlLine(EControlPraseResult& result)
	{
		mInput.skipSpaceInLine();

		CodeLoc saveLoc = mInput;

		if (!mInput.tokenChar('#'))
			return false;

		mInput.skipSpaceInLine();

		StringView controlStr;
		if (!mInput.tokenIdentifier(controlStr))
			return false;

		switch (HashValue(controlStr))
		{
		case HashValue("define"):
#if PARSE_DEFINE_IF
			if (isSkipText())
			{
				skipToNextLine();
			}
			else
			{
				if (!parseDefine())
				{
					return false;
				}

				if (bReplaceMarcoText)
				{
					result = EControlPraseResult::OK;
				}
				else
				{
					result = EControlPraseResult::RetainText;
				}
			}
#else
			result = EControlPraseResult::RetainText;
#endif
			break;
		case HashValue("include"):
			if (isSkipText())
			{
				skipToNextLine();
			}
			else
			{
				if (!parseInclude())
				{

				}
			}
			break;
		case HashValue("if"):
#if PARSE_DEFINE_IF
			if (!parseIf())
			{
				result = EControlPraseResult::RetainText;
			}
#else
			result = EControlPraseResult::RetainText;
#endif
			break;
		case HashValue("ifdef"):
#if PARSE_DEFINE_IF
			if (!parseIfdef())
			{
				result = EControlPraseResult::RetainText;
			}
#else
			result = EControlPraseResult::RetainText;
#endif
			break;
		case HashValue("ifndef"):
#if PARSE_DEFINE_IF
			if (!parseIfndef())
			{
				result = EControlPraseResult::RetainText;
			}
#else
			result = EControlPraseResult::RetainText;
#endif
			break;
		case HashValue("line"):
			result = EControlPraseResult::RetainText;
			break;
		case HashValue("undef"):
			if (isSkipText())
			{
				skipToNextLine();
			}
			else
			{
				if (!parseUndef())
				{
					return false;
				}

				if (bReplaceMarcoText)
				{
					result = EControlPraseResult::OK;
				}
				else
				{
					result = EControlPraseResult::RetainText;
				}
			}
			break;
		case HashValue("pragma"):
			if (isSkipText())
			{
				skipToNextLine();
			}
			else
			{
				if (parsePragma())
				{


				}
				else
				{
					result = EControlPraseResult::RetainText;
				}
			}
			break;
		case HashValue("elif"):
		case HashValue("else"):
		case HashValue("endif"):
#if PARSE_DEFINE_IF
			mInput.setLoc(saveLoc);
			result = EControlPraseResult::ExitBlock;
#else
			result = EControlPraseResult::RetainText;
#endif
			break;
		case HashValue("error"):
			if (isSkipText())
			{
				skipToNextLine();
			}
			else
			{
				if (parseError())
				{
					skipToNextLine();
				}
			}
			break;
		default:
			{
				result = EControlPraseResult::RetainText;
				return true;
			}
		}

		return true;
	}

	bool Preprocessor::parseInclude()
	{
		mInput.skipSpaceInLine();

		StringView pathText;
		if (!mInput.tokenLineString(pathText))
			PARSE_ERROR("Error include Format");

		std::string expendText;
		ExpandMarcoResult expandResult;
		if (!expandMarco(pathText, expendText, expandResult))
		{
			return false;
		}
		if (expandResult.isExpanded())
		{
			pathText = expendText;
		}

		if (pathText.size() < 2)
			PARSE_ERROR("Error include Format");

		bool bSystemPath = false;
		if (pathText[0] == '\"')
		{
			if (pathText[pathText.size() - 1] != '\"')
				PARSE_ERROR("Error include Format");
		}
		else if (pathText[0] == '<')
		{
			if (pathText[pathText.size() - 1] != '>')
				PARSE_ERROR("Error include Format");

			bSystemPath = true;
		}
		else
		{
			PARSE_ERROR("Error include Format");
		}

		StringView path{ pathText.data() + 1 , pathText.size() - 2 };
		if (path.size() == 0)
		{
			PARSE_ERROR("No include file Name");
		}


		std::string fullPath;
		if (!findFile(path.toStdString(), fullPath))
			PARSE_ERROR("Can't find include file : %s", fullPath.c_str());

		skipToNextLine();

		HashString fullPathHS = HashString(fullPath.c_str());

		mUsedFiles.insert(fullPathHS);
		if (mPragmaOnceSet.find(fullPathHS) != mPragmaOnceSet.end())
			return true;

		if (mSourceLibrary == nullptr)
		{
			mSourceLibrary = new CodeSourceLibrary;
			mbSourceLibraryManaged = true;
		}

		CHECK(mSourceLibrary);
		CodeSource* includeSource = mSourceLibrary->findOrLoadSource(fullPathHS);
		if (includeSource == nullptr)
		{
			PARSE_ERROR("Can't load include file : %s" , fullPath.c_str());
		}

		if (bCommentIncludeFileName)
		{
			mOutput->push("//Include ");
			mOutput->push(path);
			mOutput->pushEoL();
		}

		InputEntry entry;
		entry.input = mInput;
		entry.onChildFinish = [this, savedPath = path.toStdString() ]()
		{
			mOutput->pushEoL();
			if (bCommentIncludeFileName)
			{
				mOutput->push("//~Include ");
				mOutput->push(savedPath);
				mOutput->pushEoL();
			}

			bRequestSourceLine = true;
		};

		mInputStack.push_back(entry);
		mInput.source = includeSource;
		mInput.resetSeek();
		bRequestSourceLine = true;

		return true;
	}

	bool Preprocessor::parseDefine()
	{
		char const* debugView = mInput.getCur();

		mInput.skipSpaceInLine();

		StringView marcoName;
		if (!mInput.tokenIdentifier(marcoName))
			return false;

		mInput.skipSpaceInLine();

		MarcoSymbol marco;
		marco.vaArgs.indexArg = INDEX_NONE;
		marco.vaArgs.offset = -1;

		if (mInput.tokenChar('('))
		{
			if (!bSupportMarcoArg)
			{
				PARSE_ERROR("Marco don't support Arg input");
			}

			TArray< StringView > argList;
			for(;;)
			{
				if (mInput.tokenChar(')'))
					break;

				mInput.skipSpaceInLine();

				if (mInput.getCur()[0] == '.')
				{
					mInput.advance();

					if (!mInput.tokenChar('.') )
						return false;
					if (!mInput.tokenChar('.'))
						return false;


					mInput.skipSpaceInLine();
					if (!mInput.tokenChar(')'))
						return false;
					
					marco.vaArgs.indexArg = argList.size();
					break;
				}
				else
				{
					StringView arg;
					if (!mInput.tokenIdentifier(arg))
					{
						return false;
					}

					mInput.skipSpaceInLine();

					argList.push_back(arg);
					if (mInput.tokenChar(')'))
						break;


					if (!mInput.tokenChar(','))
					{
						return false;
					}
				}
			}

			//parse expr

			auto FindArg = [&argList](StringView const& str) -> int
			{
				for( int i = 0 ; i < argList.size(); ++i )
				{
					if (str == argList[i])
						return i;
				}
				return INDEX_NONE;
			};

			mInput.skipSpaceInLine();
			marco.numArgs = argList.size();

			while (!mInput.isEoL())
			{
				bool bHadConcatArg = false;
				bool bHadArgString = false;
				StringView exprUnit;

				if (mInput[0] == '#')
				{
					if (mInput[1] == '#')
					{
						bHadConcatArg = true;
						mInput.advance();
						mInput.advance();
					}
					else
					{
						bHadArgString = true;
						mInput.advance();
					}
				}


				if (mInput.tokenIdentifier(exprUnit))
				{
					if (bHadArgString)
						marco.expr += '\"';

					if (marco.vaArgs.indexArg != INDEX_NONE && exprUnit == "__VA_ARGS__")
					{
						marco.vaArgs.offset = marco.expr.size();
						marco.bVaEatComma = bHadConcatArg;
					}
					else
					{
						int idxArg = FindArg(exprUnit);
						if (idxArg != INDEX_NONE)
						{
							MarcoSymbol::ArgEntry entry;
							entry.indexArg = idxArg;
							entry.offset = marco.expr.size();
							marco.argEntries.push_back(entry);
						}
						else
						{
							marco.expr.append(exprUnit.begin(), exprUnit.end());
						}
					}

					if (bHadArgString)
						marco.expr += '\"';
				}
				else
				{
					if (bHadArgString)
						marco.expr += '#';
					else if (bHadConcatArg)
						marco.expr += "##";

					marco.expr += mInput[0];
					mInput.advance();
				}
			}

		}
		else
		{
			mInput.skipSpaceInLine();

			marco.numArgs = 0;
			StringView exprText;
			if (mInput.tokenLineString(exprText))
			{
				//Cut Comment
				for (int i = 0; i < exprText.size() - 1; ++i)
				{
					if (exprText[i] == '/' && exprText[i + 1] == '/')
					{
						exprText = StringView(exprText.data(), i);
						exprText.trimEnd();
						break;
					}
				}

				marco.expr.append(exprText.begin(), exprText.end());
				marco.evalFrame = -1;
			}
			else
			{
				marco.expr = "";
				marco.evalFrame = INT_MAX;
				marco.cachedEvalValue = 0;
			}
		}

		HashString marcoString{ marcoName , true };
		auto iter = mMarcoSymbolMap.find(marcoString);
		if (iter != mMarcoSymbolMap.end())
		{
			if (bAllowRedefineMarco)
			{
				PARSE_WARNING("Marco %s is defined : %s -> %s" , marcoString.c_str() , iter->second.expr.c_str() ,  marco.expr.c_str() );
				mMarcoSymbolMap.emplace_hint(iter, marcoString, std::move(marco));
				++mCurFrame;
			}
			else
			{
				PARSE_ERROR("Marco %s is defined", marcoString.c_str());
			}
		}
		else
		{
			mMarcoSymbolMap.emplace(marcoString, std::move(marco));
		}
		skipToNextLine();
		return true;
	}

	bool Preprocessor::parseIf()
	{
		int exprRet = 0;
		if (!isSkipText())
		{
			if (!parseConditionExpression(exprRet))
			{
				return false;
			}
		}

		skipToNextLine();
		return parseIfInternal(exprRet);
	}

	bool Preprocessor::parseIfdef()
	{
		int exprRet = 0;
		if (!isSkipText())
		{
			if (!parseDefined(exprRet))
				return false;
		}

		skipToNextLine();
		return parseIfInternal(exprRet);
	}

	bool Preprocessor::parseIfndef()
	{
		int exprRet = 0;
		if (!isSkipText())
		{
			if (!parseDefined(exprRet))
				return false;
		}

		skipToNextLine();
		return parseIfInternal(!exprRet);
	}

	bool Preprocessor::parseIfInternal(int exprRet)
	{
		char const* debugView = mInput.getCur();

		++mIfScopeDepth;
		if (!parseCode(exprRet == 0))
		{
			return false;
		}

		while (!IsInputEnd())
		{
			bool bElseFound = false;
			StringView control;
			if (!tokenControl(control))
				return false;

			switch (HashValue(control))
			{
			case HashValue("elif"):
				{
					if (bElseFound)
					{
						PARSE_ERROR("if syntax error");
					}

					if (!exprRet && !isSkipText())
					{
						if (!parseConditionExpression(exprRet))
						{
							return false;
						}

						skipToNextLine();
						if (!parseCode(exprRet == 0))
						{
							return false;
						}
					}
					else
					{
						skipToNextLine();
						if (!parseCode(true))
						{
							return false;
						}
					}
				}
				break;
			case HashValue("else"):
				{
					skipToNextLine();
					if (!parseCode(exprRet != 0))
					{
						return false;
					}
					bElseFound = true;
				}
				break;
			case HashValue("endif"):
				goto End;

			default:
				break;
			}
		}

	End:

		skipToNextLine();
		--mIfScopeDepth;

		if (!isSkipText())
		{
			bRequestSourceLine = true;
		}
		return true;
	}

	bool Preprocessor::parsePragma()
	{
		mInput.skipSpaceInLine();

		StringView command;
		if (!mInput.tokenIdentifier(command))
			return false;

		switch (HashValue(command))
		{
		case HashValue("once"):
			{
				auto filePath = mInput.source ? mInput.source->getFilePath() : HashString();
				if (!filePath.empty())
				{
					mPragmaOnceSet.insert(filePath);
				}
			}
			break;

		default:
			//PARSE_ERROR("Unknown Param Command");
			return false;
		}

		skipToNextLine();
		return true;
	}

	bool Preprocessor::parseError()
	{
		StringView text(ForceInit);

		mInput.skipSpaceInLine();
		mInput.tokenLineString(text);
		PARSE_ERROR("Error : \"%s\"" , text.toCString());
		return true;
	}

	bool Preprocessor::parseUndef()
	{
		mInput.skipSpaceInLine();

		StringView idName;
		if (!mInput.tokenIdentifier(idName))
			return false;

		HashString nameKey;
		if (!HashString::Find(idName, true, nameKey))
			return nullptr;

		mMarcoSymbolMap.erase(nameKey);
		return true;
	}

	bool Preprocessor::parseDefined(int& ret)
	{
		mInput.skipSpaceInLine();

		StringView idName;
		if (!mInput.tokenIdentifier(idName))
			return false;

		ret = (findMarco(idName) == nullptr) ? 0 : 1;
		return true;
	}

	bool Preprocessor::parseExpression(int& ret)
	{
		mInput.skipSpaceInLine();

#if USE_TEMPLATE_PARSE_OP
		if (!parseExprOp< EOperatorPrecedence::Type(0)>(ret))
			return false;
#else
		if ( !parseExprOp(ret) )
			return false;
#endif
		return true;
	}

	bool Preprocessor::parseExprOp(int& ret, EOperatorPrecedence::Type precedence /*= 0*/)
	{
		char const* debugView = mInput.getCur();

		if (precedence == EOperatorPrecedence::Preifx)
		{
			return parseExprFactor(ret);
		}

		if (!parseExprOp(ret, EOperatorPrecedence::Type(precedence + 1)))
			return false;

		EOperator::Type op;
		while (tokenOp(precedence, op))
		{
			mInput.skipSpaceInLine();

			int rhs;
			if (!parseExprOp(rhs, EOperatorPrecedence::Type(precedence + 1)))
				return false;

			switch (op)
			{
#define CASE_OP( NAME , OP , P ) case EOperator::NAME: ret = ret OP rhs; break;
				BINARY_OPERATOR_LIST(CASE_OP)
#undef  CASE_OP
			default:
				break;
			}
		}
		return true;
	}

	template< EOperatorPrecedence::Type Precedence >
	bool Preprocessor::parseExprOp(int& ret)
	{
		char const* debugView = mInput.getCur();

		if (!parseExprOp<EOperatorPrecedence::Type(Precedence + 1)>(ret))
			return false;

		EOperator::Type op;
		while (tokenOp< Precedence >(op))
		{
			mInput.skipSpaceInLine();

			int rhs;
			if (!parseExprOp<EOperatorPrecedence::Type(Precedence + 1)>(rhs))
				return false;

			switch (op)
			{
#define CASE_OP( NAME , OP , P ) case EOperator::NAME: if constexpr ( P == Precedence ) ret = ret OP rhs; break;
				BINARY_OPERATOR_LIST(CASE_OP)
#undef  CASE_OP
			default:
				break;
			}
		}
		return true;
	}

	template<>
	bool Preprocessor::parseExprOp< EOperatorPrecedence::Preifx >(int& ret)
	{
		return parseExprFactor(ret);
	}


	bool Preprocessor::parseExprFactor(int& ret)
	{
		switch (mInput[0])
		{
		case '+':
			if (mInput[1] == '+')
			{
				mInput.advanceNoEoL(2);
				if (!parseExprFactor(ret))
					return false;

				++ret;
			}
			else
			{
				mInput.advanceNoEoL(1);
				if (!parseExprFactor(ret))
					return false;
			}
			break;
		case '-':
			if (mInput[1] == '-')
			{
				mInput.advanceNoEoL(2);
				if (!parseExprFactor(ret))
					return false;

				--ret;
			}
			else
			{
				mInput.advanceNoEoL(1);
				if (!parseExprFactor(ret))
					return false;

				ret = -ret;
			}
			break;
		case '~':
			{
				mInput.advanceNoEoL(1);
				if (!parseExprFactor(ret))
					return false;
				ret = ~ret;
			}
			break;
		case '!':
			{
				mInput.advanceNoEoL(1);
				if (!parseExprFactor(ret))
					return false;

				ret = !ret;
			}
			break;
		default:
			if (!parseExprValue(ret))
				return false;
		}

		mInput.skipSpaceInLine();
		return true;
	}

	bool Preprocessor::parseExprValue(int& ret)
	{
		if (mInput.tokenChar('('))
		{
			parseExpression(ret);

			if (!mInput.tokenChar(')'))
				PARSE_ERROR("Expression is invalid : less \')\'" );

			return true;
		}
		else if (mInput.tokenNumber(ret))
		{
			mInput.skipSpaceInLine();
			return true;
		}


		ret = 0;
		return true;
	}

	bool Preprocessor::tokenOp(EOperatorPrecedence::Type precedence, EOperator::Type& outType)
	{
#if USE_OPERATION_CACHE
		if (mParsedCachedOP == EOperator::None)
		{
			if (mInput.isEoL())
			{
				return false;
			}

			int len = FExpressionUitlity::FindOperatorToEnd(mInput.getCur(), precedence, mParsedCachedOP);
			if (len == 0)
				return false;

			assert(len <= 2);
			mParesedCacheOPPrecedence = FExpressionUitlity::GetOperationInfo(mParsedCachedOP).precedence;
			mInput.advanceNoEoL(len);
		}

		if (mParesedCacheOPPrecedence != precedence)
			return false;

		outType = mParsedCachedOP;
		mParsedCachedOP = EOperator::None;
		return true;

#else
		if (mInput.isEoL())
		{
			return false;
		}

		int len = FExpressionUitlity::FindOperator(mInput.getCur(), precedence, outType);
		if (len == 0)
			return false;

		assert(len <= 2);
		mInput.advanceNoEoL(len);
		return true;
#endif
	}

	template< EOperatorPrecedence::Type Precedence >
	NOINLINE bool Preprocessor::tokenOp(EOperator::Type& outType)
	{
		if (mInput.isEoL())
		{
			return false;
		}

		int len = FExpressionUitlity::FindOperator< Precedence >(mInput.getCur(), outType);
		if (len == 0)
			return false;

		assert(len <= 2);
		mInput.advanceNoEoL(len);
		return true;
	}

	bool Preprocessor::ensureInputValid()
	{
		while (mInput.isEoF())
		{
			if (mInputStack.empty())
				return false;

			mInput = mInputStack.back().input;

			if (mInputStack.back().onChildFinish)
				mInputStack.back().onChildFinish();

			mInputStack.pop_back();
		}
		return true;
	}

	bool Preprocessor::IsInputEnd()
	{
		ensureInputValid();
		return mInput.isEoF();
	}


	bool Preprocessor::skipToNextLine()
	{
		if (mInput.isEoF())
		{
			ensureInputValid();
		}
		else
		{
			mInput.skipToNextLine();
			if (mInput.isEoF())
				return false;
		}
		return true;
	}

	class LineStringViewCode : public StringView
					       , public CodeTokenUtiliyT<LineStringViewCode>
	{
	public:
		LineStringViewCode(std::string const& code)
			:StringView(code)
		{

		}
		LineStringViewCode(StringView const& code)
			:StringView(code)
		{

		}

		void advance()
		{
			++mData;
			if (mNum > 1)
			{
				mNum -= 1 + SkipConcat(mData);
			}
			else
			{
				mNum -= 1;
			}
		}
		char const* getCur() { return mData; }

		void skipToLineEnd()
		{
			mData += mNum;
			mNum = 0;
		}

		char getChar() 
		{
			if (mNum == 0)
				return 0;
			return *mData;
		}

		bool isEOF() const
		{
			return mNum == 0;
		}

		void offsetTo(char const* dest)
		{
			mNum -= dest - mData;
			mData = dest;
		}
	};


	bool Preprocessor::parseConditionExpression(int& ret)
	{
		mInput.skipSpaceInLine();

		StringView exprText;
		if (!mInput.tokenLineString(exprText))
			return false;

		LineStringViewCode exprCode(exprText);

		StringView id{ ForceInit };
		if (exprCode.tokenIdentifier(id))
		{
			LineStringViewCode temp = exprCode;
			temp.skipSpaceInLine();
			if (temp.length() == 0)
			{
				auto marco = findMarco(id);
				if (marco == nullptr)
				{
					PARSE_WARNING( "%s is not defined" , id.toCString() );
					ret = 0;
				}
				else
				{
					if (marco->evalFrame < mCurFrame)
					{
						std::string expandedExpr;
						if (!expandMarco(marco->expr, expandedExpr))
						{
							return false;
						}
						CodeLoc exprLoc;
						exprLoc.mCur = expandedExpr.c_str();
						exprLoc.mLineCount = 0;
						if (!parseExpression(exprLoc, marco->cachedEvalValue))
						{
							PARSE_ERROR("Define \"%s\" is not Const Expresstion", id.toCString());
						}
						marco->evalFrame = mCurFrame;
					}

					ret = marco->cachedEvalValue;
				}

				return true;
			}
		}

		std::string exprTextConv;


		ExpandMarcoResult expandResult;
		bool bHadExpanded = false;
		if (id.length())
		{
			if (!checkIdentifierToExpand(id, exprCode, exprTextConv, expandResult))
				return false;
		}

		if (!expandMarcoInternal(exprCode, exprTextConv, expandResult))
		{
			return false;
		}

		CodeLoc exprLoc;
		exprLoc.mCur = exprTextConv.c_str();
		exprLoc.mLineCount = 0;
		if (!parseExpression(exprLoc, ret))
		{
			PARSE_ERROR("Condition Expresstion of If eval Fail : %s", exprText.toCString());
			return false;
		}

		mInput.skipSpaceInLine();
		if (!mInput.isEoF() && !mInput.isEoL())
		{

			return false;
		}

		return true;
	}


	bool Preprocessor::expandMarco(StringView const& lineText,std::string& outText)
	{
		ExpandMarcoResult expandResult;
		LineStringViewCode codeView(lineText);
		return expandMarcoInternal(codeView, outText, expandResult);
	}

	bool Preprocessor::expandMarco(StringView const& lineText, std::string& outText, ExpandMarcoResult& outExpandResult)
	{
		LineStringViewCode codeView(lineText);
		return expandMarcoInternal(codeView, outText, outExpandResult);
	}

	bool Preprocessor::checkIdentifierToExpand(StringView const& id, class LineStringViewCode& code, std::string& outText, ExpandMarcoResult& outResult)
	{
		MarcoSymbol* marco = findMarco(id);
		if (marco)
		{
			if (marco->numArgs > 0)
			{
				TArray< StringView > argList;

				code.skipSpaceInLine();

				if (!code.tokenChar('('))
				{
					outText.append(id.begin(), id.end());
					return true;
				}

				for(;;)
				{
					//#TODO: consider SkipConcat ?
					char const* start = code.getCur();
					char const* end;
					for(;;)
					{
						end = FStringParse::FindCharN(code.getCur(), code.getCur() + code.size(), ',', ')', '(');
						if (*end == 0)
						{
							PARSE_ERROR("Marco Syntax Error : %s", (char const*)id.toCString());
						}

						code.offsetTo(end + 1);
						if ( *end != '(' )
							break;
						
						int scopeDepth = 1;
						while (scopeDepth)
						{
							char const* pFind = FStringParse::FindCharN(code.getCur(), code.getCur() + code.size(), ')', '(');
							if (*pFind == 0)
							{
								PARSE_ERROR("Marco Syntax Error : %s", (char const*)id.toCString());
							}

							code.offsetTo(pFind + 1);
							if (*pFind == '(')
								++scopeDepth;
							else if (*pFind == ')')
								--scopeDepth;
						}
					}

					StringView arg = StringView(start, end - start);
					arg.trimStartAndEnd();

					argList.push_back(arg);

					if (*end == ')')
						break;

					if (*end != ',')
						return false;
				}


				if (marco->numArgs != argList.size() && marco->vaArgs.indexArg == INDEX_NONE)
				{
					PARSE_WARNING("Marco Args Count is not match");
				}

				int lastAppendOffset = 0;
				auto CheckAppendPrevExpr = [&]( int inOffset)
				{
					if (lastAppendOffset != inOffset)
					{
						auto itStart = marco->expr.begin() + lastAppendOffset;
						auto itEnd = marco->expr.begin() + inOffset;
						outText.append(itStart, itEnd);

						lastAppendOffset = inOffset;
					}
				};


				auto OutputArg = [&](StringView const& arg)
				{
#if 1
					std::string expandText;
					LineStringViewCode codeView(arg);
					if (!expandMarcoInternal(codeView, expandText, outResult))
					{
						PARSE_ERROR("Expand Arg Error : %s", arg.toCString());
						return false;
					}
					outText += expandText;
#else
					outText.append(arg.begin(), arg.end());
#endif
				};

				for (auto const& entry : marco->argEntries)
				{
					CheckAppendPrevExpr(entry.offset);

					if (entry.indexArg < argList.size())
					{
						auto const& arg = argList[entry.indexArg];
						OutputArg(arg);
					}
				}

				if (marco->vaArgs.indexArg != INDEX_NONE && marco->vaArgs.offset >= 0 )
				{
					CheckAppendPrevExpr(marco->vaArgs.offset);

					if (marco->vaArgs.indexArg < argList.size())
					{
						for (int indexArg = marco->vaArgs.indexArg; indexArg < argList.size(); ++indexArg)
						{
							auto const& arg = argList[indexArg];

							if (indexArg != marco->vaArgs.indexArg)
								outText += ",";

							OutputArg(arg);
						}
					}
					else if (marco->bVaEatComma)
					{
						for( auto index = outText.size() - 1; index ; --index)
						{
							if (outText[index] == ',')
							{
								outText.erase(index);
								break;
							}

							if (!FCString::IsSpace(outText[index]))
								break;
						}
					}
				}

				CheckAppendPrevExpr(marco->expr.size());

				outResult.numRefMarcoWithArgs += 1;
			}
			else
			{
				LineStringViewCode codeView(marco->expr);
				if (!expandMarcoInternal(codeView, outText, outResult))
				{
					return false;
				}

				outResult.numRefMarco += 1;
			}
		}
		else
		{
			outText.append(id.begin(), id.end());
		}

		return true;
	}

	bool Preprocessor::expandMarcoInternal(class LineStringViewCode& code, std::string& outText, ExpandMarcoResult& outResult)
	{
		while (!code.isEOF())
		{
			StringView id;
			if (code.tokenIdentifier(id))
			{
				if (!checkIdentifierToExpand(id, code, outText, outResult))
					return false;
			}
			else
			{
				outText += code.getChar();
				code.advance();
			}
		}

		if (outResult.numRefMarcoWithArgs)
		{
			std::string tempText = std::move(outText);
			LineStringViewCode tempCode(tempText);
			ExpandMarcoResult childResult;
			if (!expandMarcoInternal(tempCode, outText, childResult))
				return false;

			outResult.append(childResult);
			return true;
		}

		return true;
	}

	bool Preprocessor::findFile(std::string const& name, std::string& fullPath)
	{
		if( FFileSystem::IsExist(name.c_str()) )
		{
			fullPath = name;
			return true;
		}
		for( int i = 0; i < mFileSreachDirs.size(); ++i )
		{
			fullPath = mFileSreachDirs[i] + name;
			if( FFileSystem::IsExist(fullPath.c_str()) )
				return true;
		}
		return false;
	}

	Preprocessor::MarcoSymbol* Preprocessor::findMarco(StringView const& name)
	{
		HashString nameKey;
		if (!HashString::Find(name, true, nameKey))
			return nullptr;

		auto iter = mMarcoSymbolMap.find(nameKey);
		if (iter == mMarcoSymbolMap.end())
			return nullptr;


		return &iter->second;
	}

	void Preprocessor::setOutput(CodeOutput& output)
	{
		mOutput = &output;
	}

	void Preprocessor::setSourceLibrary(CodeSourceLibrary& sourceLibrary)
	{
		if (mbSourceLibraryManaged)
		{
			delete mSourceLibrary;
			mbSourceLibraryManaged = false;
		}
		mSourceLibrary = &sourceLibrary;
	}

	void Preprocessor::addSreachDir(char const* dir)
	{
		mFileSreachDirs.push_back(dir);
		std::string& dirAdd = mFileSreachDirs.back();
		if( dirAdd[dirAdd.length() - 1] != '/' )
		{
			dirAdd += '/';
		}
	}

	void Preprocessor::addDefine(char const* name, int value)
	{
		MarcoSymbol marco;
		marco.expr = FStringConv::From(value);
		marco.evalFrame = INT_MAX;
		marco.cachedEvalValue = value;
		mMarcoSymbolMap.emplace( HashString(name, true) , std::move(marco));
	}

	void Preprocessor::addInclude(char const* fileName, bool bSystemPath)
	{

	}

	void Preprocessor::getUsedIncludeFiles(std::unordered_set< HashString >& outFiles)
	{
		outFiles.insert(mUsedFiles.begin(), mUsedFiles.end());
	}

	void Preprocessor::emitSourceLine(int lineOffset)
	{
		InlineString< 512 > lineMarco;
		int len = 0;
		switch (lineFormat)
		{
		case LF_LineNumber:
			len = lineMarco.format("#line %d\n", mInput.getLine() + lineOffset);
			break;
		case LF_LineNumberAndFilePath:
			len = lineMarco.format("#line %d \"%s\"\n", mInput.getLine() + lineOffset, mInput.getSourceName());
			break;
		}
		if (len)
		{
			mOutput->push(StringView(lineMarco.c_str(), len));
		}
	}

	void Preprocessor::emitCode(StringView const& code)
	{
		if (bAddLineMarco && bRequestSourceLine)
		{
			for (int i = 0; i < code.size(); ++i)
			{
				if (!FCString::IsSpace(code[i]))
				{
					bRequestSourceLine = false;
					emitSourceLine(-1);
					break;
				}
			}
		}
		mOutput->push(code);
	}

	bool Preprocessor::tokenControl(StringView& outName)
	{
		mInput.skipSpaceInLine();

		if (!mInput.tokenChar('#'))
			return false;

		mInput.skipSpaceInLine();
		if (!mInput.tokenIdentifier(outName))
			return false;

		return true;
	}

	void CodeBufferSource::appendString(StringView const& str)
	{
		if (!mBuffer.empty())
			mBuffer.pop_back();

		char const* start = str.data();
		FCodeParse::SkipBOM(start);
		mBuffer.append(start, str.end());
		mBuffer.push_back(0);
	}

	bool CodeBufferSource::loadFile(char const* path)
	{
		if (!FFileUtility::LoadToBuffer(path, mBuffer, true))
			return false;

		return true;
	}

	bool CodeBufferSource::appendFile(char const* path)
	{
		if (!mBuffer.empty())
			mBuffer.pop_back();

		if (!FFileUtility::LoadToBuffer(path, mBuffer, true, true))
			return false;

		return true;
	}

	constexpr bool CompareOperatorText(char const* code, ConstString const& opText)
	{
		assert(opText.size() <= 2);

		if (code[0] != opText[0])
			return false;

		if (opText.size() == 2)
		{
			if (code[1] != opText[1])
			{
				return false;
			}
		}
		else
		{
#define IGNORE_CASE( S )\
			if (opText[0] == S[0])\
			{\
				if (code[1] == S[1] ) return false;\
			}

			IGNORE_CASE("||");
			IGNORE_CASE("&&");
			IGNORE_CASE("==");
			IGNORE_CASE(">=");
			IGNORE_CASE(">>");
			IGNORE_CASE("<=");
			IGNORE_CASE("<<");
			IGNORE_CASE("++");
			IGNORE_CASE("--");
			IGNORE_CASE("!=");

		}
		return true;
	}

	struct OpRange
	{
		int first;
		int last;
	};

	//#TODO
	constexpr OpRange PrecedenceOpRange[] =
	{
		{ EOperator::LogicalOR, EOperator::LogicalOR },
		{ EOperator::LogicalAND, EOperator::LogicalAND },
		{ EOperator::BitwiseOR, EOperator::BitwiseOR },
		{ EOperator::BitwiseXOR, EOperator::BitwiseXOR },
		{ EOperator::BitwiseAND, EOperator::BitwiseAND },
		{ EOperator::EQ, EOperator::NEQ },
		{ EOperator::CompGE, EOperator::CompL },
		{ EOperator::LShift, EOperator::RShift },
		{ EOperator::Add, EOperator::Sub },
		{ EOperator::Mul, EOperator::Rem },
	};

	int FExpressionUitlity::FindOperator(char const* code, EOperatorPrecedence::Type precedence, EOperator::Type& outType)
	{
		OpRange range = PrecedenceOpRange[precedence];
		for (int i = range.first; i <= range.last; ++i)
		{
			OperationInfo const& info = GetOperationInfo(EOperator::Type(i));
		
			if ( CompareOperatorText(code , info.text) )
			{
				outType = info.type;
				return info.text.size();
			}
		}
		return 0;
	}

	template< EOperatorPrecedence::Type Precedence >
	NOINLINE int FExpressionUitlity::FindOperator(char const* code, EOperator::Type& outType)
	{
		constexpr OpRange range = PrecedenceOpRange[Precedence];
		for (int i = range.first; i <= range.last; ++i)
		{
			OperationInfo const& info = GetOperationInfo(EOperator::Type(i));

			if (CompareOperatorText(code, info.text))
			{
				outType = info.type;
				return info.text.size();
			}
		}
		return 0;
	}

	int FExpressionUitlity::FindOperatorToEnd(char const* code, EOperatorPrecedence::Type precedenceStart, EOperator::Type& outType)
	{
		constexpr int PrecedenceStartIndex[] =
		{
			EOperator::LogicalOR,
			EOperator::LogicalAND,
			EOperator::BitwiseOR,
			EOperator::BitwiseXOR,
			EOperator::BitwiseAND,
			EOperator::EQ,
			EOperator::CompGE,
			EOperator::LShift,
			EOperator::Add,
			EOperator::Mul,
		};

		for (int index = PrecedenceStartIndex[precedenceStart]; index >= 0; --index)
		{
			OperationInfo const& info = GetOperationInfo(EOperator::Type(index));

			if (CompareOperatorText(code, info.text))
			{
				outType = info.type;
				return info.text.size();
			}
		}
		return 0;
	}

	constexpr OperationInfo OpInfos[] =
	{
#define OPINFO_OP( NAME , OP , P ) { EOperator::NAME , #OP , P } ,
				BINARY_OPERATOR_LIST(OPINFO_OP)
#undef OPINFO_OP
	};

	constexpr OperationInfo FExpressionUitlity::GetOperationInfo(EOperator::Type type)
	{
		return OpInfos[type];
	}

	CodeSourceLibrary::~CodeSourceLibrary()
	{
		cleanup();
	}

	CodeBufferSource* CodeSourceLibrary::findOrLoadSource(HashString const& path)
	{
		auto iter = mSourceMap.find(path);

		CodeBufferSource* result;
		if (iter == mSourceMap.end())
		{
			TPtrHolder< CodeBufferSource > includeSourcePtr(new CodeBufferSource);
			if (!includeSourcePtr->loadFile(path.c_str()))
			{
				return nullptr;
			}
			result = includeSourcePtr.get();
			result->filePath = path;
			mSourceMap.emplace(path, includeSourcePtr.release());
		}
		else
		{
			result = iter->second;
		}

		return result;
	}

	void CodeSourceLibrary::cleanup()
	{
		for (auto& pair : mSourceMap)
		{
			delete pair.second;
		}
		mSourceMap.clear();
	}

}//namespace CPP
