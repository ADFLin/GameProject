#include "CPreprocessor.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "FixString.h"
#include "Holder.h"
#include "Core/StringConv.h"
#include "Core/FNV1a.h"

#define PARSE_ERROR( MSG ) throw SyntaxError( MSG );
#define FUNCTION_CHECK( func ) func

#define PARSE_DEFINE_IF 1

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


	Preprocessor::Preprocessor()
	{

	}

	Preprocessor::~Preprocessor()
	{
		for( auto& pair : mLoadedSourceMap )
		{
			delete pair.second;
		}
		mLoadedSourceMap.clear();
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
					emitCode(StringView(start.mCur, mInput.mCur - start.mCur));
					//FIXME
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

			if (numLine == 214)
			{
				int i = 1;
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
			if (!parseInclude())
			{

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
				result = EControlPraseResult::RetainText;
			}
			break;
		case HashValue("error"):
			if (isSkipText())
			{
				skipToNextLine();
			}
			else
			{
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
			PARSE_ERROR("Can't find include file");

		skipToNextLine();

		HashString fullPathHS = HashString(fullPath.c_str());
		if (mParamOnceSet.find(fullPathHS) == mParamOnceSet.end())
		{
			auto iter = mLoadedSourceMap.find(fullPathHS);

			CodeSource* includeSource;
			if (iter == mLoadedSourceMap.end())
			{
				TPtrHolder< CodeSource > includeSourcePtr(new CodeSource);
				if (!includeSourcePtr->loadFile(fullPath.c_str()))
				{
					PARSE_ERROR("Can't open include file");
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
				mOutput->pushNewline();
			}

			InputEntry entry;
			entry.input = mInput;

			entry.onChildFinish = [this,path]()
			{
				mOutput->pushNewline();
				if (bCommentIncludeFileName)
				{
					mOutput->push("//~Include ");
					mOutput->push(path);
					mOutput->pushNewline();
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
			marco.name = marcoName;

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

		int exprRet;
		{
			std::string exprTextConv;
			replaceMarco(exprText, exprTextConv);
			CodeLoc exprLoc;
			exprLoc.mCur = exprTextConv.c_str();
			exprLoc.mLineCount = 0;

			InputLocScope scope(*this, exprLoc);

			if (!parseExpression(exprRet))
			{
				PARSE_ERROR("Condition Expresstion of If eval Fail");
			}
		}

		skipToNextLine();
		return parseIfInternal(exprRet);
	}

	bool Preprocessor::parseIfdef()
	{
		int exprRet;
		if (!parseDefined(exprRet))
			return false;

		skipToNextLine();
		return parseIfInternal(exprRet);
	}

	bool Preprocessor::parseIfndef()
	{
		int exprRet;
		if (!parseDefined(exprRet))
			return false;

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

					if (!exprRet)
					{
						if (!parseExpression(exprRet))
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

	bool Preprocessor::parseDefined(int& ret)
	{
		if (!skipSpaceInLine())
			return false;

		StringView value;
		if (!tokenIdentifier(value))
			return false;

		auto marco = mMarcoSymbolMap.find(value);
		ret = (marco == mMarcoSymbolMap.end()) ? 0 : 1;
		return true;
	}

	bool Preprocessor::parseExpression(int& ret)
	{
		if (!skipSpaceInLine())
			return false;

		return parseExprOp(ret);
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

		skipSpaceInLine();

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
				mInput.advance();
				mInput.advance();
				if (!parseExprFactor(ret))
					return false;

				++ret;
			}
			else
			{
				mInput.advance();
				if (!parseExprFactor(ret))
					return false;
			}
			break;
		case '-':
			if (mInput.mCur[1] == '-')
			{
				mInput.advance();
				mInput.advance();
				if (!parseExprFactor(ret))
					return false;

				--ret;
			}
			else
			{
				mInput.advance();
				if (!parseExprFactor(ret))
					return false;

				ret = -ret;
			}
			break;
		case '!':
			{
				mInput.advance();
				if (!parseExprFactor(ret))
					return false;
				ret = !ret;
			}
			break;
		default:
			if (!parseExprValue(ret))
				return false;
		}

		return true;
	}

	bool Preprocessor::parseExprValue(int& ret)
	{
		if (tokenChar('('))
		{
			parseExpression(ret);

			if (!tokenChar(')'))
				PARSE_ERROR("Error Expression");

			return true;
		}
		else if (tokenNumber(ret))
		{
			return true;
		}
		StringView value;
		if (!tokenIdentifier(value))
			return false;

		auto marco = mMarcoSymbolMap.find(value);
		if (marco == mMarcoSymbolMap.end())
		{
			ret = 0;
			return true;
		}

		if (marco->second.evalFrame < mCurFrame)
		{
			CodeLoc exprLoc;
			exprLoc.mCur = marco->second.expr.c_str();
			exprLoc.mLineCount = 0;
			InputLocScope scope(*this, exprLoc);
			if (!parseExpression(marco->second.cacheEvalValue))
			{
				PARSE_ERROR("Define is not eval Expresstion");
			}
			marco->second.evalFrame = mCurFrame;
		}

		ret = marco->second.cacheEvalValue;
		return true;
	}

	bool Preprocessor::tokenOp(EOperatorPrecedence::Type precedence, EOperator::Type& outType)
	{
		int len = FExpressionUitlity::FindOperator( mInput.mCur , precedence , outType );
		if (len == 0)
			return false;
		
		assert(len <= 2);
		mInput.advance();
		if (len == 2)
		{
			mInput.advance();
		}
		return true;
	}

	bool Preprocessor::ensureInputValid()
	{
		if (mInput.isEof())
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
		return !mInput.isEof();
	}

	bool Preprocessor::skipSpace()
	{
		while (ensureInputValid())
		{
			mInput.skipSpace();
			if (!mInput.isEof())
				return true;
		}

		return false;
	}

	bool Preprocessor::skipToNextLine()
	{
		if (mInput.isEof())
		{
			ensureInputValid();
		}
		else
		{
			mInput.skipToNextLine();
			if (mInput.isEof())
				return false;
		}
		return true;
	}

	bool Preprocessor::skipSpaceInLine()
	{
		mInput.skipSpaceInLine();
		return !mInput.isEof();
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
		char const* cur = text.data();
		char const* end = text.data() + text.size();
		while (cur < end)
		{
			if (IsValidStartCharForIdentifier(*cur))
			{
				char const* pIdStart = cur;

				for (;;)
				{
					CodeLoc::Advance(cur);
					if (!IsValidCharForIdentifier(*cur))
						break;
				}

				StringView id = StringView(pIdStart, cur - pIdStart);

				auto iter = mMarcoSymbolMap.find(id);
				if (iter != mMarcoSymbolMap.end())
				{
					outText += iter->second.expr;
				}
				else
				{
					outText.append(id.data(), id.size());
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

	void Preprocessor::getIncludeFiles(std::vector< HashString >& outFiles)
	{
		for( auto& pair : mLoadedSourceMap )
		{
			outFiles.push_back(pair.first);
		}
	}

	void Preprocessor::emitSourceLine(int lineOffset)
	{
		FixString< 512 > lineMarco;
		lineMarco.format("#line %d \"%s\"\n", mInput.getLine() + lineOffset, (mInput.source) ? mInput.source->filePath.c_str() : "");
		mOutput->push(StringView(lineMarco.c_str()));
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

		for (;;)
		{
			mInput.advance();
			if (!IsValidCharForIdentifier(*mInput.mCur))
				break;
		}

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

	void CodeSource::appendString(char const* str)
	{
		if (!mBuffer.empty())
			mBuffer.pop_back();
		mBuffer.insert(mBuffer.end(), str, str + strlen(str));
		mBuffer.push_back(0);
	}

	void CodeSource::appendString(char const* str , int num)
	{
		if (!mBuffer.empty())
			mBuffer.pop_back();
		mBuffer.insert(mBuffer.end(), str, str + num);
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
		struct OpInfo
		{
			EOperator::Type type;
			StaticString    text;
		};

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
		static const OpInfo OpInfos[] =
		{
#define OPINFO_OP( NAME , OP , P ) { EOperator::NAME , #OP } ,
				BINARY_OPERATOR_LIST(OPINFO_OP)
#undef OPINFO_OP
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

	CPP::OperationInfo FExpressionUitlity::GetOperationInfo(EOperator::Type type)
	{
		static const OperationInfo OpInfos[] =
		{
#define OPINFO_OP( NAME , OP , P ) { EOperator::NAME , #OP } ,
				BINARY_OPERATOR_LIST(OPINFO_OP)
#undef OPINFO_OP
		};

		return OpInfos[type];
	}

}//namespace CPP
