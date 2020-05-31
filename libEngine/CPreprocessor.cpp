#include "CPreprocessor.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "FixString.h"
#include "Holder.h"
#include "Core/StringConv.h"

#define SYNTAX_ERROR( MSG ) throw SyntaxError( MSG );
#define FUNCTION_CHECK( func ) func

#define PARSE_DEFINE_IF 1

namespace CPP
{
	static uint32 HashValue(StringView const& str)
	{
		return FNV1a::MakeHash<uint32>((uint8 const*)str.data(), str.length()) & 0xfff;
	}
	static constexpr uint32 HashValue(char const* str)
	{
		return FNV1a::MakeStringHash<uint32>(str) & 0xfff;
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

	bool Preprocessor::parseCode(bool bSkip)
	{
		if (!mScopeStack.empty())
		{
			bSkip |= mScopeStack.back().bSkipText;
		}

		ScopeState state;
		state.bSkipText = bSkip;

		mScopeStack.push_back(state);


		while (haveMore())
		{
			EControlPraseResult result = EControlPraseResult::OK;
			char const* start = mInput.mCur;
			if (parseControlLine(result))
			{
				if (result == EControlPraseResult::ExitBlock)
				{
					break;
				}
				else if (result == EControlPraseResult::RetainText && !bSkip)
				{
					mInput.mCur = start;
					mInput.skipToNextLine();
					mOutput->push(start, mInput.mCur - start);
					++numLine;
				}
			}
			else if (!bSkip)
			{
				mInput.mCur = start;
				mInput.skipToNextLine();
				++numLine;

				if (bReplaceMarcoText)
				{
					//FIXME
					mOutput->push(start, mInput.mCur - start);
				}
				else
				{
					mOutput->push(start, mInput.mCur - start);
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

	bool Preprocessor::parseDefine()
	{
		if (!skipSpaceInLine())
			return false;

		StringView marcoName;
		if (!tokenIdentifier(marcoName))
			return false;

		if (tokenChar('('))
		{




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
				marco.setExpression(exprText);
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

				mMarcoSymbolMap.emplace_hint(iter, marcoName, marco);
			}
			else
			{
				mMarcoSymbolMap.emplace(marcoName, marco);
			}
		}

		skipToNextLine();
		return true;
	}

	bool Preprocessor::parseIf()
	{
		int exprRet;
		if (!parseExpression(exprRet))
			return false;

		skipToNextLine();
		return parseIfInternal(exprRet);
	}

	bool Preprocessor::parseIfdef()
	{
		int exprRet;
		if (!parseDefined(exprRet))
			return false;

		return parseIfInternal(exprRet);
	}

	bool Preprocessor::parseIfndef()
	{
		int exprRet;
		if (!parseDefined(exprRet))
			return false;

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
						SYNTAX_ERROR("if error");
					if (!exprRet)
					{
						if (!parseExpression(exprRet))
							return false;

						if (!parseCode(exprRet == 0))
						{
							return false;
						}

						skipToNextLine();
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
				if ( mInput.source && !mInput.source->mFilePath.empty() )
					mParamOnceSet.insert(mInput.source->mFilePath);
			}
			break;

		default:
			SYNTAX_ERROR("Unknown Param Command");
			return false;
		}


		skipToNextLine();
		return true;
	}

	bool Preprocessor::parseExpression(int& ret)
	{
		if (!skipSpaceInLine())
			return false;

		if (!parseExprValue(ret))
			return false;

		skipSpaceInLine();
		int rValue;
		switch ( *mInput.mCur )
		{
		case '!':
			if (mInput.mCur[1] == '=')
			{
				mInput.advance();
				mInput.advance();

				if (!parseExprValue(rValue))
					return false;
				ret = (ret != rValue);
			}
		case '>':
			mInput.advance();
			if (mInput.mCur[1] == '=')
			{
				mInput.advance();
				if (!parseExprValue(rValue))
					return false;
				ret = (ret >= rValue);
			}
			else
			{
				if (!parseExprValue(rValue))
					return false;
				ret = (ret > rValue);
			}
			break;
		case '<':
			mInput.advance();
			if (mInput.mCur[1] == '=')
			{
				mInput.advance();
				int rValue;
				if (!parseExprValue(rValue))
					return false;
				ret = (ret <= rValue);
			}
			else
			{
				if (!parseExprValue(rValue))
					return false;
				ret = (ret < rValue);
			}
			break;
		case '=':
			{
				if (mInput.mCur[1] != '=')
				{
					return false;
				}
				mInput.advance();
				mInput.advance();
				int rValue;
				if (!parseExprValue(rValue))
					return false;
				ret = (ret == rValue);
			}
			break;

		default:
			break;
		}

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

	bool Preprocessor::parseExprValue(int& ret)
	{
		if (!parseExprPart(ret))
			return false;

		skipSpaceInLine();
		while ((*mInput.mCur == '|' && mInput.mCur[1] == '|') || (*mInput.mCur == '&' && mInput.mCur[1] == '&'))
		{
			int temp;
			switch (*mInput.mCur)
			{
			case '|':
				{
					mInput.advance();
					mInput.advance();
					if (!parseExprPart(temp))
						return false;
					ret = (ret || temp);
				}
				break;
			case '&':
				{
					mInput.advance();
					mInput.advance();
					if (!parseExprPart(temp))
						return false;
					ret = (ret && temp);
				}
				break;
			}
			skipSpaceInLine();
		}
		return true;
	}

	bool Preprocessor::parseExprPart(int& ret)
	{
		if (!parseExprTerm(ret))
			return false;

		skipSpaceInLine();
		while (*mInput.mCur == '+' || *mInput.mCur == '-')
		{
			switch (*mInput.mCur)
			{
			case '+':
				{
					mInput.advance();
					int temp;
					if (!parseExprTerm(temp))
						return false;
					ret += temp;
				}
				break;
			case '-':
				{
					mInput.advance();
					int temp;
					if (!parseExprTerm(temp))
						return false;
					ret -= temp;
				}
				break;
			}
			skipSpaceInLine();
		}

		return true;
	}

	bool Preprocessor::parseExprTerm(int& ret)
	{
		if (!parseExprFactor(ret))
			return false;

		skipSpaceInLine();
		while (*mInput.mCur == '*' || *mInput.mCur == '/' || *mInput.mCur == '%')
		{
			int temp;
			switch (*mInput.mCur)
			{
			case '+':
				{
					mInput.advance();
					if (!parseExprFactor(temp))
						return false;
					ret += temp;
				}
				break;
			case '-':
				{
					mInput.advance();
					if (!parseExprFactor(temp))
						return false;
					ret -= temp;
				}
				break;
			case '%':
				{
					mInput.advance();
					if (!parseExprFactor(temp))
						return false;
					ret %= temp;
				}
				break;
			}
			skipSpaceInLine();
		}
		return true;
	}

	bool Preprocessor::parseExprFactor(int& ret)
	{
		if (!skipSpaceInLine())
			return false;

		if (tokenChar('!'))
		{
			if (!parseExprFactor(ret))
				return false;
			ret = !ret;
		}
		else
		{
			if (!parseExprAtom(ret))
				return false;
		}
		return true;
	}

	bool Preprocessor::parseExprAtom(int& ret)
	{
		if (!skipSpaceInLine())
			return false;

		if (tokenChar('('))
		{
			parseExpression(ret);

			if (!tokenChar(')'))
				SYNTAX_ERROR("Error Expression");

			return true;
		}
		else if (tokenNumber(ret))
		{
			return true;
		}
		StringView value;
		if (!tokenIdentifier(value))
			return false;

		if (value == "USE_PERSPECTIVE_DEPTH")
		{
			int i = 1;
		}

		auto marco = mMarcoSymbolMap.find(value);
		if (marco == mMarcoSymbolMap.end())
		{
			ret = 0;
			return true;
		}


		if (marco->second.evalFrame < mCurFrame)
		{
			marco->second.cacheEvalValue = FStringConv::To<int>(marco->second.expr.c_str());
			marco->second.evalFrame = mCurFrame;
		}

		ret = marco->second.cacheEvalValue;
		return true;
	}

	bool Preprocessor::parseInclude()
	{
		if (!skipSpaceInLine())
			return false;

		StringView pathText;
		if (!tokenString(pathText))
			SYNTAX_ERROR("Error include Format");
		if (pathText.size() < 2)
			SYNTAX_ERROR("Error include Format");

		bool bSystemPath = false;
		if (pathText[1] == '\"')
		{
			if (pathText[pathText.size() - 1] != '\"')
				SYNTAX_ERROR("Error include Format");
		}
		else if (pathText[0] == '<')
		{
			if (pathText[pathText.size() - 1] != '>')
				SYNTAX_ERROR("Error include Format");

			bSystemPath = true;
		}

		StringView path{ pathText.data() + 1 , pathText.size() - 2 };
		if (path.size() == 0)
		{
			SYNTAX_ERROR("No include file Name");
		}

		std::string fullPath;
		if (!findFile(path.toStdString(), fullPath))
			SYNTAX_ERROR("Can't find include file");

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
					SYNTAX_ERROR("Can't open include file");
				}
				includeSource = includeSourcePtr.get();
				mLoadedSourceMap.emplace(fullPathHS, includeSourcePtr.release());
			}
			else
			{
				includeSource = iter->second;
			}


			includeSource->mFilePath = fullPathHS;

			if (bCommentIncludeFileName)
			{
				mOutput->push("//Include ");
				mOutput->push(path);
				mOutput->pushNewline();
			}

			if (bAddLineMarco)
			{
				FixString< 512 > lineMarco;
				lineMarco.format("#line %d \"%s\"\n", 1, includeSource->mFilePath.c_str());
				mOutput->push(lineMarco);
			}

			InputEntry entry;
			entry.input = mInput;

			CodeInput temp = mInput;
			entry.onFinish = [this,temp,path]()
			{
				mOutput->pushNewline();
				if (bCommentIncludeFileName)
				{
					mOutput->push("//~Include ");
					mOutput->push(path);
					mOutput->pushNewline();
				}
				if (bAddLineMarco)
				{
					FixString< 512 > lineMarco;
					lineMarco.format("#line %d \"%s\"\n", temp.mLine, temp.source->mFilePath.c_str());
					mOutput->push(lineMarco);
				}
			};
			mInputStack.push_back(entry);
			mInput.source = includeSource;
			mInput.resetSeek();
		}

		return true;
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


	void Preprocessor::translate(CodeSource& sorce)
	{
		mInputStack.clear();
		mInput.source = &sorce;
		mInput.resetSeek();
		mScopeStack.clear();
		mIfScopeDepth = 0;

		parseCode(false);
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


	

	void CodeSource::appendString(char const* str)
	{
		if (!mBuffer.empty())
			mBuffer.pop_back();
		mBuffer.insert(mBuffer.end(), str, str + strlen(str));
		mBuffer.push_back(0);
	}

	bool CodeSource::loadFile(char const* path)
	{
		if (!FileUtility::LoadToBuffer(path, mBuffer, true))
			return false;

		return true;
	}

}//namespace CPP
