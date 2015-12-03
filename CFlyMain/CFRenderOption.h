#ifndef CFRenderOption_h__
#define CFRenderOption_h__

namespace CFly
{
	enum RenderOption
	{	
		CFRO_LIGHTING = 0,
		CFRO_FOG ,
		CFRO_ALPHA_BLENGING ,
		CFRO_Z_BUFFER_WRITE ,
		CFRO_Z_TEST ,
		CFRO_ZBIAS ,
		CFRO_ALPHA_TEST ,
		CFRO_SRC_BLEND ,
		CFRO_DEST_BLEND ,
		CFRO_CULL_FACE ,

		CFRO_OPTION_NUM ,
		CFRO_ALL_OPTION_BIT = 0xffffffff, 
	};

	enum RenderMode
	{
		CFRM_TEXTURE ,
		CFRM_NO_TEXTURE ,
		CFRM_WIREFRAME ,
		CFRM_POINT ,
	};

	enum BlendMode
	{
		CF_BLEND_ZERO ,
		CF_BLEND_ONE ,
		CF_BLEND_SRC_COLOR ,
		CF_BLEND_INV_SRC_COLOR ,
		CF_BLEND_SRC_ALPHA ,
		CF_BLEND_INV_SRC_ALPHA ,
		CF_BLEND_DEST_ALPHA ,
		CF_BLEND_INV_DEST_ALPHA ,
		CF_BLEND_DEST_COLOR ,
		CF_BLEND_INV_DESTCOLOR ,

		CF_BLEND_MODE_NUM ,
	};

	enum CullFace
	{
		CF_CULL_NONE ,
		CF_CULL_CCW ,
		CF_CULL_CW ,
	};

}//namespace CFly

#endif // CFRenderOption_h__ 