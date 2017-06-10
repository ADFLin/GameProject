#include "GIFLoader.h"

#include <fstream>
#include <cstring>

// ****************************************************************************
//
// WINIMAGE.CPP : Generic classes for raster images (MSWindows specialization)
//
//  Content: Member definitions for:
//  - class C_Image             : Storage class for single images
//  - class C_ImageSet          : Storage class for sets of images
//  - class C_AnimationWindow   : Window Class to display animations
//
//  (Includes routines to Load and Save BMP files and to load GIF files into
// these classes).
//
//  --------------------------------------------------------------------------
//
// Copyright ?2000, Juan Soulie <jsoulie@cplusplus.com>
//
// Permission to use, copy, modify, distribute and sell this software or any
// part thereof and/or its documentation for any purpose is granted without fee
// provided that the above copyright notice and this permission notice appear
// in all copies.
//
// This software is provided "as is" without express or implied warranty of
// any kind. The author shall have no liability with respect to the
// infringement of copyrights or patents that any modification to the content
// of this file or this file itself may incur.
//
// ****************************************************************************

// pre-declaration:
int LZWDecoder ( unsigned char*, unsigned char*, short, int, int, int, const int);

#define ERRORMSG( MSG ) {}

using namespace std;

#ifdef _WIN32
#pragma pack(1)
#endif
struct GIFGCEtag 
{				
	unsigned char BlockSize;		// Block Size: 4 bytes
	unsigned char PackedFields;		// Packed Fields. Bits detail:
	//    0: Transparent Color Flag
	//    1: User Input Flag
	//  2-4: Disposal Method
	unsigned short Delay;			// Delay Time (1/100 seconds)
	unsigned char  Transparent;		// Transparent Color Index
};

struct GIFLSDtag 
{
	unsigned short ScreenWidth;		// Logical Screen Width
	unsigned short ScreenHeight;	// Logical Screen Height
	unsigned char PackedFields;		
	// Packed Fields. Bits detail:
	//  0-2: Size of Global Color Table
	//    3: Sort Flag
	//  4-6: Color Resolution
	//    7: Global Color Table Flag
	unsigned char Background;		// Background Color Index
	unsigned char PixelAspectRatio;	// Pixel Aspect Ratio
};

struct GIFIDtag 
{	
	unsigned short xPos;			// Image Left Position
	unsigned short yPos;			// Image Top Position
	unsigned short Width;			// Image Width
	unsigned short Height;			// Image Height
	unsigned char  PackedFields;		
	// Packed Fields. Bits detail:
	//  0-2: Size of Local Color Table
	//  3-4: (Reserved)
	//    5: Sort Flag
	//    6: Interlace Flag
	//    7: Local Color Table Flag
};
#ifdef _WIN32
#pragma pack()
#endif

GIFLoader::GIFLoader()
{
	mImgRowAlign = GIF_DEFAULT_ALIGN;
}

// ****************************************************************************
// * LoadGIF                                                                  *
// *   Load a GIF File into the C_ImageSet object                             *
// *                        (c) Nov 2000, Juan Soulie <jsoulie@cplusplus.com> *
// ****************************************************************************
int GIFLoader::load( char const* path , Listener& listener )
{
	// OPEN FILE
	std::ifstream fs( path , ios::in|ios::binary );

	if (!fs.is_open()) 
	{
		ERRORMSG("File not found");
		return 0;
	}
	return load( fs , listener );
}

int GIFLoader::load( std::istream& fs , Listener& listener )
{
	// *1* READ HEADER (SIGNATURE + VERSION)
	char szSignature[6];				// First 6 bytes (GIF87a or GIF89a)
	fs.read(szSignature,6);

	if ( memcmp(szSignature,"GIF",2) != 0)
	{ 
		ERRORMSG("Not a GIF File"); 
		return 0; 
	}

	// *2* READ LOGICAL SCREEN DESCRIPTOR

	GIFLSDtag  giflsd;

	fs.read((char*)&giflsd,sizeof(giflsd));

	// Global GIF variables:
	int globalBPP = (giflsd.PackedFields & 0x07) + 1;

	GIFImageInfo info;
	// fill some animation data:
	info.frameWidth  = giflsd.ScreenWidth;
	info.frameHeight = giflsd.ScreenHeight;

	// *3* READ/GENERATE GLOBAL COLOR MAP
	int num =  1 << globalBPP;
	globalColorMap.resize( num );
	if (giflsd.PackedFields & 0x80)	// File has global color map?
	{
		for ( int i=0;i < num; i++ )
		{
			globalColorMap[i].r = fs.get();
			globalColorMap[i].g = fs.get();
			globalColorMap[i].b = fs.get();
		}
	}
	else	// GIF standard says to provide an internal default Palette:
	{
		globalColorMap.resize( 256 );
		for (int i=0;i<256;i++)
			globalColorMap[i].r = globalColorMap[i].g = globalColorMap[i].b = i;
	}

	// *4* NOW WE HAVE 3 POSSIBILITIES:
	//  4a) Get and Extension Block (Blocks with additional information)
	//  4b) Get an Image Separator (Introductor to an image)
	//  4c) Get the trailer Char (End of GIF File)



	// GRAPHIC CONTROL EXTENSION
	int graphicExtensionFound = 0;
	GIFGCEtag gifgce;

	int const GIF_EXTENSION_BOCK  = 0x21;
	int const GIF_IMAGE_SEPARATOR = 0x2c;

	int nImages = 0;

	do
	{
		int charGot = fs.get();

		if (charGot == 0x21)		// *A* EXTENSION BLOCK 
		{
			switch (fs.get())
			{
			case 0xF9:			// Graphic Control Extension
				{
					fs.read((char*)&gifgce,sizeof(gifgce));
					graphicExtensionFound++;
					fs.get(); // Block Terminator (always 0)
				}
				break;

			case 0xFE:			// Comment Extension: Ignored
			case 0x01:			// PlainText Extension: Ignored
			case 0xFF:			// Application Extension: Ignored
			default:			// Unknown Extension: Ignored
				// read (and ignore) data sub-blocks
				while (int nBlockLength = fs.get())
				{
					for (int i=0; i < nBlockLength; i++ ) 
						fs.get();
				}
				break;
			}
		}
		else if (charGot == 0x2c) // *B* IMAGE (0x2c Image Separator)
		{	

			// Read Image Descriptor
			GIFIDtag gifid;
			fs.read((char*)&gifid, sizeof(gifid));

			bool isLocalColorMap = (gifid.PackedFields & 0x08 ) != 0;

			info.imgWidth  = gifid.Width;
			info.imgHeight = gifid.Height;
			info.imgPosX = gifid.xPos;
			info.imgPosY = gifid.yPos;

			info.bpp = isLocalColorMap ? ( gifid.PackedFields & 7 ) + 1 : globalBPP;

			{
				info.bytesPerRow = info.imgWidth;
				// Animation Extra members setup:

				if ( info.bpp == 24 )
					info.bytesPerRow *= 3;

				info.bytesPerRow += ( mImgRowAlign - info.imgWidth % mImgRowAlign ) % mImgRowAlign;	// Align BytesPerRow
			}

			// Fill NextImage Data
			if ( graphicExtensionFound )
			{
				info.transparent  = ( gifgce.PackedFields & 0x01 ) ? gifgce.Transparent : -1;
				info.transparency = ( gifgce.PackedFields & 0x1c ) > 1 ? 1 : 0;
				info.delay = gifgce.Delay * 10;
			}
			else
			{
				info.transparent = -1;
				info.transparency = 0;
				info.delay = 0;
			}

			if (isLocalColorMap)		// Read Color Map (if descriptor says so)
			{
				int num = 1 << info.bpp;
				localColorMap.resize( num );
				fs.read( (char*)&localColorMap[0] , sizeof(COLOR) * num );

				info.palette = &localColorMap[0];
			}
			else					// Otherwise copy Global
			{
				info.palette = &globalColorMap[0];
			}

			short firstbyte = fs.get();	// 1st byte of img block (CodeSize)

			// Calculate compressed image block size
			// to fix: this allocates an extra byte per block
			long ImgStart,ImgEnd;				
			ImgEnd = ImgStart = fs.tellg();
			while ( int n = fs.get() ) 
			{
				fs.seekg (ImgEnd += n+1);
			}
			fs.seekg (ImgStart);

			// Allocate Space for Compressed Image
			mCompressedBuffer.resize( ImgEnd - ImgStart+4 );
			// Read and store Compressed Image
			char * pTemp = (char*)&mCompressedBuffer[0];
			while (int nBlockLength = fs.get())
			{
				fs.read(pTemp,nBlockLength);
				pTemp+=nBlockLength;
			}

			mRasterBuffer.resize( info.bytesPerRow * info.imgHeight );
			info.raster = &mRasterBuffer[0];
			// Call LZW/GIF decompressor
			int n= LZWDecoder(
				&mCompressedBuffer[0] ,
				&mRasterBuffer[0] ,
				firstbyte, info.bytesPerRow ,//NextImage->AlignedWidth,
				gifid.Width, gifid.Height,
				(gifid.PackedFields & 0x40) ? 1 : 0	//Interlaced?
				);

			if (n)
			{
				++nImages;
				if ( !listener.loadImage( info ) )
					break;
			}
			else
			{
				ERRORMSG("GIF File Corrupt");
			}

			// Some cleanup
			graphicExtensionFound = 0;
		}
		else if (charGot == 0x3b) 
		{	// *C* TRAILER: End of GIF Info
			break; // Ok. Standard End.
		}

	} 
	while ( fs.good() );

	if (nImages==0) 
	{
		ERRORMSG("Premature End Of File");
	}
	return nImages;
}

// ****************************************************************************
// * LZWDecoder (C/C++)                                                       *
// * Codec to perform LZW (GIF Variant) decompression.                        *
// *                         (c) Nov2000, Juan Soulie <jsoulie@cplusplus.com> *
// ****************************************************************************
//
// Parameter description:
//  - bufIn: Input buffer containing a "de-blocked" GIF/LZW compressed image.
//  - bufOut: Output buffer where result will be stored.
//  - InitCodeSize: Initial CodeSize to be Used
//    (GIF files include this as the first byte in a picture block)
//  - AlignedWidth : Width of a row in memory (including alignment if needed)
//  - Width, Height: Physical dimensions of image.
//  - Interlace: 1 for Interlaced GIFs.
//
int LZWDecoder ( unsigned char * bufIn, unsigned char * bufOut,
				short initCodeSize, int AlignedWidth,
				int width, int height, const int Interlace)
{
	int  const MAX_TABLE_NUM = 4096;
	int  const maxPixels = width * height;
		
	// Set up values that depend on InitCodeSize Parameter.
	short const clearCode = (1 << initCodeSize); // Clear code : resets decompressor
	short const endCode    = clearCode + 1;         // End code : marks end of information
	short const firstEntry = clearCode + 2; // Index of first free entry in table
	

	short codeSize   = initCodeSize +1;      // Current CodeSize (size in bits of codes)
	short nextEntry  = firstEntry;    // Index of next free entry in table
	int   nPixels = 0; // Output pixel counter
	int   rowOffset =0; // Offset in output buffer for current row
	int   row=0;
	int   col=0;				// used to point output if Interlaced
	long  whichBit =0;					// Index of next bit in bufIn
	short prevCode;					// Previous Code

	// Translation Table:
	short prefix[ MAX_TABLE_NUM ];				// Prefix: index of another Code
	unsigned char suffix[ MAX_TABLE_NUM ];		// Suffix: terminating character
	unsigned char outStack[ MAX_TABLE_NUM + 1 ];// Output buffer	

	while (nPixels<maxPixels) 
	{
	
		// Characters in OutStack
		int outIndex = 0;

		// GET NEXT CODE FROM bufIn:
		// LZW compression uses code items longer than a single byte.
		// For GIF Files, code sizes are variable between 9 and 12 bits 
		// That's why we must read data (Code) this way:
		long LongCode=*((long*)(bufIn+whichBit/8));	// Get some bytes from bufIn
		LongCode>>=(whichBit&7);				    // Discard too low bits

		// Code extracted
		short Code =(LongCode & ((1<<codeSize)-1) );	// Discard too high bits
		whichBit += codeSize;					// Increase Bit Offset

		// SWITCH, DIFFERENT POSIBILITIES FOR CODE:
		if (Code == endCode)					// END CODE
			break;								// Exit LZW Decompression loop

		// CLEAR CODE:
		if (Code == clearCode)
		{				
			codeSize = initCodeSize+1;			// Reset CodeSize
			nextEntry = firstEntry;				// Reset Translation Table
			prevCode = Code;				    // Prevent next to be added to table.
			continue;							// restart, to get another code
		}

		short outCode;					       // Code to output
		if (Code < nextEntry)					// CODE IS IN TABLE
		{
			outCode = Code;						// Set code to output.
		}
		else 
		{									// CODE IS NOT IN TABLE:
			outIndex++;			// Keep "first" character of previous output.
			outCode = prevCode;					// Set PrevCode to be output
		}

		// EXPAND OutCode IN OutStack
		// - Elements up to FirstEntry are Raw-Codes and are not expanded
		// - Table Prefices contain indexes to other codes
		// - Table Suffices contain the raw codes to be output

		while (outCode >= firstEntry) 
		{
			if (outIndex > MAX_TABLE_NUM ) 
				return 0;

			outStack[outIndex++] = suffix[outCode];	// Add suffix to Output Stack
			outCode = prefix[outCode];				// Loop with preffix
		}

		// NOW OutCode IS A RAW CODE, ADD IT TO OUTPUT STACK.
		if ( outIndex > MAX_TABLE_NUM ) 
			return 0;

		outStack[outIndex++] = (unsigned char) outCode;

		// ADD NEW ENTRY TO TABLE (PrevCode + OutCode)
		// (EXCEPT IF PREVIOUS CODE WAS A CLEARCODE)
		if (prevCode!=clearCode) 
		{
			prefix[nextEntry] = prevCode;
			suffix[nextEntry] = (unsigned char) outCode;
			nextEntry++;

			// Prevent Translation table overflow:
			if (nextEntry >= MAX_TABLE_NUM ) 
				return 0;
      
			// INCREASE CodeSize IF NextEntry IS INVALID WITH CURRENT CodeSize
			if (nextEntry >= (1<<codeSize)) 
			{
				if (codeSize < 12) codeSize++;
				else {}				// Do nothing. Maybe next is Clear Code.
			}
		}

		prevCode = Code;

		// Avoid the possibility of overflow on 'bufOut'.
		if (nPixels + outIndex > maxPixels) 
			outIndex = maxPixels - nPixels;

		// OUTPUT OutStack (LAST-IN FIRST-OUT ORDER)
		for ( int n = outIndex-1; n >= 0; n-- ) 
		{
			if ( col == width )						// Check if new row.
			{
				if ( Interlace ) 
				{
					if      ((row&7)==0) {row+=8; if (row>=height) row=4;}
					else if ((row&3)==0) {row+=8; if (row>=height) row=2;}
					else if ((row&1)==0) {row+=4; if (row>=height) row=1;}
					else row+=2;
				}
				else							// If not interlaced:
				{
					row++;
				}

				rowOffset = row * AlignedWidth;		// Set new row offset
				col = 0;
			}
			bufOut[ rowOffset + col ] = outStack[n];	// Write output
			col++;	nPixels++;					// Increase counters.
		}

	}	// while (main decompressor loop)

	return whichBit;
}

class MyListener : public GIFLoader::Listener
{
public:
	virtual bool loadImage( GIFImageInfo const& info )
	{ 
		return callback( info , userData ); 
	}

	void*           userData;
	GIFLoadCallback callback;
};

static GIFLoader gLoader;
int LoadGIFImage( char const* path , GIFLoadCallback callback , void* userData /*= NULL */, int imgRowlign /*= GIF_DEFAULT_ALIGN */ )
{
	gLoader.setImageRowAlign( imgRowlign );
	MyListener listener;

	listener.callback = callback;
	listener.userData = userData;

	return gLoader.load( path , listener );
}

// Refer to WINIMAGE.TXT for copyright and patent notices on GIF and LZW.
