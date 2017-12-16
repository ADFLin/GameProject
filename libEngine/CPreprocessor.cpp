#include "CPreprocessor.h"

#include "MarcoCommon.h"
#include "FileSystem.h"
#include "Holder.h"

#define SYNTAX_ERROR( MSG ) throw SyntaxError( MSG );
#define FUNCTION_CHECK( fun ) fun

namespace CPP
{
	struct
	{
		Command com;
		char const* name;
	} gCommandList[] =
	{
		{ Command::Include , "include" },
		{ Command::Pragma , "pragma" } ,
#if 0
		{ Command::Define , "define" },

		{ Command::If , "if" },
		{ Command::Ifdef ,"ifdef" },
		{ Command::Ifndef ,"ifndef" },
		{ Command::Elif , "elif" },
		{ Command::Else , "else" },
		{ Command::Endif , "endif" },
		
#endif
	};


	Preprocessor::Preprocessor()
	{
		mScopeCount = 0;
		mDelimsTable.addDelims(" \t\r", DelimsTable::DropMask);
		mDelimsTable.addDelims("\n", DelimsTable::StopMask);

		mDelimsTable.addDelims(" \t\r", DelimsTable::DropMask);
		mDelimsTable.addDelims("+-*/><=!&|()\n", DelimsTable::StopMask);

		if( sCommandMap.empty() )
		{
			for( int i = 0; i < ARRAY_SIZE(gCommandList); ++i )
			{
				sCommandMap.insert(std::make_pair(TokenString(gCommandList[i].name), gCommandList[i].com));
			}
		}
	}

	Preprocessor::~Preprocessor()
	{
		for( auto input : mLoadedInput )
		{
			delete input;
		}
		mLoadedInput.clear();
	}

	CPP::TokenInfo Preprocessor::nextToken(CodeInput& input)
	{
		TokenInfo result;

		TokenString str;
		switch( ParseUtility::StringToken(input.mCur, mDelimsTable, str) )
		{
		case ParseUtility::eDelimsType:

			break;
		case ParseUtility::eStringType:
			result.type = Token_String;
			result.str = str;
			break;
		case ParseUtility::eNoToken:
			result.type = Token_Eof;
			break;
		};
		return result;
	}

	bool Preprocessor::execInclude(CodeInput& input)
	{
		TokenInfo token = nextToken(input);
		if( token.type != Token_String )
			SYNTAX_ERROR("Error include Format");

		if( token.str.num < 2 )
			SYNTAX_ERROR("Error include Format");

		bool bSystemPath = false;
		if( token.str[1] == '\"' )
		{
			if( token.str[token.str.num - 1] != '\"' )
				SYNTAX_ERROR("Error include Format");
		}
		else if( token.str[0] == '<' )
		{
			if( token.str[token.str.num - 1] != '>' )
				SYNTAX_ERROR("Error include Format");

			bSystemPath = true;
		}

		TokenString path;
		path.ptr = token.str.ptr + 1;
		path.num = token.str.num - 2;

		if( mParamOnceSet.find(path.toStdString()) == mParamOnceSet.end() )
		{
			std::string fullPath;
			if ( !findFile(path.toStdString() , fullPath ) )
				SYNTAX_ERROR("Can't find include file");

			TPtrHolder< CodeInput > includeInput( new CodeInput );
			if( !includeInput->loadFile(fullPath.c_str()) )
			{
				SYNTAX_ERROR("Can't open include file");
			}
			includeInput->mFileName = path.toStdString();
			includeInput->reset();

			if( bCommentIncludeFileName )
			{
				mOutput->push("//Include ");
				mOutput->push( path.ptr , path.num );
				mOutput->pushNewline();
			}

			translate(*includeInput);
			mOutput->pushNewline();

			if( bCommentIncludeFileName )
			{
				mOutput->push("//~Include ");
				mOutput->push(path.ptr, path.num);
				mOutput->pushNewline();
			}
			mLoadedInput.push_back(includeInput.release());
		}

		input.skipLine();
		return true;
	}

	bool Preprocessor::execIf(CodeInput& input)
	{
		return true;

		//int evalValue = execExpression(input);

		//if( evalValue != 0 )
		//{
		//	translate(input);
		//}
		//else
		//{

		//}
	}

	bool Preprocessor::execDefine(CodeInput& input)
	{
		return true;

		TokenInfo token = nextToken(input);

		if ( token.type == Token_Eof )
			SYNTAX_ERROR("Define Syntex Error");

		auto iter = mMarcoSymbolMap.find(token.str);

		MarcoSymbol*  marco = nullptr;
		if( iter != mMarcoSymbolMap.end() )
		{
			if( !bCanRedefineMarco )
			{
				SYNTAX_ERROR("Define Syntex Error");
			}
			marco = &iter->second;
		}
		else
		{
			auto iterInsert = mMarcoSymbolMap.insert(std::make_pair(token.str, MarcoSymbol()));
			marco = &iterInsert.first->second;
			marco->name = token.str;
		}

		input.skipSpace();
		TokenString expr = ParseUtility::StringTokenLine(input.mCur);


	}

	std::map< TokenString, Command, Preprocessor::StrCmp > Preprocessor::sCommandMap;

	Command Preprocessor::nextCommand( CodeInput& input, bool bOutString , char const*& comStart )
	{
		char const*& str = input.mCur;

		char const* start = str;
		char const* end = str;
		Command result = Command::None;
		for( ;;)
		{
			str = ParseUtility::SkipChar(str, " \r");
			if( *str == 0 )
			{
				end = str;
				break;
			}

			if( *str != '#' )
			{
				str = ParseUtility::FindChar(str, '\n');
				if( *str != 0 )
				{
					++str;
				}
			}
			else
			{
				comStart = str;
				end = str;
				TokenString strCom;
				//#TODO skip space char
				if( !isalpha(*(str + 1)) )
				{

				}
				strCom.ptr = str + 1;
				str = ParseUtility::FindChar(str, " \r\n");
				strCom.num = str - strCom.ptr;

				auto iter = sCommandMap.find(strCom);
				if( iter != sCommandMap.end() )
				{
					result = iter->second;
					end = comStart;
					break;
				}
			}

			if( *str == 0 )
			{
				end = str;
				break;
			}
		}

		if( bOutString )
		{
			if ( start != end )
			{
				TokenString tokenOut;
				tokenOut.ptr = start;
				tokenOut.num = end - start;
				mOutput->push(tokenOut);
			}
		}
		return result;
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

	Preprocessor::ExprToken Preprocessor::nextExprToken(CodeInput& input)
	{
		//TODO : Skip Comment
		TokenString str;
		if( !ParseUtility::StringToken(input.mCur, mExprDelimsTable, str) )
			return ExprToken(ExprToken::eEof);

		if( isalpha(str[0]) )
			return ExprToken(str);

		if( '0' <= str[0] && str[0] <= '9' )
		{
			int numEval = 0;
			int value = ParseUtility::ParseIntNumber(str.ptr, numEval);
			if ( numEval != str.num )
				SYNTAX_ERROR("Unknow Token");

			return ExprToken(ExprToken::eNumber, value);
		}

		if( !mExprDelimsTable.isStopDelims(str[0]) )
			SYNTAX_ERROR("Unknow Token");

		int op = -1;
		switch( str[0] )
		{
		case '!':
			if( str[1] == '=' )
			{
				++input.mCur;
				op = OP_EQ;
			}
			else
			{
				op = str[0];
			}
			break;
		case '>':
			if( str[1] == '=' )
			{
				++input.mCur;
				op = OP_BEQ;
			}
			else
			{
				op = str[0];
			}
			break;
		case '<':
			if( str[1] == '=' )
			{
				++input.mCur;
				op = OP_SEQ;
			}
			else
			{
				op = str[0];
			}
			break;
		case '=':
			if( str[1] == '=' )
			{
				++input.mCur;
				op = OP_EQ;
			}
			else
			{
				op = str[0];
			}
			break;
		}

		return ExprToken(ExprToken::eOP, op);
	}

	bool Preprocessor::eval_Atom(ExprToken token , CodeInput& input)
	{
		if( token.type == ExprToken::eNumber )
			return token.value;
		else if( token.type == ExprToken::eString )
		{
			MarcoSymbol* marco = findMarco(token.string);
			if( marco == nullptr )
				SYNTAX_ERROR("Unknow Marco");
			return marco->cacheEvalValue;
		}
		else if( token.type == ExprToken::eOP )
		{
			if( token.value == '(' )
			{
				//int value = execExpression(input);
				//token = nextExprToken(input);
				//if( token.type != ExprToken::eOP || token.value != ')' )
				//	SYNTAX_ERROR("Missing \")\"");
				//return value;
			}	
		}

		SYNTAX_ERROR("Unknow Expr Node");
	}

	void Preprocessor::translate(CodeInput& input)
	{
		bool done = false;
		do
		{
			char const* comStart;
			Command com = nextCommand( input , evalNeedOutput() , comStart );
			switch( com )
			{
			case Command::None:
				done = true;
				break;
			case Command::Include:
				FUNCTION_CHECK(execInclude(input);)
				break;
			case Command::Pragma:
				{
					TokenInfo token = nextToken(input);

					if( token.type != Token_String )
						SYNTAX_ERROR("Error");

					if( token.str == "once" )
					{
						if( !input.mFileName.empty() )
							mParamOnceSet.insert(input.mFileName);
					}
					else
					{
						SYNTAX_ERROR("Unknown Param Command");
					}

					input.skipLine();
				}
				break;
			case Command::Define:
				FUNCTION_CHECK(execDefine(input));
				break;

			case Command::If:
			case Command::Ifdef:
			case Command::Ifndef:
				{
					if ( mStateStack.empty() || mStateStack.back().bEval == true )
					{
						++mScopeCount;
						State state;
						state.bEval = false;
						mStateStack.push_back(state);
					}
				}
				break;
			case Command::Endif:
				{
					--mScopeCount;
					if( mStateStack.empty() )
						SYNTAX_ERROR("");
					mStateStack.pop_back();
				}
				break;
			case Command::Elif:
				{
					if( mStateStack.empty() )
						SYNTAX_ERROR("Error Command");

					if( mStateStack.back().bHaveElse )
						SYNTAX_ERROR("Error Command");

					if( mStateStack.back().bEval )
					{
						mStateStack.back().bEval = false;
						//skip
					}
					else
					{


					}
				}
			case Command::Else:
				{
					if ( mStateStack.empty() )
						SYNTAX_ERROR("Error Command");

					mStateStack.back().bHaveElse = true;

					if( mStateStack.back().bEval )
					{
						mStateStack.back().bEval = false;
						//skip
					}
					else
					{

					}
				}
			default:
				SYNTAX_ERROR("Error Command");
			}
		}
		while( !done );

		if ( !mStateStack.empty() )
			SYNTAX_ERROR("");

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

	bool CodeInput::loadFile(char const* path)
	{
		if( !FileUtility::LoadToBuffer(path , mBuffer , true) )
			return false;
		reset();
		return true;
	}

	

}//namespace CPP
