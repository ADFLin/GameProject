#include "CPreprocessor.h"

#include "CommonMarco.h"
#include <fstream>

#define SYNTAL_ERROR( MSG ) SyntalError( MSG );

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


	Translator::Translator()
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

	CPP::TokenInfo Translator::nextToken(SourceInput& input)
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

	void Translator::execInclude(SourceInput& input)
	{
		TokenInfo token = nextToken(input);
		if( token.type != Token_String )
			throw SYNTAL_ERROR("Error include Format");

		if( token.str.num < 2 )
			throw SYNTAL_ERROR("Error include Format");

		bool bSystemPath = false;
		if( token.str[1] == '\"' )
		{
			if( token.str[token.str.num - 1] != '\"' )
				throw SYNTAL_ERROR("Error include Format");
		}
		else if( token.str[0] == '<' )
		{
			if( token.str[token.str.num - 1] != '>' )
				throw SYNTAL_ERROR("Error include Format");

			bSystemPath = true;
		}

		TokenString path;
		path.ptr = token.str.ptr + 1;
		path.num = token.str.num - 2;

		if( mParamOnceSet.find(path.toStdString()) == mParamOnceSet.end() )
		{
			SourceInput* includeInput = new SourceInput;
			//#TODO
			std::string fullPath = "Shader/";
			fullPath += path.toStdString().c_str();
			if( !includeInput->loadFile(fullPath.c_str()) )
			{
				delete includeInput;
				throw SYNTAL_ERROR("Can't open include file");
			}
			includeInput->mFileName = path.toStdString();
			includeInput->reset();
			parse(*includeInput);
			delete includeInput;
		}

		input.skipLine();
	}

	void Translator::execIf(SourceInput& input)
	{
		std::string expr = getExpression(input);
		int evalValue;
		if( !evalExpression(expr, evalValue) )
			SYNTAL_ERROR("Error expression");

		if( evalValue != 0 )
		{
			parse(input);
		}
		else
		{

		}
	}

	std::map< TokenString, Command, Translator::StrCmp > Translator::sCommandMap;

	CPP::Command Translator::nextCommand( SourceInput& input, bool bOutString , char const*& comStart )
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

	void Translator::parse(SourceInput& input)
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
				execInclude(input);
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
				{
					++mScopeCount;
					State state;
					state.bEval = false;
					mStateStack.push_back(state);
				}
				break;
			case Command::Ifdef:
				{
					++mScopeCount;
					State state;
					state.bEval = false;
					mStateStack.push_back(state);
				}
				break;
			case Command::Ifndef:
				{
					++mScopeCount;
					State state;
					state.bEval = false;
					mStateStack.push_back(state);
				}
				break;


			case Command::Endif:
				{
					--mScopeCount;
					mStateStack.pop_back();
				}
				break;
			case Command::Elif:
			case Command::Else:
			default:
				SYNTAL_ERROR("Error Command");
			}
		}
		while( !done );
	}

	bool SourceInput::loadFile(char const* path)
	{
		std::ifstream fs(path, std::ios::binary);
		if( !fs.is_open() )
			return false;
		fs.seekg(0, fs.end);
		std::ios::pos_type size = fs.tellg();
		mBuffer.resize(size);
		fs.seekg(0, fs.beg);
		fs.read((char*)&mBuffer[0], size);
		fs.close();
		mBuffer.push_back(0);
		return true;
	}

	

}//namespace CPP
