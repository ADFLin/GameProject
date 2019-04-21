#include "rcPCH.h"
#include "SgxFileLoader.h"

#include <fstream>
#include <cassert>

#include "Serialize/StreamBuffer.h"
#include "FileSystem.h"

int const PIXEL_OFFSET = 4;

typedef TStreamBuffer<> SreamBuffer;

static uint32 Decode555(uint16 color) 
{
	uint32 rgb = 0xff000000;
	// Red
	rgb |= ((color & 0x7c00) << 9) | ((color & 0x7000) << 4);
	// Green
	rgb |= ((color & 0x3e0) << 6) | ((color & 0x300));
	// Blue
	rgb |= ((color & 0x1f) << 3) | ((color & 0x1c) >> 2);

	return rgb;
}


static inline void FillB8G8R8( char* buff , uint16 color )
{
	buff[2] = ((color & 0x7c00) >> 7 ) | ((color & 0x7000) >> 12 );
	buff[1] = ((color & 0x3e0) >> 2 ) | ( (color & 0x300) >> 8 );
	buff[0] = ((color & 0x1f) << 3) | ((color & 0x1c) >> 2);
}

void ReadTitleBitmap( SreamBuffer& buffer , SgxImageInfo const& bmpInfo , char* buf )
{
	uint32 offset;

	offset = PIXEL_OFFSET * ( bmpInfo.width - 2 );
	for ( int16 len = 2 ; len <= tileWidth; len += 4 )
	{
		char* temp = buf ;
		for( uint16 i = len ; i ; --i )
		{
			uint16 c;
			buffer.take( c );
			FillB8G8R8( temp , c );	
			temp += PIXEL_OFFSET;
		}
		buf += offset;
	}

	buf += PIXEL_OFFSET * 2;

	offset = PIXEL_OFFSET * ( bmpInfo.width + 2 );
	for ( int16 len = tileWidth ; len >= 2 ; len -= 4 )
	{
		char* temp = buf ;
		for( uint16 i = len ; i ; --i )
		{
			uint16 c;
			buffer.take( c );
			FillB8G8R8( temp , c );	
			temp += PIXEL_OFFSET;
		}
		buf += offset;
	}
}

void ReadCompressedBitmap( SreamBuffer& buffer , int len , char* buf )
{
	while( len > 0 )
	{
		uint8 n;
		buffer.take( n );

		if ( n != 255 )
		{
			len -= n;
			for( ; n ; --n )
			{
				uint16 c;
				buffer.take( c );
				FillB8G8R8( buf , c );
				buf += PIXEL_OFFSET;
			}
		}
		else
		{
			uint8 skip;
			buffer.take( skip );

			buf += PIXEL_OFFSET * skip;
			len -= skip;
		}
	}
}


void readIsometricBitmap( SreamBuffer& buffer , SgxImageInfo const& bmpInfo , char* buf )
{
	uint16 size = bmpInfo.width / tileWidth;

	assert( bmpInfo.width == size * ( tileWidth + 2 ) - 2  );

	uint16 jMax = 2 * size - 1;

	char* temp = buf + PIXEL_OFFSET * ( bmpInfo.width / 2 + bmpInfo.width * ( bmpInfo.height - size * tileHeight  ) - 1 );

	for ( uint16  j = 0 ; j < jMax  ; ++j  )
	{
		uint16 iMax = ( j < size ) ? j + 1 : jMax - j;

		char* tileBuf = temp - PIXEL_OFFSET * ( (iMax - 1)*( tileWidth + 2 )/ 2  );

		for(  uint16 i = 0 ; i < iMax ; ++i  )
		{
			ReadTitleBitmap( buffer , bmpInfo , tileBuf );
			tileBuf += PIXEL_OFFSET * (  tileWidth + 2  ) ;
		}

		temp += PIXEL_OFFSET * ( bmpInfo.width *( tileHeight / 2 ) ) ;
	}

	if ( bmpInfo.sizeTotal != bmpInfo.sizeCompressed )
	{
		int length = bmpInfo.width * ( bmpInfo.height - ( bmpInfo.width + 2 )/ 4 - 1 );
		ReadCompressedBitmap( buffer , length , buf );
	}
}

void ReadPlainBitmap( SreamBuffer& buffer , SgxImageInfo const& bmpInfo , char* buf )
{
	uint16 const maskColor = 0xf81f;
	unsigned len = bmpInfo.width * bmpInfo.height;
	for ( ; len ; --len )
	{
		uint16 c;
		buffer.take( c );
		if ( c != maskColor )
			FillB8G8R8( buf , c );
		buf += PIXEL_OFFSET;
	}
}

bool SgxFileLoader::loadFormFile( char const* path )
{
	using std::ios;
	using std::ifstream;

	std::ifstream fs( path , ios::binary );

	if ( !fs )
		return false;

	fs.seekg( 0 , ios::beg );
	unsigned size = fs.tellg();
	fs.seekg( 0 , ios::end );
	size = (unsigned) fs.tellg() - size;

	mInfoData = new uint8[ size ];

	fs.seekg( ios::beg );
	fs.read( (char*)mInfoData  , size  );

	SgxHeader* header = getHeader();

	mImageGroupInfo = (SgxImageGroupInfo*)( mInfoData + sizeof( SgxHeader ) );
	mImageInfo      = (SgxImageInfo*)( mImageGroupInfo + SG2_MAX_IMG_GROUP_NUM ) + 1;


	fs.close();
	char extraPath[512];

	strcpy( extraPath , path );
	char* dotPos = strrchr( extraPath , '.' );
	strcpy( dotPos , ".555" );

	mInternalData = new uint8[ header->fileSizeInternal ];
	fs.open( extraPath , ios::binary );
	fs.read( (char*)mInternalData , header->fileSizeInternal );


	return true;
}


SgxFileLoader::SgxFileLoader()
{
	mInfoData = NULL;
	mImageGroupInfo = NULL;
	mImageInfo = NULL;
}

bool SgxFileLoader::readBitmap( int idx , char* buf )
{
	SgxImageInfo const*      imageInfo = getImageInfo( idx );
	SgxImageGroupInfo const* groupInfo = getImageGroupInfo( imageInfo->imgID );
	char path[512];

	uint8* data;
	switch( imageInfo->flag )
	{
	case 0: 
		data = mInternalData; 
		break;
	case 1:
		return false;
		groupInfo->groupName;
		data -= 1;
		break;
	default:
		assert( 0 );
		return false;
	}

	data += imageInfo->offset;

	SreamBuffer buffer( (char*) data , imageInfo->sizeTotal );

	switch( imageInfo->type )
	{
	case 0: case 1: case 10: case 12: case 13:
		ReadPlainBitmap( buffer , *imageInfo , buf ); 
		break;
	case 256: case 257: case 276:
		ReadCompressedBitmap( buffer , imageInfo->width * imageInfo->height , buf ); 
		break;
	case 30:
		readIsometricBitmap( buffer , *imageInfo , buf ); 
		break;
	}
	return true;
}

SgxFileLoader::~SgxFileLoader()
{
	delete [] mInfoData;
	delete [] mInternalData;
}
