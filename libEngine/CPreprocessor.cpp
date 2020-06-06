#include "CPreprocessor.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "FixString.h"
#include "Holder.h"
#include "Core/StringConv.h"
#include "Core/FNV1a.h"

#define PARSE_ERROR( MSG , ... ) \
	{\
		FixString<512> msg; msg.format( "%s (%d) - "MSG , mInput.source ? mInput.source->filePath.c_str() : "" , mInput.getLine() , msg.c_str() , ##__VA_ARGS__ );\
		throw SyntaxError( msg );\
	}
#define PARSE_WARNING( MSG , ... )\
	{\
		FixString<512> msg; msg.format( "%s (%d) - "MSG , mInput.source ? mInput.source->filePath.c_str() : "" , mInput.getLine() , msg.c_str() , ##__VA_ARGS__ );\
		warning( msg );\
	}

#define FUNCTION_CHECK( func ) func

#define PARSE_DEFINE_IF 1

#define USE_OPERATION_CACHE 1

namespace CPP
{
	static uint32 HashValue(StringView const& str)
	{
		return FNV1a::MakeHash<uint32>((uint8 const*)str.data(), str.length()) & 0xffff;
	}
	static constexpr uint32 HashValue(char const* str)
	{
		return FNV1a::MakeStringHash<uint32>(str) & 0xffff;
	}

	std::unordered_map< HashString, CodeSource* > Preprocessor::mLoadedSourceMap;

	Preprocessor::Preprocessor()
	{

	}

	Preprocessor::~Preprocessor()
	{
#if 0
		for( auto& pair : mLoadedSourceMap )
		{
			delete pair.second;
		}
		mLoadedSourceMap.clear();
#endif
	}

	void Preprocessor::translate(CodeSource& sorce)
	{
		mInputStack.clear();
		mInput.source = &sorce;
		mInput.resetSeek();
		mScopeStack.clear();
		mIfScopeDepth = 0;

		parseCode(false);
	}

	bool Preprocessor::parseCode(bool bSkip)
	{
		if (!mScopeStack.empty())
		{
			bSkip |= mScopeStack.back().bSkipText;

			if (!bSkip)
			{
				bRequestSourceLine = true;
			}
		}

		ScopeState state;
		state.bSkipText = bSkip;
		mScopeStack.push_back(state);

		while (haveMore())
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
					emitCode(StringView(start.mCur, mInput.mCur - start.mCur));
					++numLine;
				}
			}
			else if (!bSkip)
			{
				mInput.setLoc(start);
				mInput.skipToNextLine();
				++numLine;

				if (bReplaceMarcoText)
				{
					//TODO
					emitCode(StringView(start.mCur, mInput.mCur - start.mCur));
					
				}
				else
				{
					emitCode(StringView(start.mCur, mInput.mCur - start.mCur));
				}
			}
			else
			{
				mInput.skipToNextLine();
			}
		}

		mScopeStack.pop_back();
		return true;
	}

	bool Preprocessor::parseControlLine(EControlPraseResult& result)
	{
		if (!skipSpaceInLine())
			return false;

		CodeLoc saveLoc = mInput;

		if (!tokenChar('#'))
			return false;

		if (!skipSpaceInLine())
			return false;

		StringView controlStr;
		if (!tokenIdentifier(controlStr))
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

				}
				result = EControlPraseResult::RetainText;
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
				if (parseUndef())
				{


				}
				result = EControlPraseResult::RetainText;
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
		if (!skipSpaceInLine())
			return false;

		StringView pathText;
		if (!tokenString(pathText))
			PARSE_ERROR("Error include Format");
		if (pathText.size() < 2)
			PARSE_ERROR("Error include Format");

		bool bSystemPath = false;
		if (pathText[1] == '\"')
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

		StringView path{ pathText.data() + 1 , pathText.size() - 2 };
		if (path.size() == 0)
		{
			PARSE_ERROR("No include file Name");
		}

		std::string fullPath;
		if (!findFile(path.toStdString(), fullPath))
			PARSE_ERROR("Can't find include file : %s", fullPath.c_str() );

		skipToNextLine();

		HashString fullPathHS = HashString(fullPath.c_str());

		mUsedFiles.insert(fullPathHS);
		if (mParamOnceSet.find(fullPathHS) == mParamOnceSet.end())
		{
			auto iter = mLoadedSourceMap.find(fullPathHS);

			CodeSource* includeSource;
			if (iter == mLoadedSourceMap.end())
			{
				TPtrHolder< CodeSource > includeSourcePtr(new CodeSource);
				if (!includeSourcePtr->loadFile(fullPath.c_str()))
				{
					PARSE_ERROR("Can't load include file : %s" , fullPath.c_str());
				}
				includeSource = includeSourcePtr.get();
				mLoadedSourceMap.emplace(fullPathHS, includeSourcePtr.release());
			}
			else
			{
				includeSource = iter->second;
			}


			includeSource->filePath = fullPathHS;

			if (bCommentIncludeFileName)
			{
				mOutput->push("//Include ");
				mOutput->push(path);
				mOutput->pushEoL();
			}

			InputEntry entry;
			entry.input = mInput;

			entry.onChildFinish = [this,path]()
			{
				mOutput->pushEoL();
				if (bCommentIncludeFileName)
				{
					mOutput->push("//~Include ");
					mOutput->push(path);
					mOutput->pushEoL();
				}

				bRequestSourceLine = true;
			};
			mInputStack.push_back(entry);
			mInput.source = includeSource;
			mInput.resetSeek();
			bRequestSourceLine = true;
		}

		return true;
	}

	bool Preprocessor::parseDefine()
	{
		char const* debugView = mInput.mCur;

		if (!skipSpaceInLine())
			return false;

		StringView marcoName;
		if (!tokenIdentifier(marcoName))
			return false;

		if (tokenChar('('))
		{
			return false;
		}
		else
		{
			if (!skipSpaceInLine())
				return false;

			MarcoSymbol marco;

			StringView exprText;
			if (tokenString(exprText))
			{
				replaceMarco(exprText, marco.expr);
				marco.evalFrame = -1;
			}
			else
			{
				marco.expr = "";
				marco.evalFrame = INT_MAX;
				marco.cacheEvalValue = 0;
			}

			auto iter = mMarcoSymbolMap.find(marcoName);
			if (iter != mMarcoSymbolMap.end())
			{
				if (!warning("Marco is defined"))
					return false;

				mMarcoSymbolMap.emplace_hint(iter, marcoName, std::move(marco));
			}
			else
			{
				mMarcoSymbolMap.emplace(marcoName, std::move(marco));
			}
		}

		skipToNextLine();
		return true;
	}

	bool Preprocessor::parseIf()
	{
		StringView exprText;
		if (!tokenString(exprText))
			return false;

		int exprRet = 0;
		if (!isSkipText())
		{
			std::string exprTextConv;
			replaceMarco(exprText, exprTextConv);

			CodeLoc exprLoc;
			exprLoc.mCur = exprTextConv.c_str();
			exprLoc.mLineCount = 0;
			if (!parseExpression(exprLoc , exprRet))
			{
				PARSE_ERROR("Condition Expresstion of If eval Fail : %s" , exprText.toCString());
			}

			skipSpaceInLine();
			if (!mInput.isEoF() && !mInput.isEoL())
				return false;
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
		char const* temp = mInput.mCur;

		++mIfScopeDepth;
		if (!parseCode(exprRet == 0))
		{
			return false;
		}
		while (haveMore())
		{
			bool bElseFind = false;
			StringView control;
			if (!tokenControl(control))
				return false;

			switch (HashValue(control))
			{
			case HashValue("elif"):
				{
					if (bElseFind)
					{
						PARSE_ERROR("if error");
					}

					if (!exprRet && !isSkipText())
					{
						if (!parseExpression(exprRet))
							return false;

						skipSpaceInLine();
						if (!mInput.isEoF() && !mInput.isEoL())
							return false;

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
					bElseFind = true;
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
		if (!skipSpaceInLine())
			return false;

		StringView command;
		if (!tokenIdentifier(command))
			return false;

		switch (HashValue(command))
		{
		case HashValue("once"):
			{
				if (mInput.source && !mInput.source->filePath.empty())
				{
					mParamOnceSet.insert(mInput.source->filePath);
				}
			}
			break;

		default:
			PARSE_ERROR("Unknown Param Command");
			return false;
		}


		skipToNextLine();
		return true;
	}

	bool Preprocessor::parseError()
	{
		StringView text;
		tokenString(text);
		PARSE_ERROR("Error");
		return true;
	}

	bool Preprocessor::parseUndef()
	{
		StringView idName;
		if (!tokenIdentifier(idName))
			return false;

		mMarcoSymbolMap.erase(idName);
		return true;
	}

	bool Preprocessor::parseDefined(int& ret)
	{
		if (!skipSpaceInLine())
			return false;

		StringView idName;
		if (!tokenIdentifier(idName))
			return false;

		auto marco = mMarcoSymbolMap.find(idName);
		ret = (marco == mMarcoSymbolMap.end()) ? 0 : 1;
		return true;
	}

	bool Preprocessor::parseExpression(int& ret)
	{
		if (!skipSpaceInLine())
			return false;

		if ( !parseExprOp(ret) )
			return false;

		return true;
	}

	bool Preprocessor::parseExprOp(int& ret, EOperatorPrecedence::Type precedence /*= 0*/)
	{
		char const* debugView = mInput.mCur;

		if (precedence == EOperatorPrecedence::Preifx)
		{
			return parseExprFactor(ret);
		}

		if (!parseExprOp(ret, EOperatorPrecedence::Type(precedence + 1)))
			return false;

		EOperator::Type op;
		while (tokenOp(precedence, op))
		{
			skipSpaceInLine();
			int rhs;
			if (!parseExprOp(rhs, EOperatorPrecedence::Type(precedence + 1)))
				return false;

			switch (op)
			{
#define CASE_OP( NAME , OP , P ) case EOperator::NAME: ret = ret OP rhs; break;
				BINARY_OPERATOR_LIST(CASE_OP)
			default:
				break;
			}
		}
		return true;
	}

	bool Preprocessor::parseExprFactor(int& ret)
	{
		switch (*mInput.mCur)
		{
		case '+':
			if (mInput.mCur[1] == '+')
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
			if (mInput.mCur[1] == '-')
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

		skipSpaceInLine();
		return true;
	}

	bool Preprocessor::parseExprValue(int& ret)
	{
		if (tokenChar('('))
		{
			parseExpression(ret);

			if (!tokenChar(')'))
				PARSE_ERROR("Expression is invalid : less \')\'" );

			return true;
		}
		else if (tokenNumber(ret))
		{
			skipSpaceInLine();
			return true;
		}
		StringView value;
		if (!tokenIdentifier(value))
			return false;

		auto marco = mMarcoSymbolMap.find(value);
		if (marco == mMarcoSymbolMap.end())
		{
			warning("");
			ret = 0;
			return true;
		}
		else
		{
			if (marco->second.evalFrame < mCurFrame)
			{
				CodeLoc exprLoc;
				exprLoc.mCur = marco->second.expr.c_str();
				exprLoc.mLineCount = 0;
				if (!parseExpression(exprLoc, marco->second.cacheEvalValue))
				{
					PARSE_ERROR("Define \"%s\" is not Const Expresstion", (char const*)value.toCString());
				}
				marco->second.evalFrame = mCurFrame;
			}

			ret = marco->second.cacheEvalValue;
		}

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

			int len = FExpressionUitlity::FindOperatorToEnd(mInput.mCur, precedence, mParsedCachedOP);
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

		int len = FExpressionUitlity::FindOperator(mInput.mCur, precedence, outType);
		if (len == 0)
			return false;

		assert(len <= 2);
		mInput.advanceNoEoL(len);
		return true;
#endif
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

	bool Preprocessor::haveMore()
	{
		ensureInputValid();
		return !mInput.isEoF();
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

	bool Preprocessor::skipSpaceInLine()
	{
		mInput.skipSpaceInLine();
		return !mInput.isEoF();
	}

	bool Preprocessor::tokenChar(char c)
	{
		if (*mInput.mCur != c)
			return false;

		mInput.advance();
		return true;
	}

	void Preprocessor::replaceMarco(StringView const& text, std::string& outText)
	{
		char const* cur = text.begin();
		char const* end = text.end();
		while (cur < end)
		{
			if (IsValidStartCharForIdentifier(*cur))
			{
				char const* pIdStart = cur;

				do 
				{
					CodeLoc::Advance(cur);
					if (cur >= end)
						break;
				}
				while (IsValidCharForIdentifier(*cur));

				StringView id = StringView(pIdStart, cur - pIdStart);

				auto iter = mMarcoSymbolMap.find(id);
				if (iter != mMarcoSymbolMap.end())
				{
					outText += iter->second.expr;
				}
				else
				{
					outText.append(id.begin(), id.end());
				}
			}
			else
			{
				outText += *cur;
				CodeLoc::Advance(cur);
			}
		}
	}

	bool Preprocessor::findFile(std::string const& name, std::string& fullPath)
	{
		if( FileSystem::IsExist(name.c_str()) )
		{
			fullPath = name;
			return true;
		}
		for( int i = 0; i < mFileSreachDirs.size(); ++i )
		{
			fullPath = mFileSreachDirs[i] + name;
			if( FileSystem::IsExist(fullPath.c_str()) )
				return true;
		}
		return false;
	}

	void Preprocessor::setOutput(CodeOutput& output)
	{
		mOutput = &output;
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
		marco.cacheEvalValue = value;
		mMarcoSymbolMap.emplace(name, std::move(marco));
	}

	void Preprocessor::getIncludeFiles(std::vector< HashString >& outFiles)
	{
		outFiles.insert(outFiles.end(), mUsedFiles.begin(), mUsedFiles.end());
	}

	void Preprocessor::emitSourceLine(int lineOffset)
	{
		FixString< 512 > lineMarco;
		int len = lineMarco.format("#line %d \"%s\"\n", mInput.getLine() + lineOffset, (mInput.source) ? mInput.source->filePath.c_str() : "");
		mOutput->push(StringView(lineMarco.c_str(), len));
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

	bool Preprocessor::tokenIdentifier(StringView& outIdentifier)
	{
		char const* start = mInput.mCur;

		if (!IsValidStartCharForIdentifier(*mInput.mCur))
			return false;
		do
		{
			mInput.advance();
		} 
		while (IsValidCharForIdentifier(*mInput.mCur));

		outIdentifier = StringView(start, mInput.mCur - start);
		return true;
	}

	bool Preprocessor::tokenNumber(int& outValue)
	{
		char const* start = mInput.mCur;

		if (!isdigit(*mInput.mCur))
			return false;

		for (;;)
		{
			mInput.advance();
			if (!isdigit(*mInput.mCur))
				break;
		}

		outValue = FStringConv::To<int>(start, mInput.mCur - start);
		return true;
	}

	bool Preprocessor::tokenString(StringView& outString)
	{
		if (!skipSpaceInLine())
			return false;

		char const* start = mInput.mCur;
		mInput.skipToLineEnd();
		char const* end = mInput.mCur;
		if (end == start)
			return false;

		while (end != start)
		{
			if (!FCString::IsSpace(*end))
				break;

			CodeLoc::Backward(end);
		}

		outString = StringView(start, end - start + 1);
		return true;
	}

	bool Preprocessor::tokenControl(StringView& outName)
	{
		if (!skipSpaceInLine())
			return false;

		if (!tokenChar('#'))
			return false;

		skipSpaceInLine();
		if (!tokenIdentifier(outName))
			return false;

		return true;
	}

	void CodeSource::appendString(StringView const& str)
	{
		if (!mBuffer.empty())
			mBuffer.pop_back();
		mBuffer.insert(mBuffer.end(), str.begin() , str.end());
		mBuffer.push_back(0);
	}

	bool CodeSource::loadFile(char const* path)
	{
		if (!FileUtility::LoadToBuffer(path, mBuffer, true))
			return false;

		return true;
	}

	int FExpressionUitlity::FindOperator(char const* code, EOperatorPrecedence::Type precedence, EOperator::Type& outType)
	{
		struct OpRange
		{
			int first;
			int last;
		};

		//TODO
		static const OpRange PrecedenceOpRange[] =
		{
			{0,0},
			{1,1},
			{2,2},
			{3,3},
			{4,4},
			{5,6},
			{7,10},
			{11,12},
			{13,14},
			{15,17},
		};

		OpRange range = PrecedenceOpRange[precedence];
		for (int i = range.first; i <= range.last; ++i)
		{
			OperationInfo const& info = GetOperationInfo(EOperator::Type(i));
			assert(info.text.size() <= 2);
			if (code[0] != info.text[0])
				continue;

			if (info.text.size() == 2)
			{
				if (code[1] != info.text[1])
				{
					continue;
				}
			}
			else
			{
				// '||' and '&&' case
				if (code[1] == '|' || code[1] == '&')
					continue;
			}

			outType = info.type;
			return info.text.size();
		}

		return 0;
	}

	int FExpressionUitlity::FindOperatorToEnd(char const* code, EOperatorPrecedence::Type precedenceStart, EOperator::Type& outType)
	{
		static const int PrecedenceStartIndex[] =
		{
			0,
			1,
			2,
			3,
			4,
			6,
			10,
			12,
			14,
			17,
		};

		for (int index = PrecedenceStartIndex[precedenceStart]; index >= 0; --index)
		{
			OperationInfo const& info = GetOperationInfo(EOperator::Type(index));

			assert(info.text.size() <= 2);
			if (code[0] != info.text[0])
				continue;
			
			if (info.text.size() == 2)
			{
				if (code[1] != info.text[1])
				{
					continue;
				}
			}
			else
			{
				// '||' and '&&' case
				if (code[1] == '|' || code[1] == '&')
					continue;
			}

			outType = info.type;
			return info.text.size();

		}
		return 0;
	}

	OperationInfo FExpressionUitlity::GetOperationInfo(EOperator::Type type)
	{
		static const OperationInfo OpInfos[] =
		{
#define OPINFO_OP( NAME , OP , P ) { EOperator::NAME , #OP , P } ,
				BINARY_OPERATOR_LIST(OPINFO_OP)
#undef OPINFO_OP
		};

		return OpInfos[type];
	}

}//namespace CPP
