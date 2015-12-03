#ifndef CFShuFa_h__
#define CFShuFa_h__

#include "CFBase.h"

namespace CFly
{
	class ShuFa
	{
	public:
		ShuFa( D3DDevice* d3dDevice , char const* fontName ,int size , bool beBold= false , bool beItalic = false );
		int  write( char* str , int ox , int oy , Color4ub const& color , bool beClip = false , int length = 1024 );

		int  getCharWidth() const { return mWidth; }
		int  getCharHeight() const { return mHeight; }

		void begin();
		void end();

		static void setTextDepth( float depth );
	private:
		LPD3DXFONT   mD3DFont;
		int  mWidth;
		int  mHeight;
	};

}//namespace CFly
#endif // CFShuFa_h__