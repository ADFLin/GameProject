#include "TFlyConsoleSystem.h"


#include "debug.h"

void TFlyConsoleSystem::doCommand()
{
	if ( inputStr.size() == 0 )
		return;

	if ( command( inputStr.c_str() ) )
	{
		saveStr[ pevStrIndex ] = inputStr;
		pevStrIndex = ( pevStrIndex + 1 ) % 10 ;
		saveStrIndex = pevStrIndex;
		Msg( inputStr.c_str() );
	}
	else
	{
		Msg( m_errorMsg.c_str() );
	}

	inputStr.clear();
}

void TFlyConsoleSystem::pushChar( char c )
{
	inputStr.push_back( c ); 
	findStrNum = findCommandName2( inputStr.c_str() , &findStr[0] , 10 );
	findStrIndex = 0;
}

void TFlyConsoleSystem::popChar()
{
	if ( inputStr.empty() )
		return;

	inputStr.erase( inputStr.size() - 1 , 1 );
	findStrNum = findCommandName2( inputStr.c_str() , &findStr[0] , 10 );
	findStrIndex = 0;
}

void TFlyConsoleSystem::setFindCommand( int offset )
{
	if ( findStrNum == 0 )
		return;

	findStrIndex = ( findStrIndex + findStrNum ) % findStrNum;
	inputStr = TString( findStr[findStrIndex] ) + " ";
	findStrIndex += offset;
}

void TFlyConsoleSystem::setSaveString( int offset )
{
	
	saveStrIndex += offset;
	saveStrIndex = ( saveStrIndex + 10 ) % 10;
	inputStr = saveStr[ saveStrIndex  ];

	//DevMsg( 5 , "saveStrIndex = %d" , saveStrIndex );
}