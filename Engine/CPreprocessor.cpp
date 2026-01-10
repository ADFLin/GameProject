#include "CPreprocessor.h"

#include "MacroCommon.h"
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

	void Preprocessor::pushInput(CodeSource& source)
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

		mInput.source = &source;
		mInput.resetSeek();
	}

	void Preprocessor::translate()
	{
		mBlockStateStack.clear();
		mIfScopeDepth = 0;
		parseCode(false);
	}

	void Preprocessor::translate(CodeSource& source)
	{
		mInputStack.clear();
		mInput.source = &source;
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
			if (isSkipText())
			{
				while (!IsInputEnd())
				{
					CodeLoc lineStart = mInput;
					mInput.skipSpaceInLine();
					if (mInput.tokenChar('#'))
					{
						mInput.setLoc(lineStart);
						break;
					}
					mInput.setLoc(lineStart);
					mInput.skipToNextLine();
				}
			}

			EControlParseResult result = EControlParseResult::OK;
			CodeLoc start = mInput;
			if (parseControlLine(result))
			{
				if (result == EControlParseResult::ExitBlock)
				{
					break;
				}
				else if (result == EControlParseResult::RetainText && !bSkip)
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

				if (bReplaceMacroText)
				{
					StringView lineText = mInput.getDifference(start);
					std::string text;
					expandMacro(lineText, text);
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

	bool Preprocessor::parseControlLine(EControlParseResult& result)
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

				if (bReplaceMacroText)
				{
					result = EControlParseResult::OK;
				}
				else
				{
					result = EControlParseResult::RetainText;
				}
			}
#else
			result = EControlParseResult::RetainText;
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
				result = EControlParseResult::RetainText;
			}
#else
			result = EControlParseResult::RetainText;
#endif
			break;
		case HashValue("ifdef"):
#if PARSE_DEFINE_IF
			if (!parseIfdef())
			{
				result = EControlParseResult::RetainText;
			}
#else
			result = EControlParseResult::RetainText;
#endif
			break;
		case HashValue("ifndef"):
#if PARSE_DEFINE_IF
			if (!parseIfndef())
			{
				result = EControlParseResult::RetainText;
			}
#else
			result = EControlParseResult::RetainText;
#endif
			break;
		case HashValue("line"):
			result = EControlParseResult::RetainText;
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

				if (bReplaceMacroText)
				{
					result = EControlParseResult::OK;
				}
				else
				{
					result = EControlParseResult::RetainText;
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
					result = EControlParseResult::RetainText;
				}
			}
			break;
		case HashValue("elif"):
		case HashValue("else"):
		case HashValue("endif"):
#if PARSE_DEFINE_IF
			mInput.setLoc(saveLoc);
			result = EControlParseResult::ExitBlock;
#else
			result = EControlParseResult::RetainText;
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
				result = EControlParseResult::RetainText;
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
		ExpandMacroResult expandResult;
		if (!expandMacro(pathText, expendText, expandResult))
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

		StringView macroName;
		if (!mInput.tokenIdentifier(macroName))
			return false;

		mInput.skipSpaceInLine();

		MacroSymbol macro;
		macro.vaArgs.indexArg = INDEX_NONE;
		macro.vaArgs.offset = -1;

		if (mInput.tokenChar('('))
		{
			if (!bSupportMacroArg)
			{
				PARSE_ERROR("Macro don't support Arg input");
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
					
					macro.vaArgs.indexArg = argList.size();
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
			macro.numArgs = argList.size();

			bool bNextArgConcat = false;

			while (!mInput.isEoL())
			{
				// Handle Concat Operator ##
				if (mInput[0] == '#' && mInput[1] == '#')
				{
					// Strip trailing space in expr (optimized: single resize instead of multiple pop_back)
					{
						size_t newLen = macro.expr.size();
						size_t limit = 0;
						if (!macro.argEntries.empty())
						{
							limit = macro.argEntries.back().offset;
						}

						while (newLen > limit && FCString::IsSpace(macro.expr[newLen - 1]))
							--newLen;
						macro.expr.resize(newLen);
					}

					// Mark previous arg as concat if it's at the end
					if (!macro.argEntries.empty())
					{
						auto& lastEntry = macro.argEntries.back();
						if (lastEntry.offset == macro.expr.size())
						{
							lastEntry.bConcat = true;
						}
					}

					bNextArgConcat = true;
					mInput.advance();
					mInput.advance();
					mInput.skipSpaceInLine();
					continue;
				}

				bool bHadArgString = false;
				StringView exprUnit;

				// Handle Stringify Operator #
				if (mInput[0] == '#')
				{
					bHadArgString = true;
					mInput.advance();
					// Note: mInput.skipSpaceInLine() should arguably happen here too by standard, 
					// but let's stick to simple logic first.
				}

				if (mInput.tokenIdentifier(exprUnit))
				{
					if (bHadArgString)
						macro.expr += '\"';

					if (macro.vaArgs.indexArg != INDEX_NONE && exprUnit == "__VA_ARGS__")
					{
						macro.vaArgs.offset = macro.expr.size();
						macro.vaArgs.bConcat = bNextArgConcat;
						macro.vaArgs.bStringify = bHadArgString;
					}
					else
					{
						int idxArg = FindArg(exprUnit);
						if (idxArg != INDEX_NONE)
						{
							MacroSymbol::ArgEntry entry;
							entry.indexArg = idxArg;
							entry.offset = macro.expr.size();
							entry.bStringify = bHadArgString;
							entry.bConcat = bNextArgConcat;
							macro.argEntries.push_back(entry);
						}
						else
						{
							if (bHadArgString)
								macro.expr += '#'; // Was not an arg, so # is just a char

							macro.expr.append(exprUnit.begin(), exprUnit.end());
						}
					}

					if (bHadArgString)
						macro.expr += '\"';
					
					bNextArgConcat = false;
				}
				else
				{
					if (bHadArgString)
						macro.expr += '#';
					
					macro.expr += mInput[0];
					mInput.advance();
					bNextArgConcat = false;
				}
			}

		}
		else
		{
			mInput.skipSpaceInLine();

			macro.numArgs = 0;
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

				macro.expr.append(exprText.begin(), exprText.end());
				macro.evalFrame = -1;
			}
			else
			{
				macro.expr = "";
				macro.evalFrame = INT_MAX;
				macro.cachedEvalValue = 0;
			}
		}

		HashString macroString{ macroName , true };
		auto iter = mMacroSymbolMap.find(macroString);
		if (iter != mMacroSymbolMap.end())
		{
			if (bAllowRedefineMacro)
			{
				PARSE_WARNING("Macro %s is defined : %s -> %s" , macroString.c_str() , iter->second.expr.c_str() ,  macro.expr.c_str() );
				mMacroSymbolMap.emplace_hint(iter, macroString, std::move(macro));
				++mCurFrame;
			}
			else
			{
				PARSE_ERROR("Macro %s is defined", macroString.c_str());
			}
		}
		else
		{
			mMacroSymbolMap.emplace(macroString, std::move(macro));
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
			return false;

		mMacroSymbolMap.erase(nameKey);
		return true;
	}

	bool Preprocessor::parseDefined(int& ret)
	{
		mInput.skipSpaceInLine();

		StringView idName;
		if (!mInput.tokenIdentifier(idName))
			return false;

		ret = (findMacro(idName) == nullptr) ? 0 : 1;
		return true;
	}

	bool Preprocessor::parseExpression(int& ret)
	{
		mInput.skipSpaceInLine();

		ExpressionEvaluator evaluator(mInput);
		if (!evaluator.evaluate(ret))
			return false;
		
		mInput.setLoc(evaluator.getInput());
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
					         , public CodeTokenUtilityT<LineStringViewCode>
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
				auto macro = findMacro(id);
				if (macro == nullptr)
				{
					PARSE_WARNING( "%s is not defined" , id.toCString() );
					ret = 0;
				}
				else
				{
					if (macro->evalFrame < mCurFrame)
					{
						std::string expandedExpr;
						if (!expandMacro(macro->expr, expandedExpr))
						{
							return false;
						}
						CodeLoc exprLoc;
						exprLoc.mCur = expandedExpr.c_str();
						exprLoc.mLineCount = 0;
						if (!parseExpression(exprLoc, macro->cachedEvalValue))
						{
							PARSE_ERROR("Define \"%s\" is not Const Expression", id.toCString());
						}
						macro->evalFrame = mCurFrame;
					}

					ret = macro->cachedEvalValue;
				}

				return true;
			}
		}

		std::string exprTextConv;


		ExpandMacroResult expandResult;
		bool bHadExpanded = false;
		if (id.length())
		{
			if (!checkIdentifierToExpand(id, exprCode, exprTextConv, expandResult, 0, nullptr))
				return false;
		}

		if (!expandMacroInternal(exprCode, exprTextConv, expandResult, 0, nullptr))
		{
			return false;
		}

		CodeLoc exprLoc;
		exprLoc.mCur = exprTextConv.c_str();
		exprLoc.mLineCount = 0;
		if (!parseExpression(exprLoc, ret))
		{
			PARSE_ERROR("Condition Expression of If eval Fail : %s", exprText.toCString());
			return false;
		}

		mInput.skipSpaceInLine();
		if (!mInput.isEoF() && !mInput.isEoL())
		{

			return false;
		}

		return true;
	}


	bool Preprocessor::expandMacro(StringView const& lineText,std::string& outText)
	{
		ExpandMacroResult expandResult;
		LineStringViewCode codeView(lineText);
		return expandMacroInternal(codeView, outText, expandResult, 0, nullptr);
	}

	bool Preprocessor::expandMacro(StringView const& lineText, std::string& outText, ExpandMacroResult& outExpandResult)
	{
		LineStringViewCode codeView(lineText);
		return expandMacroInternal(codeView, outText, outExpandResult, 0, nullptr);
	}

	bool Preprocessor::checkIdentifierToExpand(StringView const& id, class LineStringViewCode& code, std::string& outText, ExpandMacroResult& outResult, int depth, MacroExpansionContext const* context)
	{
		if (depth >= MAX_MACRO_EXPANSION_DEPTH)
		{
			PARSE_ERROR("Macro expansion depth exceeded maximum limit (%d) for: %s", MAX_MACRO_EXPANSION_DEPTH, id.toCString());
			return false;
		}

		MacroSymbol* macro = findMacro(id);
		if (macro)
		{
			if (context && context->isSuppressed(macro))
			{
				outText.append(id.begin(), id.end());
				return true;
			}
			
			MacroExpansionContext newContext{ macro, context };

			if (macro->numArgs > 0)
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
							PARSE_ERROR("Macro Syntax Error : %s", (char const*)id.toCString());
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
								PARSE_ERROR("Macro Syntax Error : %s", (char const*)id.toCString());
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


				if (macro->numArgs != argList.size() && macro->vaArgs.indexArg == INDEX_NONE)
				{
					PARSE_WARNING("Macro Args Count is not match");
				}

				// Pre-expand arguments
				TArray<std::string> expandedArgs;
				expandedArgs.resize(argList.size());
				for (int i = 0; i < argList.size(); ++i)
				{
					LineStringViewCode codeView(argList[i]);
					// Use newContext?? No, args are expanded in the caller's context?
					// Standard says: "Macro arguments are expanded... before being substituted"
					// This expansion happens in the CURRENT context, not the new macro's context.
					if (!expandMacroInternal(codeView, expandedArgs[i], outResult, depth + 1, context))
					{
						PARSE_ERROR("Expand Arg Error : %s", argList[i].toCString());
						return false;
					}
				}

				// Construct substitution text
				std::string substitutionText;
				// Reserve heuristic size
				substitutionText.reserve(macro->expr.size() + argList.size() * 16);

				int lastAppendOffset = 0;
				auto CheckAppendPrevExpr = [&]( int inOffset)
				{
					if (lastAppendOffset != inOffset)
					{
						auto itStart = macro->expr.begin() + lastAppendOffset;
						auto itEnd = macro->expr.begin() + inOffset;
						substitutionText.append(itStart, itEnd);

						lastAppendOffset = inOffset;
					}
				};

				for (auto const& entry : macro->argEntries)
				{
					CheckAppendPrevExpr(entry.offset);

					if (entry.indexArg < expandedArgs.size())
					{
						if (entry.bStringify || entry.bConcat)
							substitutionText.append(argList[entry.indexArg].begin(), argList[entry.indexArg].end());
						else
							substitutionText += expandedArgs[entry.indexArg];
					}
				}

				if (macro->vaArgs.indexArg != INDEX_NONE && macro->vaArgs.offset >= 0 )
				{
					CheckAppendPrevExpr(macro->vaArgs.offset);

					if (macro->vaArgs.indexArg < expandedArgs.size())
					{
						for (int indexArg = macro->vaArgs.indexArg; indexArg < expandedArgs.size(); ++indexArg)
						{
							if (indexArg != macro->vaArgs.indexArg)
								substitutionText += ",";

							if (macro->vaArgs.bConcat || macro->vaArgs.bStringify)
								substitutionText.append(argList[indexArg].begin(), argList[indexArg].end());
							else
								substitutionText += expandedArgs[indexArg];
						}
					}
					else if (macro->bVaEatComma)
					{
						for( auto index = substitutionText.size() - 1; index ; --index)
						{
							if (substitutionText[index] == ',')
							{
								substitutionText.erase(index);
								break;
							}

							if (!FCString::IsSpace(substitutionText[index]))
								break;
						}
					}
				}

				CheckAppendPrevExpr(macro->expr.size());

				// Deep Expand the result
				LineStringViewCode subCode(substitutionText);
				if (!expandMacroInternal(subCode, outText, outResult, depth + 1, &newContext))
				{
					return false;
				}

				outResult.numRefMacroWithArgs += 1;
			}
			else
			{
				LineStringViewCode codeView(macro->expr);
				if (!expandMacroInternal(codeView, outText, outResult, depth + 1, &newContext))
				{
					return false;
				}
				outResult.numRefMacro += 1;
			}
		}
		else
		{
			outText.append(id.begin(), id.end());
		}

		return true;
	}

	bool Preprocessor::expandMacroInternal(class LineStringViewCode& code, std::string& outText, ExpandMacroResult& outResult, int depth, MacroExpansionContext const* context)
	{
		if (depth >= MAX_MACRO_EXPANSION_DEPTH)
		{
			PARSE_ERROR("Macro expansion depth exceeded maximum limit (%d)", MAX_MACRO_EXPANSION_DEPTH);
			return false;
		}

		while (!code.isEOF())
		{
			StringView id;
			if (code.tokenIdentifier(id))
			{
				if (!checkIdentifierToExpand(id, code, outText, outResult, depth, context))
						return false;
			}
			else
			{
				// Batch append non-identifier characters for better performance
				char const* start = code.getCur();
				do
				{
					code.advance();
				} while (!code.isEOF() && !FCodeParse::IsValidStartCharForIdentifier(code.getChar()));
				outText.append(start, code.getCur() - start);
			}
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
		// Reserve capacity to reduce reallocations
		size_t maxDirLen = 0;
		for (auto const& dir : mFileSearchDirs)
		{
			if (dir.size() > maxDirLen)
				maxDirLen = dir.size();
		}
		fullPath.reserve(maxDirLen + name.size());

		for (auto const& dir : mFileSearchDirs)
		{
			fullPath = dir;
			fullPath += name;
			if( FFileSystem::IsExist(fullPath.c_str()) )
				return true;
		}
		return false;
	}


	Preprocessor::MacroSymbol* Preprocessor::findMacro(StringView const& name)
	{
		HashString nameKey;
		if (!HashString::Find(name, true, nameKey))
			return nullptr;

		auto iter = mMacroSymbolMap.find(nameKey);
		if (iter == mMacroSymbolMap.end())
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

	void Preprocessor::addSearchDir(char const* dir)
	{
		mFileSearchDirs.push_back(dir);
		std::string& dirAdd = mFileSearchDirs.back();
		if( dirAdd[dirAdd.length() - 1] != '/' )
		{
			dirAdd += '/';
		}
	}

	void Preprocessor::addDefine(char const* name, int value)
	{
		MacroSymbol macro;
		macro.expr = FStringConv::From(value);
		macro.evalFrame = INT_MAX;
		macro.cachedEvalValue = value;
		mMacroSymbolMap.emplace( HashString(name, true) , std::move(macro));
	}

	void Preprocessor::addInclude(char const* fileName, bool bSystemPath)
	{
		//#TODO
	}

	void Preprocessor::getUsedIncludeFiles(std::unordered_set< HashString >& outFiles)
	{
		outFiles.insert(mUsedFiles.begin(), mUsedFiles.end());
	}

	void Preprocessor::emitSourceLine(int lineOffset)
	{
		InlineString< 512 > lineMacro;
		int len = 0;
		switch (lineFormat)
		{
		case LF_LineNumber:
			len = lineMacro.format("#line %d\n", mInput.getLine() + lineOffset);
			break;
		case LF_LineNumberAndFilePath:
			len = lineMacro.format("#line %d \"%s\"\n", mInput.getLine() + lineOffset, mInput.getSourceName());
			break;
		}
		if (len)
		{
			mOutput->push(StringView(lineMacro.c_str(), len));
		}
	}

	void Preprocessor::emitCode(StringView const& code)
	{
		if (bAddLineMacro && bRequestSourceLine)
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

	int FExpressionUtility::FindOperator(char const* code, EOperatorPrecedence::Type precedence, EOperator::Type& outType)
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
	NOINLINE int FExpressionUtility::FindOperator(char const* code, EOperator::Type& outType)
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

	int FExpressionUtility::FindOperatorToEnd(char const* code, EOperatorPrecedence::Type precedenceStart, EOperator::Type& outType)
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

	constexpr OperationInfo FExpressionUtility::GetOperationInfo(EOperator::Type type)
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

	bool ExpressionEvaluator::evaluate(int& ret)
	{
		return parseExpression(ret);
	}

	bool ExpressionEvaluator::parseExpression(int& ret)
	{
		mLoc.skipSpaceInLine();

#if USE_TEMPLATE_PARSE_OP
		if (!parseExprOp< EOperatorPrecedence::Type(0)>(ret))
			return false;
#else
		if (!parseExprOp(ret))
			return false;
#endif
		return true;
	}

	bool ExpressionEvaluator::parseExprOp(int& ret, EOperatorPrecedence::Type precedence)
	{
		if (precedence == EOperatorPrecedence::Prefix)
		{
			return parseExprFactor(ret);
		}

		if (!parseExprOp(ret, EOperatorPrecedence::Type(precedence + 1)))
			return false;

		EOperator::Type op;
		while (tokenOp(precedence, op))
		{
			mLoc.skipSpaceInLine();

			int rhs;
			if (!parseExprOp(rhs, EOperatorPrecedence::Type(precedence + 1)))
				return false;

			if ((op == EOperator::Div || op == EOperator::Rem) && rhs == 0)
			{
				// Division by zero error
				return false;
			}

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
	bool ExpressionEvaluator::parseExprOp(int& ret)
	{
		if (!parseExprOp<EOperatorPrecedence::Type(Precedence + 1)>(ret))
			return false;

		EOperator::Type op;
		while (tokenOp< Precedence >(op))
		{
			mLoc.skipSpaceInLine();

			int rhs;
			if (!parseExprOp<EOperatorPrecedence::Type(Precedence + 1)>(rhs))
				return false;

			if constexpr (Precedence == EOperatorPrecedence::Multiplication)
			{
				if ((op == EOperator::Div || op == EOperator::Rem) && rhs == 0)
				{
					// Division by zero error
					return false;
				}
			}

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
	bool ExpressionEvaluator::parseExprOp< EOperatorPrecedence::Prefix >(int& ret)
	{
		return parseExprFactor(ret);
	}

	bool ExpressionEvaluator::parseExprFactor(int& ret)
	{
		switch (mLoc[0])
		{
		case '+':
			if (mLoc[1] == '+')
			{
				mLoc.advanceNoEoL(2);
				if (!parseExprFactor(ret))
					return false;

				++ret;
			}
			else
			{
				mLoc.advanceNoEoL(1);
				if (!parseExprFactor(ret))
					return false;
			}
			break;
		case '-':
			if (mLoc[1] == '-')
			{
				mLoc.advanceNoEoL(2);
				if (!parseExprFactor(ret))
					return false;

				--ret;
			}
			else
			{
				mLoc.advanceNoEoL(1);
				if (!parseExprFactor(ret))
					return false;

				ret = -ret;
			}
			break;
		case '~':
			{
				mLoc.advanceNoEoL(1);
				if (!parseExprFactor(ret))
					return false;
				ret = ~ret;
			}
			break;
		case '!':
			{
				mLoc.advanceNoEoL(1);
				if (!parseExprFactor(ret))
					return false;

				ret = !ret;
			}
			break;
		default:
			if (!parseExprValue(ret))
				return false;
		}

		mLoc.skipSpaceInLine();
		return true;
	}

	bool ExpressionEvaluator::parseExprValue(int& ret)
	{
		if (mLoc.tokenChar('('))
		{
			parseExpression(ret);

			if (!mLoc.tokenChar(')'))
			{
				// Error: Expression is invalid: missing ')'
				return false;
			}

			return true;
		}
		else if (mLoc.tokenNumber(ret))
		{
			mLoc.skipSpaceInLine();
			return true;
		}

		ret = 0;
		return true;
	}

	bool ExpressionEvaluator::tokenOp(EOperatorPrecedence::Type precedence, EOperator::Type& outType)
	{
#if USE_OPERATION_CACHE
		if (mParsedCachedOP == EOperator::None)
		{
			if (mInput.isEoL())
			{
				return false;
			}

			int len = FExpressionUtility::FindOperatorToEnd(mInput.getCur(), precedence, mParsedCachedOP);
			if (len == 0)
				return false;

			assert(len <= 2);
			mParesedCachedOPPrecedence = FExpressionUtility::GetOperationInfo(mParsedCachedOP).precedence;
			mInput.advanceNoEoL(len);
		}

		if (mParesedCachedOPPrecedence != precedence)
			return false;

		outType = mParsedCachedOP;
		mParsedCachedOP = EOperator::None;
		return true;
#else
		if (mLoc.isEoL())
		{
			return false;
		}

		int len = FExpressionUtility::FindOperator(mLoc.getCur(), precedence, outType);
		if (len == 0)
			return false;

		assert(len <= 2);
		mLoc.advanceNoEoL(len);
		return true;
#endif
	}

	template< EOperatorPrecedence::Type Precedence >
	NOINLINE bool ExpressionEvaluator::tokenOp(EOperator::Type& outType)
	{
		if (mLoc.isEoL())
		{
			return false;
		}

		int len = FExpressionUtility::FindOperator< Precedence >(mLoc.getCur(), outType);
		if (len == 0)
			return false;

		assert(len <= 2);
		mLoc.advanceNoEoL(len);
		return true;
	}

	// Explicit instantiations for template methods
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::LogicalOR>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::LogicalAND>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::BitwiseOR>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::BitwiseXOR>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::BitwiseAND>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::Equality>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::Comparison>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::Shift>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::Addition>(int&);
	template bool ExpressionEvaluator::parseExprOp<EOperatorPrecedence::Multiplication>(int&);

	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::LogicalOR>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::LogicalAND>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::BitwiseOR>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::BitwiseXOR>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::BitwiseAND>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::Equality>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::Comparison>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::Shift>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::Addition>(EOperator::Type&);
	template bool ExpressionEvaluator::tokenOp<EOperatorPrecedence::Multiplication>(EOperator::Type&);
	
}//namespace CPP
