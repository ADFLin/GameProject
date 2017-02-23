#include "CPreprocessor.h"

#include "CommonMarco.h"
#include "FileSystem.h"
#include "THolder.h"

#define SYNTAL_ERROR( MSG ) throw SyntalError( MSG );
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
		if( sCommandMap.empty() )
		{
			for( int i = 0; i < ARRAY_SIZE(gCommandList); ++i )
			{
				sCommandMap.insert(std::make_pair(TokenString(gCommandList[i].name), gCommandList[i].com));
			}
		}
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

	void Preprocessor::execInclude(CodeInput& input)
	{
		TokenInfo token = nextToken(input);
		if( token.type != Token_String )
			SYNTAL_ERROR("Error include Format");

		if( token.str.num < 2 )
			SYNTAL_ERROR("Error include Format");

		bool bSystemPath = false;
		if( token.str[1] == '\"' )
		{
			if( token.str[token.str.num - 1] != '\"' )
				SYNTAL_ERROR("Error include Format");
		}
		else if( token.str[0] == '<' )
		{
			if( token.str[token.str.num - 1] != '>' )
				SYNTAL_ERROR("Error include Format");

			bSystemPath = true;
		}

		TokenString path;
		path.ptr = token.str.ptr + 1;
		path.num = token.str.num - 2;

		if( mParamOnceSet.find(path.toStdString()) == mParamOnceSet.end() )
		{
			std::string fullPath;
			if ( !findFile(path.toStdString() , fullPath ) )
				SYNTAL_ERROR("Can't find include file");

			TPtrHolder< CodeInput > includeInput( new CodeInput );
			if( !includeInput->loadFile(fullPath.c_str()) )
			{
				SYNTAL_ERROR("Can't open include file");
			}
			includeInput->mFileName = path.toStdString();
			includeInput->reset();
			translate(*includeInput);
		}

		input.skipLine();
	}

	void Preprocessor::execIf(CodeInput& input)
	{
		std::string expr = getExpression(input);
		int evalValue;
		if( !evalExpression(expr, evalValue) )
			SYNTAL_ERROR("Error expression");

		if( evalValue != 0 )
		{
			translate(input);
		}
		else
		{

		}
	}

	std::map< TokenString, Command, Preprocessor::StrCmp > Preprocessor::sCommandMap;

	CPP::Command Preprocessor::nextCommand( CodeInput& input, bool bOutString , char const*& comStart )
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
		if( FileSystem::isExist(name.c_str()) )
		{
			fullPath = name;
			return true;
		}
		for( int i = 0; i < mFileSreachDirs.size(); ++i )
		{
			fullPath = mFileSreachDirs[i] + name;
			if( FileSystem::isExist(fullPath.c_str()) )
				return true;
		}
		return false;
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
						SYNTAL_ERROR("Error");

					if( token.str == "once" )
					{
						if( !input.mFileName.empty() )
							mParamOnceSet.insert(input.mFileName);
					}
					else
					{
						SYNTAL_ERROR("Unknown Param Command");
					}

					input.skipLine();
				}
				break;
			case Command::Define:
				{

				}
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
						SYNTAL_ERROR("");
					mStateStack.pop_back();
				}
				break;
			case Command::Elif:
				{
					if( mStateStack.empty() )
						SYNTAL_ERROR("Error Command");

					if( mStateStack.back().bHaveElse )
						SYNTAL_ERROR("Error Command");

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
						SYNTAL_ERROR("Error Command");

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
				SYNTAL_ERROR("Error Command");
			}
		}
		while( !done );

		if ( !mStateStack.empty() )
			SYNTAL_ERROR("");

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
		if( !FileUtility::LoadToBuffer(path , mBuffer ) )
			return false;
		reset();
		return true;
	}

	

}//namespace CPP
