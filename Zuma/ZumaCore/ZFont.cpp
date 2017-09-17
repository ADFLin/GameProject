#include "ZumaPCH.h"

#include "ZFont.h"
#include "ZResManager.h"
#include "IRenderSystem.h"

namespace Zuma
{
	static bool sbInitFunMap = false;

	typedef std::map< std::string , ZFontLoader::LoadFun > LayerFunMap;
	static LayerFunMap sLayerFunMap;

	ZFontLoader::ZFontLoader()
	{
		if ( !sbInitFunMap )
		{
			sLayerFunMap["LayerSetCharWidths"]  = &ZFontLoader::loadLayerSetCharWidths;
			sLayerFunMap["LayerSetImageMap"]    = &ZFontLoader::loadLayerSetImageMap;
			sLayerFunMap["LayerSetCharOffsets"] = &ZFontLoader::loadLayerSetCharOffsets;
			sLayerFunMap["LayerSetImage" ]      = &ZFontLoader::loadLayerSetImage;
			sLayerFunMap["LayerSetPointSize" ]  = &ZFontLoader::loadLayerSetPointSize;
			//layerFunMap["LayerSetImage" ]      = &ZFontLoader::loadLayerSetImage;
			//layerFunMap["LayerSetImage" ]      = &ZFontLoader::loadLayerSetImage;
			//layerFunMap["LayerSetImage" ]      = &ZFontLoader::loadLayerSetImage;
			//layerFunMap["LayerSetImage" ]      = &ZFontLoader::loadLayerSetImage;
			sbInitFunMap = true;
		}
	}

	bool ZFontLoader::load( char const* path )
	{
		Stream stream;
		stream.open( path , std::ios::binary );
		if ( !stream.is_open() )
			return false;
		return analyzeFont( stream );
	}

	void ZFontLoader::skipToChar( Stream& stream , char ch )
	{
		bool inStr = false;
		int  numParentheses = 0;
		char strChar = 0;
		char c;

		while( stream )
		{
			stream >> c;

			if ( c == '\'' || c =='\"' )
			{
				if ( strChar )
				{
					if ( strChar == c )
						strChar = 0;
				}
				else
				{
					strChar = c;
				}
			}

			//skip string
			if ( strChar )
				continue;

			if ( c == ch && numParentheses == 0 )
				return;
			else if ( c == '(')
				++numParentheses;
			else if ( c == ')')
				--numParentheses;
		}
	}

	char ZFontLoader::getToken( Stream& stream , char* buf )
	{
		char c;
		char strChar = 0;

		stream >> c;

		while( stream )
		{
			if ( strChar )
			{
				if ( strChar == c )
				{
					*buf = '\0';
					return c;
				}
				else
				{
					*(buf++) = c;
				}
			}
			else
			{
				switch( c )
				{
				case '\'': case '\"':
					assert( strChar == 0 );
					strChar = c;
					break;
				case '(': case ',': case ')':case ' ':
					*buf = '\0';
					return c;
				case ';':
					stream.seekg( -1 , std::ios::cur );
					*buf = '\0';
					return c;
				default:
					*(buf++) = c;
				}
			}

			stream.get( c );
		}

		return '\0';
	}

	void ZFontLoader::loadCharList( Stream& stream )
	{
		char c;
		char token[256];

		StreamPos valuePos;
		bool useDeifine = checkDefineMap( stream , valuePos );

		if ( charListPos == valuePos )
			return;

		charListPos = valuePos;

		StreamPos oldPos = stream.tellg();
		stream.seekg( valuePos );

		c = getToken( stream , token );
		assert( c == '(');

		charList.clear();

		while( stream )
		{
			c = getToken( stream , token );

			if ( c == ')')
				break;

			switch ( c )
			{
			case '(':
			case ',':
				break;
			default:
				charList.push_back( (unsigned char)token[0] );
				break;
			}
		}
		if ( useDeifine )
			stream.seekg( oldPos );
	}

	void ZFontLoader::loadLayerSetCharWidths( Stream& stream , ZFontLayer* layer )
	{
		char c ;
		char token[256];

		loadCharList( stream );

		StreamPos valuePos;
		bool useDeifine  = checkDefineMap( stream , valuePos );

		StreamPos oldPos = stream.tellg();

		stream.seekg( valuePos );

		c = getToken( stream , token );
		assert( c == '(');

		int idx = 0;

		while( stream && idx < charList.size() )
		{
			c = getToken( stream , token );

			ZCharData& data = layer->data[ charList[idx++] ];

			data.width = atoi( token );

			if ( c == ')')
				break;
		}

		if ( useDeifine )
			stream.seekg( oldPos );
	}

	void ZFontLoader::loadLayerSetImageMap( Stream& stream , ZFontLayer* layer )
	{
		char c ;
		char token[256];

		loadCharList( stream );

		StreamPos valuePos;
		bool useDefine = checkDefineMap( stream , valuePos );

		StreamPos oldPos = stream.tellg();

		stream.seekg( valuePos );

		c = getToken( stream , token );
		assert( c == '(');

		int idx = 0;
		while( stream && idx < charList.size() )
		{
			c = getToken( stream , token );

			ZCharData& data = layer->data[ charList[idx++] ];

			assert( c == '(');
			c = getToken( stream , token );
			data.rectPos.x = atoi( token );
			c = getToken( stream , token );
			data.rectPos.y = atoi( token );
			c = getToken( stream , token );
			data.rectSize.x= atoi( token );
			c = getToken( stream , token );
			data.rectSize.y = atoi( token );

			c = getToken( stream , token );

			if ( c == ')')
				break;

			assert( c == ',' );
		}
		if ( useDefine )
			stream.seekg( oldPos );
	}

	void ZFontLoader::loadLayerSetCharOffsets( Stream& stream , ZFontLayer* layer )
	{
		char c ;
		char token[256];

		loadCharList( stream );
		StreamPos valuePos;
		bool useDefine = checkDefineMap( stream , valuePos );

		StreamPos oldPos = stream.tellg();

		stream.seekg( valuePos );

		c = getToken( stream , token );
		assert( c == '(');

		int idx = 0;
		while( stream && idx < charList.size() )
		{
			c = getToken( stream , token );

			ZCharData& data = layer->data[ charList[idx++] ];

			assert( c == '(');
			c = getToken( stream , token );
			data.offset.x = atoi( token );
			c = getToken( stream , token );
			data.offset.y = atoi( token );

			c = getToken( stream , token );

			if ( c == ')')
				break;

			assert( c == ',' );
		}
		if ( useDefine )
			stream.seekg( oldPos );
	}

	void ZFontLoader::loadLayerSetImage( Stream& stream , ZFontLayer* layer )
	{
		char token[64];
		getToken( stream , token );

		layer->imagePath = Global::getResManager().getWorkDir();
		layer->imagePath += "fonts/";
		layer->imagePath += token;

		if ( isFileExist( layer->imagePath.c_str() , ".gif") )
			layer->imagePath += ".gif";
		else
			layer->imagePath.clear();

		layer->alhpaPath = Global::getResManager().getWorkDir();
		layer->alhpaPath += "fonts/_";
		layer->alhpaPath += token;
		layer->alhpaPath += ".gif";

	}

	bool ZFontLoader::checkDefineMap( Stream& stream , StreamPos& pos )
	{
		char token[256];
		pos = stream.tellg();
		if ( getToken( stream , token ) != '(' )
		{
			pos = defineMap[ token ];
			return true;
		}
		return false;
	}

	void ZFontLoader::loadLayerSetPointSize( Stream& stream , ZFontLayer* layer )
	{
		char token[256];
		getToken( stream , token );
		layer->pointSize = atoi( token );
	}

	bool ZFontLoader::analyzeFont( Stream& stream )
	{
		char c;
		char token[256];

		while ( stream )
		{
			stream >> token;
			if ( strcmp( token , "Define" ) == 0 )
			{
				stream >> token;
				StreamPos pos = stream.tellg();
				defineMap.insert( std::make_pair( token , pos ) );
			}
			else if ( strcmp( token , "CreateLayer" ) == 0 )
			{
				getToken( stream , token );
				layerMap[ token ] = new ZFontLayer;
			}
			else if ( strncmp( token ,  "Layer" , 5 ) == 0 )
			{
				std::string layerFun = token;
				getToken( stream , token );

				ZFontLayer* layer = layerMap[ token ];

				if ( layer != NULL )
				{
					LoadFun fun = sLayerFunMap[ layerFun ];
					if ( fun )
						(this->*fun)( stream , layer );
				}
			}
			else if ( strcmp( token ,  "SetCharMap" ) == 0 )
			{
				loadFontSetCharMap( stream );
			}

			skipToChar( stream , ';' );
		}

		return true;
	}

	void ZFontLoader::loadFontSetCharMap( Stream& stream )
	{
		char c;
		char token[256];

		loadCharList( stream );

		StreamPos valuePos;
		bool useDefine = checkDefineMap( stream , valuePos );

		StreamPos oldPos = stream.tellg();

		stream.seekg( valuePos );

		c = getToken( stream , token );
		assert( c == '(');

		int idx = 0;
		while( stream && idx < charList.size() )
		{
			c = getToken( stream , token );

			for ( LayerMap::iterator iter = layerMap.begin();
				iter != layerMap.end() ; ++iter )
			{
				ZFontLayer* layer = iter->second;
				if ( layer == NULL )
					continue;

				ZCharData& data = layer->data[ charList[idx] ];
				data.offset   = layer->data[ token[0] ].offset;
				data.rectPos  = layer->data[ token[0] ].rectPos;
				data.rectSize = layer->data[ token[0] ].rectSize;
				data.width    = layer->data[ token[0] ].width;
			}

			++idx;

			c = getToken( stream , token );

			if ( c == ')')
				break;

			assert( c == ',' );
		}
		if ( useDefine )
			stream.seekg( oldPos );
	}

	int ZFontLoader::getFontLayer( ZFontLayer* layer[] )
	{
		int count = layerMap.size();
		int num = 0;
		for( LayerMap::iterator iter = layerMap.begin();
			iter != layerMap.end() ; ++iter )
		{
			layer[ num++ ] = iter->second;
		}
		return num;
	}

	ZFont::ZFont( ZFontLoader& loader )
		:ResBase( RES_FONT )
	{
		IRenderSystem& renderSys = Global::getRenderSystem();

		mNumLayer = loader.getFontLayer( mFontLayer );

		assert( mNumLayer <= MAX_FONTLAYER_NUM );

		for( int i = 0 ; i < mNumLayer ; ++i )
		{
			mTex[i] = renderSys.createFontTexture( *mFontLayer[i] );
		}
	}

	void ZFont::release()
	{
		for ( int i = 0 ; i < mNumLayer ; ++i )
		{
			delete mTex[i];
			delete mFontLayer[i];

			mTex[i]       = NULL;
			mFontLayer[i] = NULL;
		}
	}

	int ZFont::print( Vec2f const& pos ,char const* str , unsigned flag )
	{
		IRenderSystem& renderSys = Global::getRenderSystem();

		if ( *str == '\0' )
			return 0;

		if ( !( flag & FF_LOCAL_POS ) )
			renderSys.setWorldIdentity();

		renderSys.translateWorld( pos );

		if ( flag & ( FF_ALIGN_RIGHT | FF_ALIGN_HCENTER ) )
		{
			float w = ZFont::calcStringWidth( str );

			if ( flag & FF_ALIGN_HCENTER )
				w *= 0.5f;
			renderSys.translateWorld( -w , 0 );
		}

		if ( flag & ( FF_ALIGN_BOTTOM | FF_ALIGN_VCENTER ) )
		{
			ZCharData& data = mFontLayer[0]->data[ *str ];

			float h = data.rectSize.y;

			if ( flag & FF_ALIGN_VCENTER )
				h *= 0.5f;

			renderSys.translateWorld( 0 , -h );
		}

		int  width = 0;
		char c;
		while ( ( c = *str ) != '\0' )
		{
			ZCharData& data = mFontLayer[0]->data[ c ];

			renderSys.drawBitmap( *mTex[0] , data.rectPos , data.rectSize );

			renderSys.translateWorld( data.width /*+ data.offset.x */,  0/*data.offset.y*/  );

			width += data.width;
			++str;
		}

		return width;
	}

	int ZFont::calcStringWidth( char const* str )
	{
		int  width = 0;
		char c;
		while ( ( c = *str ) != '\0' )
		{
			ZCharData& data = mFontLayer[0]->data[ c ];
			width += data.width;
			++str;
		}

		return width;
	}

}//namespace Zuma

