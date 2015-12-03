#ifndef CFTypeDef_h__
#define CFTypeDef_h__

namespace CFly
{
	enum PrimitiveType
	{
		CFPT_UNKNOWN          = 0,
		CFPT_POINTLIST        = 1,
		CFPT_LINELIST         = 2,
		CFPT_LINESTRIP        = 3,
		CFPT_TRIANGLELIST     = 4,
		CFPT_TRIANGLESTRIP    = 5,
		CFPT_TRIANGLEFAN      = 6,
	};

	enum FilterMode
	{
		CF_FILTER_NONE = 0,
		CF_FILTER_POINT  , 
		CF_FILTER_LINEAR  ,
		CF_FILTER_ANISOTROPIC  , 
		CF_FILTER_FLAT_CUBIC ,
		CF_FILTER_GAUSSIAN_CUBIC , 

		CF_FILTER_MODE_NUM ,
	};

	enum TextureFormat
	{
		CF_TEX_FMT_ARGB32 ,
		CF_TEX_FMT_RGB32  ,
		CF_TEX_FMT_RGB24  ,
		CF_TEX_FMT_R5G6B5 ,
		CF_TEX_FMT_R32F   ,


		CF_TEX_FMT_DEFAULT ,
	};

	enum LightingComponent
	{
		CFLC_AMBIENT = 0,
		CFLC_DIFFUSE ,
		CFLC_SPECLAR ,
		CFLC_EMISSIVE ,
	};

	enum UsageColorType
	{
		CFMC_MATERIAL  = 0 ,
		CFMC_VERTEX_C1 = 1 ,
		CFMC_VERTEX_C2 = 2 ,
	};

	enum TexAddressMode
	{
		CFAM_WRAP   ,
		CFAM_MIRROR ,
		CFAM_CLAMP  ,
		CFAM_BORDER ,
		CFAM_MIRROR_ONCE ,
	};


}//namespace CFly

#endif // CFTypeDef_h__
