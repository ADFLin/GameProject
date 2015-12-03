#include "CFlyPCH.h"
#include "CFShuFa.h"

namespace CFly
{
	static LPD3DXSPRITE gTextSprite = NULL;
	static float        gTextDepth = 0;

	void ShuFa::setTextDepth( float depth )
	{
		gTextDepth = depth;
	}

	ShuFa::ShuFa( D3DDevice* d3dDevice , char const* fontName,int size , bool beBold/*= FALSE*/, bool beItalic /*= FALSE */ )
	{
		if ( gTextSprite == NULL )
		{
			D3DXCreateSprite( d3dDevice , &gTextSprite );
		}

		D3DXFONT_DESC fontDesc = 
		{
			size ,
			0,
			beBold ? 700 : 400 ,
			0,
			beItalic ,
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS ,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_PITCH,
		};
		strcpy( fontDesc.FaceName , fontName );
		::D3DXCreateFontIndirect( d3dDevice , &fontDesc ,&mD3DFont );

		TEXTMETRIC metric;
		mD3DFont->GetTextMetrics( &metric );

		mWidth  = metric.tmAveCharWidth;
		mHeight = size;

	}


	int ShuFa::write( char* str , int ox , int oy , Color4ub const& color , bool beClip /*= FALSE*/, int length /*= 1024*/ )
	{
		RECT rect;
		rect.left = ox;
		rect.top  = oy;
		rect.right =  ox + length;
		rect.bottom = oy + 100;

		DWORD flag = DT_LEFT | DT_TOP | DT_SINGLELINE;
		if ( ! beClip )
			flag |= DT_NOCLIP; 
		return mD3DFont->DrawText(
			gTextSprite , str , -1, &rect , flag ,
			color.toARGB() );
	}

	void ShuFa::begin()
	{
		gTextSprite->Begin( D3DXSPRITE_OBJECTSPACE | D3DXSPRITE_ALPHABLEND	| D3DXSPRITE_DONOTSAVESTATE );
		D3DXMATRIX matrix;
		D3DXMatrixIdentity( &matrix );
		//sTextSprite->SetTransform(&matrix);
		D3DXMatrixTranslation( &matrix, 0.0f, 0.0f, gTextDepth );

		D3DDevice* device;
		gTextSprite->GetDevice( &device );
		device->SetTransform( D3DTS_WORLD , &matrix );
		device->SetSamplerState( 0 , D3DSAMP_MAGFILTER , D3DTEXF_POINT );
		device->SetSamplerState( 0 , D3DSAMP_MINFILTER , D3DTEXF_POINT );
		
	}

	void ShuFa::end()
	{
		gTextSprite->End();
	}

} //namespace CFly
