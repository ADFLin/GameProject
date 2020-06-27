#ifndef ZFont_h__
#define ZFont_h__

#include "ZBase.h"

#include <map>
#include <fstream>
#include "ZResManager.h"


namespace Zuma
{
	class IRenderSystem;
	class ZFontLoader;
	class ITexture2D;

	struct ZCharData
	{
		int    width;
		Vector2  offset;
		Vector2  rectPos;
		Vector2  rectSize;

		ZCharData()
			:width(0)
			,offset(0,0)
			,rectPos(0,0)
			,rectSize(0,0)
		{

		}

	};

	struct ZFontLayer
	{
		std::string imagePath;
		std::string alhpaPath;
		int         pointSize;
		ZCharData   data[256];
	};

	enum
	{
		FF_ALIGN_LEFT    = 0 ,
		FF_ALIGN_HCENTER = 1 ,
		FF_ALIGN_RIGHT   = 2 ,

		FF_ALIGN_TOP     = 0 << 2 ,
		FF_ALIGN_VCENTER = 1 << 2 ,
		FF_ALIGN_BOTTOM  = 2 << 2 ,

		FF_LOCAL_POS     = BIT(6),

	};

	class ZFont : public ResBase
	{
	public:
		ZFont( ZFontLoader& loader );

		virtual void release();

		int  calcStringWidth( char const* str );
		int  print( Vector2 const& pos ,char const* str , unsigned flag = 0 );

	protected:
		static int const MAX_FONTLAYER_NUM = 6;
		int          mNumLayer;
		ZFontLayer*  mFontLayer[ MAX_FONTLAYER_NUM ];
		ITexture2D*  mTex[ MAX_FONTLAYER_NUM ];
	};


	class ZFontLoader
	{
	public:

		ZFontLoader();

		bool load( char const* path );
		int  getFontLayer( ZFontLayer* layer[] );
	
	
	private:
		typedef std::ios::pos_type StreamPos;
		typedef std::ifstream Stream;

		typedef std::map< std::string , StreamPos >    DefineMap;
		typedef std::map< std::string , ZFontLayer* >  LayerMap;

		LayerMap    layerMap;
		DefineMap   defineMap;
	
		std::vector< unsigned char > charList;
		StreamPos  charListPos;

		

		bool analyzeFont( Stream& stream );

		static void skipToChar( Stream& stream , char ch );
		static char getToken( Stream& stream , char* buf );

		bool checkDefineMap( Stream& stream , StreamPos& pos );
		
		void loadCharList( Stream& stream );
		void loadFontSetCharMap( Stream& stream );

		
		void loadLayerSetCharWidths ( Stream& stream , ZFontLayer* layer );
		void loadLayerSetImageMap   ( Stream& stream , ZFontLayer* layer );
		void loadLayerSetPointSize  ( Stream& stream , ZFontLayer* layer );
		void loadLayerSetCharOffsets( Stream& stream , ZFontLayer* layer );
		void loadLayerSetImage      ( Stream& stream , ZFontLayer* layer );

	public:
		typedef void (ZFontLoader::*LoadFuncc)( Stream& , ZFontLayer* );
	};

}//namespace Zuma


#endif // ZFont_h__
