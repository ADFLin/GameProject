#include "TFileIO.h"


bool TFileIO::load( char const* fileName )
{
	fs.open( fileName , std::ios::in );
	if ( !fs.good() )
		return false;

	return true;
}

bool TFileIO::save( char const* fileName )
{
	fs.open( fileName , std::ios::out );
	if ( !fs.good() )
		return false;
	return true;
}

void TFileIO::goToString( char const* s )
{
	do 
	{
		fs.getline( buf , sizeof( buf ) );
		fs >> str;
	} while ( str != s );
}

void TFileIO::batchElement( int num )
{
	for (int i=0;i<num;++i)
	{
		fs >> str;
	}
}
