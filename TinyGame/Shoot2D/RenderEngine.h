#ifndef RenderEngine_H
#define RenderEngine_H

#include "CommonFwd.h"
#include "Win32Header.h"
#include "BitmapDC.h"


namespace Shoot2D
{
	class BmpData;
	class RenderEngine
	{
	public:
		RenderEngine( HDC hDC );
		void beginRender();

		void endRender();

		void SetDrawOrg(int cx,int cy)
		{
			Orgx=cx;Orgy=cy;
		}

		void drawText( int x ,int y, char* str )
		{
			SetTextColor( mhDC , RGB(255,255,0) );
			TextOut( mhDC , x , y , str , strlen(str) );
		}

		void draw(BmpData& data , int x ,int y , bool invDir);
		void draw(Object* obj);
		void draw( unsigned id );

	protected:
		int    Orgx,Orgy;
		HDC    mhDC;
	};

}//namespace Shoot2D

#endif //RenderEngine_H