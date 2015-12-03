#include "CFlyPCH.h"
#include "CFFileLoader.h"

namespace CFly
{


	int BinaryStream::readCString( char* buf , int maxSize )
	{
		int num = 0;
		int check = maxSize - 1;

		bool beEnd = false;
		while( num < check )
		{
			int c = mFS.get();
			if ( c == '\0' || c == EOF )
			{
				beEnd = true;
				break;
			}
			buf[ num ] = (char)c;
			++num;
		}
		buf[ num ] = '\0';
		if ( !beEnd )
		{
			while( mFS.good() )
			{
				int c = mFS.get();
				if ( c == '\0' )
					break;
			}
		}
		return num;
	}

	int BinaryStream::getLength()
	{
		std::ios::pos_type cur = mFS.tellg();

		mFS.seekg( 0 , std::ios::end );
		std::ios::pos_type e = mFS.tellg();

		mFS.seekg( cur );
		return e;
	}

}//namespace CFly