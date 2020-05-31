#pragma once
#ifndef CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
#define CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685

#include "StringParse.h"
#include "HashString.h"

#include <vector>
#include <string>
#include <ostream>
#include "Core/FNV1a.h"
#include <unordered_map>
#include <functional>
#include <unordered_set>

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

	class CodeLoc
	{
	public:
		void skipToNextLine()
		{
			for(;;)
			{
				mCur = FStringParse::FindChar(mCur, '\\', '\n');
				if (*mCur == 0)
					break;

				if (*mCur == '\\')
				{
					if (mCur[1] == '\n')
					{
						mCur += 2;
						mLine += 1;
					}
					else if (mCur[1] == '\r' && mCur[2] == '\n')
					{
						mCur += 3;
						mLine += 1;
					}
				}
				else
				{
					++mLine;
					++mCur;
					break;
				}
			}
		}

		void skipToLineEnd()
		{
			for (;;)
			{
				mCur = FStringParse::FindChar(mCur, '\\', '\n');
				if (*mCur == 0)
					break;

				if (*mCur == '\\')
				{
					if (mCur[1] == '\n')
					{
						mCur += 2;
						mLine += 1;
					}
					else if (mCur[1] == '\r' && mCur[2] == '\n')
					{
						mCur += 3;
						mLine += 1;
					}
				}
				else
				{
					break;
				}
			}
		}


		void skipSpace()
		{
			while (*mCur)
			{
				if (!::FCString::IsSpace(*mCur))
					break;

				if (*mCur == '\n')
					++mLine;
				++mCur;
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


		void advance()
		{
			++mCur;
			if (mCur[0] == '\\')
			{
				if (mCur[1] == '\n')
				{
					mCur += 2;
					mLine += 1;
				}
				else if (mCur[1] == '\r' && mCur[2] == '\n')
				{
					mCur += 3;
					mLine += 1;
				}
			}
		}

		bool isEof() const { return *mCur == 0; }

		static void Advance(char const*& code)
		{
			++code;
			if (code[0] == '\\')
			{
				if (code[1] == '\n')
				{
					code += 2;
				}
				else if (code[1] == '\r' && code[2] == '\n')
				{
					code += 3;
				}			
			}
		}
		static void Backward(char const*& code)
		{
			--code;
			if (code[0] == '\n')
			{
				if (code[-1] == '\\')
				{
					code -= 2;
				}
				else if (code[-1] == '\r' && code[-2] == '\\')
				{
					code -= 3;
				}
			}
		}

		char const* mCur;
		int mLine;
	};

	class CodeSource
	{
	public:
		void appendString(char const* str);
		bool loadFile(char const* path);

	private:

		std::vector< char > mBuffer;
		HashString    mFilePath;
		friend class Preprocessor;
		friend class CodeInput;
	};

	class CodeInput : public CodeLoc
	{
	public:
		CodeSource* source;

		void resetSeek()
		{
			assert(source);
			mCur = &source->mBuffer[0];
			//skip BOM
			if( mCur[0] == '\xef' &&
			    mCur[1] == '\xbb' &&
			    mCur[2] == '\xbF' )
			{
				mCur += 3;
			}

			mLine = 0;
		}
		void setLoc(CodeLoc const& loc)
		{
			mCur = loc.mCur;
			mLine = loc.mLine;
		}
	private:
		friend class Preprocessor;
	};

	class SyntaxError : public std::exception
	{
	public:
		STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( SyntaxError )
	};

	struct MarcoSymbol
	{
		StringView  name;
		std::string expr;

		void setExpression(StringView const& exprText)
		{
			expr = exprText.toStdString();
		}
		
		struct ArgEntry
		{
			int indexArg;
			int pos;
		};
		std::vector< ArgEntry > argEntries;
		
		int cacheEvalValue;
		int evalFrame;
	};

	class Preprocessor
	{
	public:
		Preprocessor();
		~Preprocessor();

		void translate( CodeSource& sorce );
		void setOutput( CodeOutput& output);
		void addSreachDir(char const* dir);
		void addDefine(char const* name, int value)
		{



		}

		void getIncludeFiles(std::vector< HashString >& outFiles);

		bool bSupportMarcoArg = false;
		bool bReplaceMarcoText = false;
		bool bCanRedefineMarco = false;
		bool bCommentIncludeFileName = true;
		bool bAddLineMarco = false;

	private:

		bool ensureInputValid()
		{
			if (mInput.isEof())
			{
				if (mInputStack.empty())
					return false;

				if (mInputStack.back().onFinish)
					mInputStack.back().onFinish();

				mInput = mInputStack.back().input;
				mInputStack.pop_back();
			}
			return true;
		}
		bool haveMore()
		{
			ensureInputValid();
			return !mInput.isEof();
		}

		bool skipSpace()
		{
			while (ensureInputValid())
			{
				mInput.skipSpace();
				if (!mInput.isEof())
					return true;
			}
			
			return false;
		}

		bool skipToNextLine()
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

		bool skipSpaceInLine()
		{
			mInput.skipSpaceInLine();
			return !mInput.isEof();
		}

		bool tokenChar(char c)
		{
			if (*mInput.mCur != c)
				return false;

			mInput.advance();
			return true;
		}

		static bool IsValidStartCharForIdentifier(char c)
		{
			return ::isalpha(c) || c == '_';
		}

		static bool IsValidCharForIdentifier(char c)
		{
			return ::isalnum(c) || c == '_';
		}


		bool tokenIdentifier(StringView& outIdentifier)
		{
			char const* start = mInput.mCur;

			if (!IsValidStartCharForIdentifier(*mInput.mCur))
				return false;

			for (;;)
			{
				mInput.advance();
				if ( !IsValidCharForIdentifier(*mInput.mCur) )
					break;
			}

			outIdentifier = StringView(start, mInput.mCur - start);
			return true;
		}

		bool tokenNumber(int& outValue);

		bool tokenString(StringView& outString)
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
				if ( !FCString::IsSpace(*end))
					break;

				CodeLoc::Backward(end);
			}

			outString = StringView(start, end - start + 1);
			return true;
		}

		bool tokenControl(StringView& outName)
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

		enum EControlPraseResult
		{
			OK ,
			ExitBlock ,
			RetainText ,
		};

		bool parseControlLine(EControlPraseResult& result);
		bool parseCode(bool bSkip);
		bool parseDefine();
		bool parseIf();
		bool parseIfdef();
		bool parseIfndef();
		bool parseIfInternal(int exprRet);

		bool parsePragma();
		bool parseExpression(int& ret);
		bool parseDefined(int& ret);

		bool parseExprValue(int& ret);
		bool parseExprPart(int& ret);
		bool parseExprTerm(int& ret);
		bool parseExprFactor(int& ret);
		bool parseExprAtom(int& ret);
		int mCurFrame = 0;
		bool parseInclude();

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

		int mIfScopeDepth;
		std::unordered_map< HashString, MarcoSymbol > mMarcoSymbolMap;

		bool execExpression(CodeInput& input, int& evalValue)
		{
			return false;
		}

		bool execExpression( std::string const& expr , int& outValue )
		{
			//#TODO:support \ connect
			return true;
		}

		CodeInput mInput;
		struct  InputEntry
		{
			CodeInput input;
			std::function< void() > onFinish;
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
		std::unordered_map< HashString, CodeSource* > mLoadedSourceMap;


		struct OperatorInfo
		{
			char value;
			bool bRightAssoc;
			int  prodence;
		};

	};
}


#endif // CPreprocessor_H_09DCE0C0_9F70_44E8_A8B4_1C2DF2BC8685
