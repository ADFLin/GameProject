#ifndef GIFLoader_h__
#define GIFLoader_h__

#include "DataStructure/Array.h"
#include <iosfwd>

struct COLOR {unsigned char b,g,r,x;};	// Windows GDI expects 4bytes per color
#define GIF_DEFAULT_ALIGN sizeof(int)				// Windows GDI expects all int-aligned

struct GIFImageInfo
{
	int    frameWidth;
	int    frameHeight;

	int    imgWidth;
	int    imgHeight;
	unsigned char*  raster;      // Bits of Raster Data (Byte Aligned)
	COLOR* palette;
	int    transparent; // Index of Transparent color (-1 for none)

	int    bytesPerRow; // Width (in bytes) including alignment!
	int    bpp;         // Bits Per Pixel
	// Extra members for animations:
	int    transparency; // Animation Transparency.
	int    imgPosX;      // Relative Position
	int    imgPosY;
	int    delay;        // Delay after image in 1/1000 seconds.

};

class GIFLoader
{
public:
	GIFLoader();
	class Listener
	{
	public:
		virtual ~Listener(){}
		virtual bool loadImage( GIFImageInfo const& info ){ return true; }
		virtual void errorMsg( char const* msg ){}
	};
	void setImageRowAlign( int align ){ mImgRowAlign = align; }
	int  load( char const* path , Listener& listener );
	int  load( std::istream& fs , Listener& listener );
private:
	int    mImgRowAlign;
	typedef TArray< unsigned char > Buffer;
	Buffer mCompressedBuffer;
	Buffer mRasterBuffer;
	TArray< COLOR > globalColorMap;
	TArray< COLOR > localColorMap;
};

typedef bool (*GIFLoadCallback )( GIFImageInfo const& info , void* userData );
int LoadGIFImage( char const* path , GIFLoadCallback callback , void* userData /*= NULL */, int imgRowlign = GIF_DEFAULT_ALIGN  );

#endif // GIFLoader_h__
