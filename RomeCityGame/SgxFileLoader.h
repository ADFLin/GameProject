#ifndef SgxFileLoader_h__
#define SgxFileLoader_h__

#include <string>

typedef unsigned int   uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8;
typedef int            int32;
typedef short          int16;

//The header of an SG file is always exactly 680 bytes long. 
//I only know the purpose of the first 40 bytes of the header.
//OFFSET  TYPE  DESCRIPTION
//0    uint  For some SG's, the filesize, for others, it's a fixed value
//4    uint  Version number, always 0xd3 for C3's sg2 files
//8    uint  (unknown)
//12    int   Maximum number of image records in this file
//16    int   Number of image records in use
//20    int   Number of bitmap records in use (maximum is 100 for SG2, 200 for SG3
//24    int   Unknown: appears to be the number of bitmap records minus one
//28    uint  Total filesize of all loaded graphics (.555 files)
//32    uint  Filesize of the .555 file that "belongs" to this .sg2: the one with the same name
//36    uint  Filesize of any images pulled from external .555 files


#define SG2_MAX_IMG_GROUP_NUM 100
#define SG3_MAX_USE_IMG 200


struct SgxHeader
{
	uint32 fileSize;
	uint32 version;
	uint32 unknown1;
	int32  maxNumImage;
	int32  numImage;
	int32  numImgGroup;
	int32  unknown2;
	uint32 fileSizeTotal;
	uint32 fileSizeInternal;
	uint32 s52;

	uint8   reserve[680 - 40 ];
};


//OFFSET  TYPE    DESCRIPTION
//0    string  65 bytes, the filename of the .bmp file. This translates to the
//name of the .555 file, if it's external.
//65    string  51 bytes, comment for this bitmap record
//116    int     Width of some image in the bitmap (don't pay much attention to this)
//120    int     Height of some image in the bitmap (idem)
//124    uint    Number of images in this bitmap
//128    uint    Index of the first image of this bitmap
//132    uint    Index of the last image of this bitmap
struct SgxImageGroupInfo
{
	char   groupName[65];
	char   comment[51];
	int32  width;
	int32  height;
	uint32 numImage;
	uint32 indexFirst;
	uint32 indexLast;

	uint8   reserve[200 - 136];
};


//OFFSET  TYPE  DESCRIPTION
//0    uint  Offset into the .555 file. Of note: if it's an external .555 file,
//you have to subtract one from this offset!
//4    uint  Length of the image data (total)
//8    uint  Length of the compressed image data
//12    uint  (unknown, zero bytes)
//16    uint  Invert offset. For sprites (walkers), the image of the walker
//moving to the left is the mirrored version of the image of the
//walker going right. If this is set, copy the image at the index
//(current - invert offset) and mirror it vertically.
//20   ushort Width of the image
//22   ushort Height of the image
//24    byte* 26 unknown bytes. At least the first 4 bytes are 2 shorts
//50   ushort Type of image. See below.
//52    byte  Flag: 0 = use internal .555, 1 = use external .555 (defined in bitmap)
//53    byte* 3 bytes, 3 unknown flags
//56    byte  Bitmap ID (not always filled in correctly for SG3 files)
//57    byte* 7 bytes, unknown
//
//-- For sg3 files with alpha masks: --
//64    uint  Offset of alpha mask
//68    uint  Length of alpha mask data

struct SgxImageInfo
{
	uint32 offset;
	uint32 sizeTotal;
	uint32 sizeCompressed;
	uint32 unknown1;
	uint32 invOffset;
	uint16 width;
	uint16 height;
	
	uint16 xUnknown;
	uint16 yUnknown;
	uint8   unknown2[2];
	uint16 numAnimImage;
	uint16 unknown3;
	uint16 xAnimOffset;
	uint16 yAnimOffset;
	uint8   unknown4[26 - 14];
	uint16 type;
	uint8   flag;
	uint8   unknownFlag[3];
	uint16 imgID;
	uint8   reserve[6];
};

uint16 const tileWidth = 58;
uint16 const tileHeight = 30;

class SgxFileLoader
{
public:
	SgxFileLoader();
	~SgxFileLoader();
	bool loadFormFile( char const* path );
	bool readBitmap( int idx , char* buf );

	SgxHeader*  getHeader()  {  return (SgxHeader*)( mInfoData );  }
	SgxImageGroupInfo const* getImageGroupInfo( int idx ){  return mImageGroupInfo + idx;  }
	SgxImageInfo const*      getImageInfo( int idx )     {  return mImageInfo + idx;  }

	void releaseInternalData()
	{
		delete [] mInternalData;
		mInternalData = NULL;
	}
private:
	std::string         mFilePath;
	SgxImageGroupInfo*  mImageGroupInfo;
	SgxImageInfo*       mImageInfo;

	uint8*               mInternalData;
	uint8*               mInfoData;
};
#endif // SgxFileLoader_h__